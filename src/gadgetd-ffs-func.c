/*
 * ffs-service.c
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

#define _GNU_SOURCE /* for asprintf */
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <linux/limits.h>
#include <endian.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "gadgetd-ffs-func.h"
#include "common.h"

struct gd_ffs_func_type *
gd_ref_gd_ffs_func_type(struct gd_ffs_func_type *srv)
{
	struct gd_ffs_func_type *ret = NULL;
	if (!srv)
		goto out;

	/* check if service is already in use */
	if (srv->refcnt > 1 && !(srv->options & FFS_SERVICE_ALLOW_MULTIPLE))
		goto out;

	++(srv->refcnt);
	ret = srv;
out:
	return ret;
}

void
gd_unref_gd_ffs_func_type(struct gd_ffs_func_type *srv)
{
	if (srv && --(srv->refcnt) == 0) {
		if (srv->cleanup)
			srv->cleanup(srv);
	}
}


/* Generates path according to policy and mounts functionfs in that dir.
   returns pointer to new allocated path to ffs root */
static char *
mount_ffs_instance(const char *service, const char *name)
{
	/* currently hardcoded, should be configurable.
	   Assume that this directory exist */
	static const char *prefix = "/tmp/gadgetd/";
	int ret;
	char service_dir[PATH_MAX];
	char *mount_dir = NULL;

	ret = snprintf(service_dir, sizeof(service_dir), "%s/%s",
		       prefix, service);
	if (ret < 0 || ret >= sizeof(service_dir))
		goto error_out;

	ret = mkdir(service_dir, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret != 0 && errno != EEXIST) {
		ERRNO("Unable to create dir for ffs service");
		goto error_out;
	}

	ret = asprintf(&mount_dir, "%s/%s", service_dir, name);
	if (ret < 0)
		goto error_out;

	ret = mkdir(mount_dir, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret != 0) {
		ERRNO("Unable to create dir for ffs instance");
		goto error;
	}

	ret = mount(name, mount_dir, "functionfs", 0, NULL);
	if (ret) {
		ERRNO("Unable to mount ffs instance");
		goto error;
	}

	return mount_dir;

error:
	free(mount_dir);
error_out:
	return NULL;
}

/* This function assume that path is correct and formed due to
   given policy */
static void
umount_ffs_instance(char *path)
{
	char *slash = NULL;
	DIR *dir = NULL;
	struct dirent d;
	struct dirent *result = 0;
	int i = 0, ret = 0;

	if (!path)
		return;
	/* We ignore error */
	ret = umount(path);
	if (ret < 0) {
		ERRNO("Unable to umount ffs.");
		return;
	}

	ret = rmdir(path);
	if (ret < 0) {
		ERRNO("Unable to remove ffs instance dir.");
		return;
	}

	slash = strrchr(path, '/');
	if (!slash)
		return;

	*slash = '\0';
	/* If directory is empty, remove it */
	dir = opendir(path);
	if (!dir)
		return;
	/* each directory has . and .. */
	while(i < 2) {
		ret = readdir_r(dir, &d, &result);
		if (ret || !result)
			break;
		else
			++i;
	}
	closedir(dir);

	/* chec if dir was realy empty or error occurred */
	if (i < 2 && !ret) {
		INFO("All ffs instances removed. Removing ffs service dir");
		ret = rmdir(path);
		if (ret < 0) {
			ERRNO("Unable to remove ffs service dir");
			return;
		}
	}

}

int
gd_ffs_prepare_instance(struct gd_ffs_func_type *srv, struct gd_ffs_func *func)
{
	char ep0_file[PATH_MAX];
	int ret = 0;

	if (!srv || !func)
		goto out;

	func->service = gd_ref_gd_ffs_func_type(srv);
	if (!func->service)
		goto out;

	func->mount_dir = mount_ffs_instance(srv->reg_type.name,
					     func->func.instance);
	if (!func->mount_dir)
		goto err_unref;

	ret = snprintf(ep0_file, sizeof(ep0_file), "%s/ep0", func->mount_dir);
	if (ret < 0 || ret >= sizeof(ep0_file))
		goto err_umount;

	func->ep0_fd = open(ep0_file, O_RDWR);
	if (func->ep0_fd < 0)
		goto err_umount;

	/* Write descriptors */
	ret = write(func->ep0_fd, func->service->desc,
		    func->service->desc_size);
	if (ret < 0) {
		ERRNO("Unable to write descriptors");
		goto err_close;
	}

	/* Write strings */
	ret = write(func->ep0_fd, func->service->str, func->service->str_size);
	if (ret < 0) {
		ERRNO("Umable to write strings");
		goto err_close;
	}

	func->state = FFS_INSTANCE_READY;

out:
	return GD_SUCCESS;

err_close:
	close(func->ep0_fd);
	func->ep0_fd = -1;
err_umount:
	umount_ffs_instance(func->mount_dir);
	free(func->mount_dir);
err_unref:
	gd_unref_gd_ffs_func_type(func->service);
	return GD_ERROR_OTHER_ERROR;
}

