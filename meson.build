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

project (
  'applaunchd',
  'c',
  version : '0.1.0',
  license : 'Apache-2.0',
  meson_version : '>= 0.46.0',
  default_options :
    [
      'warning_level=1',
      'buildtype=debugoptimized',
      'c_std=gnu11'
    ],
)

cc = meson.get_compiler('c')

app_name = meson.project_name()

prefix = get_option('prefix')
datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
bindir = join_paths(prefix, get_option('bindir'))
ifacedir = join_paths(prefix, join_paths(datadir, 'dbus-1', 'interfaces'))
servicedir = join_paths(prefix, join_paths(datadir, 'dbus-1', 'services'))

if datadir.startswith('/')
  full_datadir = datadir
else
  full_datadir = join_paths(prefix, datadir)
endif

if sysconfdir.startswith('/')
  full_sysconfdir = sysconfdir
else
  full_sysconfdir = join_paths(prefix, sysconfdir)
endif

config_data = configuration_data()
config_data.set_quoted('APP_DATA_NAME', app_name)
config_data.set_quoted('DATADIR', full_datadir)
config_data.set_quoted('SYSCONFDIR', full_sysconfdir)

config_h = configure_file (
    output: 'config.h',
    configuration: config_data
)

subdir('data')
subdir('src')
