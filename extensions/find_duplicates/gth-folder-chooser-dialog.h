/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_FOLDER_CHOOSER_DIALOG_H
#define GTH_FOLDER_CHOOSER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_FOLDER_CHOOSER_DIALOG            (gth_folder_chooser_dialog_get_type ())
#define GTH_FOLDER_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FOLDER_CHOOSER_DIALOG, GthFolderChooserDialog))
#define GTH_FOLDER_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FOLDER_CHOOSER_DIALOG, GthFolderChooserDialogClass))
#define GTH_IS_FOLDER_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FOLDER_CHOOSER_DIALOG))
#define GTH_IS_FOLDER_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FOLDER_CHOOSER_DIALOG))
#define GTH_FOLDER_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FOLDER_CHOOSER_DIALOG, GthFolderChooserDialogClass))

typedef struct _GthFolderChooserDialog GthFolderChooserDialog;
typedef struct _GthFolderChooserDialogClass GthFolderChooserDialogClass;
typedef struct _GthFolderChooserDialogPrivate GthFolderChooserDialogPrivate;

struct _GthFolderChooserDialog {
	GtkDialog parent_instance;
	GthFolderChooserDialogPrivate *priv;
};

struct _GthFolderChooserDialogClass {
	GtkDialogClass parent_class;
};

GType        gth_folder_chooser_dialog_get_type     (void);
GtkWidget *  gth_folder_chooser_dialog_new          (GList *folders);
GHashTable * gth_folder_chooser_dialog_get_selected (GthFolderChooserDialog *self);

G_END_DECLS

#endif /* GTH_FOLDER_CHOOSER_DIALOG_H */
