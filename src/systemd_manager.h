/*
 * Copyright (C) 2022 Konsulko Group
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

#ifndef SYSTEMDMANAGER_H
#define SYSTEMDMANAGER_H

#include <glib-object.h>

#include "app_info.h"

G_BEGIN_DECLS

#define APPLAUNCHD_TYPE_SYSTEMD_MANAGER systemd_manager_get_type()

G_DECLARE_FINAL_TYPE(SystemdManager, systemd_manager,
                     APPLAUNCHD, SYSTEMD_MANAGER, GObject);

SystemdManager *systemd_manager_new(void);

gboolean systemd_manager_start_app(SystemdManager *self,
                                   AppInfo *app_info);

G_END_DECLS

#endif
