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

#ifndef GTH_SEARCH_EDITOR_DIALOG_H
#define GTH_SEARCH_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include "gth-search.h"

#define GTH_TYPE_SEARCH_EDITOR_DIALOG            (gth_search_editor_dialog_get_type ())
#define GTH_SEARCH_EDITOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SEARCH_EDITOR_DIALOG, GthSearchEditorDialog))
#define GTH_SEARCH_EDITOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SEARCH_EDITOR_DIALOG, GthSearchEditorDialogClass))
#define GTH_IS_SEARCH_EDITOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SEARCH_EDITOR_DIALOG))
#define GTH_IS_SEARCH_EDITOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SEARCH_EDITOR_DIALOG))
#define GTH_SEARCH_EDITOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SEARCH_EDITOR_DIALOG, GthSearchEditorDialogClass))

typedef struct _GthSearchEditorDialog        GthSearchEditorDialog;
typedef struct _GthSearchEditorDialogClass   GthSearchEditorDialogClass;
typedef struct _GthSearchEditorDialogPrivate GthSearchEditorDialogPrivate;

struct _GthSearchEditorDialog {
	GtkDialog parent_instance;
	GthSearchEditorDialogPrivate *priv;
};

struct _GthSearchEditorDialogClass {
	GtkDialogClass parent_class;
};

GType       gth_search_editor_dialog_get_type   (void);
GtkWidget * gth_search_editor_dialog_new        (const char             *title,
						 GthSearch              *search,
						 GtkWindow              *parent);
void        gth_search_editor_dialog_set_search (GthSearchEditorDialog  *self,
						 GthSearch              *search);
GthSearch * gth_search_editor_dialog_get_search (GthSearchEditorDialog  *self,
						 GError                **error);
void        gth_search_editor_dialog_focus_first_rule
						(GthSearchEditorDialog	*self);

#endif /* GTH_SEARCH_EDITOR_DIALOG_H */
