/*
 * gadgetd-udc-object.c
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
#include <string.h>

#include <gio/gio.h>
#include <glib/gprintf.h>

#include <gadgetd-udc-object.h>
#include <gadgetd-gdbus-codegen.h>
#include <gadgetd-common.h>
#include <gadgetd-udc-iface.h>

typedef struct _GadgetdUdcObjectClass   GadgetdUdcObjectClass;

struct _GadgetdUdcObject
{
	GadgetdObjectSkeleton parent_instance;
	GadgetDaemon *daemon;

	gchar *enabled_gadget_path;
	usbg_udc *u;
};

struct _GadgetdUdcObjectClass
{
	GadgetdObjectSkeletonClass parent_class;
};


G_DEFINE_TYPE(GadgetdUdcObject, gadgetd_udc_object, GADGETD_TYPE_OBJECT_SKELETON);

enum
{
	PROP_0,
	PROP_UDC_PTR,
	PROP_DAEMON,
} prop_udc_obj;

/**
 * @brief gadgetd udc object Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
 void
gadgetd_udc_object_set_property(GObject            *object,
				    guint           property_id,
				    const GValue   *value,
				    GParamSpec     *pspec)
{
	GadgetdUdcObject *udc_object = GADGETD_UDC_OBJECT(object);

	switch(property_id) {
	case PROP_UDC_PTR:
		g_assert(udc_object->u == NULL);
		udc_object->u = g_value_get_pointer(value);
		break;
	case PROP_DAEMON:
		g_assert(udc_object->daemon == NULL);
		udc_object->daemon = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget config get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectGetPropertyFunc()
 */
static void
gadgetd_udc_object_get_property(GObject *object, guint property_id, GValue *value,
			   GParamSpec *pspec)
{
	switch(property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief Gets the daemon
 * @param[in] object GadgetdUdcObject
 * @return GadgetDaemon, dont free
 */
GadgetDaemon *
gadgetd_udc_object_get_daemon(GadgetdUdcObject *object)
{
	g_return_val_if_fail(GADGETD_IS_UDC_OBJECT(object), NULL);
	return object->daemon;
}

/**
 * @brief Gets the udc
 * @param[in] object GadgetdUdcObject
 * @return usbg_udc
 */
usbg_udc *
gadgetd_udc_object_get_udc(GadgetdUdcObject *object)
{
	g_return_val_if_fail(GADGETD_IS_UDC_OBJECT(object), NULL);
	return object->u;
}

/**
 * @brief Gets the enabled gadget path
 * @param[in] object GadgetdUdcObject
 * @return enabled gadget path
 */
gchar *
gadgetd_udc_object_get_enabled_gadget_path(GadgetdUdcObject *object)
{
	g_return_val_if_fail(GADGETD_IS_UDC_OBJECT(object), NULL);
	return object->enabled_gadget_path;
}

/**
 * @brief Set the enabled gadget path
 * @param[in] object GadgetdUdcObject
 * @return enabled gadget path
 */
gint
gadgetd_udc_object_set_enabled_gadget_path(GadgetdUdcObject *object, const gchar *path)
{
	g_return_val_if_fail(GADGETD_IS_UDC_OBJECT(object), GD_ERROR_OTHER_ERROR);

	if (object->enabled_gadget_path != NULL) {
		g_free(object->enabled_gadget_path);
		object->enabled_gadget_path = NULL;
	}

	if (path == NULL)
		return GD_SUCCESS;

	object->enabled_gadget_path = g_strdup(path);

	if (object->enabled_gadget_path == NULL)
		return GD_ERROR_NO_MEM;

	return GD_SUCCESS;
}

/**
 * @brief gadgetd udc object init
 * @param[in] object GadgetdUdcObject
 */
static void
gadgetd_udc_object_init(GadgetdUdcObject *object)
{
	/*nop*/
}

/**
 * @brief gadgetd gadget object constructed
 * @param[in] object GObject
 */
static void
gadgetd_udc_object_constructed(GObject *object)
{
	GadgetdUdcObject *udc_object = GADGETD_UDC_OBJECT(object);
	gchar _cleanup_g_free_ *path = NULL;
	const gchar *udc_name = NULL;
	gchar _cleanup_g_free_ *valid_udc_name = NULL;
	GadgetdUDCDevice *udc_iface;
	int ret;

	udc_name = usbg_get_udc_name(udc_object->u);
	if (udc_name == NULL) {
		ERROR("Unable to get udc name");
		return;
	}

	ret = make_valid_object_path_part(udc_name, &valid_udc_name);
	if (ret != GD_SUCCESS) {
		ERROR("Unable to create valid path part for udc");
		return;
	}

	path = g_strdup_printf("%s/UDC/%s", gadgetd_path, valid_udc_name);

	udc_iface = gadgetd_udc_device_new(udc_object);

	get_iface(G_OBJECT(udc_object),GADGETD_TYPE_UDC_DEVICE, &udc_iface);

	if (path != NULL && g_variant_is_object_path(path) && udc_object != NULL)
		g_dbus_object_skeleton_set_object_path(G_DBUS_OBJECT_SKELETON(udc_object), path);
	else
		ERROR("Unexpected error during udc object creation");

	if (G_OBJECT_CLASS(gadgetd_udc_object_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(gadgetd_udc_object_parent_class)->constructed(object);
}

/**
 * @brief gadgetd udc object finalize
 * @param[in] object GObject
 */
static void
gadgetd_udc_object_finalize(GObject *object)
{
	GadgetdUdcObject *udc_object = GADGETD_UDC_OBJECT(object);

	g_free(udc_object->enabled_gadget_path);

	if (G_OBJECT_CLASS(gadgetd_udc_object_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadgetd_udc_object_parent_class)->finalize(object);
}

/**
 * @brief gadgetd udc object class init
 * @param[in] klass GadgetdUdcObjectClass
 */
static void
gadgetd_udc_object_class_init(GadgetdUdcObjectClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructed  = gadgetd_udc_object_constructed;
	gobject_class->set_property = gadgetd_udc_object_set_property;
	gobject_class->get_property = gadgetd_udc_object_get_property;
	gobject_class->finalize     = gadgetd_udc_object_finalize;

	g_object_class_install_property(gobject_class,
                                   PROP_UDC_PTR,
                                   g_param_spec_pointer("udc-pointer",
                                                       "udc-pointer",
                                                       "Pointer to udc",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object("daemon",
                                                       "Daemon",
                                                       "daemon for the object",
                                                       GADGET_TYPE_DAEMON,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));
}

/**
 * @brief gadgetd udc object new
 * @param[in] gadget_name gchar gadget name
 * @return GadgetdUdcObject object
 */
GadgetdUdcObject *
gadgetd_udc_object_new(usbg_udc *u, GadgetDaemon *daemon)
{
	g_return_val_if_fail(u != NULL, NULL);
	g_return_val_if_fail(daemon != NULL, NULL);

	GadgetdUdcObject *object;
	object = g_object_new(GADGETD_TYPE_UDC_OBJECT,
			     "udc-pointer", u,
			     "daemon",daemon,
			      NULL);

	return object;
}
