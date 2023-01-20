
source_files = files(
  'actions.c',
  'albumtheme-private.c',
  'callbacks.c',
  'dlg-web-exporter.c',
  'gth-web-exporter.c',
  'main.c'
)

albumtheme_lexer = custom_target('albumtheme_lexer',
  input : 'albumtheme.l',
  output : 'albumtheme-lex.c',
  command : [ flex, '-Pgth_albumtheme_yy', '-o@OUTPUT@', '@INPUT@' ]
)

albumtheme_parser = custom_target('albumtheme_parser',
   input : 'albumtheme.y',
   output : 'albumtheme.c',
   command : [ bison, '@INPUT@', '-pgth_albumtheme_yy', '-o@OUTPUT@' ],
   depends : albumtheme_lexer
)

shared_module('webalbums',
  sources : [ source_files, albumtheme_parser ],
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : [ export_tools_mod, image_rotation_mod ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'webalbums.extension.desktop.in.in',
  output : 'webalbums.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'webalbums.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
