#include "php_httprouter.h"

static zend_class_entry *roo_class_entry;
static zend_object_handlers ro_object_handlers;

static zend_object* ro_object_new(zend_class_entry *ce)
{
	ro_object *ro;

	ro = ecalloc(1, sizeof(ro_object) + zend_object_properties_size(ce));
	ro->prefix = NULL;
	ro->uri = NULL;
	ro->method = NULL;
	ro->parameters = NULL;

	zend_object_std_init(&ro->zo, ce);
	object_properties_init(&ro->zo, ce);

	ro->zo.handlers = &ro_object_handlers;

	return &ro->zo;
}

static zend_object *httprouter_clone_obj(zval *this_ptr)
{
	ro_object *old_obj = Z_ROO_P(this_ptr);
	ro_object *new_obj = ro_object_from_obj(ro_object_new(old_obj->zo.ce));

	zend_objects_clone_members(&new_obj->zo, &old_obj->zo);

	return &new_obj->zo;
}

static void ro_object_free_storage(zend_object *obj)
{
	ro_object *ro;

	ro = ro_object_from_obj(obj);

	if (ro->uri) {
		free(ro->uri);
	}

	if (ro->method) {
		free(ro->method);
	}

	zend_object_std_dtor(&ro->zo);
	
	if (ro->parameters) {
		free(ro->parameters);
	}

	if (ro->prefix) {
		free(ro->prefix);
	}
}

static char *trim_helper(char *str)
{
	char *end;

	while (mustcut((unsigned char)*str))
		str++;

	if (*str == 0)
		return str;

	end = str + strlen(str) - 1;

	while (end > str && isspace((unsigned char)*end))
		end--;

	end[1] = '\0';

	return str;
}

static int strpos(char *haystack, char *needle)
{
	char *p = strstr(haystack, needle);

	if (p)
		return p - haystack;

	return -1;
}

static char *regex_replace_char(const char *regex, const char *replacement, const char *input)
{
	zend_string *zs_input, *zs_replaced, *zs_regex;
	zval z_replacement;
	char *ret;
	ZVAL_STRING(&z_replacement, replacement);

	zs_regex = to_zend_string(regex);
	zs_input = to_zend_string(input);
	zs_replaced = regex_replace(zs_regex, zs_input, &z_replacement);
	zend_string_release(zs_input);
	zend_string_release(zs_regex);

	ret = ZSTR_VAL(zs_replaced);

	zend_string_release(zs_replaced);

	return ret;
}

