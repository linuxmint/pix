/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#ifndef GTH_TEMPLATE_EDITOR_DIALOG_H
#define GTH_TEMPLATE_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include "gth-template-selector.h"
#include "str-utils.h"

G_BEGIN_DECLS

#define GTH_TYPE_TEMPLATE_EDITOR_DIALOG            (gth_template_editor_dialog_get_type ())
#define GTH_TEMPLATE_EDITOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TEMPLATE_EDITOR_DIALOG, GthTemplateEditorDialog))
#define GTH_TEMPLATE_EDITOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TEMPLATE_EDITOR_DIALOG, GthTemplateEditorDialogClass))
#define GTH_IS_TEMPLATE_EDITOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TEMPLATE_EDITOR_DIALOG))
#define GTH_IS_TEMPLATE_EDITOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TEMPLATE_EDITOR_DIALOG))
#define GTH_TEMPLATE_EDITOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TEMPLATE_EDITOR_DIALOG, GthTemplateEditorDialogClass))

typedef struct _GthTemplateEditorDialog        GthTemplateEditorDialog;
typedef struct _GthTemplateEditorDialogClass   GthTemplateEditorDialogClass;
typedef struct _GthTemplateEditorDialogPrivate GthTemplateEditorDialogPrivate;

struct _GthTemplateEditorDialog {
	GtkDialog parent_instance;
	GthTemplateEditorDialogPrivate *priv;
};

struct _GthTemplateEditorDialogClass {
	GtkDialogClass parent_class;
};

GType       gth_template_editor_dialog_get_type	(void);
GtkWidget * gth_template_editor_dialog_new		(GthTemplateCode          *allowed_codes,
							 int                       n_codes,
							 TemplateFlags             template_flags,
							 const char               *title,
							 GtkWindow                *parent);
void        gth_template_editor_dialog_set_template	(GthTemplateEditorDialog  *self,
							 const char               *value);
char *      gth_template_editor_dialog_get_template	(GthTemplateEditorDialog  *self);
void        gth_template_editor_dialog_set_preview_func (GthTemplateEditorDialog  *self,
							 TemplatePreviewFunc       func,
							 gpointer                  user_data);
void        gth_template_editor_dialog_set_preview_cb   (GthTemplateEditorDialog  *self,
							 TemplateEvalFunc          func,
							 gpointer                  user_data);
void        gth_template_editor_dialog_set_date_formats (GthTemplateEditorDialog  *self,
							 char                    **formats); /* NULL terminated. */
void        gth_template_editor_dialog_default_response (GtkDialog                *dialog,
							 int                       response_id,
							 gpointer                  user_data);

G_END_DECLS

#endif /* GTH_TEMPLATE_EDITOR_DIALOG_H */
