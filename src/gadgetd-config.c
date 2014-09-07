/*
 * gadgetd-config.c
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
 *@file gadgetd-config .c
 *@brief gadgetd congig handler
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <grp.h>
#include <limits.h>
#include "strdelim.h"

#include <gadgetd-config.h>
#include <gadgetd-common.h>

#define _GNU_SOURCE

#define QUOTE "\""

#define MAX_LINE_LENGTH 512

typedef enum {
	O_CONFIGFS_MOUNT_POINT,
	O_BCD_USB,
	O_BDEVICE_CLASS,
	O_BDEVICE_SUB_CLASS,
	O_BDEVICE_PROTOCOL,
	O_BMAX_PACKED_SIZE_0,
	O_ID_VENDOR,
	O_ID_PRODUCT,
	O_BCD_DEVICE,
	O_SERIAL_NUMBER,
	O_MANUFACTURER,
	O_PRODUCT_NAME,
	O_GD_CONFIGURATION,
	O_BAD_OPTION
} op_code;

/* TODO make error handle  */
static op_code
gadgetd_parse_opcode(const char *cp, const char *filename, int linenum)
{

	static struct {
		const char *name;
		op_code opcode;
	} config_op[] = {
		{ "configfs_mount_point", O_CONFIGFS_MOUNT_POINT},
		{ "bcdusb", O_BCD_USB},
		{ "bdeviceclass", O_BDEVICE_CLASS},
		{ "bdevicesubclass", O_BDEVICE_SUB_CLASS},
		{ "bdeviceprotocol", O_BDEVICE_PROTOCOL},
		{ "bmaxpacketsize0", O_BMAX_PACKED_SIZE_0},
		{ "idvendor", O_ID_VENDOR},
		{ "idproduct", O_ID_PRODUCT},
		{ "bcddevice", O_BCD_DEVICE},
		{ "serial_number", O_SERIAL_NUMBER},
		{ "product_name", O_PRODUCT_NAME},
		{ "manufacturer", O_MANUFACTURER},
		{ "gd_configuration", O_GD_CONFIGURATION},
		{ NULL, O_BAD_OPTION}
	};

	u_int i;
	for (i = 0; config_op[i].name; i++)
		if (strcasecmp(cp, config_op[i].name) == 0)
			break;
	/*error handle, filename for information purposes*/
	return config_op[i].opcode;
}

/*TODO prevent problems parsing config value*/

