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

	gchar *gadget_name;
};

struct _GadgetDescriptorsClass
{
	GadgetdGadgetDescriptorsSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_DESC_BCDUSB,
	PROP_DESC_BDEVICECLASS,
	PROP_DESC_BDEVICESUBCLASS,
	PROP_DESC_BDEVICEPROTOCOL,
	PROP_DESC_BMAXPACKETSIZE,
	PROP_DESC_IDVENDOR,
	PROP_DESC_IDPRODUCT,
	PROP_DESC_BCDDEVICE,
	PROP_GADGET_NAME
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
	guint16 tuint16;
	guint8 tuint8;
	usbg_gadget *g = NULL;
	gint usbg_ret = USBG_SUCCESS;

	if (descriptors->gadget_name != NULL)
		g = usbg_get_gadget(ctx.state, descriptors->gadget_name);

	if (g == NULL && property_id != PROP_GADGET_NAME) {
		ERROR("Cant get a gadget device by name");
		return;
	}

	switch(property_id) {
	case PROP_DESC_BCDUSB:
		/* We are not checking range of received *value*
		   because dbus only allow to send/recive type 'q' value*/
		tuint16 = g_value_get_uint(value);
		usbg_ret = usbg_set_gadget_device_bcd_usb(g, (uint16_t)tuint16);
		break;
	case PROP_DESC_BDEVICECLASS:
		tuint8 = g_value_get_uchar(value);
		usbg_ret = usbg_set_gadget_device_class(g, (uint8_t)tuint8);
		break;
	case PROP_DESC_BDEVICESUBCLASS:
		tuint8 = g_value_get_uchar(value);
		usbg_ret = usbg_set_gadget_device_subclass(g, (uint8_t)tuint8);
		break;
	case PROP_DESC_BDEVICEPROTOCOL:
		tuint8 = g_value_get_uchar(value);
		usbg_ret = usbg_set_gadget_device_protocol(g, (uint8_t)tuint8);
		break;
	case PROP_DESC_BMAXPACKETSIZE:
		tuint8 = g_value_get_uchar(value);
		usbg_ret = usbg_set_gadget_device_max_packet(g, (uint8_t)tuint8);
		break;
	case PROP_DESC_IDVENDOR:
		tuint16 = g_value_get_uchar(value);
		usbg_ret = usbg_set_gadget_vendor_id(g, (uint16_t)tuint16);
		break;
	case PROP_DESC_IDPRODUCT:
		tuint16 = g_value_get_uchar(value);
		usbg_ret = usbg_set_gadget_product_id(g, (uint16_t)tuint16);
		break;
	case PROP_DESC_BCDDEVICE:
		tuint16 = g_value_get_uint(value);
		usbg_ret = usbg_set_gadget_device_bcd_device(g, (uint16_t)tuint16);
		break;
	case PROP_GADGET_NAME:
		g_assert (descriptors->gadget_name == NULL);
		descriptors->gadget_name = g_value_dup_string(value);
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
 * @brief gadget descriptors new
 * @param[in] connection A #GDBusConnection
 * @return #GadgetDescriptors object.
 */
GadgetDescriptors *
gadget_descriptors_new(const gchar *gadget_name)
{
	g_return_val_if_fail(gadget_name != NULL, NULL);
	GadgetDescriptors *object;

	object = g_object_new(GADGET_TYPE_DESCRIPTORS,
			     "gadget_name", gadget_name,
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
	gint usbg_ret = USBG_SUCCESS;
	usbg_gadget *g = NULL;
	GadgetDescriptors *descriptors = GADGET_DESCRIPTORS(object);
	usbg_gadget_attrs g_attrs;

	if (descriptors->gadget_name == NULL)
		return;

	g = usbg_get_gadget(ctx.state, descriptors->gadget_name);

	if (g == NULL) {
		ERROR("Cant get a gadget device by name");
		return;
	}

	/* actualize data for gadget */
	usbg_ret = usbg_get_gadget_attrs(g, &g_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		ERROR("Cant get the USB gadget descriptors\n");
		return;
	}

	switch(property_id) {
	case PROP_DESC_BCDUSB:
		g_value_set_uint(value, g_attrs.bcdUSB);
		break;
	case PROP_DESC_BDEVICECLASS:
		g_value_set_uchar(value, g_attrs.bDeviceClass);
		break;
	case PROP_DESC_BDEVICESUBCLASS:
		g_value_set_uchar(value, g_attrs.bDeviceSubClass);
		break;
	case PROP_DESC_BDEVICEPROTOCOL:
		g_value_set_uchar(value, g_attrs.bDeviceProtocol);
		break;
	case PROP_DESC_BMAXPACKETSIZE:
		g_value_set_uchar(value, g_attrs.bMaxPacketSize0);
		break;
	case PROP_DESC_IDVENDOR:
		g_value_set_uint(value, g_attrs.idVendor);
		break;
	case PROP_DESC_IDPRODUCT:
		g_value_set_uint(value, g_attrs.idProduct);
		break;
	case PROP_DESC_BCDDEVICE:
		g_value_set_uint(value, g_attrs.bcdDevice);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget descriptors finalize
 * @param[in] object GObject
 */
static void
gadget_descriptors_finalize(GObject *object)
{
	GadgetDescriptors *gadget_descriptors = GADGET_DESCRIPTORS(object);

	g_free(gadget_descriptors->gadget_name);

	if (G_OBJECT_CLASS(gadget_descriptors_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadget_descriptors_parent_class)->finalize(object);
}

/**
 * @brief gadget daemon class init
 * @param[in] klass GadgetDescriptorsClass
 */
static void
gadget_descriptors_class_init(GadgetDescriptorsClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gadget_descriptors_set_property;
	gobject_class->get_property = gadget_descriptors_get_property;
	gobject_class->finalize = gadget_descriptors_finalize;

	g_object_class_override_property(gobject_class,
					PROP_DESC_BCDUSB,
					"bcd-usb");

	g_object_class_override_property(gobject_class,
					PROP_DESC_BDEVICECLASS,
					"b-device-class");

	g_object_class_override_property(gobject_class,
					PROP_DESC_BDEVICESUBCLASS,
					"b-device-sub-class");

	g_object_class_override_property(gobject_class,
					PROP_DESC_BDEVICEPROTOCOL,
					"b-device-protocol");

	g_object_class_override_property(gobject_class,
					PROP_DESC_BMAXPACKETSIZE,
					"b-max-packet-size0");

	g_object_class_override_property(gobject_class,
					PROP_DESC_IDVENDOR,
					"id-vendor");

	g_object_class_override_property(gobject_class,
					PROP_DESC_IDPRODUCT,
					"id-product");

	g_object_class_override_property(gobject_class,
					PROP_DESC_BCDDEVICE,
					"bcd-device");

	g_object_class_install_property(gobject_class,
                                   PROP_GADGET_NAME,
                                   g_param_spec_string("gadget_name",
                                                       "Gadget name",
                                                       "gadget name",
                                                       NULL,
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

