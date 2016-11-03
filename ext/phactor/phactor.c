/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_phactor.h"
#include "debug.h"
#include "ext/standard/php_rand.h"
#include "ext/standard/php_var.h"
#include "main/php_main.h"
#include "zend_interfaces.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

#include "prepare.h"

ZEND_DECLARE_MODULE_GLOBALS(phactor)

#if COMPILE_DL_PTHREADS
	ZEND_TSRMLS_CACHE_DEFINE();
	ZEND_GET_MODULE(pthreads)
#endif

int thread_count;
thread_t main_thread;
pthread_mutex_t phactor_mutex;
pthread_mutex_t phactor_task_mutex;
pthread_mutex_t phactor_actors_mutex;
actor_system_t actor_system;
task_queue_t tasks;
int php_shutdown = 0;
int daemonise_actor_system = 0;
thread_t *worker_threads;
dtor_func_t (default_resource_dtor);
zend_object_handlers phactor_actor_handlers;
zend_object_handlers phactor_actor_system_handlers;
void ***phactor_instance = NULL;

zend_class_entry *ActorSystem_ce;
zend_class_entry *Actor_ce;

void *worker_function(thread_t *phactor_thread)
{
    phactor_thread->id = (ulong) pthread_self();
    phactor_thread->ls = ts_resource(0);
    // phactor_thread->thread = tsrm_thread_id();

    TSRMLS_CACHE_UPDATE();

    SG(server_context) = PHACTOR_SG(main_thread.ls, server_context);

    PG(expose_php) = 0;
    PG(auto_globals_jit) = 0;

    php_request_startup();

    initialise_worker_thread_environments(phactor_thread);

    while (1) {
        pthread_mutex_lock(&PHACTOR_G(phactor_task_mutex));

        if (PHACTOR_G(tasks).task == NULL) {
            if (PHACTOR_G(php_shutdown)) {
                pthread_mutex_unlock(&PHACTOR_G(phactor_task_mutex));
                break;
            }
            pthread_mutex_unlock(&PHACTOR_G(phactor_task_mutex));
            continue;
        }
        debug_tasks(PHACTOR_G(tasks));

        task_t *current_task = dequeue_task();

        pthread_mutex_unlock(&PHACTOR_G(phactor_task_mutex));

        switch (current_task->task_type) {
            case SEND_MESSAGE_TASK:
                send_message(current_task);
                break;
            case PROCESS_MESSAGE_TASK:
                process_message(current_task);
                break;
        }

        free(current_task);
    }

    pthreads_prepared_shutdown(phactor_thread);
    pthread_exit(NULL);
}

void process_message(task_t *task)
{
    mailbox_t *mail = task->task.pmt.for_actor->mailbox;
    actor_t *actor = task->task.pmt.for_actor;
    zval *return_value = malloc(sizeof(zval));
    zval *sender = malloc(sizeof(zval));

    ZVAL_OBJ(sender, &mail->from_actor->obj);
    actor->mailbox = actor->mailbox->next_message;

    // zend_call_user_method(actor->obj, return_value, sender, mail->message);
    // zval_ptr_dtor(mail->message);
    free(mail);

    // zval_ptr_dtor(sender);
    free(sender);

    // if (actor->return_value != NULL) {
        // free(actor->return_value); // tmp hack to fix mem leak
    // }
    // actor->return_value = return_value; // @todo memory leak
    free(return_value); // @todo remove this line (return the value instead? Or store it elsewhere?)
    // zval_ptr_dtor(mail->from_actor);
    // free(mail->message);
}

void send_message(task_t *task)
{
    if (task->task.smt.to_actor == NULL) {
        send_remote_message(task);
    } else {
        send_local_message(task);
    }
}

/*
Add the message (containing the from_actor and message) to the to_actor's mailbox.
Enqueue the to_actor as a new task to have its mailbox processed.
*/
void send_local_message(task_t *task)
{
    actor_t *actor = task->task.smt.to_actor;
    mailbox_t *current_message = actor->mailbox;

    if (current_message == NULL) {
        actor->mailbox = task->task.smt.message;
    } else {
        while (current_message->next_message != NULL) {
            current_message = current_message->next_message;
        }

        current_message->next_message = task->task.smt.message;
    }

    enqueue_task(create_process_message_task(actor));
}

void send_remote_message(task_t *task)
{
    // @todo debugging purposes only - no implementation yet
    printf("Tried to send a message to a non-existent (or remote) actor\n");
    assert(0);
}

