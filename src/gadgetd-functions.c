/*
 * gadgetd-functions.c
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

#include "gadgetd-core-func.h"
#include "gadgetd-introspection.h"

struct gd_kernel_func_type {
	int func_type;
	struct gd_function_type reg_type;
};

static int
gd_create_kernel_func(struct gd_gadget *g,
				 struct gd_function_type *t,
				 const char *instance,
				 struct gd_function **f)
{
	struct gd_kernel_func_type *type;
	int usbg_ret;
	int ret;
	struct gd_function *func;

	type = container_of(t, struct gd_kernel_func_type, reg_type);
	func = g_malloc(sizeof(*func));
	if (!func) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	usbg_ret = usbg_create_function(g->g, type->func_type,
					instance, NULL, &(func->f));
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	*f = func;
	func->parent = g;
	func->function_group = t->function_group;
	g->funcs = g_list_append(g->funcs, func);
	ret = GD_SUCCESS;
out:
	return ret;
}

static int
gd_rm_kernel_func(struct gd_function *f)
{
	int usbg_ret;

	/* We allow to remove function even if it is added
	 * to some config.
	 */
	usbg_ret = usbg_rm_function(f->f, USBG_RM_RECURSE);
	if (usbg_ret != USBG_SUCCESS)
		goto out;

	f->parent->funcs = g_list_remove(f->parent->funcs, f);
	g_free(f);
out:
	return usbg_ret;
}

static int
gd_cleanup_kernel_func_type(struct gd_function_type *t)
{
	struct gd_kernel_func_type *type;
	type = container_of(t, struct gd_kernel_func_type, reg_type);

	free((char *)type->reg_type.name);
	g_free(type);

	return 0;
}

static int
gd_determine_function_group(int type)
{
	int group;

	switch (type) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		group = FUNC_GROUP_SERIAL;
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		group = FUNC_GROUP_NET;
		break;
	case F_PHONET:
		group = FUNC_GROUP_PHONET;
		break;
	case F_FFS:
		group = FUNC_GROUP_FFS;
		break;
	default:
		group = FUNC_GROUP_OTHER;
	}

	return group;
}

static int
gd_register_kernel_funcs()
{
	int ret;
	int func_type;
	gchar **functions;
	gchar **func;
	struct gd_kernel_func_type *type;

	/* Check what is available in Kernel and in modules */
	ret = gd_list_functions(&functions);
	if (ret != GD_SUCCESS)
		goto out;

	for (func = functions; *func; ++func) {
		func_type = usbg_lookup_function_type(*func);
		if (func_type < 0) {
			/* If this function is not supported by libusbg we
			 * will be unable to create its instance but it's not
			 * a reason to report an error. Just don't register it
			 * so it won't be available through gadgetd API
			 */
			INFO("Unsupported kernel function %s found. Skipping...",
			     *func);
			free(*func);
			continue;
		}

		type = g_malloc(sizeof(*type));
		if (!type) {
			ret = GD_ERROR_NO_MEM;
			goto error;
		}

		type->func_type = func_type;
		/* Func will be freed in cleanup function while unregister */
		type->reg_type.name = *func;
		type->reg_type.function_group = gd_determine_function_group(func_type);
		type->reg_type.create_instance = gd_create_kernel_func;
		type->reg_type.rm_instance = gd_rm_kernel_func;
		type->reg_type.on_unregister = gd_cleanup_kernel_func_type;

		ret = gd_register_func_t(&(type->reg_type));
		if (ret != GD_SUCCESS) {
			ERROR("Unable to register func: %s", *func);
			g_free(type);
			goto error;
		}
		/* We don't free type because it will be freed
		 * while unregistering function type or on exit
		 */
	}

	free(functions);
out:
	return ret;
error:
	for (; *func; ++func)
		free(*func);
	free(functions);
	/* We don'y unregister previously registered functions.
	 * We leave decision what to do with them to caller.
	 */
	return ret;
}

int
gd_init_functions()
{
	int ret;

	ret = gd_register_kernel_funcs();
	if (ret != GD_SUCCESS)
		gd_unregister_all_func_t();

	return ret;
}

