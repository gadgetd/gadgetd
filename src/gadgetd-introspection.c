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
#include <errno.h>

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

static int
gd_str_cmp(const void *a, const void *b)
{
	if (*(gchar **)a == NULL)
		return 1;
	if (*(gchar **)b == NULL)
		return -1;
	return strcmp(*(gchar **)a, *(gchar **)b);
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

static int
gd_append_usbfunc_modules(char *path, gchar ***funclist,
	int *cap, int *count)
{
	gchar pattern[] = "usbfunc:";
	gchar alias[] = "alias";
	gchar buff[MAX(sizeof(pattern), sizeof(alias))];
	gchar *tmpstr;
	int tmp;
	int ret = GD_ERROR_OTHER_ERROR;
	FILE *fp;
	int newcnt;
	int newcap;
	int i;
	gchar **res;
	gchar **ptr;

	newcnt = *count;
	newcap = *cap;

	res = *funclist;
	fp = fopen(path, "r");
	if (fp == NULL) {
		ret = gd_translate_error(errno);
		return ret;
	}

	qsort(res, *count, sizeof(*res), gd_str_cmp);

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
			ptr = (gchar **)bsearch(&tmpstr, res, *count,
				sizeof(*res), gd_str_cmp);
			if (ptr == NULL) {
				tmp = gd_str_list_append(&res, tmpstr,
					newcap, newcnt++);
				if (tmp < 0) {
					ret = tmp;
					goto error;
				}
				newcap = tmp;
			}
		}

		gd_skip_till_eol(fp);
		if (feof(fp))
			break;
	}

	tmp = gd_str_list_append(&res, NULL, newcap, newcnt);
	if (tmp < 0) {
		ret = tmp;
		goto error;
	}
	*cap = tmp;
	*count = newcnt;
	*funclist = res;
	fclose(fp);
	return GD_SUCCESS;

error:
	if (fp != NULL)
		fclose(fp);
	*cap = newcap;
	for (i = *count; i < newcnt; i++)
		free(res[i]);
	res[*count] = NULL;
	*funclist = res;

	return ret;
}

static int
gd_append_func_list(char *path, gchar ***funclist, int *cap, int *count)
{
	FILE *fp;
	gchar *tmpstr;
	gchar **ptr;
	gchar **res;
	int tmp;
	int err_code = GD_ERROR_OTHER_ERROR;
	int newcnt = *count;
	int newcap = *cap;
	int i;

	fp = fopen(path, "r");
	if (fp == NULL) {
		err_code = gd_translate_error(errno);
		return err_code;
	}

	res = *funclist;
	qsort(res, *count, sizeof(*res), gd_str_cmp);
	while (1) {
		tmp = gd_alloc_get_next_str(fp, &tmpstr);
		if (tmp == 0)
			break;
		if (tmp < 0) {
			err_code = tmp;
			goto error;
		}
		ptr = (gchar **)bsearch(&tmpstr, res, newcnt,
				sizeof(*res), gd_str_cmp);

		if (ptr == NULL) {
			tmp = gd_str_list_append(&res, tmpstr,
				newcap, newcnt++);
			if (tmp < 0) {
				err_code = tmp;
				goto error;
			}
			newcap = tmp;
		}
	}
	tmp = gd_str_list_append(&res, NULL, newcap, newcnt);
	if (tmp < 0) {
		err_code = tmp;
		goto error;
	}
	*cap = tmp;
	*count = newcnt;
	*funclist = res;
	fclose(fp);
	return GD_SUCCESS;

error:
	fclose(fp);
	*cap = newcap;
	for (i = *count; i < newcnt; i++)
		free(res[i]);
	res[*count] = NULL;
	*funclist = res;
	return err_code;
}

int
gd_list_functions(gchar ***dest)
{
	char path[PATH_MAX];
	struct utsname name;
	/* default size of list is number of usb functions in kernel config */
	int cap = KERNEL_USB_FUNCTIONS_COUNT + 1;
	int count = 0;
	gchar **list;
	int tmp;

	if (dest == NULL)
		return GD_ERROR_INVALID_PARAM;

	if (uname(&name) != 0)
		return gd_translate_error(errno);

	tmp = snprintf(path, PATH_MAX,
		"/lib/modules/%s/modules.alias", name.release);
	if (tmp >= PATH_MAX) {
		*dest = NULL;
		ERROR("Path too long");
		return GD_ERROR_PATH_TOO_LONG;
	}

	list = malloc(cap * sizeof(gchar *));
	if (list == NULL) {
		tmp = GD_ERROR_NO_MEM;
		goto error;
	}
	list[0] = NULL;

	tmp = gd_append_usbfunc_modules(path, &list, &cap, &count);
	if (tmp < 0 && tmp != GD_ERROR_FILE_OPEN_FAILED)
		goto error;
	else if (tmp == GD_ERROR_FILE_OPEN_FAILED)
		INFO("modules.alias file not found");

	tmp = gd_append_func_list("/sys/class/usb_gadget/func_list",
		&list, &cap, &count);
	if (tmp < 0 && tmp != GD_ERROR_FILE_OPEN_FAILED)
		goto error;
	else if (tmp == GD_ERROR_FILE_OPEN_FAILED)
		INFO("func_list file not found");

	cap = count + 1;
	*dest = realloc(list, cap * sizeof(*list));
	if (*dest == NULL) {
		tmp = GD_ERROR_NO_MEM;
		goto error;
	}

	return GD_SUCCESS;

error:
	gd_free_2d(list, count);
	*dest = NULL;

	return tmp;
}
