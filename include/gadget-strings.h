/*
 * gadgetd-strings.h
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

#ifndef __GADGET_STRINGS_H__
#define __GADGET_STRINGS_H__

#include <glib-object.h>

G_BEGIN_DECLS

struct _GadgetStrings;
typedef struct _GadgetStrings GadgetStrings;

typedef struct _GadgetStringsClass	GadgetStringsClass;

#define GADGET_TYPE_STRINGS         (gadget_strings_get_type ())
#define GADGET_STRINGS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGET_TYPE_STRINGS, GadgetStrings))
#define GADGET_IS_STRINGS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGET_TYPE_STRINGS))

GType gadget_strings_get_type (void) G_GNUC_CONST;
GadgetStrings                 *gadget_strings_new                   (const gchar *gadget_name);

G_END_DECLS

#endif /* __GADGET_STRINGS_H__ */
