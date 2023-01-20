/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_FILTER_EDITOR_DIALOG_H
#define GTH_FILTER_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include "gth-filter.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILTER_EDITOR_DIALOG            (gth_filter_editor_dialog_get_type ())
#define GTH_FILTER_EDITOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILTER_EDITOR_DIALOG, GthFilterEditorDialog))
#define GTH_FILTER_EDITOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILTER_EDITOR_DIALOG, GthFilterEditorDialogClass))
#define GTH_IS_FILTER_EDITOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILTER_EDITOR_DIALOG))
#define GTH_IS_FILTER_EDITOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILTER_EDITOR_DIALOG))
#define GTH_FILTER_EDITOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILTER_EDITOR_DIALOG, GthFilterEditorDialogClass))

typedef struct _GthFilterEditorDialog        GthFilterEditorDialog;
typedef struct _GthFilterEditorDialogClass   GthFilterEditorDialogClass;
typedef struct _GthFilterEditorDialogPrivate GthFilterEditorDialogPrivate;

struct _GthFilterEditorDialog {
	GtkDialog parent_instance;
	GthFilterEditorDialogPrivate *priv;
};

struct _GthFilterEditorDialogClass {
	GtkDialogClass parent_class;
};

GType       gth_filter_editor_dialog_get_type   (void);
GtkWidget * gth_filter_editor_dialog_new        (const char             *title, 
						 GtkWindow              *parent);
void        gth_filter_editor_dialog_set_filter (GthFilterEditorDialog  *self, 
						 GthFilter              *filter);
GthFilter * gth_filter_editor_dialog_get_filter (GthFilterEditorDialog  *self,
						 GError                **error);

G_END_DECLS

#endif /* GTH_FILTER_EDITOR_DIALOG_H */
