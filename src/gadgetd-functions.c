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

#include <glib-unix.h>
#include <string.h>

#include "gadgetd-core-func.h"
#include "gadgetd-introspection.h"
#include "gadgetd-ffs-func.h"

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
	struct gd_function *func = NULL;

	type = container_of(t, struct gd_kernel_func_type, reg_type);
	func = g_malloc(sizeof(*func));
	if (!func) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	func->instance = g_strdup(instance);
	func->type = g_strdup(t->name);

	if (!func->instance || !func->type) {
		ret = USBG_ERROR_NO_MEM;
		goto error;
	}

	usbg_ret = usbg_create_function(g->g, type->func_type,
					instance, NULL, &(func->f));
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto error;
	}

	*f = func;
	func->parent = g;
	func->function_group = t->function_group;
	g->funcs = g_list_append(g->funcs, func);
	ret = GD_SUCCESS;
out:
	return ret;
error:
	g_free(func->instance);
	g_free(func->type);
	g_free(func);
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
	g_free(f->instance);
	g_free(f->type);
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
			/* If this function is not supported by libusbgx we
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

/* Compatible layer for old (<2.36 glib version). Since 2.36 g_unix_fd_add()
 * should be used
 */
#if !(GLIB_CHECK_VERSION(2, 36, 0))

struct gd_ffs_func_source {
	GSource source;
	GPollFD pfd;
};

static gboolean
gd_ffs_func_source_prepare(GSource *source, gint *timeout)
{
	return FALSE;
}

static gboolean
gd_ffs_func_source_check(GSource *source)
{
	struct gd_ffs_func_source *src = (typeof(src))source;
	return src->pfd.revents != 0;
}

static gboolean
gd_ffs_func_source_dispatch(GSource *source, GSourceFunc callback,
			    gpointer user_data)
{
	gboolean (*func)(gint, GIOCondition, gpointer) = (typeof(func))callback;
	struct gd_ffs_func_source *src = (typeof(src))source;
	if (!callback) {
		ERROR("callback not set");
		return FALSE;
	}

	return func(src->pfd.fd, src->pfd.revents, user_data);
}

static void
gd_ffs_func_source_finalize(GSource *source)
{
	/* nop */
}

#endif /* GLIB_CHECK_VERSION() */
/* ************************************************************************* */

gboolean gd_ffs_read_event(gint fd, GIOCondition condition, gpointer user_data)
{
	struct gd_ffs_func *func = (typeof(func)) user_data;
	struct usb_functionfs_event event;
	gboolean poll_again = FALSE;
	gint ret;

	if (condition & ~G_IO_IN) {
		ERROR("Unexpected event received from poll");
		goto out;
	}

	ret = read(func->ep0_fd, &event, sizeof(event));
	if (ret < sizeof(event)) {
		ERROR("Unable to read event from ffs");
		goto out;
	}
		INFO("Event %d", event.type);
	ret = gd_ffs_received_event(func, event.type);
	if (ret > 0) {
		INFO("FFS service started. PID: %d", func->pid);
	} else if (ret < 0) {
		ERROR("Error while processing FFS event");
	} else {
		/* 0 means that this event didn't cause service startup */
		poll_again = TRUE;
	}

out:
	return poll_again;
}