static char **
prepare_args(struct gd_ffs_func *inst)
{
	char **args;
	int size = 2;
	int i = 0;

	args = calloc(size, sizeof(char *));
	if (!args)
		goto out;

	/* We don't have to do this, but we do to
	   follow memory allocation convention */
	args[i] = strdup(inst->service->exec_path);
	if (!args[i++])
		goto error;

	args[i] = NULL;
out:
	return args;

error:
	while (i)
		free(args[--i]);

	free(args);
	return NULL;
}

static char **
prepare_environ(struct gd_ffs_func *inst, int n_fds)
{
	pid_t pid;
	int event;
	int ret;
	char **envp = NULL;
	int size = 4;
	int i = 0;

	/* Max number of usb endpoints is 32 */
	if (n_fds > 32)
		goto out;

	envp = calloc(size, sizeof(char*));
	if (!envp)
		goto out;

	ret = asprintf(&(envp[i++]), "LISTEN_FDS=%d", n_fds);
	if (ret < 0)
		goto error;

	pid = getpid();
	ret = asprintf(&(envp[i++]), "LISTEN_PID=%d", pid);
	if (ret < 0)
		goto error;

	event = inst->service->activation_event;
	ret = asprintf(&(envp[i++]), "ACTIVATION_EVENT=%d", event);
	if (ret < 0)
		goto error;

	envp[i++] = NULL;
out:
	return envp;

error:
	while (i)
		free(envp[--i]);

	free(envp);
	return NULL;
}

/* based on systemd */
static int
shift_fds(int *pattern, int n_fds)
{
	int i = 0, restart_from = -1, start = 0, ret = 0;

	/* We execute this loop until all descriptors will be in their places */
	do {
		restart_from = -1;

		for (i = start; i < n_fds; ++i) {
			int fd;

			if (pattern[i] == i + 3)
				continue;

			fd = fcntl(pattern[i], F_DUPFD, i + 3);
			if (fd < 0) {
				ret = -errno;
				goto out;
			}

			ret = close(pattern[i]);
			if (unlikely(ret < 0)) {
				if (errno == EINTR)
					ret = 0;
				else
					goto out;
			}

			pattern[i] = fd;
			/* Check if new descriptor is the one
			   which we requested */
			if (fd != i + 3 && restart_from < 0)
				restart_from = i;
		}

		start = restart_from;
	} while (restart_from >= 0);

out:
	return ret;
}

static int
find_int_array(int to_find, int *array, int size)
{
	int i = 0;

	for (i = 0; i < size; ++i) {
		if (array[i] == to_find)
			break;
	}

	return i != size ? i : -1;
}

/* based on systemd */
static int
close_all_fds(int *except, int n_except)
{
	int fd;
	int ret;
	DIR *d;
	struct dirent fd_entry;
	struct dirent *result;
	struct rlimit rl;

	/* try to use system features */
        d = opendir("/proc/self/fd");
        if (!d)
		goto fallback;
	do {
		ret = readdir_r(d, &fd_entry, &result);
		if (ret != 0) {
			ret = -ret;
			break;
		}
		/* if we are at the end of dir just exit the loop */
		if (!result)
			break;
		/* ignore . and .. in /proc there will be no hidden files */
		if (fd_entry.d_name[0] == '.')
			continue;

		ret = sscanf(fd_entry.d_name, "%d", &fd);
		/* This should not happend but just in case ignore this one */
		if (ret != 1)
			continue;

		/* check if this one should be ignored */
		if (find_int_array(fd, except, n_except) >= 0)
			continue;

		/* ignore stdin, stdout, sdterr */
		if (fd < 3)
			continue;

		/* Check if it is not the opened /proc/self/fd dir fd */
		ret = dirfd(d);
		if (ret < 0)
			break;

		if (ret == fd)
			continue;

		/* Filtration is done so we can now close this descriptor */
		ret = close(fd);
		if (unlikely(ret < 0)) {
			/* EBADF - issue with valgrind descriptor */
			if (errno != EINTR && errno != EBADF) {
				ret = -errno;
				break;
			}
		}
	} while(1);

        closedir(d);
        return ret;

fallback:
	/* If /proc is not available let's iterate
	   throught all possible descriptors values */
	ret = getrlimit(RLIMIT_NOFILE, &rl);
	if (unlikely(ret < 0))
		goto out;

	for (fd = 3; fd < (int) rl.rlim_max; ++fd) {

		if (find_int_array(fd, except, n_except) >= 0)
			continue;

		ret = close(fd);
		if (unlikely(ret < 0)) {
			/* EBADF - issue with valgrind descriptor */
			if (errno != EINTR && errno != EBADF) {
				ret = -errno;
				break;
			}
		}
	 }

out:
	 return ret;
}