task_t *create_send_message_task(zval *from_actor_zval, zval *to_actor_zval, zval *message)
{
    actor_t *to_actor = get_actor_from_zval(to_actor_zval);
    actor_t *from_actor = get_actor_from_zval(from_actor_zval);
    task_t *new_task = malloc(sizeof(task_t));

    new_task->task.smt.to_actor = to_actor;
    new_task->task.smt.message = create_new_message(from_actor, message);
    new_task->task_type = SEND_MESSAGE_TASK;
    new_task->next_task = NULL;

    return new_task;
}

mailbox_t *create_new_message(actor_t *from_actor, zval *message)
{
    mailbox_t *new_message = malloc(sizeof(mailbox_t));

    new_message->from_actor = from_actor;
    new_message->message = malloc(sizeof(zval));
    ZVAL_COPY(new_message->message, message); // @todo ZVAL_DUP instead?
    new_message->next_message = NULL;

    return new_message;
}

task_t *create_process_message_task(actor_t *actor)
{
    task_t *new_task = malloc(sizeof(task_t));

    new_task->task.pmt.for_actor = actor;
    new_task->task_type = PROCESS_MESSAGE_TASK;
    new_task->next_task = NULL;

    return new_task;
}

void enqueue_task(task_t *new_task)
{
    pthread_mutex_lock(&PHACTOR_G(phactor_task_mutex));
    task_t *current_task = PHACTOR_G(tasks).task;

    if (current_task == NULL) {
        PHACTOR_G(tasks).task = new_task;
        pthread_mutex_unlock(&PHACTOR_G(phactor_task_mutex));
        return;
    }

    while (current_task->next_task != NULL) {
        current_task = current_task->next_task;
    }

    current_task->next_task = new_task;
    pthread_mutex_unlock(&PHACTOR_G(phactor_task_mutex));
}

/* This function is only invoked in the scheduler, which already locks phactor_task_mutex */
static task_t *dequeue_task(void)
{
    task_t *task = PHACTOR_G(tasks).task;

    PHACTOR_G(tasks).task = task->next_task;

    return task;
}

zval* zend_call_user_method(zend_object object, zval *retval_ptr, zval *from_actor, zval *message)
{
    int result;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zend_class_entry *obj_ce;
    zend_function *receive_function;
    zend_string *receive_function_name;
    zval params[2];

    ZVAL_COPY_VALUE(&params[0], from_actor);
    ZVAL_COPY_VALUE(&params[1], message);

    fci.size = sizeof(fci);
    fci.object = &object;
    fci.retval = retval_ptr;
    fci.param_count = 2;
    fci.params = params;
    fci.no_separation = 1;

    receive_function_name = zend_string_init(ZEND_STRL("receive"), 0);
    receive_function = zend_hash_find_ptr(&object.ce->function_table, receive_function_name);

    fcc.initialized = 1;
	fcc.object = &object;
	fcc.calling_scope = object.ce;
	fcc.called_scope = object.ce;
	fcc.function_handler = receive_function;

    result = zend_call_function(&fci, &fcc);

    if (result == FAILURE) {
        /* error at c-level */
        obj_ce = object.ce;

        if (!EG(exception)) {
            zend_error_noreturn(
                E_CORE_ERROR,
                "Couldn't execute method %s%s%s",
                obj_ce ? ZSTR_VAL(obj_ce->name) : "",
                obj_ce ? "::" : "",
                receive_function_name);
        }
    }

    zend_string_free(receive_function_name);

    return retval_ptr;
}

void initialise_worker_thread_environments(thread_t *phactor_thread)
{
    pthread_mutex_lock(&PHACTOR_G(phactor_mutex));
    // taken from pthreads...
    pthreads_prepare_sapi(phactor_thread);

    pthreads_prepare_ini(phactor_thread);

    pthreads_prepare_constants(phactor_thread);

    pthreads_prepare_functions(phactor_thread);

    pthreads_prepare_classes(phactor_thread);

    pthreads_prepare_includes(phactor_thread);

    pthreads_prepare_exception_handler(phactor_thread);

    pthreads_prepare_resource_destructor(phactor_thread);

    pthread_mutex_unlock(&PHACTOR_G(phactor_mutex));
}

