
/* (C) 2007-2012 Frank DENIS <j at pureftpd.org>
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_dav.h"
#include <ne_socket.h>
#include <ne_session.h>
#include <ne_utils.h>
#include <ne_auth.h>
#include <ne_basic.h>
#include <ne_207.h>
#include "zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"

ZEND_DECLARE_MODULE_GLOBALS(dav)

/* True global resources - no need for thread safety here */
static int le_dav_session;
#define le_dav_session_name "DAV Session Buffer"

typedef struct DavSession_ {
    ne_session *sess;
    char *base_uri_path;
    size_t base_uri_path_len;
    char *user_name;
    char *user_password;
} DavSession;

/* {{{ dav_functions[]
 *
 * Every user visible function must have an entry in dav_functions[].
 */
zend_function_entry dav_functions[] = {
    PHP_FE(webdav_connect, NULL)
    PHP_FALIAS(webdav_open, webdav_connect, NULL)
    PHP_FE(webdav_close, NULL)
    PHP_FE(webdav_get, NULL)
    PHP_FE(webdav_put, NULL)
    PHP_FE(webdav_delete, NULL)
    PHP_FALIAS(webdav_unlink, webdav_delete, NULL)
    PHP_FALIAS(webdav_remove, webdav_delete, NULL)
    PHP_FALIAS(webdav_rmdir, webdav_delete, NULL)
    PHP_FE(webdav_mkcol, NULL)
    PHP_FALIAS(webdav_mkdir, webdav_mkcol, NULL)
    PHP_FE(webdav_copy, NULL)
    PHP_FE(webdav_move, NULL)
    PHP_FALIAS(webdav_rename, webdav_move, NULL)
    {NULL, NULL, NULL}  /* Must be the last line in dav_functions[] */
};
/* }}} */

/* {{{ dav_module_entry
 */
zend_module_entry dav_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
        "dav",
        dav_functions,
        PHP_MINIT(dav),
    PHP_MSHUTDOWN(dav),
    PHP_RINIT(dav),
        NULL,
        PHP_MINFO(dav),
#if ZEND_MODULE_API_NO >= 20010901
    "1.3",
#endif
        STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_DAV
ZEND_GET_MODULE(dav)
#endif

static void dav_set_default_link(int id TSRMLS_DC)
{
    if (DAV_G(default_link) != -1) {
        zend_list_delete(DAV_G(default_link));
    }
    DAV_G(default_link) = id;
    zend_list_addref(id);
}

static int dav_get_default_link(INTERNAL_FUNCTION_PARAMETERS)
{
    return DAV_G(default_link);
}

PHP_RINIT_FUNCTION(dav)
{
    DAV_G(default_link) = -1;

    return SUCCESS;
}

/* {{{ proto string webdav_close([resource session])
 Close a DAV session */

PHP_FUNCTION(webdav_close)
{
    zval *z_dav;
    DavSession *dav_session;
    int id = -1;

    if (ZEND_NUM_ARGS() TSRMLS_CC < 1) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    } else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r", &z_dav) == FAILURE) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_close", 0 TSRMLS_CC);
        }
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    if (id == -1) {
        if (zend_list_delete(Z_LVAL_P(z_dav)) != SUCCESS) {
            zend_throw_exception(spl_ce_RuntimeException, "Can`t delete resource", 0 TSRMLS_CC);
        }
    }
    if (id != -1 || (z_dav && Z_RESVAL_P(z_dav)) == DAV_G(default_link)) {
        if (zend_list_delete(DAV_G(default_link)) != SUCCESS) {
            zend_throw_exception(spl_ce_RuntimeException, "Can`t delete default resource", 0 TSRMLS_CC);
        }
        dav_set_default_link(-1 TSRMLS_CC);
    }
    RETURN_TRUE;
}

