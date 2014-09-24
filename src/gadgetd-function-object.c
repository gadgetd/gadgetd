/*
 * gadgetd-function-object.c
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

#include <stdio.h>
#include <unistd.h>
#include <glib-object.h>
#include <usbg/usbg.h>

#include <gio/gio.h>
#include <glib/gprintf.h>

#include <gadgetd-function-object.h>
#include <gadgetd-gdbus-codegen.h>
#include <gadgetd-common.h>
#include <gadget-function-manager.h>
#include <dbus-function-ifaces/gadgetd-serial-function-iface.h>

typedef struct _GadgetdFunctionObjectClass   GadgetdFunctionObjectClass;

struct _GadgetdFunctionObject
{
	GadgetdObjectSkeleton parent_instance;

	struct gd_function *func;
	gchar *function_path;

	FunctionSerialAttrs *f_serial_attrs_iface;
};

struct _GadgetdFunctionObjectClass
{
	GadgetdObjectSkeletonClass parent_class;
};


G_DEFINE_TYPE(GadgetdFunctionObject, gadgetd_function_object, GADGETD_TYPE_OBJECT_SKELETON);

enum
{
	PROP_0,
	PROP_FUNC_PTR,
	PROP_FUNC_PATH
} prop_func_obj;

/**
 * @brief object finalize
 * @param[in] object GObject
 */
static void
gadgetd_function_object_finalize(GObject *object)
{
	GadgetdFunctionObject *function_object = GADGETD_FUNCTION_OBJECT(object);

	g_free(function_object->function_path);

	if (function_object->f_serial_attrs_iface != NULL)
		g_object_unref(function_object->f_serial_attrs_iface);

	if (G_OBJECT_CLASS(gadgetd_function_object_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadgetd_function_object_parent_class)->finalize(object);
}

/**
 * @brief Get function
 * @param[in] function_object GadgetdFunctionObject
 * @return str_type
 */
struct gd_function *
gadgetd_function_object_get_function(GadgetdFunctionObject *function_object)
{
	g_return_val_if_fail(GADGETD_IS_FUNCTION_OBJECT(function_object), NULL);
	return function_object->func;
}

/**
 * @brief gadgetd function object get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 */
static void
gadgetd_function_object_get_property(GObject          *object,
                                    guint             property_id,
                                    GValue           *value,
                                    GParamSpec       *pspec)
{
	switch(property_id) {
	case PROP_0:
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd function object Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
void
gadgetd_function_object_set_property(GObject            *object,
				    guint              property_id,
				    const GValue      *value,
				    GParamSpec        *pspec)
{
	GadgetdFunctionObject *function_object = GADGETD_FUNCTION_OBJECT(object);

	switch(property_id) {
	case PROP_FUNC_PTR:
		g_assert(function_object->func == NULL);
		function_object->func = g_value_get_pointer(value);
		break;
	case PROP_FUNC_PATH:
		g_assert(function_object->function_path == NULL);
		function_object->function_path = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd function object init
 * @param[in] object GadgetdFunctionObject
 */
static void
gadgetd_function_object_init(GadgetdFunctionObject *object)
{
	/*nop*/
}

/**
 * @brief add interface
 * @param[in] type usbg_function_type
 * @param[in] function_object GadgetdFunctionObject
 */
static void
gadgetd_function_object_add_interface(usbg_function_type type, GadgetdFunctionObject *function_object)
{
	switch (type) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		function_object->f_serial_attrs_iface = function_serial_attrs_new(function_object);

		get_iface(G_OBJECT(function_object),FUNCTION_TYPE_SERIAL_ATTRS,
			  &function_object->f_serial_attrs_iface);
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		ERROR("Function interface not implemented");
		break;
	case F_PHONET:
		ERROR("Function interface not implemented");
		break;
	case F_FFS:
		ERROR("Function interface not implemented");
		break;
	default:
		ERROR("Unsupported function type\n");
	}
}

/**
 * @brief gadgetd gadget object constructed
 * @param[in] object GObject
 */
static void
gadgetd_function_object_constructed(GObject *object)
{
	GadgetdFunctionObject *function_object = GADGETD_FUNCTION_OBJECT(object);
	gchar *path = function_object->function_path;
	gint type;

	type = usbg_get_function_type(function_object->func->f);
	if (type < 0) {
		ERROR("Invalid function type");
		return;
	}

	gadgetd_function_object_add_interface(type, function_object);

	if (path != NULL && g_variant_is_object_path(path) && function_object != NULL)
		g_dbus_object_skeleton_set_object_path(G_DBUS_OBJECT_SKELETON(function_object), path);
	else
		ERROR("Unexpected error during gadget object creation");

	if (G_OBJECT_CLASS(gadgetd_function_object_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(gadgetd_function_object_parent_class)->constructed(object);
}

/**
 * @brief gadgetd function object class init
 * @param[in] klass GadgetdFunctionObjectClass
 */
static void
gadgetd_function_object_class_init(GadgetdFunctionObjectClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gadgetd_function_object_finalize;
	gobject_class->constructed  = gadgetd_function_object_constructed;
	gobject_class->set_property = gadgetd_function_object_set_property;
	gobject_class->get_property = gadgetd_function_object_get_property;

	g_object_class_install_property(gobject_class,
                                   PROP_FUNC_PTR,
                                   g_param_spec_pointer("gd_function",
                                                       "Function ptr",
                                                       "function ptr",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(gobject_class,
				   PROP_FUNC_PATH,
                                   g_param_spec_string("function_path",
                                                       "Function path",
                                                       "function path",
                                                       NULL,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

/**
 * @brief gadgetd function object new
 * @param[in] gadget_name gchar gadget name
 * @return GadgetdFunctionObject object
 */
GadgetdFunctionObject *
gadgetd_function_object_new(const gchar *function_path,
			    struct gd_function *func)
{
	g_return_val_if_fail(function_path != NULL, NULL);
	g_return_val_if_fail(func          != NULL, NULL);

	GadgetdFunctionObject *object;
	object = g_object_new(GADGETD_TYPE_FUNCTION_OBJECT,
			     "function_path",      function_path,
			     "gd_function", func,
			      NULL);

	return object;
}
