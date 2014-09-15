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

	gchar *gadget_name;
};

struct _GadgetFunctionManagerClass
{
	GadgetdGadgetFunctionManagerSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_DAEMON,
	PROP_GADGET_NAME
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

	g_free(function_manager->gadget_name);

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
	case PROP_GADGET_NAME:
		g_assert(fun_manager->gadget_name == NULL);
		fun_manager->gadget_name = g_value_dup_string(value);
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
                                   PROP_GADGET_NAME,
                                   g_param_spec_string("gadget_name",
                                                       "Gadget name",
                                                       "gadget name",
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
 * @return GadgetFunctionManager object neet to be free.
 */
GadgetFunctionManager *
gadget_function_manager_new(GadgetDaemon *daemon, const gchar *gadget_name)
{
	g_return_val_if_fail(gadget_name != NULL, NULL);

	GadgetFunctionManager *object;

	object = g_object_new(GADGET_TYPE_FUNCTION_MANAGER,
			     "daemon", daemon,
			     "gadget_name", gadget_name,
			      NULL);

	return object;
}

/**
 * @brief get function type
 * @param[in] _str_type
 * @param[out] _type
 */
static gboolean
get_function_type(const gchar *_str_type, usbg_function_type *_type)
{
	int i;

	static const struct _f_type
	{
		gchar *name;
		usbg_function_type type;
	} type[] = {
		{.name = "gser",  .type = F_SERIAL},
		{.name = "acm",   .type = F_ACM},
		{.name = "obex",  .type = F_OBEX},
		{.name = "geth",  .type = F_SUBSET},
		{.name = "ecm",   .type = F_ECM},
		{.name = "ncm",   .type = F_NCM},
		{.name = "eem",   .type = F_EEM},
		{.name = "rndis", .type = F_RNDIS},
		{.name = "phonet",.type = F_PHONET},
		{.name = "ffs",   .type = F_FFS}
	};

	for (i=0; i < G_N_ELEMENTS(type); i++) {
		if (g_strcmp0(type[i].name, _str_type) == 0) {
			*_type = type[i].type;
			return TRUE;
		}
	}

	return FALSE;
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
		        const gchar			*_str_type)
{
	gchar _cleanup_g_free_ *function_path = NULL;
	gint usbg_ret = USBG_SUCCESS;
	usbg_function_type type;
	usbg_gadget *g = NULL;
	usbg_function *f;
	const gchar *msg = NULL;
	GadgetFunctionManager *func_manager = GADGET_FUNCTION_MANAGER(object);
	GadgetdFunctionObject *function_object;
	GadgetDaemon *daemon;

	daemon = gadget_function_manager_get_daemon(GADGET_FUNCTION_MANAGER(object));

	/* TODO add create function handler */
	INFO("handled create function");

	if (!instance)
		return FALSE;

	if (!get_function_type(_str_type, &type)) {
		msg = "Invalid function type";
		goto err;
	}

	if (func_manager->gadget_name == NULL) {
		msg = "Unknown gadget name";
		goto err;
	}

	function_path = g_strdup_printf("%s/%s/%s/%s",
					gadgetd_path,
					func_manager->gadget_name,
					_str_type,
					instance);

	if (function_path == NULL || !g_variant_is_object_path(function_path)) {
		msg = "Invalid function instance or type";
		goto err;
	}

	g = usbg_get_gadget(ctx.state, func_manager->gadget_name);
	if (g == NULL) {
		msg = "Cant get a gadget device by name";
		goto err;
	}

	usbg_ret = usbg_create_function(g,
					type,
					instance,
					NULL,
					&f);

	if (usbg_ret != USBG_SUCCESS) {
		ERROR("Error on function create");
		ERROR("Error: %s: %s", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		msg = usbg_error_name(usbg_ret);
		goto err;
	}

	function_object = gadgetd_function_object_new(func_manager->gadget_name,
						instance,
						_str_type);
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
	return FALSE;
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
		      const gchar				*function_name)
{
	/* TODO add find function by name handler*/
	INFO("find function by name handler");

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

