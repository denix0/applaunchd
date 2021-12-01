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

#include "app_launcher.h"
#include "process_manager.h"

struct _ProcessManager {
    GObject parent_instance;

    GList *process_data;
};

G_DEFINE_TYPE(ProcessManager, process_manager, G_TYPE_OBJECT);

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
struct process_runtime_data {
    guint watcher;
    GPid pid;
    const gchar *app_id;
};

/*
 * Initialization & cleanup functions
 */

static void process_manager_dispose(GObject *object)
{
    ProcessManager *self = APPLAUNCHD_PROCESS_MANAGER(object);

    g_return_if_fail(APPLAUNCHD_IS_PROCESS_MANAGER(self));

    if (self->process_data)
        g_list_free_full(g_steal_pointer(&self->process_data), g_free);

    G_OBJECT_CLASS(process_manager_parent_class)->dispose(object);
}

static void process_manager_finalize(GObject *object)
{
    G_OBJECT_CLASS(process_manager_parent_class)->finalize(object);
}

static void process_manager_class_init(ProcessManagerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;

    object_class->dispose = process_manager_dispose;
    object_class->finalize = process_manager_finalize;

    signals[STARTED] = g_signal_new("started", G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST, 0 ,
                                    NULL, NULL, NULL, G_TYPE_NONE,
                                    1, G_TYPE_STRING);

    signals[TERMINATED] = g_signal_new("terminated", G_TYPE_FROM_CLASS (klass),
                                       G_SIGNAL_RUN_LAST, 0 ,
                                       NULL, NULL, NULL, G_TYPE_NONE,
                                       1, G_TYPE_STRING);
}

static void process_manager_init(ProcessManager *self)
{
}

/*
 * Internal functions
 */

static const gchar *get_app_id_for_pid(ProcessManager *self, GPid pid)
{
    g_return_val_if_fail(APPLAUNCHD_IS_PROCESS_MANAGER(self), NULL);

    AppLauncher *app_launcher = app_launcher_get_default();
    guint len = g_list_length(self->process_data);

    for (guint i = 0; i < len; i++) {
        struct process_runtime_data *runtime_data =
                                g_list_nth_data(self->process_data, i);

        if (runtime_data->pid == pid)
            return runtime_data->app_id;
    }

    return NULL;
}

/*
 * Internal callbacks
 */

/*
 * This function is called when a watched process terminated, so we can:
 *   - cleanup this application's data (and reap the process so it
 *     doesn't become a zombie)
 *   - notify listeners that the process terminated
 */
static void process_manager_app_terminated_cb(GPid pid,
                                              gint wait_status,
                                              gpointer data)
{
    ProcessManager *self = data;
    AppLauncher *app_launcher = app_launcher_get_default();
    struct process_runtime_data *runtime_data;
    const gchar *app_id;
    AppInfo *app_info;

    g_return_if_fail(APPLAUNCHD_IS_PROCESS_MANAGER(self));

    app_id = get_app_id_for_pid(self, pid);
    if (!app_id) {
        g_warning("Unable to retrieve app id for pid %d", pid);
        return;
    }

    app_info = app_launcher_get_app_info(app_launcher, app_id);
    if (!app_info) {
        g_warning("Unable to find running app with pid %d", pid);
        return;
    }

    if (g_spawn_check_exit_status(wait_status, NULL))
        g_debug("Application '%s' terminated with exit code %i",
                app_id, WEXITSTATUS(wait_status));
    else
        g_warning("Application '%s' crashed", app_id);

    g_spawn_close_pid(pid);

    runtime_data = app_info_get_runtime_data(app_info);
    g_source_remove(runtime_data->watcher);

    app_info_set_status(app_info, APP_STATUS_INACTIVE);
    app_info_set_runtime_data(app_info, NULL);

    self->process_data = g_list_remove(self->process_data, runtime_data);
    g_free(runtime_data);

    g_signal_emit(self, signals[TERMINATED], 0, app_id);
}

/*
 * Public functions
 */

ProcessManager *process_manager_new(void)
{
    return g_object_new(APPLAUNCHD_TYPE_PROCESS_MANAGER, NULL);
}

/*
 * Start an application by executing the provided command line.
 */
gboolean process_manager_start_app(ProcessManager *self,
                                   AppInfo *app_info)
{
    g_return_val_if_fail(APPLAUNCHD_IS_PROCESS_MANAGER(self), FALSE);
    g_return_val_if_fail(APPLAUNCHD_IS_APP_INFO(app_info), FALSE);

    gboolean success;
    g_autofree GStrv args = NULL;
    const gchar *app_id = app_info_get_app_id(app_info);
    const gchar *command = app_info_get_command(app_info);
    struct process_runtime_data *runtime_data;

    runtime_data = g_new0(struct process_runtime_data, 1);
    if (!runtime_data) {
        g_critical("Unable to allocate runtime data structure for '%s'",
                   app_id);
        return FALSE;
    }

    runtime_data->app_id = app_id;

    args = g_strsplit(command, " ", -1);
    success = g_spawn_async(NULL, args, NULL,
                            G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                            NULL, NULL, &runtime_data->pid, NULL);
    if (!success) {
        g_critical("Unable to start application '%s'", app_id);
        g_free(runtime_data);
        return FALSE;
    }

    /*
     * Add a watcher for the child PID in order to get notified when it dies
     */
    runtime_data->watcher = g_child_watch_add(runtime_data->pid,
                                              process_manager_app_terminated_cb,
                                              self);
    self->process_data = g_list_append(self->process_data, runtime_data);
    app_info_set_runtime_data(app_info, runtime_data);

    g_signal_emit(self, signals[STARTED], 0, app_id);

    return TRUE;
}
