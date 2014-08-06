/*
 * gadget-function-manager.h
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

#ifndef GADGET_FUNCTION_MANAGER_H
#define GADGET_FUNCTION_MANAGER_H

#include <glib-object.h>
#include <gio/gio.h>

#include <gadget-manager.h>
#include <gadgetd-common.h>

G_BEGIN_DECLS

struct _GadgetFunctionManager;
typedef struct _GadgetFunctionManager GadgetFunctionManager;

#define GADGET_TYPE_FUNCTION_MANAGER	(gadget_function_manager_get_type ())
#define GADGET_FUNCTION_MANAGER(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GADGET_TYPE_FUNCTION_MANAGER, GadgetFunctionManager))
#define GADGET_IS_FUNCTION_MANAGER(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGET_TYPE_FUNCTION_MANAGER))

GType                          gadget_function_manager_get_type           (void) G_GNUC_CONST;
GadgetFunctionManager         *gadget_function_manager_new                (const gchar *gadget_name);

G_END_DECLS

#endif /* GADGETD_FUNCTION_MANAGER_H */
