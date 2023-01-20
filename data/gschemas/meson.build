schemas_dir = join_paths(datadir, 'glib-2.0', 'schemas')

gschema_files = files(
  'org.gnome.gthumb.gschema.xml',
  'org.gnome.gthumb.catalogs.gschema.xml',
  'org.gnome.gthumb.change-date.gschema.xml',
  'org.gnome.gthumb.comments.gschema.xml',
  'org.gnome.gthumb.contact-sheet.gschema.xml',
  'org.gnome.gthumb.convert-format.gschema.xml',
  'org.gnome.gthumb.crop.gschema.xml',
  'org.gnome.gthumb.facebook.gschema.xml',
  'org.gnome.gthumb.file-manager.gschema.xml',
  'org.gnome.gthumb.flickr.gschema.xml',
  'org.gnome.gthumb.gstreamer-tools.gschema.xml',
  'org.gnome.gthumb.image-print.gschema.xml',
  'org.gnome.gthumb.image-viewer.gschema.xml',
  'org.gnome.gthumb.importer.gschema.xml',
  'org.gnome.gthumb.photo-importer.gschema.xml',
  'org.gnome.gthumb.picasaweb.gschema.xml',
  'org.gnome.gthumb.pixbuf-savers.gschema.xml',
  'org.gnome.gthumb.rename-series.gschema.xml',
  'org.gnome.gthumb.resize.gschema.xml',
  'org.gnome.gthumb.resize-images.gschema.xml',
  'org.gnome.gthumb.rotate.gschema.xml',
  'org.gnome.gthumb.slideshow.gschema.xml',
  'org.gnome.gthumb.terminal.gschema.xml',
  'org.gnome.gthumb.webalbums.gschema.xml'
)

enums_schema = custom_target('enums_schema',
  output : 'org.gnome.gthumb.enums.xml',
  command : [
    find_program('make-enums.py'),
    join_paths(meson.source_root(), 'gthumb'),
    join_paths(meson.source_root(), 'extensions'),
  ],
  capture : true,
  install : true,
  install_dir : schemas_dir
)

install_data(gschema_files, install_dir : schemas_dir)
