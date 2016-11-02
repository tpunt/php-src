#ifndef COPY_H
#define COPY_H

#include "phactor.h"

HashTable *copy_ces(HashTable *class_table);
zend_class_entry *copy_ce(zend_class_entry *old_ce);
zend_trait_precedence **copy_trait_precedences(zend_trait_precedence **trait_precedences);
zend_trait_alias **copy_trait_aliases(zend_trait_alias **trait_aliases);
zend_class_entry **copy_traits(zend_class_entry **old_traits, uint32_t num_traits);
zend_class_entry **copy_interfaces(zend_class_entry **old_interfaces, uint32_t num_interfaces);
zval *copy_def_prop_table(zval *old_default_prop_table, int prop_count);
zval *copy_def_statmem_table(zval *old_default_static_members_table, int member_count);
zval *copy_statmem_table(zval *old_static_members_table, int member_count);
zend_function *set_function_from_name(HashTable *function_table, char *name);
void copy_iterator_functions(zend_class_iterator_funcs *new_if, zend_class_iterator_funcs old_if);
zend_class_entry *create_new_ce(void);

#endif
