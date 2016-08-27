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

#include "TSRM.h"

#define PHACTOR_ZG(v) TSRMG(phactor_globals_id, zend_phactor_globals *, v)

// #define PHACTOR_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(phactor, v)
#define PHACTOR_G(v) phactor_globals.v

#if defined(COMPILE_DL_PHACTOR)
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


typedef struct _mailbox {
    zval *message;
    struct _mailbox *next_message;
} mailbox;

typedef struct _actor {
    zend_object actor;
    mailbox *mailbox;
    struct _actor *next;
    zend_string *actor_ref;
} actor;

struct _actor_system {
    // char system_reference[10]; // @todo needed when remote actors are introduced
    actor *actors;
};

typedef struct _process_message_task {
    actor *actor;
} process_message_task;

typedef struct _send_message_task {
    // actor *from_actor; // @todo to get the sending actor info
    actor *to_actor;
    mailbox *message;
} send_message_task;

typedef struct _task {
    union {
        process_message_task pmt;
        send_message_task smt;
    } task;
    int task_type;
    struct _task *next_task;
} task;

typedef struct _task_queue {
    task *task;
} task_queue;

typedef struct _thread {
    pthread_t thread;
    pthread_mutex_t mutex;
    zend_ulong id;
	void*** ls;
} thread;
/* }}} */

extern thread scheduler_thread;
extern pthread_mutex_t phactor_mutex;
extern zend_object_handlers phactor_actor_handlers;
extern zend_object_handlers phactor_actor_system_handlers;

ZEND_EXTERN_MODULE_GLOBALS(phactor)

ZEND_BEGIN_MODULE_GLOBALS(phactor)
    int php_shutdown;
    zend_bool daemonise_actor_system;
    struct _actor_system actor_system;
    task_queue tasks;
    int thread_count;
    thread *worker_threads; // create own struct instead
    pthread_mutex_t task_queue_mutex;
    pthread_mutex_t actor_list_mutex;
ZEND_END_MODULE_GLOBALS(phactor)

/* {{{ */
void *scheduler();
void process_message(task *task);
void enqueue_task(task *task);
void dequeue_task(task *task);
zend_string *spl_object_hash(zend_object *obj);
zend_string *spl_zval_object_hash(zval *zval_obj);
zval* zend_call_user_method(zend_object object, const char *function_name, size_t function_name_len, zval *retval_ptr, int param_count, zval* arg1);
actor *get_actor_from_hash(zend_string *actor_object_ref);
actor *get_actor_from_object(zend_object *actor_obj);
actor *get_actor_from_zval(zval *actor_zval_obj);
task *create_send_message_task(zval *actor_zval, zval *message);
task *create_process_message_task(actor *actor);
zend_object* phactor_actor_ctor(zend_class_entry *entry);
void add_new_actor(actor *new_actor);
mailbox *create_new_message(zval *message);
void send_message(task *task);
void send_local_message(task *task);
void send_remote_message(task *task);
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
