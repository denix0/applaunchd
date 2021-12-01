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

#include "app_launcher.h"
#include "dbus_activation_manager.h"
#include "fdo-dbus.h"

struct _DBusActivationManager {
    GObject parent_instance;
};

G_DEFINE_TYPE(DBusActivationManager, dbus_activation_manager, G_TYPE_OBJECT);

enum {
    STARTED,
    TERMINATED,
    N_SIGNALS
};
static guint signals[N_SIGNALS];

/*
 * Application info structure, used for storing relevant data
 * in the `starting_apps` and `running_apps` lists
 */
struct dbus_runtime_data {
    guint watcher;
    fdoApplication *fdo_proxy;
};

/*
 * Initialization & cleanup functions
 */

static void dbus_activation_manager_dispose(GObject *object)
{
    G_OBJECT_CLASS(dbus_activation_manager_parent_class)->dispose(object);
}

static void dbus_activation_manager_finalize(GObject *object)
{
    G_OBJECT_CLASS(dbus_activation_manager_parent_class)->finalize(object);
}

static void dbus_activation_manager_class_init(DBusActivationManagerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;

    object_class->dispose = dbus_activation_manager_dispose;
    object_class->finalize = dbus_activation_manager_finalize;

    signals[STARTED] = g_signal_new("started", G_TYPE_FROM_CLASS(klass),
                                    G_SIGNAL_RUN_LAST, 0 ,
                                    NULL, NULL, NULL, G_TYPE_NONE,
                                    1, G_TYPE_STRING);

    signals[TERMINATED] = g_signal_new("terminated", G_TYPE_FROM_CLASS(klass),
                                       G_SIGNAL_RUN_LAST, 0,
                                       NULL, NULL, NULL, G_TYPE_NONE,
                                       1, G_TYPE_STRING);
}

static void dbus_activation_manager_init(DBusActivationManager *self)
{
}

/*
 * Internal callbacks
 */

/*
 * This function is called when a D-Bus name we're watching just vanished
 * from the session bus. This indicates the underlying application terminated,
 * so we must remove it from the list of running apps and notify listeners
 * of the app termination so they can act accordingly.
 */
static void dbus_activation_manager_app_terminated_cb(GDBusConnection *connection,
                                                      const gchar *name,
                                                      gpointer data)
{
    DBusActivationManager *self = data;
    AppLauncher *app_launcher = app_launcher_get_default();
    AppInfo *app_info;
    struct dbus_runtime_data *runtime_data;

    g_return_if_fail(APPLAUNCHD_IS_DBUS_ACTIVATION_MANAGER(self));
    g_return_if_fail(APPLAUNCHD_IS_APP_LAUNCHER(app_launcher));

    app_info = app_launcher_get_app_info(app_launcher, name);
    g_return_if_fail(APPLAUNCHD_IS_APP_INFO(app_info));

    runtime_data = app_info_get_runtime_data(app_info);
    g_debug("Application '%s' vanished from D-Bus", name);

    app_info_set_status(app_info, APP_STATUS_INACTIVE);
    app_info_set_runtime_data(app_info, NULL);

    dbus_activation_manager_free_runtime_data(runtime_data);

    g_signal_emit(self, signals[TERMINATED], 0, name);
}

/*
 * Function called when the name appeared on D-Bus, meaning the application
 * successfully started and registered its D-Bus service.
 */
static void dbus_activation_manager_app_started_cb(GDBusConnection *connection,
                                                   const gchar *name,
                                                   const gchar *name_owner,
                                                   gpointer data)
{
    DBusActivationManager *self = data;
    AppLauncher *app_launcher = app_launcher_get_default();
    AppInfo *app_info;
    struct dbus_runtime_data *runtime_data;

    g_return_if_fail(APPLAUNCHD_IS_DBUS_ACTIVATION_MANAGER(self));
    g_return_if_fail(APPLAUNCHD_IS_APP_LAUNCHER(app_launcher));

    app_info = app_launcher_get_app_info(app_launcher, name);
    g_return_if_fail(APPLAUNCHD_IS_APP_INFO(app_info));

    runtime_data = app_info_get_runtime_data(app_info);

    g_debug("Application '%s' appeared on D-Bus", name);
    app_info_set_status(app_info, APP_STATUS_RUNNING);
    dbus_activation_manager_activate_app(self, app_info);
}

