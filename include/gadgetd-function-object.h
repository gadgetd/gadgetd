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

#ifndef GADGETD_FUNCTION_OBJECT_H
#define GADGETD_FUNCTION_OBJECT_H

#include <gadget-daemon.h>
#include "gadget-manager.h"
#include <usbg/usbg.h>

G_BEGIN_DECLS

struct _GadgetdFunctionObject;
typedef struct _GadgetdFunctionObject GadgetdFunctionObject;

#define GADGETD_TYPE_FUNCTION_OBJECT         (gadgetd_function_object_get_type ())
#define GADGETD_FUNCTION_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGETD_TYPE_FUNCTION_OBJECT, GadgetdFunctionObject))
#define GADGETD_IS_FUNCTION_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGETD_TYPE_FUNCTION_OBJECT))

GType                  gadgetd_function_object_get_type (void) G_GNUC_CONST;
GadgetdFunctionObject *gadgetd_function_object_new      (const gchar *function_path,
							 const gchar *instance,
							/* str_type: Type of function as a string.
							   Can be one of:
							   - Kernel functions (acm, ecm, ...)
							   - Userspace functions implemented
							   using FunctitonFS (ffs.sdb, ffs.mtp)
							*/
							 const gchar *_str_type,
							 usbg_function *f);
usbg_function *gadgetd_function_object_get_function(GadgetdFunctionObject *function_object);

G_END_DECLS

#endif /* GADGETD_FUNCTION_OBJECT_H */
