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

#ifndef GTH_CONTACT_SHEET_THEME_DIALOG_H
#define GTH_CONTACT_SHEET_THEME_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-contact-sheet-theme.h"

G_BEGIN_DECLS

#define GTH_TYPE_CONTACT_SHEET_THEME_DIALOG            (gth_contact_sheet_theme_dialog_get_type ())
#define GTH_CONTACT_SHEET_THEME_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CONTACT_SHEET_THEME_DIALOG, GthContactSheetThemeDialog))
#define GTH_CONTACT_SHEET_THEME_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CONTACT_SHEET_THEME_DIALOG, GthContactSheetThemeDialogClass))
#define GTH_IS_CONTACT_SHEET_THEME_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CONTACT_SHEET_THEME_DIALOG))
#define GTH_IS_CONTACT_SHEET_THEME_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CONTACT_SHEET_THEME_DIALOG))
#define GTH_CONTACT_SHEET_THEME_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CONTACT_SHEET_THEME_DIALOG, GthContactSheetThemeDialogClass))

typedef struct _GthContactSheetThemeDialog GthContactSheetThemeDialog;
typedef struct _GthContactSheetThemeDialogClass GthContactSheetThemeDialogClass;
typedef struct _GthContactSheetThemeDialogPrivate GthContactSheetThemeDialogPrivate;

struct _GthContactSheetThemeDialog {
	GtkDialog parent_instance;
	GthContactSheetThemeDialogPrivate *priv;
};

struct _GthContactSheetThemeDialogClass {
	GtkDialogClass parent_class;
};

GType                   gth_contact_sheet_theme_dialog_get_type    (void);
GtkWidget *             gth_contact_sheet_theme_dialog_new         (GthContactSheetTheme       *theme,
								    GList                      *all_themes);
GthContactSheetTheme *  gth_contact_sheet_theme_dialog_get_theme   (GthContactSheetThemeDialog *self);

G_END_DECLS

#endif /* GTH_CONTACT_SHEET_THEME_DIALOG_H */
