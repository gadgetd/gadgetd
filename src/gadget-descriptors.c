/*
 * gadgetd-descriptors.c
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

#include <gio/gio.h>
#include <glib/gprintf.h>

#include <gadgetd-config.h>
#include <gadgetd-create.h>
#include <gadgetd-common.h>

#include <gadgetd-gdbus-codegen.h>
#include <gadget-descriptors.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

struct _GadgetDescriptors
{
	GadgetdGadgetDescriptorsSkeleton parent_instance;

	struct gd_gadget *gadget;
};

struct _GadgetDescriptorsClass
{
	GadgetdGadgetDescriptorsSkeletonClass parent_class;
};

enum
{
	PROP_0,
	/* We add one to each value because usbg enumerates attributes from 0 */
	PROP_DESC_BCDUSB = BCD_USB + 1,
	PROP_DESC_BDEVICECLASS = B_DEVICE_CLASS + 1,
	PROP_DESC_BDEVICESUBCLASS = B_DEVICE_SUB_CLASS + 1,
	PROP_DESC_BDEVICEPROTOCOL = B_DEVICE_PROTOCOL + 1,
	PROP_DESC_BMAXPACKETSIZE = B_MAX_PACKET_SIZE_0 + 1,
	PROP_DESC_IDVENDOR = ID_VENDOR + 1,
	PROP_DESC_IDPRODUCT = ID_PRODUCT + 1,
	PROP_DESC_BCDDEVICE = BCD_DEVICE + 1,
	PROP_GADGET_PTR
} prop_descriptors;

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetDescriptors, gadget_descriptors, GADGETD_TYPE_GADGET_DESCRIPTORS_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_DESCRIPTORS, NULL));

/**
 * @brief gadget descriptors set property func
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
gadget_descriptors_set_property(GObject      *object,
				 guint         property_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	GadgetDescriptors *descriptors = GADGET_DESCRIPTORS(object);
	struct gd_gadget *gadget = descriptors->gadget;
	gint usbg_ret = USBG_SUCCESS;
	gint val;

	if (gadget == NULL && property_id != PROP_GADGET_PTR) {
		ERROR("Cant get a gadget object");
		return;
	}

	switch (property_id) {
	case PROP_DESC_BCDUSB:
	case PROP_DESC_IDVENDOR:
	case PROP_DESC_IDPRODUCT:
	case PROP_DESC_BCDDEVICE:
		val = g_value_get_uint(value);
		break;

	case PROP_DESC_BDEVICECLASS:
	case PROP_DESC_BDEVICESUBCLASS:
	case PROP_DESC_BDEVICEPROTOCOL:
	case PROP_DESC_BMAXPACKETSIZE:
		val = g_value_get_uchar(value);
		break;
	case PROP_GADGET_PTR:
		g_assert (descriptors->gadget == NULL);
		descriptors->gadget = g_value_get_pointer(value);
		goto out;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		goto out;

	}

	usbg_ret = usbg_set_gadget_attr(gadget->g, property_id - 1, val);
	if (usbg_ret != USBG_SUCCESS) {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		ERROR("Error: %s: %s", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
	}

out:
	return;
}

/**
 * @brief gadget descriptors new
 * @param[in] connection A #GDBusConnection
 * @return #GadgetDescriptors object.
 */
GadgetDescriptors *
gadget_descriptors_new(struct gd_gadget *gadget)
{
	g_return_val_if_fail(gadget != NULL, NULL);
	GadgetDescriptors *object;

	object = g_object_new(GADGET_TYPE_DESCRIPTORS,
			     "gd_gadget", gadget,
			      NULL);
	return object;
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
gadget_descriptors_get_property(GObject    *object,
				 guint       property_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	GadgetDescriptors *descriptors = GADGET_DESCRIPTORS(object);
	struct gd_gadget *gadget = descriptors->gadget;
	gint val;

	if (gadget == NULL) {
		ERROR("Cant get a gadget object");
		return;
	}

	switch (property_id) {
	case PROP_DESC_BCDUSB:
	case PROP_DESC_IDVENDOR:
	case PROP_DESC_IDPRODUCT:
	case PROP_DESC_BCDDEVICE:
		val = usbg_get_gadget_attr(gadget->g, property_id - 1);
		if (val < 0)
			goto error;
		g_value_set_uint(value, (uint)val);
		break;

	case PROP_DESC_BDEVICECLASS:
	case PROP_DESC_BDEVICESUBCLASS:
	case PROP_DESC_BDEVICEPROTOCOL:
	case PROP_DESC_BMAXPACKETSIZE:
		val = usbg_get_gadget_attr(gadget->g, property_id - 1);
		if (val < 0)
			goto error;
		g_value_set_uchar(value, (char)val);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		return;

	}

	return;
error:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	ERROR("Cant get the USB gadget attribute\n");
	return;
}

/**
 * @brief gadget daemon class init
 * @param[in] klass GadgetDescriptorsClass
 */
static void
gadget_descriptors_class_init(GadgetDescriptorsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);;
	int i;
	struct {
		int id;
		const char *name;
	} properties[] = {
		{ PROP_DESC_BCDUSB, "bcd-usb" },
		{ PROP_DESC_BDEVICECLASS, "b-device-class" },
		{ PROP_DESC_BDEVICESUBCLASS, "b-device-sub-class" },
		{ PROP_DESC_BDEVICEPROTOCOL, "b-device-protocol" },
		{ PROP_DESC_BMAXPACKETSIZE, "b-max-packet-size0" },
		{ PROP_DESC_IDVENDOR, "id-vendor" },
		{ PROP_DESC_IDPRODUCT, "id-product" },
		{ PROP_DESC_BCDDEVICE, "bcd-device" }
	};

	gobject_class->set_property = gadget_descriptors_set_property;
	gobject_class->get_property = gadget_descriptors_get_property;

	for (i = 0; i < ARRAY_SIZE(properties); ++i)
		g_object_class_override_property(gobject_class,
						 properties[i].id,
						 properties[i].name);

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
 * @brief descriptors init
 */
static void
gadget_descriptors_init(GadgetDescriptors *descriptors)
{
	/* noop */
}

