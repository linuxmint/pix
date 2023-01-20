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

#ifndef GTH_TAGS_FILE_H
#define GTH_TAGS_FILE_H

#include <glib.h>
#include "gth-test.h"

G_BEGIN_DECLS

typedef struct _GthTagsFile GthTagsFile;

GthTagsFile *  gth_tags_file_new              (void);
void           gth_tags_file_free             (GthTagsFile  *tags);
gboolean       gth_tags_file_load_from_data   (GthTagsFile  *tags,
					       const char   *data,
					       gsize         length,
					       GError      **error);
gboolean       gth_tags_file_load_from_file   (GthTagsFile  *bookmark,
                                               GFile        *file,
                                               GError      **error);
char *         gth_tags_file_to_data          (GthTagsFile  *tags,
					       gsize        *len,
					       GError      **data_error);
gboolean       gth_tags_file_to_file          (GthTagsFile  *tags,
					       GFile        *filename,
                                               GError      **error);
char **        gth_tags_file_get_tags         (GthTagsFile  *tags);
gboolean       gth_tags_file_has_tag          (GthTagsFile  *tags,
					       const char   *tag);
gboolean       gth_tags_file_add              (GthTagsFile  *tags,
					       const char   *tag);
gboolean       gth_tags_file_remove           (GthTagsFile  *tags,
					       const char   *tag);
void           gth_tags_file_clear            (GthTagsFile  *tags);

G_END_DECLS

#endif /* GTH_TAGS_FILE_H */
