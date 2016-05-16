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
 
#ifndef GTH_EDIT_COMMENT_DIALOG_H
#define GTH_EDIT_COMMENT_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_EDIT_COMMENT_DIALOG            (gth_edit_comment_dialog_get_type ())
#define GTH_EDIT_COMMENT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EDIT_COMMENT_DIALOG, GthEditCommentDialog))
#define GTH_EDIT_COMMENT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EDIT_COMMENT_DIALOG, GthEditCommentDialogClass))
#define GTH_IS_EDIT_COMMENT_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EDIT_COMMENT_DIALOG))
#define GTH_IS_EDIT_COMMENT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EDIT_COMMENT_DIALOG))
#define GTH_EDIT_COMMENT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EDIT_COMMENT_DIALOG, GthEditCommentDialogClass))

#define GTH_TYPE_EDIT_COMMENT_PAGE               (gth_edit_comment_page_get_type ())
#define GTH_EDIT_COMMENT_PAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EDIT_COMMENT_PAGE, GthEditCommentPage))
#define GTH_IS_EDIT_COMMENT_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EDIT_COMMENT_PAGE))
#define GTH_EDIT_COMMENT_PAGE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_EDIT_COMMENT_PAGE, GthEditCommentPageInterface))

typedef struct _GthEditCommentDialog GthEditCommentDialog;
typedef struct _GthEditCommentDialogClass GthEditCommentDialogClass;
typedef struct _GthEditCommentDialogPrivate GthEditCommentDialogPrivate;

struct _GthEditCommentDialog {
	GtkDialog parent_instance;
	GthEditCommentDialogPrivate *priv;
};

struct _GthEditCommentDialogClass {
	GtkDialogClass parent_class;
};

typedef struct _GthEditCommentPage GthEditCommentPage;
typedef struct _GthEditCommentPageInterface GthEditCommentPageInterface;

struct _GthEditCommentPageInterface {
	GTypeInterface parent_iface;
	void         (*set_file_list) (GthEditCommentPage *self,
				       GList               *file_list /* GthFileData list */);
	void         (*update_info)   (GthEditCommentPage *self,
				       GFileInfo           *info,
				       gboolean             only_modified_fields);
	const char * (*get_name)      (GthEditCommentPage *self);
};

/* GthEditCommentDialog */

GType          gth_edit_comment_dialog_get_type       (void);

/* GthEditCommentPage */

GType          gth_edit_comment_page_get_type         (void);
void           gth_edit_comment_page_set_file_list    (GthEditCommentPage   *self,
						       GList                *file_list /* GthFileData list */);
void           gth_edit_comment_page_update_info      (GthEditCommentPage   *self,
						       GFileInfo            *info,
						       gboolean              only_modified_fields);
const char *   gth_edit_comment_page_get_name         (GthEditCommentPage   *self);

G_END_DECLS

#endif /* GTH_EDIT_COMMENT_DIALOG_H */
