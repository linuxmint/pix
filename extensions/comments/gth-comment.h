/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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

#ifndef GTH_COMMENT_H
#define GTH_COMMENT_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gthumb.h>

#define GTH_TYPE_COMMENT (gth_comment_get_type ())
#define GTH_COMMENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_COMMENT, GthComment))
#define GTH_COMMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_COMMENT, GthCommentClass))
#define GTH_IS_COMMENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_COMMENT))
#define GTH_IS_COMMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_COMMENT))
#define GTH_COMMENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_COMMENT, GthCommentClass))

typedef struct _GthComment GthComment;
typedef struct _GthCommentClass GthCommentClass;
typedef struct _GthCommentPrivate GthCommentPrivate;

struct _GthComment {
        GObject     parent_instance;
        GthCommentPrivate *priv;
};

struct _GthCommentClass {
        GObjectClass parent_class;
};

GFile *           gth_comment_get_comment_file           (GFile         *file);
GType             gth_comment_get_type                   (void);
GthComment *      gth_comment_new                        (void);
GthComment *      gth_comment_new_for_file               (GFile         *file,
							  GCancellable  *cancellable,
							  GError       **error);
char *            gth_comment_to_data                    (GthComment    *comment,
							  gsize         *length);
GthComment *      gth_comment_dup                        (GthComment    *comment);
void              gth_comment_reset                      (GthComment    *comment);
void              gth_comment_set_caption                (GthComment    *comment,
							  const char    *value);
void              gth_comment_set_note                   (GthComment    *comment,
							  const char    *value);
void              gth_comment_set_place                  (GthComment    *comment,
							  const char    *value);
void              gth_comment_set_rating                 (GthComment    *comment,
							  int            value);
void              gth_comment_clear_categories           (GthComment    *comment);
void              gth_comment_add_category               (GthComment    *comment,
							  const char    *value);
void              gth_comment_reset_time                 (GthComment    *comment);
void              gth_comment_set_time_from_exif_format  (GthComment    *comment,
							  const char    *value);
void              gth_comment_set_time_from_time_t       (GthComment    *comment,
							  time_t         value);
const char *      gth_comment_get_caption                (GthComment    *comment);
const char *      gth_comment_get_note                   (GthComment    *comment);
const char *      gth_comment_get_place                  (GthComment    *comment);
int               gth_comment_get_rating                 (GthComment    *comment);
GPtrArray *       gth_comment_get_categories             (GthComment    *comment);
GDate *           gth_comment_get_date                   (GthComment    *comment);
GthTime *         gth_comment_get_time_of_day            (GthComment    *comment);
char *            gth_comment_get_time_as_exif_format    (GthComment    *comment);
void              gth_comment_update_general_attributes  (GthFileData   *file_data);
void              gth_comment_update_from_general_attributes
							 (GthFileData   *file_data);

#endif /* GTH_COMMENT_H */
