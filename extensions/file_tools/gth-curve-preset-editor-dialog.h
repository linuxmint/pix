/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTH_CURVE_PRESET_EDITOR_DIALOG_H
#define GTH_CURVE_PRESET_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include "gth-curve-preset.h"

G_BEGIN_DECLS

#define GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG            (gth_curve_preset_editor_dialog_get_type ())
#define GTH_CURVE_PRESET_EDITOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG, GthCurvePresetEditorDialog))
#define GTH_CURVE_PRESET_EDITOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG, GthCurvePresetEditorDialogClass))
#define GTH_IS_CURVE_PRESET_EDITOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG))
#define GTH_IS_CURVE_PRESET_EDITOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG))
#define GTH_CURVE_PRESET_EDITOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG, GthCurvePresetEditorDialogClass))

typedef struct _GthCurvePresetEditorDialog        GthCurvePresetEditorDialog;
typedef struct _GthCurvePresetEditorDialogClass   GthCurvePresetEditorDialogClass;
typedef struct _GthCurvePresetEditorDialogPrivate GthCurvePresetEditorDialogPrivate;

struct _GthCurvePresetEditorDialog {
	GtkDialog parent_instance;
	GthCurvePresetEditorDialogPrivate *priv;
};

struct _GthCurvePresetEditorDialogClass {
	GtkDialogClass parent_class;
};

GType       gth_curve_preset_editor_dialog_get_type   (void);
GtkWidget * gth_curve_preset_editor_dialog_new        (GtkWindow	*parent,
						       GthCurvePreset	*preset);

G_END_DECLS

#endif /* GTH_CURVE_PRESET_EDITOR_DIALOG_H */
