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

/*
@NikiC Is there any way I can execute some code from an extension before ZE shuts down? In this case, I want to join a thread so that it blocks shutdown until the thread has finished executing. At the moment, I'm doing it in PHP_MSHUTDOWN_FUNCTION, which is obviously a bad idea...
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_phactor.h"
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

struct ActorSystem {
    // char system_reference[10];
    struct Actor *actors;
};

struct Actor {
    zval actor;
    struct Mailbox *mailbox;
    struct Actor *next;
    char actor_ref[32];
};

struct Mailbox {
    zval message;
    struct Mailbox *next;
};

struct TaskQueue {
    struct Task *task;
};

struct Task {
    struct Actor *actor;
    struct Task *next_task;
};

static int actor_system_alive = 1;
static int php_shutdown = 0;
static pthread_t scheduler_thread;
static zend_object_handlers php_actor_handlers;
struct ActorSystem actor_system;
struct TaskQueue tasks;



void remove_actor_from_task_queue(struct Actor *actor)
{
    struct Task *previous_task = tasks.task;
    struct Task *current_task = tasks.task;

    if (previous_task->actor == actor) {
        tasks.task = tasks.task->next_task;
        return;
    }

    while (current_task->actor != actor) {
        previous_task = current_task;
        current_task = current_task->next_task;
    }

    previous_task->next_task = current_task->next_task;

    efree(current_task);
}

void process_message(struct Actor *actor)
{
    struct Mailbox *mail = actor->mailbox;
    zval *return_value = emalloc(sizeof(zval));

    actor->mailbox = actor->mailbox->next;
    zend_call_method_with_1_params(&actor->actor, Z_OBJCE(actor->actor), NULL, "receive", return_value, &mail->message);

    free(mail);
    remove_actor_from_task_queue(actor);
}

void *scheduler()
{
    struct Task *current_task = tasks.task;

    while (1) {
        if (php_shutdown && actor_system.actors == NULL) {
            break;
        }

        if (current_task == NULL) {
            current_task = tasks.task;
            continue;
        }

        if (current_task->actor->mailbox == NULL) {
            continue;
        }

        process_message(current_task->actor);

        current_task = current_task->next_task;
    }

    return NULL;
}

void initialise_actor_system()
{
    // int core_count = sysconf(_SC_NPROCESSORS_ONLN); // not portable, and gives logical, not physical core count

    tasks.task = NULL;

    pthread_create(&scheduler_thread, NULL, scheduler, NULL);
}

intptr_t     hash_mask_handle;
intptr_t     hash_mask_handlers;
int          hash_mask_init;

char *spl_object_hash(zval *obj) /* {{{*/
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

	hash_handle   = hash_mask_handle ^(intptr_t)Z_OBJ_HANDLE_P(obj);
	hash_handlers = hash_mask_handlers;

	return strpprintf(32, "%016lx%016lx", hash_handle, hash_handlers)->val;
}
/* }}} */

struct Actor *get_actor_from_zval(zval *actor_zval)
{
    struct Actor *current_actor = actor_system.actors;
    char actor_object_ref[32];
    strncpy(actor_object_ref, spl_object_hash(actor_zval), sizeof(actor_object_ref));

    while (strncmp(current_actor->actor_ref, actor_object_ref, sizeof(actor_object_ref)) == 0) {
        current_actor = current_actor->next;
    }

    return current_actor;
}

struct Actor *create_new_actor(zval *actor_zval)
{
    struct Actor *new_actor = malloc(sizeof(struct Actor));

    ZVAL_COPY(&new_actor->actor, actor_zval);
    strncpy(new_actor->actor_ref, spl_object_hash(actor_zval), 32 * sizeof(char));
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

    // struct Actor **current_actor = &actor_system.actors;
    // char actor_object_ref[32];
    // strncpy(actor_object_ref, spl_object_hash(actor_zval), 32 * sizeof(char));
    //
    // while (*current_actor != NULL) {
    //     current_actor = &(*current_actor)->next;
    // }
    //
    // *current_actor = malloc(sizeof(struct Actor));
    // ZVAL_COPY(&(*current_actor)->actor, actor_zval);
    // strncpy((*current_actor)->actor_ref, actor_object_ref, 32 * sizeof(char));
    // (*current_actor)->next = NULL;
    // (*current_actor)->mailbox = NULL;
}

struct Mailbox *create_new_message(zval *message)
{
    struct Mailbox *new_message = malloc(sizeof(struct Mailbox));
    ZVAL_COPY(&new_message->message, message);
    new_message->next = NULL;
    return new_message;
}

