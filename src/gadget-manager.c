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
parse_strings(GVariant *strings, usbg_gadget *g)
{
	GVariant *value;
	GVariantIter *iter;
	const gchar *key;
	const gchar *str;
	gint i;
	gint count = 0;

	g_variant_get(strings, "a{sv}", &iter);

	static const struct {
		char* name;
		int (*gchar_func)(usbg_gadget *,int, const gchar *str);
	} strs[] = {
		{ "serialnumber", usbg_set_gadget_serial_number       },
		{ "manufacturer", usbg_set_gadget_manufacturer        },
		{ "product",      usbg_set_gadget_product             }
	};

	while(g_variant_iter_next(iter, "{&sv}", &key, &value)) {
		count++;
		if (count > G_N_ELEMENTS(strs))
			goto out;
		for (i = 0; i < G_N_ELEMENTS(strs); i++) {
			if (g_strcmp0(key, strs[i].name) == 0) {
				if (!g_variant_is_of_type(value, G_VARIANT_TYPE("s")))
					goto out;
				str = g_variant_get_string(value, NULL);
				/* actually strings available only in en_US lang */
				strs[i].gchar_func(g, LANG_US_ENG, str);
				g_variant_unref(value);
				break;
			}
		}
		if (i == G_N_ELEMENTS(strs))
			goto out;
	}
	g_variant_iter_free(iter);

	return TRUE;
out:
	g_variant_iter_free(iter);
	return FALSE;
}

/**
 * @brief parse descriptors
 * @param[in] descriptors
 * @return true if parsed
 */
static gboolean
parse_descriptors(GVariant *descriptors, usbg_gadget *g)
{
	GVariant *value;
	GVariantIter *iter;
	const gchar *key;
	gint i;
	int count = 0;

	g_variant_get(descriptors, "a{sv}", &iter);

	static const struct {
		char* name;
		int  type;
		int (*guint8_func)(usbg_gadget *,  uint8_t);
		int (*guint16_func)(usbg_gadget *, uint16_t);
	} descs[] = {
		{ "bcdUSB",          'q', NULL, usbg_set_gadget_device_bcd_usb     },
		{ "bDeviceClass",    'y', usbg_set_gadget_device_class, NULL       },
		{ "bDeviceSubClass", 'y', usbg_set_gadget_device_subclass, NULL    },
		{ "bDeviceProtocol", 'y', usbg_set_gadget_device_protocol, NULL    },
		{ "bMaxPacketSize0", 'y', usbg_set_gadget_device_max_packet, NULL  },
		{ "idVendor",        'q', NULL, usbg_set_gadget_vendor_id          },
		{ "idProduct",       'q', NULL, usbg_set_gadget_product_id         },
		{ "bcdDevice",       'q', NULL, usbg_set_gadget_device_bcd_device  }
	};

	while(g_variant_iter_next(iter, "{&sv}", &key, &value)) {
		count++;
		if (count > G_N_ELEMENTS(descs))
			goto out;
		for (i = 0; i < G_N_ELEMENTS(descs); i++) {
			if (g_strcmp0(key, descs[i].name) == 0) {
				switch(descs[i].type) {
				case 'q':
					if (!g_variant_is_of_type(value, G_VARIANT_TYPE("q")))
						goto out;
					guint16 val16 = g_variant_get_uint16(value);
					descs[i].guint16_func(g, val16);
					break;
				case 'y':
					if (!g_variant_is_of_type(value, G_VARIANT_TYPE("y")))
						goto out;
					guint8 val8 = g_variant_get_byte(value);
					descs[i].guint8_func(g, val8);
					break;
				default:
					goto out;
					break;
				}
				g_variant_unref(value);
				break;
			}
		}
		if (i == G_N_ELEMENTS(descs))
			goto out;
	}
	g_variant_iter_free(iter);

	return TRUE;
out:
	g_variant_iter_free(iter);
	return FALSE;
}

static gboolean
get_data(GVariant *descriptors, GVariant *strings, usbg_gadget *g)
{
	/*descriptors*/
	if (descriptors != NULL) {
		if (parse_descriptors(descriptors, g) == TRUE)
			DEBUG("parse descriptors\n");
		else
			return FALSE;
	}
	/*strings*/
	if (strings != NULL) {
		if (parse_strings(strings, g) == TRUE)
			DEBUG("parse strings\n");
		else
			return FALSE;
	}

	return TRUE;
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
	gint usbg_ret = USBG_SUCCESS;
	usbg_gadget *g;
	const char *msg = "Unknown error";
	gchar *path;

	INFO("handled create gadget \n");

	path = g_strdup_printf("%s/%s", gadgetd_path, gadget_name);

	if (path == NULL || !g_variant_is_object_path(path)) {
		msg = "Invalid gadget name";
		goto out;
	}

	/* CREATE GADGET()*/
	usbg_ret = usbg_create_gadget(ctx.state,
				   gadget_name,
				   NULL,
				   NULL,
				   &(g));

	if (usbg_ret != USBG_SUCCESS) {
		ERROR("Error on create gadget\n");
		ERROR("Error: %s : %s\n", usbg_error_name(g_ret),
			usbg_strerror(g_ret));
		msg = usbg_error_name(g_ret);
		goto out;
	}

	/*get strings and descriptors*/
	g_ret = get_data(descriptors, strings, g);
	if (!g_ret) {
		ERROR("Invalid argument\n");
		usbg_rm_gadget(g, USBG_RM_RECURSE);
		msg = "Invalid strings or descriptors argument";
		goto out;
	}

	/* send gadget path*/
	g_dbus_method_invocation_return_value(invocation,
					      g_variant_new("(o)", path));
	g_free(path);

	return TRUE;

out:
	g_dbus_method_invocation_return_dbus_error(invocation,
			manager_iface,
			msg);
	g_free(path);
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
