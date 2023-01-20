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

#ifndef GTH_MEDIA_VIEWER_PAGE_H
#define GTH_MEDIA_VIEWER_PAGE_H

#include <gst/gst.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_MEDIA_VIEWER_PAGE (gth_media_viewer_page_get_type ())
#define GTH_MEDIA_VIEWER_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MEDIA_VIEWER_PAGE, GthMediaViewerPage))
#define GTH_MEDIA_VIEWER_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MEDIA_VIEWER_PAGE, GthMediaViewerPageClass))
#define GTH_IS_MEDIA_VIEWER_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MEDIA_VIEWER_PAGE))
#define GTH_IS_MEDIA_VIEWER_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_MEDIA_VIEWER_PAGE))
#define GTH_MEDIA_VIEWER_PAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_MEDIA_VIEWER_PAGE, GthMediaViewerPageClass))

typedef struct _GthMediaViewerPage GthMediaViewerPage;
typedef struct _GthMediaViewerPageClass GthMediaViewerPageClass;
typedef struct _GthMediaViewerPagePrivate GthMediaViewerPagePrivate;

struct _GthMediaViewerPage {
	GObject parent_instance;
	GthMediaViewerPagePrivate * priv;
};

struct _GthMediaViewerPageClass {
	GObjectClass parent_class;
};

GType          gth_media_viewer_page_get_type      (void);
GthBrowser *   gth_media_viewer_page_get_browser    (GthMediaViewerPage *self);
GstElement *   gth_media_viewer_page_get_playbin    (GthMediaViewerPage *self);
gboolean       gth_media_viewer_page_is_playing     (GthMediaViewerPage *self);
void           gth_media_viewer_page_get_video_fps  (GthMediaViewerPage *self,
						     int                *video_fps_n,
						     int                *video_fps_d);
GthFileData *  gth_media_viewer_page_get_file_data  (GthMediaViewerPage *self);
void           gth_media_viewer_page_toggle_play    (GthMediaViewerPage *self);
void           gth_media_viewer_page_set_fit_if_larger
						    (GthMediaViewerPage *self,
						     gboolean            fit_if_larger);
void           gth_media_viewer_page_skip	    (GthMediaViewerPage *self,
						     int                 seconds);
void           gth_media_viewer_page_next_frame	    (GthMediaViewerPage *self);
void           gth_media_viewer_page_toggle_mute    (GthMediaViewerPage *self);
void           gth_media_viewer_page_play_faster    (GthMediaViewerPage *self);
void           gth_media_viewer_page_play_slower    (GthMediaViewerPage *self);

G_END_DECLS

#endif /* GTH_MEDIA_VIEWER_PAGE_H */
