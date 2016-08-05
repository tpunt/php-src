PHP_ARG_ENABLE(testing, whether to enable testing support,
[  --enable-testing           Enable testing support])

if test "$PHP_TESTING" != "no"; then
  PHP_NEW_EXTENSION(testing, testing.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
