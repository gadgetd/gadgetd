/*
 * gadget-config.c
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
#include <dbus-config-ifaces/gadget-config.h>

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

struct _GadgetConfig
{
	GadgetdGadgetConfigSkeleton parent_instance;
	GadgetdConfigObject *cfg_object;
};

struct _GadgetConfigClass
{
	GadgetdGadgetConfigSkeletonClass parent_class;
};

enum
{
	PROP_0,
	PROP_CFG_LABEL,
	PROP_CFG_ID,
	PROP_CFG_OBJ
} prop_config_iface;

static void gadget_config_iface_init(GadgetdGadgetConfigIface *iface);

/**
 * @brief G_DEFINE_TYPE_WITH_CODE
 * @details A convenience macro for type implementations. Similar to G_DEFINE_TYPE(), but allows
 * to insert custom code into the *_get_type() function,
 * @see G_DEFINE_TYPE()
 */
G_DEFINE_TYPE_WITH_CODE(GadgetConfig, gadget_config, GADGETD_TYPE_GADGET_CONFIG_SKELETON,
			 G_IMPLEMENT_INTERFACE(GADGETD_TYPE_GADGET_CONFIG, gadget_config_iface_init));

/**
 * @brief gadget config set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 * @see GObjectSetPropertyFunc()
 */
static void
gadget_config_set_property(GObject *object, guint property_id,
			   const GValue *value, GParamSpec *pspec)
{
	GadgetConfig *gadget_config = GADGET_CONFIG(object);

	switch(property_id) {
	case PROP_CFG_OBJ:
		g_assert(gadget_config->cfg_object == NULL);
		gadget_config->cfg_object = g_value_get_object(value);
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
gadget_config_get_property(GObject *object, guint property_id, GValue *value,
			   GParamSpec *pspec)
{
	GadgetConfig *gadget_config = GADGET_CONFIG(object);
	int id;
	const gchar *label;
	usbg_config *cfg;

	cfg = gadgetd_config_object_get_config(gadget_config->cfg_object);

	if (cfg == NULL)
		return;

	switch(property_id) {
	case PROP_CFG_LABEL:
		label = usbg_get_config_label(cfg);
		if (label == NULL) {
			ERROR("Cant get function label");
			return;
		}
		g_value_set_string(value, label);
		break;
	case PROP_CFG_ID:
		id = usbg_get_config_id(cfg);
		g_value_set_int(value, id);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadget config finalize
 * @param[in] object GObject
 */
static void
gadget_config_finalize(GObject *object)
{
	if (G_OBJECT_CLASS(gadget_config_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadget_config_parent_class)->finalize(object);
}

/**
 * @brief gadget config class init
 * @param[in] klass GadgetConfigClass
 */
static void
gadget_config_class_init(GadgetConfigClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gadget_config_set_property;
	gobject_class->get_property = gadget_config_get_property;
	gobject_class->finalize = gadget_config_finalize;

	g_object_class_override_property(gobject_class,
					PROP_CFG_ID,
					"id");

	g_object_class_override_property(gobject_class,
					PROP_CFG_LABEL,
					"label");

	g_object_class_install_property(gobject_class,
                                   PROP_CFG_OBJ,
                                   g_param_spec_object("config-object",
                                                       "config object",
                                                       "Config object",
                                                       GADGETD_TYPE_CONFIG_OBJECT,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));
}

/**
 * @brief gadget config new
 * @param[in] connection A #GDBusConnection
 * @return #GadgetConfig object.
 */
GadgetConfig *
gadget_config_new(GadgetdConfigObject *cfg_object)
{
	g_return_val_if_fail(cfg_object != NULL, NULL);
	GadgetConfig *object;

	object = g_object_new(GADGET_TYPE_CONFIG,
			     "config-object", cfg_object,
			      NULL);
	return object;
}

/**
 * @brief gadget config init
 */
static void
gadget_config_init(GadgetConfig *gadget_config)
{
	/* noop */
}

/**
 * @brief attach function
 * @param[in] object
 * @param[in] invocation
 * @param[in] function_path
 * @return true if metod handled
 */
static gboolean
handle_attach_function(GadgetdGadgetConfig	*object,
			GDBusMethodInvocation	*invocation,
			const gchar *function_path)
{
	/*TODO attach function*/
	INFO("attach function handler");

	return TRUE;
}

/**
 * @brief gadget config iface init
 * @param[in] iface GadgetdGadgetManagerIface
 */
static void
gadget_config_iface_init(GadgetdGadgetConfigIface *iface)
{
	iface->handle_attach_function = handle_attach_function;
}

