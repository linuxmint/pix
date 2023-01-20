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

#ifndef GTH_SEARCH_H
#define GTH_SEARCH_H

#include <glib-object.h>
#include <gio/gio.h>
#include <gthumb.h>
#include <extensions/catalogs/gth-catalog.h>

#define GTH_TYPE_SEARCH         (gth_search_get_type ())
#define GTH_SEARCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_SEARCH, GthSearch))
#define GTH_SEARCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_SEARCH, GthSearchClass))
#define GTH_IS_SEARCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_SEARCH))
#define GTH_IS_SEARCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_SEARCH))
#define GTH_SEARCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_SEARCH, GthSearchClass))

typedef struct _GthSearch         GthSearch;
typedef struct _GthSearchPrivate  GthSearchPrivate;
typedef struct _GthSearchClass    GthSearchClass;

struct _GthSearch
{
	GthCatalog __parent;
	GthSearchPrivate *priv;
};

struct _GthSearchClass
{
	GthCatalogClass __parent_class;
};

GType             gth_search_get_type         (void) G_GNUC_CONST;
GthSearch *       gth_search_new              (void);
GthSearch *       gth_search_new_from_data    (void         *buffer,
		   			       gsize         count,
		   			       GError      **error);
void              gth_search_set_sources      (GthSearch    *search,
					       GList        *sources /* GthSearchSource list */);
void              gth_search_set_source       (GthSearch    *search,
					       GFile        *folder,
					       gboolean      recursive);
GList *           gth_search_get_sources      (GthSearch    *search);
void              gth_search_set_test         (GthSearch    *search,
					       GthTestChain *test);
GthTestChain *    gth_search_get_test         (GthSearch    *search);
gboolean          gth_search_equal            (GthSearch    *a,
					       GthSearch    *b);

#endif /* GTH_SEARCH_H */
