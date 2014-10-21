/*
 * gadget-config-manager.h
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef GADGET_CONFIG_MANAGER_H
#define GADGET_CONFIG_MANAGER_H

#include <glib-object.h>
#include <gio/gio.h>

#include <gadget-manager.h>
#include <gadgetd-common.h>
#include <gadgetd-core.h>

G_BEGIN_DECLS

struct _GadgetConfigManager;
typedef struct _GadgetConfigManager GadgetConfigManager;

#define GADGET_TYPE_CONFIG_MANAGER	(gadget_config_manager_get_type ())
#define GADGET_CONFIG_MANAGER(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GADGET_TYPE_CONFIG_MANAGER, GadgetConfigManager))
#define GADGET_IS_CONFIG_MANAGER(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGET_TYPE_CONFIG_MANAGER))

GType                gadget_config_manager_get_type (void) G_GNUC_CONST;
GadgetConfigManager *gadget_config_manager_new      (GadgetDaemon *daemon,
						     const gchar *gadget_path,
						     struct gd_gadget *gadget);

G_END_DECLS

#endif /* GADGET_CONFIG_MANAGER_H */
