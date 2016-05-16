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

#ifndef GTH_MAP_VIEW_H
#define GTH_MAP_VIEW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_MAP_VIEW            (gth_map_view_get_type ())
#define GTH_MAP_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MAP_VIEW, GthMapView))
#define GTH_MAP_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MAP_VIEW, GthMapViewClass))
#define GTH_IS_MAP_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MAP_VIEW))
#define GTH_IS_MAP_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_MAP_VIEW))
#define GTH_MAP_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_MAP_VIEW, GthMapViewClass))

typedef struct _GthMapView GthMapView;
typedef struct _GthMapViewClass GthMapViewClass;
typedef struct _GthMapViewPrivate GthMapViewPrivate;

struct _GthMapView {
	GtkBox parent_instance;
	GthMapViewPrivate *priv;
};

struct _GthMapViewClass {
	GtkBoxClass parent_class;
};

GType gth_map_view_get_type (void);

G_END_DECLS

#endif /* GTH_MAP_VIEW_H */
