/*
 * ffs-daemon.h
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

#ifndef FFS_DAEMON_H
#define FFS_DAEMON_H

#include <unistd.h>
#include <sys/types.h>
#include <linux/usb/functionfs.h>

/* The first passed file descriptor is fd 3 */
#define GD_ENDPOINT_FDS_START 3

/*
  Returns how many endpoint descriptors have been passed,
  including ep0 or negative errno code on failure.
*/
int gd_nmb_of_ep(int unset_environment);

/* Currently stab */
/*
  Return what is the type of given edpoint
 */
int gd_get_ep_type(int ep_fd);

/* Currently stab */
/*
  Return what is the number of given endpoint
  This functions does not depend on gadgetd conventions
  but check the real number of ep on function fs.
 */
int gd_get_ep_nmb(int ep_fd);

/*
  Get your endpoint descriptor by given number.
  You should not call this function with
  nmb >= gd_nmb_of_eps
 */
extern int gd_get_ep_by_nmb(int nmb);
inline int gd_get_ep_by_nmb(int nmb)
{
	return GD_ENDPOINT_FDS_START + nmb;
}

/*
  Get event which activated this daemon.
*/
enum usb_functionfs_event_type gd_get_activation_event(int unset_environment);

#endif /* FFS_DAEMON_H */
