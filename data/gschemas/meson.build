schemas_dir = join_paths(datadir, 'glib-2.0', 'schemas')

gschema_files = files(
  'org.x.pix.gschema.xml',
  'org.x.pix.catalogs.gschema.xml',
  'org.x.pix.change-date.gschema.xml',
  'org.x.pix.comments.gschema.xml',
  'org.x.pix.contact-sheet.gschema.xml',
  'org.x.pix.convert-format.gschema.xml',
  'org.x.pix.crop.gschema.xml',
  'org.x.pix.facebook.gschema.xml',
  'org.x.pix.file-manager.gschema.xml',
  'org.x.pix.flickr.gschema.xml',
  'org.x.pix.gstreamer-tools.gschema.xml',
  'org.x.pix.image-print.gschema.xml',
  'org.x.pix.image-viewer.gschema.xml',
  'org.x.pix.importer.gschema.xml',
  'org.x.pix.photo-importer.gschema.xml',
  'org.x.pix.picasaweb.gschema.xml',
  'org.x.pix.pixbuf-savers.gschema.xml',
  'org.x.pix.rename-series.gschema.xml',
  'org.x.pix.resize.gschema.xml',
  'org.x.pix.resize-images.gschema.xml',
  'org.x.pix.rotate.gschema.xml',
  'org.x.pix.slideshow.gschema.xml',
  'org.x.pix.terminal.gschema.xml',
  'org.x.pix.webalbums.gschema.xml'
)

enums_schema = custom_target('enums_schema',
  output : 'org.x.pix.enums.xml',
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
