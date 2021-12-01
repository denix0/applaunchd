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

gnome = import('gnome')

# DBus server interface
dbus_prefix = 'org.automotivelinux.'
dbus_namespace = 'applaunchd'

generated_dbus_sources = []

dbus_interfaces = [ 'org.automotivelinux.AppLaunch.xml' ]

generated_dbus_sources += gnome.gdbus_codegen('applaunch-dbus',
    sources          : dbus_interfaces,
    object_manager   : false,
    interface_prefix : 'org.automotivelinux.',
    install_header   : false,
    namespace        : 'applaunchd')

dbus_header_dir = meson.current_build_dir()
dbus_inc = include_directories('.')
install_data(dbus_interfaces, install_dir: ifacedir)

# Application DBus interface
generated_dbus_sources += gnome.gdbus_codegen('fdo-dbus',
    sources          : [ 'org.freedesktop.Application.xml' ],
    object_manager   : false,
    interface_prefix : 'org.freedesktop.',
    install_header   : false,
    namespace        : 'fdo')

# systemd service file
service_data = configuration_data()
service_data.set('bindir', bindir)
service_file = 'org.automotivelinux.AppLaunch.service'

configure_file(
  input : service_file + '.in',
  output : service_file,
  configuration : service_data,
  install : true,
  install_dir: servicedir,
)
