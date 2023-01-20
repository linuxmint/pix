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

#ifndef GTH_OVERWRITE_DIALOG_H
#define GTH_OVERWRITE_DIALOG_H

#include <gtk/gtk.h>
#include "gth-image.h"

G_BEGIN_DECLS

typedef enum {
	GTH_OVERWRITE_RESPONSE_UNSPECIFIED,
	GTH_OVERWRITE_RESPONSE_YES,
	GTH_OVERWRITE_RESPONSE_NO,
	GTH_OVERWRITE_RESPONSE_ALWAYS_YES,
	GTH_OVERWRITE_RESPONSE_ALWAYS_NO,
	GTH_OVERWRITE_RESPONSE_RENAME,
	GTH_OVERWRITE_RESPONSE_CANCEL
} GthOverwriteResponse;

#define GTH_OVERWRITE_RESPONSE_ASK GTH_OVERWRITE_RESPONSE_UNSPECIFIED

#define GTH_TYPE_OVERWRITE_DIALOG            (gth_overwrite_dialog_get_type ())
#define GTH_OVERWRITE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_OVERWRITE_DIALOG, GthOverwriteDialog))
#define GTH_OVERWRITE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_OVERWRITE_DIALOG, GthOverwriteDialogClass))
#define GTH_IS_OVERWRITE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_OVERWRITE_DIALOG))
#define GTH_IS_OVERWRITE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_OVERWRITE_DIALOG))
#define GTH_OVERWRITE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_OVERWRITE_DIALOG, GthOverwriteDialogClass))

typedef struct _GthOverwriteDialog GthOverwriteDialog;
typedef struct _GthOverwriteDialogClass GthOverwriteDialogClass;
typedef struct _GthOverwriteDialogPrivate GthOverwriteDialogPrivate;

struct _GthOverwriteDialog {
	GtkDialog parent_instance;
	GthOverwriteDialogPrivate *priv;
};

struct _GthOverwriteDialogClass {
	GtkDialogClass parent_class;
};

GType                 gth_overwrite_dialog_get_type      (void);
GtkWidget *           gth_overwrite_dialog_new           (GFile                *source,
							  GthImage             *source_image,
						          GFile                *destination,
						          GthOverwriteResponse  default_respose,
						          gboolean              single_file);
GthOverwriteResponse  gth_overwrite_dialog_get_response  (GthOverwriteDialog   *dialog);
const char *          gth_overwrite_dialog_get_filename  (GthOverwriteDialog   *dialog);

G_END_DECLS

#endif /* GTH_OVERWRITE_DIALOG_H */