PHP_METHOD(httprouter, __construct)
{
	char *uri, *method;
	size_t uri_len = 0, method_len = 0;
	ro_object *ro;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
        Z_PARAM_STRING(uri, uri_len)
		Z_PARAM_STRING(method, method_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	if (method_len == 0) {
		zend_throw_exception(NULL, "Request method cannot be empty", -1);

		return;
	}

	ro = Z_ROO_P(getThis());

	ro->parameters = malloc(sizeof(zval));
	ro->prefix = malloc(1);
	ro->uri = malloc(uri_len);
	ro->method = malloc(method_len);

	strcpy(ro->prefix, "");
	strcpy(ro->uri, uri);
	strcpy(ro->method, method);

	if (uri_len > 0) {
		ro->uri = trim_helper(strtok(ro->uri, "?"));
	}

	update_property_string("uri", ro->uri);
	update_property_string("method", ro->method);

	TSRMLS_SET_CTX(ro->thread_ctx);
}

PHP_METHOD(httprouter, action)
{
	zval retval, method_name, *action, rv;
	zend_string *class_name, *fname = NULL;
	zend_class_entry *ce;

	ro_object *ro = Z_ROO_P(getThis());

	action = zend_read_property(roo_class_entry, getThis(), "action", sizeof("action") - 1, 1, &rv);

	if (action == NULL || Z_TYPE_P(action) == IS_NULL) {
		return;
	}

	if (zend_is_callable(action, 0, &fname)) {
		zval z_fname;
		ZVAL_STR(&z_fname, fname);

		call_user_function_ex(NULL, action, &z_fname, &retval, ro->parameters_len, ro->parameters, 0, NULL);

		*return_value = retval;
		zend_string_release(fname);

		return;
	}

	zend_string_release(fname);

	if (Z_TYPE_P(action) == IS_STRING) {
		zval *obj = malloc(sizeof(zval*));
		char *action_buf = malloc(sizeof(char*));

		strcpy(action_buf, Z_STRVAL_P(action));
		action_buf = strtok(action_buf, "@");

		class_name = to_zend_string(action_buf);

		if ((action_buf = strtok(NULL, "@")) == NULL) {
			zend_string_release(class_name);
			free(action_buf);
			free(obj);

			return;
		}

		ZVAL_STRING(&method_name, action_buf);
		free(action_buf);
		
		ce = zend_fetch_class(class_name, ZEND_FETCH_CLASS_DEFAULT);
		
		zend_string_release(class_name);
		object_init_ex(obj, ce);

		call_user_function_ex(NULL, obj, &method_name, &retval, ro->parameters_len, ro->parameters, 0, NULL);

		*return_value = retval;

		free(obj);
		zval_ptr_dtor(&method_name);

		return;
	}
}

PHP_METHOD(httprouter, method)
{
	ro_object *ro;
	pcre_cache_entry *pce;
	char *regex, *method_name, *url_pattern;
	size_t method_name_len = 0, url_pattern_len = 0;
	zval subpats, ret, *item_param, *current_param;
	HashPosition hpos;
	zval *action;

	ro = Z_ROO_P(getThis());

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_STRING(method_name, method_name_len)
		Z_PARAM_STRING(url_pattern, url_pattern_len)
		Z_PARAM_ZVAL(action)
	ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	if (Z_TYPE_P(action) == IS_NULL) {
		zend_throw_exception(NULL, "Action does not specified", -1);

		return;
	}

	if (strcmp(method_name, ro->method) != 0) {
		RETURN_THIS();
	}

	zend_update_property(roo_class_entry, getThis(), "action", sizeof("action") - 1, action);

#if PHP_VERSION_ID >= 70400
	zend_string *uri = to_zend_string(ro->uri);
#endif

	char full_url_pattern[strlen(ro->prefix) + url_pattern_len];
	strcpy(full_url_pattern, ro->prefix);
	strcat(full_url_pattern, url_pattern);

	char *res = regex_replace_char(
		"#\\{(\\w+?)\\}#",
		"([\\w\\-]+)",
		full_url_pattern
	);

	regex = malloc(strlen(res) + 5);
	strcpy(regex, "#^");
	strcat(regex, res);
	strcat(regex, "$#");
	zend_string *z_regex = to_zend_string(regex);

	pce = pcre_get_compiled_regex_cache(z_regex);

	free(regex);
	zend_string_release(z_regex);

	if (pce == NULL) {
		zend_throw_exception(NULL, "Invalid regex caching", -1);
		return;
	}

	ZVAL_NULL(&subpats);
	ZVAL_NULL(&ret);

	php_pcre_match_impl(
		pce,
#if PHP_VERSION_ID >= 70400
		uri,
#else
		ro->uri,
		strlen(ro->uri),
#endif
		&ret,
		&subpats,
		1, /* global */
		1, /* use flags */
		2, /* PREG_SET_ORDER */
		0
	);

#if PHP_VERSION_ID >= 70400
	zend_string_release(uri);
#endif

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL(subpats), &hpos);

	if ((item_param = zend_hash_get_current_data_ex(Z_ARRVAL(subpats), &hpos)) != NULL) {
		zend_hash_internal_pointer_reset(Z_ARRVAL_P(item_param));

		for (int i = 0; zend_hash_move_forward(Z_ARRVAL_P(item_param)) == SUCCESS; i++) { 
			// for-trick to skip first element
			current_param = zend_hash_get_current_data(Z_ARRVAL_P(item_param));

			if (current_param) {
				ro->parameters = realloc(ro->parameters, sizeof(zval) * (i + 1));
				ZVAL_COPY(&ro->parameters[i], current_param);
				ro->parameters_len = i + 1;
			}
		};
	}

	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&subpats);

	RETURN_THIS();
}

PHP_METHOD(httprouter, group)
{
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval retval;
	char *prefix;
	size_t prefix_len = 0;
	ro_object *ro;

	ro = Z_ROO_P(getThis());

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STRING(prefix, prefix_len)
		Z_PARAM_FUNC(fci, fcc)
	ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	if (prefix_len == 0) {
		zend_throw_exception(NULL, "Prefix cannot be empty", -1);
		return;
	}

	char new_prefix[strlen(ro->prefix) + prefix_len];
	strcpy(new_prefix, ro->prefix);
	strcat(new_prefix, prefix);

	if (strpos(ro->uri, new_prefix) == 0) {
		ro->prefix = realloc(ro->prefix, strlen(new_prefix) + 1);
		strcpy(ro->prefix, new_prefix);
		strcat(ro->prefix, "/");
		update_property_string("prefix", ro->prefix);

		fci.param_count = 0;
		fci.retval = &retval;

		zend_call_function(&fci, &fcc);
		zval_ptr_dtor(&retval);
	}

	RETURN_THIS();
}

