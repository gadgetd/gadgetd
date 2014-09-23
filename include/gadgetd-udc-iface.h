/*
 * gadgetd-udc-iface.h
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

#ifndef GADGETD_UDC_IFACE_H
#define GADGETD_UDC_IFACE_H

#include <glib-object.h>
#include <gio/gio.h>
#include <usbg/usbg.h>

#include <gadgetd-udc-object.h>
#include <gadgetd-config-object.h>

G_BEGIN_DECLS

struct _GadgetdUDCDevice;
typedef struct _GadgetdUDCDevice GadgetdUDCDevice;

typedef struct _GadgetdUDCDeviceClass	GadgetdUDCDeviceClass;

#define GADGETD_TYPE_UDC_DEVICE  (gadgetd_udc_device_get_type ())
#define GADGETD_UDC_DEVICE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGETD_TYPE_UDC_DEVICE, GadgetdUDCDevice))
#define GADGETD_IS_UDC_DEVICE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGETD_TYPE_UDC_DEVICE))

GType gadgetd_udc_device_get_type (void) G_GNUC_CONST;
GadgetdUDCDevice   *gadgetd_udc_device_new(GadgetdUdcObject *udc_object);
G_END_DECLS

#endif /* GADGETD_UDC_IFACE_H */
