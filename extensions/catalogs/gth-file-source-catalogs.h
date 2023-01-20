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

#ifndef GTH_FILE_SOURCE_CATALOGS_H
#define GTH_FILE_SOURCE_CATALOGS_H

#include <gthumb.h>

#define GTH_TYPE_FILE_SOURCE_CATALOGS         (gth_file_source_catalogs_get_type ())
#define GTH_FILE_SOURCE_CATALOGS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_SOURCE_CATALOGS, GthFileSourceCatalogs))
#define GTH_FILE_SOURCE_CATALOGS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_SOURCE_CATALOGS, GthFileSourceCatalogsClass))
#define GTH_IS_FILE_SOURCE_CATALOGS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_SOURCE_CATALOGS))
#define GTH_IS_FILE_SOURCE_CATALOGS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_SOURCE_CATALOGS))
#define GTH_FILE_SOURCE_CATALOGS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_SOURCE_CATALOGS, GthFileSourceCatalogsClass))

typedef struct _GthFileSourceCatalogs         GthFileSourceCatalogs;
typedef struct _GthFileSourceCatalogsPrivate  GthFileSourceCatalogsPrivate;
typedef struct _GthFileSourceCatalogsClass    GthFileSourceCatalogsClass;

struct _GthFileSourceCatalogs
{
	GthFileSource __parent;
	GthFileSourceCatalogsPrivate *priv;
};

struct _GthFileSourceCatalogsClass
{
	GthFileSourceClass __parent_class;
};

GType gth_file_source_catalogs_get_type (void) G_GNUC_CONST;
void  gth_catalog_manager_remove_files  (GtkWindow   *parent,
				  	 GthFileData *location,
				  	 GList       *file_list /* GFile list */,
					 gboolean     notify);

#endif /* GTH_FILE_SOURCE_CATALOGS_H */
