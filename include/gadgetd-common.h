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

/**
 * @file gadgetd-common.h
 * @brief gadgetd functions
 */

#include <glib.h>
#include "common.h"

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
	GD_ERROR_OTHER_ERROR = -99
} gd_error;

/**
 * @brief Translate errno into gadgetd error code
 **/
int gd_translate_error(int error);

static inline void _cleanup_fn_g_free_(void *p) {
	g_free(*(gpointer *)p);
}

#define _cleanup_g_free_ _cleanup_(_cleanup_fn_g_free_)
