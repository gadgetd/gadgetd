/*
 * gadgetd-config-object.c
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

#include <gadgetd-config-object.h>
#include <gadgetd-gdbus-codegen.h>
#include <gadgetd-common.h>
#include <gadget-config-manager.h>
#include <dbus-config-ifaces/gadget-config.h>

typedef struct _GadgetdConfigObjectClass   GadgetdConfigObjectClass;

struct _GadgetdConfigObject
{
	GadgetdObjectSkeleton parent_instance;
	GadgetDaemon *daemon;

	gchar *config_path;
	gint  config_id;
	gchar *config_label;

	usbg_config *cfg;
	GadgetConfig *gadget_cfg_iface;
};

struct _GadgetdConfigObjectClass
{
	GadgetdObjectSkeletonClass parent_class;
};

G_DEFINE_TYPE(GadgetdConfigObject, gadgetd_config_object, GADGETD_TYPE_OBJECT_SKELETON);

enum
{
	PROP_0,
	PROP_CFG_OBJ_LABEL,
	PROP_CFG_OBJ_ID,
	PROP_CFG_PTR,
	PROP_CFG_PATH,
	PROP_DAEMON,
} prop_cfg_obj;

/**
 * @brief config object finalize
 * @param[in] object GObject
 */
static void
gadgetd_config_object_finalize(GObject *object)
{
	GadgetdConfigObject *config_object = GADGETD_CONFIG_OBJECT(object);

	g_free(config_object->config_path);
	g_free(config_object->config_label);

	if (G_OBJECT_CLASS(gadgetd_config_object_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadgetd_config_object_parent_class)->finalize(object);
}

/**
 * @brief Get config
 * @param[in] config_object GadgetdConfigObject
 * @return cfg usbg_config
 */
usbg_config *
gadgetd_config_object_get_config(GadgetdConfigObject *config_object)
{
	g_return_val_if_fail(GADGETD_IS_CONFIG_OBJECT(config_object), NULL);
	return config_object->cfg;
}

/**
 * @brief Get config id
 * @param[in] config_object GadgetdConfigObject
 * @return cfg usbg_config
 */
gint
gadgetd_config_object_get_config_id(GadgetdConfigObject *config_object)
{
	g_return_val_if_fail(GADGETD_IS_CONFIG_OBJECT(config_object), -1);
	return config_object->config_id;
}

/**
 * @brief Get config label
 * @param[in] config_object GadgetdConfigObject
 * @return cfg usbg_config
 */
const gchar *
gadgetd_config_object_get_config_label(GadgetdConfigObject *config_object)
{
	g_return_val_if_fail(GADGETD_IS_CONFIG_OBJECT(config_object), NULL);
	return config_object->config_label;
}

/**
 * @brief Gets the daemon
 * @param[in] object GadgetdConfigObject
 * @return GadgetDaemon, dont free
 */
GadgetDaemon *
gadgetd_config_object_get_daemon(GadgetdConfigObject *config_object)
{
	g_return_val_if_fail(GADGETD_IS_CONFIG_OBJECT(config_object), NULL);
	return config_object->daemon;
}

/**
 * @brief gadgetd config object get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 */
static void
gadgetd_config_object_get_property(GObject          *object,
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
 * @brief gadgetd config object Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
 void
gadgetd_config_object_set_property(GObject            *object,
				   guint              property_id,
				   const GValue      *value,
				   GParamSpec        *pspec)
{
	GadgetdConfigObject *config_object = GADGETD_CONFIG_OBJECT(object);

	switch(property_id) {
	case PROP_CFG_OBJ_LABEL:
		g_assert(config_object->config_label == NULL);
		config_object->config_label = g_value_dup_string(value);
		break;
	case PROP_CFG_OBJ_ID:
		config_object->config_id = g_value_get_int(value);
		break;
	case PROP_CFG_PTR:
		g_assert(config_object->cfg == NULL);
		config_object->cfg = g_value_get_pointer(value);
		break;
	case PROP_CFG_PATH:
		g_assert(config_object->config_path == NULL);
		config_object->config_path = g_value_dup_string(value);
		break;
	case PROP_DAEMON:
		g_assert(config_object->daemon == NULL);
		config_object->daemon = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd config object init
 * @param[in] object GadgetdConfigObject
 */
static void
gadgetd_config_object_init(GadgetdConfigObject *object)
{
	/*nop*/
}

/**
 * @brief gadgetd gadget object constructed
 * @param[in] object GObject
 */
static void
gadgetd_config_object_constructed(GObject *object)
{
	GadgetdConfigObject *config_object = GADGETD_CONFIG_OBJECT(object);
	gchar *path = config_object->config_path;

	config_object->gadget_cfg_iface = gadget_config_new(config_object);

	get_iface(G_OBJECT(config_object),
		  GADGETD_TYPE_CONFIG_OBJECT,
		  &config_object->gadget_cfg_iface);

	if (path != NULL && g_variant_is_object_path(path) && config_object != NULL)
		g_dbus_object_skeleton_set_object_path(G_DBUS_OBJECT_SKELETON(config_object), path);
	else
		ERROR("Unexpected error during config object creation");

	if (G_OBJECT_CLASS(gadgetd_config_object_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(gadgetd_config_object_parent_class)->constructed(object);
}

/**
 * @brief gadgetd config object class init
 * @param[in] klass GadgetdConfigObjectClass
 */
static void
gadgetd_config_object_class_init(GadgetdConfigObjectClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gadgetd_config_object_finalize;
	gobject_class->constructed  = gadgetd_config_object_constructed;
	gobject_class->set_property = gadgetd_config_object_set_property;
	gobject_class->get_property = gadgetd_config_object_get_property;

	g_object_class_install_property(gobject_class,
                                   PROP_CFG_OBJ_ID,
                                   g_param_spec_int("config_id",
                                                    "config id",
                                                    "Config_id",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(gobject_class,
                                   PROP_CFG_OBJ_LABEL,
                                   g_param_spec_string("config_label",
                                                       "config label",
                                                       "Config label",
                                                       NULL,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(gobject_class,
                                   PROP_CFG_PTR,
                                   g_param_spec_pointer("config-pointer",
                                                       "config-pointer",
                                                       "Pointer to config",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(gobject_class,
					PROP_CFG_PATH,
					g_param_spec_string("config_path",
							    "Config path",
							    "config path",
							    NULL,
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
 * @brief gadgetd config object new
 * @param[in] gadget_name gchar gadget name
 * @return GadgetdConfigObject object
 */
GadgetdConfigObject *
gadgetd_config_object_new(const gchar *config_path, gint config_id, const gchar *config_label,
			  usbg_config *cfg, GadgetDaemon *daemon)
{
	g_return_val_if_fail(config_path  != NULL, NULL);
	g_return_val_if_fail(config_id    != FALSE, NULL);
	g_return_val_if_fail(config_label != NULL, NULL);
	g_return_val_if_fail(daemon != NULL, NULL);

	GadgetdConfigObject *object;
	object = g_object_new(GADGETD_TYPE_CONFIG_OBJECT,
			     "config_path",  config_path,
			     "config_id",    config_id,
			     "config_label", config_label,
			     "config-pointer", cfg,
			     "daemon",daemon,
			      NULL);

	return object;
}
