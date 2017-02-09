/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2017 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors:                                                             |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_API.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_async.h"

ZEND_API zend_class_entry *zend_ce_awaitable;

void zend_register_awaitable_ce(void) /* {{{ */
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Awaitable", awaitable_functions);
	zend_ce_awaitable = zend_register_internal_class(&ce);
	zend_ce_awaitable->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_awaitable->create_object = zend_generator_create;
	zend_ce_awaitable->serialize = zend_class_serialize_deny;
	zend_ce_awaitable->unserialize = zend_class_unserialize_deny;

	memcpy(&zend_generator_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_generator_handlers.free_obj = zend_generator_free_storage;
	zend_generator_handlers.dtor_obj = zend_generator_dtor_storage;
	zend_generator_handlers.get_gc = zend_generator_get_gc;
	zend_generator_handlers.clone_obj = NULL;
	zend_generator_handlers.get_constructor = zend_generator_get_constructor;

	INIT_CLASS_ENTRY(ce, "ClosedGeneratorException", NULL);
	zend_ce_ClosedGeneratorException = zend_register_internal_class_ex(&ce, zend_ce_exception);
}
/* }}} */