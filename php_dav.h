/* $Id: php_dav.h 213 2007-03-11 20:37:47Z j $ */

#ifndef PHP_DAV_H
#define PHP_DAV_H

extern zend_module_entry dav_module_entry;
#define phpext_dav_ptr &dav_module_entry

#ifdef PHP_WIN32
#define PHP_DAV_API __declspec(dllexport)
#else
#define PHP_DAV_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(dav);
PHP_MSHUTDOWN_FUNCTION(dav);
PHP_RINIT_FUNCTION(dav);
PHP_RSHUTDOWN_FUNCTION(dav);
PHP_MINFO_FUNCTION(dav);

PHP_FUNCTION(webdav_connect);
PHP_FUNCTION(webdav_open);
PHP_FUNCTION(webdav_close);
PHP_FUNCTION(webdav_get);
PHP_FUNCTION(webdav_put);
PHP_FUNCTION(webdav_delete);
PHP_FUNCTION(webdav_remove);
PHP_FUNCTION(webdav_unlink);
PHP_FUNCTION(webdav_rmdir);
PHP_FUNCTION(webdav_mkcol);
PHP_FUNCTION(webdav_mkdir);
PHP_FUNCTION(webdav_copy);
PHP_FUNCTION(webdav_move);
PHP_FUNCTION(webdav_rename);

ZEND_BEGIN_MODULE_GLOBALS(dav)
	int default_link;
ZEND_END_MODULE_GLOBALS(dav)

#ifdef ZTS
#define DAV_G(v) TSRMG(dav_globals_id, zend_dav_globals *, v)
#else
#define DAV_G(v) (dav_globals.v)
#endif

#endif	/* PHP_DAV_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
