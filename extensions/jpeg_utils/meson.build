source_files = files(
  'jmemorydest.c',
  'jmemorysrc.c',
  'jpeg-info.c',
  'jpegtran.c',
  'main.c'
)

if have_libjpeg_80
  source_files += files('transupp-80.c')
else
  source_files += files('transupp-62.c')
endif

jpeg_utils_mod = shared_module('jpeg_utils',
  sources : source_files,
  dependencies : [
    extension_common_deps,
    jpeg_deps
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'jpeg_utils.extension.desktop.in.in',
  output : 'jpeg_utils.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'jpeg_utils.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
