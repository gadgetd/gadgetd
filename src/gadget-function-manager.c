/*
 * gadget-function-manager.c
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gio/gio.h>

#include <gadget-daemon.h>
#include "gadgetd-gdbus-codegen.h"
#include <gadget-function-manager.h>
#include <gadgetd-function-object.h>

typedef struct _GadgetFunctionManagerClass   GadgetFunctionManagerClass;

static const char func_manager_iface[] = "org.usb.device.Gadget.FunctionManager";

struct _GadgetFunctionManager
{
	GadgetdGadgetFunctionManagerSkeleton parent_instance;
	GadgetDaemon *daemon;

	struct gd_gadget *gadget;
	gchar *gadget_path;
};

struct _GadgetFunctionManagerClass
{
	GadgetdGadgetFunctionManagerSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_DAEMON,
	PROP_GADGET_PATH,
	PROP_GADGET_PTR
} prop_func_manager;

/**
 * @brief gadget function manager iface init
 * @param[in] iface GadgetdGadgetFunctionManagerIface
 */
static void gadget_function_manager_iface_init(GadgetdGadgetFunctionManagerIface *iface);

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetFunctionManager, gadget_function_manager, GADGETD_TYPE_GADGET_FUNCTION_MANAGER_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_FUNCTION_MANAGER, gadget_function_manager_iface_init));

/**
 * @brief gadget function manager init
 * @param[in] function_manager GadgetFunctionManager struct
 */
static void
gadget_function_manager_init(GadgetFunctionManager *function_manager)
{
	/* noop */
}

/**
 * @brief gadget function manager finalize
 * @param[in] object GObject
 */
