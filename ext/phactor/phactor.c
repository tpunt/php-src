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
static zend_object_handlers php_actor_handler;
static zend_object_handlers php_actor_system_handler;
struct ActorSystem actor_system;
struct TaskQueue tasks;

// for object hashing - needed?
static intptr_t     hash_mask_handle;
static intptr_t     hash_mask_handlers;
static int          hash_mask_init;



void *scheduler()
{
    struct Task *current_task = tasks.task;

    while (1) {
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
    }

    return NULL;
}

void process_message(struct Task *task)
{
    struct Mailbox *mail = task->task.pmt.actor->mailbox;
    struct Actor *actor = task->task.pmt.actor;
    zval *return_value = emalloc(sizeof(zval));

    actor->mailbox = actor->mailbox->next_message;

    // first NULL: Z_OBJCE_P(actor->actor)
    zend_call_method_with_1_params(actor->actor, NULL, NULL, "receive", return_value, mail->message);

    zval_ptr_dtor(mail->message);
    efree(mail->message);
    efree(mail);
    efree(return_value); // remove this line (return the value instead? Or store it elsewhere?)
    dequeue_task(task);
}

void send_message(struct Task *task)
{
    if (task->task.smt.to_actor == NULL) {
        send_remote_message(task);
    } else {
        send_local_message(task);
    }

    // efree(task->task.smt.message->message);
    // efree(task->task.smt.message);

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
    // debugging purposes only - no implementation yet
    printf("Tried to send a message to a non-existent (or remote) actor\n");
    assert(0);
}

void initialise_actor_system()
{
    // int core_count = sysconf(_SC_NPROCESSORS_ONLN); // not portable (also gives logical, not physical core count)

    tasks.task = NULL;

    pthread_create(&scheduler_thread, NULL, scheduler, NULL);
}

zend_string *spl_object_hash(zend_object *obj)
{
    intptr_t hash_handle, hash_handlers;

	if (!hash_mask_init) {
		if (!BG(mt_rand_is_seeded)) {
			php_mt_srand((uint32_t)GENERATE_SEED());
		}

		hash_mask_handle = (intptr_t)(php_mt_rand() >> 1);
		hash_mask_handlers = (intptr_t)(php_mt_rand() >> 1);
		hash_mask_init = 1;
	}

	hash_handle   = hash_mask_handle ^(intptr_t)obj->handle;
	hash_handlers = hash_mask_handlers;

	return strpprintf(32, "%016lx%016lx", hash_handle, hash_handlers);
}

zend_string *spl_zval_object_hash(zval *zval_obj)
{
    return spl_object_hash(Z_OBJ_P(zval_obj));
}

struct Actor *get_actor_from_hash(zend_string *actor_object_ref)
{
    struct Actor *current_actor = actor_system.actors;

    if (current_actor == NULL) {
        printf("Trying to get actor hash from no actors\n");
        return NULL;
    }

    while (strncmp(current_actor->actor_ref->val, actor_object_ref->val, sizeof(char) * 32) != 0) {
        current_actor = current_actor->next;

        if (current_actor == NULL) {
            return NULL;
        }
    }

    zend_string_free(actor_object_ref);

    return current_actor;
}

struct Actor *get_actor_from_object(zend_object *actor_obj)
{
    return get_actor_from_hash(spl_object_hash(actor_obj));
}

struct Actor *get_actor_from_zval(zval *actor_zval_obj)
{
    return get_actor_from_hash(spl_zval_object_hash(actor_zval_obj));
}

struct Actor *create_new_actor(zval *actor_zval)
{
    struct Actor *new_actor = emalloc(sizeof(struct Actor));
    new_actor->actor = emalloc(sizeof(zval));

    ZVAL_COPY(new_actor->actor, actor_zval);
    new_actor->actor_ref = spl_zval_object_hash(actor_zval);
    new_actor->next = NULL;
    new_actor->mailbox = NULL;

    return new_actor;
}

void add_new_actor(zval *actor_zval)
{
    struct Actor *previous_actor = actor_system.actors;
    struct Actor *current_actor = actor_system.actors;
    struct Actor *new_actor = create_new_actor(actor_zval);

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
    struct Task *new_task = emalloc(sizeof(struct Task));

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
    struct Task *new_task = emalloc(sizeof(struct Task));

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
        efree(task);
        return;
    }

    while (current_task != task) {
        previous_task = current_task;
        current_task = current_task->next_task;
    }

    previous_task->next_task = current_task->next_task;

    efree(task);
}

struct Mailbox *create_new_message(zval *message)
{
    struct Mailbox *new_message = emalloc(sizeof(struct Mailbox));
    new_message->message = emalloc(sizeof(zval));
    ZVAL_COPY(new_message->message, message);
    new_message->next_message = NULL;

    return new_message;
}

void initialise_actor_object(zval *actor)
{
    add_new_actor(actor);
}

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
            zval_ptr_dtor(target_actor->actor);
            efree(target_actor->actor);
            zend_string_free(target_actor->actor_ref);
            efree(target_actor);
            return;
        }

        while (current_actor != target_actor) {
            if (current_actor->next == target_actor) {
                current_actor->next = current_actor->next->next;
                zval_ptr_dtor(target_actor->actor);
                efree(target_actor->actor);
                zend_string_free(target_actor->actor_ref);
                efree(target_actor);
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


ZEND_BEGIN_ARG_INFO(Actor_construct_arginfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Actor_destruct_arginfo, 0)
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

/* {{{ proto string Actor::__construct() */
PHP_METHOD(Actor, __construct)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    initialise_actor_object(getThis());
}
/* }}} */

/* {{{ proto string Actor::__destruct() */
PHP_METHOD(Actor, __destruct)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    php_actor_free_object(Z_OBJ_P(getThis()));
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
    PHP_ME(Actor, __construct, Actor_construct_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Actor, __destruct, Actor_destruct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Actor, send, Actor_send_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Actor, remove, Actor_remove_arginfo, ZEND_ACC_PUBLIC)
    ZEND_FENTRY(receive, NULL, Actor_abstract_receive_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
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

	memcpy(&php_actor_system_handler, zh, sizeof(zend_object_handlers));

	php_actor_system_handler.get_properties = NULL;
}

void initialise_actor_class(void)
{
    zend_class_entry ce;
	zend_object_handlers *zh;

	INIT_CLASS_ENTRY(ce, "Actor", Actor_methods);
	Actor_ce = zend_register_internal_class(&ce);
	Actor_ce->ce_flags |= ZEND_ACC_ABSTRACT;

	zh = zend_get_std_object_handlers();

	memcpy(&php_actor_handler, zh, sizeof(zend_object_handlers));

	php_actor_handler.get_properties = NULL;
    php_actor_handler.dtor_obj = php_actor_free_object;
}

void php_phactor_init(void)
{
    initialise_actor_system_class();
    initialise_actor_class();
}

/* {{{ php_phactor_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_phactor_init_globals(zend_phactor_globals *phactor_globals)
{
	phactor_globals->global_value = 0;
	phactor_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(phactor)
{
    php_phactor_init();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(phactor)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

    // doesn't work
    // scheduler_blocking()

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(phactor)
{
#if defined(COMPILE_DL_PHACTOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(phactor)
{
    // doesn't work
    // scheduler_blocking()

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(phactor)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "phactor support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ phactor_module_entry
 */
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
