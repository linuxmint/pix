/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#ifndef IMPORTER_UTILS_H
#define IMPORTER_UTILS_H

G_BEGIN_DECLS

#include <gthumb.h>

GFile *   gth_import_preferences_get_destination  (void);
GFile *   gth_import_utils_get_file_destination   (GthFileData *file_data,
						   GFile       *destination,
						   const char  *subfolder_template,
						   const char  *event_name,
						   GTimeVal     import_start_time);

G_END_DECLS

#endif /* IMPORTER_UTILS_H */
