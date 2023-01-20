/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_CHOOSER_DIALOG_H
#define GTH_FILE_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_CHOOSER_DIALOG         (gth_file_chooser_dialog_get_type ())
#define GTH_FILE_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_CHOOSER_DIALOG, GthFileChooserDialog))
#define GTH_FILE_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_CHOOSER_DIALOG, GthFileChooserDialogClass))
#define GTH_IS_FILE_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_CHOOSER_DIALOG))
#define GTH_IS_FILE_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_CHOOSER_DIALOG))
#define GTH_FILE_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_CHOOSER_DIALOG, GthFileChooserDialogClass))

typedef struct _GthFileChooserDialog         GthFileChooserDialog;
typedef struct _GthFileChooserDialogPrivate  GthFileChooserDialogPrivate;
typedef struct _GthFileChooserDialogClass    GthFileChooserDialogClass;

struct _GthFileChooserDialog {
	GtkFileChooserDialog __parent;
	GthFileChooserDialogPrivate *priv;
};

struct _GthFileChooserDialogClass {
	GtkFileChooserDialogClass __parent_class;
};

GType         gth_file_chooser_dialog_get_type  (void) G_GNUC_CONST;
GtkWidget *   gth_file_chooser_dialog_new       (const char            *title,
						 GtkWindow             *parent,
						 const char            *allowed_savers);
gboolean      gth_file_chooser_dialog_get_file  (GthFileChooserDialog  *self,
						 GFile                **file,
						 const char           **mime_type);

G_END_DECLS

#endif /* GTH_FILE_CHOOSER_DIALOG_H */
