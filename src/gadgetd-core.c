/*
 * gadgetd-core.c
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gadgetd-core.h"
#include "gadgetd-core-func.h"

#include <string.h>

static GList *func_types = NULL;

/* Register unregister function type */

static struct gd_function_type *
gd_lookup_function_type(const gchar *type_name)
{
	GList *l;
	struct gd_function_type *t;

	for (l = g_list_first(func_types); l; l = g_list_next(l)) {
		t = l->data;
		if (!g_strcmp0(type_name, t->name))
			return t;
	}

	return NULL;
}

int
gd_register_func_t(struct gd_function_type *type)
{
	struct gd_function_type *t;

	/* Create and rm instace are mandatory */
	if (!type->create_instance || !type->rm_instance)
		return GD_ERROR_INVALID_PARAM;

	t = gd_lookup_function_type(type->name);
	if (t)
		return GD_ERROR_EXIST;

	func_types = g_list_append(func_types, type);

	return GD_SUCCESS;
}

int
gd_unregister_func_t(struct gd_function_type *type)
{
	GList *l;
	struct gd_function_type *t;

	l = g_list_find(func_types, type);
	if (!l)
		return GD_ERROR_NOT_FOUND;

	func_types = g_list_remove_link(func_types, l);
	t = l->data;
	if (t->on_unregister)
		t->on_unregister(t);

	g_list_free(l);
	return GD_SUCCESS;
}

void
gd_unregister_all_func_t(void)
{
	GList *l;
	struct gd_function_type *t;

	while ((l = g_list_first(func_types)) != NULL) {
		t = (struct gd_function_type *)l->data;
		if (t->on_unregister)
			t->on_unregister(t);

		func_types = g_list_delete_link(func_types, l);
	}
}

static inline void
gd_gcharpp_cleanup(void *p)
{
	g_free(*(gchar**)p);
}

GArray *
gd_list_func_types(gboolean zero_terminated)
{
	GArray *buf;
	int n_elem;
	GList *l;
	struct gd_function_type *t;
	gchar *name;

	n_elem = g_list_length(func_types);
	buf = g_array_sized_new(zero_terminated, TRUE, sizeof(gchar *), n_elem);
	if (!buf)
		goto out;

	g_array_set_clear_func(buf, gd_gcharpp_cleanup);

	for (l = g_list_first(func_types); l; l = g_list_next(l)) {
		t = (struct gd_function_type *)l->data;
		name = g_strdup(t->name);
		if (!name)
			goto error;
		g_array_append_val(buf, name);
	}

out:
	return buf;
error:
	g_array_free(buf, TRUE);
	return NULL;
}

/* Internal gadgetd API for gadget/config/function management */

static int
gd_set_gadget_strs(struct gd_gadget *g, GVariant *strings, int lang,
		   const char **error)
{
	GVariant *value;
	GVariantIter *iter;
	int ret = GD_SUCCESS;
	gchar *key;
	const gchar *s;
	gint i;
	int usbg_ret;

	static const struct {
		char *name;
		int (*s_func)(usbg_gadget *, int, const char *str);
	} strs[] = {
		{ "serialnumber", usbg_set_gadget_serial_number       },
		{ "manufacturer", usbg_set_gadget_manufacturer        },
		{ "product",      usbg_set_gadget_product             }
	};

	g_variant_get(strings, "a{sv}", &iter);

	if (g_variant_iter_n_children(iter) == 0) {
		/* If we were called with empty dictionary
		 * We have to create string for given lang.
		 *
		 * This is information for driver that this gadget
		 * provides strings in selected language
		 * and all three are empty.
		 */
		usbg_ret = strs[0].s_func(g->g, lang, "");
		if (usbg_ret != USBG_SUCCESS) {
			*error = usbg_error_name(usbg_ret);
			ret = GD_ERROR_OTHER_ERROR;
		}
	}

	while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
		/* Lookup suitable string */
		for (i = 0; i < G_N_ELEMENTS(strs); i++)
			if (!g_strcmp0(key, strs[i].name))
				break;

		/* if not found return error */
		if (i == G_N_ELEMENTS(strs)) {
			*error = "Unknown gadget string";
			ret = GD_ERROR_INVALID_PARAM;
			goto error;
		}

		if (!g_variant_is_of_type(value, G_VARIANT_TYPE("s"))) {
			*error = "Bad type of value";
			ret = GD_ERROR_INVALID_PARAM;
			goto error;
		}

		s = g_variant_get_string(value, NULL);
		usbg_ret = strs[i].s_func(g->g, lang, s);
		if (usbg_ret != USBG_SUCCESS) {
			ERROR("Unable to set string '%s': %s", key,
			      usbg_error_name(usbg_ret));
			*error = usbg_error_name(usbg_ret);
			ret = GD_ERROR_OTHER_ERROR;
			goto error;
		}
	}

	g_variant_iter_free(iter);
	return ret;
