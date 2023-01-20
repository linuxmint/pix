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

#ifndef GTH_CONTACT_SHEET_CREATOR_H
#define GTH_CONTACT_SHEET_CREATOR_H

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-contact-sheet-theme.h"

#define GTH_TYPE_CONTACT_SHEET_CREATOR            (gth_contact_sheet_creator_get_type ())
#define GTH_CONTACT_SHEET_CREATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CONTACT_SHEET_CREATOR, GthContactSheetCreator))
#define GTH_CONTACT_SHEET_CREATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CONTACT_SHEET_CREATOR, GthContactSheetCreatorClass))
#define GTH_IS_CONTACT_SHEET_CREATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CONTACT_SHEET_CREATOR))
#define GTH_IS_CONTACT_SHEET_CREATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CONTACT_SHEET_CREATOR))
#define GTH_CONTACT_SHEET_CREATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_CONTACT_SHEET_CREATOR, GthContactSheetCreatorClass))

typedef struct _GthContactSheetCreator        GthContactSheetCreator;
typedef struct _GthContactSheetCreatorClass   GthContactSheetCreatorClass;
typedef struct _GthContactSheetCreatorPrivate GthContactSheetCreatorPrivate;

struct _GthContactSheetCreator {
	GthTask __parent;
	GthContactSheetCreatorPrivate *priv;

};

struct _GthContactSheetCreatorClass {
	GthTaskClass __parent;
};

GType      gth_contact_sheet_creator_get_type              (void);
GthTask *  gth_contact_sheet_creator_new                   (GthBrowser               *browser,
							    GList                    *file_list); /* GFile list */
void       gth_contact_sheet_creator_set_header            (GthContactSheetCreator   *self,
							    const char               *value);
void       gth_contact_sheet_creator_set_footer            (GthContactSheetCreator   *self,
						   	    const char               *value);
void       gth_contact_sheet_creator_set_destination       (GthContactSheetCreator   *self,
						   	    GFile                    *destination);
void       gth_contact_sheet_creator_set_filename_template (GthContactSheetCreator   *self,
						   	    const char               *filename_template);
void       gth_contact_sheet_creator_set_mime_type         (GthContactSheetCreator   *self,
							    const char               *mime_type,
							    const char               *file_extension);
void       gth_contact_sheet_creator_set_write_image_map   (GthContactSheetCreator   *self,
							    gboolean                  value);
void       gth_contact_sheet_creator_set_theme             (GthContactSheetCreator   *self,
							    GthContactSheetTheme     *theme);
void       gth_contact_sheet_creator_set_images_per_index  (GthContactSheetCreator   *self,
							    int                       value);
void       gth_contact_sheet_creator_set_single_index      (GthContactSheetCreator   *self,
						   	    gboolean                  value);
void       gth_contact_sheet_creator_set_columns           (GthContactSheetCreator   *self,
						   	    int                       columns);
void       gth_contact_sheet_creator_set_sort_order        (GthContactSheetCreator   *self,
							    GthFileDataSort          *sort_type,
							    gboolean                  sort_inverse);
void       gth_contact_sheet_creator_set_same_size         (GthContactSheetCreator   *self,
						   	    gboolean                  value);
void       gth_contact_sheet_creator_set_thumb_size        (GthContactSheetCreator   *self,
						            gboolean                  squared,
						            int                       width,
						            int                       height);
void       gth_contact_sheet_creator_set_thumbnail_caption (GthContactSheetCreator   *self,
						            const char               *caption);
void       gth_contact_sheet_creator_set_location_name     (GthContactSheetCreator   *self,
							    const char               *name);

#endif /* GTH_CONTACT_SHEET_CREATOR_H */
