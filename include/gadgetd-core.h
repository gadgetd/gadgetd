/*
 * gadgetd-core.h
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

#ifndef GADGETD_CORE_H
#define GADGETD_CORE_H

#include <glib.h>
#include <gio/gio.h>

#include <usbg/usbg.h>

#include "gadgetd-common.h"

struct gd_gadget {
	usbg_gadget *g;
	GList *funcs;
	GList *configs;
};

struct gd_function {
	struct gd_gadget *parent;
	char *instance;
	char *type;
	usbg_function *f;
};

struct gd_udc {
	usbg_udc *u;
	struct gd_gadget *g;
};

/**
 * @brief Creates USB gadget with suitable atrs and strings
 * @param name Name of new gadget
 * @param attrs Attributes to be set, should be "a{sv}"
 * @param strings Strings in English to be set, should be "a{sv}"
 * @param g Gadget structure to be filled
 * @param error Place to store error string. Should not be freed
 * @return 0 on success, gd_error on failure
 */
int gd_create_gadget(const gchar *name, GVariant *attrs,
		     GVariant *strings, struct gd_gadget *g,
		     const gchar **error);

/**
 * @brief Lists all currently available function types
 * @param zero_terminated indicates if returned array shou be 0 terminated
 * @return List of available functions or NULL if error occurred
 */
GArray *gd_list_func_types(gboolean zero_terminated);

#endif /* GADGETD_CORE_H */

