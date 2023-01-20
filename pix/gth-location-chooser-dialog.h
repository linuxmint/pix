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

#ifndef GTH_LOCATION_CHOOSER_DIALOG_H
#define GTH_LOCATION_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_LOCATION_CHOOSER_DIALOG         (gth_location_chooser_dialog_get_type ())
#define GTH_LOCATION_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_LOCATION_CHOOSER_DIALOG, GthLocationChooserDialog))
#define GTH_LOCATION_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_LOCATION_CHOOSER_DIALOG, GthLocationChooserDialogClass))
#define GTH_IS_LOCATION_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_LOCATION_CHOOSER_DIALOG))
#define GTH_IS_LOCATION_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_LOCATION_CHOOSER_DIALOG))
#define GTH_LOCATION_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_LOCATION_CHOOSER_DIALOG, GthLocationChooserDialogClass))

typedef struct _GthLocationChooserDialog         GthLocationChooserDialog;
typedef struct _GthLocationChooserDialogPrivate  GthLocationChooserDialogPrivate;
typedef struct _GthLocationChooserDialogClass    GthLocationChooserDialogClass;

struct _GthLocationChooserDialog {
	GtkDialog __parent;
	GthLocationChooserDialogPrivate *priv;
};

struct _GthLocationChooserDialogClass {
	GtkDialogClass __parent_class;
};

GType		gth_location_chooser_dialog_get_type	(void) G_GNUC_CONST;
GtkWidget *	gth_location_chooser_dialog_new		(const char			 *title,
							 GtkWindow			 *parent);
void		gth_location_chooser_dialog_set_folder	(GthLocationChooserDialog	 *self,
							 GFile				 *folder);
GFile *		gth_location_chooser_dialog_get_folder	(GthLocationChooserDialog	 *self);

G_END_DECLS

#endif /* GTH_LOCATION_CHOOSER_DIALOG_H */
