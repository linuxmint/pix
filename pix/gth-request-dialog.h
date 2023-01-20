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

#ifndef GTH_REQUEST_DIALOG_H
#define GTH_REQUEST_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_REQUEST_DIALOG            (gth_request_dialog_get_type ())
#define GTH_REQUEST_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_REQUEST_DIALOG, GthRequestDialog))
#define GTH_REQUEST_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_REQUEST_DIALOG, GthRequestDialogClass))
#define GTH_IS_REQUEST_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_REQUEST_DIALOG))
#define GTH_IS_REQUEST_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_REQUEST_DIALOG))
#define GTH_REQUEST_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_REQUEST_DIALOG, GthRequestDialogClass))

typedef struct _GthRequestDialog        GthRequestDialog;
typedef struct _GthRequestDialogClass   GthRequestDialogClass;
typedef struct _GthRequestDialogPrivate GthRequestDialogPrivate;

struct _GthRequestDialog {
	GtkDialog parent_instance;
	GthRequestDialogPrivate *priv;
};

struct _GthRequestDialogClass {
	GtkDialogClass parent_class;
};

GType       gth_request_dialog_get_type              (void);
GtkWidget * gth_request_dialog_new                   (GtkWindow        *parent,
		    	    	    	              GtkDialogFlags    flags,
		    	    	    	              const char       *message,
		    	    	    	              const char       *secondary_message,
		    	    	    	              const char       *cancel_button_text,
		    	    	    	              const char       *ok_button_text);
GtkWidget * gth_request_dialog_get_entry             (GthRequestDialog *dialog);
char *      gth_request_dialog_get_normalized_text   (GthRequestDialog *dialog);
GtkWidget * gth_request_dialog_get_info_bar          (GthRequestDialog *dialog);
GtkWidget * gth_request_dialog_get_info_label        (GthRequestDialog *dialog);
void        gth_request_dialog_set_info_text         (GthRequestDialog *dialog,
					              GtkMessageType    message_type,
					              const char       *text);
void        gth_request_dialog_set_info_markup       (GthRequestDialog *dialog,
					              GtkMessageType    message_type,
					              const char       *markup);

G_END_DECLS

#endif /* GTH_REQUEST_DIALOG_H */

