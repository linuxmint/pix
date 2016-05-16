/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_VIEWER_H
#define GTH_IMAGE_VIEWER_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-image.h"

typedef struct _GthImageViewer GthImageViewer;

#include "gth-image-viewer-tool.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_VIEWER            (gth_image_viewer_get_type ())
#define GTH_IMAGE_VIEWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_VIEWER, GthImageViewer))
#define GTH_IMAGE_VIEWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_VIEWER, GthImageViewerClass))
#define GTH_IS_IMAGE_VIEWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_VIEWER))
#define GTH_IS_IMAGE_VIEWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_VIEWER))
#define GTH_IMAGE_VIEWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_VIEWER, GthImageViewerClass))


typedef struct _GthImageViewerClass    GthImageViewerClass;
typedef struct _GthImageViewerPrivate  GthImageViewerPrivate;

#define GTH_IMAGE_VIEWER_FRAME_BORDER    1
#define GTH_IMAGE_VIEWER_FRAME_BORDER2   (GTH_IMAGE_VIEWER_FRAME_BORDER * 2)

typedef void (*GthImageViewerPaintFunc) (GthImageViewer *image_viewer,
					 cairo_t        *cr,
					 gpointer        user_data);

typedef enum {
	GTH_ZOOM_QUALITY_HIGH = 0,
	GTH_ZOOM_QUALITY_LOW
} GthZoomQuality;


typedef enum {
	GTH_FIT_NONE = 0,
	GTH_FIT_SIZE,
	GTH_FIT_SIZE_IF_LARGER,
	GTH_FIT_WIDTH,
	GTH_FIT_WIDTH_IF_LARGER
} GthFit;


typedef enum {
	GTH_ZOOM_CHANGE_ACTUAL_SIZE = 0,
	GTH_ZOOM_CHANGE_KEEP_PREV,
	GTH_ZOOM_CHANGE_FIT_SIZE,
	GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER,
	GTH_ZOOM_CHANGE_FIT_WIDTH,
	GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER
} GthZoomChange;


typedef enum {
	GTH_TRANSP_TYPE_WHITE,
	GTH_TRANSP_TYPE_NONE,
	GTH_TRANSP_TYPE_BLACK,
	GTH_TRANSP_TYPE_CHECKED
} GthTranspType;


typedef enum {
	GTH_CHECK_TYPE_LIGHT,
	GTH_CHECK_TYPE_MIDTONE,
	GTH_CHECK_TYPE_DARK
} GthCheckType;


typedef enum {
	GTH_CHECK_SIZE_SMALL  = 4,
	GTH_CHECK_SIZE_MEDIUM = 8,
	GTH_CHECK_SIZE_LARGE  = 16
} GthCheckSize;


struct _GthImageViewer
{
	GtkWidget __parent;
	GthImageViewerPrivate *priv;

	/*< protected, used by the tools >*/

	cairo_rectangle_int_t  image_area;
	int                    x_offset;           /* Scroll offsets. */
	int                    y_offset;
	gboolean               pressed;
	int                    event_x_start;
	int                    event_y_start;
	int                    event_x_prev;
	int                    event_y_prev;
	gboolean               dragging;
	int                    drag_x_start;
	int                    drag_y_start;
	int                    drag_x_prev;
	int                    drag_y_prev;
	int                    drag_x;
	int                    drag_y;
	GtkAdjustment         *vadj;
	GtkAdjustment         *hadj;
};


struct _GthImageViewerClass
{
	GtkWidgetClass __parent_class;

	/* -- Signals -- */

	void (* clicked)                (GthImageViewer     *viewer);
	void (* zoom_changed)           (GthImageViewer     *viewer);

	/* -- Key binding signals -- */

	void (*scroll)                  (GtkWidget          *widget,
					 GtkScrollType       x_scroll_type,
					 GtkScrollType       y_scroll_type);
	void (* zoom_in)                (GthImageViewer     *viewer);
	void (* zoom_out)               (GthImageViewer     *viewer);
	void (* set_zoom)               (GthImageViewer     *viewer,
					 gdouble             zoom);
	void (* set_fit_mode)           (GthImageViewer     *viewer,
					 GthFit              fit_mode);
};


