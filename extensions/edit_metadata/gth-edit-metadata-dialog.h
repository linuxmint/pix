/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_EDIT_METADATA_DIALOG_H
#define GTH_EDIT_METADATA_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_EDIT_METADATA_DIALOG               (gth_edit_metadata_dialog_get_type ())
#define GTH_EDIT_METADATA_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialog))
#define GTH_IS_EDIT_METADATA_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EDIT_METADATA_DIALOG))
#define GTH_EDIT_METADATA_DIALOG_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialogInterface))

typedef struct _GthEditMetadataDialog GthEditMetadataDialog;
typedef struct _GthEditMetadataDialogInterface GthEditMetadataDialogInterface;

struct _GthEditMetadataDialogInterface {
	GTypeInterface parent_iface;

	void		(*set_file_list)	(GthEditMetadataDialog *dialog,
						 GList                 *file_list /* GthFileData list */);
	void		(*update_info)		(GthEditMetadataDialog *dialog,
						 GList                 *file_list /* GthFileData list */);
};

/* GthEditMetadataDialog */

GType          gth_edit_metadata_dialog_get_type       (void);
void           gth_edit_metadata_dialog_set_file_list  (GthEditMetadataDialog *dialog,
						        GList                 *file_list /* GthFileData list */);
void           gth_edit_metadata_dialog_update_info    (GthEditMetadataDialog *dialog,
						        GList                 *file_list /* GthFileData list */);

G_END_DECLS


#endif /* GTH_EDIT_METADATA_DIALOG_H */
