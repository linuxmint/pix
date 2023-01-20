source_files = files(
  'callbacks.c',
  'main.c'
)

gresource_files = gnome.compile_resources('export-tools-resources', 'resources.xml', c_name : 'gth')

export_tools_mod = shared_module('export_tools',
  sources : [ source_files, gresource_files ],
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'export_tools.extension.desktop.in.in',
  output : 'export_tools.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'export_tools.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
