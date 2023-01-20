
source_files = files(
  'main.c',
  'oauth-ask-authorization-dialog.c',
  'oauth-account.c',
  'oauth-account-chooser-dialog.c',
  'oauth-account-manager-dialog.c',
  'oauth-consumer.c',
  'oauth-service.c',
  'web-service.c'
)

oauth_mod = shared_module('oauth',
  sources : source_files,
  dependencies : [
    extension_common_deps,
    jpeg_deps,
    libsoup_dep,
    webkit2_dep,
    use_libsecret ? libsecret_dep : [],
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'oauth.extension.desktop.in.in',
  output : 'oauth.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'oauth.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
