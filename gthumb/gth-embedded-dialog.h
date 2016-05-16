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

#ifndef GTH_EMBEDDED_DIALOG_H
#define GTH_EMBEDDED_DIALOG_H

#include <gtk/gtk.h>
#include "gedit-message-area.h"

G_BEGIN_DECLS

#define GTH_TYPE_EMBEDDED_DIALOG         (gth_embedded_dialog_get_type ())
#define GTH_EMBEDDED_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_EMBEDDED_DIALOG, GthEmbeddedDialog))
#define GTH_EMBEDDED_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_EMBEDDED_DIALOG, GthEmbeddedDialogClass))
#define GTH_IS_EMBEDDED_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_EMBEDDED_DIALOG))
#define GTH_IS_EMBEDDED_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_EMBEDDED_DIALOG))
#define GTH_EMBEDDED_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_EMBEDDED_DIALOG, GthEmbeddedDialogClass))

typedef struct _GthEmbeddedDialog         GthEmbeddedDialog;
typedef struct _GthEmbeddedDialogPrivate  GthEmbeddedDialogPrivate;
typedef struct _GthEmbeddedDialogClass    GthEmbeddedDialogClass;

struct _GthEmbeddedDialog
{
	GeditMessageArea __parent;
	GthEmbeddedDialogPrivate *priv;
};

struct _GthEmbeddedDialogClass
{
	GeditMessageAreaClass __parent_class;
};

GType         gth_embedded_dialog_get_type           (void) G_GNUC_CONST;
GtkWidget *   gth_embedded_dialog_new                (void);
void          gth_embedded_dialog_set_icon           (GthEmbeddedDialog *dialog,
						      const char        *icon_stock_id,
						      GtkIconSize        size);
void          gth_embedded_dialog_set_gicon          (GthEmbeddedDialog *dialog,
						      GIcon             *icon,
						      GtkIconSize        size);
void          gth_embedded_dialog_set_primary_text   (GthEmbeddedDialog *dialog,
						      const char        *primary_text);
void          gth_embedded_dialog_set_secondary_text (GthEmbeddedDialog *dialog,
						      const char        *secondary_text);
void          gth_embedded_dialog_set_from_file      (GthEmbeddedDialog *dialog,
						      GFile             *file);
GtkWidget *   gth_embedded_dialog_get_chooser        (GthEmbeddedDialog *dialog);

G_END_DECLS

#endif /* GTH_EMBEDDED_DIALOG_H */
