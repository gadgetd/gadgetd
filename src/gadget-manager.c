/*
 * gadgetd-manager.c
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

#include <gio/gio.h>
#include <glib/gprintf.h>

#include <gadget-daemon.h>
#include "gadgetd-gdbus-codegen.h"
#include <gadget-manager.h>
#include <gadgetd-config.h>
#include <usbg/usbg.h>
#include <gadgetd-common.h>
#include <gadgetd-gadget-object.h>


typedef struct _GadgetManagerClass   GadgetManagerClass;

struct _GadgetManager
{
	GadgetdGadgetManagerSkeleton parent_instance;

	GadgetDaemon *daemon;

	GadgetdObjectSkeleton *manager_object;
};

struct _GadgetManagerClass
{
	GadgetdGadgetManagerSkeletonClass parent_class;
};

enum
{
	PROPERTY,
	PROPERTY_DAEMON,
};

static const char manager_iface[] = "org.usb.device.GadgetManager";

static void gadget_manager_iface_init(GadgetdGadgetManagerIface *iface);

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetManager, gadget_manager, GADGETD_TYPE_GADGET_MANAGER_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_MANAGER, gadget_manager_iface_init));

/**
 * @brief gadget manager finalize
 * @param[in] object GObject
 */
static void
gadget_manager_finalize(GObject *object)
{
	G_OBJECT_CLASS(gadget_manager_parent_class)->finalize(object);
}

/**
 * @brief gadget manager get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 */