static void dav_destructor_dav_session(zend_rsrc_list_entry * rsrc TSRMLS_DC)
{
    DavSession *dav_session = (DavSession *) rsrc->ptr;

    if (dav_session->sess != NULL) {
        ne_session_destroy(dav_session->sess);
        ne_sock_exit();
        efree(dav_session->base_uri_path);
        efree(dav_session->user_name);
        efree(dav_session->user_password);
        dav_session->sess = NULL;
    }
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(dav)
{
    le_dav_session = zend_register_list_destructors_ex
        (dav_destructor_dav_session, NULL, le_dav_session_name, module_number);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(dav)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(dav)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "dav support", "enabled");
    php_info_print_table_end();
}
/* }}} */

static int cb_dav_auth(void *userdata, const char *realm, int attempts,
                       char *username, char *password) {
    const DavSession *dav_session = (const DavSession *) userdata;

    strlcpy(username, dav_session->user_name, NE_ABUFSIZ);
    strlcpy(password, dav_session->user_password, NE_ABUFSIZ);

    return attempts;
}

/* {{{ proto resource webdav_connect(string base_url [, string user [, string password [, int timeout]]])
 Initialize a new DAV session */
PHP_FUNCTION(webdav_connect)
{
    char *arg = NULL;
    int arg_len, len;
    char *base_url = NULL;
    int base_url_len;
    char *user_name = NULL, *user_password = NULL;
    int user_name_len, user_password_len;
    long timeout = 5L;
    DavSession *dav_session;
    ne_session *sess;
    ne_uri uri;

    memset(&uri, 0, sizeof uri);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ssl",
                              &base_url, &base_url_len,
                              &user_name, &user_name_len,
                              &user_password, &user_password_len,
                              &timeout) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_connect", 0 TSRMLS_CC);
    }
    if (ne_uri_parse(base_url, &uri) != 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Invalid base URL", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 4) {
        timeout = 5L;
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 3) {
        user_password = NULL;
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 2) {
        user_name = NULL;
    }
    if (timeout < 0L || timeout > INT_MAX) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Invalid timeout", 0 TSRMLS_CC);
    }
    if (uri.scheme == NULL) {
        uri.scheme = "http";
    }
    if (uri.port == 0) {
        uri.port = ne_uri_defaultport(uri.scheme);
    }
    if (ne_sock_init()) {
        zend_throw_exception(spl_ce_RuntimeException, "Unable to initialize socket libraries", 0 TSRMLS_CC);
    }
    if ((sess = ne_session_create(uri.scheme, uri.host, uri.port)) == NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "Unable to open a new DAV session", 0 TSRMLS_CC);
    }
    ne_set_read_timeout(sess, (int) timeout);
    dav_session = emalloc(sizeof *dav_session);
    dav_session->base_uri_path = estrdup(uri.path);
    dav_session->base_uri_path_len = strlen(uri.path);
    if (user_name == NULL) {
        dav_session->user_name = NULL;
    } else {
        dav_session->user_name = estrdup(user_name);
    }
    if (user_password == NULL) {
        dav_session->user_password = NULL;
    } else {
        dav_session->user_password = estrdup(user_password);
    }
    dav_session->sess = sess;
    ZEND_REGISTER_RESOURCE(return_value, dav_session, le_dav_session);
    if (user_name != NULL && user_password != NULL) {
        ne_set_server_auth(sess, cb_dav_auth, dav_session);
    }
    dav_set_default_link(Z_LVAL_P(return_value) TSRMLS_CC);
}
/* }}} */

static int cb_dav_reader(void *userdata, const char *buf, size_t len) {
    zval * const return_value = (zval * const) userdata;
    size_t old_len, full_len;

    if (len <= (size_t) 0U) {
        return 0;
    }
    old_len = Z_STRLEN_P(return_value);
    full_len = old_len + len;
    if (full_len < old_len || full_len < len) {
        return -1;
    }
    Z_STRVAL_P(return_value) = erealloc(Z_STRVAL_P(return_value), full_len);
    Z_STRLEN_P(return_value) = full_len;
    memcpy(Z_STRVAL_P(return_value) + old_len, buf, len);

    return 0;
}

