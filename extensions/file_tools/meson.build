
header_files = [
  'cairo-blur.h',
  'cairo-effects.h',
  'cairo-rotate.h',
  'gth-curve.h',
  'gth-curve-editor.h',
  'gth-curve-preset.h',
  'gth-curve-preset-editor-dialog.h',
  'gth-file-tool-adjust-colors.h',
  'gth-file-tool-adjust-contrast.h',
  'gth-file-tool-color-picker.h',
  'gth-file-tool-crop.h',
  'gth-file-tool-curves.h',
  'gth-file-tool-effects.h',
  'gth-file-tool-flip.h',
  'gth-file-tool-grayscale.h',
  'gth-file-tool-lomo.h',
  'gth-file-tool-mirror.h',
  'gth-file-tool-negative.h',
  'gth-file-tool-redo.h',
  'gth-file-tool-resize.h',
  'gth-file-tool-rotate.h',
  'gth-file-tool-rotate-left.h',
  'gth-file-tool-rotate-right.h',
  'gth-file-tool-rotate.h',
  'gth-file-tool-save.h',
  'gth-file-tool-save-as.h',
  'gth-file-tool-sharpen.h',
  'gth-file-tool-undo.h',
  'gth-image-line-tool.h',
  'gth-image-rotator.h',
  'gth-points.h',
  'gth-preview-tool.h',
  'preferences.h'
]

enum_files = gnome.mkenums_simple('file-tools-enum-types', sources: header_files)

source_files = files(
  'actions.c',
  'cairo-blur.c',
  'cairo-effects.c',
  'cairo-rotate.c',
  'callbacks.c',
  'callbacks.h',
  'gth-curve.c',
  'gth-curve-editor.c',
  'gth-curve-preset.c',
  'gth-curve-preset-editor-dialog.c',
  'gth-file-tool-adjust-colors.c',
  'gth-file-tool-adjust-contrast.c',
  'gth-file-tool-color-picker.c',
  'gth-file-tool-crop.c',
  'gth-file-tool-curves.c',
  'gth-file-tool-effects.c',
  'gth-file-tool-flip.c',
  'gth-file-tool-grayscale.c',
  'gth-file-tool-lomo.c',
  'gth-file-tool-mirror.c',
  'gth-file-tool-negative.c',
  'gth-file-tool-redo.c',
  'gth-file-tool-resize.c',
  'gth-file-tool-rotate.c',
  'gth-file-tool-rotate-left.c',
  'gth-file-tool-rotate-right.c',
  'gth-file-tool-save.c',
  'gth-file-tool-save-as.c',
  'gth-file-tool-sharpen.c',
  'gth-file-tool-undo.c',
  'gth-image-line-tool.c',
  'gth-image-rotator.c',
  'gth-points.c',
  'gth-preview-tool.c',
  'main.c'
)

file_tools_mod = shared_module('file_tools',
  sources : [ source_files, enum_files ],
  dependencies : extension_common_deps,
  include_directories : [
    config_inc,
    gthumb_inc
  ],
  c_args : c_args,
  link_with : image_viewer_mod,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

file_tools_inc = include_directories('.')
file_tools_dep = declare_dependency(sources : enum_files)

# .extension file

extension_in_file = configure_file(
  input : 'file_tools.extension.desktop.in.in',
  output : 'file_tools.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'file_tools.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
