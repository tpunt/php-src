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
#include "ext/standard/php_rand.h"
#include "ext/standard/php_var.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

/* If you declare any globals in php_phactor.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(phactor)
*/

struct ActorSystem {
    // char system_reference[10];
    // HashTable         actors;
    // zend_long         index;
	// HashPosition      pos;
	// zend_long         flags;
	// zend_function    *fptr_get_hash;
	// zval             *gcdata;
	// size_t            gcdata_num;
	// zend_object       std;
    struct Actor *actors;
};

/*
spl_SplObjectStorageElement *pelement, element;
zend_hash_key key;
if (spl_object_storage_get_hash(&key, intern, this, obj) == FAILURE) {
    return NULL;
}

pelement = spl_object_storage_get(intern, &key);


RETURN_NEW_STR(php_spl_object_hash(obj));
*/

struct Actor {
    zval actor;
    struct Mailbox *mailbox;
    struct Actor *next;
    zend_string actor_ref;
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

void process_message(struct Actor *actor)
{
    struct Mailbox *mail = actor->mailbox;
    printf("mp: %p", mail);
    // actor->mailbox = actor->mailbox->next;
    // printf("aaaaaaaaaaaa\n");
    php_debug_zval_dump(mail->message, 1);
}

void *scheduler()
{
    struct Actor *current_actor = actor_system.actors;

    while (1) {
        // printf("current_actor: %p\n", current_actor);
        if (php_shutdown && actor_system.actors == NULL) {
            break;
        }

        if (current_actor == NULL) {
            current_actor = actor_system.actors;
            continue;
        }
        // printf("\n3");
        if (current_actor->mailbox == NULL) {
            continue;
        }
        // printf("4");
        process_message(current_actor);

        // spl_observer.c:123
        // zend_call_method_with_1_params(this, intern->std.ce, &intern->fptr_get_hash, "getHash", &rv, obj);

        current_actor = current_actor->next;
    }

    printf("\n");

    return NULL;
}

void initialise_actor_system()
{
    // int core_count = sysconf(_SC_NPROCESSORS_ONLN); // not portable, and gives logical, not physical core count

    pthread_create(&scheduler_thread, NULL, scheduler, NULL);
}

zend_string *autoload_extensions;
HashTable   *autoload_functions;
intptr_t     hash_mask_handle;
intptr_t     hash_mask_handlers;
int          hash_mask_init;
int          autoload_running;

zend_string *spl_object_hash(zval *obj) /* {{{*/
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

	return strpprintf(32, "%016lx%016lx", hash_handle, hash_handlers);
}
/* }}} */

struct Actor *get_actor_from_zval(zval *actor_zval)
{
    struct Actor *current_actor = actor_system.actors;
    zend_string *actor_object_ref = spl_object_hash(actor_zval);

    // if (current_actor == NULL) {
        // return NULL;
    // }

    printf("current_actor: %p\n", current_actor);
    while (strcmp(current_actor->actor_ref.val, actor_object_ref->val) != 0) {
        printf("current_actor ref: %s\nactor_object ref: %s\n", current_actor->actor_ref.val, actor_object_ref->val);
        current_actor = current_actor->next;
        printf("current_actor: %p\n", current_actor);
    }

    return current_actor;
}

void add_new_actor(zval *actor_zval)
{
    struct Actor **current_actor = &actor_system.actors;
    zend_string *actor_ref = spl_object_hash(actor_zval);

    while (*current_actor != NULL) {
        *current_actor = (*current_actor)->next;
    }

    *current_actor = malloc(sizeof(struct Actor));
    ZVAL_COPY(&(*current_actor)->actor, actor_zval);
    (*current_actor)->actor_ref = *actor_ref;
    (*current_actor)->next = NULL;
    // printf("Added actor: %p\n", *current_actor);
    // printf("actor_system.actors: %p\n", actor_system.actors);
}

void send_message(zval *actor_zval, zval *message)
{
    struct Actor *actor = get_actor_from_zval(actor_zval);
    struct Mailbox *current_message = actor->mailbox;

    if (current_message == NULL) {
        actor->mailbox = malloc(sizeof(struct Mailbox));
        ZVAL_COPY(actor->mailbox->message, message);
        actor->mailbox->next = NULL;
        // printf("sent 1 message\n");
        return;
    }

    while (current_message != NULL) {
        // printf("current_message: %p\n", current_message);
        // printf("current_message->next: %p\n", current_message->next);
        if (current_message->next == NULL) {
            // printf("sent more than 1 message\n");
            current_message->next = malloc(sizeof(struct Mailbox));
            ZVAL_COPY(current_message->next->message, message);
            current_message->next->next = NULL;
            break;
        } else {
            current_message = current_message->next;
        }
    }
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
    zval *actor;
    zval *message;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "oz", &actor, &message) != SUCCESS) {
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
