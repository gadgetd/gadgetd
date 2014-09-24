/*
 * gadgetd-daemon.c
 * Copyright(c) 2012-2014 Samsung Electronics Co., Ltd.
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

#include <gadgetd-config.h>
#include <gadgetd-create.h>
#include <gadgetd-common.h>
#include <gadget-daemon.h>

#include <gadgetd-gdbus-codegen.h>
#include <gadget-strings.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

struct _GadgetStrings
{
	GadgetdGadgetStringsSkeleton parent_instance;

	struct gd_gadget *gadget;
};

struct _GadgetStringsClass
{
	GadgetdGadgetStringsSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_STR_MANUFACTURER,
	PROP_STR_PRODUCT,
	PROP_STR_SERIAL_NUMBER,
	PROP_GADGET_PTR
};

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetStrings, gadget_strings, GADGETD_TYPE_GADGET_STRINGS_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_STRINGS, NULL));

/**
 * @brief gadget strings set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
gadget_strings_set_property(GObject        *object,
			     guint           property_id,
			     const GValue   *value,
			     GParamSpec     *pspec)
{
	/* TODO add lang support */
	GadgetStrings *strings = GADGET_STRINGS(object);
	struct gd_gadget *gadget = strings->gadget;
	const gchar *str = NULL;
	gint usbg_ret = USBG_SUCCESS;

	if (gadget == NULL && property_id != PROP_GADGET_PTR) {
		ERROR("Cant get a gadget device");
		return;
	}

	if (property_id != PROP_GADGET_PTR)
		str = g_value_get_string(value);

	switch(property_id) {
	case PROP_STR_PRODUCT:
		usbg_ret = usbg_set_gadget_product(gadget->g, LANG_US_ENG, str);
		break;
	case PROP_STR_SERIAL_NUMBER:
		usbg_ret = usbg_set_gadget_serial_number(gadget->g, LANG_US_ENG, str);
		break;
	case PROP_STR_MANUFACTURER:
		usbg_ret = usbg_set_gadget_manufacturer(gadget->g, LANG_US_ENG, str);
		break;
	case PROP_GADGET_PTR:
		g_assert(strings->gadget == NULL);
		strings->gadget = g_value_get_pointer(value);
		str = NULL;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	if (usbg_ret != USBG_SUCCESS) {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		ERROR("Error: %s: %s", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
	}
}

/**
 * @brief get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectGetPropertyFunc()
 */
static void
gadget_strings_get_property(GObject     *object,
			     guint        property_id,
			     GValue      *value,
			     GParamSpec  *pspec)
{
	gint usbg_ret;
	usbg_gadget_strs g_strs;
	GadgetStrings *strings = GADGET_STRINGS(object);
	struct gd_gadget *gadget = strings->gadget;

	if (gadget == NULL)
		return;

	/* actually strings available only in en_US lang */
	/* actualize data for gadget */
	usbg_ret = usbg_get_gadget_strs(gadget->g, LANG_US_ENG, &g_strs);
	if (usbg_ret != USBG_SUCCESS) {
		ERROR("Cant get the USB gadget strings");
		return;
	}

	switch(property_id) {
	case PROP_STR_PRODUCT:
		g_value_set_string(value, g_strs.str_prd);
		break;
	case PROP_STR_MANUFACTURER:
		g_value_set_string(value, g_strs.str_mnf);
		break;
	case PROP_STR_SERIAL_NUMBER:
		g_value_set_string(value, g_strs.str_ser);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget strings new
 * @param[in] gadget_name string gadget name
 * @return #GadgetStrings object.
 */
GadgetStrings *
gadget_strings_new(struct gd_gadget *gadget)
{
	g_return_val_if_fail(gadget != NULL, NULL);
	GadgetStrings *object;

	object = g_object_new(GADGET_TYPE_STRINGS,
			     "gd_gadget", gadget,
			      NULL);

	return object;
}

/**
 * @brief gadget daemon class init
 * @param[in] klass GadgetStringsClass
 */
static void
gadget_strings_class_init(GadgetStringsClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gadget_strings_set_property;
	gobject_class->get_property = gadget_strings_get_property;

	g_object_class_override_property(gobject_class,
					  PROP_STR_MANUFACTURER,
					 "manufacturer");

	g_object_class_override_property(gobject_class,
					  PROP_STR_PRODUCT,
					 "product");

	g_object_class_override_property(gobject_class,
					  PROP_STR_SERIAL_NUMBER,
					 "serialnumber");

	g_object_class_install_property(gobject_class,
                                   PROP_GADGET_PTR,
                                   g_param_spec_pointer("gd_gadget",
                                                       "Gadget ptr",
                                                       "gadget ptr",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

/**
 * @brief daemon init
 * @param[in] daemon GadgetStrings struct
 */
static void
gadget_strings_init(GadgetStrings *strings)
{
	/* noop */
}

