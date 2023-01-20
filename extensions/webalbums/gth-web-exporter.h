/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2010 Free Software Foundation, Inc.
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

#ifndef GTH_WEB_EXPORTER_H
#define GTH_WEB_EXPORTER_H

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gthumb.h>

#define GTH_TYPE_WEB_EXPORTER            (gth_web_exporter_get_type ())
#define GTH_WEB_EXPORTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_WEB_EXPORTER, GthWebExporter))
#define GTH_WEB_EXPORTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_WEB_EXPORTER, GthWebExporterClass))
#define GTH_IS_WEB_EXPORTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_WEB_EXPORTER))
#define GTH_IS_WEB_EXPORTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_WEB_EXPORTER))
#define GTH_WEB_EXPORTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_WEB_EXPORTER, GthWebExporterClass))

typedef struct _GthWebExporter        GthWebExporter;
typedef struct _GthWebExporterClass   GthWebExporterClass;
typedef struct _GthWebExporterPrivate GthWebExporterPrivate;

struct _GthWebExporter {
	GthTask __parent;
	GthWebExporterPrivate *priv;

};

struct _GthWebExporterClass {
	GthTaskClass __parent;
};

GType      gth_web_exporter_get_type              (void);
GthTask *  gth_web_exporter_new                   (GthBrowser       *browser,
						   GList            *file_list); /* GFile list */
void       gth_web_exporter_set_header            (GthWebExporter   *self,
						   const char       *value);
void       gth_web_exporter_set_footer            (GthWebExporter   *self,
						   const char       *value);
void       gth_web_exporter_set_image_page_header (GthWebExporter   *self,
						   const char       *value);
void       gth_web_exporter_set_image_page_footer (GthWebExporter   *self,
						   const char       *value);
void       gth_web_exporter_set_style             (GthWebExporter   *self,
						   const char       *style_name);
void       gth_web_exporter_set_destination       (GthWebExporter   *self,
						   GFile            *destination);
void       gth_web_exporter_set_use_subfolders    (GthWebExporter   *self,
						   gboolean          use_subfolders);
void       gth_web_exporter_set_copy_images       (GthWebExporter   *self,
						   gboolean          copy);
void       gth_web_exporter_set_resize_images     (GthWebExporter   *self,
						   gboolean          resize,
						   int               max_width,
						   int               max_height);
void       gth_web_exporter_set_sort_order        (GthWebExporter   *self,
						   GthFileDataSort  *sort_type,
						   gboolean          sort_inverse);
void       gth_web_exporter_set_images_per_index  (GthWebExporter   *self,
						   int               value);
void       gth_web_exporter_set_single_index      (GthWebExporter   *self,
						   gboolean          single);
void       gth_web_exporter_set_columns           (GthWebExporter   *self,
						   int               cols);
void       gth_web_exporter_set_adapt_to_width    (GthWebExporter   *self,
						   gboolean          value);
void       gth_web_exporter_set_thumb_size        (GthWebExporter   *self,
						   gboolean          squared,
						   int               width,
						   int               height);
void       gth_web_exporter_set_preview_size      (GthWebExporter   *self,
						   int               width,
						   int               height);
void       gth_web_exporter_set_preview_min_size  (GthWebExporter   *self,
					           int               width,
					           int               height);
void       gth_web_exporter_set_image_attributes  (GthWebExporter   *self,
						   gboolean          image_description_enabled,
						   const char       *caption);
void       gth_web_exporter_set_thumbnail_caption (GthWebExporter   *self,
						   const char       *caption);

#endif /* GTH_WEB_EXPORTER_H */
