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

#ifndef GTH_FILE_TOOL_NEGATIVE_H
#define GTH_FILE_TOOL_NEGATIVE_H

#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_TOOL_NEGATIVE (gth_file_tool_negative_get_type ())
#define GTH_FILE_TOOL_NEGATIVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_TOOL_NEGATIVE, GthFileToolNegative))
#define GTH_FILE_TOOL_NEGATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_TOOL_NEGATIVE, GthFileToolNegativeClass))
#define GTH_IS_FILE_TOOL_NEGATIVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_TOOL_NEGATIVE))
#define GTH_IS_FILE_TOOL_NEGATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_TOOL_NEGATIVE))
#define GTH_FILE_TOOL_NEGATIVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILE_TOOL_NEGATIVE, GthFileToolNegativeClass))

typedef struct _GthFileToolNegative GthFileToolNegative;
typedef struct _GthFileToolNegativeClass GthFileToolNegativeClass;

struct _GthFileToolNegative {
	GthFileTool parent_instance;
};

struct _GthFileToolNegativeClass {
	GthFileToolClass parent_class;
};

GType  gth_file_tool_negative_get_type  (void);

G_END_DECLS

#endif /* GTH_FILE_TOOL_NEGATIVE_H */
