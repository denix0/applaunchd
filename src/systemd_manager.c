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

#include <systemd/sd-bus.h>

#include "app_launcher.h"
#include "systemd_manager.h"

struct _SystemdManager {
    GObject parent_instance;

/* we don't need the list here    GList *process_data; */
};

G_DEFINE_TYPE(SystemdManager, systemd_manager, G_TYPE_OBJECT);

enum {
  STARTED,
  TERMINATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

/*
 * Application info structure, used for storing relevant data
 * in the `running_apps` list
 */
struct systemd_runtime_data {
    const gchar *esc_service;
    SystemdManager *mgr;
    sd_bus_slot *slot;
};

/*
 * Initialization & cleanup functions
 */

static void systemd_manager_dispose(GObject *object)
{
    SystemdManager *self = APPLAUNCHD_SYSTEMD_MANAGER(object);

    g_return_if_fail(APPLAUNCHD_IS_SYSTEMD_MANAGER(self));

    G_OBJECT_CLASS(systemd_manager_parent_class)->dispose(object);
}

static void systemd_manager_finalize(GObject *object)
{
    G_OBJECT_CLASS(systemd_manager_parent_class)->finalize(object);
}

static void systemd_manager_class_init(SystemdManagerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;

    object_class->dispose = systemd_manager_dispose;
    object_class->finalize = systemd_manager_finalize;

    signals[STARTED] = g_signal_new("started", G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST, 0 ,
                                    NULL, NULL, NULL, G_TYPE_NONE,
                                    1, G_TYPE_STRING);

    signals[TERMINATED] = g_signal_new("terminated", G_TYPE_FROM_CLASS (klass),
                                       G_SIGNAL_RUN_LAST, 0 ,
                                       NULL, NULL, NULL, G_TYPE_NONE,
                                       1, G_TYPE_STRING);
}

static void systemd_manager_init(SystemdManager *self)
{
}

/*
 * Internal callbacks
 */

/*
 * This function is called when "PropertiesChanged" signal happens for
 * the matched Unit - check its "ActiveState" to update the app status
 */
int systemd_manager_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    AppLauncher *launcher = app_launcher_get_default();
    sd_bus_error err = SD_BUS_ERROR_NULL;
    char* msg = NULL;
    AppInfo *app_info = userdata;
    struct systemd_runtime_data *data;

    data = app_info_get_runtime_data(app_info);
    if(!data)
    {
        g_critical("Couldn't find runtime data for %s!", app_info_get_app_id(app_info));
        return 0;
    }

    sd_bus_get_property_string(
            app_launcher_get_bus(launcher),   /* bus */
            "org.freedesktop.systemd1",       /* destination */
            data->esc_service,                /* path */
            "org.freedesktop.systemd1.Unit",  /* interface */
            "ActiveState",                    /* member */
            &err, 
            &msg
    );

    if(!g_strcmp0(msg, "inactive"))
    {
        g_debug("Application %s has terminated", app_info_get_app_id(app_info));
        app_info_set_status(app_info, APP_STATUS_INACTIVE);
        app_info_set_runtime_data(app_info, NULL);

        g_signal_emit(data->mgr, signals[TERMINATED], 0, app_info_get_app_id(app_info));
        systemd_manager_free_runtime_data(data);
    }
    else if(!g_strcmp0(msg, "active"))
    {
        /* PropertiesChanged signal gets triggered multiple times, only handle it once */
        if(app_info_get_status(app_info) != APP_STATUS_RUNNING)
        {
            g_debug("Application %s has started", app_info_get_app_id(app_info));
            app_info_set_status(app_info, APP_STATUS_RUNNING);
            g_signal_emit(data->mgr, signals[STARTED], 0, app_info_get_app_id(app_info));
        }
    }
    return 0;
}

/*
 * Public functions
 */

SystemdManager *systemd_manager_new(void)
{
    return g_object_new(APPLAUNCHD_TYPE_SYSTEMD_MANAGER, NULL);
}

/*
 * Start an application by executing the provided command line.
 */
gboolean systemd_manager_start_app(SystemdManager *self,
                                   AppInfo *app_info)
{
    g_return_val_if_fail(APPLAUNCHD_IS_SYSTEMD_MANAGER(self), FALSE);
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(app_info), FALSE);

    AppLauncher *launcher = app_launcher_get_default();
    g_autofree gchar *service = NULL;
    gchar *esc_service = NULL;
    const gchar *app_id = app_info_get_app_id(app_info);
    const gchar *command = app_info_get_command(app_info);
    struct systemd_runtime_data *runtime_data;

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    const char *path;
    int r;

    runtime_data = g_new0(struct systemd_runtime_data, 1);
    if (!runtime_data) {
        g_critical("Unable to allocate runtime data structure for '%s'", app_id);
        return FALSE;
    }

    /* Compose the corresponding service name */
    service = g_strdup_printf("agl-app@%s.service", command);
    /* Get the escaped unit name in the systemd hierarchy */
    sd_bus_path_encode("/org/freedesktop/systemd1/unit", service, &esc_service);

    runtime_data->mgr = self;
    runtime_data->esc_service = esc_service;

    r = sd_bus_call_method(
            app_launcher_get_bus(launcher),       /* bus */
            "org.freedesktop.systemd1",           /* service to contact */
            "/org/freedesktop/systemd1",          /* object path */
            "org.freedesktop.systemd1.Manager",   /* interface name */
            "StartUnit",                          /* method name */
            &error,                               /* object to return error in */
            &m,                                   /* return message on success */
            "ss",                                 /* input signature */
            service,                              /* first argument */
            "replace"                             /* second argument */
    );
    if (r < 0) {
        g_critical("Failed to issue method call: %s", error.message);
        goto finish;
    }

    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        g_critical("Failed to parse response message: %s", strerror(-r));
        goto finish;
    }

    r = sd_bus_match_signal(
            app_launcher_get_bus(launcher),    /* bus */
            &runtime_data->slot,               /* slot */
            NULL,                              /* sender */
            esc_service,                       /* path */
            "org.freedesktop.DBus.Properties", /* interface */
            "PropertiesChanged",               /* member */
            systemd_manager_cb,                /* callback */
            app_info                           /* userdata */
    );
    if (r < 0) {
        g_critical("Failed to set match signal: %s", strerror(-r));
        goto finish;
    }

    app_info_set_runtime_data(app_info, runtime_data);

    /* The application is now starting, wait for notification to mark it running */
    g_debug("Application %s is now being started", app_info_get_app_id(app_info));
    app_info_set_status(app_info, APP_STATUS_STARTING);
    return TRUE;

finish:
    if(runtime_data->esc_service)
        g_free(runtime_data->esc_service);
    g_free(runtime_data);
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    return FALSE;
}

void systemd_manager_free_runtime_data(gpointer data)
{
    struct systemd_runtime_data *runtime_data = data;

    g_return_if_fail(runtime_data != NULL);

    sd_bus_slot_unref(runtime_data->slot);
    g_free(runtime_data->esc_service);
    g_free(runtime_data);
}
