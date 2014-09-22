/*
 * gadgetd-core-function.h
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
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

#ifndef GADGETD_CORE_FUNCTION_H
#define GADGETD_CORE_FUNCTION_H

#include "gadgetd-core.h"

struct gd_function_type {
	const char *name;
	int (*create_instance)(struct gd_gadget *, struct gd_function_type *,
			       const char *, struct gd_function **);
	int (*rm_instance)(struct gd_function *);

	int (*on_unregister)(struct gd_function_type *t);
};

/**
 * @brief Register given function type on a list of known
 * functions
 * @param[in] type Function type to be register
 * @return GD_SUCCESS on success, gd_error otherwise
 * @note memory pointed by type has to be valid untill
 * gd_unregister_func_t() call.
 */
int gd_register_func_t(struct gd_function_type *type);

/**
 * @brief Unregister given function type from list of known
 * functions.
 * @param[in] type Function type to be removed
 * @return GD_SUCCESS on success, gd_error otherwise
 * @note This function calls on_unregister callback if it
 * has been provided
 */
int gd_unregister_func_t(struct gd_function_type *type);

/**
 * @brief Unregister all registered function types;
 */
void gd_unregister_all_func_t();

#endif /* GADGETD_CORE_FUNCTION_H */

