/*
 * gadgetd-udc-iface.c
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
#include <gadgetd-udc-iface.h>
#include <gadgetd-core.h>
#include <gadgetd-udc-object.h>
#include <gadgetd-gadget-object.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

struct _GadgetdUDCDevice
{
	GadgetdUDCSkeleton parent_instance;

	GadgetdUdcObject *udc_obj;
};

struct _GadgetdUDCDeviceClass
{
	GadgetdUDCSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_UDC_NAME,
	PROP_UDC_OBJ,
	PROP_UDC_ENABLED_GD
} prop_config_iface;

static const char udc_iface[] = "org.usb.device.UDC";

static void gadgetd_udc_device_iface_init(GadgetdUDCIface *iface);

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetdUDCDevice, gadgetd_udc_device, GADGETD_TYPE_UDC_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_UDC, gadgetd_udc_device_iface_init));

/**
 * @brief gadgetd udc device set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
gadgetd_udc_device_set_property(GObject *object, guint property_id,
			   const GValue *value, GParamSpec *pspec)
{
	GadgetdUDCDevice *udc_device = GADGETD_UDC_DEVICE(object);

	switch(property_id) {
	case PROP_UDC_OBJ:
		g_assert(udc_device->udc_obj == NULL);
		udc_device->udc_obj = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd udc device get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectGetPropertyFunc()
 */
