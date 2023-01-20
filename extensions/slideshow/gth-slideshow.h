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

#ifndef GTH_SLIDESHOW_H
#define GTH_SLIDESHOW_H

#include <gthumb.h>
#ifdef HAVE_CLUTTER
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#endif /* HAVE_CLUTTER */

G_BEGIN_DECLS

#define GTH_TYPE_SLIDESHOW              (gth_slideshow_get_type ())
#define GTH_SLIDESHOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SLIDESHOW, GthSlideshow))
#define GTH_SLIDESHOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_SLIDESHOW_TYPE, GthSlideshowClass))
#define GTH_IS_SLIDESHOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SLIDESHOW))
#define GTH_IS_SLIDESHOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SLIDESHOW))
#define GTH_SLIDESHOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SLIDESHOW, GthSlideshowClass))

typedef struct _GthSlideshow         GthSlideshow;
typedef struct _GthSlideshowClass    GthSlideshowClass;
typedef struct _GthSlideshowPrivate  GthSlideshowPrivate;

struct _GthSlideshow
{
	GthWindow __parent;
#ifdef HAVE_CLUTTER
	ClutterActor        *stage;
	ClutterActor        *current_image;
	ClutterActor        *next_image;
	ClutterGeometry      current_geometry;
	ClutterGeometry      next_geometry;
	gboolean             first_frame;
#endif /* HAVE_CLUTTER */
	GthSlideshowPrivate *priv;
};

struct _GthSlideshowClass
{
	GthWindowClass __parent_class;
};

typedef struct {
	void (* construct)       (GthSlideshow *self);
	void (* paused)          (GthSlideshow *self);
	void (* show_cursor)     (GthSlideshow *self);
	void (* hide_cursor)     (GthSlideshow *self);
	void (* finalize)        (GthSlideshow *self);
	void (* image_ready)     (GthSlideshow *self,
			          GthImage     *image);
	void (* load_prev_image) (GthSlideshow *self);
	void (* load_next_image) (GthSlideshow *self);
} GthProjector;

extern GthProjector default_projector;
#ifdef HAVE_CLUTTER
extern GthProjector clutter_projector;
#endif /* HAVE_CLUTTER */

GType            gth_slideshow_get_type         (void);
GtkWidget *      gth_slideshow_new              (GthProjector   *projector,
					 	 GthBrowser     *browser,
					         GList          *file_list /* GthFileData */);
void             gth_slideshow_set_delay        (GthSlideshow   *self,
					         guint           msecs);
void             gth_slideshow_set_automatic    (GthSlideshow   *self,
					         gboolean        automatic);
void             gth_slideshow_set_wrap_around  (GthSlideshow   *self,
					         gboolean        wrap_around);
void             gth_slideshow_set_transitions  (GthSlideshow   *self,
					         GList          *transitions);
void             gth_slideshow_set_playlist     (GthSlideshow   *self,
						 char          **files);
void             gth_slideshow_set_random_order (GthSlideshow   *self,
						 gboolean        random);
void             gth_slideshow_toggle_pause     (GthSlideshow   *self);
void             gth_slideshow_load_next_image  (GthSlideshow   *self);
void             gth_slideshow_load_prev_image  (GthSlideshow   *self);
void             gth_slideshow_next_image_or_resume
						(GthSlideshow   *self);
void             gth_slideshow_close            (GthSlideshow   *self);

G_END_DECLS

#endif /* GTH_SLIDESHOW_H */
