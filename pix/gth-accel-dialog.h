/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

#ifndef GTH_ACCEL_DIALOG_H
#define GTH_ACCEL_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
	GTH_ACCEL_BUTTON_RESPONSE_DELETE
};

#define GTH_TYPE_ACCEL_DIALOG         (gth_accel_dialog_get_type ())
#define GTH_ACCEL_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_ACCEL_DIALOG, GthAccelDialog))
#define GTH_ACCEL_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_ACCEL_DIALOG, GthAccelDialogClass))
#define GTH_IS_ACCEL_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_ACCEL_DIALOG))
#define GTH_IS_ACCEL_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_ACCEL_DIALOG))
#define GTH_ACCEL_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_ACCEL_DIALOG, GthAccelDialogClass))

typedef struct _GthAccelDialog         GthAccelDialog;
typedef struct _GthAccelDialogPrivate  GthAccelDialogPrivate;
typedef struct _GthAccelDialogClass    GthAccelDialogClass;

struct _GthAccelDialog {
	GtkDialog __parent;
	GthAccelDialogPrivate *priv;
};

struct _GthAccelDialogClass {
	GtkDialogClass __parent_class;
};

GType         gth_accel_dialog_get_type  (void) G_GNUC_CONST;
GtkWidget *   gth_accel_dialog_new       (const char      *title,
					  GtkWindow       *parent,
					  guint            keycode,
					  GdkModifierType  modifiers);
gboolean      gth_accel_dialog_get_accel (GthAccelDialog  *self,
					  guint           *keycode,
					  GdkModifierType *modifiers);

G_END_DECLS

#endif /* GTH_ACCEL_DIALOG_H */