// @todo unused for now
zend_string *spl_object_hash(zend_object *obj)
{
    // @todo this is for when remote actors are introduced - not needed for now
    // this should generate a UUID instead...
    return strpprintf(20, "%s:%08d", "node_id_here", obj->handle);
}

// @todo unused for now - may not be needed...
zend_string *spl_zval_object_hash(zval *zval_obj)
{
    return spl_object_hash(Z_OBJ_P(zval_obj));
}

actor_t *get_actor_from_object(zend_object *actor_obj)
{
    pthread_mutex_lock(&PHACTOR_G(phactor_actors_mutex));
    actor_t *current_actor = PHACTOR_G(actor_system).actors;

    while (current_actor != NULL && current_actor->obj.handle != actor_obj->handle) {
        current_actor = current_actor->next;
    }

    pthread_mutex_unlock(&PHACTOR_G(phactor_actors_mutex));

    if (current_actor != NULL) {
        return current_actor;
    }

    // we did not find the actor locally, so it should be a remote actor then
    // this may be segregated into a new function, if remote actors are implemented as separate objects

    return NULL; // @todo remote actors have not been implemented yet

    // get actor hash (a UUID basically, so that actors are unique in the node cluster)

    // while (strncmp(current_actor->actor_ref->val, actor_object_ref->val, sizeof(char) * 32) != 0) {
    //     current_actor = current_actor->next;
    //
    //     if (current_actor == NULL) { // debugging only - will not work for remote actors
    //         return NULL;
    //     }
    // }

    // zend_string_free(actor_object_ref);

    // return current_actor;
}

actor_t *get_actor_from_zval(zval *actor_zval_obj)
{
    return get_actor_from_object(Z_OBJ_P(actor_zval_obj));
}

void add_new_actor(actor_t *new_actor)
{
    pthread_mutex_lock(&PHACTOR_G(phactor_actors_mutex));

    actor_t *current_actor = PHACTOR_G(actor_system).actors;

    if (current_actor == NULL) {
        PHACTOR_G(actor_system).actors = new_actor;
        pthread_mutex_unlock(&PHACTOR_G(phactor_actors_mutex));
        return;
    }

    while (current_actor->next != NULL) {
        current_actor = current_actor->next;
    }

    current_actor->next = new_actor;

    pthread_mutex_unlock(&PHACTOR_G(phactor_actors_mutex));
}

void initialise_actor_system()
{
    PHACTOR_G(tasks).task = NULL;
    PHACTOR_G(thread_count) = 1;//sysconf(_SC_NPROCESSORS_ONLN);

    PHACTOR_G(main_thread).id = (ulong) pthread_self();
    PHACTOR_G(main_thread).ls = TSRMLS_CACHE;
    // main_thread.thread = tsrm_thread_id();

    if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
        if (Z_TYPE_P(&EG(user_exception_handler)) == IS_OBJECT) {
    		rebuild_object_properties(Z_OBJ_P(&EG(user_exception_handler)));
    	} else if (Z_TYPE_P(&EG(user_exception_handler)) == IS_ARRAY) {
    		zval *object = zend_hash_index_find(Z_ARRVAL_P(&EG(user_exception_handler)), 0);
    		if (object && Z_TYPE_P(object) == IS_OBJECT) {
    			rebuild_object_properties(Z_OBJ_P(object));
    		}
    	}
    }

    PHACTOR_G(worker_threads) = malloc(sizeof(thread_t) * PHACTOR_G(thread_count));

    for (int i = 0; i < PHACTOR_G(thread_count); ++i) {
        pthread_mutex_init(&PHACTOR_G(worker_threads)[i].mutex, NULL);
        pthread_cond_init(&PHACTOR_G(worker_threads)[i].cond, NULL);
        pthread_create(&PHACTOR_G(worker_threads)[i].thread, NULL, (void *) worker_function, &PHACTOR_G(worker_threads)[i]);
    }
}