static int
gd_create_ffs_func(struct gd_gadget *g, struct gd_function_type *t,
		   const char *instance, struct gd_function **function)
{
	struct gd_ffs_func_type *type;
	int usbg_ret;
	int ret;
	struct gd_ffs_func *func = NULL;
	struct gd_function *f;
	const char *instance_prefix;
	char _cleanup_free_ *usbg_instance_name;

	type = container_of(t, struct gd_ffs_func_type, reg_type);
	func = g_malloc(sizeof(*func));
	if (!func) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	f = &(func->func);
	f->instance = g_strdup(instance);
	f->type = g_strdup(t->name);

	if (!f->instance || !f->type) {
		ret = USBG_ERROR_NO_MEM;
		goto error;
	}

	instance_prefix = strchr(t->name, '.');
	if (!instance_prefix) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto error;
	}

	++instance_prefix;
	usbg_instance_name = g_strdup_printf("%s.%s", instance_prefix, instance);
	if (!usbg_instance_name) {
		ret = USBG_ERROR_NO_MEM;
		goto error;
	}

	usbg_ret = usbg_create_function(g->g, F_FFS,
					usbg_instance_name, NULL, &(f->f));
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto error;
	}

	ret = gd_ffs_prepare_instance(type, func);
	if (ret) {
		usbg_rm_function(f->f, USBG_RM_RECURSE);
		goto error;
	}

	*function = f;
	f->parent = g;
	f->function_group = t->function_group;
	g->funcs = g_list_append(g->funcs, f);
	ret = GD_SUCCESS;

	/* add to poll */
	/* Currently value is ignored but it should be stored in gd_ffs_func */

	/* For glib >= 2.36 this one should be used: */
#if (GLIB_CHECK_VERSION(2, 36, 0))
	g_unix_fd_add(func->ep0_fd, G_IO_IN,
		      (GUnixFDSourceFunc)gd_ffs_read_event, func);
#else
	   /* For glib < 2.36 use our own event source */
	   {
		   static GSourceFuncs source_funcs = {
			   .prepare = gd_ffs_func_source_prepare,
			   .check = gd_ffs_func_source_check,
			   .dispatch = gd_ffs_func_source_dispatch,
			   .finalize = gd_ffs_func_source_finalize,
		   };
		   struct gd_ffs_func_source *src;
		   GSource *source;

		   source = g_source_new(&source_funcs, sizeof(*src));
		   src = (struct gd_ffs_func_source *) source;
		   src->pfd.fd = func->ep0_fd;
		   src->pfd.events = G_IO_IN;
		   src->pfd.revents = 0;
		   g_source_add_poll(source, &(src->pfd));
		   g_source_set_callback(source, (GSourceFunc)gd_ffs_read_event,
					 func, NULL);
		   /* collect id from this function */
		   g_source_attach(source, NULL);
		   g_source_unref(source);
	   }
#endif /* GLIB_CHECK_VERSION */
out:
	return ret;
error:
	g_free(f->instance);
	g_free(f->type);
	g_free(f);
	return ret;
}

static int
gd_rm_ffs_func(struct gd_function *f)
{
	ERROR("Not implemented yet");
	return 0;
}

static int
gd_cleanup_ffs_func_type(struct gd_function_type *t)
{
	struct gd_ffs_func_type *type;

	type = container_of(t, struct gd_ffs_func_type, reg_type);
	gd_unref_gd_ffs_func_type(type);
	return 0;
}


static int
gd_register_user_funcs()
{
	struct gd_ffs_func_type **types;
	struct gd_ffs_func_type *t;
	int i = 0;
	int ret;

	/* TODO Check if ffs is available */
	ret = gd_read_gd_ffs_func_types(&types);
	if (ret != GD_SUCCESS)
		goto out;

	for (t = types[i]; t; t = types[++i]) {
		/* We have almost filled the type structure
		 * we only need to fill callbacks
		 */
		t->reg_type.create_instance = gd_create_ffs_func;
		t->reg_type.rm_instance = gd_rm_ffs_func;
		t->reg_type.on_unregister = gd_cleanup_ffs_func_type;
		ret = gd_register_func_t(&(t->reg_type));
		if (ret != GD_SUCCESS) {
			ERROR("Unable to register func: %s", t->reg_type.name);
			t->cleanup(t);
			goto error;
		}
		/* We don't free type because it will be freed
		 * while unregistering function type or on exit
		 */

	}

	free(types);
out:
	return ret;
error:
	for (; t; t = types[++i])
		t->cleanup(t);
	free(types);

	return ret;
}

int
gd_init_functions()
{
	int ret;

	ret = gd_register_kernel_funcs();
	if (ret != GD_SUCCESS)
		goto error;


	ret = gd_register_user_funcs();
	if (ret != GD_SUCCESS)
		goto error;

	return ret;

 error:
	gd_unregister_all_func_t();
	return ret;
}

