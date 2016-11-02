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
#ifndef HAVE_PTHREADS_STORE
#define HAVE_PTHREADS_STORE

#include "prepare.h"
#include "php_phactor.h"

#define IS_CLOSURE (IS_PTR + 1)
#define IS_PHACTOR (IS_PTR + 2)

#define IS_PHACTOR_CLASS(c) \
	(instanceof_function(c, Actor_ce))

#define IS_PHACTOR_OBJECT(o)   \
        (Z_TYPE_P(o) == IS_OBJECT && IS_PHACTOR_CLASS(Z_OBJCE_P(o)))

#define PTHREADS_STORAGE_EMPTY {0, 0, 0, 0, NULL}

/* {{{ */
static inline const zend_op* pthreads_check_opline(zend_execute_data *ex, zend_long offset, zend_uchar opcode) {
	if (ex && ex->func && ex->func->type == ZEND_USER_FUNCTION) {
		zend_op_array *ops = &ex->func->op_array;
		const zend_op *opline = ex->opline;

		if ((opline + offset) >= ops->opcodes) {
			opline += offset;
			if (opline->opcode == opcode) {
				return opline;
			}
		}
	}
	return NULL;
} /* }}} */

/* {{{ */
static inline zend_bool pthreads_check_opline_ex(zend_execute_data *ex, zend_long offset, zend_uchar opcode, uint32_t extended_value) {
	const zend_op *opline = pthreads_check_opline(ex, offset, opcode);
	if (opline && opline->extended_value == extended_value)
		return 1;
	return 0;
} /* }}} */