void remove_actor(actor_t *target_actor)
{
    pthread_mutex_lock(&phactor_actors_mutex);

    actor_t *current_actor = PHACTOR_G(actor_system).actors;

    if (current_actor == target_actor) {
        actor_system.actors = current_actor->next;
        // zend_object_std_dtor(&target_actor->obj);
        // free(target_actor->obj);
        // zend_string_free(target_actor->actor_ref);

        // if (target_actor->return_value != NULL) {
        //     free(target_actor->return_value); // tmp hack to fix mem leak
        // }

        // efree(target_actor);
        pthread_mutex_unlock(&phactor_actors_mutex);
        return;
    }

    while (current_actor != target_actor) {
        if (current_actor->next == target_actor) {
            current_actor->next = current_actor->next->next;
            // zend_object_std_dtor(&target_actor->obj);
            // free(target_actor->obj);
            // zend_string_free(target_actor->actor_ref);

            // if (target_actor->return_value != NULL) {
            //     free(target_actor->return_value); // tmp hack to fix mem leak
            // }

            // efree(target_actor);
            break;
        }

        current_actor = current_actor->next;
    }
    pthread_mutex_unlock(&phactor_actors_mutex);
}

void remove_actor_object(zval *actor)
{
    remove_actor(get_actor_from_zval(actor));
}

// @todo currently unused
void php_actor_free_object(zend_object *obj)
{
    actor_t *target_actor = get_actor_from_object(obj);

    if (target_actor != NULL) {
        remove_actor(target_actor);
        return;
    }

    // @debug purposes only - could be remote actor?
    printf("Tried to free a non-existent object\n");
}

// taken (and adapted) from zend_generators.c
// static void zend_restore_call_stack(actor_t *actor) /* {{{ */
// {
// 	zend_execute_data *call, *new_call, *prev_call = NULL;
//
// 	call = actor->state;
// 	do {
// 		new_call = zend_vm_stack_push_call_frame(
// 			(ZEND_CALL_INFO(call) & ~ZEND_CALL_ALLOCATED),
// 			call->func,
// 			ZEND_CALL_NUM_ARGS(call),
// 			(Z_TYPE(call->This) == IS_UNDEF) ?
// 				(zend_class_entry*)Z_OBJ(call->This) : NULL,
// 			(Z_TYPE(call->This) != IS_UNDEF) ?
// 				Z_OBJ(call->This) : NULL);
// 		memcpy(((zval*)new_call) + ZEND_CALL_FRAME_SLOT, ((zval*)call) + ZEND_CALL_FRAME_SLOT, ZEND_CALL_NUM_ARGS(call) * sizeof(zval));
// 		new_call->prev_execute_data = prev_call;
// 		prev_call = new_call;
//
// 		call = call->prev_execute_data;
// 	} while (call);
// 	actor->state->call = prev_call;
// 	efree(actor->state);
// 	actor->state = NULL;
// }
// /* }}} */
//
// // taken from zend_generators.c
// static zend_execute_data* zend_freeze_call_stack(zend_execute_data *execute_data) /* {{{ */
// {
// 	size_t used_stack;
// 	zend_execute_data *call, *new_call, *prev_call = NULL;
// 	zval *stack;
//
// 	/* calculate required stack size */
// 	used_stack = 0;
// 	call = EX(call);
// 	do {
// 		used_stack += ZEND_CALL_FRAME_SLOT + ZEND_CALL_NUM_ARGS(call);
// 		call = call->prev_execute_data;
// 	} while (call);
//
// 	stack = emalloc(used_stack * sizeof(zval));
//
// 	/* save stack, linking frames in reverse order */
// 	call = EX(call);
// 	do {
// 		size_t frame_size = ZEND_CALL_FRAME_SLOT + ZEND_CALL_NUM_ARGS(call);
//
// 		new_call = (zend_execute_data*)(stack + used_stack - frame_size);
// 		memcpy(new_call, call, frame_size * sizeof(zval));
// 		used_stack -= frame_size;
// 		new_call->prev_execute_data = prev_call;
// 		prev_call = new_call;
//
// 		new_call = call->prev_execute_data;
// 		zend_vm_stack_free_call_frame(call);
// 		call = new_call;
// 	} while (call);
//
// 	execute_data->call = NULL;
// 	ZEND_ASSERT(prev_call == (zend_execute_data*)stack);
//
// 	return prev_call;
// }
/* }}} */

// @todo currently impossible with current ZE
void receive_block(zval *actor_zval, zval *return_value)
{
    actor_t *actor = get_actor_from_zval(actor_zval);

    // zend_execute_data *execute_data = EG(current_execute_data);

    actor->blocking = 1;
    // actor->state = zend_freeze_call_stack(EG(current_execute_data));
    // actor->return_value = actor_zval; // tmp

    EG(current_execute_data) = NULL;

    // return PHACTOR_ZG(current_message_value); // not the solution...
}