static char *get_full_uri(const DavSession * const dav_session,
                          const char * const relative_uri)
{
    char *full_uri;
    size_t relative_uri_len = strlen(relative_uri) + 1U;
    const size_t wanted_len = dav_session->base_uri_path_len +
        relative_uri_len;

    if (wanted_len < dav_session->base_uri_path_len ||
        wanted_len < relative_uri_len) {
        return NULL;
    }
    full_uri = emalloc(wanted_len);
    memcpy(full_uri,
           dav_session->base_uri_path, dav_session->base_uri_path_len);
    memcpy(full_uri + dav_session->base_uri_path_len,
           relative_uri, relative_uri_len);

    return full_uri;
}

/* {{{ proto string webdav_get(string uri [, resource session])
 Fetch data */
PHP_FUNCTION(webdav_get)
{
    zval *z_dav;
    DavSession *dav_session;
    char *relative_uri;
    int relative_uri_len;
    char *uri;
    ne_session *sess;
    ne_request *req;
    int ret;
    int id = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "s|r", &relative_uri, &relative_uri_len,
                              &z_dav) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_get", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 2) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    sess = dav_session->sess;
    if ((uri = get_full_uri(dav_session, relative_uri)) == NULL) {
        RETURN_FALSE;
    }
    req = ne_request_create(sess, "GET", uri);
    RETVAL_EMPTY_STRING();
    ne_add_response_body_reader(req, ne_accept_2xx,
                                cb_dav_reader, return_value);
    ret = ne_request_dispatch(req);
    ne_request_destroy(req);
    efree(uri);
    if (ret != NE_OK || ne_get_status(req)->klass != 2) {
        zend_throw_exception(spl_ce_RuntimeException, ne_get_error(sess), 0 TSRMLS_CC);
    }
}

/* {{{ proto bool webdav_put(string uri, string data [, resource session])
 Store data */
PHP_FUNCTION(webdav_put)
{
    zval *z_dav;
    DavSession *dav_session;
    char *relative_uri;
    int relative_uri_len;
    char *data;
    int data_len;
    char *uri;
    ne_session *sess;
    ne_request *req;
    int ret;
    int id = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "ss|r", &relative_uri, &relative_uri_len,
                              &data, &data_len, &z_dav) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_pub", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 3) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    sess = dav_session->sess;
    if ((uri = get_full_uri(dav_session, relative_uri)) == NULL) {
        RETURN_FALSE;
    }
    req = ne_request_create(sess, "PUT", uri);
    ne_set_request_body_buffer(req, data, data_len);
    ret = ne_request_dispatch(req);
    ne_request_destroy(req);
    efree(uri);
    if (ret != NE_OK || ne_get_status(req)->klass != 2) {
        zend_throw_exception(spl_ce_RuntimeException, ne_get_error(sess), 0 TSRMLS_CC);
    }
    RETURN_TRUE;
}

/* {{{ proto bool webdav_delete(string uri [, resource session])
 Delete an object */
PHP_FUNCTION(webdav_delete)
{
    zval *z_dav;
    DavSession *dav_session;
    char *relative_uri;
    int relative_uri_len;
    char *uri;
    ne_session *sess;
    ne_request *req;
    int ret;
    int id = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "s|r", &relative_uri, &relative_uri_len,
                              &z_dav) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_delete", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 2) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    sess = dav_session->sess;
    if ((uri = get_full_uri(dav_session, relative_uri)) == NULL) {
        RETURN_FALSE;
    }
    req = ne_request_create(sess, "DELETE", uri);
    ret = ne_simple_request(sess, req);
    efree(uri);
    if (ret != NE_OK || ne_get_status(req)->klass != 2) {
        zend_throw_exception(spl_ce_RuntimeException, ne_get_error(sess), 0 TSRMLS_CC);
    }
    RETURN_TRUE;
}

/* {{{ proto bool webdav_mkcol(string uri [, resource session])
 Create a new collection */
