/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2020 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_SELECTION_INFO_H
#define GTH_FILE_SELECTION_INFO_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_SELECTION_INFO            (gth_file_selection_info_get_type ())
#define GTH_FILE_SELECTION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_SELECTION_INFO, GthFileSelectionInfo))
#define GTH_FILE_SELECTION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_SELECTION_INFO, GthFileSelectionInfoClass))
#define GTH_IS_FILE_SELECTION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_SELECTION_INFO))
#define GTH_IS_FILE_SELECTION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_SELECTION_INFO))
#define GTH_FILE_SELECTION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILE_SELECTION_INFO, GthFileSelectionInfoClass))

typedef struct _GthFileSelectionInfo GthFileSelectionInfo;
typedef struct _GthFileSelectionInfoClass GthFileSelectionInfoClass;
typedef struct _GthFileSelectionInfoPrivate GthFileSelectionInfoPrivate;

struct _GthFileSelectionInfo {
	GtkBox parent_instance;
	GthFileSelectionInfoPrivate *priv;
};

struct _GthFileSelectionInfoClass {
	GtkBoxClass parent_class;
};

GType		gth_file_selection_info_get_type	(void);
GtkWidget *	gth_file_selection_info_new		(void);
void		gth_file_selection_info_set_file_list	(GthFileSelectionInfo	*self,
							 GList			*file_list);
void		gth_file_selection_info_set_visible	(GthFileSelectionInfo	*self,
							 gboolean		 visible);

G_END_DECLS

#endif /* GTH_FILE_SELECTION_INFO_H */
