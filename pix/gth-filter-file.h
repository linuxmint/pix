/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GTH_FILTER_FILE_H
#define GTH_FILTER_FILE_H

#include <glib.h>
#include "gth-test.h"

G_BEGIN_DECLS

typedef struct _GthFilterFile GthFilterFile;

GthFilterFile *  gth_filter_file_new              (void);
void             gth_filter_file_free             (GthFilterFile  *filters);
gboolean         gth_filter_file_load_from_data   (GthFilterFile  *filters,
						   const char     *data,
						   gsize           length,
						   GError        **error);
gboolean         gth_filter_file_load_from_file   (GthFilterFile  *bookmark,
						   GFile          *file,
                                                   GError        **error);
char *           gth_filter_file_to_data          (GthFilterFile  *filters,
						   gsize          *len,
			 			   GError        **data_error);
gboolean         gth_filter_file_to_file          (GthFilterFile  *filters,
						   GFile          *file,
                                                   GError        **error);
GList *          gth_filter_file_get_tests        (GthFilterFile  *filters);
gboolean         gth_filter_file_has_test         (GthFilterFile  *filters,
						   GthTest        *test);
void             gth_filter_file_add              (GthFilterFile  *filters,
						   GthTest        *test);
void             gth_filter_file_remove           (GthFilterFile  *filters,
						   GthTest        *test);
void             gth_filter_file_clear            (GthFilterFile  *filters);

G_END_DECLS

#endif /* GTH_FILTER_FILE_H */
