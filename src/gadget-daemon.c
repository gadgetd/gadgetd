/*
 * gadgetd-daemon.c
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

#include <stdio.h>

#include <gio/gio.h>
#include <glib/gprintf.h>

#include <gadgetd-config.h>
#include <gadgetd-create.h>
#include <gadgetd-common.h>

#include <gadgetd-gdbus-codegen.h>
#include <gadget-daemon.h>
#include <gadget-manager.h>
#include <gadgetd-udc-object.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

/**
 * @brief Gadget daemon address on the dbus bus
 */
static const char gadgetd_service[] = "org.usb.gadgetd";
const char gadgetd_path[] = "/org/usb/Gadget";

static GadgetDaemon *gadget_daemon = NULL;

/**
 * @brief GadgetDaemon struct
 * @details This structure contains only private data and should
 * only be accessed using the provided API.
 */
struct _GadgetDaemon
{
	GObject parent_instance;
	GDBusConnection *connection;
	GDBusObjectManagerServer *object_manager;

	GadgetdObjectSkeleton *gadget_manager;
};

struct _GadgetDaemonClass
{
	GObjectClass parent_class;
};

enum
{
	PROPERTY,
	PROPERTY_CONNECTION,
	PROPERTY_OBJECT_MANAGER
};

/**
 * @brief GadgetDaemon G_DEFINE_TYPE
 * @details Macro which declares a class initialization function
 * @param[in] GadgetDaemon new type name in camelcase
 * @param[in] gadget_daemon name in lowercase type with _ as
 * word separator
 * @param[in] G_TYPE_OBJECT fundamental type for GObject
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE(GadgetDaemon, gadget_daemon, G_TYPE_OBJECT);

/**
 * @brief Set property func
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
gadget_daemon_set_property(GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GadgetDaemon *daemon = GADGET_DAEMON(object);

	switch(property_id) {
	case PROPERTY_CONNECTION:
		g_assert(daemon->connection == NULL);
		daemon->connection = g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief Create a new daemon object for exporting objects on @connection.
 * @param[in] connection A #GDBusConnection
 * @return #GadgetDaemon object.
 */
GadgetDaemon *
gadget_daemon_new(GDBusConnection *connection)
{
	g_return_val_if_fail(G_IS_DBUS_CONNECTION(connection), NULL);
	return GADGET_DAEMON(g_object_new(GADGET_TYPE_DAEMON,
					    "connection",
					    connection,
					    NULL));
}


/**
 * @brief get connection.
 * @param[in] daemon A #GadgetDaemon.
 * @return GDBusConnection Do not free, the object is owned by @daemon.
 */
GDBusConnection *
gadget_daemon_get_connection(GadgetDaemon *daemon)
{
	g_return_val_if_fail(GADGET_IS_DAEMON(daemon), NULL);
	return daemon->connection;
}

/**
 * @brief get object manager
 * @details Get the D-Bus object manager used by daemon
 * @param[in] daemon #GadgetDaemon
 * @return #GDBusObjectManagerServer
 */
