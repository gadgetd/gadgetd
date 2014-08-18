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
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <libconfig.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
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

static int
gd_setting_get_string(config_setting_t *root, const char **str)
{
	if (config_setting_type(root) != CONFIG_TYPE_STRING) {
		ERROR("%s:%d: %s must be string",
			config_setting_source_file(root),
			config_setting_source_line(root),
			root->name);
		return GD_ERROR_BAD_VALUE;
	}

	*str = config_setting_get_string(root);

	return GD_SUCCESS;
}

static int
gd_setting_get_int(config_setting_t *root, int *dst)
{
	if (config_setting_type(root) != CONFIG_TYPE_INT) {
		ERROR("%s:%d: %s must be integer",
			config_setting_source_file(root),
			config_setting_source_line(root),
			root->name);
		return GD_ERROR_BAD_VALUE;
	}

	*dst = config_setting_get_int(root);

	return GD_SUCCESS;
}

static int
gd_setting_get_bool(config_setting_t *root, int *dst)
{
	int tmp;
	switch (config_setting_type(root)) {
	case CONFIG_TYPE_BOOL:
		*dst = config_setting_get_bool(root);
		break;
	case CONFIG_TYPE_INT:
		tmp = config_setting_get_int(root);
		if (tmp == 0 || tmp == 1) {
			*dst = tmp;
			break;
		}
	default:
		ERROR("%s:%d: %s must be bool",
			config_setting_source_file(root),
			config_setting_source_line(root),
			root->name);
		return GD_ERROR_BAD_VALUE;
	}

	return GD_SUCCESS;
}

static const char *
gd_string_skip_spaces(const char *ptr)
{
	while(isspace(*ptr) && *ptr != '\0')
		ptr++;
	return ptr;
}

static const char *
gd_string_skip_const_name(const char *ptr)
{
	while(!isspace(*ptr) && *ptr != '|' && *ptr != '\0')
		ptr++;
	return ptr;
}

static int
gd_parse_flags(const char *str, int *res, int (*translate)(const char *, int,
			int *))
{
	int tmp;
	int flag;
	const char *start;
	const char *pos;

	*res = 0;
	pos = str;
	start = pos;
	while (1) {
		pos = gd_string_skip_spaces(pos);
		if (*pos == '\0')
			break;

		start = pos;
		pos = gd_string_skip_const_name(pos);
		tmp = translate(start, pos - start, &flag);
		if (tmp < 0)
			return tmp;
		*res |= flag;

		pos = gd_string_skip_spaces(pos);
		if (*pos == '\0')
			break;

		if (*pos != '|')
			return GD_ERROR_BAD_VALUE;
		pos++;
	}

	return GD_SUCCESS;
}

/**
 * @brief structure storing name of variable and it's value
 **/
struct gd_named_const {
	const char *name;
	int len;
	int value;
};

#define DECLARE_ELEMENT(name) {#name, sizeof(#name)-1, name}
#define DECLARE_END() {NULL, 0, 0}

static int
gd_get_const_value(const char *name, int len, struct gd_named_const *keys, int *res)
{
	while (keys->name != NULL) {
		if (keys->len == len
		    && strncmp(name, keys->name, len) == 0) {
			*res = keys->value;
			return GD_SUCCESS;
		}
		keys++;
	}
	return GD_ERROR_NOT_FOUND;
}

static int
gd_desc_translate_attribute(const char *name, int len, int *att)
{
	int res = -1;
	int tmp;
	static struct gd_named_const keys[] = {
		DECLARE_ELEMENT(USB_CONFIG_ATT_ONE),
		DECLARE_ELEMENT(USB_CONFIG_ATT_SELFPOWER),
		DECLARE_ELEMENT(USB_CONFIG_ATT_WAKEUP),
		DECLARE_ELEMENT(USB_CONFIG_ATT_BATTERY),
		DECLARE_END()
	};

	if (name == NULL)
		return GD_ERROR_INVALID_PARAM;

	tmp = gd_get_const_value(name, len, keys, &res);
	if (tmp < 0) {
		ERROR("Unknown attribute flag: %.*s", len, name);
		return GD_ERROR_BAD_VALUE;
	}

	*att = res;

	return GD_SUCCESS;
}

