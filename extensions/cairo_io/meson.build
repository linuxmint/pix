source_files = files(
  'cairo-image-surface-jpeg.c',
  'cairo-image-surface-png.c',
  'cairo-image-surface-xcf.c',
  'gth-image-saver-jpeg.c',
  'gth-image-saver-png.c',
  'gth-image-saver-tga.c',
  'gth-image-saver-tiff.c',
  'preferences.c',
  'main.c'
)

if use_librsvg
  source_files += files('cairo-image-surface-svg.c')
endif

if use_libwebp
  source_files += files('cairo-image-surface-webp.c', 'gth-image-saver-webp.c')
endif

if use_libtiff
  source_files += files('cairo-image-surface-tiff.c')
endif

if use_libjxl
  source_files += files('cairo-image-surface-jxl.c')
endif

if use_libheif
  source_files += files('cairo-image-surface-avif.c', 'gth-image-saver-avif.c')
endif

enum_files = gnome.mkenums_simple('cairo-io-enum-types', sources: [ 'preferences.h' ])

shared_module('cairo_io',
  sources : [ source_files, enum_files ],
  dependencies : [
    extension_common_deps,
    jpeg_deps,
    use_libtiff ? tiff_deps : [],
    use_librsvg ? librsvg_dep : [],
    use_libwebp ? libwebp_dep : [],
    use_libjxl ? libjxl_deps : [],
    use_libheif ? libheif_dep : []
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : jpeg_utils_mod,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'cairo_io.extension.desktop.in.in',
  output : 'cairo_io.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'cairo_io.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
