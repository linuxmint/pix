/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
 
#ifndef GTH_IMPORT_DESTINATION_BUTTON_H
#define GTH_IMPORT_DESTINATION_BUTTON_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-import-preferences-dialog.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMPORT_DESTINATION_BUTTON            (gth_import_destination_button_get_type ())
#define GTH_IMPORT_DESTINATION_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMPORT_DESTINATION_BUTTON, GthImportDestinationButton))
#define GTH_IMPORT_DESTINATION_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMPORT_DESTINATION_BUTTON, GthImportDestinationButtonClass))
#define GTH_IS_IMPORT_DESTINATION_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMPORT_DESTINATION_BUTTON))
#define GTH_IS_IMPORT_DESTINATION_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMPORT_DESTINATION_BUTTON))
#define GTH_IMPORT_DESTINATION_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMPORT_DESTINATION_BUTTON, GthImportDestinationButtonClass))

typedef struct _GthImportDestinationButton GthImportDestinationButton;
typedef struct _GthImportDestinationButtonClass GthImportDestinationButtonClass;
typedef struct _GthImportDestinationButtonPrivate GthImportDestinationButtonPrivate;

struct _GthImportDestinationButton {
	GtkButton parent_instance;
	GthImportDestinationButtonPrivate *priv;
};

struct _GthImportDestinationButtonClass {
	GtkButtonClass parent_class;
};

GType        gth_import_destination_button_get_type  (void);
GtkWidget *  gth_import_destination_button_new       (GthImportPreferencesDialog *dialog);

G_END_DECLS

#endif /* GTH_IMPORT_DESTINATION_BUTTON_H */