static int
gd_lookup_desc_attributes(config_setting_t *root, __u8 *att)
{
	config_setting_t *node;
	int res;
	const char *buff;
	int tmp;

	node = config_setting_get_member(root, "bmAttributes");
	if (node == NULL) {
		ERROR("bmAttributes not defined");
		return GD_ERROR_NOT_FOUND;
	}

	if (config_setting_type(node) == CONFIG_TYPE_INT) {
		res = config_setting_get_int(node);
	} else if (config_setting_type(node) == CONFIG_TYPE_STRING) {
		buff = config_setting_get_string(node);
		tmp = gd_parse_flags(buff, &res, gd_desc_translate_attribute);
		if (tmp < 0)
			return tmp;
	} else {
		ERROR("bmAttributes must be string or number");
	}

	return GD_SUCCESS;
}

static int
gd_ffs_lookup_activation_event(config_setting_t *root,
	enum usb_functionfs_event_type *ev)
{
	const char *buff;
	config_setting_t *node;
	int tmp;

	node = config_setting_get_member(root, "activation_event");
	if (node == NULL) {
		ERROR("Activation_event not found");
		return GD_ERROR_NOT_DEFINED;
	}

	tmp = gd_setting_get_string(node, &buff);
	if (tmp < 0)
		return tmp;

	if (strcmp(buff, "FUNCTIONFS_BIND") == 0) {
		*ev = FUNCTIONFS_BIND;
	} else if (strcmp(buff, "FUNCTIONFS_ENABLE") == 0) {
		*ev = FUNCTIONFS_ENABLE;
	} else if (strcmp(buff, "FUNCTIONFS_SETUP") == 0) {
		*ev = FUNCTIONFS_SETUP;
	} else {
		ERROR("%s:%d: Unsupported event type %s",
			config_setting_source_file(node),
			config_setting_source_line(node), buff);
		return GD_ERROR_BAD_VALUE;
	}

	return GD_SUCCESS;
}

static int
gd_ffs_lookup_file(config_setting_t *root, char *name, char **path)
{
	struct stat st;
	const char *buff;
	config_setting_t *node;
	int tmp;

	node = config_setting_get_member(root, name);
	if (node == NULL)
		return GD_ERROR_NOT_DEFINED;

	tmp = gd_setting_get_string(node, &buff);
	if (tmp < 0)
		return tmp;

	if (stat(buff, &st) != 0) {
		ERROR("Path '%s' not found", buff);
		return GD_ERROR_NOT_FOUND;
	}

	if (!S_ISREG(st.st_mode)) {
		ERROR("Path %s='%s' is not file", name, buff);
		return GD_ERROR_BAD_VALUE;
	}

	*path = strdup(buff);
	if (*path == NULL) {
		tmp = gd_translate_error(errno);
		ERROR("error allocating memory");
		return tmp;
	}

	return GD_SUCCESS;
}

static int
gd_ffs_lookup_dir(config_setting_t *root, char *name, char **dir)
{
	struct stat st;
	const char *buff;
	int tmp;
	config_setting_t *node;

	node = config_setting_get_member(root, name);
	if (node == NULL)
		return GD_ERROR_NOT_DEFINED;

	tmp = gd_setting_get_string(node, &buff);
	if (tmp < 0)
		return tmp;

	if (stat(buff, &st) != 0) {
		ERROR("Path '%s' not found", buff);
		return GD_ERROR_NOT_FOUND;
	}

	if (!S_ISDIR(st.st_mode)) {
		ERROR("Path %s='%s' is not directory", name, buff);
		return GD_ERROR_BAD_VALUE;
	}

	*dir = strdup(buff);
	if (*dir == NULL)
		return GD_ERROR_NO_MEM;

	return GD_SUCCESS;
}

