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
        task_t *current_task = dequeue_task();

        if (!current_task) {
            if (PHACTOR_G(php_shutdown)) {
                break;
            }
            continue;
        }

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
    actor_t *actor = task->task.pmt.for_actor;
    message_t *message = actor->mailbox;
    zval *return_value = malloc(sizeof(zval));
    zval *sender = malloc(sizeof(zval));

    ZVAL_OBJ(sender, message->sender);
    actor->mailbox = actor->mailbox->next_message;

    zend_call_user_method(actor->obj, return_value, sender, message->message);

    // zval_ptr_dtor(message->message);
    free(message->message);
    efree(message->sender);
    free(message);

    // zval_ptr_dtor(sender);
    free(sender);
    free(return_value); // @todo remove this line (return the value instead? Or store it elsewhere?)
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
    actor_t *to_actor = task->task.smt.to_actor;
    message_t *current_message = to_actor->mailbox;
    message_t *new_message = create_new_message(task->task.smt.from_actor, task->task.smt.message);

    if (current_message == NULL) {
        to_actor->mailbox = new_message;
    } else {
        while (current_message->next_message != NULL) {
            current_message = current_message->next_message;
        }

        current_message->next_message = new_message;
    }

    enqueue_task(create_process_message_task(to_actor));
}

void send_remote_message(task_t *task)
{
    // @todo debugging purposes only - no implementation yet
    printf("Tried to send a message to a non-existent (or remote) actor\n");
    assert(0);
}

task_t *create_send_message_task(zval *from_actor_zval, zval *to_actor_zval, zval *message)
{
    task_t *new_task = malloc(sizeof(task_t));

    new_task->task_type = SEND_MESSAGE_TASK;
    new_task->next_task = NULL;
    new_task->task.smt.from_actor = get_actor_from_zval(from_actor_zval);
    new_task->task.smt.to_actor = get_actor_from_zval(to_actor_zval);
    new_task->task.smt.message = malloc(sizeof(zval));

    ZVAL_DUP(new_task->task.smt.message, message); // @todo ZVAL_DUP instead?

    return new_task;
}

message_t *create_new_message(actor_t *from_actor, zval *message)
{
    message_t *new_message = malloc(sizeof(message_t));

    new_message->from_actor = from_actor;
    new_message->message = message;
    new_message->next_message = NULL;
    new_message->sender = ecalloc(1, sizeof(zend_object));

    memcpy(new_message->sender, &from_actor->obj, sizeof(zend_object));

    return new_message;
}

task_t *create_process_message_task(actor_t *for_actor)
{
    task_t *new_task = malloc(sizeof(task_t));

    new_task->task.pmt.for_actor = for_actor;
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
    pthread_mutex_lock(&PHACTOR_G(phactor_task_mutex));

    task_t *task = PHACTOR_G(tasks).task;

    if (task) {
        PHACTOR_G(tasks).task = task->next_task;
    }

    pthread_mutex_unlock(&PHACTOR_G(phactor_task_mutex));

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

// @todo actually generate UUIDs for remote actors
void spl_object_hash(char *ref, zend_object obj)
{
    sprintf(ref, "%022d%010d", rand(), obj.handle);
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

    // while (strncmp(current_actor->ref->val, actor_object_ref->val, sizeof(char) * 32) != 0) {
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

    /*
    Check if remote actor? target_actor->is_remote ?
    Otherwise, the actor should exist
    Just perform a NULL check on target_actor? get_actor_from_object is invoked
    before this, so it will return NULL if local actor is not found.
    */

    actor_t *current_actor = PHACTOR_G(actor_system).actors;
    actor_t *previous_actor = current_actor;

    while (current_actor != target_actor) {
        previous_actor = current_actor;
        current_actor = current_actor->next;
    }

    previous_actor->next = current_actor->next;

    GC_REFCOUNT(&target_actor->obj) = 1;

    zend_object_std_dtor(&target_actor->obj);

    efree(target_actor);

    pthread_mutex_unlock(&phactor_actors_mutex);
}

void remove_actor_object(zval *actor)
{
    remove_actor(get_actor_from_zval(actor));
}

void php_actor_free_object(zend_object *obj)
{
    actor_t *target_actor = get_actor_from_object(obj);

    if (target_actor != NULL) {
        remove_actor(target_actor);
        return;
    }

    // @todo any checking? Could either be remote actor or may have been manually
    // free'd via Actor::remove
}

void php_actor_system_free_object(zend_object *obj)
{
    actor_system_t *as = XtOffsetOf(actor_system_t, obj);

    efree(as);
}

// @todo currently impossible with current ZE
void receive_block(zval *actor_zval, zval *return_value)
{
    // actor_t *actor = get_actor_from_zval(actor_zval);

    // zend_execute_data *execute_data = EG(current_execute_data);

    // actor->state = zend_freeze_call_stack(EG(current_execute_data));
    // actor->return_value = actor_zval; // tmp

    // EG(current_execute_data) = NULL;

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
No mutex since only php_shutdown is modifiable by different threads, and can
only be set to 1.
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
    actor_t *new_actor = ecalloc(1, sizeof(actor_t)); // + zend_object_properties_size(entry)

    new_actor->next = NULL;
    new_actor->mailbox = NULL;
    new_actor->state = NULL;

    zend_object_std_init(&new_actor->obj, entry);
    object_properties_init(&new_actor->obj, entry);

    ++GC_REFCOUNT(&new_actor->obj); // needed for ephemeral actors

    new_actor->obj.handlers = &phactor_actor_handlers;

    spl_object_hash(new_actor->ref, new_actor->obj);

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
    phactor_actor_system_handlers.dtor_obj = php_actor_system_free_object;

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