GType          gth_image_viewer_get_type                 (void);
GtkWidget*     gth_image_viewer_new                      (void);

/* viewer content. */

void           gth_image_viewer_set_animation            (GthImageViewer        *viewer,
							  GdkPixbufAnimation    *animation,
							  int                    original_width,
							  int                    original_height);
void           gth_image_viewer_set_pixbuf               (GthImageViewer        *viewer,
							  GdkPixbuf             *pixbuf,
							  int                    original_width,
							  int                    original_height);
void           gth_image_viewer_set_surface              (GthImageViewer        *viewer,
							  cairo_surface_t       *surface,
							  int                    original_width,
							  int                    original_height);
void           gth_image_viewer_set_image                (GthImageViewer        *viewer,
							  GthImage              *image,
							  int                    original_width,
							  int                    original_height);
void           gth_image_viewer_set_better_quality       (GthImageViewer        *viewer,
						          GthImage              *image,
							  int                    original_width,
							  int                    original_height);
void           gth_image_viewer_set_void                 (GthImageViewer        *viewer);
gboolean       gth_image_viewer_is_void                  (GthImageViewer        *viewer);
void           gth_image_viewer_add_painter              (GthImageViewer        *viewer,
							  GthImageViewerPaintFunc
							  	  	         func,
							  gpointer               user_data);
void           gth_image_viewer_remove_painter           (GthImageViewer        *viewer,
							  GthImageViewerPaintFunc
							  	  	         func,
							  gpointer               user_data);

/* image info. */

int            gth_image_viewer_get_image_width          (GthImageViewer        *viewer);
int            gth_image_viewer_get_image_height         (GthImageViewer        *viewer);
gboolean       gth_image_viewer_get_has_alpha            (GthImageViewer        *viewer);
GdkPixbuf *    gth_image_viewer_get_current_pixbuf       (GthImageViewer        *viewer);
cairo_surface_t *
	       gth_image_viewer_get_current_image        (GthImageViewer        *viewer);
void           gth_image_viewer_get_original_size        (GthImageViewer        *viewer,
				    	    	    	  int                   *width,
				    	    	    	  int                   *height);

/* animation. */

void           gth_image_viewer_start_animation          (GthImageViewer        *viewer);
void           gth_image_viewer_stop_animation           (GthImageViewer        *viewer);
void           gth_image_viewer_step_animation           (GthImageViewer        *viewer);
gboolean       gth_image_viewer_is_animation             (GthImageViewer        *viewer);
gboolean       gth_image_viewer_is_playing_animation     (GthImageViewer        *viewer);

/* zoom. */

void           gth_image_viewer_set_zoom                 (GthImageViewer        *viewer,
							  gdouble                zoom);
gdouble        gth_image_viewer_get_zoom                 (GthImageViewer        *viewer);
void           gth_image_viewer_set_zoom_quality         (GthImageViewer        *viewer,
							  GthZoomQuality         quality);
GthZoomQuality gth_image_viewer_get_zoom_quality         (GthImageViewer        *viewer);
cairo_filter_t gth_image_viewer_get_zoom_quality_filter  (GthImageViewer        *viewer);
void           gth_image_viewer_set_zoom_change          (GthImageViewer        *viewer,
							  GthZoomChange          zoom_change);
GthZoomChange  gth_image_viewer_get_zoom_change          (GthImageViewer        *viewer);
void           gth_image_viewer_zoom_in                  (GthImageViewer        *viewer);
void           gth_image_viewer_zoom_out                 (GthImageViewer        *viewer);
void           gth_image_viewer_set_fit_mode             (GthImageViewer        *viewer,
							  GthFit                 fit_mode);
GthFit         gth_image_viewer_get_fit_mode             (GthImageViewer        *viewer);
void           gth_image_viewer_set_zoom_enabled         (GthImageViewer        *viewer,
							  gboolean               value);
gboolean       gth_image_viewer_get_zoom_enabled         (GthImageViewer        *viewer);
void           gth_image_viewer_enable_zoom_with_keys    (GthImageViewer        *viewer,
							  gboolean               value);

/* visualization options. */

void           gth_image_viewer_set_transp_type          (GthImageViewer        *viewer,
							  GthTranspType          transp_type);
