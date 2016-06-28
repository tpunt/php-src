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
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

/* If you declare any globals in php_phactor.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(phactor)
*/

struct ActorSystem {
    // char system_reference[10];
    struct Actor *actor;
};

struct Actor {
    zend_object *actor;
    struct Mailbox *mailbox;
    struct Actor *next;
};

struct Mailbox {
    zval *message;
    struct Mailbox *next;
};

static int actor_system_alive = 1;
static int php_shutdown = 0;
static pthread_t scheduler_thread;
static zend_object_handlers php_actor_handlers;
struct ActorSystem actor_system;

void *scheduler()
{
    struct Actor *start_actor = actor_system.actor;
    struct Actor *current_actor = start_actor;

    while (1) {
        // if (!actor_system_alive) {
        //     break;
        // }

        if (php_shutdown && start_actor == NULL) {
            break;
        }

        if (current_actor == NULL) {
            current_actor = start_actor;
            continue;
        }

        if (current_actor->mailbox == NULL) {
            continue;
        }

        printf("Performing some work...\n");
        // spl_observer.c:123
        // zend_call_method_with_1_params(this, intern->std.ce, &intern->fptr_get_hash, "getHash", &rv, obj);

        current_actor = current_actor->next;
    }
}

void initialise_actor_system()
{
    int core_count = sysconf(_SC_NPROCESSORS_ONLN); // not portable, and gives logical, not physical core count

    pthread_create(&scheduler_thread, NULL, scheduler, NULL);
}

struct Actor *get_actor_from_object(zend_object *actor_object)
{
    struct Actor *actor = actor_system.actor;

    while (actor->actor != actor_object) {
        actor = actor->next;
    }

    // assert(actor != NULL);

    return actor;
}

void add_new_actor(zend_object *actor_object)
{
    struct Actor *last_actor = actor_system.actor;

    if (last_actor == NULL) {
        actor_system.actor = malloc(sizeof(struct Actor));
        actor_system.actor->actor = actor_object;
        printf("Added actor %p\n", actor_object);
        return;
    }

    while (last_actor != NULL) {
        if (last_actor->next == NULL) {
            last_actor->next = malloc(sizeof(struct Actor));
            last_actor->next->actor = actor_object;
            printf("Added actor %p\n", actor_object);
            break;
        } else {
            last_actor = last_actor->next;
        }
    }
}

void send_message(zend_object *actor_object, zval *message)
{
    printf("Sending message to %p\n", actor_object);
    struct Actor *actor = get_actor_from_object(actor_object);
    struct Mailbox *last_message = actor->mailbox->next;

    while (last_message != NULL) {
        last_message = last_message->next;
    }

    last_message = malloc(sizeof(struct Mailbox));
    last_message->message = message;
}

void initialise_actor_object(zval *actor)
{
    add_new_actor(Z_OBJ_P(actor));
}

void remove_actor_object(zval *actor)
{
    struct Actor *target_actor = get_actor_from_object(Z_OBJ_P(actor));
    struct Actor *last_actor = actor_system.actor;

    if (last_actor == target_actor) {
        actor_system.actor = last_actor->next;
        free(target_actor);
        return;
    }

    while (last_actor != target_actor) {
        if (last_actor->next == target_actor) {
            last_actor->next = last_actor->next->next;
            free(target_actor);
            break;
        } else {
            last_actor = last_actor->next;
        }
    }
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
    zend_object *actor;
    zval *message;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Oz", &actor, Actor_ce, &message) != SUCCESS) {
		return;
	}

    send_message(actor, message); // do asynchronously
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

	memcpy(&php_actor_handlers, zh, sizeof(zend_object_handlers));

	php_actor_handlers.get_properties = NULL;
}

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

void php_phactor_init(void)
{
    initialise_actor_system_class();
    initialise_actor_class();
    // initialise_actor_system();
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
