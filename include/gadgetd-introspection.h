/*
 * gadgetd-introspection.h
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
#ifndef GADGETD_INTROSPECTION_H
#define GADGETD_INTROSPECTION_H

#include <glib.h>
#include <ffs-service.h>

/**
 * @brief Gets list of avaible usb functions
 * @param[out] dest Pointer to null terminated array containg names
 * of found functions. May be empty if nothing was found or NULL if
 * error occurred. Must be freed by caller.
 * @return Error code if failed or GD_SUCCESS if succeed
 **/
int gd_list_functions(gchar ***dest);

/**
 * @brief Parse given config file into gd_ffs_func_type structure
 * @param[in] path Path to configuration file
 * @param[out] service Pointer to destination service
 * @param[in] destroy_at_cleanup if zero, service will not be freed by its
 * cleanup function
 * @return Error code if failed or GD_SUCCESS if succeed
 **/
int gd_read_gd_ffs_func_type(const char *path, struct gd_ffs_func_type *service,
		int destroy_at_cleanup);

/**
 * @brief Parse all files from given directory into gd_ffs_func_type structures.
 * @param[in] path Directory path
 * @param[out] srvs Pointer to null-terminated list of pointers to created
 * structures
 * @return number of elements on srvs list
 **/
int gd_read_gd_ffs_func_types_from_dir(const char *path, struct gd_ffs_func_type ***srvs);

#endif /* GADGETD_INTROSPECTION_H */
