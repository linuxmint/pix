/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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

#ifndef GTH_FILTER_GRID_H
#define GTH_FILTER_GRID_H

#include <glib.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>

G_BEGIN_DECLS

#define GTH_FILTER_GRID_NO_FILTER -1
#define GTH_FILTER_GRID_NEW_FILTER_ID -2

#define GTH_TYPE_FILTER_GRID (gth_filter_grid_get_type ())
#define GTH_FILTER_GRID(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILTER_GRID, GthFilterGrid))
#define GTH_FILTER_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILTER_GRID, GthFilterGridClass))
#define GTH_IS_FILTER_GRID(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILTER_GRID))
#define GTH_IS_FILTER_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILTER_GRID))
#define GTH_FILTER_GRID_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILTER_GRID, GthFilterGridClass))

typedef struct _GthFilterGrid GthFilterGrid;
typedef struct _GthFilterGridClass GthFilterGridClass;
typedef struct _GthFilterGridPrivate GthFilterGridPrivate;

struct _GthFilterGrid {
	GtkBox parent_instance;
	GthFilterGridPrivate *priv;
};

struct _GthFilterGridClass {
	GtkBoxClass parent_class;

	/*< signals >*/

	void	(*activated)	(GthFilterGrid	*self,
				 int             filter_id);
};

GType		gth_filter_grid_get_type		(void);
GtkWidget *	gth_filter_grid_new			(void);
void		gth_filter_grid_add_filter		(GthFilterGrid		*self,
							 int			 filter_id,
							 GthTask		*task,
							 const char		*label,
							 const char		*tooltip);
void		gth_filter_grid_add_filter_with_preview	(GthFilterGrid		*self,
							 int			 filter_id,
							 cairo_surface_t	*preview,
							 const char		*label,
							 const char		*tooltip);
void		gth_filter_grid_set_filter_preview	(GthFilterGrid		*self,
							 int			 filter_id,
							 cairo_surface_t	*preview);
void            gth_filter_grid_activate                (GthFilterGrid		*self,
							 int			 filter_id);
void            gth_filter_grid_generate_previews       (GthFilterGrid		*self,
							 cairo_surface_t	*image);
void            gth_filter_grid_generate_preview        (GthFilterGrid		*self,
							 int			 filter_id,
							 cairo_surface_t	*image);
GthTask *       gth_filter_grid_get_task		(GthFilterGrid		*self,
							 int			 filter_id);
void		gth_filter_grid_rename_filter		(GthFilterGrid		*self,
							 int			 filter_id,
							 const char		*new_name);
void		gth_filter_grid_remove_filter		(GthFilterGrid		*self,
							 int			 filter_id);
void		gth_filter_grid_change_order		(GthFilterGrid		*self,
							 GList			*id_list);

G_END_DECLS

#endif /* GTH_FILTER_GRID_H */