PHP_FUNCTION(webdav_mkcol)
{
    zval *z_dav;
    DavSession *dav_session;
    char *relative_uri;
    int relative_uri_len;
    char *uri;
    ne_session *sess;
    ne_request *req;
    int ret;
    int id = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "s|r", &relative_uri, &relative_uri_len,
                              &z_dav) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_mkcol", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 2) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    sess = dav_session->sess;
    if ((uri = get_full_uri(dav_session, relative_uri)) == NULL) {
        RETURN_FALSE;
    }
    req = ne_request_create(sess, "MKCOL", uri);
    ret = ne_simple_request(sess, req);
    efree(uri);
    if (ret != NE_OK || ne_get_status(req)->klass != 2) {
        zend_throw_exception(spl_ce_RuntimeException, ne_get_error(sess), 0 TSRMLS_CC);
    }
    RETURN_TRUE;
}

/* {{{ proto bool webdav_copy(string source_uri, string target_uri [, bool overwrite [, bool recursive [, resource session]]])
 Copy an object */
PHP_FUNCTION(webdav_copy)
{
    zval *z_dav;
    DavSession *dav_session;
    char *relative_source_uri;
    int relative_source_uri_len;
    char *relative_target_uri;
    int relative_target_uri_len;
    char *source_uri, *target_uri;
    ne_session *sess;
    int ret;
    int id = -1;
    zend_bool overwrite = 1;
    zend_bool depth = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "ss|bbr",
                              &relative_source_uri, &relative_source_uri_len,
                              &relative_target_uri, &relative_target_uri_len,
                              &overwrite, &depth, &z_dav) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_copy", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 5) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 4) {
        depth = 1;
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 3) {
        overwrite = 1;
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    sess = dav_session->sess;
    if ((source_uri = get_full_uri(dav_session,
                                   relative_source_uri)) == NULL) {
        RETURN_FALSE;
    }
    if ((target_uri = get_full_uri(dav_session,
                                   relative_target_uri)) == NULL) {
        efree(source_uri);
        RETURN_FALSE;
    }
    ret = ne_copy(sess, (int) overwrite,
                  depth ? NE_DEPTH_INFINITE : NE_DEPTH_ZERO,
                  source_uri, target_uri);
    efree(source_uri);
    efree(target_uri);
    if (ret != NE_OK) {
        zend_throw_exception(spl_ce_RuntimeException, ne_get_error(sess), 0 TSRMLS_CC);
    }
    RETURN_TRUE;
}

/* {{{ proto bool webdav_move(string source_uri, string target_uri [, bool overwrite, [, resource session]])
 Move an object */
PHP_FUNCTION(webdav_move)
{
    zval *z_dav;
    DavSession *dav_session;
    char *relative_source_uri;
    int relative_source_uri_len;
    char *relative_target_uri;
    int relative_target_uri_len;
    char *source_uri, *target_uri;
    ne_session *sess;
    int ret;
    int id = -1;
    zend_bool overwrite = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "ss|br",
                              &relative_source_uri, &relative_source_uri_len,
                              &relative_target_uri, &relative_target_uri_len,
                              &overwrite, &z_dav) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Can`t parse parameters webdav_move", 0 TSRMLS_CC);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 4) {
        id = dav_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
    if (ZEND_NUM_ARGS() TSRMLS_CC < 3) {
        overwrite = 1;
    }
    if (z_dav == NULL && id == -1) {
        zend_throw_exception(spl_ce_UnexpectedValueException, "No link", 0 TSRMLS_CC);
    }
    ZEND_FETCH_RESOURCE(dav_session, DavSession *, &z_dav, id,
                        le_dav_session_name, le_dav_session);
    sess = dav_session->sess;
    if ((source_uri = get_full_uri(dav_session,
                                   relative_source_uri)) == NULL) {
        RETURN_FALSE;
    }
    if ((target_uri = get_full_uri(dav_session,
                                   relative_target_uri)) == NULL) {
        efree(source_uri);
        RETURN_FALSE;
    }
    ret = ne_move(sess, (int) overwrite, source_uri, target_uri);
    efree(source_uri);
    efree(target_uri);

    if (ret != NE_OK) {
        zend_throw_exception(spl_ce_RuntimeException, ne_get_error(sess), 0 TSRMLS_CC);
    }
    RETURN_TRUE;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
