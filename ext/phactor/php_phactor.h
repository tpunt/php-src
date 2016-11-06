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

#ifndef PHACTOR_H
#define PHACTOR_H

#include "Zend/zend_modules.h"

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

#include <php.h>
#include <php_ticks.h>
#include <php_globals.h>
#include <php_main.h>
#include <php_network.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <php_ticks.h>
#include <ext/standard/info.h>
#include <ext/standard/basic_functions.h>
#include <ext/standard/php_var.h>
#include "TSRM.h"
#include "Zend/zend_types.h"
#include "Zend/zend_modules.h"
#include <Zend/zend.h>
#include <Zend/zend_closures.h>
#include <Zend/zend_compile.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_extensions.h>
#include <Zend/zend_globals.h>
#include <Zend/zend_hash.h>
#include <Zend/zend_ts_hash.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_inheritance.h>
#include <Zend/zend_list.h>
#include <Zend/zend_object_handlers.h>
#include <Zend/zend_smart_str.h>
#include <Zend/zend_variables.h>
#include <Zend/zend_vm.h>
#include "store.h"

#define zend_string_new(s) zend_string_dup((s), GC_FLAGS((s)) & IS_STR_PERSISTENT)

extern zend_class_entry *ActorSystem_ce;
extern zend_class_entry *Actor_ce;

#define PHACTOR_ZG(v) TSRMG(phactor_globals_id, zend_phactor_globals *, v)
// #define PHACTOR_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(phactor, v)
#define PHACTOR_G(v) v // phactor_globals.v

#if defined(COMPILE_DL_PHACTOR)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

/* {{{ */
#define PROCESS_MESSAGE_TASK 1
#define SEND_MESSAGE_TASK 2

#define PHACTOR_CTX(ls, id, type, element) (((type) (*((void ***) ls))[TSRM_UNSHUFFLE_RSRC_ID(id)])->element)
#define PHACTOR_EG(ls, v) PHACTOR_CTX(ls, executor_globals_id, zend_executor_globals*, v)
#define PHACTOR_CG(ls, v) PHACTOR_CTX(ls, compiler_globals_id, zend_compiler_globals*, v)
#define PHACTOR_SG(ls, v) PHACTOR_CTX(ls, sapi_globals_id, sapi_globals_struct*, v)
/* }}} */


typedef struct _message_t {
    struct _actor_t *from_actor; // actor struct or char[33] UUID?
    zend_object *sender;
    zval *message;
    struct _message_t *next_message;
} message_t;

typedef struct _actor_t {
    char ref[33]; // 33
    message_t *mailbox; // 8
    struct _actor_t *next; // 8
    zend_execute_data *state; // 8
    zend_object obj; // 56
} actor_t;
// 115
typedef struct _actor_system_t {
    // char system_reference[10]; // @todo needed when remote actors are introduced
    actor_t *actors;
    zend_object obj;
} actor_system_t;

typedef struct _process_message_task {
    actor_t *for_actor;
} process_message_task;

typedef struct _send_message_task {
    actor_t *from_actor;
    actor_t *to_actor;
    zval *message;
} send_message_task;

typedef struct _task_t {
    union {
        process_message_task pmt;
        send_message_task smt;
    } task;
    int task_type;
    struct _task_t *next_task;
} task_t;

typedef struct _task_queue_t {
    task_t *task;
} task_queue_t;

typedef struct _thread_t {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    zend_ulong id; // local storage ID used to fetch local storage data
	void*** ls; // pointer to local storage in TSRM
} thread_t;
/* }}} */

extern thread_t main_thread;
extern pthread_mutex_t phactor_task_mutex;
extern dtor_func_t (default_resource_dtor);
extern zend_object_handlers phactor_actor_handlers;
extern zend_object_handlers phactor_actor_system_handlers;

ZEND_EXTERN_MODULE_GLOBALS(phactor)

ZEND_BEGIN_MODULE_GLOBALS(phactor)
    zval this; //
    HashTable *resources; // used in store.c::pthreads_resources_keep
    HashTable resolve; // used in prepare.c::pthreads_copy_entry
    HashTable symbol_tracker;
ZEND_END_MODULE_GLOBALS(phactor)

// #include "copy.h"

/* {{{ */
void *scheduler();
void process_message(task_t *task);
void enqueue_task(task_t *task);
static task_t *dequeue_task(void);
void spl_object_hash(char *ref, zend_object obj);
zval* zend_call_user_method(zend_object object, zval *retval_ptr, zval *from_actor, zval *message);
actor_t *get_actor_from_hash(char *ref);
actor_t *get_actor_from_object(zend_object *actor_obj);
actor_t *get_actor_from_zval(zval *actor_zval_obj);
task_t *create_send_message_task(zval *from_actor, zval *actor_zval, zval *message);
void initialise_worker_thread_environments(thread_t *phactor_thread);
task_t *create_process_message_task(actor_t *for_actor);
zend_object* phactor_actor_ctor(zend_class_entry *entry);
void add_new_actor(actor_t *new_actor);
message_t *create_new_message(actor_t *from_actor, zval *message);
void send_message(task_t *task);
void send_local_message(task_t *task);
void send_remote_message(task_t *task);
void initialise_actor_system();
/* }}} */

#endif	/* PHACTOR_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
