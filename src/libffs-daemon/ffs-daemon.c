/*
 * ffs-daemon.c
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

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "ffs-daemon.h"

#if (__GNUC__ >= 4)
#  ifdef GD_EXPORT_SYMBOLS
/* Export symbols */
#    define _gd_export_ __attribute__ ((visibility("default")))
#  else
/* Don't export the symbols */
#    define _gd_export_ __attribute__ ((visibility("hidden")))
#  endif
#else
#  define _gd_export_
#endif

_gd_export_ int
gd_nmb_of_ep(int unset_environment)
{
#if defined(DISABLE_GADGETD) || !defined(__linux__)
	return 0;
#else
        int r, fd;
        const char *env;
        char *end_ptr = NULL;
        unsigned long val;

        env = getenv("LISTEN_PID");
        if (!env) {
                r = 0;
                goto finish;
        }

        errno = 0;
        val = strtoul(env, &end_ptr, 10);

        if (errno > 0) {
                r = -errno;
                goto finish;
        }

        if (!end_ptr || end_ptr == env || *end_ptr || val <= 0) {
                r = -EINVAL;
                goto finish;
        }

        /* Is this for us? */
        if (getpid() != (pid_t) val) {
                r = 0;
                goto finish;
        }

	/* Use enviroment variables similar to systemd */
        env = getenv("LISTEN_FDS");
        if (!env) {
                r = 0;
                goto finish;
        }

        errno = 0;
        val = strtoul(env, &end_ptr, 10);

        if (errno > 0) {
                r = -errno;
                goto finish;
        }

        if (!end_ptr || end_ptr == env || *end_ptr) {
                r = -EINVAL;
                goto finish;
        }

        for (fd = GD_ENDPOINT_FDS_START;
	     fd < GD_ENDPOINT_FDS_START + (int) val;
	     fd++) {
                int flags;

                flags = fcntl(fd, F_GETFD);
                if (flags < 0) {
                        r = -errno;
                        goto finish;
                }

                if (flags & FD_CLOEXEC)
                        continue;

                if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
                        r = -errno;
                        goto finish;
                }
        }

        r = (int) val;

finish:
        if (unset_environment) {
                unsetenv("LISTEN_PID");
                unsetenv("LISTEN_FDS");
        }

        return r;


#endif
}

_gd_export_ int
gd_get_ep_type(int ep_fd)
{
	// how to do this?
	/* Waiting for kernel extension */
	return 0;
}

_gd_export_ int
gd_get_ep_nmb(int ep_fd)
{
	/* Currently it's a hack. Kernel ffs will be extended
	   with ioctls for endpoints */
	return ep_fd - GD_ENDPOINT_FDS_START;
}

_gd_export_ enum usb_functionfs_event_type
gd_get_activation_event(int unset_environment)
{
	enum usb_functionfs_event_type e;
        const char *env;
        char *end_ptr = NULL;
        unsigned long val;

        env = getenv("ACTIVATION_EVENT");
        if (!env) {
                e = (enum usb_functionfs_event_type)-1;
                goto finish;
        }

        errno = 0;
        val = strtoul(env, &end_ptr, 10);

        if (errno > 0) {
                e = (enum usb_functionfs_event_type)-1;
                goto finish;
        }

        if (!end_ptr || end_ptr == env || *end_ptr || val <= 0) {
                e = (enum usb_functionfs_event_type)-1;
                goto finish;
        }

	e = (enum usb_functionfs_event_type)val;

finish:
        if (unset_environment)
		unsetenv("ACTIVATION_EVENT");

	return e;
}