GDBusObjectManagerServer *
gadget_daemon_get_object_manager(GadgetDaemon *daemon)
{
	g_return_val_if_fail(GADGET_IS_DAEMON(daemon), NULL);
	return daemon->object_manager;
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
gadget_daemon_get_property(GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GadgetDaemon *daemon = GADGET_DAEMON(object);

	switch(property_id) {
	case PROPERTY_CONNECTION:
		g_value_set_object(value, gadget_daemon_get_connection(daemon));
		break;
	case PROPERTY_OBJECT_MANAGER:
		g_value_set_object(value, gadget_daemon_get_object_manager(daemon));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	break;
	}
}

/**
 * @brief daemon constructed
 * @details called by g_object_new() in the final step of the object creation process
 * @param[in] object GObject
 */
static void
gadget_daemon_constructed(GObject *object)
{
	GadgetDaemon *daemon = GADGET_DAEMON(object);
	GadgetdGadgetManager *gadget_manager;
	GDBusConnection *connection;
	GadgetdUdcObject *udc_object;
	GList *l;

	daemon->object_manager = g_dbus_object_manager_server_new(gadgetd_path);

	connection = gadget_daemon_get_connection(daemon);

	/* export object manager */
	g_dbus_object_manager_server_set_connection(daemon->object_manager, daemon->connection);

	/* add gadget manager */
	gadget_manager = gadget_manager_new(daemon);
	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(gadget_manager),
					 connection,
					 gadgetd_path,
					 NULL);

	/* create dbus udc objects */
	for (l = gd_udcs; l != NULL; l = l->next) {
		udc_object = gadgetd_udc_object_new((usbg_udc *)(l->data), daemon);
		g_dbus_object_manager_server_export(gadget_daemon_get_object_manager(daemon),
					    G_DBUS_OBJECT_SKELETON(udc_object));
	}

	if (G_OBJECT_CLASS(gadget_daemon_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(gadget_daemon_parent_class)->constructed(object);
}

/**
 * @brief daemon finalize
 * @param[in] object GObject
 */
static void
gadget_daemon_finalize(GObject *object)
{
	GadgetDaemon *daemon = GADGET_DAEMON(object);

	g_object_unref(daemon->connection);

	if (G_OBJECT_CLASS(gadget_daemon_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadget_daemon_parent_class)->finalize(object);
}

/**
 * @brief gadget daemon class init
 * @param[in] klass GadgetDaemonClass
 */
static void
gadget_daemon_class_init(GadgetDaemonClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize     = gadget_daemon_finalize;
	gobject_class->constructed  = gadget_daemon_constructed;
	gobject_class->set_property = gadget_daemon_set_property;
	gobject_class->get_property = gadget_daemon_get_property;

	g_object_class_install_property(gobject_class,
                                   PROPERTY_CONNECTION,
                                   g_param_spec_object("connection",
                                                        "Connection",
                                                        "connection for daemon",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class,
                                   PROPERTY_OBJECT_MANAGER,
                                   g_param_spec_object("object-manager",
                                                        "Object Manager",
                                                        "Object Manager server for daemon",
                                                        G_TYPE_DBUS_OBJECT_MANAGER_SERVER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}

/**
 * @brief daemon init
 * @param[in] daemon GadgetDaemon struct
 */
static void
gadget_daemon_init(GadgetDaemon *daemon)
{
}

/**
 * @brief bus acquired callback
 * @details called when connection to a message base has been done
 * @param[in] connection GDBusConnection to a message bus
 * @param[in] name Name to be owned
 * @param[in] user_data User data to pass
 */
static void
on_bus_acquired(GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
	gadget_daemon = gadget_daemon_new(connection);
	INFO("Connected to the system bus");
}

/**
 * @brief name acquired callback
 * @param[in] connection GDBusConnection on which acquie the name
 * @param[in] name Name to be owned
 * @param[in] user_data User data to pass
 */
static void
on_name_acquired(GDBusConnection *connection,
		  const gchar     *name,
		  gpointer         user_data)
{
	INFO("Acquired the name %s", name);

}

/**
 * @brief name lost callback
 * @details when lost the name or connection
 * @param[in] connection GDBusConnection on which acquie the name
 * @param[in] name Name to be owned
 * @param[in] user_data User data to pass
 */
static void
on_name_lost(GDBusConnection *connection,
	      const gchar   *name,
	      gpointer       user_data)
{
	INFO("Lost the name %s", name);
}

/**
 * @brief gadget daemon run
 * @details Start the gadget daemon
 * @return GD_SUCCESS if success
 */
int
gadget_daemon_run(void)
{
	GMainLoop *loop;
	guint id;

	loop = g_main_loop_new(NULL, FALSE);

	g_type_init();

	id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
			gadgetd_service,
			G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
			G_BUS_NAME_OWNER_FLAGS_REPLACE,
			on_bus_acquired,
			on_name_acquired,
			on_name_lost,
			loop,
			NULL);

	g_main_loop_run(loop);

	g_bus_unown_name(id);
	g_main_loop_unref(loop);

	return GD_SUCCESS;
}
