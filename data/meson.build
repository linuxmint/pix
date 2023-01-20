
install_man('gthumb.1')

# .desktop files

desktop_data = configuration_data()
desktop_data.set('VERSION', meson.project_version())
desktop_in_file = configure_file(
  input : 'org.gnome.gThumb.desktop.in.in',
  output : 'org.gnome.gThumb.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_in_file,
  output : 'org.gnome.gThumb.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

desktop_import_in_file = configure_file(
  input : 'org.gnome.gThumb.Import.desktop.in.in',
  output : 'org.gnome.gThumb.Import.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_import_in_file,
  output : 'org.gnome.gThumb.Import.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

# .pc file

pkg = import('pkgconfig')
gthumb_pc = pkg.generate(
  name : 'gThumb',
  description : 'gThumb extensions',
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

subdir('appdata')
subdir('gschemas')
subdir('icons')
subdir('ui')