static int
gd_ffs_lookup_uid(config_setting_t *root, uid_t *id)
{
	int tmp;
	struct passwd *pw;
	config_setting_t *user;
	config_setting_t *uid;

	uid = config_setting_get_member(root, "uid");
	user = config_setting_get_member(root, "user");
	if (uid != NULL && user != NULL) {
		ERROR("user and uid cannot be both defined");
		return GD_ERROR_OTHER_ERROR;
	}

	if (user != NULL) {
		const char *buff;
		tmp = gd_setting_get_string(user, &buff);
		if (tmp < 0)
			return tmp;
		pw = getpwnam(buff);
	} else if (uid != NULL) {
		int tmpid;
		tmp = gd_setting_get_int(uid, &tmpid);
		if (tmp < 0)
			return tmp;
		pw = getpwuid(tmpid);
	} else {
		return GD_ERROR_NOT_DEFINED;
	}

	if (pw == NULL) {
		ERROR("User not found");
		return GD_ERROR_NOT_FOUND;
	}
	*id = pw->pw_uid;

	return GD_SUCCESS;
}

static int
gd_ffs_lookup_gid(config_setting_t *root, gid_t *id)
{
	int tmp;
	struct group *gr;
	config_setting_t *group;
	config_setting_t *gid;

	gid = config_setting_get_member(root, "gid");
	group = config_setting_get_member(root, "group");
	if (gid != NULL && group != NULL) {
		ERROR("group and gid cannot be both defined");
		return GD_ERROR_OTHER_ERROR;
	}

	if (group != NULL) {
		const char *buff;
		tmp = gd_setting_get_string(group, &buff);
		if (tmp < 0)
			return tmp;
		gr = getgrnam(buff);
	} else if (gid != NULL) {
		int tmpid;
		tmp = gd_setting_get_int(gid, &tmpid);
		if (tmp < 0)
			return tmp;
		gr = getgrgid(tmpid);
	} else {
		return GD_ERROR_NOT_DEFINED;
	}

	if (gr == NULL) {
		ERROR("Group not found");
		return GD_ERROR_NOT_FOUND;
	}
	*id = gr->gr_gid;

	return GD_SUCCESS;
}

static int
gd_ffs_lookup_options(config_setting_t *root, int *opt)
{
	int mult = 0;
	int conc = 0;
	int tmp;
	config_setting_t *node;

	node = config_setting_get_member(root, "allow_multiple");
	if (node != NULL) {
	       tmp = gd_setting_get_bool(node, &mult);
	       if (tmp < 0)
		       return tmp;
	}

	node = config_setting_get_member(root, "allow_concurent");
	if (node != NULL) {
		tmp = gd_setting_get_bool(node, &conc);
		if (tmp < 0)
			return tmp;
	}

	if (mult == 0 && conc == 1) {
		ERROR("Conflicted options");
		return GD_ERROR_BAD_VALUE;
	}

	if (mult)
		*opt |= FFS_SERVICE_ALLOW_MULTIPLE;

	if (conc)
		*opt |= FFS_SERVICE_ALLOW_CONCURENT;

	return GD_SUCCESS;
}

static int
gd_ffs_fill_str_config(config_setting_t *root, struct ffs_service *srv)
{
	config_setting_t *list;
	config_setting_t *group;
	config_setting_t *node;
	int i, j;
	int len;
	const char *str;
	int lang;
	int tmp;
	struct ffs_str_per_lang *res;

	list = config_setting_get_member(root, "strings");
	if (list == NULL) {
		ERROR("strings not defined");
		return GD_ERROR_NOT_FOUND;
	}

	if (config_setting_is_list(list) == CONFIG_FALSE) {
		ERROR("%s:%d strings: Expected list",
			config_setting_source_file(list),
			config_setting_source_line(list));
		return GD_ERROR_BAD_VALUE;
	}

	len = config_setting_length(list);
	res = calloc(len, sizeof(*res));
	if (res == NULL)
		return GD_ERROR_NO_MEM;

	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);
		if (group == NULL) {
			tmp = GD_ERROR_OTHER_ERROR;
			goto out;
		}
		node = config_setting_get_member(group, "lang");
		if (node == NULL) {
			ERROR("lang not defined");
			tmp = GD_ERROR_BAD_VALUE;
			goto out;
		}
		tmp = gd_setting_get_int(node, &lang);
		if (tmp < 0)
			return tmp;
		node = config_setting_get_member(group, "str");
		if (node == NULL) {
			ERROR("str not defined");
			tmp = GD_ERROR_NOT_FOUND;
			goto out;
		}
		tmp = gd_setting_get_string(node, &str);
		if (tmp < 0)
			goto out;
		res[i].code = lang;
		res[i].str = calloc(1, sizeof(*res[i].str));
		res[i].str[0] = strdup(str);
		if (res[i].str[0] == NULL) {
			ERROR("error allocating memory");
			tmp = GD_ERROR_NO_MEM;
			goto out;
		}
	}

	for (i = 0; i < len; i++)
		for (j = i+1; j < len; j++)
			if (res[i].code == res[j].code) {
				ERROR("lang %d defined more than once\n",
					res[i].code);
				tmp = GD_ERROR_OTHER_ERROR;
				goto out;
			}

	gd_ffs_fill_str(srv, res, len, 1);

	tmp = GD_SUCCESS;