/*
force shut down the actor system (used for daemonised actor systems)
No mutex is used since php_shutdown can only be set to 1 anyway...
*/
void force_shutdown_actor_system()
{
    PHACTOR_G(php_shutdown) = 1;
}

/*
No mutex since:
 - php_shutdown can only be set to 1 anyway
 - thread_count is not modified after being set (upon actor system startup)
 - worker_threads is not modified after being set (upon actor system startup)
*/
void scheduler_blocking()
{
    if (PHACTOR_G(daemonise_actor_system) == 0) {
        PHACTOR_G(php_shutdown) = 1;
    }

    while (1) {
        if (PHACTOR_G(php_shutdown) == 1) {
            break;
        }
    }

    for (int i = 0; i < PHACTOR_G(thread_count); i += sizeof(thread_t)) {
        pthread_join(PHACTOR_G(worker_threads)[i].thread, NULL);
    }

    free(worker_threads);
}



ZEND_BEGIN_ARG_INFO(ActorSystem_construct_arginfo, 0)
ZEND_ARG_INFO(0, flag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(ActorSystem_shutdown_arginfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(ActorSystem_block_arginfo, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(Actor_send_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, actor, Actor, 0)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Actor_remove_arginfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Actor_abstract_receive_arginfo, 0)
    ZEND_ARG_INFO(0, sender)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Actor_abstract_receiveblock_arginfo, 0)
ZEND_END_ARG_INFO()



/* {{{ proto string ActorSystem::__construct() */
PHP_METHOD(ActorSystem, __construct)
{
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &daemonise_actor_system) != SUCCESS) {
		return;
	}

    initialise_actor_system();
}
/* }}} */

/* {{{ proto void ActorSystem::shutdown() */
PHP_METHOD(ActorSystem, shutdown)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    force_shutdown_actor_system();
}
/* }}} */

/* {{{ proto void ActorSystem::block() */
PHP_METHOD(ActorSystem, block)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    scheduler_blocking();
}
/* }}} */


/* {{{ proto string Actor::send(Actor $actor, mixed $message) */
PHP_METHOD(Actor, send)
{
    zval *actor_zval;
    zval *message;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "oz", &actor_zval, &message) != SUCCESS) {
		return;
	}

    enqueue_task(create_send_message_task(getThis(), actor_zval, message));
}
/* }}} */

/* {{{ proto string Actor::remove() */
PHP_METHOD(Actor, remove)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    remove_actor_object(getThis());
}
/* }}} */

/* {{{ proto string Actor::receiveBlock() */
PHP_METHOD(Actor, receiveBlock)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    receive_block(getThis(), return_value);
}
/* }}} */



