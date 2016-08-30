PHP_ARG_ENABLE(phactor, whether to enable phactor support,
[  --enable-phactor           Enable phactor support])

if test "$PHP_PHACTOR" != "no"; then
  PHP_NEW_EXTENSION(phactor, phactor.c prepare.c store.c debug.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
