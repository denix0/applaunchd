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

#include "utils.h"

/* Search by descending quality level */
static const gchar *icon_sizes[] = {
    "scalable",
    "512x512",
    "256x256",
    "192x192",
    "128x128",
    "96x96",
    "72x72",
    "64x64",
    "48x48",
    "32x32",
    "24x24",
    "16x16",
    "symbolic",
    NULL
};

/*
 * Recursively search the `base_path` folder for a file whose name
 * starts with `icon_name`: this way we don't care about the extension
 * and can also get files with an extended name (for example,
 * "`icon_name`-symbolic.png" would still be recognized as a valid match)
 */
static gchar *find_icon(const gchar *base_path, const gchar *icon_name)
{
    g_autoptr(GFile) base_dir = g_file_new_for_path(base_path);
    g_autoptr(GFileEnumerator) child_list =
            g_file_enumerate_children(base_dir, "*", G_FILE_QUERY_INFO_NONE,
                                      NULL, NULL);

    if (!child_list)
        return NULL;

    while (TRUE) {
        GFile *current_file;
        GFileInfo *current_file_info;

        g_file_enumerator_iterate(child_list, &current_file_info,
                                  &current_file, NULL, NULL);
        if (!current_file_info || !current_file)
            break;

        if (g_file_info_get_file_type(current_file_info) == G_FILE_TYPE_DIRECTORY) {
            gchar *icon_path = find_icon(g_file_get_path(current_file), icon_name);
            if (icon_path)
                return icon_path;
        } else if (g_str_has_prefix(g_file_get_basename(current_file), icon_name)) {
            return g_strdup(g_file_get_path(current_file));
        }
    }

    return NULL;
}

/*
 * Search through a list of folders for the `icon_name` icon file
 */
gchar *applaunchd_utils_get_icon(GStrv dir_list, const gchar *icon_name)
{
    gchar *icon_path = NULL;

    for (GStrv base_path = dir_list; *base_path != NULL ; base_path++) {
        g_autoptr(GFile) base_dir = g_file_new_build_filename(*base_path,
                                                              "icons", NULL);

        g_autoptr(GFileEnumerator) child_list =
                g_file_enumerate_children(base_dir, "standard::type=directory",
                                          G_FILE_QUERY_INFO_NONE, NULL, NULL);

        if (!child_list)
            continue;

        while (TRUE) {
            GFile *theme_dir;
            g_file_enumerator_iterate(child_list, NULL, &theme_dir,
                                      NULL, NULL);
            if (!theme_dir)
                break;

            for (gint i = 0; icon_sizes[i]; i++) {
                g_autofree gchar *theme_path =
                        g_build_filename(g_file_get_path(theme_dir),
                                         icon_sizes[i], NULL);
                icon_path = find_icon(theme_path, icon_name);
                if (icon_path)
                    return icon_path;
            }
        }
    }

    return NULL;
}
