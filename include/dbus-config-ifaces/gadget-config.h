/*
 * gadgett-config.h
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
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

#ifndef GADGET_CONFIG_H
#define GADGET_CONFIG_H

#include <glib-object.h>
#include <gio/gio.h>
#include <usbg/usbg.h>

#include <gadgetd-config-object.h>

G_BEGIN_DECLS

struct _GadgetConfig;
typedef struct _GadgetConfig GadgetConfig;

typedef struct _GadgetConfigClass	GadgetConfigClass;

#define GADGET_TYPE_CONFIG      (gadget_config_get_type ())
#define GADGET_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGET_TYPE_CONFIG, GadgetConfig))
#define GADGET_IS_CONFIG(o)     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGET_TYPE_CONFIG))

GType gadget_config_get_type (void) G_GNUC_CONST;
GadgetConfig *gadget_config_new(usbg_config *cfg);
G_END_DECLS

#endif /* GADGET_CONFIG_H */
