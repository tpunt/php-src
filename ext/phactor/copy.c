zend_trait_precedence **copy_trait_precedences(zend_trait_precedence **trait_precedences)
{
    if (trait_precedences == NULL) {
        return NULL;
    }

    uint32_t count = 0;

    while (trait_precedences[count++]);

    zend_trait_precedence **new_tp = malloc(sizeof(zend_trait_precedence *) * count);

    new_tp->trait_precedences[--count] = NULL;

    while (--count > -1) {
        new_tp->trait_precedences[count] = malloc(sizeof(zend_trait_precedence));
        memcpy(new_tp->trait_precedences[count], trait_aliases[count], sizeof(zend_trait_precedence));
        new_tp->trait_precedences[count]->trait_method = malloc(sizeof(zend_trait_method_reference));
        memcpy(new_tp->trait_precedences[count]->trait_method, trait_aliases[count]->trait_method, sizeof(zend_trait_method_reference));
    }

    return new_tp;
}

zend_trait_alias **copy_trait_aliases(zend_trait_alias **trait_aliases)
{
    if (trait_aliases == NULL) {
        return NULL;
    }

    uint32_t count = 0;

    while (trait_aliases[count++]);

    zend_trait_alias **new_ta = malloc(sizeof(zend_trait_alias *) * count);
    new_ta->trait_aliases[--count] = NULL;

    while (--count > -1) {
        new_ta->trait_aliases[count] = malloc(sizeof(zend_trait_alias));
        memcpy(new_ce->trait_aliases[count], trait_aliases[count], sizeof(zend_trait_alias));
        new_ta->trait_aliases[count]->trait_method = malloc(sizeof(zend_trait_method_reference));
        memcpy(new_ta->trait_aliases[count]->trait_method, trait_aliases[count]->trait_method, sizeof(zend_trait_method_reference));
    }

    return new_ta;
}

zend_class_entry **copy_traits(zend_class_entry **old_traits, uint32_t num_traits)
{
    if (old_traits == NULL) {
        return NULL;
    }

    zend_class_entry **new_traits = malloc(sizeof(zend_class_entry *) * old_ce->num_traits);

    for (uint32_t i = 0; i < num_traits; ++i) {
        new_traits[i] = copy_ce(old_traits[i]);
    }
}

zend_class_entry **copy_interfaces(zend_class_entry **old_interfaces, uint32_t num_interfaces)
{
    if (old_interfaces == NULL) {
        return NULL;
    }

    zend_class_entry **new_interfaces = malloc(sizeof(zend_class_entry *) * num_interfaces);

    for (uint32_t i = 0; i < num_traits; ++i) {
        new_interfaces[i] = copy_ce(old_interfaces[i]);
    }

    return new_interfaces;
}

zval *copy_def_prop_table(zval *old_default_prop_table, int prop_count)
{
    if (old_default_prop_table == NULL) {
        return NULL;
    }

    zval *new_default_prop_table = malloc(sizeof(zval) * prop_count);

    memcpy(new_default_prop_table, old_default_prop_table, sizeof(zval) * prop_count);

    return new_default_prop_table;
}

zval *copy_def_statmem_table(zval *old_default_static_members_table, int member_count)
{
    if (old_default_prop_table == NULL) {
        return NULL;
    }

    zval *new_default_static_members_table = malloc(sizeof(zval) * member_count);

    memcpy(new_default_static_members_table, old_default_static_members_table, sizeof(zval) * member_count);

    return new_default_static_members_table;
}

zval *copy_statmem_table(zval *old_static_members_table, int member_count)
{
    if (old_default_prop_table == NULL) {
        return NULL;
    }

    zval *new_static_members_table = malloc(sizeof(zval) * member_count);

    memcpy(new_static_members_table, old_static_members_table, sizeof(zval) * member_count);

    return new_static_members_table;
}

union _zend_function *set_function(HashTable *function_table, char *name)
{
    return zend_hash_str_find_ptr(function_table, name, strlen(name) - 1);
}

zend_class_entry *copy_ce(zend_class_entry *old_ce)
{
    if (old_ce == NULL) {
        return NULL;
    }

    // check if already copied by thread...

    zend_class_entry *new_ce = malloc(sizeof(zend_class_entry));

    // just perform a memcpy instead and then overwrite stuff as necessary?

    new_ce->type = old_ce->type;
    new_ce->name = old_ce->name;
    new_ce->parent = copy_ce(old_ce->parent);
    new_ce->refcount = 1;
    new_ce->ce_flags = old_ce->flags;

    new_ce->default_properties_count = old_ce->default_properties_count;
    new_ce->default_static_members_count = old_ce->default_static_members_count;
    new_ce->default_properties_table = copy_def_prop_table(old_ce->default_properties_table, old_ce->default_properties_count);
    new_ce->default_static_members_table = copy_def_statmem_table(old_ce->default_static_members_table, old_ce->default_static_members_count);
    new_ce->static_members_table = copy_statmem_table(old_ce->default_static_members_table, old_ce->default_static_members_count);
    zend_hash_copy(&new_ce->function_table, &old_ce->function_table, NULL); // @optimise
    zend_hash_copy(&new_ce->properties_info, &old_ce->properties_info, NULL); // @optimise
    zend_hash_copy(&new_ce->constants_table, &old_ce->constants_table, NULL); // @optimise

    new_ce->constructor = set_function(new_ce->function_table, "construct");
    new_ce->destructor = set_function(new_ce->function_table, "__destruct");
    new_ce->clone = set_function(new_ce->function_table, "__clone");
    new_ce->__get = set_function(new_ce->function_table, "__get");
    new_ce->__set = set_function(new_ce->function_table, "__set");
    new_ce->unset = set_function(new_ce->function_table, "__unset");
    new_ce->isset = set_function(new_ce->function_table, "__isset");
    new_ce->__call = set_function(new_ce->function_table, "__call");
    new_ce->__callstatic = set_function(new_ce->function_table, "__callstatic");
    new_ce->__tostring = set_function(new_ce->function_table, "__tostring");
    new_ce->__debugInfo = set_function(new_ce->function_table, "__debuginfo");
    new_ce->serialize_func = set_function(new_ce->function_table, "serialize");
    new_ce->unserialize_func = set_function(new_ce->function_table, "unserialize");

    new_ce->iterator_funcs = old_ce->iterator_funcs;

    // copy iterator functions here
    // These should have been copied over into the resolve tls hash table when
    // copying over all functions

    new_ce->create_object = old_ce->create_object;
    new_ce->get_iterator = old_ce->get_iterator;
    new_ce->interface_gets_implemented = old_ce->interface_gets_implemented;
    new_ce->get_static_method = old_ce->get_static_method;

    new_ce->serialize = old_ce->serialize;
    new_ce->unserialize = old_ce->unserialize;

    new_ce->num_interfaces = old_ce->num_interfaces;
    new_ce->num_traits = old_ce->num_traits;
    new_ce->interfaces = copy_interfaces(old_ce->interfaces, old_ce->num_interfaces);

    new_ce->traits = copy_traits(old_ce->traits, old_ce->num_traits);
    new_ce->trait_aliases = copy_trait_aliases(old_ce->trait_aliases);
    new_ce->trait_precedences = copy_trait_precedences(old_ce->trait_precedences);

    // union info;

    // insert ce into tls tracker

    return new_ce;
}