error:
	g_variant_unref(value);
	g_variant_iter_free(iter);
	return ret;

}


static int
gd_set_gadget_attrs(struct gd_gadget *g, GVariant *attrs, const char **error)
{
	GVariant *value;
	GVariantIter *iter;
	const GVariantType *type;
	int attr_code;
	int ret = GD_SUCCESS;
	int val;
	gchar *key;
	int usbg_ret;

	g_variant_get(attrs, "a{sv}", &iter);
	while(g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
		attr_code = usbg_lookup_gadget_attr(key);
		if (attr_code < 0) {
			*error = "Unknown attribute";
			ret = GD_ERROR_INVALID_PARAM;
			goto error;
		}

		type = g_variant_get_type(value);
		/* We don't check type (between q and y) because library
		 * does it for us.
		 */
		if (g_variant_type_equal(type, G_VARIANT_TYPE("q"))) {
			val = g_variant_get_uint16(value);
		} else if (g_variant_type_equal(type, G_VARIANT_TYPE("y"))) {
			val = g_variant_get_byte(value);
		} else {
			*error = "Bad type of value";
			ret = GD_ERROR_INVALID_PARAM;
			goto error;
		}

		usbg_ret = usbg_set_gadget_attr(g->g, attr_code, val);
		if (usbg_ret != USBG_SUCCESS) {
			ERROR("Unable to set descriptor '%s': %s", key,
			      usbg_error_name(usbg_ret));
			*error = usbg_error_name(usbg_ret);
			ret = GD_ERROR_OTHER_ERROR;
			goto error;
		}
	}

	g_variant_iter_free(iter);
	return ret;
error:
	g_variant_unref(value);
	g_variant_iter_free(iter);
	return ret;
}

int
gd_create_gadget(const gchar *name, GVariant *attrs, GVariant *strings,
		 struct gd_gadget *g, const gchar **error)
{
	int usbg_ret;
	int ret = GD_SUCCESS;

	memset(g, 0, sizeof(*g));

	usbg_ret = usbg_create_gadget(ctx.state, name, NULL, NULL, &(g->g));
	if (usbg_ret != USBG_SUCCESS) {
		*error = usbg_error_name(usbg_ret);
		ret = GD_ERROR_OTHER_ERROR;
		goto out;
	}

	ret = gd_set_gadget_attrs(g, attrs, error);
	if (ret != GD_SUCCESS)
		goto rm_gadget;

	/* Currently we use only strings in English (US)
	 * This function will create all strings empty for
	 * this language if empty directory has been provided
	 */
	ret = gd_set_gadget_strs(g, strings, LANG_US_ENG, error);

rm_gadget:
	if (ret != GD_SUCCESS)
		usbg_rm_gadget(g->g, USBG_RM_RECURSE);
out:
	return ret;
}

int
gd_create_function(struct gd_gadget *gadget, const gchar *type_name,
		   const gchar *instance, struct gd_function **f,
		   const gchar **error)
{
	struct gd_function_type *type;
	int usbg_ret;
	int ret;
	struct gd_function *func;

	type = gd_lookup_function_type(type_name);
	if (!type) {
		*error = "Type not found";
		ret = GD_ERROR_NOT_FOUND;
		goto out;
	}

	usbg_ret = type->create_instance(gadget, type, instance, &func);
	if (usbg_ret != USBG_SUCCESS) {
		*error = usbg_error_name(usbg_ret);
		ret = GD_ERROR_OTHER_ERROR;
		goto out;
	}

	*f = func;
	ret = GD_SUCCESS;
out:
	return ret;
}

