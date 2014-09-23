/*
 * gadgetd-function-object.h
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

#ifndef GADGETD_UDC_OBJECT_H
#define GADGETD_UDC_OBJECT_H

#include <gadget-daemon.h>
#include "gadget-manager.h"
#include <usbg/usbg.h>

G_BEGIN_DECLS

struct _GadgetdUdcObject;
typedef struct _GadgetdUdcObject GadgetdUdcObject;

#define GADGETD_TYPE_UDC_OBJECT         (gadgetd_udc_object_get_type ())
#define GADGETD_UDC_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGETD_TYPE_UDC_OBJECT, GadgetdUdcObject))
#define GADGETD_IS_UDC_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGETD_TYPE_UDC_OBJECT))

GType            gadgetd_udc_object_get_type (void) G_GNUC_CONST;
GadgetdUdcObject *gadgetd_udc_object_new     (usbg_udc *u);
usbg_udc         *gadgetd_udc_object_get_udc(GadgetdUdcObject *object);
gchar            *gadgetd_udc_object_get_enabled_gadget_path(GadgetdUdcObject *object);
gint              gadgetd_udc_object_set_enabled_gadget_path(GadgetdUdcObject *object, const gchar *path);

G_END_DECLS

#endif /* GADGETD_UDC_OBJECT_H */
