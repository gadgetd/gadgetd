/*
 * gadgetd-serial-function-iface.h
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

#ifndef GADGETD_SERIAL_FUNCTION_IFACE_H
#define GADGETD_SERIAL_FUNCTION_IFACE_H

#include <glib-object.h>
#include <gio/gio.h>
#include <usbg/usbg.h>

#include <gadgetd-function-object.h>

G_BEGIN_DECLS

struct _FunctionSerialAttrs;
typedef struct _FunctionSerialAttrs FunctionSerialAttrs;

typedef struct _FunctionSerialAttrsClass	FunctionSerialAttrsClass;

#define FUNCTION_TYPE_SERIAL_ATTRS      (function_serial_attrs_get_type ())
#define FUNCTION_SERIAL_ATTRS(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), FUNCTION_TYPE_SERIAL_ATTRS, FunctionSerialAttrs))
#define FUNCTION_IS_SERIAL_ATTRS(o)     (G_TYPE_CHECK_INSTANCE_TYPE ((o), FUNCTION_TYPE_SERIAL_ATTRS))

GType function_serial_attrs_get_type (void) G_GNUC_CONST;
FunctionSerialAttrs *function_serial_attrs_new(GadgetdFunctionObject *function_object);
G_END_DECLS

#endif /* GADGETD_SERIAL_FUNCTION_IFACE_H */
