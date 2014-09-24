/*
 * gadget-config-manager.c
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gio/gio.h>

#include <gadget-daemon.h>
#include "gadgetd-gdbus-codegen.h"
#include <gadget-config-manager.h>
#include <gadgetd-config-object.h>

typedef struct _GadgetConfigManagerClass GadgetConfigManagerClass;

static const char cfg_manager_iface[] = "org.usb.device.Gadget.ConfigManager";

struct _GadgetConfigManager
{
	GadgetdGadgetConfigManagerSkeleton parent_instance;
	GadgetDaemon *daemon;

	struct gd_gadget *gadget;
	gchar *gadget_path;
};

struct _GadgetConfigManagerClass
{
	GadgetdGadgetConfigManagerSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_DAEMON,
	PROP_GADGET_PATH,
	PROP_GADGET_PTR
} prop_cfg_manager;

/**
 * @brief gadget config manager iface init
 * @param[in] iface GadgetdGadgetConfigManagerIface
 */
static void gadget_config_manager_iface_init(GadgetdGadgetConfigManagerIface *iface);

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() config,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetConfigManager, gadget_config_manager, GADGETD_TYPE_GADGET_CONFIG_MANAGER_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_CONFIG_MANAGER, gadget_config_manager_iface_init));

/**
 * @brief gadget config manager init
 * @param[in] config_manager GadgetConfigManager struct
 */
static void
gadget_config_manager_init(GadgetConfigManager *config_manager)
{
	/* noop */
}

/**
 * @brief gadget config manager finalize
 * @param[in] object GObject
 */
