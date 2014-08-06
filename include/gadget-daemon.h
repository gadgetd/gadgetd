/*
 * gadgetd-daemon.h
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

#ifndef GADGET_DAEMON_H
#define GADGET_DAEMON_H

#include <gio/gio.h>

G_BEGIN_DECLS

struct _GadgetDaemon;
typedef struct _GadgetDaemon GadgetDaemon;

typedef struct _GadgetDaemonClass	GadgetDaemonClass;

extern const char gadgetd_path[];

#define GADGET_TYPE_DAEMON     (gadget_daemon_get_type ())
#define GADGET_DAEMON(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GADGET_TYPE_DAEMON, GadgetDaemon))
#define GADGET_IS_DAEMON(o)    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GADGET_TYPE_DAEMON))

GType gadget_daemon_get_type (void) G_GNUC_CONST;

GadgetDaemon             *gadget_daemon_new                   (GDBusConnection *connection);
GDBusConnection          *gadget_daemon_get_connection        (GadgetDaemon    *daemon);
GDBusObjectManagerServer *gadget_daemon_get_object_manager    (GadgetDaemon    *daemon);
int                       gadget_daemon_run                   (void);

G_END_DECLS

#endif /* GADGET_DAEMON_H */