static void
gadget_function_manager_finalize(GObject *object)
{
	GadgetFunctionManager *function_manager = GADGET_FUNCTION_MANAGER(object);

	g_free(function_manager->gadget_path);

	if (G_OBJECT_CLASS(gadget_function_manager_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadget_function_manager_parent_class)->finalize(object);
}

/**
 * @brief gadget function manager Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
static void
gadget_function_manager_set_property(GObject            *object,
				      guint               property_id,
				      const GValue       *value,
				      GParamSpec         *pspec)
{
	GadgetFunctionManager *fun_manager = GADGET_FUNCTION_MANAGER(object);

	switch(property_id) {
	case PROP_DAEMON:
		g_assert(fun_manager->daemon == NULL);
		fun_manager->daemon = g_value_get_object(value);
		break;
	case PROP_GADGET_PATH:
		g_assert(fun_manager->gadget_path == NULL);
		fun_manager->gadget_path = g_value_dup_string(value);
		break;
	case PROP_GADGET_PTR:
		g_assert(fun_manager->gadget == NULL);
		fun_manager->gadget = g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
* @brief gadget function manager get property.
* @details generic Getter for all properties of this type
* @param[in] object a GObject
* @param[in] property_id numeric id under which the property was registered with
* @param[in] value a GValue to return the property value in
* @param[in] pspec the GParamSpec structure describing the property
*/
static void
gadget_function_manager_get_property(GObject          *object,
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
 * @brief gadget function manager class init
 * @param[in] klass GadgetFunctionManagerClass
 */
static void
gadget_function_manager_class_init(GadgetFunctionManagerClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize     = gadget_function_manager_finalize;
	gobject_class->set_property = gadget_function_manager_set_property;
	gobject_class->get_property = gadget_function_manager_get_property;

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
 * @brief gadget function manager new
 * @details Create a new GadgetFunctionManager instance
 * @param[in] daemon GadgetDaemon
 * @param[in] gadget_path Path where gadget is exported
 * @param[in] gadget Pointer to gadget
 * @return GadgetFunctionManager object neet to be free.
 */
GadgetFunctionManager *
gadget_function_manager_new(GadgetDaemon *daemon, const gchar *gadget_path,
			    struct gd_gadget *gadget)
{
	g_return_val_if_fail(gadget != NULL, NULL);

	GadgetFunctionManager *object;

	object = g_object_new(GADGET_TYPE_FUNCTION_MANAGER,
			      "daemon", daemon,
			      "gadget_path", gadget_path,
			      "gd_gadget", gadget,
			      NULL);

	return object;
}

/**
 * @brief Gets the daemon used by @function manager
 * @param[in] function_manager GadgetManager
 * @return GadgetDaemon object, dont free used by @manager
 */
GadgetDaemon *
gadget_function_manager_get_daemon(GadgetFunctionManager *function_manager)
{
	g_return_val_if_fail(GADGET_IS_FUNCTION_MANAGER(function_manager), NULL);
	return function_manager->daemon;
}

/**
 * @brief Create function handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] instance
 * @param[in] type
 * @return true if metod handled
 */
static gboolean
handle_create_function(GadgetdGadgetFunctionManager	*object,
		        GDBusMethodInvocation		*invocation,
		        const gchar			*instance,
		        const gchar			*type)
{
	gchar _cleanup_g_free_ *function_path = NULL;
	const gchar *msg = NULL;
	GadgetFunctionManager *func_manager = GADGET_FUNCTION_MANAGER(object);
	GadgetdFunctionObject *function_object;
	GadgetDaemon *daemon;
	struct gd_gadget *gadget = func_manager->gadget;
	const gchar *gadget_name = usbg_get_gadget_name(gadget->g);
	gchar _cleanup_g_free_ *instance_path = NULL;
	gchar _cleanup_g_free_ *type_path = NULL;
	struct gd_function *func = NULL;
	gint ret;

	daemon = gadget_function_manager_get_daemon(GADGET_FUNCTION_MANAGER(object));

	INFO("handled create function");

	if (gadget == NULL || gadget_name == NULL) {
		msg = "Unable to get gadget";
		goto err;
	}

	ret = make_valid_object_path_part(instance, &instance_path);
	if (ret != GD_SUCCESS) {
		msg = "Invalid instance name";
		goto err;
	}

	ret = make_valid_object_path_part(type, &type_path);
	if (ret != GD_SUCCESS) {
		msg = "Invalid type name";
		goto err;
	}

	function_path = g_strdup_printf("%s/Function/%s/%s",
					func_manager->gadget_path,
					type_path,
					instance_path);

	if (function_path == NULL || !g_variant_is_object_path(function_path)) {
		msg = "Invalid function instance or type";
		goto err;
	}

	ret = gd_create_function(gadget, type, instance, &func, &msg);
	if (ret != GD_SUCCESS)
		goto err;

	function_object = gadgetd_function_object_new(function_path, func);
	if (function_object == NULL) {
		msg = "Unable to create function object";
		goto err;
	}

	g_dbus_object_manager_server_export(gadget_daemon_get_object_manager(daemon),
					    G_DBUS_OBJECT_SKELETON(function_object));

	/* send function path*/
	g_dbus_method_invocation_return_value(invocation,
					      g_variant_new("(o)",
					      function_path));
	return TRUE;

err:
	g_dbus_method_invocation_return_dbus_error(invocation,
			func_manager_iface,
			msg);
	return TRUE;
}

/**
 * @brief remove function handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] function_path
 * @return true if metod handled
 */
static gboolean
handle_remove_function(GadgetdGadgetFunctionManager	*object,
			GDBusMethodInvocation		*invocation,
			const gchar			*function_path)
{
	/* TODO add remove function handler */
	INFO("remove function handler");

	return TRUE;
}

/**
 * @brief find function by name handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] function_name
 * @return true if metod handled
 */
static gboolean
handle_find_function_by_name(GadgetdGadgetFunctionManager	*object,
		      GDBusMethodInvocation			*invocation,
		      const gchar				*type,
		      const gchar				*instance)
{
	const gchar *path = NULL;
	const gchar *msg;
	gchar _cleanup_g_free_ *function_name = NULL;
	GadgetDaemon *daemon;
	GDBusObjectManager *object_manager;
	GList *objects;
	GList *l;
	struct gd_function *gd_func;
	GadgetFunctionManager *func_manager = GADGET_FUNCTION_MANAGER(object);

	INFO("find function by name handler");

	daemon = gadget_function_manager_get_daemon(GADGET_FUNCTION_MANAGER(object));
	if (daemon == NULL) {
		msg = "Failed to get daemon";
		goto out2;
	}

	object_manager = G_DBUS_OBJECT_MANAGER(gadget_daemon_get_object_manager(daemon));
	if (object_manager == NULL) {
		ERROR("Failed to get object manager");
		goto out2;
	}

	msg = "Failed to find function";
	objects = g_dbus_object_manager_get_objects(object_manager);
	for (l = objects; l != NULL; l = l->next)
	{
		GadgetdObject *object = GADGETD_OBJECT (l->data);
		path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));
		if (!GADGETD_IS_FUNCTION_OBJECT(G_DBUS_OBJECT(object)) ||
					!g_str_has_prefix(path, func_manager->gadget_path))
			continue;

		gd_func = gadgetd_function_object_get_function(GADGETD_FUNCTION_OBJECT(object));
		if (gd_func == NULL) {
			msg = "Failed to get function";
			break;
		}
		if (g_strcmp0(gd_func->type, type) == 0 && (strcmp(gd_func->instance, instance)== 0)) {
			path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));
			msg = NULL;
			goto out;
		}
	}

	if (path == NULL)
		msg = "Failed to find function";

	g_list_foreach(objects, (GFunc)g_object_unref, NULL);
	g_list_free(objects);

out2:
	if (msg != NULL) {
		ERROR("%s", msg);
		g_dbus_method_invocation_return_dbus_error(invocation,
				func_manager_iface,
				msg);
		return TRUE;
	}
out:
	/* send gadget path */
	g_dbus_method_invocation_return_value(invocation,
				      g_variant_new("(o)", path));

	return TRUE;
}

/**
 * @brief gadget function manager iface init
 * @param[in] iface GadgetdGadgetFunctionManagerIface
 */
static void
gadget_function_manager_iface_init(GadgetdGadgetFunctionManagerIface *iface)
{
	iface->handle_create_function = handle_create_function;
	iface->handle_remove_function = handle_remove_function;
	iface->handle_find_function_by_name = handle_find_function_by_name;
}

