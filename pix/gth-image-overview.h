/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_OVERVIEW_H
#define GTH_IMAGE_OVERVIEW_H

#include <gtk/gtk.h>
#include "gth-image-viewer.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_OVERVIEW            (gth_image_overview_get_type ())
#define GTH_IMAGE_OVERVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_OVERVIEW, GthImageOverview))
#define GTH_IMAGE_OVERVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_OVERVIEW, GthImageOverviewClass))
#define GTH_IS_IMAGE_OVERVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_OVERVIEW))
#define GTH_IS_IMAGE_OVERVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_OVERVIEW))
#define GTH_IMAGE_OVERVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_OVERVIEW, GthImageOverviewClass))

typedef struct _GthImageOverview        GthImageOverview;
typedef struct _GthImageOverviewClass   GthImageOverviewClass;
typedef struct _GthImageOverviewPrivate GthImageOverviewPrivate;

struct _GthImageOverview {
	GtkWidget __parent;
	GthImageOverviewPrivate *priv;
};

struct _GthImageOverviewClass {
	GtkWidgetClass __parent;
};

GType		gth_image_overview_get_type		(void);
GtkWidget *	gth_image_overview_new			(GthImageViewer		*viewer);
void            gth_image_overview_get_size         	(GthImageOverview	*viewer,
							 int			*width,
							 int			*height);
void            gth_image_overview_get_visible_area	(GthImageOverview	*viewer,
							 int			*x,
							 int			*y,
							 int			*width,
							 int			*height);
void		gth_image_overview_activate_scrolling	(GthImageOverview	*self,
							 gboolean		 active,
							 GdkEventButton		*event);
gboolean	gth_image_overview_get_scrolling_is_active
							(GthImageOverview	*self);
gboolean        gth_image_overview_has_preview		(GthImageOverview	*self);

G_END_DECLS

#endif /* GTH_IMAGE_OVERVIEW_H */

