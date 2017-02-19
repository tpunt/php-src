<?php

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
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+

     $Id$
*/

const HEADER_TEXT = <<< DATA
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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+
*/


DATA;

error_reporting(E_ALL);

class ZendVmKind
{
    const CALL = 1;
    const SWITCH = 2;
    const GOTO = 3;

    const NAME = [
        ZendVmKind::CALL => 'ZEND_VM_KIND_CALL',
        ZendVmKind::SWITCH => 'ZEND_VM_KIND_SWITCH',
        ZendVmKind::GOTO => 'ZEND_VM_KIND_GOTO'
    ];
}

class VmOpFlags
{
    const ZEND_VM_OP_SPEC = 1 << 0;
    const ZEND_VM_OP_CONST = 1 << 1;
    const ZEND_VM_OP_TMPVAR = 1 << 2;
    const ZEND_VM_OP_TMPVARCV = 1 << 3;
    const ZEND_VM_OP_MASK = 0xf0;
    const ZEND_VM_OP_NUM = 0x10;
    const ZEND_VM_OP_JMP_ADDR = 0x20;
    const ZEND_VM_OP_TRY_CATCH = 0x30;
    const ZEND_VM_OP_LIVE_RANGE = 0x40;
    const ZEND_VM_OP_THIS = 0x50;
    const ZEND_VM_OP_NEXT = 0x60;
    const ZEND_VM_OP_CLASS_FETCH = 0x70;
    const ZEND_VM_OP_CONSTRUCTOR = 0x80;

    const ZEND_VM_EXT_VAR_FETCH = 1 << 16;
    const ZEND_VM_EXT_ISSET = 1 << 17;
    const ZEND_VM_EXT_ARG_NUM = 1 << 18;
    const ZEND_VM_EXT_ARRAY_INIT = 1 << 19;
    const ZEND_VM_EXT_REF = 1 << 20;
    const ZEND_VM_EXT_MASK = 0x0f000000;
    const ZEND_VM_EXT_NUM = 0x01000000;
    // unused 0x2000000
    const ZEND_VM_EXT_JMP_ADDR = 0x03000000;
    const ZEND_VM_EXT_DIM_OBJ = 0x04000000;
    const ZEND_VM_EXT_CLASS_FETCH = 0x05000000;
    const ZEND_VM_EXT_CONST_FETCH = 0x06000000;
    const ZEND_VM_EXT_TYPE = 0x07000000;
    const ZEND_VM_EXT_EVAL = 0x08000000;
    // unused 0x09000000
    // unused 0x0a000000
    const ZEND_VM_EXT_SRC = 0x0b000000;
    // unused 0x0c000000,
    const ZEND_VM_NO_CONST_CONST = 0x40000000;
    const ZEND_VM_COMMUTATIVE = 0x80000000;
}

class VmOpDecode
{
    const MAP = [
        'ANY'                  => 0,
        'CONST'                => VmOpFlags::ZEND_VM_OP_SPEC | VmOpFlags::ZEND_VM_OP_CONST,
        'TMP'                  => VmOpFlags::ZEND_VM_OP_SPEC,
        'VAR'                  => VmOpFlags::ZEND_VM_OP_SPEC,
        'UNUSED'               => VmOpFlags::ZEND_VM_OP_SPEC,
        'CV'                   => VmOpFlags::ZEND_VM_OP_SPEC,
        'TMPVAR'               => VmOpFlags::ZEND_VM_OP_SPEC | VmOpFlags::ZEND_VM_OP_TMPVAR,
        'TMPVARCV'             => VmOpFlags::ZEND_VM_OP_SPEC | VmOpFlags::ZEND_VM_OP_TMPVARCV,
        'NUM'                  => VmOpFlags::ZEND_VM_OP_NUM,
        'JMP_ADDR'             => VmOpFlags::ZEND_VM_OP_JMP_ADDR,
        'TRY_CATCH'            => VmOpFlags::ZEND_VM_OP_TRY_CATCH,
        'LIVE_RANGE'           => VmOpFlags::ZEND_VM_OP_LIVE_RANGE,
        'THIS'                 => VmOpFlags::ZEND_VM_OP_THIS,
        'NEXT'                 => VmOpFlags::ZEND_VM_OP_NEXT,
        'CLASS_FETCH'          => VmOpFlags::ZEND_VM_OP_CLASS_FETCH,
        'CONSTRUCTOR'          => VmOpFlags::ZEND_VM_OP_CONSTRUCTOR,
    ];
}

class VmExtDecode
{
    const MAP = [
        'NUM'                  => VmOpFlags::ZEND_VM_EXT_NUM,
        'JMP_ADDR'             => VmOpFlags::ZEND_VM_EXT_JMP_ADDR,
        'DIM_OBJ'              => VmOpFlags::ZEND_VM_EXT_DIM_OBJ,
        'CLASS_FETCH'          => VmOpFlags::ZEND_VM_EXT_CLASS_FETCH,
        'CONST_FETCH'          => VmOpFlags::ZEND_VM_EXT_CONST_FETCH,
        'VAR_FETCH'            => VmOpFlags::ZEND_VM_EXT_VAR_FETCH,
        'ARRAY_INIT'           => VmOpFlags::ZEND_VM_EXT_ARRAY_INIT,
        'TYPE'                 => VmOpFlags::ZEND_VM_EXT_TYPE,
        'EVAL'                 => VmOpFlags::ZEND_VM_EXT_EVAL,
        'ISSET'                => VmOpFlags::ZEND_VM_EXT_ISSET,
        'ARG_NUM'              => VmOpFlags::ZEND_VM_EXT_ARG_NUM,
        'REF'                  => VmOpFlags::ZEND_VM_EXT_REF,
        'SRC'                  => VmOpFlags::ZEND_VM_EXT_SRC,
    ];
}

class VmOps
{
    const TYPES = [
        'ANY',
        'CONST',
        'TMP',
        'VAR',
        'UNUSED',
        'CV'
    ];

    const EXTENDED_TYPES = [
        'ANY',
        'CONST',
        'TMP',
        'VAR',
        'UNUSED',
        'CV',
        'TMPVAR',
        'TMPVARCV'
    ];

    const PREFIXES = [
        'ANY'      => '',
        'TMP'      => '_TMP',
        'VAR'      => '_VAR',
        'CONST'    => '_CONST',
        'UNUSED'   => '_UNUSED',
        'CV'       => '_CV',
        'TMPVAR'   => '_TMPVAR',
        'TMPVARCV' => '_TMPVARCV'
    ];

    const COMMUTATIVE_ORDERS = [
        'ANY'      => 0,
        'TMP'      => 1,
        'VAR'      => 2,
        'CONST'    => 0,
        'UNUSED'   => 0,
        'CV'       => 4,
        'TMPVAR'   => 2,
        'TMPVARCV' => 4
    ];

    const OP1_TYPE = [
        'ANY'      => 'opline->op1_type',
        'TMP'      => 'IS_TMP_VAR',
        'VAR'      => 'IS_VAR',
        'CONST'    => 'IS_CONST',
        'UNUSED'   => 'IS_UNUSED',
        'CV'       => 'IS_CV',
        'TMPVAR'   => '(IS_TMP_VAR|IS_VAR)',
        'TMPVARCV' => '(IS_TMP_VAR|IS_VAR|IS_CV)'
    ];

    const OP2_TYPE = [
        'ANY'      => 'opline->op2_type',
        'TMP'      => 'IS_TMP_VAR',
        'VAR'      => 'IS_VAR',
        'CONST'    => 'IS_CONST',
        'UNUSED'   => 'IS_UNUSED',
        'CV'       => 'IS_CV',
        'TMPVAR'   => '(IS_TMP_VAR|IS_VAR)',
        'TMPVARCV' => '(IS_TMP_VAR|IS_VAR|IS_CV)'
    ];

