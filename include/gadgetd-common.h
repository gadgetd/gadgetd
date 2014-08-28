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

#include <string.h>
#include <errno.h>

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


#define ERROR(msg, ...) fprintf(stderr, "%s()  "msg" \n",  __func__, ##__VA_ARGS__)
#define ERRNO(msg, ...) fprintf(stderr, "%s()  "msg": %m \n", __func__, ##__VA_ARGS__)
#define INFO(msg, ...)  fprintf(stderr, "%s()  "msg" \n", __func__, ##__VA_ARGS__)
#define DEBUG(msg, ...) fprintf(stderr, "%s():%d  "msg" \n", __func__, LINENO, ##__VA_ARGS__)

/**
 * @brief Translate errno into gadgetd error code
 **/
int gd_translate_error(int error);