/*
 * Public functions
 */

DBusActivationManager *dbus_activation_manager_new(void)
{
    return g_object_new(APPLAUNCHD_TYPE_DBUS_ACTIVATION_MANAGER, NULL);
}

/*
 * Start an application using D-Bus activation.
 */
gboolean dbus_activation_manager_start_app(DBusActivationManager *self,
                                           AppInfo *app_info)
{
    g_return_val_if_fail(APPLAUNCHD_IS_DBUS_ACTIVATION_MANAGER(self), FALSE);
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(app_info), FALSE);

    g_autoptr(GError) error = NULL;
    const gchar *app_id = app_info_get_app_id(app_info);
    struct dbus_runtime_data *runtime_data = g_new0(struct dbus_runtime_data, 1);

    if (!runtime_data) {
        g_critical("Unable to allocate runtime data structure for '%s'",
                   app_id);
        return FALSE;
    }

    /*
     * g_bus_watch_name() requests D-Bus activation (due to
     * G_BUS_NAME_WATCHER_FLAGS_AUTO_START) and subscribes to name
     * owner changes so we get notified when the requested application
     * appears and vanishes from D-Bus.
     */
    runtime_data->watcher = g_bus_watch_name(G_BUS_TYPE_SESSION, app_id,
                                             G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                             dbus_activation_manager_app_started_cb,
                                             dbus_activation_manager_app_terminated_cb,
                                             self, NULL);
    if (runtime_data->watcher == 0) {
        g_critical("Unable to request D-Bus activation for '%s'", app_id);
        dbus_activation_manager_free_runtime_data(runtime_data);
        return FALSE;
    }

    /* Update application status */
    app_info_set_status(app_info, APP_STATUS_STARTING);
    app_info_set_runtime_data(app_info, runtime_data);

    return TRUE;
}

/*
 * Once an application has been started through D-Bus, we must activate it
 * so it shows its main window, if any.
 * This function doesn't raise an error as headless applications will likely
 * not implement the org.freedesktop.Application interface.
 */
gboolean dbus_activation_manager_activate_app(DBusActivationManager *self,
                                              AppInfo *app)
{
    g_return_val_if_fail(APPLAUNCHD_IS_DBUS_ACTIVATION_MANAGER(self), FALSE);
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(app), FALSE);

    g_autoptr(GError) error = NULL;
    struct dbus_runtime_data *runtime_data = app_info_get_runtime_data(app);
    const gchar *app_id = app_info_get_app_id(app);

    g_return_val_if_fail(runtime_data != NULL, FALSE);

    /* Base object for interface "org.example.Iface" is "/org/example/Iface" */
    g_autofree gchar *path = g_strconcat("/", app_id, NULL);
    g_strdelimit(path, ".", '/');

    if (!runtime_data->fdo_proxy) {
        runtime_data->fdo_proxy =
                fdo_application_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       app_id, path, NULL,
                                                       &error);
    }

    if (runtime_data->fdo_proxy) {
        GVariantDict dict;

        g_variant_dict_init(&dict, NULL);

        fdo_application_call_activate_sync (runtime_data->fdo_proxy,
                                            g_variant_dict_end(&dict),
                                            NULL, &error);
        if (error) {
            g_warning("Error activating application %s: %s", app_id,
                      error->message);
        }
    } else if (error) {
        g_warning("Error creating D-Bus proxy for %s: %s", app_id,
                    error->message);
    }

    g_signal_emit(self, signals[STARTED], 0, app_id);

    return TRUE;
}

void dbus_activation_manager_free_runtime_data(gpointer data)
{
    struct dbus_runtime_data *runtime_data = data;

    g_return_if_fail(runtime_data != NULL);

    g_clear_object(&runtime_data->fdo_proxy);
    if (runtime_data->watcher > 0)
        g_bus_unwatch_name(runtime_data->watcher);
    g_free(runtime_data);
}
