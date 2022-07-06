/*
 * Copyright (C) 2021 Collabora Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APPLAUNCHER_H
#define APPLAUNCHER_H

#include <glib-object.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "applaunch-dbus.h"
#include "app_info.h"

G_BEGIN_DECLS

#define APPLAUNCHD_TYPE_APP_LAUNCHER app_launcher_get_type()

G_DECLARE_FINAL_TYPE(AppLauncher, app_launcher, APPLAUNCHD, APP_LAUNCHER,
                     applaunchdAppLaunchSkeleton);

AppLauncher *app_launcher_get_default(void);

AppInfo *app_launcher_get_app_info(AppLauncher *self, const gchar *app_id);
sd_bus *app_launcher_get_bus(AppLauncher *self);
sd_event *app_launcher_get_event(AppLauncher *self);

G_END_DECLS

#endif
