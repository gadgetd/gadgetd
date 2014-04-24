/*
 * gadgetd-config.h
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
 * @file gadgetd-config.h
 * @brief gadgetd config handler
 */

#include <usbg/usbg.h>

/**
 * @brief gadgetd config
 * @param configfs_mnt configfs mount point
 * @param usb_config_str gadgetd configuration file
 * @param g_attrs USB gadget device attributes
 * @param g_strs USB gadget device strings
 * @param cfg_strs USB configuration strings
 */

struct gd_config {
	char *configfs_mnt;
	char *gd_config_file_path;
	usbg_gadget_attrs *g_attrs;
	usbg_config_strs *cfg_strs;
	usbg_gadget_strs *g_strs;
};

extern struct gd_config config;

void gd_write_config(void);
int gd_read_config_file(struct gd_config *pconfig);
char *gd_check_conf_file(char *file);
int gd_parse_value(char *s, char **charptr, int *intptr,
                   const char *filename, int linenum);
