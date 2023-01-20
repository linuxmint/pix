source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-personalize-scripts.c',
  'gth-script.c',
  'gth-script-editor-dialog.c',
  'gth-script-file.c',
  'gth-script-task.c',
  'main.c'
)

gresource_files = gnome.compile_resources('list-tools-resources', 'resources.xml', c_name : 'gth')

list_tools_mod = shared_module('list_tools',
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
  input : 'list_tools.extension.desktop.in.in',
  output : 'list_tools.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'list_tools.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
