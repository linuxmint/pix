/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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

#ifndef GTH_SCRIPT_EDITOR_DIALOG_H
#define GTH_SCRIPT_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include "gth-script.h"

G_BEGIN_DECLS

#define GTH_TYPE_SCRIPT_EDITOR_DIALOG            (gth_script_editor_dialog_get_type ())
#define GTH_SCRIPT_EDITOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SCRIPT_EDITOR_DIALOG, GthScriptEditorDialog))
#define GTH_SCRIPT_EDITOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SCRIPT_EDITOR_DIALOG, GthScriptEditorDialogClass))
#define GTH_IS_SCRIPT_EDITOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SCRIPT_EDITOR_DIALOG))
#define GTH_IS_SCRIPT_EDITOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SCRIPT_EDITOR_DIALOG))
#define GTH_SCRIPT_EDITOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SCRIPT_EDITOR_DIALOG, GthScriptEditorDialogClass))

typedef struct _GthScriptEditorDialog        GthScriptEditorDialog;
typedef struct _GthScriptEditorDialogClass   GthScriptEditorDialogClass;
typedef struct _GthScriptEditorDialogPrivate GthScriptEditorDialogPrivate;

struct _GthScriptEditorDialog {
	GtkDialog parent_instance;
	GthScriptEditorDialogPrivate *priv;
};

struct _GthScriptEditorDialogClass {
	GtkDialogClass parent_class;
};

GType       gth_script_editor_dialog_get_type   (void);
GtkWidget * gth_script_editor_dialog_new        (const char             *title,
						 GthWindow              *shortcut_window,
						 GtkWindow              *parent);
void        gth_script_editor_dialog_set_script (GthScriptEditorDialog  *self,
						 GthScript              *script);
GthScript * gth_script_editor_dialog_get_script (GthScriptEditorDialog  *self,
						 GError                **error);

G_END_DECLS

#endif /* GTH_SCRIPT_EDITOR_DIALOG_H */
