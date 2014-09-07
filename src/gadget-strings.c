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

	gchar *gadget_name;
};

struct _GadgetStringsClass
{
	GadgetdGadgetStringsSkeletonClass parent_class;
};

enum
{
	PROPERTY,
	MANUFACTURER,
	PRODUCT,
	SERIALNUMBER,
	PROPERTY_GADGET_NAME
};

static void gadget_strings_iface_init(GadgetdGadgetStringsIface *iface);

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetStrings, gadget_strings, GADGETD_TYPE_GADGET_STRINGS_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_STRINGS, gadget_strings_iface_init));

/**
 * @brief gadget strings finalize
 * @param[in] object GObject
 */
static void
gadget_strings_finalize(GObject *object)
{
	GadgetStrings *gadget_strings = GADGET_STRINGS(object);

	g_free(gadget_strings->gadget_name);

	if (G_OBJECT_CLASS(gadget_strings_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadget_strings_parent_class)->finalize(object);
}

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
	usbg_gadget *g = NULL;
	GadgetStrings *strings = GADGET_STRINGS(object);
	gchar *str = NULL;
	gint usbg_ret = USBG_SUCCESS;

	if (strings->gadget_name != NULL)
		g = usbg_get_gadget(ctx.state, strings->gadget_name);

	if (g == NULL && property_id != PROPERTY_GADGET_NAME) {
		ERROR("Cant get a gadget device by name");
		return;
	}

	str = g_value_dup_string(value);
	switch(property_id) {
	case PRODUCT:
		usbg_ret = usbg_set_gadget_product(g, LANG_US_ENG, str);
		break;
	case SERIALNUMBER:
		usbg_ret = usbg_set_gadget_serial_number(g, LANG_US_ENG, str);
		break;
	case MANUFACTURER:
		usbg_ret = usbg_set_gadget_manufacturer(g, LANG_US_ENG, str);
		break;
	case PROPERTY_GADGET_NAME:
		g_assert(strings->gadget_name == NULL);
		strings->gadget_name = str;
		str = NULL;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
	g_free(str);
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
	usbg_gadget *g = NULL;
	usbg_gadget_strs g_strs;
	GadgetStrings *strings = GADGET_STRINGS(object);

	if (strings->gadget_name == NULL)
		return;

	g = usbg_get_gadget(ctx.state, strings->gadget_name);
	if (g == NULL) {
		ERROR("Cant get a gadget device by name");
		return;
	}

	/* actually strings available only in en_US lang */
	/* actualize data for gadget */
	usbg_ret = usbg_get_gadget_strs(g, LANG_US_ENG, &g_strs);
	if (usbg_ret) {
		ERROR("Cant get the USB gadget strings");
		return;
	}

	switch(property_id) {
	case PRODUCT:
		g_value_set_string(value, g_strs.str_prd);
		break;
	case MANUFACTURER:
		g_value_set_string(value, g_strs.str_mnf);
		break;
	case SERIALNUMBER:
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
gadget_strings_new(const gchar *gadget_name)
{
	g_return_val_if_fail(gadget_name != NULL, NULL);
	GadgetStrings *object;

	object = g_object_new(GADGET_TYPE_STRINGS,
			     "gadget_name", gadget_name,
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
	gobject_class->finalize = gadget_strings_finalize;

	g_object_class_override_property(gobject_class,
					  MANUFACTURER,
					 "manufacturer");

	g_object_class_override_property(gobject_class,
					  PRODUCT,
					 "product");

	g_object_class_override_property(gobject_class,
					  SERIALNUMBER,
					 "serialnumber");

	g_object_class_install_property(gobject_class,
                                   PROPERTY_GADGET_NAME,
                                   g_param_spec_string("gadget_name",
                                                       "Gadget name",
                                                       "gadget name",
                                                       NULL,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

/**
 * @brief gadget strings iface init
 * @param[in] daemon GadgetStrings struct
 */
static void
gadget_strings_iface_init(GadgetdGadgetStringsIface *iface)
{
	/* noop */
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

