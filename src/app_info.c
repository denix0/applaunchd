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

#include <gio/gio.h>

#include "app_info.h"
#include "dbus_activation_manager.h"

struct _AppInfo {
    GObject parent_instance;

    gchar *app_id;
    gchar *name;
    gchar *icon_path;
    gchar *command;
    gboolean dbus_activated;
    gboolean systemd_activated;
    gboolean graphical;

    AppStatus status;

    /*
     * `runtime_data` is an opaque pointer depending on the app startup method.
     * It is set in by ProcessManager or DBusActivationManager.
     */
    gpointer runtime_data;
};

G_DEFINE_TYPE(AppInfo, app_info, G_TYPE_OBJECT);

/*
 * Initialization & cleanup functions
 */

static void app_info_dispose(GObject *object)
{
    AppInfo *self = APPLAUNCHD_APP_INFO(object);

    g_clear_pointer(&self->app_id, g_free);
    g_clear_pointer(&self->name, g_free);
    g_clear_pointer(&self->icon_path, g_free);
    g_clear_pointer(&self->app_id, g_free);

    if (self->dbus_activated) {
        g_clear_pointer(&self->runtime_data,
                        dbus_activation_manager_free_runtime_data);
    }
/* else if (self->systemd_activated) {
        g_clear_pointer(&self->runtime_data,
                        systemd_manager_free_runtime_data); */
    else {
        g_clear_pointer(&self->runtime_data, g_free);
    }

    G_OBJECT_CLASS(app_info_parent_class)->dispose(object);
}

static void app_info_finalize(GObject *object)
{
    G_OBJECT_CLASS(app_info_parent_class)->finalize(object);
}

static void app_info_class_init(AppInfoClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;

    object_class->dispose = app_info_dispose;
    object_class->finalize = app_info_finalize;
}

static void app_info_init(AppInfo *self)
{
}

/*
 * Public functions
 */

AppInfo *app_info_new(const gchar *app_id, const gchar *name,
                      const gchar *icon_path, const gchar *command,
                      gboolean dbus_activated, gboolean systemd_activated,
                      gboolean graphical)
{
    AppInfo *self = g_object_new(APPLAUNCHD_TYPE_APP_INFO, NULL);

    self->app_id = g_strdup(app_id);
    self->name = g_strdup(name);
    self->icon_path = g_strdup(icon_path);
    self->command = g_strdup(command);
    self->dbus_activated = dbus_activated;
    self->systemd_activated = systemd_activated;
    self->graphical = graphical;

    return self;
}

const gchar *app_info_get_app_id(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), NULL);

    return self->app_id;
}

const gchar *app_info_get_name(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), NULL);

    return self->name;
}

const gchar *app_info_get_icon_path(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), NULL);

    return self->icon_path;
}

const gchar *app_info_get_command(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), NULL);

    return self->command;
}

gboolean app_info_get_dbus_activated(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), FALSE);

    return self->dbus_activated;
}

gboolean app_info_get_systemd_activated(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), FALSE);

    return self->systemd_activated;
}

gboolean app_info_get_graphical(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), FALSE);

    return self->graphical;
}

AppStatus app_info_get_status(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), APP_STATUS_INACTIVE);

    return self->status;
}

gpointer app_info_get_runtime_data(AppInfo *self)
{
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(self), NULL);

    return self->runtime_data;
}

void app_info_set_runtime_data(AppInfo *self, gpointer runtime_data)
{
    g_return_if_fail(APPLAUNCHD_IS_APP_INFO(self));

    self->runtime_data = runtime_data;
}

void app_info_set_status(AppInfo *self, AppStatus status)
{
    g_return_if_fail(APPLAUNCHD_IS_APP_INFO(self));

    self->status = status;
}
