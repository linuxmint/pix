
install_man('pix.1')

# .desktop files

desktop_data = configuration_data()
desktop_data.set('VERSION', meson.project_version())
desktop_in_file = configure_file(
  input : 'pix.desktop.in.in',
  output : 'pix.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_in_file,
  output : 'pix.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

desktop_import_in_file = configure_file(
  input : 'pix-import.desktop.in.in',
  output : 'pix-import.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_import_in_file,
  output : 'pix-import.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

# .pc file

pkg = import('pkgconfig')
pix_pc = pkg.generate(
  name : 'Pix',
  description : 'Pix extensions',
  filebase : meson.project_name(),
  subdirs : meson.project_name(),
  requires : [ 'gtk+-3.0' ],
  version: meson.project_version(),
  variables : [
    '='.join(['extensionsdir', extensions_install_dir]),
    '='.join(['uidir', ui_install_dir]),
    '='.join(['apiversion', api_version])
  ]
)

# Subdirectories

subdir('gschemas')
subdir('icons')
subdir('ui')
