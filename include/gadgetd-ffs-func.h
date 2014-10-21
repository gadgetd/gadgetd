/*
 * gadgetd-ffs-func-type.h
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

#ifndef GADGETD_FFS_FUNC_TYPE_H
#define GADGETD_FFS_FUNC_TYPE_H

#include <unistd.h>
#include <sys/types.h>

#include <linux/usb/functionfs.h>

#include <gadgetd-core-func.h>

enum ffs_service_options {
	FFS_SERVICE_ALLOW_MULTIPLE = 1,
	FFS_SERVICE_ALLOW_CONCURENT = 2
};


enum ffs_usb_max_speed {
#ifdef __FFS_LEGACY_API_SUPPORT
	FFS_USB_FULL_SPEED = 1,
	FFS_USB_HIGH_SPEED = 2,
	FFS_USB_TERMINATOR = 4
#else
	FFS_USB_FULL_SPEED = FUNCTIONFS_HAS_FS_DESC,
	FFS_USB_HIGH_SPEED = FUNCTIONFS_HAS_HS_DESC,
	FFS_USB_SUPER_SPEED = FUNCTIONFS_HAS_SS_DESC,
	FFS_USB_TERMINATOR = FFS_USB_SUPER_SPEED << 1
#endif
};

struct ffs_desc_per_seed {
	int desc_size;
	int desc_count;
	char *desc;
} __attribute__ ((__packed__));

struct ffs_str_per_lang {
	__le16 code;
	char **str;
} __attribute__ ((__packed__));

/* Currently only exec is done, rest is TODO */
struct gd_ffs_func_type {
	struct gd_function_type reg_type;
	char *exec_path;
	char *work_dir;
	char *chroot_dir;
	uid_t user_id;
	gid_t group_id;

	int options;
	enum usb_functionfs_event_type activation_event;

	int refcnt;

	void *desc;
	int desc_size;

	void *str;
	int str_size;

	void (*cleanup)(struct gd_ffs_func_type *);
};

enum ffs_instance_state {
	FFS_INSTANCE_READY,
	FFS_INSTANCE_BOUND,
	FFS_INSTANCE_ENABLED,
	FFS_INSTANCE_RUNNING
};

struct gd_ffs_func {
	struct gd_function func;
	char *mount_dir;
	int ep0_fd;

	struct gd_ffs_func_type *service;
	enum ffs_instance_state state;
	pid_t pid;
};

struct gd_ffs_func_type *gd_ref_gd_ffs_func_type(struct gd_ffs_func_type *srv);

void gd_unref_gd_ffs_func_type(struct gd_ffs_func_type *srv);

/*
 * Creates instance of ffs service with given name
 * Service will be in state FFS_INSTANCE_MOUNTED
 */
int gd_ffs_prepare_instance(struct gd_ffs_func_type *srv, struct gd_ffs_func *func);

/*
 * Informs instance that event has been received
 * This functions starts required service if event type is suitable to do so.
 * Returns <0 if error occurred, 0 if event processed, pid of child if event
 * processed and service started
 */
int gd_ffs_received_event(struct gd_ffs_func *inst,
			  enum usb_functionfs_event_type type);

/*
 * Fills gd_ffs_func_type with given descriptors
 */
int gd_ffs_fill_desc(struct gd_ffs_func_type *srv, struct ffs_desc_per_seed *desc,
		     int desc_mask);

/* Cleans up memory allocated for descriptors */
void gd_ffs_put_desc(struct gd_ffs_func_type *srv);

/*
 * Fills gd_ffs_func_type with given strings
 */
int gd_ffs_fill_str(struct gd_ffs_func_type *srv, struct ffs_str_per_lang *str,
		    int lang_nmb, int str_nmb);

void gd_ffs_put_str(struct gd_ffs_func_type *srv);

#endif /* GADGETD_FFS_FUNC_TYPE_H */
