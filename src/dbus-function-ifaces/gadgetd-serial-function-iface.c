/*
 * gadgetd-serial-function-iface.c
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
#include <usbg/function/serial.h>
#include <stdio.h>
#include <gio/gio.h>
#include <gadgetd-common.h>

#include <gadgetd-gdbus-codegen.h>
#include <dbus-function-ifaces/gadgetd-serial-function-iface.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

struct _FunctionSerialAttrs
{
	GadgetdFunctionSerialAttrsSkeleton parent_instance;

	GadgetdFunctionObject *function_object;
};

struct _FunctionSerialAttrsClass
{
	GadgetdFunctionSerialAttrsSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_SERIAL_PORTNUM,
	PROP_SERIAL_FUNC_OBJECT,
} prop_serial_attrs;

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(FunctionSerialAttrs, function_serial_attrs, GADGETD_TYPE_FUNCTION_SERIAL_ATTRS_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_FUNCTION_SERIAL_ATTRS, NULL));

/**
 * @brief function serial attrs set property function
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
function_serial_attrs_set_property(GObject      *object,
				 guint         property_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	FunctionSerialAttrs *serial_attrs = FUNCTION_SERIAL_ATTRS(object);

	switch(property_id) {
	case PROP_SERIAL_FUNC_OBJECT:
		g_assert(serial_attrs->function_object == NULL);
		serial_attrs->function_object = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static GadgetdFunctionObject *
function_serial_attrs_get_function_object(FunctionSerialAttrs *serial_attrs)
{
	g_return_val_if_fail(FUNCTION_IS_SERIAL_ATTRS(serial_attrs), NULL);
	return serial_attrs->function_object;
}

/**
 * @brief function serial attrs get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectGetPropertyFunc()
 */
static void
function_serial_attrs_get_property(GObject    *object,
				 guint       property_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	gint usbg_ret = USBG_SUCCESS;
	FunctionSerialAttrs *serial_attrs = FUNCTION_SERIAL_ATTRS(object);
	GadgetdFunctionObject *function_object;
	usbg_function *f;
	usbg_f_serial *f_serial;
	int port_num;

	function_object = function_serial_attrs_get_function_object(serial_attrs);

	f = gadgetd_function_object_get_function(function_object)->f;

	if (f == NULL) {
		ERROR("Cant get function by name");
		return;
	}

	f_serial = usbg_to_serial_function(f);

	switch(property_id) {
	case PROP_SERIAL_PORTNUM:
		usbg_ret = usbg_f_serial_get_port_num(f_serial, &port_num);
		if (usbg_ret != USBG_SUCCESS) {
			ERROR("Cant get serial port number");
			return;
		}
		g_value_set_int(value, port_num);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief function serial attrs finalize
 * @param[in] object GObject
 */
static void
function_serial_attrs_finalize(GObject *object)
{
	if (G_OBJECT_CLASS(function_serial_attrs_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(function_serial_attrs_parent_class)->finalize(object);
}

/**
 * @brief function serial attrs class init
 * @param[in] klass FunctionSerialAttrsClass
 */
static void
function_serial_attrs_class_init(FunctionSerialAttrsClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = function_serial_attrs_set_property;
	gobject_class->get_property = function_serial_attrs_get_property;
	gobject_class->finalize = function_serial_attrs_finalize;

	g_object_class_override_property(gobject_class,
					PROP_SERIAL_PORTNUM,
					"port-num");

	g_object_class_install_property(gobject_class,
                                   PROP_SERIAL_FUNC_OBJECT,
                                   g_param_spec_object("function-object",
                                                        "function-object",
                                                        "function object",
                                                        GADGETD_TYPE_FUNCTION_OBJECT,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}

/**
 * @brief function serial attrs new
 * @param[in] connection A #GDBusConnection
 * @return #FunctionSerialAttrs object.
 */
FunctionSerialAttrs *
function_serial_attrs_new(GadgetdFunctionObject *function_object)
{
	g_return_val_if_fail(function_object != NULL, NULL);
	FunctionSerialAttrs *object;

	object = g_object_new(FUNCTION_TYPE_SERIAL_ATTRS,
			     "function-object", function_object,
			      NULL);
	return object;
}

/**
 * @brief function serial attrs init
 */
static void
function_serial_attrs_init(FunctionSerialAttrs *descriptors)
{
	/* noop */
}

