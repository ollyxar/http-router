#ifndef PHP_HTTPROUTER_H
#define PHP_HTTPROUTER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include "ext/pcre/php_pcre.h"
#include "ext/standard/info.h"

#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

#define PHP_HTTPROUTER_VERSION "1.0.0"

extern zend_module_entry httprouter_module_entry;

#define phpext_httprouter_ptr &httprouter_module_entry

#if defined(ZTS) && defined(COMPILE_DL_HTTPROUTER)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

PHP_MINIT_FUNCTION(httprouter);
PHP_MSHUTDOWN_FUNCTION(httprouter);
PHP_MINFO_FUNCTION(httprouter);

typedef struct {
	char *uri;
	char *method;
	char *prefix;
	zval *parameters;
	size_t parameters_len;
	zval *this_ptr;
	void ***thread_ctx;
	zend_object zo;
} ro_object;

static inline ro_object *ro_object_from_obj(zend_object *obj) {
    return (ro_object*)((char*)(obj) - XtOffsetOf(ro_object, zo));
}

static inline ro_object *Z_ROO_P(zval *zv) {
	ro_object *soo = ro_object_from_obj(Z_OBJ_P((zv)));
	soo->this_ptr = zv;
	return soo;
}

#define RETURN_THIS() RETURN_ZVAL(getThis(), 1, 0)

#define to_zend_string(s) zend_string_init(s, strlen(s), 0)

static inline int __mustcut(int _c) {
	return _c == ' ' || _c == '/' || (unsigned)_c-'\t' < 5;
}

#define mustcut(a) __mustcut(a)

#define update_property_string(name, value) zend_update_property_string(roo_class_entry, getThis(), name, sizeof(name) - 1, value)


#if PHP_VERSION_ID < 70200
#define regex_replace(regex, subject, replace) \
    php_pcre_replace( \
        regex, \
        subject, \
        ZSTR_VAL(subject), \
        ZSTR_LEN(subject), \
        replace, \
        0, \
        -1, \
        NULL \
    )
#else
#define regex_replace(regex, subject, replace) \
    php_pcre_replace( \
        regex, \
        subject, \
        ZSTR_VAL(subject), \
        ZSTR_LEN(subject), \
        Z_STR_P(replace), \
        -1, \
        NULL \
    )
#endif


#endif