static int
ep_select(const struct dirent *dent)
{
	int ret = 1;
	/* We ignore dot files and also ep0 because
	   it is already opened */
        if ((strcmp(dent->d_name, ".") == 0) ||
	    (strcmp(dent->d_name, "..") == 0) ||
	    (strcmp(dent->d_name, "ep0") == 0))
                ret = 0;

	return ret;
}

static int
ep_sort(const struct dirent **epa, const struct dirent **epb)
{
	int epa_nmb, epb_nmb, ret;

	ret = sscanf((*epa)->d_name, "ep%d", &epa_nmb);
	if (ret != 1)
		return -1;

	ret = sscanf((*epb)->d_name, "ep%d", &epb_nmb);
	if (ret !=1)
		return -1;

	if (epa_nmb < epb_nmb)
		ret = -1;
	else if (epa_nmb > epb_nmb)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int
prepare_fds_table(struct gd_ffs_func *inst, int **fdsp)
{
	struct dirent **ep;
	int *fds;
	/* When using i in soome loop please check
	   error handling procedure at the end of funciton */
	int ep_nmb = -1, i = 0;
	int j, path_len, ret;
	char path[PATH_MAX];

	/* we know that this path is not as long as buffer because we were
	   able to open ep0 */
	path_len = snprintf(path, sizeof(path), "%s", inst->mount_dir);
	if (path_len >= sizeof(path))
		goto error;

	ep_nmb = scandir(path, &ep, ep_select, ep_sort);
	if (ep_nmb < 0)
		goto error;

	fds = calloc(ep_nmb + 1, sizeof(int));
	if (!fds)
		goto free_ep;

	fds[0] = inst->ep0_fd;

	for (i = 0; i < ep_nmb; ++i) {
		ret = snprintf(&(path[path_len]), sizeof(path) - path_len,
			       "/%s", ep[i]->d_name);
		if (ret >= sizeof(path) - path_len)
			goto close_fds;

		fds[i + 1] = open(path, O_RDWR);
		if (fds[i + 1] < 0)
			goto close_fds;

		free(ep[i]);
	}
	free(ep);

	*fdsp = fds;

	/* add 1 for ep0 */
	return ep_nmb + 1;

close_fds:
	j = i + 1;
	while(--j > 0)
		close(fds[j]);
free_ep:
	for (; i < ep_nmb; ++i )
		free(ep[i]);

	free(ep);
	free(fds);
error:
	return -1;
}

static int
setup_child(struct gd_ffs_func *inst)
{
	int *fds;
	int n_fds, ret, i;
	char **envp;
	char **args;

	n_fds = prepare_fds_table(inst, &fds);
	if (n_fds < 0)
		goto err;

	ret = close_all_fds(fds, n_fds);
	if (ret < 0)
		goto err;

	ret = shift_fds(fds, n_fds);
	if (ret < 0)
		goto err_fds;

	args = prepare_args(inst);
	if (!args)
		goto err_fds;

	envp = prepare_environ(inst, n_fds);
	if (!envp)
		goto err_args;

	execve(inst->service->exec_path, args, envp);

	/* We should not came here otherwise it's error */

	for (i = 0; envp[i]; ++i)
		free(envp[i]);
	free(envp);
err_args:
	for (i = 0; args[i]; ++i)
		free(args[i]);
	free(args);
err_fds:
	free(fds);
err:
	exit(-1);
}

static int
run_ffs_instance(struct gd_ffs_func *inst)
{
	int ret;

	ret = fork();

	if (ret == 0) {
		/* we have a child so set it up*/
		/* we will never return from this function */
		setup_child(inst);
	} else if (likely(ret > 0)) {
		inst->pid = ret;
		inst->state = FFS_INSTANCE_RUNNING;
		close(inst->ep0_fd);
		inst->ep0_fd = -1;
	}

	return ret;
}

int
gd_ffs_received_event(struct gd_ffs_func *inst,
			  enum usb_functionfs_event_type type)
{
	int ret = -1;

	if (!inst || inst->state == FFS_INSTANCE_RUNNING)
		goto out;

	switch(type) {
	case FUNCTIONFS_BIND:
		inst->state = FFS_INSTANCE_BOUND;
		break;
	case FUNCTIONFS_UNBIND:
		inst->state = FFS_INSTANCE_READY;
		break;
	case FUNCTIONFS_ENABLE:
		inst->state = FFS_INSTANCE_ENABLED;
		break;

	default:
		/* Other events should not appear here */
		assert(0);
	}

	/* Check if we should run the service */
	if (type == inst->service->activation_event) {
		INFO("Received sutable event. Running ffs instance.");
		ret = run_ffs_instance(inst);
	} else {
		ret = 0;
	}
out:
	return ret;
}

int
gd_ffs_fill_desc(struct gd_ffs_func_type *srv, struct ffs_desc_per_seed *desc,
		 int desc_mask)
{
#ifdef __FFS_LEGACY_API_SUPPORT
	struct usb_functionfs_descs_head *header;
#else
	struct {
		__le32 magic;
		__le32 length;
		__le32 flags;
		__le32 fs_count;
		__le32 hs_count;
	} *header;
#endif
	int size = sizeof(*header);
	int ret = 0, i = 0, j = 0;
	void *pos;

	for (i = FFS_USB_FULL_SPEED; i < FFS_USB_TERMINATOR; i = i << 1)
		if (i & desc_mask)
			size += desc[j++].desc_size;

#ifndef __FFS_LEGACY_API_SUPPORT
	size += j*sizeof(__le32);
#endif

	/* TODO: what if srv->desc != NULL ? */
	pos = malloc(size);
	if (!pos) {
		ret = -1;
		goto out;
	}
	srv->desc = pos;
	srv->desc_size = size;

	/* Fill header of functionfs descriptors */
	header = pos;
	header->length = htole32(size);
	j = 0;
#ifdef __FFS_LEGACY_API_SUPPORT
	header->magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC);
	header->fs_count = desc_mask & FFS_USB_FULL_SPEED ?
		htole32(desc[j++].desc_count) : 0;
	header->hs_count = desc_mask & FFS_USB_HIGH_SPEED ?
		htole32(desc[j++].desc_count) : 0;
	pos += sizeof(*header);
#else
	header->magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC_V2);
	for (i = FFS_USB_FULL_SPEED; i < FFS_USB_TERMINATOR; i = i << 1)
		if (i & desc_mask) {
			header->flags &= i;
			*(__le32*)(pos + sizeof(*header) + sizeof(__le32)*j) =
				htole32(desc[j].desc_count);
			++j;
		}
	pos += sizeof(*header) + sizeof(__le32)*j;
