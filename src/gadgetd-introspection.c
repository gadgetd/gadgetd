/*
 * gadgetd-introspection.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <limits.h>

#include <gadgetd-config.h>
#include <gadgetd-create.h>
#include <gadgetd-common.h>
#include <gadgetd-introspection.h>

#include <glib.h>

#define ALIAS_BUFF_LEN 4096
/* Number of usb functions in kernel config */
#define KERNEL_USB_FUNCTIONS_COUNT 12

static gchar
gd_skip_till_eol(FILE *fp)
{
	gchar c;

	do
		c = getc(fp);
	while (!feof(fp) && c != '\n');
	return c;
}

static gchar
gd_skip_spaces(FILE *fp)
{
	gchar c;

	do
		c = getc(fp);
	while (isspace(c) && !feof(fp));
	return c;
}

static int
gd_realloc_buffer(gchar **src, int len, int newlen,
	int element_size, int is_static)
{
	gchar *result = *src;

	if (is_static) {
		result = malloc(newlen * element_size);
		if (result == NULL)
			return GD_ERROR_NO_MEM;
		memcpy(result, src, len);
	} else {
		result = realloc(src, newlen * element_size);
		if (result == NULL) {
			free(src);
			return GD_ERROR_NO_MEM;
		}
	}
	*src = result;

	return newlen;
}

static int
gd_str_list_append(gchar ***list, gchar *str, int cap, int count)
{
	gchar **tmp;

	if (count >= cap) {
		cap *= 2;
		tmp = realloc(*list, cap * sizeof(gchar *));
		if (tmp == NULL)
			return GD_ERROR_NO_MEM;
		*list = tmp;
	}
	(*list)[count] = str;

	return cap;
}

static void
gd_free_2d(gchar **src, int len)
{
	int i;

	if (src == NULL)
		return;
	for (i = 0; i < len; i++)
		free(src[i]);
	free(src);
}

static int
gd_get_next_str(FILE *fp, gchar *dest, int len)
{
	gchar c;
	int count = 0;

	c = gd_skip_spaces(fp);
	if (feof(fp))
		return 0;
	dest[count++] = c;

	while (count < len - 1) {
		c = getc(fp);
		if (feof(fp)) {
			ERROR("Error: wrong file format");
			break;
		}
		dest[count++] = c;
	}

	dest[count++] = '\0';

	return count;
}

static int
gd_alloc_get_next_str(FILE *fp, gchar **str)
{
	gchar c;
	gchar sbuff[ALIAS_BUFF_LEN];
	gchar *buff = sbuff;
	int count = 0;
	int tmp;
	int buffcap = sizeof(sbuff);

	c = gd_skip_spaces(fp);
	if (feof(fp))
		return 0;

	do {
		buff[count++] = c;
		if (count >= buffcap) {
			buffcap *= 2;
			tmp = gd_realloc_buffer(&buff, count, buffcap,
				sizeof(gchar), buff == sbuff);
			if (tmp < 0)
				return tmp;
		}
		c = getc(fp);
	} while (!isspace(c) && !feof(fp));

	buff[count++] = '\0';
	if (buff == sbuff) {
		buff = strdup(buff);
		if (buff == NULL)
			return GD_ERROR_NO_MEM;
	} else {
		buff = realloc(buff, count * sizeof(gchar));
		if(buff == NULL) {
			free(buff);
			return GD_ERROR_NO_MEM;
		}
	}
	*str = buff;

	return count;
}

int
gd_list_usbfunc_modules(char *path, gchar ***aliases)
{
	gchar pattern[] = "usbfunc:";
	gchar alias[] = "alias";
	gchar buff[MAX(sizeof(pattern), sizeof(alias))];
	gchar *tmpstr;
	int tmp;
	int count = 0;
	int ret = GD_ERROR_OTHER_ERROR;
	/* default size of buffer is number of usb functions in kernel config */
	int cap = KERNEL_USB_FUNCTIONS_COUNT;
	FILE *fp;
	gchar **res;

	if (aliases == NULL || path == NULL)
		return GD_ERROR_INVALID_PARAM;

	res = *aliases;
	fp = fopen(path, "r");
	if (fp == NULL) {
		ret = gd_translate_error(errno);
		ERROR("Error opening file: %s", path);
		return ret;
	}

	res = malloc(cap * sizeof(gchar *));
	if (res == NULL) {
		ERROR("Error allocating memory");
		ret = GD_ERROR_NO_MEM;
		goto error;
	}

	while (1) {
		tmp = gd_get_next_str(fp, buff, sizeof(alias));
		if (tmp == 0)
			break;
		if (tmp < sizeof(alias))
			goto error;

		if (buff[0] == '#') {
			gd_skip_till_eol(fp);
			if (feof(fp))
				break;
			continue;
		}

		if (strcmp(buff, alias) != 0) {
			ERROR("Error: wrong alias file format");
			goto error;
		}

		tmp = gd_get_next_str(fp, buff, sizeof(pattern));
		if (tmp == 0) {
			ERROR("Error: wrong alias file format");
			goto error;
		}
		if (tmp < sizeof(pattern))
			goto error;

		if (strcmp(buff, pattern) == 0) {
			tmp = gd_alloc_get_next_str(fp, &tmpstr);
			if (tmp == 0) {
				ERROR("Error: wrong alias file format");
				goto error;
			}
			if (tmp < 0) {
				ret = tmp;
				goto error;
			}
			cap = gd_str_list_append(&res, tmpstr, cap, count++);
			if (cap < 0) {
				ret = cap;
				goto error;
			}
		}

		gd_skip_till_eol(fp);
		if (feof(fp))
			break;
	}

	gd_str_list_append(&res, NULL, cap, count);
	*aliases = res;
	fclose(fp);
	return cap;

error:
	gd_free_2d(res, count);
	if (fp != NULL)
		fclose(fp);
	return ret;
}