out:
	for (i = 0; i < len; i++){
		free(res[i].str[0]);
		free(res[i].str);
	}
	free(res);
	return tmp;
}

static int
gd_lookup_desc_interface_class(config_setting_t *root, __u8 *cl)
{
	const char *buff;
	int buflen;
	config_setting_t *node;
	int res;
	int tmp;

	node = config_setting_get_member(root, "bInterfaceClass");
	if (node == NULL) {
		ERROR("Interface class not defined");
		return GD_ERROR_NOT_FOUND;
	}
	if (config_setting_type(node) == CONFIG_TYPE_INT) {
		res = config_setting_get_int(node);
	} else if (config_setting_type(node) == CONFIG_TYPE_STRING) {
		buff = config_setting_get_string(node);
		buflen = strlen(buff);
		res = -1;
		static struct gd_named_const keys[] = {
			DECLARE_ELEMENT(USB_CLASS_PER_INTERFACE),
			DECLARE_ELEMENT(USB_CLASS_AUDIO),
			DECLARE_ELEMENT(USB_CLASS_COMM),
			DECLARE_ELEMENT(USB_CLASS_HID),
			DECLARE_ELEMENT(USB_CLASS_PHYSICAL),
			DECLARE_ELEMENT(USB_CLASS_STILL_IMAGE),
			DECLARE_ELEMENT(USB_CLASS_PRINTER),
			DECLARE_ELEMENT(USB_CLASS_MASS_STORAGE),
			DECLARE_ELEMENT(USB_CLASS_HUB),
			DECLARE_ELEMENT(USB_CLASS_CDC_DATA),
			DECLARE_ELEMENT(USB_CLASS_CSCID),
			DECLARE_ELEMENT(USB_CLASS_CONTENT_SEC),
			DECLARE_ELEMENT(USB_CLASS_VIDEO),
			DECLARE_ELEMENT(USB_CLASS_WIRELESS_CONTROLLER),
			DECLARE_ELEMENT(USB_CLASS_MISC),
			DECLARE_ELEMENT(USB_CLASS_APP_SPEC),
			DECLARE_ELEMENT(USB_CLASS_VENDOR_SPEC),
			DECLARE_END()
		};
		tmp = gd_get_const_value(buff, buflen, keys, &res);
		if (tmp < 0) {
			ERROR("Unknown interface class %s", buff);
			return GD_ERROR_BAD_VALUE;
		}
	} else {
		ERROR("Interface class: expected string or number");
		return GD_ERROR_BAD_VALUE;
	}

	if (res > UCHAR_MAX || res < 0) {
		ERROR("bInterfaceClass out of range");
		return GD_ERROR_BAD_VALUE;
	}
	*cl = res;

	return GD_SUCCESS;
}

static int
gd_lookup_desc_interface_subclass(config_setting_t *root, __u8 *cl)
{
	int res;
	config_setting_t *node;

	node = config_setting_get_member(root, "bInterfaceSubClass");
	if (node == NULL) {
		ERROR("No subclass was defined");
		return GD_ERROR_NOT_FOUND;
	}

	if (config_setting_type(node) == CONFIG_TYPE_INT) {
		res = config_setting_get_int(node);
	} else if (config_setting_type(node) == CONFIG_TYPE_STRING) {
		/* TODO string-defined subclasses */
		return GD_ERROR_OTHER_ERROR;
	} else {
		ERROR("Interface subclass must be string or number");
		return GD_ERROR_BAD_VALUE;
	}

	if (res > UCHAR_MAX || res < 0) {
		ERROR("bInterfaceSubClass out of range");
		return GD_ERROR_BAD_VALUE;
	}
	*cl = res;

	return GD_SUCCESS;
}