GthTranspType  gth_image_viewer_get_transp_type          (GthImageViewer        *viewer);
void           gth_image_viewer_set_check_type           (GthImageViewer        *viewer,
							  GthCheckType           check_type);
GthCheckType   gth_image_viewer_get_check_type           (GthImageViewer        *viewer);
void           gth_image_viewer_set_check_size           (GthImageViewer        *view,
							  GthCheckSize           check_size);
GthCheckSize   gth_image_viewer_get_check_size           (GthImageViewer        *viewer);

/* misc. */

void           gth_image_viewer_clicked                  (GthImageViewer        *viewer);
void           gth_image_viewer_set_black_background     (GthImageViewer        *viewer,
							  gboolean               set_black);
gboolean       gth_image_viewer_is_black_background      (GthImageViewer        *viewer);
void           gth_image_viewer_set_tool                 (GthImageViewer        *viewer,
							  GthImageViewerTool    *tool);

/* Scrolling. */

void           gth_image_viewer_scroll_to                (GthImageViewer        *viewer,
							  int                    x_offset,
							  int                    y_offset);
void           gth_image_viewer_scroll_to_center         (GthImageViewer        *viewer);
void           gth_image_viewer_scroll_step_x            (GthImageViewer        *viewer,
							  gboolean               increment);
void           gth_image_viewer_scroll_step_y            (GthImageViewer        *viewer,
							  gboolean               increment);
void           gth_image_viewer_scroll_page_x            (GthImageViewer        *viewer,
							  gboolean               increment);
void           gth_image_viewer_scroll_page_y            (GthImageViewer        *viewer,
							  gboolean               increment);
void           gth_image_viewer_get_scroll_offset        (GthImageViewer        *viewer,
							  int                   *x,
							  int                   *y);
void           gth_image_viewer_set_reset_scrollbars     (GthImageViewer        *viewer,
  							  gboolean               reset);
gboolean       gth_image_viewer_get_reset_scrollbars     (GthImageViewer        *viewer);
void           gth_image_viewer_needs_scrollbars         (GthImageViewer        *self,
							  GtkAllocation         *allocation,
							  GtkWidget             *hscrollbar,
							  GtkWidget             *vscrollbar,
							  gboolean              *hscrollbar_visible,
							  gboolean              *vscrollbar_visible);

/* Cursor. */

void           gth_image_viewer_show_cursor              (GthImageViewer        *viewer);
void           gth_image_viewer_hide_cursor              (GthImageViewer        *viewer);
void           gth_image_viewer_set_cursor               (GthImageViewer        *viewer,
							  GdkCursor             *cursor);
gboolean       gth_image_viewer_is_cursor_visible        (GthImageViewer        *viewer);

/* Frame. */

void           gth_image_viewer_show_frame               (GthImageViewer        *viewer);
void           gth_image_viewer_hide_frame               (GthImageViewer        *viewer);
gboolean       gth_image_viewer_is_frame_visible         (GthImageViewer        *viewer);

/*< protected, used by the tools >*/

void           gth_image_viewer_paint                    (GthImageViewer        *viewer,
							  cairo_t               *cr,
							  cairo_surface_t       *surface,
							  int                    src_x,
							  int                    src_y,
							  int                    dest_x,
							  int                    dest_y,
							  int                    width,
							  int                    height,
							  cairo_filter_t         filter);
void           gth_image_viewer_paint_region             (GthImageViewer        *viewer,
							  cairo_t               *cr,
							  cairo_surface_t       *surface,
							  int                    src_x,
							  int                    src_y,
							  cairo_rectangle_int_t *pixbuf_area,
							  cairo_region_t        *region,
							  cairo_filter_t         filter);
void           gth_image_viewer_paint_background         (GthImageViewer        *self,
				   	   	          cairo_t               *cr);
void           gth_image_viewer_apply_painters           (GthImageViewer        *image_viewer,
							  cairo_t               *cr);
void           gth_image_viewer_crop_area                (GthImageViewer        *viewer,
							  cairo_rectangle_int_t *area);

G_END_DECLS

#endif /* GTH_IMAGE_VIEWER_H */
