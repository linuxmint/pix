
/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_DRAGGER_H
#define GTH_IMAGE_DRAGGER_H

#include <glib.h>
#include <glib-object.h>
#include "gth-image-viewer.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_DRAGGER            (gth_image_dragger_get_type ())
#define GTH_IMAGE_DRAGGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_DRAGGER, GthImageDragger))
#define GTH_IMAGE_DRAGGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_DRAGGER, GthImageDraggerClass))
#define GTH_IS_IMAGE_DRAGGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_DRAGGER))
#define GTH_IS_IMAGE_DRAGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_DRAGGER))
#define GTH_IMAGE_DRAGGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_DRAGGER, GthImageDraggerClass))

typedef struct _GthImageDragger         GthImageDragger;
typedef struct _GthImageDraggerClass    GthImageDraggerClass;
typedef struct _GthImageDraggerPrivate  GthImageDraggerPrivate;

struct _GthImageDragger
{
	GObject __parent;
	GthImageDraggerPrivate *priv;
};

struct _GthImageDraggerClass
{
	GObjectClass __parent_class;
};

GType			gth_image_dragger_get_type		(void);
GthImageViewerTool *	gth_image_dragger_new			(gboolean		 show_frame);
void			gth_image_dragger_enable_drag_source	(GthImageDragger	*self,
								 GdkModifierType	 start_button_mask,
								 const GtkTargetEntry	*targets,
								 int			 n_targets,
								 GdkDragAction		 actions);
void			gth_image_dragger_disable_drag_source	(GthImageDragger	*self);

G_END_DECLS

#endif /* GTH_IMAGE_DRAGGER_H */