static int
gd_ffs_parse_interface_desc(config_setting_t *root,
	struct usb_interface_descriptor *desc)
{
	int tmp;
	int res;
	config_setting_t *node;

	desc->bLength = USB_DT_INTERFACE_SIZE;
	tmp = gd_lookup_desc_interface_class(root, &desc->bInterfaceClass);
	if (tmp < 0)
		return tmp;
	tmp = gd_lookup_desc_interface_subclass(root, &desc->bInterfaceSubClass);
	if (tmp < 0)
		return tmp;
	node = config_setting_get_member(root, "iInterface");
	if (node == NULL) {
		ERROR("iInterface not defined");
		return GD_ERROR_NOT_FOUND;
	}
	tmp = gd_setting_get_int(node, &res);
	if (tmp < 0)
		return tmp;
	if (res > UCHAR_MAX || res < 0) {
		ERROR("iInterface out of range");
		return GD_ERROR_BAD_VALUE;
	}
	desc->iInterface = res;

	return GD_SUCCESS;
}

static int
gd_ffs_parse_ep_desc_no_audio(config_setting_t *root,
	struct usb_endpoint_descriptor_no_audio *desc)
{
	int res;
	int tmp;
	const char *buff;
	char dir_in[] = "in";
	char dir_out[] = "out";
	config_setting_t *node;

	desc->bLength = sizeof(*desc);
	node = config_setting_get_member(root, "address");
	if (node == NULL) {
		ERROR("address not defined");
		return GD_ERROR_NOT_FOUND;
	}

	tmp = gd_setting_get_int(node, &res);
	if (tmp < 0)
		return tmp;

	if (res > UCHAR_MAX || res < 0) {
		ERROR("Adress out of range");
		return GD_ERROR_BAD_VALUE;
	}
	/* only part of address, direction bit is set later */
	desc->bEndpointAddress = res;
	tmp = gd_lookup_desc_attributes(root, &desc->bmAttributes);
	if (tmp < 0)
		return tmp;

	node = config_setting_get_member(root, "direction");
	if (node == NULL) {
		ERROR("direction not defined");
		return GD_ERROR_NOT_FOUND;
	}

	tmp = gd_setting_get_string(node, &buff);
	if (tmp < 0)
		return tmp;

	if (strcmp(buff, dir_in) == 0) {
		desc->bEndpointAddress |= USB_DIR_IN;
	} else if (strcmp(buff, dir_out) == 0) {
		desc->bEndpointAddress |= USB_DIR_OUT;
	} else {
		ERROR("Invalid direction value");
		return GD_ERROR_BAD_VALUE;
	}

	return GD_SUCCESS;
}

static int
gd_ffs_fill_desc_list(config_setting_t *list, struct ffs_desc_per_seed *desc)
{
	int len;
	char *pos;
	int i;
	int tmp;
	const char *buff;
	config_setting_t *group;
	struct usb_interface_descriptor *inter;
	struct usb_endpoint_descriptor_no_audio *ep;
	int j = 0;

	len = config_setting_length(list);

	desc->desc_count = len;
	desc->desc_size = 0;
	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);
		if (config_setting_is_group(group) == CONFIG_FALSE) {
			ERROR("%s:%d Descriptor definition: expected group",
				config_setting_source_file(group),
				config_setting_source_line(group));
			return GD_ERROR_BAD_VALUE;
		}
		tmp = config_setting_lookup_string(group, "type", &buff);
		if (tmp == CONFIG_FALSE) {
			ERROR("Descriptor type not defined");
			return GD_ERROR_NOT_FOUND;
		}
		if (strcmp(buff, "INTERFACE_DESC") == 0)
			desc->desc_size += USB_DT_INTERFACE_SIZE;
		else if (strcmp(buff, "EP_NO_AUDIO_DESC") == 0)
			desc->desc_size += USB_DT_ENDPOINT_SIZE;
		else {
			ERROR("%s descriptor type unsupported", buff);
			return GD_ERROR_NOT_SUPPORTED;
		}
	}

	desc->desc = malloc(desc->desc_size);
	if (desc->desc == NULL) {
		ERROR("Error allocating memory");
		return GD_ERROR_NO_MEM;
	}

	pos = desc->desc;
	for (i = 0; i < len; i++) {
		group = config_setting_get_elem(list, i);
		tmp = config_setting_lookup_string(group, "type", &buff);
		if (strcmp(buff, "INTERFACE_DESC") == 0) {
			inter = (struct usb_interface_descriptor *)pos;
			tmp = gd_ffs_parse_interface_desc(group, inter);
			if (tmp < 0)
				goto out;
			inter->bNumEndpoints = 0;
			inter->bInterfaceNumber = j++;
			pos += USB_DT_INTERFACE_SIZE;
		} else if (strcmp(buff, "EP_NO_AUDIO_DESC") == 0) {
			ep = (struct usb_endpoint_descriptor_no_audio *)pos;
			tmp = gd_ffs_parse_ep_desc_no_audio(group, ep);
			if (tmp < 0)
				goto out;
			inter->bNumEndpoints++;
			pos += USB_DT_ENDPOINT_SIZE;
		}
	}

	return GD_SUCCESS;
