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

typedef struct _GadgetdUdcObjectClass   GadgetdUdcObjectClass;

struct _GadgetdUdcObject
{
	GadgetdObjectSkeleton parent_instance;

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
 * @brief gadgetd udc object init
 * @param[in] object GadgetdUdcObject
 */
static void
gadgetd_udc_object_init(GadgetdUdcObject *object)
{
	/*nop*/
}

/**
 * @brief get proper udc name
 * @param[in] udc_object GadgetdUdcObject
 * @param[out] name proper name
 */
static void
get_udc_name(GadgetdUdcObject *udc_object, gchar **name)
{
	char tmp_name[256];
	int i=0;

	usbg_cpy_udc_name(udc_object->u, tmp_name, sizeof(tmp_name));

	for (i = 0; tmp_name[i] != '\0'; i++)
		if (tmp_name[i] == '.')
			tmp_name[i] = '/';

	*name = strdup(tmp_name);
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
	gchar _cleanup_g_free_ *udc_name = NULL;

	get_udc_name(udc_object, &udc_name);

	if (udc_name != NULL)
		path = g_strdup_printf("%s/UDC/%s", gadgetd_path, udc_name);

	if (path != NULL && g_variant_is_object_path(path) && udc_object != NULL)
		g_dbus_object_skeleton_set_object_path(G_DBUS_OBJECT_SKELETON(udc_object), path);
	else
		ERROR("Unexpected error during udc object creation");

	if (G_OBJECT_CLASS(gadgetd_udc_object_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(gadgetd_udc_object_parent_class)->constructed(object);
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

	g_object_class_install_property(gobject_class,
                                   PROP_UDC_PTR,
                                   g_param_spec_pointer("udc-pointer",
                                                       "udc-pointer",
                                                       "Pointer to udc",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

/**
 * @brief gadgetd udc object new
 * @param[in] gadget_name gchar gadget name
 * @return GadgetdUdcObject object
 */
GadgetdUdcObject *
gadgetd_udc_object_new(usbg_udc *u)
{
	g_return_val_if_fail(u != NULL, NULL);

	GadgetdUdcObject *object;
	object = g_object_new(GADGETD_TYPE_UDC_OBJECT,
			     "udc-pointer", u,
			      NULL);

	return object;
}
