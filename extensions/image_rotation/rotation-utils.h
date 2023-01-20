/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005, 2009 Free Software Foundation, Inc.
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

#ifndef ROTATION_UTILS_H
#define ROTATION_UTILS_H

#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/jpeg_utils/jpegtran.h>


typedef void (*TrimResponseFunc) (JpegMcuAction action, gpointer user_data);

GtkWidget *     ask_whether_to_trim            (GtkWindow        *parent_window,
		     				GthFileData      *file_data,
		     				TrimResponseFunc  done_func,
		     				gpointer          done_data);
GthTransform	get_next_transformation	       (GthTransform      original,
						GthTransform      transform);
void            apply_transformation_async     (GthFileData      *file_data,
						GthTransform      transform,
						JpegMcuAction     mcu_action,
						GCancellable     *cancellable,
						ReadyFunc         ready_func,
						gpointer          data);

#endif /* ROTATION_UTILS_H */