/* {{{ */
zend_function_entry ActorSystem_methods[] = {
    PHP_ME(ActorSystem, __construct, ActorSystem_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(ActorSystem, shutdown, ActorSystem_shutdown_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(ActorSystem, block, ActorSystem_block_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
zend_function_entry Actor_methods[] = {
	PHP_ME(Actor, send, Actor_send_arginfo, ZEND_ACC_PROTECTED)
    PHP_ME(Actor, remove, Actor_remove_arginfo, ZEND_ACC_PUBLIC)
    PHP_ABSTRACT_ME(Actor, receive, Actor_abstract_receive_arginfo)
    PHP_ME(Actor, receiveBlock, Actor_abstract_receiveblock_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */



void phactor_actor_write_property(zval *object, zval *member, zval *value, void **cache_slot)
{
    //
    // rebuild_object_properties
}

zend_object* phactor_actor_ctor(zend_class_entry *entry)
{
    actor_t *new_actor = ecalloc(1, sizeof(actor_t) + zend_object_properties_size(entry));

    // @todo create the UUID on actor creation - for remote actors
    // new_actor->actor_ref = spl_zval_object_hash(actor_zval);
    new_actor->next = NULL;
    new_actor->mailbox = NULL;
    new_actor->blocking = 0;
    new_actor->state = NULL;
    // new_actor->return_value = NULL;

    zend_object_std_init(&new_actor->obj, entry);
    object_properties_init(&new_actor->obj, entry);

    new_actor->obj.handlers = &phactor_actor_handlers;

    add_new_actor(new_actor);

	return &new_actor->obj;
}

zend_object* phactor_actor_system_ctor(zend_class_entry *entry)
{
    actor_system_t *new_actor_system = ecalloc(1, sizeof(actor_system_t) + zend_object_properties_size(entry));

    // @todo create the UUID on actor creation - this is needed for remote actor systems only

    zend_object_std_init(&new_actor_system->obj, entry);
	object_properties_init(&new_actor_system->obj, entry);

    new_actor_system->obj.handlers = &phactor_actor_system_handlers;

	return &new_actor_system->obj;
}

void phactor_globals_ctor(zend_phactor_globals *phactor_globals)
{
    phactor_globals->resources = NULL;
}

void phactor_globals_dtor(zend_phactor_globals *phactor_globals)
{

}

HashTable *phactor_actor_get_debug_info(zval *object, int *is_temp)
{
    // return zend_std_get_properties(object);
    return NULL;
}

HashTable *phactor_actor_get_properties(zval *object)
{
    return zend_std_get_properties(object);
}

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(phactor)
{
    zend_class_entry ce;
	zend_object_handlers *zh = zend_get_std_object_handlers();

    /* ActorSystem Class */
	INIT_CLASS_ENTRY(ce, "ActorSystem", ActorSystem_methods);
	ActorSystem_ce = zend_register_internal_class(&ce);
    // ActorSystem_ce->create_object = phactor_actor_system_ctor;

	memcpy(&phactor_actor_system_handlers, zh, sizeof(zend_object_handlers));

    phactor_actor_system_handlers.offset = XtOffsetOf(actor_system_t, obj);

    /* Actor Class */
	INIT_CLASS_ENTRY(ce, "Actor", Actor_methods);
	Actor_ce = zend_register_internal_class(&ce);
	Actor_ce->ce_flags |= ZEND_ACC_ABSTRACT;
    Actor_ce->create_object = phactor_actor_ctor;
    // Actor_ce->write_property = phactor_actor_write_property;

	memcpy(&phactor_actor_handlers, zh, sizeof(zend_object_handlers));

    phactor_actor_handlers.offset = XtOffsetOf(actor_t, obj);
    phactor_actor_handlers.dtor_obj = php_actor_free_object;
    phactor_actor_handlers.get_debug_info = phactor_actor_get_debug_info;
    phactor_actor_handlers.get_properties = phactor_actor_get_properties;

    ZEND_INIT_MODULE_GLOBALS(phactor, phactor_globals_ctor, NULL);

    TSRMLS_CACHE_UPDATE();
    PHACTOR_G(phactor_instance) = TSRMLS_CACHE;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(phactor)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(phactor)
{
	ZEND_TSRMLS_CACHE_UPDATE();

    zend_hash_init(&PHACTOR_ZG(resolve), 15, NULL, NULL, 0);

    if (PHACTOR_G(phactor_instance) != TSRMLS_CACHE) {
		if (memcmp(sapi_module.name, ZEND_STRL("cli")) == SUCCESS) {
			sapi_module.deactivate = NULL;
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(phactor)
{
    // zend_hash_destroy(&PHACTOR_ZG(resolve));

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(phactor)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "phactor support", "enabled");
	php_info_print_table_end();
}
/* }}} */

PHP_GINIT_FUNCTION(phactor)
{
    //     pthread_mutexattr_t at;
    // 	pthread_mutexattr_init(&at);
    //
    // #if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
    // 	pthread_mutexattr_settype(&at, PTHREAD_MUTEX_RECURSIVE);
    // #else
    // 	pthread_mutexattr_settype(&at, PTHREAD_MUTEX_RECURSIVE_NP);
    // #endif
    //
    //     pthread_mutex_init(&phactor_task_mutex, &at);

    pthread_mutex_init(&phactor_task_mutex, NULL);
}

PHP_GSHUTDOWN_FUNCTION(phactor)
{
    pthread_mutex_destroy(&phactor_task_mutex);
}

/* {{{ phactor_module_entry */
zend_module_entry phactor_module_entry = {
	STANDARD_MODULE_HEADER,
	"phactor",
	NULL,
	PHP_MINIT(phactor),
	PHP_MSHUTDOWN(phactor),
	PHP_RINIT(phactor),
	PHP_RSHUTDOWN(phactor),
	PHP_MINFO(phactor),
	PHP_PHACTOR_VERSION,
    PHP_MODULE_GLOBALS(phactor),
    PHP_GINIT(phactor),
    PHP_GSHUTDOWN(phactor),
    NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_PHACTOR
ZEND_TSRMLS_CACHE_DEFINE()
ZEND_GET_MODULE(phactor)
#endif
