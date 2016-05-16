/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_SOURCE_SELECTIONS_H
#define GTH_FILE_SOURCE_SELECTIONS_H

#include <gthumb.h>

#define GTH_TYPE_FILE_SOURCE_SELECTIONS         (gth_file_source_selections_get_type ())
#define GTH_FILE_SOURCE_SELECTIONS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_SOURCE_SELECTIONS, GthFileSourceSelections))
#define GTH_FILE_SOURCE_SELECTIONS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_SOURCE_SELECTIONS, GthFileSourceSelectionsClass))
#define GTH_IS_FILE_SOURCE_SELECTIONS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_SOURCE_SELECTIONS))
#define GTH_IS_FILE_SOURCE_SELECTIONS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_SOURCE_SELECTIONS))
#define GTH_FILE_SOURCE_SELECTIONS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_SOURCE_SELECTIONS, GthFileSourceSelectionsClass))

typedef struct _GthFileSourceSelections         GthFileSourceSelections;
typedef struct _GthFileSourceSelectionsPrivate  GthFileSourceSelectionsPrivate;
typedef struct _GthFileSourceSelectionsClass    GthFileSourceSelectionsClass;

struct _GthFileSourceSelections
{
	GthFileSource __parent;
	GthFileSourceSelectionsPrivate *priv;
};

struct _GthFileSourceSelectionsClass
{
	GthFileSourceClass __parent_class;
};

GType gth_file_source_selections_get_type (void) G_GNUC_CONST;

#endif /* GTH_FILE_SOURCE_SELECTIONS_H */
