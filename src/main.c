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

#include <glib.h>
#include <glib-unix.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "app_launcher.h"
#include "applaunch-dbus.h"

#define APPLAUNCH_DBUS_NAME "org.automotivelinux.AppLaunch"
#define APPLAUNCH_DBUS_PATH "/org/automotivelinux/AppLaunch"

typedef struct SDEventSource {
  GSource source;
  GPollFD pollfd;
  sd_event *event;
  sd_bus *bus;
} SDEventSource;

GMainLoop *main_loop = NULL;

static gboolean quit_cb(gpointer user_data)
{
    g_info("Quitting...");

    if (main_loop)
        g_idle_add(G_SOURCE_FUNC(g_main_loop_quit), main_loop);
    else
        exit(0);

    return G_SOURCE_REMOVE;
}

static void bus_acquired_cb(GDBusConnection *connection, const gchar *name,
                            gpointer user_data)
{
    AppLauncher *launcher = user_data;

    g_debug("Bus acquired, starting service...");
    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(launcher),
                                     connection, APPLAUNCH_DBUS_PATH, NULL);
}

static void name_acquired_cb(GDBusConnection *connection, const gchar *name,
                             gpointer user_data)
{
    g_debug("D-Bus name '%s' was acquired", name);
}

static void name_lost_cb(GDBusConnection *connection, const gchar *name,
                         gpointer user_data)
{
    g_critical("Lost the '%s' service name, quitting...", name);
    g_main_loop_quit(main_loop);
}

static gboolean event_prepare(GSource *source, gint *timeout_) {
  return sd_event_prepare(((SDEventSource *)source)->event) > 0;
}

static gboolean event_check(GSource *source) {
  return sd_event_wait(((SDEventSource *)source)->event, 0) > 0;
}

static gboolean event_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
  g_critical("sd event dispatch");
  return sd_event_dispatch(((SDEventSource *)source)->event) > 0;
}

static void event_finalize(GSource *source) {
  sd_event_unref(((SDEventSource *)source)->event);
}

static GSourceFuncs event_funcs = {
  .prepare = event_prepare,
  .check = event_check,
  .dispatch = event_dispatch,
  .finalize = event_finalize,
};

GSource *g_sd_event_create_source(sd_event *event, sd_bus *bus) {
  SDEventSource *source;

  source = (SDEventSource *)g_source_new(&event_funcs, sizeof(SDEventSource));

  source->event = sd_event_ref(event);
  source->bus = sd_bus_ref(bus);
  source->pollfd.fd = sd_bus_get_fd(bus);
  source->pollfd.events = sd_bus_get_events(bus);

  g_source_add_poll((GSource *)source, &source->pollfd);

  return (GSource *)source;
}

int main(int argc, char *argv[])
{
    g_unix_signal_add(SIGTERM, quit_cb, NULL);
    g_unix_signal_add(SIGINT, quit_cb, NULL);

    main_loop = g_main_loop_new(NULL, FALSE);

    AppLauncher *launcher = app_launcher_get_default();

    gint owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, APPLAUNCH_DBUS_NAME,
                                   G_BUS_NAME_OWNER_FLAGS_NONE, bus_acquired_cb,
                                   name_acquired_cb, name_lost_cb,
                                   launcher, NULL);

    g_main_loop_run(main_loop);
    g_main_loop_unref(main_loop);

    g_object_unref(launcher);

    g_bus_unown_name (owner_id);

    return 0;
}