static void
gadget_config_manager_finalize(GObject *object)
{
	GadgetConfigManager *config_manager = GADGET_CONFIG_MANAGER(object);

	g_free(config_manager->gadget_path);

	if (G_OBJECT_CLASS(gadget_config_manager_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadget_config_manager_parent_class)->finalize(object);
}

/**
 * @brief gadget config manager Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
static void
gadget_config_manager_set_property(GObject            *object,
				   guint               property_id,
				   const GValue       *value,
				   GParamSpec         *pspec)
{
	GadgetConfigManager *cfg_manager = GADGET_CONFIG_MANAGER(object);

	switch(property_id) {
	case PROP_DAEMON:
		g_assert(cfg_manager->daemon == NULL);
		cfg_manager->daemon = g_value_get_object(value);
		break;
	case PROP_GADGET_PTR:
		g_assert(cfg_manager->gadget == NULL);
		cfg_manager->gadget = g_value_get_pointer(value);
		break;
	case PROP_GADGET_PATH:
		g_assert(cfg_manager->gadget_path == NULL);
		cfg_manager->gadget_path = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
* @brief gadget config manager get property.
* @details generic Getter for all properties of this type
* @param[in] object a GObject
* @param[in] property_id numeric id under which the property was registered with
* @param[in] value a GValue to return the property value in
* @param[in] pspec the GParamSpec structure describing the property
*/
static void
gadget_config_manager_get_property(GObject          *object,
                                   guint             property_id,
                                   GValue           *value,
                                   GParamSpec       *pspec)
{
	switch(property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget config manager class init
 * @param[in] klass GadgetConfigManagerClass
 */
static void
gadget_config_manager_class_init(GadgetConfigManagerClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize     = gadget_config_manager_finalize;
	gobject_class->set_property = gadget_config_manager_set_property;
	gobject_class->get_property = gadget_config_manager_get_property;

	g_object_class_install_property(gobject_class,
                                   PROP_GADGET_PTR,
                                   g_param_spec_pointer("gd_gadget",
                                                       "Gadget ptr",
                                                       "gadget ptr",
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(gobject_class,
                                   PROP_GADGET_PATH,
                                   g_param_spec_string("gadget_path",
                                                       "Gadget path",
                                                       "gadget path",
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
 * @brief gadget config manager new
 * @details Create a new GadgetConfigManager instance
 * @param[in] daemon GadgetDaemon
 * @param[in] gadget_path Place where gaget is exported
 * @param[in] gadget Pointer to gadget
 * @return GadgetConfigManager object neet to be free.
 */
GadgetConfigManager *
gadget_config_manager_new(GadgetDaemon *daemon, const gchar *gadget_path,
			  struct gd_gadget *gadget)
{
	g_return_val_if_fail(gadget != NULL, NULL);

	GadgetConfigManager *object;

	object = g_object_new(GADGET_TYPE_CONFIG_MANAGER,
			      "daemon", daemon,
			      "gadget_path", gadget_path,
			      "gd_gadget", gadget,
			      NULL);

	return object;
}

/**
 * @brief Gets the daemon used by @config manager
 * @param[in] config_manager GadgetManager
 * @return GadgetDaemon object, dont free used by @manager
 */
GadgetDaemon *
gadget_config_manager_get_daemon(GadgetConfigManager *config_manager)
{
	g_return_val_if_fail(GADGET_IS_CONFIG_MANAGER(config_manager), NULL);
	return config_manager->daemon;
}

/**
 * @brief Create config handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] instance
 * @param[in] type
 * @return true if metod handled
 */
static gboolean
handle_create_config(GadgetdGadgetConfigManager  *object,
		     GDBusMethodInvocation       *invocation,
		     gint config_id, const gchar *config_label)
{
	gchar _cleanup_g_free_ *config_path = NULL;
	const gchar *msg = NULL;
	GadgetConfigManager *config_manager = GADGET_CONFIG_MANAGER(object);
	gint usbg_ret = USBG_SUCCESS;
	usbg_config *c;
	GadgetDaemon *daemon;
	GadgetdConfigObject *config_object;
	struct gd_gadget *gadget = config_manager->gadget;

	INFO("handled create config");

	daemon = gadget_config_manager_get_daemon(GADGET_CONFIG_MANAGER(object));

	config_path = g_strdup_printf("%s/Config/%d",
				      config_manager->gadget_path,
				      config_id);

	if (config_path == NULL || config_label == NULL ||
			!g_variant_is_object_path(config_path)) {
		msg = "Invalid config id";
		goto err;
	}

	usbg_ret = usbg_create_config(gadget->g, config_id, config_label,
				      NULL, NULL, &c);
	if (usbg_ret != USBG_SUCCESS) {
		ERROR("Error on config create");
		ERROR("Error: %s: %s", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		msg = usbg_error_name(usbg_ret);
		goto err;
	}

	config_object = gadgetd_config_object_new(config_path,
						  config_id, config_label, c);
	if (config_object == NULL) {
		msg = "Unable to create function object";
		goto err;
	}

	g_dbus_object_manager_server_export(gadget_daemon_get_object_manager(daemon),
					    G_DBUS_OBJECT_SKELETON(config_object));

	/* send function path*/
	g_dbus_method_invocation_return_value(invocation,
					      g_variant_new("(o)",
					      config_path));

	return TRUE;

err:
	g_dbus_method_invocation_return_dbus_error(invocation,
			cfg_manager_iface,
			msg);
	return FALSE;
}

/**
 * @brief remove config handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] config_path
 * @return true if metod handled
 */
static gboolean
handle_remove_config(GadgetdGadgetConfigManager		*object,
		     GDBusMethodInvocation		*invocation,
		     const gchar			*config_path)
{
	/* TODO add remove config handler */
	INFO("remove config handler");

	return TRUE;
}

/**
 * @brief find config by name handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] config_name
 * @return true if metod handled
 */
static gboolean
handle_find_config_by_id(GadgetdGadgetConfigManager	*object,
			 GDBusMethodInvocation		*invocation,
			 gint				config_id)
{
	/* TODO add find config by name handler*/
	INFO("find config by name handler");

	return TRUE;
}

/**
 * @brief gadget config manager iface init
 * @param[in] iface GadgetdGadgetConfigManagerIface
 */
static void
gadget_config_manager_iface_init(GadgetdGadgetConfigManagerIface *iface)
{
	iface->handle_create_config = handle_create_config;
	iface->handle_remove_config = handle_remove_config;
	iface->handle_find_config_by_id = handle_find_config_by_id;
}

