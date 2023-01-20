project('example', ['c'],
  license : 'GPL2+',
  version : '1.0',
  meson_version : '>=0.43'
)

gnome = import('gnome')
i18n = import('i18n')
gettext_package = meson.project_name()

extensions_install_dir = dependency('gthumb').get_pkgconfig_variable('extensionsdir')
extension_data = configuration_data()
extension_data.set('GTHUMB_API', dependency('gthumb').get_pkgconfig_variable('apiversion'))

extension_deps = [
  dependency('gthumb'),
]

po_dir = join_paths(meson.source_root(), 'po')

config_data = configuration_data()
config_data.set_quoted('PACKAGE_NAME', meson.project_name())
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config_data.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_data.set_quoted('EXTENSION_LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_file = configure_file(output : 'config.h', configuration : config_data)
config_inc = include_directories('.')

subdir('src')
subdir('po')