static int
gd_parse_str_value(char *s, char **charptr2)
{
	char *arg;
	int g_ret = GD_SUCCESS;

	arg = strdelim(&s);
	if (!arg || *arg == '\0') {
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	*charptr2 = strdup(arg);
	g_ret = GD_SUCCESS;
out:
	return g_ret;
}

static int
gd_parse_string(char *s, char *charptr)
{
	char *arg;
	int g_ret = GD_SUCCESS;

	arg = strdelim(&s);
	if (!arg || *arg == '\0') {
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	if (strlen(arg) > USBG_MAX_STR_LENGTH - 1) {
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	strncpy(charptr, arg, USBG_MAX_STR_LENGTH - 1);
	/*String length checked and will be copied to table
					with \0 terminator*/
	g_ret = GD_SUCCESS;
out:
	return g_ret;
}

static int
gd_parse_uint16_value(char *s, uint16_t *intptr)
{
	int value;
	char *endofnmb = NULL;
	char *arg;
	int g_ret = GD_SUCCESS;

	arg = strdelim(&s);
	value = strtol(arg, &endofnmb, 0);

	if (value < 0 || value > USHRT_MAX){
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	if (arg == endofnmb || *endofnmb != '\0') {
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	*intptr = value;
	g_ret = GD_SUCCESS;
out:
	return g_ret;
}

static int
gd_parse_uint8_value(char *s, uint8_t *intptr)
{
	int value;
	char *endofnmb, *arg;
	int g_ret = GD_SUCCESS;

	arg = strdelim(&s);
	value = strtol(arg, &endofnmb, 0);
	if (value < 0 || value > 255){
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	if (arg == endofnmb || *endofnmb != '\0') {
		g_ret = GD_ERROR_BAD_VALUE;
		goto out;
	}
	*intptr = value;
	g_ret = GD_SUCCESS;
out:
	return g_ret;
}

static int
gd_skip_keyword(char *keyword)
{
        if (keyword == NULL || !*keyword || *keyword == '\n'
	    || *keyword == '#' || *keyword == '[') {
		 return -1;
	}
        return 0;
}

static int
gd_read_config_line(struct gd_config *pconfig, usbg_gadget_attrs *g_attrs,
		 usbg_gadget_strs *g_strs, usbg_config_strs *cfg_strs,
		 char *line, const char *filename, int linenum)
{
	int opcode;
	size_t len;
	char *s, *keyword;
	int g_ret = GD_SUCCESS;
	uint16_t *uint16ptr = NULL;
	uint8_t *uint8ptr = NULL;
	char *charptr = NULL;
	char **charptr2 = NULL;

	len = strlen(line);
	if (line[len - 1]  != '\n') {
		g_ret = GD_ERROR_LINE_TOO_LONG;
		goto out;
	}

	for (len = strlen(line) -1; len > 0 ; len--) {
		if (isspace(line[len]) == 0)
			break;
		line[len] = '\0';
	}
	s = line;
	if ((keyword = strdelim(&s)) == NULL)
		return 0;
	if (*keyword == '\0')
		keyword = strdelim(&s);
	if (gd_skip_keyword(keyword) != 0)
		return 0;

	opcode = gadgetd_parse_opcode(keyword, filename, linenum);

	switch (opcode) {
	case O_BAD_OPTION:
		g_ret = GD_ERROR_OTHER_ERROR;
		break;
	case O_PRODUCT_NAME:
		charptr = g_strs->str_prd;
		break;
	case O_SERIAL_NUMBER:
		charptr = g_strs->str_ser;
		break;
	case O_MANUFACTURER:
		charptr = g_strs->str_mnf;
		break;
	case O_GD_CONFIGURATION:
		charptr = cfg_strs->configuration;
		break;
	case O_CONFIGFS_MOUNT_POINT:
		charptr2 = &pconfig->configfs_mnt;
		break;
	case O_BCD_USB:
		uint16ptr = &g_attrs->bcdUSB;
		break;
	case O_BDEVICE_CLASS:
		uint8ptr = &g_attrs->bDeviceClass;
		break;
	case O_BDEVICE_SUB_CLASS:
		uint8ptr = &g_attrs->bDeviceSubClass;
		break;
	case O_BDEVICE_PROTOCOL:
		uint8ptr = &g_attrs->bDeviceProtocol;
		break;
	case O_BMAX_PACKED_SIZE_0:
		uint8ptr = &g_attrs->bMaxPacketSize0;
		break;
	case O_BCD_DEVICE:
		uint16ptr = &g_attrs->bcdDevice;
		break;
	case O_ID_VENDOR:
		uint16ptr = &g_attrs->idVendor;
		break;
	case O_ID_PRODUCT:
		uint16ptr = &g_attrs->idProduct;
		break;
	default:
		break;
		ERROR("unnknown eror %d", opcode);
		g_ret = GD_ERROR_OTHER_ERROR;
	}



	if (uint8ptr) {
		g_ret = gd_parse_uint8_value(s, uint8ptr);
		if(g_ret != 0)
			ERROR("bad value in file %.100s at line %d expected uint8",
				filename, linenum);
	}
	else if (uint16ptr) {
		g_ret = gd_parse_uint16_value(s, uint16ptr) ;
		if(g_ret != 0)
			ERROR("bad value in file %.100s at line %d, expected uint16",
				filename, linenum);
	}
	else if (charptr) {
		g_ret = gd_parse_string(s, charptr);
		if(g_ret != 0)
			ERROR("bad value in file %.100s at line %d, expected char[%d]",
				filename, linenum, USBG_MAX_STR_LENGTH);
	}
	else if (charptr2) {
		g_ret = gd_parse_str_value(s, charptr2) ;
		if(g_ret != 0)
			ERROR("bad value in file %.100s at line %d",
				filename, linenum);
	}

out:
	return g_ret;
}

int
gd_read_config_file(struct gd_config *pconfig)
{
	FILE *fp;
	char line[MAX_LINE_LENGTH];
	int linenr = 0;
	int bad_opt = 0;
	char *filename = NULL;
	int g_ret;

	filename = pconfig->gd_config_file_path;

	INFO("read config file %s", filename);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		ERROR("read config failed");
		return GD_ERROR_FILE_OPEN_FAILED;
	}

	while (fgets(line, sizeof(line), fp)) {
		linenr++;
		g_ret = gd_read_config_line(pconfig, pconfig->g_attrs, pconfig->g_strs,
					pconfig->cfg_strs, line, filename, linenr);
		if (g_ret == GD_ERROR_LINE_TOO_LONG) {
			ERROR("Read file error");
			fclose(fp);
			goto out;
		}
		else if(g_ret != 0)
			bad_opt++;
	}
	INFO("close config file");

	fclose(fp);

	if (bad_opt > 0)
		/*error handle*/
		ERROR("config file %s:, contain %d bad options",
			filename, bad_opt);
out:
	return g_ret;
}

void
gadgetd_write_config(void)
{
	/* skel */
}


