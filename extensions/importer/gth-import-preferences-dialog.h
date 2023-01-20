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
 
#ifndef GTH_IMPORT_PREFERENCES_DIALOG_H
#define GTH_IMPORT_PREFERENCES_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-import-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMPORT_PREFERENCES_DIALOG            (gth_import_preferences_dialog_get_type ())
#define GTH_IMPORT_PREFERENCES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMPORT_PREFERENCES_DIALOG, GthImportPreferencesDialog))
#define GTH_IMPORT_PREFERENCES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMPORT_PREFERENCES_DIALOG, GthImportPreferencesDialogClass))
#define GTH_IS_IMPORT_PREFERENCES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMPORT_PREFERENCES_DIALOG))
#define GTH_IS_IMPORT_PREFERENCES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMPORT_PREFERENCES_DIALOG))
#define GTH_IMPORT_PREFERENCES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMPORT_PREFERENCES_DIALOG, GthImportPreferencesDialogClass))

typedef struct _GthImportPreferencesDialog GthImportPreferencesDialog;
typedef struct _GthImportPreferencesDialogClass GthImportPreferencesDialogClass;
typedef struct _GthImportPreferencesDialogPrivate GthImportPreferencesDialogPrivate;

struct _GthImportPreferencesDialog {
	GtkDialog parent_instance;
	GthImportPreferencesDialogPrivate *priv;
};

struct _GthImportPreferencesDialogClass {
	GtkDialogClass parent_class;

	/*< signals >*/

	void (*destination_changed) (GthImportPreferencesDialog *self);
};

GType        gth_import_preferences_dialog_get_type  (void);
GtkWidget *  gth_import_preferences_dialog_new       (void);
void         gth_import_preferences_dialog_set_event (GthImportPreferencesDialog *self,
						      const char                 *event);
GFile *      gth_import_preferences_dialog_get_destination
						     (GthImportPreferencesDialog *self);
GFile *      gth_import_preferences_dialog_get_subfolder_example
						     (GthImportPreferencesDialog *self);

G_END_DECLS

#endif /* GTH_IMPORT_PREFERENCES_DIALOG_H */
