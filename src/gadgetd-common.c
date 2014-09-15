/*
 * gadgetd-common.c
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
#include <gadgetd-common.h>

gd_error gd_translate_error(int error)
{
	int ret;

	switch (error) {
	case ENOTSUP:
		ret = GD_ERROR_NOT_SUPPORTED;
		break;
	case ENOMEM:
		ret = GD_ERROR_NO_MEM;
		break;
	case EINVAL:
		ret = GD_ERROR_INVALID_PARAM;
		break;
	case EDQUOT:
	case EACCES:
	case ENOENT:
		ret = GD_ERROR_FILE_OPEN_FAILED;
		break;
	default:
		ret = GD_ERROR_OTHER_ERROR;
	}

	return ret;
}

void
get_iface (GObject *object, GType skeleton, gpointer _pointer_to_iface)
{
	GDBusInterface **pointer_to_iface = _pointer_to_iface;

	if (*pointer_to_iface == NULL) {
		*pointer_to_iface = g_object_new(skeleton, NULL);
	}

	g_dbus_object_skeleton_add_interface(G_DBUS_OBJECT_SKELETON (object),
					G_DBUS_INTERFACE_SKELETON (*pointer_to_iface));
}
