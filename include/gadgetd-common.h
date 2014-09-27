/*
 * gadgetd-common.h
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
#ifndef GADGETD_COMMON_H
#define GADGETD_COMMON_H

/**
 * @file gadgetd-common.h
 * @brief gadgetd functions
 */

#include <glib.h>
#include "common.h"
#include <usbg/usbg.h>
#include <gio/gio.h>

typedef enum  {
	GD_SUCCESS = 0,
	GD_ERROR_NOT_FOUND = -1,
	GD_ERROR_NOT_SUPPORTED = -2,
	GD_ERROR_FILE_OPEN_FAILED = -4,
	GD_ERROR_BAD_VALUE = -5,
	GD_ERROR_NO_MEM = -6,
	GD_ERROR_LINE_TOO_LONG = -7,
	GD_ERROR_INVALID_PARAM = -8,
	GD_ERROR_PATH_TOO_LONG = -9,
	GD_ERROR_NOT_DEFINED = -10,
	GD_ERROR_EXIST = -11,
	GD_ERROR_OTHER_ERROR = -99
} gd_error;

struct gd_context
{
	usbg_state *state;
};

extern struct gd_context ctx;

/*TODO move to core ? */
/* list of UDC-s on device */
extern GList *gd_udcs;

/**
 * @brief Get interface
 **/
void get_iface (GObject *object, GType skeleton, gpointer pointer_to_iface);

/**
 * @brief get function type
 * @param[in] _str_type
 * @param[out] _type
 */
gboolean get_function_type(const gchar *_str_type, usbg_function_type *_type);

/**
 * @brief Translate errno into gadgetd error code
 **/
int gd_translate_error(int error);

/**
 * @brief Makes part of object path from str
 * @details Object path part may contain only [A-Z][a-z][0-9]_ characters.
 * This functions replaces all other characters in given string with _.
 * / Character is reserved for path parts separation and in can not be used in str
 * @param[in] str Str to make path part from it
 * @param[out] path_part Valid part of path. Should be freed by caller
 * @return GD_SUCCESS on success, gd_error otherwise
 */
gint make_valid_object_path_part(const gchar *str, char **path_part);

static inline void _cleanup_fn_g_free_(void *p) {
	g_free(*(gpointer *)p);
}

#define _cleanup_g_free_ _cleanup_(_cleanup_fn_g_free_)

#endif /* GADGETD_COMMON_H */
