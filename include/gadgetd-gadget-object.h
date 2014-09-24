/*
 * gadgetd-gadget-object.h
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

#ifndef GADGETD_OBJECT_H
#define GADGETD_OBJECT_H

#include <gadget-daemon.h>
#include "gadget-manager.h"
#include "gadgetd-core.h"

G_BEGIN_DECLS

struct _GadgetdGadgetObject;
typedef struct _GadgetdGadgetObject GadgetdGadgetObject;

#define GADGETD_TYPE_GADGET_OBJECT         (gadgetd_gadget_object_get_type ())
#define GADGETD_GADGET_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGETD_TYPE_GADGET_OBJECT, GadgetdGadgetObject))
#define GADGETD_IS_GADGET_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGETD_TYPE_GADGET_OBJECT))

GType                 gadgetd_gadget_object_get_type   (void) G_GNUC_CONST;
GadgetdGadgetObject  *gadgetd_gadget_object_new        (GadgetDaemon         *object,
							const gchar          *gadget_path,
							struct gd_gadget     *gadget);
GadgetDaemon         *gadgetd_gadget_object_get_daemon (GadgetdGadgetObject  *object);

G_END_DECLS

#endif /* GADGETD_OBJECT_H */
