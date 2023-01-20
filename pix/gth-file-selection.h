/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_SELECTION_H
#define GTH_FILE_SELECTION_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_SELECTION               (gth_file_selection_get_type ())
#define GTH_FILE_SELECTION(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_SELECTION, GthFileSelection))
#define GTH_IS_FILE_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_SELECTION))
#define GTH_FILE_SELECTION_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_FILE_SELECTION, GthFileSelectionInterface))

typedef struct _GthFileSelection GthFileSelection;
typedef struct _GthFileSelectionInterface GthFileSelectionInterface;

struct _GthFileSelectionInterface {
	GTypeInterface parent_iface;

	/*< signals >*/

	void          (*file_selection_changed)  (GthFileSelection *self);

	/*< virtual functions >*/

	void          (*set_selection_mode)      (GthFileSelection *self,
					          GtkSelectionMode  mode);
	GList *       (*get_selected)            (GthFileSelection *self);
	void          (*select)                  (GthFileSelection *self,
					          int               pos);
	void          (*unselect)                (GthFileSelection *self,
					          int               pos);
	void          (*select_all)              (GthFileSelection *self);
	void          (*unselect_all)            (GthFileSelection *self);
	gboolean      (*is_selected)             (GthFileSelection *self,
					          int               pos);
	GtkTreePath * (*get_first_selected)      (GthFileSelection *self);
	GtkTreePath * (*get_last_selected)       (GthFileSelection *self);
	guint         (*get_n_selected)          (GthFileSelection *self);
};

GType         gth_file_selection_get_type           (void);
void          gth_file_selection_set_selection_mode (GthFileSelection *self,
						     GtkSelectionMode  mode);
GList *       gth_file_selection_get_selected       (GthFileSelection *self);
void          gth_file_selection_select             (GthFileSelection *self,
						     int               pos);
void          gth_file_selection_unselect           (GthFileSelection *self,
						     int               pos);
void          gth_file_selection_select_all         (GthFileSelection *self);
void          gth_file_selection_unselect_all       (GthFileSelection *self);
gboolean      gth_file_selection_is_selected        (GthFileSelection *self,
						     int               pos);
GtkTreePath * gth_file_selection_get_first_selected (GthFileSelection *self);
GtkTreePath * gth_file_selection_get_last_selected  (GthFileSelection *self);
guint         gth_file_selection_get_n_selected     (GthFileSelection *self);
void          gth_file_selection_changed            (GthFileSelection *self);

G_END_DECLS

#endif /* GTH_FILE_SELECTION_H */