#endif

	/* Fill endpoint descriptors */
	j = 0;
	for (i = FFS_USB_FULL_SPEED; i < FFS_USB_TERMINATOR; i = i << 1)
		if (i & desc_mask) {
			memcpy(pos, &(desc[j].desc[0]), desc[j].desc_size);
			pos += desc[j].desc_size;
			++j;
		}

out:
	return ret;
}

void
gd_ffs_put_desc(struct gd_ffs_func_type *srv)
{
	free(srv->desc);
	srv->desc = NULL;
	srv->desc_size = -1;
}

int
gd_ffs_fill_str(struct gd_ffs_func_type *srv, struct ffs_str_per_lang *str,
		    int lang_nmb, int str_nmb)
{
	struct usb_functionfs_strings_head *header;
	int size = sizeof(*header);
	void *pos;
	int i = 0, j = 0, ret = 0;

	for (i = 0; i < lang_nmb; ++i) {
		size += sizeof(str[i].code);
		for (j = 0; j < str_nmb; ++j)
			size += strlen(str[i].str[j]) + 1;
	}

	/* TODO: What if srv->str != NULL */
	pos = malloc(size);
	if (!pos) {
		ret = -1;
		goto out;
	}
	srv->str = pos;
	srv->str_size = size;

	/* Fill header of funcitonfs strings */
	header = pos;
	header->magic = htole32(FUNCTIONFS_STRINGS_MAGIC);
	header->length = size;
	header->str_count = htole32(str_nmb);
	header->lang_count = htole32(lang_nmb);
	pos += sizeof(*header);

	/* Fill strings */
	for (i = 0; i < lang_nmb; ++i) {
		*(typeof(str[i].code)*)pos = str[i].code;
		pos += sizeof(str[i].code);
		for (j = 0; j < str_nmb; ++j) {
			strcpy(pos, str[i].str[j]);
			pos += strlen(str[i].str[j]);
		}
	}
out:
	return ret;
}

void
gd_ffs_put_str(struct gd_ffs_func_type *srv)
{
	free(srv->str);
	srv->str = NULL;
	srv->str_size = -1;
}