    const OP1_FREE = [
        'ANY'      => '(free_op1 != NULL)',
        'TMP'      => '1',
        'VAR'      => '(free_op1 != NULL)',
        'CONST'    => '0',
        'UNUSED'   => '0',
        'CV'       => '0',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_FREE = [
        'ANY'      => '(free_op2 != NULL)',
        'TMP'      => '1',
        'VAR'      => '(free_op2 != NULL)',
        'CONST'    => '0',
        'UNUSED'   => '0',
        'CV'       => '0',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_ZVAL_PTR = [
        'ANY'      => 'get_zval_ptr(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1)',
        'VAR'      => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'EX_CONSTANT(opline->op1)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'TMPVARCV' => '???'
    ];

    const OP2_GET_ZVAL_PTR = [
        'ANY'      => 'get_zval_ptr(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op2.var, execute_data, &free_op2)',
        'VAR'      => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'EX_CONSTANT(opline->op2)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_ZVAL_PTR_PTR = [
        'ANY'      => 'get_zval_ptr_ptr(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'NULL',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_GET_ZVAL_PTR_PTR = [
        'ANY'      => 'get_zval_ptr_ptr(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'NULL',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_ZVAL_PTR_DEREF = [
        'ANY'      => 'get_zval_ptr_deref(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1)',
        'VAR'      => '_get_zval_ptr_var_deref(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'EX_CONSTANT(opline->op1)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_deref_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_GET_ZVAL_PTR_DEREF = [
        'ANY'      => 'get_zval_ptr_deref(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op2.var, execute_data, &free_op2)',
        'VAR'      => '_get_zval_ptr_var_deref(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'EX_CONSTANT(opline->op2)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_deref_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_ZVAL_PTR_UNDEF = [
        'ANY'      => 'get_zval_ptr_undef(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1)',
        'VAR'      => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'EX_CONSTANT(opline->op1)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_undef(execute_data, opline->op1.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'TMPVARCV' => 'EX_VAR(opline->op1.var)'
    ];

    const OP2_GET_ZVAL_PTR_UNDEF = [
        'ANY'      => 'get_zval_ptr_undef(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op2.var, execute_data, &free_op2)',
        'VAR'      => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'EX_CONSTANT(opline->op2)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_undef(execute_data, opline->op2.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'TMPVARCV' => 'EX_VAR(opline->op2.var)'
    ];

    const OP1_GET_ZVAL_PTR_PTR_UNDEF = [
        'ANY'      => 'get_zval_ptr_ptr_undef(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'NULL',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_undef_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => 'EX_VAR(opline->op1.var)'
    ];

    const OP2_GET_ZVAL_PTR_PTR_UNDEF = [
        'ANY'      => 'get_zval_ptr_ptr_undef(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'NULL',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_undef_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => 'EX_VAR(opline->op2.var)'
    ];

    const OP1_GET_OBJ_ZVAL_PTR = [
        'ANY'      => 'get_obj_zval_ptr(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1)',
        'VAR'      => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'EX_CONSTANT(opline->op1)',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'TMPVARCV' => '???'
    ];

    const OP2_GET_OBJ_ZVAL_PTR = [
        'ANY'      => 'get_obj_zval_ptr(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op2.var, execute_data, &free_op2)',
        'VAR'      => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'EX_CONSTANT(opline->op2)',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_OBJ_ZVAL_PTR_UNDEF = [
        'ANY'      => 'get_obj_zval_ptr_undef(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1)',
        'VAR'      => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'EX_CONSTANT(opline->op1)',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_undef(execute_data, opline->op1.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'TMPVARCV' => 'EX_VAR(opline->op1.var)'
    ];

    const OP2_GET_OBJ_ZVAL_PTR_UNDEF = [
        'ANY'      => 'get_obj_zval_ptr_undef(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op2.var, execute_data, &free_op2)',
        'VAR'      => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'EX_CONSTANT(opline->op2)',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_undef(execute_data, opline->op2.var)',
        'TMPVAR'   => '_get_zval_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'TMPVARCV' => 'EX_VAR(opline->op2.var)'
    ];

    const OP1_GET_OBJ_ZVAL_PTR_DEREF = [
        'ANY'      => 'get_obj_zval_ptr(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1)',
        'VAR'      => '_get_zval_ptr_var_deref(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'EX_CONSTANT(opline->op1)',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_deref_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_GET_OBJ_ZVAL_PTR_DEREF = [
        'ANY'      => 'get_obj_zval_ptr(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => '_get_zval_ptr_tmp(opline->op2.var, execute_data, &free_op2)',
        'VAR'      => '_get_zval_ptr_var_deref(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'EX_CONSTANT(opline->op2)',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_deref_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_OBJ_ZVAL_PTR_PTR = [
        'ANY'      => 'get_obj_zval_ptr_ptr(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'NULL',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_GET_OBJ_ZVAL_PTR_PTR = [
        'ANY'      => 'get_obj_zval_ptr_ptr(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'NULL',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_GET_OBJ_ZVAL_PTR_PTR_UNDEF = [
        'ANY'      => 'get_obj_zval_ptr_ptr(opline->op1_type, opline->op1, execute_data, &free_op1, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op1.var, execute_data, &free_op1)',
        'CONST'    => 'NULL',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_undef_\\1(execute_data, opline->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => 'EX_VAR(opline->op1.var)'
    ];

    const OP2_GET_OBJ_ZVAL_PTR_PTR_UNDEF = [
        'ANY'      => 'get_obj_zval_ptr_ptr(opline->op2_type, opline->op2, execute_data, &free_op2, \\1)',
        'TMP'      => 'NULL',
        'VAR'      => '_get_zval_ptr_ptr_var(opline->op2.var, execute_data, &free_op2)',
        'CONST'    => 'NULL',
        'UNUSED'   => '_get_obj_zval_ptr_unused(execute_data)',
        'CV'       => '_get_zval_ptr_cv_undef_\\1(execute_data, opline->op2.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => 'EX_VAR(opline->op2.var)'
    ];

    const OP1_FREE_OP = [
        'ANY'      => 'FREE_OP(free_op1)',
        'TMP'      => 'zval_ptr_dtor_nogc(free_op1)',
        'VAR'      => 'zval_ptr_dtor_nogc(free_op1)',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => 'zval_ptr_dtor_nogc(free_op1)',
        'TMPVARCV' => '???'
    ];

    const OP2_FREE_OP = [
        'ANY'      => 'FREE_OP(free_op2)',
        'TMP'      => 'zval_ptr_dtor_nogc(free_op2)',
        'VAR'      => 'zval_ptr_dtor_nogc(free_op2)',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => 'zval_ptr_dtor_nogc(free_op2)',
        'TMPVARCV' => '???'
    ];

    const OP1_FREE_OP_IF_VAR = [
        'ANY'      => 'if (opline->op1_type == IS_VAR) {zval_ptr_dtor_nogc(free_op1);}',
        'TMP'      => '',
        'VAR'      => 'zval_ptr_dtor_nogc(free_op1)',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_FREE_OP_IF_VAR = [
        'ANY'      => 'if (opline->op2_type == IS_VAR) {zval_ptr_dtor_nogc(free_op2);}',
        'TMP'      => '',
        'VAR'      => 'zval_ptr_dtor_nogc(free_op2)',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_FREE_OP_VAR_PTR = [
        'ANY'      => 'if (free_op1) {zval_ptr_dtor_nogc(free_op1);}',
        'TMP'      => '',
        'VAR'      => 'if (UNEXPECTED(free_op1)) {zval_ptr_dtor_nogc(free_op1);}',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP2_FREE_OP_VAR_PTR = [
        'ANY'      => 'if (free_op2) {zval_ptr_dtor_nogc(free_op2);}',
        'TMP'      => '',
        'VAR'      => 'if (UNEXPECTED(free_op2)) {zval_ptr_dtor_nogc(free_op2);}',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP1_FREE_UNFETCHED = [
        'ANY'      => 'FREE_UNFETCHED_OP(opline->op1_type, opline->op1.var)',
        'TMP'      => 'zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))',
        'VAR'      => 'zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => 'zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))',
        'TMPVARCV' => '???'
    ];

    const OP2_FREE_UNFETCHED = [
        'ANY'      => 'FREE_UNFETCHED_OP(opline->op2_type, opline->op2.var)',
        'TMP'      => 'zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))',
        'VAR'      => 'zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => 'zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))',
        'TMPVARCV' => '???'
    ];

    const OP_DATA_TYPE = [
        'ANY'      => '(opline+1)->op1_type',
        'TMP'      => 'IS_TMP_VAR',
        'VAR'      => 'IS_VAR',
        'CONST'    => 'IS_CONST',
        'UNUSED'   => 'IS_UNUSED',
        'CV'       => 'IS_CV',
        'TMPVAR'   => '(IS_TMP_VAR|IS_VAR)',
        'TMPVARCV' => '(IS_TMP_VAR|IS_VAR|IS_CV)'
    ];

    const OP_DATA_GET_ZVAL_PTR = [
        'ANY'      => 'get_zval_ptr((opline+1)->op1_type, (opline+1)->op1, execute_data, &free_op_data, \\1)',
        'TMP'      => '_get_zval_ptr_tmp((opline+1)->op1.var, execute_data, &free_op_data)',
        'VAR'      => '_get_zval_ptr_var((opline+1)->op1.var, execute_data, &free_op_data)',
        'CONST'    => 'EX_CONSTANT((opline+1)->op1)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_\\1(execute_data, (opline+1)->op1.var)',
        'TMPVAR'   => '_get_zval_ptr_var((opline+1)->op1.var, execute_data, &free_op_data)',
        'TMPVARCV' => '???'
    ];

    const OP_DATA_GET_ZVAL_PTR_DEREF = [
        'ANY'      => 'get_zval_ptr((opline+1)->op1_type, (opline+1)->op1, execute_data, &free_op_data, \\1)',
        'TMP'      => '_get_zval_ptr_tmp((opline+1)->op1.var, execute_data, &free_op_data)',
        'VAR'      => '_get_zval_ptr_var_deref((opline+1)->op1.var, execute_data, &free_op_data)',
        'CONST'    => 'EX_CONSTANT((opline+1)->op1)',
        'UNUSED'   => 'NULL',
        'CV'       => '_get_zval_ptr_cv_deref_\\1(execute_data, (opline+1)->op1.var)',
        'TMPVAR'   => '???',
        'TMPVARCV' => '???'
    ];

    const OP_DATA_FREE_OP = [
        'ANY'      => 'FREE_OP(free_op_data)',
        'TMP'      => 'zval_ptr_dtor_nogc(free_op_data)',
        'VAR'      => 'zval_ptr_dtor_nogc(free_op_data)',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => 'zval_ptr_dtor_nogc(free_op_data)',
        'TMPVARCV' => '???'
    ];

    const OP_DATA_FREE_UNFETCHED = [
        'ANY'      => 'FREE_UNFETCHED_OP((opline+1)->op1_type, (opline+1)->op1.var)',
        'TMP'      => 'zval_ptr_dtor_nogc(EX_VAR((opline+1)->op1.var))',
        'VAR'      => 'zval_ptr_dtor_nogc(EX_VAR((opline+1)->op1.var))',
        'CONST'    => '',
        'UNUSED'   => '',
        'CV'       => '',
        'TMPVAR'   => 'zval_ptr_dtor_nogc(EX_VAR((opline+1)->op1.var))',
        'TMPVARCV' => '???'
    ];
}

class DefinitionFileParser
{
    private $lineNo = 1;
    private $opnames = []; // opcode name to code mapping
    private $opcodes = []; // opcode handlers by code
    private $handler = null;
    private $helper = null;
    private $helpers = []; // opcode helpers by name
    private $list = []; // list of opcode handlers and helpers in original order
    private $maxOpcodeNameLen = 0;
    private $maxOpcode = 0;
    private $definitionFile = '';
    private $usedExtraSpec = [];
    private $params = [];

    public function getParams() : array
    {
        return $this->params;
    }

    public function getMaxOpcodeNameLen() : int
    {
        return $this->maxOpcodeNameLen;
    }

    public function getOpcodes() : array
    {
        return $this->opcodes;
    }

    public function getOpnames() : array
    {
        return $this->opnames;
    }

    public function getMaxOpcode() : int
    {
        return $this->maxOpcode;
    }

    public function getList() : array
    {
        return $this->list;
    }

    public function getHelpers() : array
    {
        return $this->helpers;
    }

    public function getDefinitionFile() : string
    {
        return $this->definitionFile;
    }

    public function getUsedExtraSpec() : array
    {
        return $this->usedExtraSpec;
    }

    public function __construct(string $vmDef)
    {
        $input = @file($vmDef) or die("ERROR: Can not open definition file '{$vmDef}'\n");
        $this->definitionFile = realpath($vmDef);

        $this->parse($input);
    }

    private function parse(array $input) : void
    {
        foreach ($input as $line) {
            if (strpos($line, 'ZEND_VM_HANDLER(') === 0) {
                $this->parseZendVmHandler($line);
            } elseif (strpos($line, 'ZEND_VM_TYPE_SPEC_HANDLER(') === 0) {
                $this->parseZendVmTypeSpecHandler($line);
            } elseif (strpos($line, 'ZEND_VM_HELPER(') === 0 || strpos($line, 'ZEND_VM_INLINE_HELPER(') === 0) {
                $this->parseZendVmHelper($line);
            } elseif (strpos($line,'ZEND_VM_DEFINE_OP(') === 0) {
                $this->parseZendVmDefineOp($line);
            } elseif ($this->handler !== null) {
                $this->opcodes[$this->handler]['code'] .= $line; // Add line of code to current opcode handler
            } elseif ($this->helper !== null) {
            	$this->helpers[$this->helper]['code'] .= $line; // Add line of code to current helper
            }
            ++$this->lineNo;
        }

        $this->verifyDispatchHandlerOpcodesValidity();
    }

    private function verifyDispatchHandlerOpcodesValidity() : void
    {
        ksort($this->opcodes);

        foreach ($this->opcodes as $dsc) {
            if (preg_match('~ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)~m', $dsc['code'], $m)) {
                $op = $m[1];
                if (!isset($this->opnames[$op])) {
                    die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with name '{$op}' is not defined.\n");
                }
                $code = $this->opnames[$op];
                $this->opcodes[$code]['use'] = 1;
            }
        }
    }

    private function parseOperandSpec(string $spec, &$flags) : array
    {
        $flags = 0;
        $a = explode('|', $spec);

        foreach ($a as $val) {
            if (isset(VmOpDecode::MAP[$val])) {
                $flags |= VmOpDecode::MAP[$val];
            } else {
                die("ERROR ({$this->definitionFile}:{$this->lineNo}): Wrong operand type '{$spec}'\n");
            }
        }

        if (!($flags & VmOpFlags::ZEND_VM_OP_SPEC)) {
            if (count($a) !== 1) {
                die("ERROR ({$this->definitionFile}:{$this->lineNo}): Wrong operand type '{$spec}'\n");
            }
            $a = ['ANY'];
        }

        return array_flip($a);
    }

    private function parseExtSpec(string $spec) : int
    {
        $flags = 0;
        $a = explode('|', $spec);

        foreach ($a as $val) {
            if (!isset(VmExtDecode::MAP[$val])) {
                die("ERROR ({$this->definitionFile}:{$this->lineNo}): Wrong extended_value type '{$spec}'\n");
            }
            $flags |= VmExtDecode::MAP[$val];
        }

        return $flags;
    }

    private function parseSpecRules(string $spec) : array
    {
        $ret = [];
        $a = explode(',', $spec);

        foreach ($a as $rule) {
            $n = strpos($rule, '=');

            if ($n !== false) {
                $id = trim(substr($rule, 0, $n));
                $val = trim(substr($rule, $n + 1));

                if ($id !== 'OP_DATA') {
                    throw new Exception("ERROR ({$this->definitionFile}:{$this->lineNo}): Wrong specialization rules '{$spec}'");
                }

                $ret['OP_DATA'] = $this->parseOperandSpec($val, $devnull);
                $this->usedExtraSpec[$id] = 1;
            } else {
                switch ($rule) {
                    case 'RETVAL':
                    case 'QUICK_ARG':
                        $ret[$rule] = [0, 1];
                        break;
                    case 'SMART_BRANCH':
                    case 'DIM_OBJ':
                        $ret[$rule] = [0, 1, 2];
                        break;
                    case 'NO_CONST_CONST':
                    case 'COMMUTATIVE':
                        $ret[$rule] = [1];
                        break;
                    default:
                        throw new Exception("ERROR ({$this->definitionFile}:{$this->lineNo}): Wrong specialization rules '{$spec}'");
                }
                $this->usedExtraSpec[$rule] = 1;
            }
        }

        return $ret;
    }

    // Parsing opcode handler's definition
    private function parseZendVmHandler(string $line) : void
    {
        $matchCount = preg_match(
            '~^ZEND_VM_HANDLER\(
                \s*([0-9]+)\s*,                 # ... m[1]
                \s*([A-Z_]+)\s*,              	# ... m[2]
                \s*([A-Z_|]+)\s*,               # ... m[3]
                \s*([A-Z_|]+)\s*                # ... m[4]
                (,\s*([A-Z_|]+)\s*)?            # ... m[5] m[6]
                (,\s*SPEC\(([A-Z_|=,]+)\)\s*)?  # ... m[7] m[8] m[9]
            \)~x', $line, $m);

        if ($matchCount === 0) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Invalid ZEND_VM_HANDLER definition.\n");
        }

        $code = (int) $m[1];
        $op = $m[2];
        $opcodeNameLen = strlen($op);
        $op1 = $this->parseOperandSpec($m[3], $flags1);
        $op2 = $this->parseOperandSpec($m[4], $flags2);
        $flags = $flags1 | ($flags2 << 8);

        if (!empty($m[6])) {
            $flags |= $this->parseExtSpec($m[6]);
        }

        if ($opcodeNameLen > $this->maxOpcodeNameLen) {
            $this->maxOpcodeNameLen = $opcodeNameLen;
        }

        if ($code > $this->maxOpcode) {
            $this->maxOpcode = $code;
        }

        if (isset($this->opcodes[$code])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with code '{$code}' is already defined.\n");
        }

        if (isset($this->opnames[$op])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with name '{$op}' is already defined.\n");
        }

        $this->opcodes[$code] = ['op' => $op, 'op1' => $op1, 'op2' => $op2, 'code' => '', 'flags' => $flags];

        if (isset($m[8])) {
            $this->opcodes[$code]['spec'] = $this->parseSpecRules($m[8]);

            if (isset($this->opcodes[$code]['spec']['NO_CONST_CONST'])) {
                $this->opcodes[$code]['flags'] |= VmOpFlags::ZEND_VM_NO_CONST_CONST;
            }

            if (isset($this->opcodes[$code]['spec']['COMMUTATIVE'])) {
                $this->opcodes[$code]['flags'] |= VmOpFlags::ZEND_VM_COMMUTATIVE;
            }
        }

        $this->opnames[$op] = $code;
        $this->handler = $code;
        $this->helper = null;
        $this->list[$this->lineNo] = ['handler' => $this->handler];
    }

    // Parsing opcode handler's definition
    private function parseZendVmTypeSpecHandler(string $line) : void
    {
        static $extraNum = 256;

        $matchCount = preg_match(
            '~^ZEND_VM_TYPE_SPEC_HANDLER\(
                \s*([A-Z_]+)\s*,                # opname (m[1])
                \s*([^,]+),                     # condition (m[2])
                \s*([A-Za-z_]+)\s*,             # op (m[3])
                \s*([A-Z_|]+)\s*,               # op1 types (m[4])
                \s*([A-Z_|]+)\s*              	# op2 types (m[5])
                (?:,\s*([A-Z_|]+)\s*)?          # ? (m[6])
                (,\s*SPEC\(([A-Z_|=,]+)\)\s*)?  # spec (m[8])
            \)~x', $line, $m);

        if ($matchCount === 0) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Invalid ZEND_VM_TYPE_HANDLER_HANDLER definition.\n");
        }

        $opname = $m[1];
        $condition = $m[2];
        $op = $m[3];
        $op1_types = $m[4];
        $op2_types = $m[5];

        if (!isset($this->opnames[$opname])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with name '{$opname}' is not defined.\n");
        }

        $orig_code = $this->opnames[$opname];
        $code = $extraNum++;

        $op1 = $this->parseOperandSpec($op1_types, $flags1);
        $op2 = $this->parseOperandSpec($op2_types, $flags2);
        $flags = $flags1 | ($flags2 << 8);

        if (!empty($m[6])) {
            $flags |= $this->parseExtSpec($m[6]);
        }

        if (isset($this->opcodes[$code])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with name '{$code}' is already defined.\n");
        }

        $this->opcodes[$orig_code]['type_spec'][$code] = $condition;
        $this->usedExtraSpec['TYPE'] = 1;
        $this->opcodes[$code] = ['op' => $op, 'op1' => $op1, 'op2' => $op2, 'code' => '', 'flags' => $flags];

        if (isset($m[8])) {
            $this->opcodes[$code]['spec'] = $this->parseSpecRules($m[8]);

            if (isset($this->opcodes[$code]['spec']['NO_CONST_CONST'])) {
                $this->opcodes[$code]['flags'] |= VmOpFlags::ZEND_VM_NO_CONST_CONST;
            }

            if (isset($this->opcodes[$code]['spec']['COMMUTATIVE'])) {
                $this->opcodes[$code]['flags'] |= VmOpFlags::ZEND_VM_COMMUTATIVE;
            }
        }

        $this->opnames[$op] = $code;
        $this->handler = $code;
        $this->helper = null;
        $this->list[$this->lineNo] = ['handler' => $this->handler];
    }

    // Parsing helper's definition
    private function parseZendVmHelper(string $line) : void
    {
        $matchCount = preg_match(
            '~^ZEND_VM(_INLINE)?_HELPER\(
                \s*([A-Za-z_]+)\s*,                 # inline function name (m[1])
                \s*([A-Z_|]+)\s*,                   # op1 types (m[2])
                \s*([A-Z_|]+)\s*                	# op2 types (m[3])
                (?:,\s*SPEC\(([A-Z_|=,]+)\)\s*)?    # spec (m[4])
                (?:,\s*([^)]*)\s*)?                 # overloaded type / offset information (m[5], m[6])
            \)~x', $line, $m);

        if ($matchCount === 0) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Invalid ZEND_VM_HELPER definition.\n");
        }

        $inline = !empty($m[1]);
        $this->helper = $m[2];
        $op1 = $this->parseOperandSpec($m[3], $flags1);
        $op2 = $this->parseOperandSpec($m[4], $flags2);
        $param = $m[6] ?? null;

        if (isset($this->helpers[$this->helper])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Helper with name '{$this->helper}' is already defined.\n");
        }

        // Store parameters
        foreach (explode(',', $param) as $p) {
            $p = trim($p);

            if ($p !== '') {
                $this->params[$p] = 1;
            }
        }

        $this->helpers[$this->helper] = ['op1' => $op1, 'op2' => $op2, 'param' => $param, 'code' => '', 'inline' => $inline];

        if (!empty($m[5])) {
            $this->helpers[$this->helper]["spec"] = $this->parseSpecRules($m[5]);
        }

        $this->handler = null;
        $this->list[$this->lineNo] = ['helper' => $this->helper];
    }

    private function parseZendVmDefineOp(string $line) : void
    {
        $matchCount = preg_match(
            '~^ZEND_VM_DEFINE_OP\(
                \s*([0-9]+)\s*,        # opcode number (m[1])
                \s*([A-Z_]+)\s*        # opname (m[2])
            \);~x', $line, $m);

        if ($matchCount === 0) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Invalid ZEND_VM_DEFINE_OP definition.\n");
        }

        $code = (int)$m[1];
        $op = $m[2];
        $opcodeNameLen = strlen($op);

        if ($opcodeNameLen > $this->maxOpcodeNameLen) {
            $this->maxOpcodeNameLen = $opcodeNameLen;
        }

        if ($code > $this->maxOpcode) {
            $this->maxOpcode = $code;
        }

        if (isset($this->opcodes[$code])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with code '{$code}' is already defined.\n");
        }

        if (isset($this->opnames[$op])) {
            die("ERROR ({$this->definitionFile}:{$this->lineNo}): Opcode with name '{$op}' is already defined.\n");
        }

        $this->opcodes[$code] = ['op' => $op, 'code' => ''];
        $this->opnames[$op] = $code;
    }
}

class OpcodesHeaderFileGen
{
	private $fw;
	private $opcodes;
	private $maxOpcode;
	private $maxOpcodeNameLen;

	public function __construct(DefinitionFileParser $dfp, FileWriter $fw)
	{
		$this->fw = $fw;
		$this->opcodes = $dfp->getOpcodes();
		$this->maxOpcode = $dfp->getMaxOpcode();
		$this->maxOpcodeNameLen = $dfp->getMaxOpcodeNameLen();

		$this->generateOpcodesHeaderFile();
	}

	private function generateOpcodesHeaderFile() : void
    {
        $maxOpcodeCodeLength = strlen((string) $this->maxOpcode);

        $this->fw->out(HEADER_TEXT);
        $this->fw->write("#ifndef ZEND_VM_OPCODES_H\n#define ZEND_VM_OPCODES_H\n\n");
        $this->fw->write("#define ZEND_VM_SPEC\t\t" . ZEND_VM_SPEC . "\n");
        $this->fw->write("#define ZEND_VM_LINES\t\t" . ZEND_VM_LINES . "\n");
        $this->fw->write("#define ZEND_VM_KIND_CALL\t" . ZendVmKind::CALL . "\n");
        $this->fw->write("#define ZEND_VM_KIND_SWITCH\t" . ZendVmKind::SWITCH . "\n");
        $this->fw->write("#define ZEND_VM_KIND_GOTO\t" . ZendVmKind::GOTO . "\n");
        $this->fw->write("#define ZEND_VM_KIND\t\t" . ZendVmKind::NAME[ZEND_VM_KIND] . "\n");
        $this->fw->write("\n");

        foreach ((new ReflectionClass(VmOpFlags::class))->getConstants() as $name => $val) {
            $this->fw->writef("#define %-24s 0x%08x\n", $name, $val);
        }

        $this->fw->write("#define ZEND_VM_OP1_FLAGS(flags) (flags & 0xff)\n");
        $this->fw->write("#define ZEND_VM_OP2_FLAGS(flags) ((flags >> 8) & 0xff)\n");
        $this->fw->write("\n");
        $this->fw->write("BEGIN_EXTERN_C()\n\n");
        $this->fw->write("ZEND_API const char *zend_get_opcode_name(zend_uchar opcode);\n");
        $this->fw->write("ZEND_API uint32_t zend_get_opcode_flags(zend_uchar opcode);\n\n");
        $this->fw->write("END_EXTERN_C()\n\n");

        foreach ($this->opcodes as $code => $dsc) {
            $code = str_pad((string) $code, $maxOpcodeCodeLength, ' ', STR_PAD_LEFT);
            $op = str_pad($dsc['op'], $this->maxOpcodeNameLen);

            if ($code <= $this->maxOpcode) {
                $this->fw->write("#define {$op} {$code}\n");
            }
        }

        $code = str_pad((string) $this->maxOpcode, $maxOpcodeCodeLength, ' ', STR_PAD_LEFT);
        $op = str_pad('ZEND_VM_LAST_OPCODE', $this->maxOpcodeNameLen);

        $this->fw->write("\n#define {$op} {$code}\n");
        $this->fw->write("\n#endif\n");

        echo "zend_vm_opcodes.h generated successfully.\n";
    }
}

class OpcodesMainFileGen
{
	private $fw;
	private $opcodes;
	private $maxOpcode;

	public function __construct(DefinitionFileParser $dfp, FileWriter $fw)
	{
		$this->fw = $fw;
		$this->opcodes = $dfp->getOpcodes();
		$this->maxOpcode = $dfp->getMaxOpcode();

		$this->generateOpcodesMainFile();
	}

	private function generateOpcodesMainFile() : void
    {
        $this->fw->nout(HEADER_TEXT); // see nout function comment
        $this->fw->out(HEADER_TEXT);

        $this->fw->write("#include <stdio.h>\n");
        $this->fw->write("#include <zend.h>\n\n");

        $this->fw->write('static const char *zend_vm_opcodes_names[' . ($this->maxOpcode + 1) . "] = {\n");
        for ($i = 0; $i <= $this->maxOpcode; ++$i) {
            $this->fw->write("\t" . (isset($this->opcodes[$i]['op']) ? '"' . $this->opcodes[$i]['op'] . '"' : 'NULL') . ",\n");
        }
        $this->fw->write("};\n\n");

        $this->fw->write('static uint32_t zend_vm_opcodes_flags[' . ($this->maxOpcode + 1) . "] = {\n");
        for ($i = 0; $i <= $this->maxOpcode; $i++) {
            $this->fw->writef("\t0x%08x,\n", $this->opcodes[$i]["flags"] ?? 0);
        }
        $this->fw->write("};\n\n");

        $this->fw->write("ZEND_API const char* zend_get_opcode_name(zend_uchar opcode) {\n");
        $this->fw->write("\treturn zend_vm_opcodes_names[opcode];\n");
        $this->fw->write("}\n");

        $this->fw->write("ZEND_API uint32_t zend_get_opcode_flags(zend_uchar opcode) {\n");
        $this->fw->write("\treturn zend_vm_opcodes_flags[opcode];\n");
        $this->fw->write("}\n");

        echo "zend_vm_opcodes.c generated successfully.\n";
    }
}

class ExecuteFileGen
{
    private $dfp;
	private $fw;
    private $skel;
    private $definitionFile;
    private $skeletonFile;

    private $opcodes;

    public function __construct(DefinitionFileParser $dfp, string $vmSkel, FileWriter $fw)
    {
        $this->dfp = $dfp;
		$this->fw = $fw;
        $this->skel = @file($vmSkel) or die("ERROR: Can not open skeleton file '{$skel}'\n");
        $this->skeletonFile = realpath($vmSkel);

        $this->opcodes = $dfp->getOpcodes(); // store locally, since it is updated in this stage too

        $this->generateExecuteFile();
    }

    private function generateExecuteFile() : void
    {
        $opnames = $this->dfp->getOpnames();
        $helpers = $this->dfp->getHelpers();

        $this->fw->nout(HEADER_TEXT); // see nout function comment
        $this->fw->nout(HEADER_TEXT); // see nout function comment
        $this->fw->out(HEADER_TEXT);

        $this->fw->out("#ifdef ZEND_WIN32\n");
        // Suppress free_op1 warnings on Windows
        $this->fw->out("# pragma warning(disable : 4101)\n");
        if (ZEND_VM_SPEC) {
            // Suppress (<non-zero constant> || <expression>) warnings on windows
            $this->fw->out("# pragma warning(once : 6235)\n");
            // Suppress (<zero> && <expression>) warnings on windows
            $this->fw->out("# pragma warning(once : 6237)\n");
            // Suppress (<non-zero constant> && <expression>) warnings on windows
            $this->fw->out("# pragma warning(once : 6239)\n");
            // Suppress (<expression> && <non-zero constant>) warnings on windows
            $this->fw->out("# pragma warning(once : 6240)\n");
            // Suppress (<non-zero constant> || <non-zero constant>) warnings on windows
            $this->fw->out("# pragma warning(once : 6285)\n");
            // Suppress (<non-zero constant> || <expression>) warnings on windows
            $this->fw->out("# pragma warning(once : 6286)\n");
            // Suppress constant with constant comparison warnings on windows
            $this->fw->out("# pragma warning(once : 6326)\n");
        }
        $this->fw->out("#endif\n");

        // Support for ZEND_USER_OPCODE
        $this->fw->out("static user_opcode_handler_t zend_user_opcode_handlers[256] = {\n");
        for ($i = 0; $i < 255; ++$i) {
            $this->fw->out("\t(user_opcode_handler_t)NULL,\n");
        }
        $this->fw->out("\t(user_opcode_handler_t)NULL\n};\n\n");

        $this->fw->out("static zend_uchar zend_user_opcodes[256] = {");
        for ($i = 0; $i < 255; ++$i) {
            if ($i % 16 === 1) {
                $this->fw->out("\n\t");
            }
            $this->fw->out("$i,");
        }
        $this->fw->out("255\n};\n\n");

        $this->genExecutor();

        // Generate zend_vm_get_opcode_handler() function
        $this->fw->out("static const void *zend_vm_get_opcode_handler_ex(uint32_t spec, const zend_op* op)\n");
        $this->fw->out("{\n");

        $usedExtraSpec = $this->dfp->getUsedExtraSpec();

        if (!ZEND_VM_SPEC) {
            $this->fw->out("\treturn zend_opcode_handlers[spec];\n");
        } else {
            $this->fw->out("\tstatic const int zend_vm_decode[] = {\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 0              */\n");
            $this->fw->out("\t\t_CONST_CODE,  /* 1 = IS_CONST   */\n");
            $this->fw->out("\t\t_TMP_CODE,    /* 2 = IS_TMP_VAR */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 3              */\n");
            $this->fw->out("\t\t_VAR_CODE,    /* 4 = IS_VAR     */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 5              */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 6              */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 7              */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 8 = IS_UNUSED  */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 9              */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 10             */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 11             */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 12             */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 13             */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 14             */\n");
            $this->fw->out("\t\t_UNUSED_CODE, /* 15             */\n");
            $this->fw->out("\t\t_CV_CODE      /* 16 = IS_CV     */\n");
            $this->fw->out("\t};\n");
            $this->fw->out("\tuint32_t offset = 0;\n");
            $this->fw->out("\tif (spec & SPEC_RULE_OP1) offset = offset * 5 + zend_vm_decode[op->op1_type];\n");
            $this->fw->out("\tif (spec & SPEC_RULE_OP2) offset = offset * 5 + zend_vm_decode[op->op2_type];\n");

            if (isset($usedExtraSpec['OP_DATA'])) {
                $this->fw->out("\tif (spec & SPEC_RULE_OP_DATA) offset = offset * 5 + zend_vm_decode[(op + 1)->op1_type];\n");
            }

            if (isset($usedExtraSpec['RETVAL'])) {
                $this->fw->out("\tif (spec & SPEC_RULE_RETVAL) offset = offset * 2 + (op->result_type != IS_UNUSED);\n");
            }

            if (isset($usedExtraSpec['QUICK_ARG'])) {
                $this->fw->out("\tif (spec & SPEC_RULE_QUICK_ARG) offset = offset * 2 + (op->op2.num < MAX_ARG_FLAG_NUM);\n");
            }

            if (isset($usedExtraSpec['SMART_BRANCH'])) {
                $this->fw->out("\tif (spec & SPEC_RULE_SMART_BRANCH) {\n");
                $this->fw->out("\t\toffset = offset * 3;\n");
                $this->fw->out("\t\tif ((op+1)->opcode == ZEND_JMPZ) {\n");
                $this->fw->out("\t\t\toffset += 1;\n");
                $this->fw->out("\t\t} else if ((op+1)->opcode == ZEND_JMPNZ) {\n");
                $this->fw->out("\t\t\toffset += 2;\n");
                $this->fw->out("\t\t}\n");
                $this->fw->out("\t}\n");
            }

            if (isset($usedExtraSpec['DIM_OBJ'])) {
                $this->fw->out("\tif (spec & SPEC_RULE_DIM_OBJ) {\n");
                $this->fw->out("\t\toffset = offset * 3;\n");
                $this->fw->out("\t\tif (op->extended_value == ZEND_ASSIGN_DIM) {\n");
                $this->fw->out("\t\t\toffset += 1;\n");
                $this->fw->out("\t\t} else if (op->extended_value == ZEND_ASSIGN_OBJ) {\n");
                $this->fw->out("\t\t\toffset += 2;\n");
                $this->fw->out("\t\t}\n");
                $this->fw->out("\t}\n");
            }

            $this->fw->out("\treturn zend_opcode_handlers[(spec & SPEC_START_MASK) + offset];\n");
        }

        $this->fw->out("}\n\n");
        $this->fw->out("static const void *zend_vm_get_opcode_handler(zend_uchar opcode, const zend_op* op)\n");
        $this->fw->out("{\n");

        if (!ZEND_VM_SPEC) {
            $this->fw->out("\treturn zend_vm_get_opcode_handler_ex(opcode, op);\n");
        } else {
            $this->fw->out("\treturn zend_vm_get_opcode_handler_ex(zend_spec_handlers[opcode], op);\n");
        }

        $this->fw->out("}\n\n");

        // Generate zend_vm_get_opcode_handler() function
        $this->fw->out("ZEND_API void zend_vm_set_opcode_handler(zend_op* op)\n");
        $this->fw->out("{\n");
        $this->fw->out("\top->handler = zend_vm_get_opcode_handler(zend_user_opcodes[op->opcode], op);\n");
        $this->fw->out("}\n\n");

        // Generate zend_vm_set_opcode_handler_ex() function
        $this->fw->out("ZEND_API void zend_vm_set_opcode_handler_ex(zend_op* op, uint32_t op1_info, uint32_t op2_info, uint32_t res_info)\n");
        $this->fw->out("{\n");
        $this->fw->out("\tzend_uchar opcode = zend_user_opcodes[op->opcode];\n");

        if (!ZEND_VM_SPEC) {
            $this->fw->out("\top->handler = zend_vm_get_opcode_handler_ex(opcode, op);\n");
        } else {
            $this->fw->out("\tuint32_t spec = zend_spec_handlers[opcode];\n");

            if (isset($usedExtraSpec['TYPE'])) {
                $this->fw->out("\tswitch (opcode) {\n");

                foreach ($this->opcodes as $code => $dsc) {
                    if (isset($dsc['type_spec'])) {
                        $orig_op = $dsc['op'];
                        $this->fw->out("\t\tcase {$orig_op}:\n");
                        $first = true;

                        foreach ($dsc['type_spec'] as $code => $condition) {
							if ($condition[0] !== '(' || $condition[strlen($condition) - 1] !== ')') {
								$condition = "({$condition})";
							}

                            if ($first) {
                                $this->fw->out("\t\t\tif {$condition} {\n");
                                $first = false;
                            } else {
                                $this->fw->out("\t\t\t} else if {$condition} {\n");
                            }

                            $spec_dsc = $this->opcodes[$code];

                            if (isset($spec_dsc['spec']['NO_CONST_CONST'])) {
                                $this->fw->out("\t\t\t\tif (op->op1_type == IS_CONST && op->op2_type == IS_CONST) {\n");
                                $this->fw->out("\t\t\t\t\tbreak;\n");
                                $this->fw->out("\t\t\t\t}\n");
                            }

                            $this->fw->out("\t\t\t\tspec = {$spec_dsc['spec_code']};\n");

                            if (isset($spec_dsc['spec']['COMMUTATIVE'])) {
                                $this->fw->out("\t\t\t\tif (op->op1_type > op->op2_type) {\n");
                                $this->fw->out("\t\t\t\t\tzend_swap_operands(op);\n");
                                $this->fw->out("\t\t\t\t}\n");
                            }
                        }

                        if (!$first) {
                            $this->fw->out("\t\t\t}\n");
                        }

                        $this->fw->out("\t\t\tbreak;\n");
                    }
                }

                $this->fw->out("\t\tdefault:\n");
                $this->fw->out("\t\t\tbreak;\n");
                $this->fw->out("\t}\n");
            }

            $this->fw->out("\top->handler = zend_vm_get_opcode_handler_ex(spec, op);\n");
        }

        $this->fw->out("}\n\n");

        // Generate zend_vm_call_opcode_handler() function
        if (ZEND_VM_KIND === ZendVmKind::CALL) {
            $this->fw->out("ZEND_API int zend_vm_call_opcode_handler(zend_execute_data* ex)\n");
            $this->fw->out("{\n");
            $this->fw->out("\tint ret;\n");
            $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
            $this->fw->out("\tconst zend_op *orig_opline = opline;\n");
            $this->fw->out("#endif\n");
            $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
            $this->fw->out("\tzend_execute_data *orig_execute_data = execute_data;\n");
            $this->fw->out("\texecute_data = ex;\n");
            $this->fw->out("#else\n");
            $this->fw->out("\tzend_execute_data *execute_data = ex;\n");
            $this->fw->out("#endif\n");
            $this->fw->out("\n");
            $this->fw->out("\tLOAD_OPLINE();\n");
            $this->fw->out("#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)\n");
            $this->fw->out("\t((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
            $this->fw->out("\tif (EXPECTED(opline)) {\n");
            $this->fw->out("\t\tret = execute_data != ex ? (int)(execute_data->prev_execute_data != ex) + 1 : 0;\n");
            $this->fw->out("\t\tSAVE_OPLINE();\n");
            $this->fw->out("\t} else {\n");
            $this->fw->out("\t\tret = -1;\n");
            $this->fw->out("\t}\n");
            $this->fw->out("#else\n");
            $this->fw->out("\tret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
            $this->fw->out("\tSAVE_OPLINE();\n");
            $this->fw->out("#endif\n");
            $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
            $this->fw->out("\texecute_data = orig_execute_data;\n");
            $this->fw->out("#endif\n");
            $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
            $this->fw->out("\topline = orig_opline;\n");
            $this->fw->out("#endif\n");
            $this->fw->out("\treturn ret;\n");
            $this->fw->out("}\n\n");
        } else {
            $this->fw->out("ZEND_API int zend_vm_call_opcode_handler(zend_execute_data* ex)\n");
            $this->fw->out("{\n");
            $this->fw->out("\tzend_error_noreturn(E_CORE_ERROR, \"zend_vm_call_opcode_handler() is not supported\");\n");
            $this->fw->out("\treturn 0;\n");
            $this->fw->out("}\n\n");
        }

        echo "zend_vm_execute.h generated successfully.\n";
    }

    private function genCode(string $code, string $op1, string $op2, string $name, ?array $extraSpec = null) : void
    {
        $prefix = VmOps::PREFIXES;

        // Specializing
        $code = preg_replace([
            '/OP1_TYPE/',
            '/OP2_TYPE/',
            '/OP1_FREE/',
            '/OP2_FREE/',
            '/GET_OP1_ZVAL_PTR\(([^)]*)\)/',
            '/GET_OP2_ZVAL_PTR\(([^)]*)\)/',
            '/GET_OP1_ZVAL_PTR_DEREF\(([^)]*)\)/',
            '/GET_OP2_ZVAL_PTR_DEREF\(([^)]*)\)/',
            '/GET_OP1_ZVAL_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP2_ZVAL_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP1_ZVAL_PTR_PTR\(([^)]*)\)/',
            '/GET_OP2_ZVAL_PTR_PTR\(([^)]*)\)/',
            '/GET_OP1_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP2_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP1_OBJ_ZVAL_PTR\(([^)]*)\)/',
            '/GET_OP2_OBJ_ZVAL_PTR\(([^)]*)\)/',
            '/GET_OP1_OBJ_ZVAL_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP2_OBJ_ZVAL_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP1_OBJ_ZVAL_PTR_DEREF\(([^)]*)\)/',
            '/GET_OP2_OBJ_ZVAL_PTR_DEREF\(([^)]*)\)/',
            '/GET_OP1_OBJ_ZVAL_PTR_PTR\(([^)]*)\)/',
            '/GET_OP2_OBJ_ZVAL_PTR_PTR\(([^)]*)\)/',
            '/GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/',
            '/GET_OP2_OBJ_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/',
            '/FREE_OP1\(\)/',
            '/FREE_OP2\(\)/',
            '/FREE_OP1_IF_VAR\(\)/',
            '/FREE_OP2_IF_VAR\(\)/',
            '/FREE_OP1_VAR_PTR\(\)/',
            '/FREE_OP2_VAR_PTR\(\)/',
            '/FREE_UNFETCHED_OP1\(\)/',
            '/FREE_UNFETCHED_OP2\(\)/',
            '/^#(\s*)ifdef\s+ZEND_VM_SPEC\s*\n/m',
            '/^#(\s*)ifndef\s+ZEND_VM_SPEC\s*\n/m',
            '/\!defined\(ZEND_VM_SPEC\)/m',
            '/defined\(ZEND_VM_SPEC\)/m',
            '/ZEND_VM_C_LABEL\(\s*([A-Za-z_]*)\s*\)/m',
            '/ZEND_VM_C_GOTO\(\s*([A-Za-z_]*)\s*\)/m',
            '/^#(\s*)if\s+1\s*\\|\\|.*[^\\\\]$/m',
            '/^#(\s*)if\s+0\s*&&.*[^\\\\]$/m',
            '/OP_DATA_TYPE/',
            '/GET_OP_DATA_ZVAL_PTR\(([^)]*)\)/',
            '/GET_OP_DATA_ZVAL_PTR_DEREF\(([^)]*)\)/',
            '/FREE_OP_DATA\(\)/',
            '/FREE_UNFETCHED_OP_DATA\(\)/',
            '/RETURN_VALUE_USED\(opline\)/',
            '/arg_num <= MAX_ARG_FLAG_NUM/',
            '/ZEND_VM_SMART_BRANCH\(\s*([^,)]*)\s*,\s*([^)]*)\s*\)/',
            '/opline->extended_value\s*==\s*0/',
            '/opline->extended_value\s*==\s*ZEND_ASSIGN_DIM/',
            '/opline->extended_value\s*==\s*ZEND_ASSIGN_OBJ/'
        ], [
            VmOps::OP1_TYPE[$op1],
            VmOps::OP2_TYPE[$op2],
            VmOps::OP1_FREE[$op1],
            VmOps::OP2_FREE[$op2],
            VmOps::OP1_GET_ZVAL_PTR[$op1],
            VmOps::OP2_GET_ZVAL_PTR[$op2],
            VmOps::OP1_GET_ZVAL_PTR_DEREF[$op1],
            VmOps::OP2_GET_ZVAL_PTR_DEREF[$op2],
            VmOps::OP1_GET_ZVAL_PTR_UNDEF[$op1],
            VmOps::OP2_GET_ZVAL_PTR_UNDEF[$op2],
            VmOps::OP1_GET_ZVAL_PTR_PTR[$op1],
            VmOps::OP2_GET_ZVAL_PTR_PTR[$op2],
            VmOps::OP1_GET_ZVAL_PTR_PTR_UNDEF[$op1],
            VmOps::OP2_GET_ZVAL_PTR_PTR_UNDEF[$op2],
            VmOps::OP1_GET_OBJ_ZVAL_PTR[$op1],
            VmOps::OP2_GET_OBJ_ZVAL_PTR[$op2],
            VmOps::OP1_GET_OBJ_ZVAL_PTR_UNDEF[$op1],
            VmOps::OP2_GET_OBJ_ZVAL_PTR_UNDEF[$op2],
            VmOps::OP1_GET_OBJ_ZVAL_PTR_DEREF[$op1],
            VmOps::OP2_GET_OBJ_ZVAL_PTR_DEREF[$op2],
            VmOps::OP1_GET_OBJ_ZVAL_PTR_PTR[$op1],
            VmOps::OP2_GET_OBJ_ZVAL_PTR_PTR[$op2],
            VmOps::OP1_GET_OBJ_ZVAL_PTR_PTR_UNDEF[$op1],
            VmOps::OP2_GET_OBJ_ZVAL_PTR_PTR_UNDEF[$op2],
            VmOps::OP1_FREE_OP[$op1],
            VmOps::OP2_FREE_OP[$op2],
            VmOps::OP1_FREE_OP_IF_VAR[$op1],
            VmOps::OP2_FREE_OP_IF_VAR[$op2],
            VmOps::OP1_FREE_OP_VAR_PTR[$op1],
            VmOps::OP2_FREE_OP_VAR_PTR[$op2],
            VmOps::OP1_FREE_UNFETCHED[$op1],
            VmOps::OP2_FREE_UNFETCHED[$op2],
            ($op1 !== 'ANY' || $op2 !== 'ANY') ? "#\\1if 1\n" : "#\\1if 0\n",
            ($op1 !== 'ANY' || $op2 !== 'ANY') ? "#\\1if 0\n" : "#\\1if 1\n",
            ($op1 !== 'ANY' || $op2 !== 'ANY') ? '0' : '1',
            ($op1 !== 'ANY' || $op2 !== 'ANY') ? '1' : '0',
            '\\1' . ((ZEND_VM_SPEC && ZEND_VM_KIND !== ZendVmKind::CALL)
                ? ("_SPEC{$prefix[$op1]}{$prefix[$op2]}{$this->extraSpecName($extraSpec)}") : ''),
            'goto \\1' . ((ZEND_VM_SPEC && ZEND_VM_KIND !== ZendVmKind::CALL)
                ? ("_SPEC{$prefix[$op1]}{$prefix[$op2]}{$this->extraSpecName($extraSpec)}") : ''),
            '#\\1if 1',
            '#\\1if 0',
            VmOps::OP_DATA_TYPE[$extraSpec['OP_DATA'] ?? 'ANY'],
            VmOps::OP_DATA_GET_ZVAL_PTR[$extraSpec['OP_DATA'] ?? 'ANY'],
            VmOps::OP_DATA_GET_ZVAL_PTR_DEREF[$extraSpec['OP_DATA'] ?? 'ANY'],
            VmOps::OP_DATA_FREE_OP[$extraSpec['OP_DATA'] ?? 'ANY'],
            VmOps::OP_DATA_FREE_UNFETCHED[$extraSpec['OP_DATA'] ?? 'ANY'],
            $extraSpec['RETVAL'] ?? 'RETURN_VALUE_USED(opline)',
            $extraSpec['QUICK_ARG'] ?? 'arg_num <= MAX_ARG_FLAG_NUM',
            isset($extraSpec['SMART_BRANCH'])
                ? ($extraSpec['SMART_BRANCH'] === 1
                    ? 'ZEND_VM_SMART_BRANCH_JMPZ(\\1, \\2)'
                    : ($extraSpec['SMART_BRANCH'] === 2
                        ? 'ZEND_VM_SMART_BRANCH_JMPNZ(\\1, \\2)'
                        : ''))
                : 'ZEND_VM_SMART_BRANCH(\\1, \\2)',
            isset($extraSpec['DIM_OBJ'])
                ? ($extraSpec['DIM_OBJ'] === 0 ? '1' : '0')
                : '\\0',
            isset($extraSpec['DIM_OBJ'])
                ? ($extraSpec['DIM_OBJ'] === 1 ? '1' : '0')
                : '\\0',
            isset($extraSpec['DIM_OBJ'])
                ? ($extraSpec['DIM_OBJ'] === 2 ? '1' : '0')
                : '\\0'
        ], $code);

        switch (ZEND_VM_KIND) {
            case ZendVmKind::CALL:
                $code = preg_replace_callback_array([
                    '/EXECUTE_DATA/m' => static function ($match) use ($op1, $op2, $extraSpec) {
                        return 'execute_data';
                    },
                    '/ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)/m' => function ($match) use ($op1, $op2, $extraSpec) {
                        return 'ZEND_VM_TAIL_CALL('
                            . $this->opcodeName($match[1], $op1, $op2)
                            . '_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU))';
                    },
                    '/ZEND_VM_DISPATCH_TO_HELPER\(\s*([A-Za-z_]*)\s*(,[^)]*)?\)/m' => function ($match) use ($op1, $op2, $extraSpec) {
                        if (isset($match[2])) {
                            // extra args
                            $args = substr(preg_replace('/,\s*[A-Za-z_]*\s*,\s*([^,)\s]*)\s*/', ', $1', $match[2]), 2);

                            return 'ZEND_VM_TAIL_CALL('
                                . $this->helperName($match[1], $op1, $op2, $extraSpec)
                                . "({$args} ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC))";
                        }

                        return 'ZEND_VM_TAIL_CALL('
                            . $this->helperName($match[1], $op1, $op2, $extraSpec)
                            . '(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU))';
                    },
                ], $code);
                break;
            case ZendVmKind::SWITCH:
                $code = preg_replace_callback_array([
                    '/EXECUTE_DATA/m' => static function ($match) use ($op1, $op2, $extraSpec) {
                        return 'execute_data';
                    },
                    '/ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)/m' => function ($match) use ($op1, $op2, $extraSpec) {
                        return "goto {$this->opcodeName($match[1], $op1, $op2)}_LABEL";
                    },
                    '/ZEND_VM_DISPATCH_TO_HELPER\(\s*([A-Za-z_]*)\s*(,[^)]*)?\)/m' => function ($match) use ($op1, $op2, $extraSpec) {
                        if (isset($match[2])) {
                            // extra args
                            $args = preg_replace('/,\s*([A-Za-z_]*)\s*,\s*([^,)\s]*)\s*/', '$1 = $2; ', $match[2]);

                            return "{$args}goto {$this->helperName($match[1], $op1, $op2, $extraSpec)}";
                        }

                        return "goto {$this->helperName($match[1], $op1, $op2, $extraSpec)}";
                    }
                ], $code);
                break;
            case ZendVmKind::GOTO:
                $code = preg_replace_callback_array([
                    '/EXECUTE_DATA/m' => static function ($match) use ($op1, $op2, $extraSpec) {
                        return 'execute_data';
                    },
                    '/ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)/m' => function ($match) use ($op1, $op2, $extraSpec) {
                        return "goto {$this->opcodeName($match[1], $op1, $op2)}_HANDLER";
                    },
                    '/ZEND_VM_DISPATCH_TO_HELPER\(\s*([A-Za-z_]*)\s*(,[^)]*)?\)/m' => function ($match) use ($op1, $op2, $extraSpec) {
                        if (isset($match[2])) {
                            // extra args
                            $args = preg_replace('/,\s*([A-Za-z_]*)\s*,\s*([^,)\s]*)\s*/', '$1 = $2; ', $match[2]);

                            return "{$args}goto {$this->helperName($match[1], $op1, $op2, $extraSpec)}";
                        }

                        return "goto {$this->helperName($match[1], $op1, $op2, $extraSpec)}";
                    }
                ], $code);
        }

        /* Remove unused free_op1 and free_op2 declarations */
        if (ZEND_VM_SPEC && preg_match_all('~^\s*zend_free_op\s+[^;]+;\s*$~me', $code, $matches, PREG_SET_ORDER)) {
            $n = 0;

            foreach ($matches as $match) {
                $code = preg_replace('/'.preg_quote($match[0],'/').'/', "\$D{$n}", $code);
                ++$n;
            }

            $del_free_op1 = (strpos($code, 'free_op1') === false);
            $del_free_op2 = (strpos($code, 'free_op2') === false);
            $del_free_op_data = (strpos($code, 'free_op_data') === false);
            $n = 0;

            foreach ($matches as $match) {
                $dcl = $match[0];
                $changed = false;

                if ($del_free_op1 && strpos($dcl, 'free_op1') !== false) {
                    $dcl = preg_replace('/free_op1\s*,\s*/', '', $dcl);
                    $dcl = preg_replace('/free_op1\s*;/', ';', $dcl);
                    $changed = true;
                }

                if ($del_free_op2 && strpos($dcl, 'free_op2') !== false) {
                    $dcl = preg_replace('/free_op2\s*,\s*/', '', $dcl);
                    $dcl = preg_replace('/free_op2\s*;/', ';', $dcl);
                    $changed = true;
                }

                if ($del_free_op_data && strpos($dcl, 'free_op_data') !== false) {
                    $dcl = preg_replace('/free_op_data\s*,\s*/', '', $dcl);
                    $dcl = preg_replace('/free_op_data\s*;/', ';', $dcl);
                    $changed = true;
                }

                if ($changed) {
                    $dcl = preg_replace('/,\s*;/', ';', $dcl);
                    $dcl = preg_replace('/zend_free_op\s*;/', '', $dcl);
                }

                 $code = preg_replace("/\\\$D{$n}/", $dcl, $code);
                 ++$n;
            }
        }

        /* Remove unnecessary ';' and WS */
        $code = preg_replace(['/^\s*;\s*$/m', '/[ \t]+\n/m'], ['', "\n"], $code);

        $this->fw->out($code);
    }

    // Returns name of specialized helper
    private function helperName(string $name, string $op1, string $op2, ?array $extraSpec) : string
    {
        $helpers = $this->dfp->getHelpers();
        $prefix = VmOps::PREFIXES;
        $specSuffix = ZEND_VM_SPEC ? '_SPEC' : '';
        $extra = '';

        if (isset($helpers[$name])) {
            // If we haven't helper with specified spicialized operands then
            // using unspecialized helper
            if (!isset($helpers[$name]['op1'][$op1]) && isset($helpers[$name]['op1']['ANY'])) {
                $op1 = 'ANY';
            }

            if (!isset($helpers[$name]['op2'][$op2]) && isset($helpers[$name]['op2']['ANY'])) {
                $op2 = 'ANY';
            }

            /* forward common specs (e.g. in ZEND_VM_DISPATCH_TO_HELPER) */
            if (isset($extraSpec, $helpers[$name]['spec'])) {
                $extra = $this->extraSpecName(array_intersect_key($extraSpec, $helpers[$name]['spec']));
            }
        }
        return $name . $specSuffix . $prefix[$op1] . $prefix[$op2] . $extra;
    }

    private function opcodeName(string $name, string $op1, string $op2) : string
    {
        $opnames = $this->dfp->getOpnames();
        $prefix = VmOps::PREFIXES;
        $specSuffix = ZEND_VM_SPEC ? '_SPEC' : '';

        if (isset($opnames[$name])) {
            $opcode = $this->opcodes[$opnames[$name]];
            // If we haven't helper with specified spicialized operands then
            // using unspecialized helper
            if (!isset($opcode['op1'][$op1]) && isset($opcode['op1']['ANY'])) {
                $op1 = 'ANY';
            }

            if (!isset($opcode['op2'][$op2]) && isset($opcode['op2']['ANY'])) {
                $op2 = 'ANY';
            }
        }

        return $name . $specSuffix . $prefix[$op1] . $prefix[$op2];
    }

    // Generates all opcode handlers and helpers (specialized or unspecilaized)
    private function genExecutorCode(array &$switchLabels = []) : void
    {
        $list = $this->dfp->getList();
        $helpers = $this->dfp->getHelpers();

        if (ZEND_VM_SPEC) {
            // Produce specialized executor
            // for each op1.op_type
            foreach (VmOps::EXTENDED_TYPES as $op1) {
                // for each op2.op_type
                foreach (VmOps::EXTENDED_TYPES as $op2) {
                    // for each handlers in helpers in original order
                    foreach ($list as $lineno => $dsc) {
                        if (isset($dsc['handler'])) {
                            $num = $dsc['handler'];

                            foreach ($this->extraSpecHandler($this->opcodes[$num]) as $extraSpec) {
                                // Check if handler accepts such types of operands (op1 and op2)
                                if (isset($this->opcodes[$num]['op1'][$op1], $this->opcodes[$num]['op2'][$op2])) {
                                      // Generate handler code
                                    $this->genHandler($num, $op1, $op2, $lineno, $extraSpec, $switchLabels);
                                }
                            }
                        } elseif (isset($dsc['helper'])) {
                            $name = $dsc['helper'];

                            foreach ($this->extraSpecHandler($helpers[$name]) as $extraSpec) {
                                // Check if handler accepts such types of operands (op1 and op2)
                                if (isset($helpers[$name]['op1'][$op1], $helpers[$name]['op2'][$op2])) {
                                      // Generate helper code
                                    $this->genHelper($name, $op1, $op2, $lineno, $extraSpec);
                                }
                            }
                        } else {
                            var_dump($dsc);
                            die('??? ' . ZEND_VM_KIND . ":{$num}\n");
                        }
                    }
                }
            }
        } else {
            // Produce unspecialized executor

            // for each handlers in helpers in original order
            foreach ($list as $lineno => $dsc) {
                if (isset($dsc['handler'])) {
                    $num = $dsc['handler'];
                    // Generate handler code
                    if ($num < 256) {
                        $this->genHandler($num, 'ANY', 'ANY', $lineno);
                    }
                } elseif (isset($dsc['helper'])) {
                    $name = $dsc['helper'];
                    // Generate helper code
                    $this->genHelper($name, 'ANY', 'ANY', $lineno);
                } else {
                    var_dump($dsc);
                    die('??? ' . ZEND_VM_KIND . ":{$num}\n");
                }
            }
        }

        if (ZEND_VM_LINES) {
            $this->fw->outLine(); // Reset #line directives
        }

        // Generate handler for undefined opcodes
        switch (ZEND_VM_KIND) {
            case ZendVmKind::CALL:
				$this->fw->out("static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL ZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
				$this->fw->out("{\n");
				$this->fw->out("\tUSE_OPLINE\n");
				$this->fw->out("\n");
				$this->fw->out("\tzend_error_noreturn(E_ERROR, \"Invalid opcode %d/%d/%d.\", OPLINE->opcode, OPLINE->op1_type, OPLINE->op2_type);\n");
				$this->fw->out("\tZEND_VM_NEXT_OPCODE(); /* Never reached */\n");
				$this->fw->out("}\n\n");
                break;
            case ZendVmKind::SWITCH:
                $this->fw->out("default:\n");
                $this->fw->out("\tzend_error_noreturn(E_ERROR, \"Invalid opcode %d/%d/%d.\", OPLINE->opcode, OPLINE->op1_type, OPLINE->op2_type);\n");
                $this->fw->out("\tZEND_VM_NEXT_OPCODE(); /* Never reached */\n");
                break;
            case ZendVmKind::GOTO:
                $this->fw->out("ZEND_NULL_HANDLER:\n");
                $this->fw->out("\tzend_error_noreturn(E_ERROR, \"Invalid opcode %d/%d/%d.\", OPLINE->opcode, OPLINE->op1_type, OPLINE->op2_type);\n");
                $this->fw->out("\tZEND_VM_NEXT_OPCODE(); /* Never reached */\n");
        }
    }

    private function genExecutor() : void
    {
        $switchLabels = [];
        $executorName = 'execute';
        $initializerName = 'zend_init_opcodes_handlers';

        foreach ($this->skel as $line) {
              // Skeleton file contains special markers in form %NAME% those are
              // substituted by custom code
            if (preg_match('~(.*)[{][%]([A-Z_]*)[%][}](.*)~', $line, $m) === 0) {
                // Copy the line as is
                $this->fw->out($line);
                continue;
            }

            switch ($m[2]) {
                case 'DEFINES':
                    $this->fw->out("#define SPEC_START_MASK        0x0000ffff\n");
                    $this->fw->out("#define SPEC_RULE_OP1          0x00010000\n");
                    $this->fw->out("#define SPEC_RULE_OP2          0x00020000\n");
                    $this->fw->out("#define SPEC_RULE_OP_DATA      0x00040000\n");
                    $this->fw->out("#define SPEC_RULE_RETVAL       0x00080000\n");
                    $this->fw->out("#define SPEC_RULE_QUICK_ARG    0x00100000\n");
                    $this->fw->out("#define SPEC_RULE_SMART_BRANCH 0x00200000\n");
                    $this->fw->out("#define SPEC_RULE_DIM_OBJ      0x00400000\n");
                    $this->fw->out("\n");
                    $this->fw->out("static const uint32_t *zend_spec_handlers;\n");
                    $this->fw->out("static const void **zend_opcode_handlers;\n");
                    $this->fw->out("static int zend_handlers_count;\n");
                    $this->fw->out("static const void *zend_vm_get_opcode_handler(zend_uchar opcode, const zend_op* op);\n\n");

                    $specSuffix = ZEND_VM_SPEC ? '_SPEC' : '';

                    switch (ZEND_VM_KIND) {
                        case ZendVmKind::CALL:
                            $this->fw->out("\n");
                            $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                            $this->fw->out("#pragma GCC diagnostic ignored \"-Wvolatile-register-var\"\n");
                            $this->fw->out("register zend_execute_data* volatile execute_data __asm__(ZEND_VM_FP_GLOBAL_REG);\n");
                            $this->fw->out("#pragma GCC diagnostic warning \"-Wvolatile-register-var\"\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("\n");
                            $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                            $this->fw->out("#pragma GCC diagnostic ignored \"-Wvolatile-register-var\"\n");
                            $this->fw->out("register const zend_op* volatile opline __asm__(ZEND_VM_IP_GLOBAL_REG);\n");
                            $this->fw->out("#pragma GCC diagnostic warning \"-Wvolatile-register-var\"\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("\n");
                            $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS void\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS_DC\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS zend_execute_data *execute_data\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU execute_data\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS_DC , ZEND_OPCODE_HANDLER_ARGS\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC , ZEND_OPCODE_HANDLER_ARGS_PASSTHRU\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("\n");
                            $this->fw->out("#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_RET void\n");
                            $this->fw->out("# define ZEND_VM_TAIL_CALL(call) call; return\n");
                            $this->fw->out("# ifdef ZEND_VM_TAIL_CALL_DISPATCH\n");
                            $this->fw->out("#  define ZEND_VM_CONTINUE()     ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU); return\n");
                            $this->fw->out("# else\n");
                            $this->fw->out("#  define ZEND_VM_CONTINUE()     return\n");
                            $this->fw->out("# endif\n");
                            $this->fw->out("# define ZEND_VM_RETURN()        opline = NULL; return\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("# define ZEND_OPCODE_HANDLER_RET int\n");
                            $this->fw->out("# define ZEND_VM_TAIL_CALL(call) return call\n");
                            $this->fw->out("# define ZEND_VM_CONTINUE()      return  0\n");
                            $this->fw->out("# define ZEND_VM_RETURN()        return -1\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("\n");
                            $this->fw->out("typedef ZEND_OPCODE_HANDLER_RET (ZEND_FASTCALL *opcode_handler_t) (ZEND_OPCODE_HANDLER_ARGS);\n");
                            $this->fw->out("\n");
                            $this->fw->out("#undef OPLINE\n");
                            $this->fw->out("#undef DCL_OPLINE\n");
                            $this->fw->out("#undef USE_OPLINE\n");
                            $this->fw->out("#undef LOAD_OPLINE\n");
                            $this->fw->out("#undef LOAD_OPLINE_EX\n");
                            $this->fw->out("#undef SAVE_OPLINE\n");
                            $this->fw->out("#define DCL_OPLINE\n");
                            $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                            $this->fw->out("# define OPLINE opline\n");
                            $this->fw->out("# define USE_OPLINE\n");
                            $this->fw->out("# define LOAD_OPLINE() opline = EX(opline)\n");
                            $this->fw->out("# define LOAD_NEXT_OPLINE() opline = EX(opline) + 1\n");
                            $this->fw->out("# define SAVE_OPLINE() EX(opline) = opline\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("# define OPLINE EX(opline)\n");
                            $this->fw->out("# define USE_OPLINE const zend_op *opline = EX(opline);\n");
                            $this->fw->out("# define LOAD_OPLINE()\n");
                            $this->fw->out("# define LOAD_NEXT_OPLINE() ZEND_VM_INC_OPCODE()\n");
                            $this->fw->out("# define SAVE_OPLINE()\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("#undef HANDLE_EXCEPTION\n");
                            $this->fw->out("#undef HANDLE_EXCEPTION_LEAVE\n");
                            $this->fw->out("#define HANDLE_EXCEPTION() LOAD_OPLINE(); ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#define HANDLE_EXCEPTION_LEAVE() LOAD_OPLINE(); ZEND_VM_LEAVE()\n");
                            $this->fw->out("#if defined(ZEND_VM_FP_GLOBAL_REG)\n");
                            $this->fw->out("# define ZEND_VM_ENTER()           execute_data = EG(current_execute_data); LOAD_OPLINE(); ZEND_VM_INTERRUPT_CHECK(); ZEND_VM_CONTINUE()\n");
                            $this->fw->out("# define ZEND_VM_LEAVE()           ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#elif defined(ZEND_VM_IP_GLOBAL_REG)\n");
                            $this->fw->out("# define ZEND_VM_ENTER()           opline = EG(current_execute_data)->opline; return 1\n");
                            $this->fw->out("# define ZEND_VM_LEAVE()           return  2\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("# define ZEND_VM_ENTER()           return  1\n");
                            $this->fw->out("# define ZEND_VM_LEAVE()           return  2\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("#define ZEND_VM_INTERRUPT()      ZEND_VM_TAIL_CALL(zend_interrupt_helper{$specSuffix}" . "(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU));\n");
                            $this->fw->out("#define ZEND_VM_LOOP_INTERRUPT() zend_interrupt_helper{$specSuffix}" . "(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
                            $this->fw->out("#define ZEND_VM_DISPATCH(opcode, opline) ZEND_VM_TAIL_CALL(((opcode_handler_t)zend_vm_get_opcode_handler(opcode, opline))(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU));\n");
                            $this->fw->out("\n");
                            $this->fw->out("static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_interrupt_helper{$specSuffix}" . "(ZEND_OPCODE_HANDLER_ARGS);");
                            $this->fw->out("\n");
                            break;
                        case ZendVmKind::SWITCH:
                            $this->fw->out("\n");
                            $this->fw->out("#undef OPLINE\n");
                            $this->fw->out("#undef DCL_OPLINE\n");
                            $this->fw->out("#undef USE_OPLINE\n");
                            $this->fw->out("#undef LOAD_OPLINE\n");
                            $this->fw->out("#undef LOAD_NEXT_OPLINE\n");
                            $this->fw->out("#undef SAVE_OPLINE\n");
                            $this->fw->out("#define OPLINE opline\n");
                            $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                            $this->fw->out("# define DCL_OPLINE register const zend_op *opline __asm__(ZEND_VM_IP_GLOBAL_REG);\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("# define DCL_OPLINE const zend_op *opline;\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("#define USE_OPLINE\n");
                            $this->fw->out("#define LOAD_OPLINE() opline = EX(opline)\n");
                            $this->fw->out("#define LOAD_NEXT_OPLINE() opline = EX(opline) + 1\n");
                            $this->fw->out("#define SAVE_OPLINE() EX(opline) = opline\n");
                            $this->fw->out("#undef HANDLE_EXCEPTION\n");
                            $this->fw->out("#undef HANDLE_EXCEPTION_LEAVE\n");
                            $this->fw->out("#define HANDLE_EXCEPTION() LOAD_OPLINE(); ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#define HANDLE_EXCEPTION_LEAVE() LOAD_OPLINE(); ZEND_VM_LEAVE()\n");
                            $this->fw->out("#define ZEND_VM_CONTINUE() goto zend_vm_continue\n");
                            $this->fw->out("#define ZEND_VM_RETURN()   return\n");
                            $this->fw->out("#define ZEND_VM_ENTER()    execute_data = EG(current_execute_data); LOAD_OPLINE(); ZEND_VM_INTERRUPT_CHECK(); ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#define ZEND_VM_LEAVE()    ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#define ZEND_VM_INTERRUPT()              goto zend_interrupt_helper{$specSuffix};\n");
                            $this->fw->out("#define ZEND_VM_LOOP_INTERRUPT()         goto zend_interrupt_helper{$specSuffix};\n");
                            $this->fw->out("#define ZEND_VM_DISPATCH(opcode, opline) dispatch_handler = zend_vm_get_opcode_handler(opcode, opline); goto zend_vm_dispatch;\n");
                            $this->fw->out("\n");
                            break;
                        case ZendVmKind::GOTO:
                            $this->fw->out("\n");
                            $this->fw->out("#undef OPLINE\n");
                            $this->fw->out("#undef DCL_OPLINE\n");
                            $this->fw->out("#undef USE_OPLINE\n");
                            $this->fw->out("#undef LOAD_OPLINE\n");
                            $this->fw->out("#undef LOAD_NEXT_OPLINE\n");
                            $this->fw->out("#undef SAVE_OPLINE\n");
                            $this->fw->out("#define OPLINE opline\n");
                            $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                            $this->fw->out("# define DCL_OPLINE register const zend_op *opline __asm__(ZEND_VM_IP_GLOBAL_REG);\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("# define DCL_OPLINE const zend_op *opline;\n");
                            $this->fw->out("#endif\n");
                            $this->fw->out("#define USE_OPLINE\n");
                            $this->fw->out("#define LOAD_OPLINE() opline = EX(opline)\n");
                            $this->fw->out("#define LOAD_NEXT_OPLINE() opline = EX(opline) + 1\n");
                            $this->fw->out("#define SAVE_OPLINE() EX(opline) = opline\n");
                            $this->fw->out("#undef HANDLE_EXCEPTION\n");
                            $this->fw->out("#undef HANDLE_EXCEPTION_LEAVE\n");

                            if (ZEND_VM_SPEC) {
                                $this->fw->out("#define HANDLE_EXCEPTION() goto ZEND_HANDLE_EXCEPTION_SPEC_HANDLER\n");
                                $this->fw->out("#define HANDLE_EXCEPTION_LEAVE() goto ZEND_HANDLE_EXCEPTION_SPEC_HANDLER\n");
                            } else {
                                $this->fw->out("#define HANDLE_EXCEPTION() goto ZEND_HANDLE_EXCEPTION_HANDLER\n");
                                $this->fw->out("#define HANDLE_EXCEPTION_LEAVE() goto ZEND_HANDLE_EXCEPTION_HANDLER\n");
                            }

                            $this->fw->out("#define ZEND_VM_CONTINUE() goto *(void**)(OPLINE->handler)\n");
                            $this->fw->out("#define ZEND_VM_RETURN()   return\n");
                            $this->fw->out("#define ZEND_VM_ENTER()    execute_data = EG(current_execute_data); LOAD_OPLINE(); ZEND_VM_INTERRUPT_CHECK(); ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#define ZEND_VM_LEAVE()    ZEND_VM_CONTINUE()\n");
                            $this->fw->out("#define ZEND_VM_INTERRUPT()              goto zend_interrupt_helper{$specSuffix};\n");
                            $this->fw->out("#define ZEND_VM_LOOP_INTERRUPT()         goto zend_interrupt_helper{$specSuffix};\n");
                            $this->fw->out("#define ZEND_VM_DISPATCH(opcode, opline) goto *(void**)(zend_vm_get_opcode_handler(opcode, opline));\n");
                            $this->fw->out("\n");
                            break;
                    }
                    break;
                case 'EXECUTOR_NAME':
                    $this->fw->out("{$m[1]}{$executorName}{$m[3]}\n");
                    break;
                case 'HELPER_VARS':
                    if (ZEND_VM_KIND !== ZendVmKind::CALL) {
                        if (ZEND_VM_KIND === ZendVmKind::SWITCH) {
                            $this->fw->out("{$m[1]}const void *dispatch_handler;\n");
                        }

                        // Emit local variables those are used for helpers' parameters
                        foreach ($this->dfp->getParams() as $param => $x) {
                            $this->fw->out("{$m[1]}{$param};\n");
                        }

                        $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                        $this->fw->out("{$m[1]}register zend_execute_data *execute_data __asm__(ZEND_VM_FP_GLOBAL_REG) = ex;\n");
                        $this->fw->out("#else\n");
                        $this->fw->out("{$m[1]}zend_execute_data *execute_data = ex;\n");
                        $this->fw->out("#endif\n");
                    } else {
                        $this->fw->out("#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                        $this->fw->out("{$m[1]}const zend_op *orig_opline = opline;\n");
                        $this->fw->out("#endif\n");
                        $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                        $this->fw->out("{$m[1]}zend_execute_data *orig_execute_data = execute_data;\n");
                        $this->fw->out("{$m[1]}execute_data = ex;\n");
                        $this->fw->out("#else\n");
                        $this->fw->out("{$m[1]}zend_execute_data *execute_data = ex;\n");
                        $this->fw->out("#endif\n");
                    }
                    break;
                case 'INTERNAL_LABELS':
                    if (ZEND_VM_KIND === ZendVmKind::GOTO) {
                          // Emit array of labels of opcode handlers and code for
                          // zend_opcode_handlers initialization
                        $prolog = $m[1];
                        $this->fw->out("{$prolog}if (UNEXPECTED(execute_data == NULL)) {\n");
                        $this->fw->out("{$prolog}\tstatic const void* labels[] = {\n");
                        $this->genLabels("{$prolog}\t\t", $specs);
                        $this->fw->out("{$prolog}\t};\n");
                        $this->fw->out("{$prolog}static const uint32_t specs[] = {\n");
                        $this->genSpecs("{$prolog}\t", $specs);
                        $this->fw->out("{$prolog}};\n");
                        $this->fw->out("{$prolog}\tzend_opcode_handlers = (const void **) labels;\n");
                        $this->fw->out("{$prolog}\tzend_handlers_count = sizeof(labels) / sizeof(void*);\n");
                        $this->fw->out("{$prolog}\tzend_spec_handlers = (const uint32_t *) specs;\n");
                        $this->fw->out("{$prolog}\treturn;\n");
                        $this->fw->out("{$prolog}}\n");
                    } else {
                        $this->skipBlanks($m[1], $m[3]);
                    }
                    break;
                case 'ZEND_VM_CONTINUE_LABEL':
                    if (ZEND_VM_KIND === ZendVmKind::CALL) {
                          // Only SWITCH dispatch method use it
                        $this->fw->out("#if !defined(ZEND_VM_FP_GLOBAL_REG) || !defined(ZEND_VM_IP_GLOBAL_REG)\n");
                        $this->fw->out("{$m[1]}\tint ret;{$m[3]}\n");
                        $this->fw->out("#endif\n");
                    } elseif (ZEND_VM_KIND === ZendVmKind::SWITCH) {
                          // Only SWITCH dispatch method use it
                        $this->fw->out("zend_vm_continue:{$m[3]}\n");
                    } else {
                        $this->skipBlanks($m[1], $m[3]);
                    }
                    break;
                case 'ZEND_VM_DISPATCH':
                      // Emit code that dispatches to opcode handler
                    switch (ZEND_VM_KIND) {
                        case ZendVmKind::CALL:
                            $this->fw->out("#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)\n");
                            $this->fw->out("{$m[1]}((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
                            $this->fw->out("{$m[1]}if (UNEXPECTED(!OPLINE)){$m[3]}\n");
                            $this->fw->out("#else\n");
                            $this->fw->out("{$m[1]}if (UNEXPECTED((ret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU)) != 0)){$m[3]}\n");
                            $this->fw->out("#endif\n");
                            break;
                        case ZendVmKind::SWITCH:
                            $this->fw->out("{$m[1]}dispatch_handler = OPLINE->handler;\nzend_vm_dispatch:\n{$m[1]}switch ((int)(uintptr_t)dispatch_handler){$m[3]}\n");
                            break;
                        case ZendVmKind::GOTO:
                            $this->fw->out("{$m[1]}goto *(void**)(OPLINE->handler);{$m[3]}\n");
                            break;
                    }
                    break;
                case 'INTERNAL_EXECUTOR':
                    if (ZEND_VM_KIND === ZendVmKind::CALL) {
                          // Executor is defined as a set of functions
                        $this->fw->out("#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                        $this->fw->out("{$m[1]}execute_data = orig_execute_data;\n");
                        $this->fw->out("# ifdef ZEND_VM_IP_GLOBAL_REG\n");
                        $this->fw->out("{$m[1]}opline = orig_opline;\n");
                        $this->fw->out("# endif\n");
                        $this->fw->out("{$m[1]}return;\n");
                        $this->fw->out("#else\n");
                        $this->fw->out("{$m[1]}if (EXPECTED(ret > 0)) {\n");
                        $this->fw->out("{$m[1]}\texecute_data = EG(current_execute_data);\n");
                        $this->fw->out("{$m[1]}\tZEND_VM_LOOP_INTERRUPT_CHECK();\n");
                        $this->fw->out("{$m[1]}} else {\n");
                        $this->fw->out("# ifdef ZEND_VM_IP_GLOBAL_REG\n");
                        $this->fw->out("{$m[1]}\topline = orig_opline;\n");
                        $this->fw->out("# endif\n");
                        $this->fw->out("{$m[1]}\treturn;\n");
                        $this->fw->out("{$m[1]}}\n");
                        $this->fw->out("#endif\n");
                    } else {
                          // Emit executor code
                        $this->genExecutorCode($switchLabels);
                    }
                    break;
                case 'EXTERNAL_EXECUTOR':
                    if (ZEND_VM_KIND === ZendVmKind::CALL) {
                        $this->genExecutorCode();
                    }
                    break;
                case 'INITIALIZER_NAME':
                    $this->fw->out("{$m[1]}{$initializerName}{$m[3]}\n");
                    break;
                case 'EXTERNAL_LABELS':
                      // Emit code that initializes zend_opcode_handlers array
                    $prolog = $m[1];
                    if (ZEND_VM_KIND === ZendVmKind::GOTO) {
                          // Labels are defined in the executor itself, so we call it
                          // with execute_data NULL and it sets zend_opcode_handlers array
                        $this->fw->out("{$prolog}");
                        $this->fw->out("{$prolog}{$executorName}_ex(NULL);\n");
                    } else {
                        $this->fw->out("{$prolog}static const void *labels[] = {\n");
                        $this->genLabels("{$prolog}\t", $specs, $switchLabels);
                        $this->fw->out("{$prolog}};\n");
                        $this->fw->out("{$prolog}static const uint32_t specs[] = {\n");
                        $this->genSpecs("{$prolog}\t", $specs);
                        $this->fw->out("{$prolog}};\n");
                        $this->fw->out("{$prolog}zend_opcode_handlers = labels;\n");
                        $this->fw->out("{$prolog}zend_handlers_count = sizeof(labels) / sizeof(void*);\n");
                        $this->fw->out("{$prolog}zend_spec_handlers = specs;\n");
                    }
                    break;
                default:
                    die("ERROR: Unknown keyword {$m[2]} in skeleton file.\n");
            }
        }
    }

    // Generates specialized offsets
    private function genSpecs(string $prolog, array $specs) : void
    {
        $lastdef = array_pop($specs);
         $last = 0;

        foreach ($specs as $num => $def) {
            while (++$last < $num) {
                $this->fw->out("{$prolog}{$lastdef},\n");
            }

            $last = $num;
            $this->fw->out("{$prolog}{$def},\n");
        }

        $this->fw->out("{$prolog}{$lastdef}\n");
    }

    // Generates opcode handler
    private function genHandler(int $num, string $op1, string $op2, int $lineno, array $extraSpec = null, &$switchLabels = [])
    {
		$name = $this->opcodes[$num]['op'];
		$use = isset($this->opcodes[$num]['use']);
		$code = $this->opcodes[$num]['code'];

        if (ZEND_VM_SPEC && $this->skipExtraSpecFunction($op1, $op2, $extraSpec)) {
            return;
        }

        if (ZEND_VM_LINES) {
            $this->fw->out("#line {$lineno} \"{$this->dfp->getDefinitionFile()}\"\n");
        }

        // Generate opcode handler's entry point according to selected threading model
        $specSuffix = ZEND_VM_SPEC ? '_SPEC' : '';
        $extraSpecName = ZEND_VM_SPEC ? $this->extraSpecName($extraSpec) : '';
        $specName = $name . $specSuffix . VmOps::PREFIXES[$op1] . VmOps::PREFIXES[$op2] . $extraSpecName;

        switch (ZEND_VM_KIND) {
            case ZendVmKind::CALL:
                $this->fw->out("static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$specName}_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
                break;
            case ZendVmKind::SWITCH:
                if (ZEND_VM_SPEC) {
                    $cur = $switchLabels ? end($switchLabels) + 1 : 0;
                    $this->fw->out("case {$cur}: /* {$specName} */");
                    $switchLabels[$specName] = $cur;
                } else {
                    $this->fw->out("case ".$name.":");
                }

                if ($use) {
                    // This handler is used by other handlers. We will add label to call it.
                    $this->fw->out(" {$specName}_LABEL:\n");
                } else {
                    $this->fw->out("\n");
                }
                break;
            case ZendVmKind::GOTO:
                $this->fw->out("{$specName}_HANDLER: ZEND_VM_GUARD({$specName});\n");
        }

        // Generate opcode handler's code
        $this->genCode($code, $op1, $op2, $name, $extraSpec);
    }

    // Generates helper
    private function genHelper(string $name, string $op1, string $op2, int $lineno, array $extraSpec = []) : void
    {
		$helpers = $this->dfp->getHelpers();
		$param = $helpers[$name]['param'];
		$code = $helpers[$name]['code'];
		$inline = $helpers[$name]['inline'];

        if (ZEND_VM_SPEC && $this->skipExtraSpecFunction($op1, $op2, $extraSpec)) {
            return;
        }

        if (ZEND_VM_LINES) {
            $this->fw->out("#line {$lineno} \"{$this->dfp->getDefinitionFile()}\"\n");
        }

        $specSuffix = ZEND_VM_SPEC ? '_SPEC' : '';
        $extraSpecName = ZEND_VM_SPEC ? $this->extraSpecName($extraSpec) : '';
        $specName = $name . $specSuffix . VmOps::PREFIXES[$op1] . VmOps::PREFIXES[$op2] . $extraSpecName;

        // Generate helper's entry point according to selected threading model
        switch (ZEND_VM_KIND) {
            case ZendVmKind::CALL:
                if ($inline) {
                    $zend_always_inline = ' zend_always_inline';
                    $zend_fastcall = '';
                } else {
                    $zend_always_inline = '';
                    $zend_fastcall = ' ZEND_FASTCALL';
                }

                if ($param === null) {
                  // Helper without parameters
                    $this->fw->out("static{$zend_always_inline} ZEND_OPCODE_HANDLER_RET{$zend_fastcall} {$specName}(ZEND_OPCODE_HANDLER_ARGS)\n");
                } else {
                  // Helper with parameter
                    $this->fw->out("static{$zend_always_inline} ZEND_OPCODE_HANDLER_RET{$zend_fastcall} {$specName}({$param} ZEND_OPCODE_HANDLER_ARGS_DC)\n");
                }
                break;
            case ZendVmKind::SWITCH:
                $this->fw->out("{$specName}:\n");
                break;
            case ZendVmKind::GOTO:
                $this->fw->out("{$specName}:\n");
        }

        // Generate helper's code
        $this->genCode($code, $op1, $op2, $name, $extraSpec);
    }

    private function extraSpecHandler(array $dsc) : array
    {
        if (!isset($dsc['spec'])) {
            return [[]];
        }

        $specs = $dsc['spec'];

        if (isset($specs['OP_DATA'])) {
            $op_data_specs = $specs['OP_DATA'];
            $specs['OP_DATA'] = [];

            foreach (VmOps::EXTENDED_TYPES as $op_data) {
                if (isset($dsc['spec']['OP_DATA'][$op_data])) {
                    $specs['OP_DATA'][] = $op_data;
                }
            }
        }

        $f = static function ($specs) use (&$f) {
            $spec = key($specs);
            $top = array_shift($specs);
            $next = $specs ? $f($specs) : [[]];
            $ret = [];

            foreach ($next as $existing) {
                foreach ($top as $mode) {
                    $ret[] = [$spec => $mode] + $existing;
                }
            }

            return $ret;
        };

        return $f($specs);
    }

    private function extraSpecName(array $extraSpec) : string
    {
        $s = '';

        foreach ($extraSpec as $name => $value) {
            switch ($name) {
                case 'OP_DATA':
                    $s .= '_OP_DATA' . VmOps::PREFIXES[$value];
                    break;
                case 'RETVAL':
                    $s .= '_RETVAL_' . ($value ? 'USED' : 'UNUSED');
                    break;
                case 'QUICK_ARG':
                    $s .= $value ? '_QUICK' : '';
                    break;
                case 'SMART_BRANCH':
                    if ($value === 1) {
                        $s .= '_JMPZ';
                    } elseif ($value === 2) {
                        $s .= '_JMPNZ';
                    }
                    break;
                case 'DIM_OBJ':
                    if ($value === 1) {
                        $s .= '_DIM';
                    } elseif ($value === 2) {
                        $s .= '_OBJ';
                    }
            }
        }

        return $s;
    }

    private function genNullLabel(string $prolog) : void
    {
        switch (ZEND_VM_KIND) {
            case ZendVmKind::CALL:
                $this->fw->out("{$prolog}ZEND_NULL_HANDLER,\n");
                break;
            case ZendVmKind::SWITCH:
                $this->fw->out("{$prolog}(void*)(uintptr_t)-1,\n");
                break;
            case ZendVmKind::GOTO:
                $this->fw->out("{$prolog}(void*)&&ZEND_NULL_HANDLER,\n");
        }
    }

    private function genLabels(string $prolog, ?array &$specs, array $switchLabels = [])
    {
        $next = 0;
        $label = 0;

        if (ZEND_VM_SPEC) {
            // Emit labels for specialized executor

            // For each opcode in opcode number order
            foreach ($this->opcodes as $num => $dsc) {
                $specs[$num] = "{$label}";
                $specOp1 = $specOp2 = $specExtra = false;
                $next = $num + 1;
                $diff = array_diff_key(array_flip(VmOps::TYPES), $dsc['op1'] ?? []);

                if (count($diff) === count(VmOps::TYPES) - 1) {
                    $cond = isset($diff['ANY']);
                } else {
                    $cond = count($diff) !== count(VmOps::TYPES);
                }

                if ($cond || isset($dsc['op1']['TMPVAR']) || isset($dsc['op1']['TMPVARCV'])) {
                    $specOp1 = true;
                    $specs[$num] .= ' | SPEC_RULE_OP1';
                }

                $diff = array_diff_key(array_flip(VmOps::TYPES), $dsc['op2'] ?? []);

                if (count($diff) === count(VmOps::TYPES) - 1) {
                    $cond = isset($diff['ANY']);
                } else {
                    $cond = count($diff) !== count(VmOps::TYPES);
                }

                if ($cond || isset($dsc['op2']['TMPVAR']) || isset($dsc['op2']['TMPVARCV'])) {
                    $specOp2 = true;
                    $specs[$num] .= ' | SPEC_RULE_OP2';
                }

                $specExtra = array_merge(...$this->extraSpecHandler($dsc));
                $flags = $this->extraSpecFlags($specExtra);

                if ($flags) {
                    $specs[$num] .= ' | ' . implode('|', $flags);
                }

                if ($num >= 256) {
                    $this->opcodes[$num]['spec_code'] = $specs[$num];
                    unset($specs[$num]);
                }

                $foreachOp1 = static function ($do) use ($dsc) {
                    return static function () use ($do, $dsc) {
                        // For each op1.op_type except ANY
                        foreach (VmOps::TYPES as $op1) {
                            if ($op1 === 'ANY') {
                                continue;
                            }

                            if (!isset($dsc['op1'][$op1])) {
                                if ($op1 === 'TMP' || $op1 === 'VAR') {
                                    if (isset($dsc['op1']['TMPVAR'])) {
                                        $op1 = 'TMPVAR';
                                    } elseif (isset($dsc['op1']['TMPVARCV'])) {
                                        $op1 = 'TMPVARCV';
                                    } else {
                                        $op1 = 'ANY';
                                    }
                                } elseif ($op1 === 'CV' && isset($dsc['op1']['TMPVARCV'])) {
                                    $op1 = 'TMPVARCV';
                                } else {
                                    // Try to use unspecialized handler
                                    $op1 = 'ANY';
                                }
                            }
                            $do($op1, 'ANY');
                        }
                    };
                };

                $foreachOp2 = static function ($do) use ($dsc) {
                    return static function ($op1) use ($do, $dsc) {
                        // For each op2.op_type except ANY
                        foreach (VmOps::TYPES as $op2) {
                            if ($op2 === 'ANY') {
                                continue;
                            }

                            if (!isset($dsc['op2'][$op2])) {
                                if ($op2 === 'TMP' || $op2 === 'VAR') {
                                    if (isset($dsc['op2']['TMPVAR'])) {
                                        $op2 = 'TMPVAR';
                                    } elseif (isset($dsc['op2']['TMPVARCV'])) {
                                        $op2 = 'TMPVARCV';
                                    } else {
                                        $op2 = 'ANY';
                                    }
                                } elseif ($op2 === 'CV' && isset($dsc['op2']['TMPVARCV'])) {
                                    $op2 = 'TMPVARCV';
                                } else {
                                    // Try to use unspecialized handler
                                    $op2 = 'ANY';
                                }
                            }
                            $do($op1, $op2);
                        }
                    };
                };

                $foreachOpData = static function ($do) use ($dsc) {
                    return static function ($op1, $op2, $extraSpec = []) use ($do, $dsc) {
                        // For each op_data.op_type except ANY
                        foreach (VmOps::TYPES as $op_data) {
                            if ($op_data === 'ANY') {
                                continue;
                            }

                            if (!isset($dsc['spec']['OP_DATA'][$op_data])) {
                                if ($op_data === 'TMP' || $op_data === 'VAR') {
                                    if (isset($dsc['spec']['OP_DATA']['TMPVAR'])) {
                                        $op_data = 'TMPVAR';
                                    } elseif (isset($dsc['spec']['OP_DATA']['TMPVARCV'])) {
                                        $op_data = 'TMPVARCV';
                                    } else {
                                        // Try to use unspecialized handler
                                        $op_data = 'ANY';
                                    }
                                } elseif ($op_data === 'CV' && isset($dsc['OP_DATA']['TMPVARCV'])) {
                                    $op_data = 'TMPVARCV';
                                } else {
                                    // Try to use unspecialized handler
                                    $op_data = 'ANY';
                                }
                            }
                            $do($op1, $op2, ['OP_DATA' => $op_data] + $extraSpec);
                        }
                    };
                };

                $foreachExtraSpec = static function ($do, $spec) use ($dsc) {
                    return static function ($op1, $op2, $extraSpec = []) use ($do, $spec, $dsc) {
                        foreach ($dsc['spec'][$spec] as $val) {
                            $do($op1, $op2, [$spec => $val] + $extraSpec);
                        }
                    };
                };

                $generate = function ($op1, $op2, $extraSpec = []) use ($dsc, $prolog, $num, $switchLabels, &$label) {
                    $prefix = VmOps::PREFIXES;
                    ++$label;

                    // Check if specialized handler is defined
                    /* TODO: figure out better way to signal "specialized and not defined" than an extra lookup */
                    if (isset($dsc['op1'][$op1], $dsc['op2'][$op2]) &&
                        (!isset($extraSpec['OP_DATA']) || isset($dsc['spec']['OP_DATA'][$extraSpec['OP_DATA']]))) {
                        if ($this->skipExtraSpecFunction($op1, $op2, $extraSpec)) {
                            $this->genNullLabel($prolog);
                            return;
                        }

                        // Emit pointer to specialized handler
                        $specName = "{$dsc['op']}_SPEC{$prefix[$op1]}{$prefix[$op2]}{$this->extraSpecName($extraSpec)}";

                        switch (ZEND_VM_KIND) {
                            case ZendVmKind::CALL:
                                $this->fw->out("{$prolog}{$specName}_HANDLER,\n");
                                break;
                            case ZendVmKind::SWITCH:
                                $this->fw->out("{$prolog}(void*)(uintptr_t){$switchLabels[$specName]},\n");
                                break;
                            case ZendVmKind::GOTO:
                                $this->fw->out("{$prolog}(void*)&&{$specName}_HANDLER,\n");
                        }
                    } else {
                        // Emit pointer to handler of undefined opcode
                        $this->genNullLabel($prolog);
                    }
                };

                $do = $generate;

                if ($specExtra) {
                    foreach (array_keys($specExtra) as $extra) {
                        if ($extra === 'OP_DATA') {
                            $do = $foreachOpData($do);
                        } else {
                            $do = $foreachExtraSpec($do, $extra);
                        }
                    }
                }

                if ($specOp2) {
                    $do = $foreachOp2($do);
                }

                if ($specOp1) {
                    $do = $foreachOp1($do);
                }

                $do('ANY', 'ANY');
            }
        } else {
              // Emit labels for unspecialized executor

              // For each opcode in opcode number order
            foreach ($this->opcodes as $num => $dsc) {
                while ($next !== $num) {
                    // If some opcode numbers are not used then fill hole with pointers
                    // to handler of undefined opcode
                    switch (ZEND_VM_KIND) {
                        case ZendVmKind::CALL:
                            $this->fw->out("{$prolog}ZEND_NULL_HANDLER,\n");
                            break;
                        case ZendVmKind::SWITCH:
                            $this->fw->out("{$prolog}(void*)(uintptr_t)-1,\n");
                            break;
                        case ZendVmKind::GOTO:
                            $this->fw->out("{$prolog}(void*)&&ZEND_NULL_HANDLER,\n");
                    }
                    ++$next;
                }

                if ($num >= 256) {
                    continue;
                }

                $next = $num + 1;

                // ugly trick for ZEND_VM_DEFINE_OP
                if ($dsc['code']) {
                    // Emit pointer to unspecialized handler
                    switch (ZEND_VM_KIND) {
                        case ZendVmKind::CALL:
                            $this->fw->out("{$prolog}{$dsc['op']}_HANDLER,\n");
                            break;
                        case ZendVmKind::SWITCH:
                            $this->fw->out("{$prolog}(void*)(uintptr_t){$num},\n");
                            break;
                        case ZendVmKind::GOTO:
                            $this->fw->out("{$prolog}(void*)&&{$dsc['op']}_HANDLER,\n");
                    }
                } else {
                    switch (ZEND_VM_KIND) {
                        case ZendVmKind::CALL:
                            $this->fw->out("{$prolog}ZEND_NULL_HANDLER,\n");
                            break;
                        case ZendVmKind::SWITCH:
                            $this->fw->out("{$prolog}(void*)(uintptr_t)-1,\n");
                            break;
                        case ZendVmKind::GOTO:
                            $this->fw->out("{$prolog}(void*)&&ZEND_NULL_HANDLER,\n");
                    }
                }
            }
        }

        // Emit last handler's label (undefined opcode)
        switch (ZEND_VM_KIND) {
            case ZendVmKind::CALL:
                $this->fw->out("{$prolog}ZEND_NULL_HANDLER\n");
                break;
            case ZendVmKind::SWITCH:
                $this->fw->out("{$prolog}(void*)(uintptr_t)-1\n");
                break;
            case ZendVmKind::GOTO:
                $this->fw->out("{$prolog}(void*)&&ZEND_NULL_HANDLER\n");
        }

        $specs[$num + 1] = "{$label}";
    }

    private function extraSpecFlags(array $extraSpec) : array
    {
        $specRules = [];
		$validSpecRules = ['OP_DATA', 'RETVAL', 'QUICK_ARG', 'SMART_BRANCH', 'DIM_OBJ'];

        foreach (array_keys($extraSpec) as $spec) {
			if (in_array($spec, $validSpecRules, true)) {
				$specRules[] = "SPEC_RULE_{$spec}";
			}
		}

		return $specRules;
    }

    private function skipExtraSpecFunction(string $op1, string $op2, array $extraSpec) : bool
    {
        if (isset($extraSpec['NO_CONST_CONST']) && $op1 === 'CONST' && $op2 === 'CONST') {
            // Skip useless constant handlers
            return true;
        }

        if (isset($extraSpec['COMMUTATIVE']) && VmOps::COMMUTATIVE_ORDERS[$op1] > VmOps::COMMUTATIVE_ORDERS[$op2]) {
            // Skip duplicate commutative handlers
            return true;
        }

        if (isset($extraSpec['DIM_OBJ']) &&
            (($op2 === 'UNUSED' && $extraSpec['DIM_OBJ'] !== 1) ||
             ($op1 === 'UNUSED' && $extraSpec['DIM_OBJ'] !== 2))) {
            // Skip useless handlers
            return true;
        }

        return false;
    }

    private function skipBlanks(string $prolog, string $epilog) : void
    {
        if (trim($prolog) !== '' || trim($epilog) !== '') {
            $this->fw->out($prolog.$epilog);
        }
    }
}

class FileWriter
{
    private $lineNo = 1;
    private $fh;
    private $executorFile = '';

    public function __construct(string $filename, string $executorFile = '')
    {
        $this->fh = fopen($filename, 'w+') or die("ERROR: Cannot create zend_vm_opcodes.h\n"); // cleaner?
        $this->executorFile = $executorFile;
    }

    public function out(string $text) : void
    {
        fputs($this->fh, $text);
        $this->lineNo += substr_count($text, "\n");
    }

    /*
    Ugly hack for 100% compatibility with old VM gen script when --with-lines flag
    is set. Due to the old global $lineno being set for HEADER_TEXT output for the
    3 generated files, the first two files added 40 onto $lineno for the generation
    of the third zend_vm_execute.h file, causing the #line directive to be off by
    40. This looks like a bug if anything, and can probably be removed.
    */
    public function nout(string $text) : void
    {
        $this->lineNo += substr_count($text, "\n");
    }

    public function outLine() : void
    {
        ++$this->lineNo;
        fputs($this->fh, "#line {$this->lineNo} \"{$this->executorFile}\"\n");
    }

    public function write(string $text) : void
    {
        fputs($this->fh, $text);
    }

    public function writef(string $text, ...$vars) : void
    {
        fprintf($this->fh, $text, ...$vars);
    }

    public function __destruct()
    {
        fclose($this->fh);
    }
}

function usage() {
    echo("\nUsage: php zend_vm_gen.php [options]\n".
         "\nOptions:".
         "\n  --with-vm-kind=CALL|SWITCH|GOTO - select threading model (default is CALL)".
         "\n  --without-specializer           - disable executor specialization".
         "\n  --with-lines                    - enable #line directives".
         "\n\n");
}

for ($i = 1; $i < $argc; ++$i) {
    if (strpos($argv[$i],'--with-vm-kind=') === 0) {
        $kind = substr($argv[$i], strlen('--with-vm-kind='));
        switch ($kind) {
            case 'CALL':
                define('ZEND_VM_KIND', ZendVmKind::CALL);
                break;
            case 'SWITCH':
                define('ZEND_VM_KIND', ZendVmKind::SWITCH);
                break;
            case 'GOTO':
                define('ZEND_VM_KIND', ZendVmKind::GOTO);
                break;
            default:
                echo("ERROR: Invalid vm kind '{$kind}'\n");
                usage();
                die();
        }
    } elseif ($argv[$i] === '--without-specializer') {
        // Disabling specialization
        define('ZEND_VM_SPEC', 0);
    } elseif ($argv[$i] === '--with-lines') {
        // Enabling debugging using original zend_vm_def.h
        define('ZEND_VM_LINES', 1);
    } else {
        if ($argv[$i] !== '--help') {
            echo("ERROR: Invalid option '{$argv[$i]}'\n");
        }
        usage();
        die();
    }
}

// Set defaults
if (!defined('ZEND_VM_KIND')) {
    define('ZEND_VM_KIND', ZendVmKind::CALL); // Use CALL threading
}
if (!defined('ZEND_VM_SPEC')) {
    define('ZEND_VM_SPEC', 1); // Use specialized executor
}
if (!defined('ZEND_VM_LINES')) {
    define('ZEND_VM_LINES', 0); // Disable #line directives
}

$vmDef = __DIR__ . '/zend_vm_def.h';
$vmExecuteSkel = __DIR__ . '/zend_vm_execute.skl';

$dfp = new DefinitionFileParser($vmDef);

$fw1 = new FileWriter(__DIR__ . '/zend_vm_opcodes.h');
$fw2 = new FileWriter(__DIR__ . '/zend_vm_opcodes.c');
$fw3 = new FileWriter(__DIR__ . '/zend_vm_execute.h', realpath(__DIR__ . '/zend_vm_execute.h'));

new OpcodesHeaderFileGen($dfp, $fw1);
new OpcodesMainFileGen($dfp, $fw2);
new ExecuteFileGen($dfp, $vmExecuteSkel, $fw3);
