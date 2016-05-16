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

#ifndef GTH_URI_LIST_H
#define GTH_URI_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_URI_LIST            (gth_uri_list_get_type ())
#define GTH_URI_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_URI_LIST, GthUriList))
#define GTH_URI_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_URI_LIST, GthUriListClass))
#define GTH_IS_URI_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_URI_LIST))
#define GTH_IS_URI_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_URI_LIST))
#define GTH_URI_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_URI_LIST, GthUriListClass))

typedef struct _GthUriList GthUriList;
typedef struct _GthUriListClass GthUriListClass;
typedef struct _GthUriListPrivate GthUriListPrivate;

struct _GthUriList {
	GtkTreeView parent_instance;
	GthUriListPrivate *priv;
};

struct _GthUriListClass {
	GtkTreeViewClass parent_class;

	/*< signals >*/

	void   (*order_changed)  (GthUriList *uri_list);
};

GType            gth_uri_list_get_type         (void);
GtkWidget *      gth_uri_list_new              (void);
void             gth_uri_list_set_uris         (GthUriList     *uri_list,
				                char          **uris);
void             gth_uri_list_set_bookmarks    (GthUriList     *uri_list,
					        GBookmarkFile  *bookmarks);
char *           gth_uri_list_get_uri          (GthUriList     *uri_list,
			   		        GtkTreeIter    *iter);
char *           gth_uri_list_get_selected     (GthUriList     *uri_list);
GList *          gth_uri_list_get_uris         (GthUriList     *uri_list);
void             gth_uri_list_update_bookmarks (GthUriList     *uri_list,
						GBookmarkFile  *bookmarks);
gboolean         gth_uri_list_remove_uri       (GthUriList     *uri_list,
				                const char     *uri);
gboolean         gth_uri_list_select_uri       (GthUriList     *uri_list,
						const char     *uri);
gboolean         gth_uri_list_update_uri       (GthUriList     *uri_list,
				                const char     *uri,
				                const char     *new_uri,
				                const char     *new_name);

G_END_DECLS

#endif /* GTH_URI_LIST_H */
