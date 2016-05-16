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
 
#ifndef GTH_EDIT_TAGS_DIALOG_H
#define GTH_EDIT_TAGS_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_EDIT_TAGS_DIALOG            (gth_edit_tags_dialog_get_type ())
#define GTH_EDIT_TAGS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EDIT_TAGS_DIALOG, GthEditTagsDialog))
#define GTH_EDIT_TAGS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EDIT_TAGS_DIALOG, GthEditTagsDialogClass))
#define GTH_IS_EDIT_TAGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EDIT_TAGS_DIALOG))
#define GTH_IS_EDIT_TAGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EDIT_TAGS_DIALOG))
#define GTH_EDIT_TAGS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EDIT_TAGS_DIALOG, GthEditTagsDialogClass))

typedef struct _GthEditTagsDialog GthEditTagsDialog;
typedef struct _GthEditTagsDialogClass GthEditTagsDialogClass;
typedef struct _GthEditTagsDialogPrivate GthEditTagsDialogPrivate;

struct _GthEditTagsDialog {
	GtkDialog parent_instance;
	GthEditTagsDialogPrivate *priv;
};

struct _GthEditTagsDialogClass {
	GtkDialogClass parent_class;
};

GType          gth_edit_tags_dialog_get_type  (void);
GtkWidget *    gth_edit_tags_dialog_new       (void);


G_END_DECLS

#endif /* GTH_EDIT_TAGS_DIALOG_H */
