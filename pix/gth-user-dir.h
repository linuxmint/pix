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

#ifndef GTH_USER_DIR_H
#define GTH_USER_DIR_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
	GTH_DIR_CONFIG,
	GTH_DIR_CACHE,
	GTH_DIR_DATA
} GthDir;

#define GTHUMB_DIR "gthumb"

void    gth_user_dir_mkdir_with_parents     (GthDir      dir_type,
					     const char *first_element,
                                             ...) G_GNUC_NULL_TERMINATED;
GFile * gth_user_dir_get_dir_for_write      (GthDir      dir_type,
					     const char *first_element,
					     ...) G_GNUC_NULL_TERMINATED;
GFile * gth_user_dir_get_file_for_read     (GthDir      dir_type,
					    const char *first_element,
				            ...) G_GNUC_NULL_TERMINATED;
GFile * gth_user_dir_get_file_for_write    (GthDir      dir_type,
					    const char *first_element,
				            ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* GTH_USER_DIR_H */
