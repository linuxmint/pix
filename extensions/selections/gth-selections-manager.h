/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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

#ifndef GTH_SELECTIONS_MANAGER_H
#define GTH_SELECTIONS_MANAGER_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_SELECTIONS_MANAGER_N_SELECTIONS 3

#define GTH_TYPE_SELECTIONS_MANAGER         (gth_selections_manager_get_type ())
#define GTH_SELECTIONS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_SELECTIONS_MANAGER, GthSelectionsManager))
#define GTH_SELECTIONS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_SELECTIONS_MANAGER, GthSelectionsManagerClass))
#define GTH_IS_SELECTIONS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_SELECTIONS_MANAGER))
#define GTH_IS_SELECTIONS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_SELECTIONS_MANAGER))
#define GTH_SELECTIONS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_SELECTIONS_MANAGER, GthSelectionsManagerClass))

typedef struct _GthSelectionsManager         GthSelectionsManager;
typedef struct _GthSelectionsManagerPrivate  GthSelectionsManagerPrivate;
typedef struct _GthSelectionsManagerClass    GthSelectionsManagerClass;

struct _GthSelectionsManager {
	GObject __parent;
	GthSelectionsManagerPrivate *priv;
};

struct _GthSelectionsManagerClass {
	GObjectClass __parent_class;
};

GType    gth_selections_manager_get_type         (void) G_GNUC_CONST;

void     gth_selections_manager_for_each_child   (GFile                *folder,
						  const char           *attributes,
						  GCancellable         *cancellable,
						  ForEachChildCallback  for_each_file_func,
						  ReadyCallback         ready_callback,
						  gpointer              user_data);
gboolean gth_selections_manager_add_files        (GFile                *folder,
						  GList                *file_list, /* GFile list */
						  int                   destination_position);
void     gth_selections_manager_remove_files     (GFile                *folder,
						  GList                *file_list,
						  gboolean              notify);
void     gth_selections_manager_reorder          (GFile                *folder,
						  GList                *visible_files, /* GFile list */
						  GList                *files_to_move, /* GFile list */
						  int                   dest_pos);
void     gth_selections_manager_set_sort_type    (GFile                *folder,
						  const char           *sort_type,
						  gboolean              sort_inverse);
void     gth_selections_manager_update_file_info (GFile                *file,
						  GFileInfo            *info);
gboolean gth_selections_manager_file_exists      (int                   n_selection,
						  GFile                *file);
gboolean gth_selections_manager_get_is_empty     (int                   n_selection);

/* utilities */

int      _g_file_get_n_selection                 (GFile                *file);
const char *
	 gth_selection_get_icon_name		 (int			n_selection);
const char *
	 gth_selection_get_symbolic_icon_name	 (int			n_selection);

G_END_DECLS

#endif /* GTH_SELECTIONS_MANAGER_H */