static void
gadget_manager_get_property(GObject          *object,
			     guint             property_id,
			     GValue           *value,
			     GParamSpec       *pspec)
{
	GadgetManager *manager = GADGET_MANAGER(object);

	switch(property_id) {
	case PROPERTY_DAEMON:
		g_value_set_object(value, gadget_manager_get_daemon(manager));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget manager Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
 void
gadget_manager_set_property(GObject            *object,
			     guint               property_id,
			     const GValue       *value,
			     GParamSpec         *pspec)
{
	GadgetManager *manager = GADGET_MANAGER(object);

	switch(property_id) {
	case PROPERTY_DAEMON:
		g_assert(manager->daemon == NULL);
		manager->daemon = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget manager init
 * @param[in] klass #GadgetManager
 */
static void
gadget_manager_init(GadgetManager *manager)
{
	/*TODO use mutex ? g_mutex_init(&(manager->lock)); */

	g_dbus_interface_skeleton_set_flags(G_DBUS_INTERFACE_SKELETON(manager),

	G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);
}

/**
 * @brief gadget manager class init
 * @param[in] klass #GadgetManagerClass
 */
static void
gadget_manager_class_init(GadgetManagerClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize     = gadget_manager_finalize;
	gobject_class->set_property = gadget_manager_set_property;
	gobject_class->get_property = gadget_manager_get_property;

	g_object_class_install_property(gobject_class,
                                   PROPERTY_DAEMON,
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
 * @brief Create a new #GadgetManager instance
 * @param[in] daemon #GadgetDaemon
 * @return GadgetManager object.
 */
GadgetdGadgetManager *
gadget_manager_new(GadgetDaemon *daemon)
{
	g_return_val_if_fail(GADGET_IS_DAEMON(daemon), NULL);

	return GADGETD_GADGET_MANAGER(g_object_new(GADGET_TYPE_MANAGER,
				      "daemon",
				       daemon,
				       NULL));
}

/**
 * @brief Gets the daemon used by @manager
 * @param[in] manager GadgetManager
 * @return GadgetDaemon object, dont free used by @manager
 */
GadgetDaemon *
gadget_manager_get_daemon(GadgetManager *manager)
{
	g_return_val_if_fail(GADGET_IS_MANAGER(manager), NULL);
	return manager->daemon;
}

/**
 * @brief Parse strings
 * @param[in] strings GVariant
 * @return true if parsed
 */
static gboolean
strings_set(GVariant *strings, usbg_gadget *g)
{
	GVariant *value;
	GVariantIter *iter;
	const gchar *key;
	gboolean found = TRUE;
	int usbg_ret;
	int i;

	static const struct {
		char *name;
		int (*s_func)(usbg_gadget *, int, const char *str);
	} strs[] = {
		{ "serialnumber", usbg_set_gadget_serial_number       },
		{ "manufacturer", usbg_set_gadget_manufacturer        },
		{ "product",      usbg_set_gadget_product             }
	};

	g_variant_get(strings, "a{sv}", &iter);

	if (g_variant_iter_n_children(iter) == 0) {
		/* Ensure that at least one string is set when creating gadget.
		 *
		 * Kernel driver provides set of strings (manufacturer, serial,
		 * product) to USB host only if at least one of them has been
		 * set. In gadgetd we follow the most popular convention that
		 * each USB device should provide strings in English (US).
		 *
		 * If user didn't provide value for any of strings we create
		 * empty one. This is information for driver that this gadget
		 * provides strings in English and all three are empty.
		 */
		usbg_ret = strs[0].s_func(g, LANG_US_ENG, "");
		if (usbg_ret != USBG_SUCCESS) {
			ERROR("Unable to set string '%s': %s", key, usbg_error_name(usbg_ret));
			goto out;
		}
	}

	while (g_variant_iter_next(iter, "{&sv}", &key, &value)) {
		for (found = FALSE, i = 0; i < G_N_ELEMENTS(strs); i++) {
			if (g_strcmp0(key, strs[i].name) != 0)
				continue;

			if (!g_variant_is_of_type(value, G_VARIANT_TYPE("s")))
				goto out;

			/* strings are currently available en_US locale only */
			usbg_ret = strs[i].s_func(g, LANG_US_ENG, g_variant_get_string(value, NULL));
			if (usbg_ret != USBG_SUCCESS) {
				ERROR("Unable to set string '%s': %s", key, usbg_error_name(usbg_ret));
				goto out;
			}

			g_variant_unref(value);
			found = TRUE;
			break;
		}
		if (!found)
			goto out;
	}

out:
	if (!found) {
		ERROR("Provided string '%s' is invalid", key);
		g_variant_unref(value);
	}
	g_variant_iter_free(iter);
	return found;
}

/**
 * @brief parse descriptors
 * @param[in] descriptors
 * @return true if parsed
 */
static gboolean
descriptors_set(GVariant *descriptors, usbg_gadget *g)
{
	GVariant *value;
	GVariantIter *iter;
	const gchar *key;
	gboolean found = TRUE;
	int usbg_ret;
	int i;

	static const struct {
		char *name;
		char *type;
		int (*y_func)(usbg_gadget *,  uint8_t);
		int (*q_func)(usbg_gadget *, uint16_t);
	} descs[] = {
		{ "bcdUSB",          "q", NULL, usbg_set_gadget_device_bcd_usb     },
		{ "bDeviceClass",    "y", usbg_set_gadget_device_class, NULL       },
		{ "bDeviceSubClass", "y", usbg_set_gadget_device_subclass, NULL    },
		{ "bDeviceProtocol", "y", usbg_set_gadget_device_protocol, NULL    },
		{ "bMaxPacketSize0", "y", usbg_set_gadget_device_max_packet, NULL  },
		{ "idVendor",        "q", NULL, usbg_set_gadget_vendor_id          },
		{ "idProduct",       "q", NULL, usbg_set_gadget_product_id         },
		{ "bcdDevice",       "q", NULL, usbg_set_gadget_device_bcd_device  }
	};

	g_variant_get(descriptors, "a{sv}", &iter);

	while (g_variant_iter_next(iter, "{&sv}", &key, &value)) {
		for (found = FALSE, i = 0; i < G_N_ELEMENTS(descs); i++) {
			if (g_strcmp0(key, descs[i].name) != 0)
				continue;

			if (!g_variant_is_of_type(value, G_VARIANT_TYPE(descs[i].type)))
				goto out;

			if (descs[i].type[0] == 'q')
				usbg_ret = descs[i].q_func(g, g_variant_get_uint16(value));
			else if (descs[i].type[0] == 'y')
				usbg_ret = descs[i].y_func(g, g_variant_get_byte(value));
			else
				goto out;

			if (usbg_ret != USBG_SUCCESS) {
				ERROR("Unable to set descriptor '%s': %s", key, usbg_error_name(usbg_ret));
				goto out;
			}

			g_variant_unref(value);
			found = TRUE;
			break;
		}
		if (!found)
			goto out;
	}

out:
	if (!found) {
		ERROR("Provided descriptor '%s' is invalid", key);
		g_variant_unref(value);
	}
	g_variant_iter_free(iter);
	return found;
}

/**
 * @brief Create gadget handler
 * @param[in] object GadgetdGadgetManager object
 * @param[in] invocation
 * @param[in] gadget_name name of the gadget
 * @param[in] descriptors gadget descriptors
 * @param[in] strings gadget strings
 * @return true if metod handled
 */
static gboolean
handle_create_gadget(GadgetdGadgetManager	*object,
		      GDBusMethodInvocation	*invocation,
		      const gchar		*gadget_name,
		      GVariant			*descriptors,
		      GVariant			*strings)
{
	gint g_ret = 0;
	int usbg_ret = USBG_SUCCESS;
	usbg_gadget *g;
	const char *msg = "Unknown error";
	_cleanup_g_free_ gchar *path = NULL;
	GadgetdGadgetObject *gadget_object;
	GadgetDaemon *daemon;

	daemon = gadget_manager_get_daemon(GADGET_MANAGER(object));

	INFO("handled create gadget");

	if (g_strcmp0(gadget_name, "") == 0
	    || (path = g_strdup_printf("%s/%s", gadgetd_path, gadget_name)) == NULL
	    || !g_variant_is_object_path(path)) {

		msg = "Unable to construct valid object path using provided gadget_name";
		goto err;
	}

	/* CREATE GADGET()*/
	usbg_ret = usbg_create_gadget(ctx.state,
				   gadget_name,
				   NULL,
				   NULL,
				   &(g));

	if (usbg_ret != USBG_SUCCESS) {
		msg = usbg_error_name(usbg_ret);
		goto err;
	}

	/* set strings and descriptors */
	g_ret = descriptors_set(descriptors, g) && strings_set(strings, g);
	if (!g_ret) {
		usbg_rm_gadget(g, USBG_RM_RECURSE);
		msg = "Invalid strings or descriptors argument";
		goto err;
	}

	/* create dbus gadget object */
	gadget_object = gadgetd_gadget_object_new(daemon, gadget_name);
	g_dbus_object_manager_server_export(gadget_daemon_get_object_manager(daemon),
					    G_DBUS_OBJECT_SKELETON(gadget_object));

	/* send gadget path */
	g_dbus_method_invocation_return_value(invocation,
					      g_variant_new("(o)", path));

	return TRUE;

err:
	ERROR("%s", msg);
	g_dbus_method_invocation_return_dbus_error(invocation,
			manager_iface,
			msg);
	return FALSE;
}

/**
 * @brief remove gadget handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] gadget_path
 * @return true if metod handled
 */
static gboolean
handle_remove_gadget(GadgetdGadgetManager	*object,
		      GDBusMethodInvocation	*invocation,
		      const gchar *gadget_path)
{
	/*TODO handle remove gadget*/
	INFO("remove gadget handler");

	return TRUE;
}

/**
 * @brief find adget by name handler
 * @param[in] object
 * @param[in] invocation
 * @param[in] gadget_name
 * @return true if metod handled
 */
static gboolean
handle_find_gadget_by_name(GadgetdGadgetManager	*object,
			    GDBusMethodInvocation	*invocation,
			    const gchar 		*gadget_name)
{
	/*TODO handle find gadget by name*/
	INFO("find gadget by name handler");

	return TRUE;
}

/**
 * @brief list avaliable functions
 * @param[in] object
 * @param[in] invocation
 * @return true if metod handled
 */
static gboolean
handle_list_avaliable_functions(GadgetdGadgetManager	*object,
			        GDBusMethodInvocation	*invocation)
{
	/*TODO handle list functions*/
	INFO("list avaliable functions handler");

	return TRUE;
}

/**
 * @brief gadget manager iface init
 * @param[in] iface GadgetdGadgetManagerIface
 */
static void
gadget_manager_iface_init(GadgetdGadgetManagerIface *iface)
{
	iface->handle_create_gadget = handle_create_gadget;
	iface->handle_remove_gadget = handle_remove_gadget;
	iface->handle_find_gadget_by_name = handle_find_gadget_by_name;
	iface->handle_list_avaliable_functions = handle_list_avaliable_functions;
}
