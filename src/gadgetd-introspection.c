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

	config_destroy(&cfg);

	return GD_SUCCESS;
out:
	config_destroy(&cfg);
	srv->cleanup(srv);

	return tmp;
}