out:
	free(desc->desc);
	return tmp;
}

static int
gd_ffs_fill_desc_config(config_setting_t *root, struct ffs_service *srv)
{
	config_setting_t *fs_desc;
	config_setting_t *hs_desc;
	config_setting_t *group;
	struct ffs_desc_per_seed desc[2];
	int tmp;
	int mask = 0;

	group = config_setting_get_member(root, "descriptors");
	if (group == NULL) {
		ERROR("descriptors not defined");
		return GD_ERROR_NOT_FOUND;
	}

	if (config_setting_is_group(group) == CONFIG_FALSE) {
		ERROR("descriptors must be group");
		return GD_ERROR_BAD_VALUE;
	}

	fs_desc = config_setting_get_member(group, "fs_desc");
	if (fs_desc == NULL) {
		ERROR("fs_desc not found");
		return GD_ERROR_NOT_FOUND;
	}

	if (config_setting_is_list(fs_desc) == CONFIG_FALSE) {
		ERROR("%s:%d fs_desc: expected list",
			config_setting_source_file(fs_desc),
			config_setting_source_line(fs_desc));
		return GD_ERROR_BAD_VALUE;
	}

	tmp = gd_ffs_fill_desc_list(fs_desc, &desc[0]);
	if (tmp < 0)
		return tmp;

	mask |= FFS_USB_FULL_SPEED;

	hs_desc = config_setting_get_member(group, "hs_desc");
	if (hs_desc != NULL) {
		if (config_setting_is_list(hs_desc) == CONFIG_FALSE) {
			ERROR("%s:%d hs_desc: expected list",
				config_setting_source_file(hs_desc),
				config_setting_source_line(hs_desc));
			return GD_ERROR_BAD_VALUE;
		}
		tmp = gd_ffs_fill_desc_list(hs_desc, &desc[1]);
		if (tmp < 0)
			return tmp;
		mask |= FFS_USB_HIGH_SPEED;
	}

	gd_ffs_fill_desc(srv, desc, mask);

	return GD_SUCCESS;
}

static void
gd_ffs_service_cleanup(struct ffs_service *srv)
{
	if (srv == NULL)
		return;
	gd_ffs_put_desc(srv);
	gd_ffs_put_str(srv);
	free(srv->name);
	free(srv->exec_path);
	free(srv->work_dir);
	free(srv->chroot_dir);
}

static void
gd_ffs_service_destroy(struct ffs_service *srv)
{
	gd_ffs_service_cleanup(srv);
	free(srv);
}

