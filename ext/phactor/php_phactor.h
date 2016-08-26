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

#ifndef PHP_PHACTOR_H
#define PHP_PHACTOR_H

extern zend_module_entry phactor_module_entry;
#define phpext_phactor_ptr &phactor_module_entry

#define PHP_PHACTOR_VERSION "0.1.0"

#ifdef PHP_WIN32
#	define PHP_PHACTOR_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PHACTOR_API __attribute__ ((visibility("default")))
#else
#	define PHP_PHACTOR_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(phactor)
	zend_long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(phactor)
*/

#define PHACTOR_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(phactor, v)

#if defined(ZTS) && defined(COMPILE_DL_PHACTOR)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

/* {{{ */
zend_class_entry *ActorSystem_ce;
zend_class_entry *Actor_ce;
/* }}} */

/* {{{ */
#define PROCESS_MESSAGE_TASK 1
#define SEND_MESSAGE_TASK 2
/* }}} */

struct _phactor_globals {
    pthread_mutex_t task_queue_mutex;
    pthread_mutex_t actor_list_mutex;
};

/* {{{ */
struct ActorSystem {
    // char system_reference[10]; // not needed until remote actors are introduced
    struct Actor *actors;
};

struct Actor {
    zend_object actor;
    struct Mailbox *mailbox;
    struct Actor *next;
    zend_string *actor_ref;
};

struct Mailbox {
    zval *message;
    struct Mailbox *next_message;
};

struct TaskQueue {
    struct Task *task;
};

struct ProcessMessageTask {
    struct Actor *actor;
};

struct SendMessageTask {
    // struct Actor *from_actor; // to do
    struct Actor *to_actor;
    struct Mailbox *message;
};

struct Task {
    union {
        struct ProcessMessageTask pmt;
        struct SendMessageTask smt;
    } task;
    int task_type;
    struct Task *next_task;
};
/* }}} */

/* {{{ */
void *scheduler();
void process_message(struct Task *task);
void enqueue_task(struct Task *task);
void dequeue_task(struct Task *task);
zend_string *spl_object_hash(zend_object *obj);
zend_string *spl_zval_object_hash(zval *zval_obj);
zval* zend_call_user_method(zend_object object, const char *function_name, size_t function_name_len, zval *retval_ptr, int param_count, zval* arg1);
struct Actor *get_actor_from_hash(zend_string *actor_object_ref);
struct Actor *get_actor_from_object(zend_object *actor_obj);
struct Actor *get_actor_from_zval(zval *actor_zval_obj);
// struct Actor *create_new_actor(zval *actor_zval);
struct Task *create_send_message_task(zval *actor_zval, zval *message);
struct Task *create_process_message_task(struct Actor *actor);
zend_object* phactor_actor_ctor(zend_class_entry *entry);
void add_new_actor(struct Actor *new_actor);
struct Mailbox *create_new_message(zval *message);
void send_message(struct Task *task);
void send_local_message(struct Task *task);
void send_remote_message(struct Task *task);
void initialise_actor_system();
/* }}} */

#endif	/* PHP_PHACTOR_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
