/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_VIEW_H
#define GTH_FILE_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_VIEW               (gth_file_view_get_type ())
#define GTH_FILE_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_VIEW, GthFileView))
#define GTH_IS_FILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_VIEW))
#define GTH_FILE_VIEW_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_FILE_VIEW, GthFileViewInterface))

typedef struct _GthFileView GthFileView;
typedef struct _GthFileViewInterface GthFileViewInterface;

typedef enum  {
	GTH_VISIBILITY_NONE,
	GTH_VISIBILITY_FULL,
	GTH_VISIBILITY_PARTIAL,
	GTH_VISIBILITY_PARTIAL_TOP,
	GTH_VISIBILITY_PARTIAL_BOTTOM
} GthVisibility;

typedef enum {
	GTH_FILE_VIEW_RENDERER_CHECKBOX,
	GTH_FILE_VIEW_RENDERER_THUMBNAIL,
	GTH_FILE_VIEW_RENDERER_TEXT
} GthFileViewRendererType;

struct _GthFileViewInterface {
	GTypeInterface parent_iface;

	/*< signals >*/

	void            (*cursor_changed)                (GthFileView              *self,
							  int                       pos);
	void            (*file_activated)                (GthFileView              *self,
					     	     	  int                       pos);

	/*< virtual functions >*/

	void            (*scroll_to)                     (GthFileView              *self,
							  int                       pos,
							  double                    yalign);
	void            (*set_vscroll)                   (GthFileView              *self,
							  double                    vscroll);
	GthVisibility   (*get_visibility)                (GthFileView              *self,
							  int                       pos);
	int             (*get_at_position)               (GthFileView              *self,
							  int                       x,
							  int                       y);
	int             (*get_first_visible)             (GthFileView              *self);
	int             (*get_last_visible)              (GthFileView              *self);
	int             (*get_cursor)                    (GthFileView              *self);
	void            (*enable_drag_source)            (GthFileView              *self,
							  GdkModifierType           start_button_mask,
							  const GtkTargetEntry     *targets,
							  gint                      n_targets,
							  GdkDragAction             actions);
	void            (*unset_drag_source)             (GthFileView              *self);
	void            (*enable_drag_dest)              (GthFileView              *self,
							  const GtkTargetEntry     *targets,
							  gint                      n_targets,
							  GdkDragAction             actions);
	void            (*unset_drag_dest)               (GthFileView              *self);
	void            (*set_drag_dest_pos)             (GthFileView              *self,
							  GdkDragContext           *context,
							  int                       x,
							  int                       y,
							  guint                     time,
			                      	          int                      *pos);
	void            (*get_drag_dest_pos)             (GthFileView             *self,
							  int                     *pos);

};

GType           gth_file_view_get_type           (void);
void            gth_file_view_set_model          (GthFileView             *self,
					 	  GtkTreeModel            *model);
GtkTreeModel *  gth_file_view_get_model          (GthFileView             *self);
void            gth_file_view_set_caption        (GthFileView             *self,
						  const char              *attributes);
char *          gth_file_view_get_caption        (GthFileView             *self);
void            gth_file_view_set_thumbnail_size (GthFileView             *self,
						  int                      value);
gboolean        gth_file_view_get_thumbnail_size (GthFileView             *self);
void		gth_file_view_set_activate_on_single_click
						 (GthFileView             *self,
						  gboolean		   single);
gboolean        gth_file_view_get_activate_on_single_click
						 (GthFileView             *self);
void            gth_file_view_scroll_to          (GthFileView             *self,
						  int                      pos,
						  double                   yalign);
void            gth_file_view_set_vscroll        (GthFileView             *self,
						  double                   vscroll);
GthVisibility   gth_file_view_get_visibility     (GthFileView             *self,
						  int                      pos);
int             gth_file_view_get_at_position    (GthFileView             *self,
						  int                      x,
						  int                      y);
int             gth_file_view_get_first_visible  (GthFileView             *self);
int             gth_file_view_get_last_visible   (GthFileView             *self);
void            gth_file_view_activated          (GthFileView             *self,
						  int                      pos);
void            gth_file_view_set_cursor         (GthFileView             *self,
						  int                      pos);
int             gth_file_view_get_cursor         (GthFileView             *self);
void            gth_file_view_enable_drag_source (GthFileView             *self,
						  GdkModifierType          start_button_mask,
						  const GtkTargetEntry    *targets,
						  int                      n_targets,
						  GdkDragAction            actions);
void            gth_file_view_unset_drag_source  (GthFileView             *self);
void            gth_file_view_enable_drag_dest   (GthFileView             *self,
						  const GtkTargetEntry    *targets,
						  int                      n_targets,
						  GdkDragAction            actions);
void            gth_file_view_unset_drag_dest    (GthFileView             *self);
void            gth_file_view_set_drag_dest_pos  (GthFileView             *self,
						  GdkDragContext          *context,
						  int                      x,
						  int                      y,
						  guint                    time,
						  int                     *pos);
void            gth_file_view_get_drag_dest_pos  (GthFileView             *self,
						  int                     *pos);

G_END_DECLS

#endif /* GTH_FILE_VIEW_H */
