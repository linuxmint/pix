/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

#ifndef GTH_SEARCH_SOURCE_H
#define GTH_SEARCH_SOURCE_H

#include <glib-object.h>
#include <gio/gio.h>

#define GTH_TYPE_SEARCH_SOURCE         (gth_search_source_get_type ())
#define GTH_SEARCH_SOURCE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_SEARCH_SOURCE, GthSearchSource))
#define GTH_SEARCH_SOURCE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_SEARCH_SOURCE, GthSearchSourceClass))
#define GTH_IS_SEARCH_SOURCE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_SEARCH_SOURCE))
#define GTH_IS_SEARCH_SOURCE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_SEARCH_SOURCE))
#define GTH_SEARCH_SOURCE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_SEARCH_SOURCE, GthSearchSourceClass))

typedef struct _GthSearchSource         GthSearchSource;
typedef struct _GthSearchSourcePrivate  GthSearchSourcePrivate;
typedef struct _GthSearchSourceClass    GthSearchSourceClass;

struct _GthSearchSource
{
	GObject __parent;
	GthSearchSourcePrivate *priv;
};

struct _GthSearchSourceClass
{
	GObjectClass __parent_class;
};

GType			gth_search_source_get_type	(void) G_GNUC_CONST;
GthSearchSource *	gth_search_source_new		(void);
void			gth_search_source_set_folder	(GthSearchSource	*source,
							 GFile			*folder);
GFile *			gth_search_source_get_folder	(GthSearchSource	*source);
void			gth_search_source_set_recursive (GthSearchSource	*source,
							 gboolean		 recursive);
gboolean		gth_search_source_is_recursive  (GthSearchSource	*source);

#endif /* GTH_SEARCH_SOURCE_H */