PHP_FUNCTION(httprouter_test1)
{
	ZEND_PARSE_PARAMETERS_NONE();

	php_printf("The extension %s is loaded and working!\r\n", "HTTPRouter");
}

ZEND_BEGIN_ARG_INFO(arginfo_httprouter_test1, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_httprouter__construct, 0, 0, 2)
	ZEND_ARG_INFO(0, uri)
	ZEND_ARG_INFO(0, method)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_httprouter_method, 0, 0, 3)
	ZEND_ARG_INFO(0, method_name)
	ZEND_ARG_INFO(0, url_pattern)
	ZEND_ARG_INFO(0, action)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_httprouter_action, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_httprouter_group, 0, ZEND_RETURN_REFERENCE, 2)
	ZEND_ARG_INFO(0, prefix)
    ZEND_ARG_INFO(0, call_back)
ZEND_END_ARG_INFO()

static zend_function_entry so_functions[] = {
	PHP_ME(httprouter,	__construct,	arginfo_httprouter__construct,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(httprouter,	method,			arginfo_httprouter_method,		ZEND_ACC_PROTECTED)
	PHP_ME(httprouter,	group,			arginfo_httprouter_group,		ZEND_ACC_PUBLIC)
	PHP_ME(httprouter,	action,			arginfo_httprouter_action,		ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static const zend_function_entry httprouter_functions[] = {
	PHP_FE(httprouter_test1, arginfo_httprouter_test1)
	PHP_FE_END
};

PHP_RINIT_FUNCTION(httprouter)
{
#if defined(ZTS) && defined(COMPILE_DL_HTTPROUTER)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}

PHP_MINIT_FUNCTION(httprouter)
{
	zend_class_entry soce;
	INIT_CLASS_ENTRY(soce, "HTTPRouter", so_functions);
	soce.create_object = ro_object_new;
	roo_class_entry = zend_register_internal_class(&soce);
	memcpy(&ro_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	
	ro_object_handlers.offset		 	= XtOffsetOf(ro_object, zo);
	ro_object_handlers.read_property 	= std_object_handlers.read_property;
	ro_object_handlers.write_property 	= std_object_handlers.write_property;
	ro_object_handlers.clone_obj 		= httprouter_clone_obj;
	ro_object_handlers.free_obj 		= ro_object_free_storage;

	zend_declare_class_constant_string(roo_class_entry, "VERSION", sizeof("VERSION") - 1, PHP_HTTPROUTER_VERSION);

	zend_declare_property_string(roo_class_entry, 	"uri", 			sizeof("uri") - 1, 		"", ZEND_ACC_PRIVATE);
	zend_declare_property_string(roo_class_entry, 	"method", 		sizeof("method") - 1,	"", ZEND_ACC_PRIVATE);
	zend_declare_property_string(roo_class_entry, 	"prefix", 		sizeof("prefix") - 1, 	"", ZEND_ACC_PRIVATE);
	zend_declare_property_null(roo_class_entry, 	"action", 		sizeof("action") - 1, 		ZEND_ACC_PROTECTED);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(httprouter)
{
	roo_class_entry = NULL;

	return SUCCESS;
}

PHP_MINFO_FUNCTION(httprouter)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "HTTPRouter support", "enabled");
	php_info_print_table_row(2, "version", PHP_HTTPROUTER_VERSION);
	php_info_print_table_end();
}

zend_module_entry httprouter_module_entry = {
	STANDARD_MODULE_HEADER,
	"HTTPRouter",					/* Extension name */
	httprouter_functions,			/* zend_function_entry */
	PHP_MINIT(httprouter),			/* PHP_MINIT - Module initialization */
	PHP_MSHUTDOWN(httprouter),		/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(httprouter),			/* PHP_RINIT - Request initialization */
	NULL,							/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(httprouter),			/* PHP_MINFO - Module info */
	PHP_HTTPROUTER_VERSION,			/* Version */
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_HTTPROUTER
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(httprouter)
#endif