static void
gadgetd_udc_device_get_property(GObject *object, guint property_id, GValue *value,
				GParamSpec *pspec)
{
	GadgetdUDCDevice *udc_device = GADGETD_UDC_DEVICE(object);
	const gchar *name = NULL;
	usbg_udc *u;
	gchar *path;
	usbg_gadget *g;

	path = gadgetd_udc_object_get_enabled_gadget_path(udc_device->udc_obj);

	u = gadgetd_udc_object_get_udc(udc_device->udc_obj);
	if (u == NULL)
		return;

	switch(property_id) {
	case PROP_UDC_NAME:
		name = usbg_get_udc_name(u);
		g_value_set_string(value, name);
		break;
	case PROP_UDC_ENABLED_GD:
		g = usbg_get_udc_gadget(u);
		if (g == NULL) {
			if (path != NULL)
				gadgetd_udc_object_set_enabled_gadget_path(udc_device->udc_obj,
									   NULL);
			path = "";
		}
		g_value_set_string(value, path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd udc device class init
 * @param[in] klass GadgetdUDCDeviceClass
 */
static void
gadgetd_udc_device_class_init(GadgetdUDCDeviceClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gadgetd_udc_device_set_property;
	gobject_class->get_property = gadgetd_udc_device_get_property;

	g_object_class_override_property(gobject_class,
					 PROP_UDC_NAME,
					 "name");
	g_object_class_override_property(gobject_class,
					 PROP_UDC_ENABLED_GD,
					 "enabled-gadget");
	g_object_class_install_property(gobject_class,
                                   PROP_UDC_OBJ,
                                   g_param_spec_object("udc-object",
                                                       "udc object",
                                                       "UDC object",
                                                       GADGETD_TYPE_UDC_OBJECT,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));
}

/**
 * @brief gadgetd udc device new
 * @param[in] connection A #GDBusConnection
 * @return #GadgetdUDCDevice object.
 */
GadgetdUDCDevice *
gadgetd_udc_device_new(GadgetdUdcObject *udc_object)
{
	g_return_val_if_fail(udc_object != NULL, NULL);
	GadgetdUDCDevice *object;

	object = g_object_new(GADGETD_TYPE_UDC_DEVICE,
			     "udc-object", udc_object,
			      NULL);
	return object;
}

/**
 * @brief gadgetd udc evice init
 */
static void
gadgetd_udc_device_init(GadgetdUDCDevice *gadgetd_udc_)
{
	/* noop */
}

/**
 * @brief handle enable gadget
 * @param[in] object
 * @param[in] invocation
 * @param[in] gadget_path
 * @return true if metod handled
 */
static gboolean
handle_enable_gadget(GadgetdUDC               *object,
			GDBusMethodInvocation *invocation,
			const gchar           *gadget_path)
{
	GadgetdUDCDevice *udc_device = GADGETD_UDC_DEVICE(object);
	gint usbg_ret = USBG_SUCCESS;
	GVariant *result;
	const gchar *msg;
	GadgetDaemon *daemon;
	GDBusObjectManager *object_manager;
	GadgetdGadgetObject *gadget_object;
	struct gd_gadget *gd_gadget;
	usbg_udc *u;
	gint g_ret = 0;

	INFO("enable gadget handler");

	daemon = gadgetd_udc_object_get_daemon(udc_device->udc_obj);
	if (daemon == NULL) {
		msg = "Failed to get daemon";
		goto error;
	}

	object_manager = G_DBUS_OBJECT_MANAGER(gadget_daemon_get_object_manager(daemon));
	if (object_manager == NULL) {
		msg = "Failed to get object manager";
		goto error;
	}

	gadget_object = GADGETD_GADGET_OBJECT(g_dbus_object_manager_get_object(object_manager,
									       gadget_path));
	if (gadget_object == NULL) {
		msg = "Failed to get gadget object";
		goto error;
	}

	gd_gadget = gadgetd_gadget_object_get_gadget(gadget_object);
	if (gd_gadget == NULL) {
		msg = "Failed to get gadget";
		goto error;
	}

	u = gadgetd_udc_object_get_udc(udc_device->udc_obj);
	if (u == NULL) {
		msg = "Failed to get udc";
		goto error;
	}

	usbg_ret = usbg_enable_gadget(gd_gadget->g, u);
	if (usbg_ret != USBG_SUCCESS) {
		msg = "Failed to enable gadget";
		goto error;
	}

	g_ret = gadgetd_udc_object_set_enabled_gadget_path(udc_device->udc_obj, gadget_path);
	if (g_ret != 0) {
		msg = "Cant set enabled gadget path, gadget will not be enabled";
		/* we can't handle possible errors so we ignore them */
		usbg_disable_gadget(gd_gadget->g);
		goto error;
	}

	result = g_variant_new("(b)", TRUE);
	g_dbus_method_invocation_return_value(invocation, result);

	return TRUE;
error:
	ERROR("%s", msg);
	g_dbus_method_invocation_return_dbus_error(invocation,
			udc_iface,
			msg);

	return FALSE;
}

/**
 * @brief handle disable gadget
 * @param[in] object
 * @param[in] invocation
 * @param[in] gadget_path
 * @return true if metod handled
 */
static gboolean
handle_disable_gadget(GadgetdUDC            *object,
		      GDBusMethodInvocation *invocation)
{
	GadgetdUDCDevice *udc_device = GADGETD_UDC_DEVICE(object);
	gint usbg_ret = USBG_SUCCESS;
	GVariant *result;
	const gchar *msg;
	usbg_udc *u;
	usbg_gadget *g;

	INFO("disable gadget handler");

	u = gadgetd_udc_object_get_udc(udc_device->udc_obj);
	if (u == NULL) {
		msg = "Failed to get udc";
		goto error;
	}

	g = usbg_get_udc_gadget(u);
	if (g == NULL) {
		msg = "No gadget enabled";
		goto error;
	}

	usbg_ret = usbg_disable_gadget(g);
	if (usbg_ret != USBG_SUCCESS) {
		msg = "Failed to disable gadget";
		goto error;
	}

	gadgetd_udc_object_set_enabled_gadget_path(udc_device->udc_obj, NULL);

	result = g_variant_new("(b)", TRUE);
	g_dbus_method_invocation_return_value(invocation, result);

	return TRUE;
error:
	ERROR("%s", msg);
	g_dbus_method_invocation_return_dbus_error(invocation,
			udc_iface,
			msg);

	return TRUE;
}

/**
 * @brief gadgetd udc device iface init
 * @param[in] iface GadgetdGadgetManagerIface
 */
static void
gadgetd_udc_device_iface_init(GadgetdUDCIface *iface)
{
	iface->handle_enable_gadget  = handle_enable_gadget;
	iface->handle_disable_gadget = handle_disable_gadget;
}

