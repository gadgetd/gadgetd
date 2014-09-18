/*
 * gadgetd-config-object.h
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

#ifndef GADGETD_CONFIG_OBJECT_H
#define GADGETD_CONFIG_OBJECT_H

#include <gadget-daemon.h>
#include "gadget-manager.h"

G_BEGIN_DECLS

struct _GadgetdConfigObject;
typedef struct _GadgetdConfigObject GadgetdConfigObject;

#define GADGETD_TYPE_CONFIG_OBJECT         (gadgetd_config_object_get_type ())
#define GADGETD_CONFIG_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGETD_TYPE_CONFIG_OBJECT, GadgetdConfigObject))
#define GADGETD_IS_CONFIG_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGETD_TYPE_CONFIG_OBJECT))

GType                gadgetd_config_object_get_type (void) G_GNUC_CONST;
GadgetdConfigObject *gadgetd_config_object_new(const gchar *gadget_name, gint config_id,
					       const gchar *config_label);

G_END_DECLS

#endif /* GADGETD_CONFIG_OBJECT_H */