static inline int pthreads_store_remove_complex(zval *pzval) {
	switch (Z_TYPE_P(pzval)) {
		case IS_ARRAY: {
			HashTable *tmp = zend_array_dup(Z_ARRVAL_P(pzval));

			ZVAL_ARR(pzval, tmp);

			zend_hash_apply(tmp, pthreads_store_remove_complex);
		} break;

		case IS_OBJECT:
			if (instanceof_function(Z_OBJCE_P(pzval), Actor_ce))
				break;
		case IS_RESOURCE:
			return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ */
static int pthreads_store_tostring(zval *pzval, char **pstring, size_t *slength, zend_bool complex) {
	int result = FAILURE;
	if (pzval && (Z_TYPE_P(pzval) != IS_NULL)) {
		smart_str smart;
		HashTable *tmp = NULL;
		zval ztmp;

		memset(&smart, 0, sizeof(smart_str));

		if (Z_TYPE_P(pzval) == IS_ARRAY && !complex) {
			tmp = zend_array_dup(Z_ARRVAL_P(pzval));

			ZVAL_ARR(&ztmp, tmp);
			pzval = &ztmp;

			zend_hash_apply(tmp, pthreads_store_remove_complex);
		}

		if ((Z_TYPE_P(pzval) != IS_OBJECT) ||
			(Z_OBJCE_P(pzval)->serialize != zend_class_serialize_deny)) {
			php_serialize_data_t vars;

			PHP_VAR_SERIALIZE_INIT(vars);
			php_var_serialize(&smart, pzval, &vars);
			PHP_VAR_SERIALIZE_DESTROY(vars);

			if (EG(exception)) {
				smart_str_free(&smart);

				if (tmp) {
					zval_dtor(&ztmp);
				}

				*pstring = NULL;
				*slength = 0;
				return FAILURE;
			}
		}

		if (tmp) {
			zval_dtor(&ztmp);
		}

		if (smart.s) {
			*slength = smart.s->len;
			if (*slength) {
				*pstring = malloc(*slength+1);
				if (*pstring) {
					memcpy(
						(char*) *pstring, (const void*) smart.s->val, smart.s->len
					);
					result = SUCCESS;
				}
			} else *pstring = NULL;
		}

		smart_str_free(&smart);
	} else {
	    *slength = 0;
	    *pstring = NULL;
	}
	return result;
} /* }}} */

/* {{{ */
static pthreads_storage* pthreads_store_create(zval *unstore, zend_bool complex){
	pthreads_storage *storage = NULL;

	if (Z_TYPE_P(unstore) == IS_INDIRECT)
		return pthreads_store_create(Z_INDIRECT_P(unstore), 1);
	if (Z_TYPE_P(unstore) == IS_REFERENCE)
		return pthreads_store_create(&Z_REF_P(unstore)->val, 1);

	storage = (pthreads_storage*) calloc(1, sizeof(pthreads_storage));

	switch((storage->type = Z_TYPE_P(unstore))){
		case IS_NULL: /* do nothing */ break;
		case IS_TRUE: storage->simple.lval = 1; break;
		case IS_FALSE: storage->simple.lval = 0; break;
		case IS_DOUBLE: storage->simple.dval = Z_DVAL_P(unstore); break;
		case IS_LONG: storage->simple.lval = Z_LVAL_P(unstore); break;

		case IS_STRING: if ((storage->length = Z_STRLEN_P(unstore))) {
			storage->data =
				(char*) malloc(storage->length+1);
			memcpy(storage->data, Z_STRVAL_P(unstore), storage->length);
		} break;

		case IS_RESOURCE: {
			if (complex) {
				pthreads_resource resource = malloc(sizeof(*resource));
				if (resource) {
					resource->original = Z_RES_P(unstore);
					resource->ls = TSRMLS_CACHE;

					storage->data = resource;
					Z_ADDREF_P(unstore);
				}
			} else storage->type = IS_NULL;
		} break;

		case IS_OBJECT:
			if (instanceof_function(Z_OBJCE_P(unstore), zend_ce_closure)) {
				const zend_function *def =
				    zend_get_closure_method_def(unstore);
				storage->type = IS_CLOSURE;
				storage->data =
				    (zend_function*) malloc(sizeof(zend_op_array));
				memcpy(storage->data, def, sizeof(zend_op_array));
				break;
			}

			if (instanceof_function(Z_OBJCE_P(unstore), Actor_ce)) {
				storage->type = IS_PHACTOR;
				storage->data = Z_OBJ_P(unstore);
				break;
			}

			if (!complex) {
				storage->type = IS_NULL;
				break;
			}

		/* break intentionally omitted */
		case IS_ARRAY: if (pthreads_store_tostring(unstore, (char**) &storage->data, &storage->length, complex)==SUCCESS) {
			if (Z_TYPE_P(unstore) == IS_ARRAY)
				storage->exists = zend_hash_num_elements(Z_ARRVAL_P(unstore));
		} break;

	}
	return storage;
}
/* }}} */

/* {{{ */
static int pthreads_store_tozval(zval *pzval, char *pstring, size_t slength) {
	int result = SUCCESS;

	if (pstring) {
		const unsigned char* pointer = (const unsigned char*) pstring;

		if (pointer) {
			php_unserialize_data_t vars;

			PHP_VAR_UNSERIALIZE_INIT(vars);
			if (!php_var_unserialize(pzval, &pointer, pointer+slength, &vars)) {
				result = FAILURE;
			}
			PHP_VAR_UNSERIALIZE_DESTROY(vars);

			if (result != FAILURE) {
				if (Z_REFCOUNTED_P(pzval) && !IS_PHACTOR_OBJECT(pzval)) {
					Z_SET_REFCOUNT_P(pzval, 1);
				}
			} else ZVAL_NULL(pzval);
		} else result = FAILURE;
	} else result = FAILURE;

	return result;
} /* }}} */

/* {{{ mark a resource for keeping */
zend_bool pthreads_resources_keep(pthreads_resource res) {
	if (!PHACTOR_ZG(resources)) {
		ALLOC_HASHTABLE(PHACTOR_ZG(resources));
		zend_hash_init(PHACTOR_ZG(resources), 15, NULL, NULL, 0);
	}

	if (zend_hash_index_update_ptr(PHACTOR_ZG(resources), (zend_long) res->original, res)) {
		return 1;
	}
	return 0;
} /* }}} */

zend_bool pthreads_resources_kept(zend_resource *entry) {
	if (PHACTOR_ZG(resources)) {
		pthreads_resource data = zend_hash_index_find_ptr(PHACTOR_ZG(resources), (zend_long) entry);
		if (data) {
			if (data->ls != TSRMLS_CACHE) {
				return 1;
			}
		}
	}
	return 0;
} /* }}} */

/* {{{ */
static int pthreads_store_convert(pthreads_storage *storage, zval *pzval){
	int result = SUCCESS;

	switch(storage->type) {
		case IS_NULL: ZVAL_NULL(pzval); break;

		case IS_STRING:
			if (storage->data && storage->length) {
				ZVAL_STRINGL(pzval, (char*)storage->data, storage->length);
			} else ZVAL_EMPTY_STRING(pzval);
		break;

		case IS_FALSE:
		case IS_TRUE: ZVAL_BOOL(pzval, storage->simple.lval); break;

		case IS_LONG: ZVAL_LONG(pzval, storage->simple.lval); break;
		case IS_DOUBLE: ZVAL_DOUBLE(pzval, storage->simple.dval); break;
		case IS_RESOURCE: {
			pthreads_resource stored = (pthreads_resource) storage->data;

			if (stored->ls != TSRMLS_CACHE) {
				zval *search = NULL;
				zend_ulong index = 0;
				zend_string *name = NULL;
				zend_resource *resource, *found = NULL;

				ZEND_HASH_FOREACH_KEY_VAL(&EG(regular_list), index, name, search) {
					resource = Z_RES_P(search);
					if (resource->ptr == stored->original->ptr) {
						found = resource;
						break;
					}
				} ZEND_HASH_FOREACH_END();

				if (!found) {
					ZVAL_RES(pzval, stored->original);
					if (zend_hash_next_index_insert(&EG(regular_list), pzval)) {
					    pthreads_resources_keep(stored);
					} else ZVAL_NULL(pzval);
					Z_ADDREF_P(pzval);
				} else ZVAL_COPY(pzval, search);
			} else {
				ZVAL_RES(pzval, stored->original);
				Z_ADDREF_P(pzval);
			}
		} break;

		case IS_CLOSURE: {
			char *name;
			size_t name_len;
			zend_function *closure = pthreads_copy_function((zend_function*) storage->data);

#if PHP_VERSION_ID >= 70100
			zend_create_closure(pzval, closure, zend_get_executed_scope(), closure->common.scope, NULL);
#else
			zend_create_closure(pzval, closure, EG(scope), closure->common.scope, NULL);
#endif
			name_len = spprintf(&name, 0, "Closure@%p", zend_get_closure_method_def(pzval));
			if (!zend_hash_str_update_ptr(EG(function_table), name, name_len, closure)) {
				result = FAILURE;
				zval_dtor(pzval);
			} else result = SUCCESS;
			efree(name);
		} break;

		case IS_PHACTOR: {
			actor_t *actor = storage->data;

			if (pthreads_check_opline_ex(EG(current_execute_data), 1, ZEND_CAST, IS_OBJECT)) {
				ZVAL_OBJ(pzval, &actor->obj);
				Z_ADDREF_P(pzval);
				break;
			}

			// if (!pthreads_globals_object_connect((zend_ulong)actor, NULL, pzval)) {
			// 	zend_throw_exception_ex(
			// 		spl_ce_RuntimeException, 0,
			// 		"pthreads detected an attempt to connect to an object which has already been destroyed");
			// 	result = FAILURE;
			// }
		} break;

		case IS_OBJECT:
		case IS_ARRAY:

			result = pthreads_store_tozval(pzval, (char*) storage->data, storage->length);
			if (result == FAILURE) {
			    ZVAL_NULL(pzval);
			}
		break;

		default: ZVAL_NULL(pzval);
	}

	return result;
}
/* }}} */

/* {{{ Will free store element */
static void pthreads_store_storage_dtor (pthreads_storage *storage){
	if (storage) {
		switch (storage->type) {
			case IS_CLOSURE:
			case IS_OBJECT:
			case IS_STRING:
			case IS_ARRAY:
			case IS_RESOURCE:
				if (storage->data) {
					free(storage->data);
				}
			break;
		}
		free(storage);
	}
} /* }}} */

/* {{{ */
int pthreads_store_separate(zval * pzval, zval *separated, zend_bool complex) {
	int result = FAILURE;
	pthreads_storage *storage;

	if (Z_TYPE_P(pzval) != IS_NULL) {
		storage = pthreads_store_create(pzval, complex);
		if (pthreads_store_convert(storage, separated) != SUCCESS) {
			ZVAL_NULL(separated);
		} else result = SUCCESS;
		pthreads_store_storage_dtor(storage);
	} else ZVAL_NULL(separated);

	return result;
} /* }}} */

#endif