void add_actor_to_task_queue(struct Actor *actor)
{
    struct Task *previous_task = tasks.task;
    struct Task *current_task = tasks.task;

    if (previous_task == NULL) {
        tasks.task = emalloc(sizeof(struct Task));
        tasks.task->actor = actor;
        tasks.task->next_task = NULL;
        return;
    }

    while (current_task != NULL) {
        previous_task = current_task;
        current_task = current_task->next_task;
    }

    previous_task->actor = actor;
}

void send_message(zval *actor_zval, zval *message)
{
    struct Actor *actor = get_actor_from_zval(actor_zval);
    struct Mailbox *previous_message = actor->mailbox;
    struct Mailbox *current_message = actor->mailbox;
    struct Mailbox *new_message = create_new_message(message);

    if (previous_message == NULL) {
        actor->mailbox = new_message;
        add_actor_to_task_queue(actor);
        return;
    }

    while (current_message != NULL) {
        previous_message = current_message;
        current_message = current_message->next;
    }

    previous_message->next = new_message;
    add_actor_to_task_queue(actor);
}

void initialise_actor_object(zval *actor)
{
    add_new_actor(actor);
}

void remove_actor_object(zval *actor)
{
    struct Actor *target_actor = get_actor_from_zval(actor);
    struct Actor *current_actor = actor_system.actors;

    if (current_actor == target_actor) {
        actor_system.actors = current_actor->next;
        free(target_actor);
        return;
    }

    while (current_actor != target_actor) {
        if (current_actor->next == target_actor) {
            current_actor->next = current_actor->next->next;
            free(target_actor);
            break;
        } else {
            current_actor = current_actor->next;
        }
    }
}

zval *receive_block(zval *actor)
{
    // ...
    return actor; // shutup
}

void shutdown_actor_system()
{
    actor_system_alive = 0;
}


ZEND_BEGIN_ARG_INFO(ActorSystem_construct_arginfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(ActorSystem_shutdown_arginfo, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(Actor_construct_arginfo, 0)
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



/* {{{ proto string Actor::__construct() */
PHP_METHOD(ActorSystem, __construct)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    initialise_actor_system();

    // initialise_actor_object(getThis());
}
/* }}} */

/* {{{ proto string Actor::__construct() */
PHP_METHOD(ActorSystem, shutdown)
{
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

    shutdown_actor_system();
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

/* {{{ proto string Actor::send(Actor $actor, mixed $message) */
PHP_METHOD(Actor, send)
{
    zval *actor;
    zval *message;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "oz", &actor, &message) != SUCCESS) {
		return;
	}

    send_message(actor, message);
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
	PHP_FE_END
}; /* }}} */

/* {{{ */
zend_function_entry Actor_methods[] = {
    PHP_ME(Actor, __construct, Actor_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Actor, send, Actor_send_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Actor, remove, Actor_remove_arginfo, ZEND_ACC_PUBLIC)
    ZEND_FENTRY(receive, NULL, Actor_abstract_receive_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Actor, receiveBlock, Actor_abstract_receiveblock_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */



/* {{{ */
void initialise_actor_system_class(void)
{
    zend_class_entry ce;
	zend_object_handlers *zh;

	INIT_CLASS_ENTRY(ce, "ActorSystem", ActorSystem_methods);
	ActorSystem_ce = zend_register_internal_class(&ce);
	ActorSystem_ce->ce_flags |= ZEND_ACC_ABSTRACT;

	zh = zend_get_std_object_handlers();

	memcpy(&php_actor_handlers, zh, sizeof(zend_object_handlers));

	php_actor_handlers.get_properties = NULL;
}

/* {{{ */
void initialise_actor_class(void)
{
    zend_class_entry ce;
	zend_object_handlers *zh;

	INIT_CLASS_ENTRY(ce, "Actor", Actor_methods);
	Actor_ce = zend_register_internal_class(&ce);
	Actor_ce->ce_flags |= ZEND_ACC_ABSTRACT;

	zh = zend_get_std_object_handlers();

	memcpy(&php_actor_handlers, zh, sizeof(zend_object_handlers));

	php_actor_handlers.get_properties = NULL;
}

/* {{{ */
void php_phactor_init(void)
{
    initialise_actor_system_class();
    initialise_actor_class();
} /* }}} */

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

    pthread_join(scheduler_thread, NULL);

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
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
	NULL,
	PHP_RINIT(phactor),		/* Replace with NULL if there's nothing to do at request start */
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