int
gd_read_ffs_service(const char *path, struct ffs_service *srv,
		int destroy_at_cleanup)
{
	config_t cfg;
	config_setting_t *root;
	int tmp;
	const char *base;

	if (path == NULL || srv == NULL)
		return GD_ERROR_INVALID_PARAM;

	config_init(&cfg);
	if (config_read_file(&cfg, path) == CONFIG_FALSE) {
		ERROR("%s:%d - %s",
			path,
			config_error_line(&cfg),
			config_error_text(&cfg));
		return GD_ERROR_OTHER_ERROR;
	}

	base = strrchr(path, '/');
	if (base == NULL)
		base = path;
	else
		base++;
	if (*base == '\0')
		goto out;

	memset(srv, 0, sizeof(*srv));
	if (destroy_at_cleanup)
		srv->cleanup = gd_ffs_service_destroy;
	else
		srv->cleanup = gd_ffs_service_cleanup;

	srv->name = strdup(base);
	if(srv->name == NULL)
		goto out;

	root = config_root_setting(&cfg);
	tmp = gd_ffs_lookup_activation_event(root, &srv->activation_event);
	if (tmp < 0)
		goto out;
	tmp = gd_ffs_lookup_file(root, "exec", &srv->exec_path);
	if (tmp < 0)
		goto out;
	tmp = gd_ffs_lookup_dir(root, "working_dir", &srv->work_dir);
	if (tmp < 0 && tmp != GD_ERROR_NOT_DEFINED)
		goto out;
	tmp = gd_ffs_lookup_uid(root, &srv->user_id);
	if (tmp < 0 && tmp != GD_ERROR_NOT_DEFINED)
		goto out;
	tmp = gd_ffs_lookup_gid(root, &srv->group_id);
	if (tmp < 0 && tmp != GD_ERROR_NOT_DEFINED)
		goto out;
	tmp = gd_ffs_lookup_dir(root, "chroot_to", &srv->chroot_dir);
	if (tmp < 0 && tmp != GD_ERROR_NOT_DEFINED)
		goto out;
	tmp = gd_ffs_lookup_options(root, &srv->options);
	if (tmp < 0)
		goto out;
	tmp = gd_ffs_fill_desc_config(root, srv);
	if (tmp < 0)
		goto out;
	tmp = gd_ffs_fill_str_config(root, srv);
	if (tmp < 0)
		goto out;

	config_destroy(&cfg);

	return GD_SUCCESS;
out:
	config_destroy(&cfg);
	srv->cleanup(srv);

	return tmp;
}

static int
gd_example_file_filter(const struct dirent *dir)
{
	const char *pos;
	char ext[] = ".example";
	int len;
	int res;

	pos = dir->d_name;
	if (pos[0] == '.')
		return 0;
	len = strlen(pos);
	pos += len - sizeof(ext) + 1;
	res = strcmp(pos, ext);
	return res;
}

int
gd_ffs_services_from_dir(const char *path, struct ffs_service ***srvs)
{
	struct dirent **namelist;
	int num;
	int i;
	int tmp;
	int pathlen, namelen;
	char filepath[PATH_MAX];
	struct ffs_service *srv;

	if (path == NULL || srvs == NULL)
		return GD_ERROR_INVALID_PARAM;

	num = scandir(path, &namelist, gd_example_file_filter, alphasort);
	if (num < 0) {
		tmp = gd_translate_error(errno);
		ERROR("error reading directory");
		return tmp;
	}

	*srvs = calloc(num + 1, sizeof(struct ffs_service *));
	if (*srvs == NULL) {
		tmp = GD_ERROR_NO_MEM;
		goto out;
	}

	tmp = snprintf(filepath, PATH_MAX, "%s/", path);
	if (tmp >= PATH_MAX) {
		ERROR("path too long");
		tmp = GD_ERROR_PATH_TOO_LONG;
		goto out;
	}

	pathlen = tmp;
	for (i = 0; i < num; i++) {
		namelen = strlen(namelist[i]->d_name);
		if (pathlen + namelen >= PATH_MAX) {
			ERROR("path too long");
			tmp = GD_ERROR_PATH_TOO_LONG;
			goto out;
		}

		strncpy(filepath + pathlen, namelist[i]->d_name, namelen + 1);

		srv = malloc(sizeof(*srv));
		if (srv == NULL) {
			tmp = GD_ERROR_NO_MEM;
			goto out;
		}

		tmp = gd_read_ffs_service(filepath, srv, 1);
		if (tmp < 0) {
			ERROR("%s: file parsing failed", filepath);
			goto out;
		}

		(*srvs)[i] = srv;
	}

	return num;
out:
	if (*srvs)
		for (i = 0; i < num; i++)
			free((*srvs)[i]);
	free(*srvs);
	*srvs = NULL;
	return tmp;
}
