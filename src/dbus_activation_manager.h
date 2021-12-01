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

#ifndef DBUSACTIVATIONMANAGER_H
#define DBUSACTIVATIONMANAGER_H

#include <glib-object.h>

#include "app_info.h"

G_BEGIN_DECLS

#define APPLAUNCHD_TYPE_DBUS_ACTIVATION_MANAGER dbus_activation_manager_get_type()

G_DECLARE_FINAL_TYPE(DBusActivationManager, dbus_activation_manager,
                     APPLAUNCHD, DBUS_ACTIVATION_MANAGER, GObject);

DBusActivationManager *dbus_activation_manager_new(void);

gboolean dbus_activation_manager_start_app(DBusActivationManager *self,
                                           AppInfo *app_info);
gboolean dbus_activation_manager_activate_app(DBusActivationManager *self,
                                              AppInfo *app_info);

void dbus_activation_manager_free_runtime_data(gpointer data);

G_END_DECLS

#endif
