/*
  +----------------------------------------------------------------------+
  | pthreads                                                             |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2012 - 2015                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Joe Watkins <krakjoe@php.net>                                |
  +----------------------------------------------------------------------+
 */
#ifndef HAVE_PTHREADS_PREPARE_H
#define HAVE_PTHREADS_PREPARE_H

#include "php_phactor.h"

void pthreads_prepare_exception_handler(thread* thread);
void pthreads_prepare_includes(thread* thread);
void pthreads_prepare_classes(thread* thread);
void pthreads_prepare_functions(thread* thread);
void pthreads_prepare_constants(thread *thread);
void pthreads_prepare_ini(thread *thread);
void pthreads_prepare_sapi(thread *thread);

/* {{{ fetch prepared class entry */
zend_class_entry* pthreads_prepared_entry(thread* thread, zend_class_entry *candidate); /* }}} */

/* {{{ */
// void pthreads_prepare_parent(pthreads_object_t *thread); /* }}} */

/* {{{ */
// int pthreads_prepared_startup(pthreads_object_t* thread, pthreads_monitor_t *ready); /* }}} */

/* {{{ */
// int pthreads_prepared_shutdown(pthreads_object_t* thread); /* }}} */
#endif
