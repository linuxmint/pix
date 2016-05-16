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

#ifndef GTH_IMAGE_VIEWER_PAGE_H
#define GTH_IMAGE_VIEWER_PAGE_H

#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_VIEWER_PAGE (gth_image_viewer_page_get_type ())
#define GTH_IMAGE_VIEWER_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_VIEWER_PAGE, GthImageViewerPage))
#define GTH_IMAGE_VIEWER_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_VIEWER_PAGE, GthImageViewerPageClass))
#define GTH_IS_IMAGE_VIEWER_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_VIEWER_PAGE))
#define GTH_IS_IMAGE_VIEWER_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_VIEWER_PAGE))
#define GTH_IMAGE_VIEWER_PAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMAGE_VIEWER_PAGE, GthImageViewerPageClass))

typedef struct _GthImageViewerPage GthImageViewerPage;
typedef struct _GthImageViewerPageClass GthImageViewerPageClass;
typedef struct _GthImageViewerPagePrivate GthImageViewerPagePrivate;

struct _GthImageViewerPage {
	GObject parent_instance;
	GthImageViewerPagePrivate * priv;
};

struct _GthImageViewerPageClass {
	GObjectClass parent_class;
};

GType              gth_image_viewer_page_get_type          (void);
GtkWidget *        gth_image_viewer_page_get_image_viewer  (GthImageViewerPage *page);
GdkPixbuf *        gth_image_viewer_page_get_pixbuf        (GthImageViewerPage *page);
void               gth_image_viewer_page_set_pixbuf        (GthImageViewerPage *page,
							    GdkPixbuf          *pixbuf,
							    gboolean            add_to_history);
cairo_surface_t *  gth_image_viewer_page_get_image         (GthImageViewerPage *page);
void               gth_image_viewer_page_set_image         (GthImageViewerPage *page,
							    cairo_surface_t    *image,
							    gboolean            add_to_history);
void               gth_image_viewer_page_undo              (GthImageViewerPage *page);
void               gth_image_viewer_page_redo              (GthImageViewerPage *page);
GthImageHistory *  gth_image_viewer_page_get_history       (GthImageViewerPage *self);
void               gth_image_viewer_page_reset             (GthImageViewerPage *self);
void               gth_image_viewer_page_copy_image        (GthImageViewerPage *self);
void               gth_image_viewer_page_paste_image       (GthImageViewerPage *self);

G_END_DECLS

#endif /* GTH_IMAGE_VIEWER_PAGE_H */
