source_files = files(
  'exiv2-utils.cpp',
  'gth-edit-iptc-page.c',
  'gth-metadata-provider-exiv2.c',
  'main.c'
)

exiv2_tools_mod = shared_module('exiv2_tools',
  sources : source_files,
  dependencies : [ extension_common_deps, exiv2_dep ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  cpp_args : cpp_args,
  link_with : [ edit_metadata_mod ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'exiv2_tools.extension.desktop.in.in',
  output : 'exiv2_tools.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'exiv2_tools.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
