PHP_ARG_ENABLE(httprouter, for HTTPRouter support,
[  --enable-httprouter          Include HTTPRouter support])

if test "$PHP_HTTPROUTER" != "no"; then

  PHP_NEW_EXTENSION(httprouter, httprouter.c, $ext_shared)
  CFLAGS="$CFLAGS -Wall -g"

  AC_CHECK_HEADER(pcre.h, , [AC_MSG_ERROR([Couldn't find pcre.h, try installing the libpcre development/headers package])])

fi