#
# Copyright (C) 2021 Collabora Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

applaunchd_deps = [
    dependency('gobject-2.0'),
    dependency('gio-unix-2.0'),
    dependency('libsystemd'),
]

executable (
    'applaunchd',
    config_h,
    [
        generated_dbus_sources,
        'main.c',
        'app_info.c', 'app_info.h',
        'app_launcher.c', 'app_launcher.h',
        'process_manager.c', 'process_manager.h',
        'systemd_manager.c', 'systemd_manager.h',
        'utils.c', 'utils.h',
    ],
    dependencies : applaunchd_deps,
    include_directories : include_directories('..'),
    install : true
)
