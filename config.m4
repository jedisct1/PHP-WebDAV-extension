dnl $Id: config.m4 213 2007-03-11 20:37:47Z j $
dnl config.m4 for extension dav

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(dav, for dav support,
[  --with-dav             Include dav support])

if test "$PHP_DAV" != "no"; then
  for i in $PHP_DAV /usr/local /usr; do
    if test -x "$i/bin/neon-config"; then
      NEON_CONFIG=$i/bin/neon-config
      break
    fi
  done

  DAV_LIBS=$($NEON_CONFIG --libs)
  DAV_INCS=$($NEON_CONFIG --cflags)
  
  PHP_EVAL_LIBLINE($DAV_LIBS, DAV_SHARED_LIBADD)
  PHP_EVAL_INCLINE($DAV_INCS)  

  PHP_SUBST(DAV_SHARED_LIBADD)
  PHP_NEW_EXTENSION(dav, dav.c, $ext_shared)
fi
