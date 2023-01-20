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

#ifndef GTH_CATALOG_H
#define GTH_CATALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gthumb.h>

typedef enum {
	GTH_CATALOG_TYPE_INVALID,
	GTH_CATALOG_TYPE_CATALOG,
	GTH_CATALOG_TYPE_EXTENSION
} GthCatalogType;

#define GTH_TYPE_CATALOG         (gth_catalog_get_type ())
#define GTH_CATALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_CATALOG, GthCatalog))
#define GTH_CATALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_CATALOG, GthCatalogClass))
#define GTH_IS_CATALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_CATALOG))
#define GTH_IS_CATALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_CATALOG))
#define GTH_CATALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_CATALOG, GthCatalogClass))

typedef struct _GthCatalog         GthCatalog;
typedef struct _GthCatalogPrivate  GthCatalogPrivate;
typedef struct _GthCatalogClass    GthCatalogClass;

struct _GthCatalog
{
	GObject __parent;
	GValueHash *attributes;
	GthCatalogPrivate *priv;
};

struct _GthCatalogClass
{
	GObjectClass __parent_class;

	/*< virtual functions >*/

	DomElement  * (*create_root)    (GthCatalog  *catalog,
					 DomDocument *doc);
	void          (*read_from_doc)  (GthCatalog  *catalog,
					 DomElement  *root);
	void          (*write_to_doc)   (GthCatalog  *catalog,
					 DomDocument *doc,
					 DomElement  *root);
};

typedef void (*CatalogReadyCallback) (GthCatalog *catalog,
				      GList      *files,
				      GError     *error,
				      gpointer    user_data);

GType         gth_catalog_get_type        (void) G_GNUC_CONST;
GthCatalog *  gth_catalog_new             (void);
GthCatalog *  gth_catalog_new_for_file    (GFile                *file);
GthCatalog *  gth_catalog_new_from_data   (const void           *buffer,
					   gsize                 count,
					   GError              **error);
void          gth_catalog_set_file        (GthCatalog           *catalog,
					   GFile                *file);
GFile *       gth_catalog_get_file        (GthCatalog           *catalog);
void          gth_catalog_set_name        (GthCatalog           *catalog,
					   const char           *name);
const char *  gth_catalog_get_name        (GthCatalog           *catalog);
void          gth_catalog_set_date        (GthCatalog           *catalog,
					   GthDateTime          *date_time);
GthDateTime * gth_catalog_get_date        (GthCatalog           *catalog);
void          gth_catalog_set_order       (GthCatalog           *catalog,
					   const char           *order,
					   gboolean              inverse);
const char *  gth_catalog_get_order       (GthCatalog           *catalog,
					   gboolean             *inverse);
char *        gth_catalog_to_data         (GthCatalog           *catalog,
		     			   gsize                *length);
void          gth_catalog_set_file_list   (GthCatalog           *catalog,
					   GList                *file_list);
GList *       gth_catalog_get_file_list   (GthCatalog           *catalog);
gboolean      gth_catalog_insert_file     (GthCatalog           *catalog,
					   GFile                *file,
					   int                   pos);
int           gth_catalog_remove_file     (GthCatalog           *catalog,
					   GFile                *file);
void          gth_catalog_update_metadata (GthCatalog           *catalog,
					   GthFileData          *file_data);
int           gth_catalog_get_size        (GthCatalog           *catalog);

/* utils */

GFile *        gth_catalog_get_base                   (void);
GFile *        gth_catalog_file_to_gio_file           (GFile         *file);
GFile *        gth_catalog_file_from_gio_file         (GFile         *file,
						       GFile         *catalog);
GFile *        gth_catalog_file_from_relative_path    (const char    *name,
						       const char    *file_extension);
char *         gth_catalog_get_relative_path          (GFile         *file);
GIcon *        gth_catalog_get_icon                   (GFile         *file);
void           gth_catalog_update_standard_attributes (GFile         *file,
						       GFileInfo     *info);
void           gth_catalog_load_from_file_async       (GFile         *file,
						       GCancellable  *cancellable,
						       ReadyCallback  ready_func,
						       gpointer       user_data);
GFile *        gth_catalog_get_file_for_date          (GthDateTime   *date_time,
	      	      	       	       	       	       const char    *extension);
GFile *        gth_catalog_get_file_for_tag           (const char    *tag,
		      	      	      	      	       const char    *extension);
GthCatalog *   gth_catalog_load_from_file             (GFile         *file);
void           gth_catalog_save                       (GthCatalog    *catalog);
void           gth_catalog_list_async                 (GFile               *catalog,
						       const char           *attributes,
						       GCancellable         *cancellable,
						       CatalogReadyCallback  ready_func,
						       gpointer              user_data);

#endif /*GTH_CATALOG_H*/
