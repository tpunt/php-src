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
#include "php_phactor_debug.h"
#include "ext/standard/php_rand.h"
#include "ext/standard/php_var.h"
#include "zend_interfaces.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

/* If you declare any globals in php_phactor.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(phactor)
*/

static int php_shutdown = 0;
static zend_bool daemonise_actor_system = 0;
static pthread_t scheduler_thread;
static zend_object_handlers phactor_actor_handlers;
static zend_object_handlers phactor_actor_system_handlers;
struct ActorSystem actor_system;
struct TaskQueue tasks;
struct _phactor_globals phactor_globals;

int thread_count;
pthread_t *worker_threads; // create own struct instead

/*
Get the scheduler to spin up X threads ?? Scheduler must be aware of the threads, but convolutes its responsibility
Recreate the execution contexts in each thread

*/

void *worker_function()
{
    struct Task *current_task = tasks.task;

    while (1) {
        pthread_mutex_lock(&phactor_globals.task_queue_mutex);

        if (php_shutdown && tasks.task == NULL) {
            break;
        }

        if (current_task == NULL) {
            current_task = tasks.task;
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

        current_task = current_task->next_task;

        pthread_mutex_unlock(&phactor_globals.task_queue_mutex);
    }

    return NULL;
}

void initialise_worker_thread_environments()
{
    //
}

void *scheduler_startup()
{
    thread_count = sysconf(_SC_NPROCESSORS_ONLN);
    worker_threads = malloc(sizeof(pthread_t) * thread_count);

    for (int i = 0; i < thread_count; i += sizeof(pthread_t)) {
        pthread_create(&worker_threads[i], NULL, worker_function, NULL);
    }

    initialise_worker_thread_environments();

    // scheduler();

    return NULL;
}

void process_message(struct Task *task)
{
    struct Mailbox *mail = task->task.pmt.actor->mailbox;
    struct Actor *actor = task->task.pmt.actor;
    zval *return_value = malloc(sizeof(zval));

    actor->mailbox = actor->mailbox->next_message;

    // zend_string *rec = zend_string_init(ZEND_STRL("run"), 1);
    // zend_function *receive;
	// pthreads_call_t call = PTHREADS_CALL_EMPTY;
	// zval zresult;
    // if ((receive = zend_hash_find_ptr(&task->task.pmt->actor.ce->function_table, rec))) {
	// 	if (receive->type == ZEND_USER_FUNCTION) {
	// 		call.fci.size = sizeof(zend_fcall_info);
	// 	    call.fci.retval = return_value;
	// 		call.fci.object = &task->task.pmt->actor;
	// 		call.fci.no_separation = 1;
	// 		call.fcc.initialized = 1;
	// 		call.fcc.object = &task->task.pmt->actor;
	// 		call.fcc.calling_scope = task->task.pmt->actor.ce;
	// 		call.fcc.called_scope = task->task.pmt->actor.ce;
	// 		call.fcc.function_handler = receive;
    //
	// 		zend_call_function(&call.fci, &call.fcc);
	// 	}
	// }

    zend_call_user_method(actor->actor, "receive", sizeof("receive") - 1, return_value, 1, mail->message);

    zval_ptr_dtor(mail->message);
    free(mail->message);
    free(mail);
    free(return_value); // @todo remove this line (return the value instead? Or store it elsewhere?)
    dequeue_task(task);
}

void send_message(struct Task *task)
{
    if (task->task.smt.to_actor == NULL) {
        send_remote_message(task);
    } else {
        send_local_message(task);
    }

    dequeue_task(task);
}

void send_local_message(struct Task *task)
{
    struct Actor *actor = task->task.smt.to_actor;
    struct Mailbox *previous_message = actor->mailbox;
    struct Mailbox *current_message = actor->mailbox;

    if (previous_message == NULL) {
        actor->mailbox = task->task.smt.message;
    } else {
        while (current_message != NULL) {
            previous_message = current_message;
            current_message = current_message->next_message;
        }

        previous_message->next_message = task->task.smt.message;
    }

    enqueue_task(create_process_message_task(actor));
}

void send_remote_message(struct Task *task)
{
    // @todo debugging purposes only - no implementation yet
    printf("Tried to send a message to a non-existent (or remote) actor\n");
    assert(0);
}

void initialise_actor_system()
{
    // int core_count = sysconf(_SC_NPROCESSORS_ONLN); // not portable (also gives logical, not physical core count - good/bad?)

    tasks.task = NULL;

    pthread_create(&scheduler_thread, NULL, scheduler_startup, NULL);
}

/* {{{ zend_call_method
 Only returns the returned zval if retval_ptr != NULL */
zval* zend_call_user_method(zend_object object, const char *function_name, size_t function_name_len, zval *retval_ptr, int param_count, zval* arg1)
{
    int result;
    zend_fcall_info fci;
    zval retval;
    zend_class_entry *obj_ce;
    zval params[1];

    ZVAL_COPY_VALUE(&params[0], arg1);

    fci.size = sizeof(fci);
    fci.object = &object;

    zend_string *zstr = malloc(_ZSTR_STRUCT_SIZE(function_name_len));
    GC_REFCOUNT(zstr) = 1;
    GC_TYPE_INFO(zstr) = IS_STRING;
    zend_string_forget_hash_val(zstr);
    ZSTR_LEN(zstr) = function_name_len;
    memcpy(ZSTR_VAL(zstr), function_name, function_name_len);
    ZSTR_VAL(zstr)[function_name_len] = '\0';

    ZVAL_NEW_STR(&fci.function_name, zstr);

    fci.retval = retval_ptr ? retval_ptr : &retval;
    fci.param_count = param_count;
    fci.params = params;
    fci.no_separation = 1;

    /* no interest in caching and no information already present that is
     * needed later inside zend_call_function. */
    result = zend_call_function(&fci, NULL);
    zval_ptr_dtor(&fci.function_name);

    if (result == FAILURE) {
        /* error at c-level */
        obj_ce = object.ce;

        if (!EG(exception)) {
            zend_error_noreturn(E_CORE_ERROR, "Couldn't execute method %s%s%s", obj_ce ? ZSTR_VAL(obj_ce->name) : "", obj_ce ? "::" : "", function_name);
        }
    }
    /* copy arguments back, they might be changed by references */
    if (Z_ISREF(params[0]) && !Z_ISREF_P(arg1)) {
        ZVAL_COPY_VALUE(arg1, &params[0]);
    }
    if (!retval_ptr) {
        zval_ptr_dtor(&retval);
        return NULL;
    }
    return retval_ptr;
}
/* }}} */

// unused for now
zend_string *spl_object_hash(zend_object *obj)
{
    // this is for when remote actors are introduced - not needed for now
    return strpprintf(20, "%s:%08d", "node_id_here", obj->handle);
}

// unused for now - may not be needed...
zend_string *spl_zval_object_hash(zval *zval_obj)
{
    return spl_object_hash(Z_OBJ_P(zval_obj));
}

struct Actor *get_actor_from_object(zend_object *actor_obj)
{
    struct Actor *current_actor = actor_system.actors;

    if (current_actor == NULL) {
        printf("Trying to get actor hash from no actors\n"); // @debug debugging only for now (remove later and invert condition)
    } else {
        while (current_actor != NULL && current_actor->actor.handle != actor_obj->handle) {
            current_actor = current_actor->next;
        }

        if (current_actor != NULL) {
            return current_actor;
        }
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

struct Actor *get_actor_from_zval(zval *actor_zval_obj)
{
    return get_actor_from_object(Z_OBJ_P(actor_zval_obj));
}

zend_object* phactor_actor_ctor(zend_class_entry *entry)
{
    struct Actor *new_actor = ecalloc(1, sizeof(struct Actor));// + zend_object_properties_size(entry));

    // @todo create the UUID on actor creation - this is needed for remote actors only
    // new_actor->actor_ref = spl_zval_object_hash(actor_zval);
    new_actor->next = NULL;
    new_actor->mailbox = NULL;

    zend_object_std_init(&new_actor->actor, entry);
	// object_properties_init(&new_actor->actor, entry);

    new_actor->actor.handlers = &phactor_actor_handlers;

    add_new_actor(new_actor);

	return &new_actor->actor;
}

// no longer needed?
// struct Actor *create_new_actor(zend_class_entry *entry)
// {
//     struct Actor *new_actor = malloc(sizeof(struct Actor));
//     // new_actor->actor = malloc(sizeof(zval));
//
//     ZVAL_COPY(new_actor->actor, actor_zval);
//     new_actor->actor_ref = spl_zval_object_hash(actor_zval);
//     new_actor->next = NULL;
//     new_actor->mailbox = NULL;
//
//     return new_actor;
// }

void add_new_actor(struct Actor *new_actor)
{
    struct Actor *previous_actor = actor_system.actors;
    struct Actor *current_actor = actor_system.actors;

    if (previous_actor == NULL) {
        actor_system.actors = new_actor;
        return;
    }

    while (current_actor != NULL) {
        previous_actor = current_actor;
        current_actor = current_actor->next;
    }

    previous_actor->next = new_actor;
}

struct Task *create_send_message_task(zval *actor_zval, zval *message)
{
    struct Actor *actor = get_actor_from_zval(actor_zval);
    struct Task *new_task = malloc(sizeof(struct Task));

    if (actor == NULL) {
        // debugging purposes only
        printf("Tried to send a message to a non-existent actor\n");
        assert(0);
        // return NULL;
    }

    new_task->task.smt.to_actor = actor;
    new_task->task.smt.message = create_new_message(message);
    new_task->task_type = SEND_MESSAGE_TASK;
    new_task->next_task = NULL;

    return new_task;
}

struct Task *create_process_message_task(struct Actor *actor)
{
    struct Task *new_task = malloc(sizeof(struct Task));

    new_task->task.pmt.actor = actor;
    new_task->task_type = PROCESS_MESSAGE_TASK;
    new_task->next_task = NULL;

    return new_task;
}

void enqueue_task(struct Task *new_task)
{
    struct Task *previous_task = tasks.task;
    struct Task *current_task = tasks.task;

    if (previous_task == NULL) {
        tasks.task = new_task;
        return;
    }

    while (current_task != NULL) {
        previous_task = current_task;
        current_task = current_task->next_task;
    }

    previous_task->next_task = new_task;
}

void dequeue_task(struct Task *task)
{
    struct Task *previous_task = tasks.task;
    struct Task *current_task = tasks.task;

    if (previous_task == task) {
        tasks.task = tasks.task->next_task;
        free(task);
        return;
    }

    while (current_task != task) {
        previous_task = current_task;
        current_task = current_task->next_task;
    }

    previous_task->next_task = current_task->next_task;

    free(task);
}

struct Mailbox *create_new_message(zval *message)
{
    struct Mailbox *new_message = malloc(sizeof(struct Mailbox));
    new_message->message = malloc(sizeof(zval));
    ZVAL_COPY(new_message->message, message);
    new_message->next_message = NULL;

    return new_message;
}

// void initialise_actor_object(zval *actor)
// {
//     add_new_actor(actor);
// }

void remove_actor(struct Actor *target_actor)
{
        if (target_actor == NULL) {
            // debugging purposes only
            printf("Tried to remove to a non-existent actor object\n");
            return;
        }

        struct Actor *current_actor = actor_system.actors;

        if (current_actor == target_actor) {
            actor_system.actors = current_actor->next;
            // zval_ptr_dtor(target_actor->actor);
            // free(target_actor->actor);
            // zend_string_free(target_actor->actor_ref);
            free(target_actor);
            return;
        }

        while (current_actor != target_actor) {
            if (current_actor->next == target_actor) {
                current_actor->next = current_actor->next->next;
                // zval_ptr_dtor(target_actor->actor);
                // free(target_actor->actor);
                // zend_string_free(target_actor->actor_ref);
                free(target_actor);
                break;
            }

            current_actor = current_actor->next;
        }
}

void remove_actor_object(zval *actor)
{
    remove_actor(get_actor_from_zval(actor));
}

void php_actor_free_object(zend_object *obj)
{
    struct Actor *target_actor = get_actor_from_object(obj);

    if (target_actor == NULL) {
        // debugging purposes only
        printf("Tried to free a non-existent object\n");
        return;
    }

    remove_actor(target_actor);
    zend_object_std_dtor(obj);
}

zval *receive_block(zval *actor)
{
    // to be implemented...

    return actor; // shutup
}

void force_shutdown_actor_system()
{
    php_shutdown = 1;
}

void scheduler_blocking()
{
    if (daemonise_actor_system == 0) {
        php_shutdown = 1;
    }

    pthread_join(scheduler_thread, NULL);
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

/* {{{ proto string ActorSystem::shutdown() */
PHP_METHOD(ActorSystem, shutdown)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    force_shutdown_actor_system();
}
/* }}} */

/* {{{ proto string ActorSystem::block() */
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

    enqueue_task(create_send_message_task(actor_zval, message));
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

    return_value = receive_block(getThis());
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
	PHP_ME(Actor, send, Actor_send_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Actor, remove, Actor_remove_arginfo, ZEND_ACC_PUBLIC)
    PHP_ABSTRACT_ME(Actor, receive, Actor_abstract_receive_arginfo)
    PHP_ME(Actor, receiveBlock, Actor_abstract_receiveblock_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */



void initialise_actor_system_class(void)
{
    zend_class_entry ce;
	zend_object_handlers *zh;

	INIT_CLASS_ENTRY(ce, "ActorSystem", ActorSystem_methods);
	ActorSystem_ce = zend_register_internal_class(&ce);
	ActorSystem_ce->ce_flags |= ZEND_ACC_ABSTRACT;

	zh = zend_get_std_object_handlers();

	memcpy(&phactor_actor_system_handlers, zh, sizeof(zend_object_handlers));

	phactor_actor_system_handlers.get_properties = NULL;
}

void initialise_actor_class(void)
{
    zend_class_entry ce;
	zend_object_handlers *zh;

	INIT_CLASS_ENTRY(ce, "Actor", Actor_methods);
	Actor_ce = zend_register_internal_class(&ce);
	Actor_ce->ce_flags |= ZEND_ACC_ABSTRACT;
    Actor_ce->create_object = phactor_actor_ctor;

	zh = zend_get_std_object_handlers();

	memcpy(&phactor_actor_handlers, zh, sizeof(zend_object_handlers));

	phactor_actor_handlers.get_properties = NULL;
    // phactor_actor_handlers.dtor_obj = php_actor_free_object;
}

void php_phactor_init(void)
{
    initialise_actor_system_class();
    initialise_actor_class();
}

// void phactor_globals_ctor(zend_phactor_globals *phactor_globals)
// {
//     phactor_globals.task_queue_mutex = malloc(sizeof(pthread_mutex_t));
//     phactor_globals.actor_list_mutex = malloc(sizeof(pthread_mutex_t));
//
//     pthread_mutex_init(phactor_globals.task_queue_mutex, NULL);
//     pthread_mutex_init(phactor_globals.actor_list_mutex, NULL);
// }



/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(phactor)
{
    php_phactor_init();

    // ZEND_INIT_MODULE_GLOBALS(phactor, phactor_globals_ctor, NULL);

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
#if defined(COMPILE_DL_PHACTOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(phactor)
{
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

/* {{{ phactor_module_entry */
zend_module_entry phactor_module_entry = {
	STANDARD_MODULE_HEADER,
	"phactor",
	NULL,
	PHP_MINIT(phactor),
	// PHP_MSHUTDOWN(phactor),
    NULL,
	PHP_RINIT(phactor),
	// PHP_RSHUTDOWN(phactor),
    NULL,
	PHP_MINFO(phactor),
	PHP_PHACTOR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PHACTOR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(phactor)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
