/*
 * gadgetd-function-iface.c
 * Copyright(c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0(the "License");
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

#include <usbg/usbg.h>
#include <stdio.h>
#include <gio/gio.h>
#include <gadgetd-common.h>

#include <gadgetd-gdbus-codegen.h>
#include <dbus-function-ifaces/gadgetd-function-iface.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

struct _FunctionAttrs
{
	GadgetdFunctionAttrsSkeleton parent_instance;

	struct gd_function *func;
};

struct _FunctionAttrsClass
{
	GadgetdFunctionAttrsSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_FUNC_INSTANCE,
	PROP_FUNC_TYPE,
	PROP_FUNC_PTR,
} prop_func_attrs;

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(FunctionAttrs, function_attrs, GADGETD_TYPE_FUNCTION_ATTRS_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_FUNCTION_ATTRS, NULL));

/**
 * @brief function attrs set property function
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
function_attrs_set_property(GObject      *object,
				 guint         property_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	FunctionAttrs *func_attrs = FUNCTION_ATTRS(object);

	switch(property_id) {
	case PROP_FUNC_PTR:
		g_assert(func_attrs->func == NULL);
		func_attrs->func = g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief function attrs get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectGetPropertyFunc()
 */
static void
function_attrs_get_property(GObject    *object,
				 guint       property_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	FunctionAttrs *func_attrs = FUNCTION_ATTRS(object);
	_cleanup_g_free_ gchar *str = NULL;

	switch(property_id) {
	case PROP_FUNC_INSTANCE:
		g_value_set_string(value, func_attrs->func->instance);
		break;
	case PROP_FUNC_TYPE:
		g_value_set_string(value, func_attrs->func->type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief function attrs class init
 * @param[in] klass FunctionAttrsClass
 */
static void
function_attrs_class_init(FunctionAttrsClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = function_attrs_set_property;
	gobject_class->get_property = function_attrs_get_property;

	g_object_class_override_property(gobject_class,
					PROP_FUNC_INSTANCE,
					"instance");

	g_object_class_override_property(gobject_class,
					PROP_FUNC_TYPE,
	/* to avoid generating wrong property name like type-
	property name was changed to type_name, this bug
	(https://bugzilla.gnome.org/show_bug.cgi?id=679473) was
	solved in libglib 2.33.4 so property name in the future
	will be changed to proper name - type */
					"type-name");
	g_object_class_install_property(gobject_class,
                                   PROP_FUNC_PTR,
                                   g_param_spec_pointer("gd_function",
                                                       "Function ptr",
                                                       "function ptr",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

/**
 * @brief function attrs new
 * @param[in] connection A #GDBusConnection
 * @return #FunctionAttrs object.
 */
FunctionAttrs *
function_attrs_new(struct gd_function *func)
{
	g_return_val_if_fail(func != NULL, NULL);
	FunctionAttrs *object;

	object = g_object_new(FUNCTION_TYPE_ATTRS,
			     "gd_function", func,
			      NULL);
	return object;
}

/**
 * @brief function attrs init
 */
static void
function_attrs_init(FunctionAttrs *func_attrs)
{
	/* noop */
}

