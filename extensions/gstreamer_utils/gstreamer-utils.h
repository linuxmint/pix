/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

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

#ifndef GSTREAMER_UTILS_H
#define GSTREAMER_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <gthumb.h>

G_BEGIN_DECLS

typedef void (*FrameReadyCallback) (GdkPixbuf *, gpointer user_data);

gboolean    gstreamer_init                    (void);
gboolean    gstreamer_read_metadata_from_file (GFile               *file,
					       GFileInfo           *info,
					       GError             **error);
gboolean    _gst_playbin_get_current_frame    (GstElement          *playbin,
					       int                  video_fps_n,
					       int                  video_fps_d,
					       FrameReadyCallback   cb,
					       gpointer             user_data);

G_END_DECLS

#endif /* GSTREAMER_UTILS_H */
