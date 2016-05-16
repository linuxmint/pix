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

#ifndef GTH_METADATA_H
#define GTH_METADATA_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "gth-string-list.h"

G_BEGIN_DECLS

typedef enum {
	GTH_METADATA_TYPE_STRING,
	GTH_METADATA_TYPE_STRING_LIST
} GthMetadataType;

typedef struct {
	const char *id;
	const char *display_name;
	int         sort_order;
} GthMetadataCategory;

typedef enum {
	GTH_METADATA_ALLOW_NOWHERE = 0,
	GTH_METADATA_ALLOW_IN_FILE_LIST = 1 << 0,
	GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW = 1 << 1,
	GTH_METADATA_ALLOW_IN_PRINT = 1 << 2
} GthMetadataFlags;

#define GTH_METADATA_ALLOW_EVERYWHERE (GTH_METADATA_ALLOW_IN_FILE_LIST | GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT)

typedef struct {
	const char         *id;
	const char         *display_name;
	const char         *category;
	int                 sort_order;
	const char         *type;
	GthMetadataFlags    flags;
} GthMetadataInfo;

#define GTH_TYPE_METADATA (gth_metadata_get_type ())
#define GTH_METADATA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_METADATA, GthMetadata))
#define GTH_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_METADATA, GthMetadataClass))
#define GTH_IS_METADATA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_METADATA))
#define GTH_IS_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_METADATA))
#define GTH_METADATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_METADATA, GthMetadataClass))

typedef struct _GthMetadata GthMetadata;
typedef struct _GthMetadataClass GthMetadataClass;
typedef struct _GthMetadataPrivate GthMetadataPrivate;

struct _GthMetadata {
	GObject parent_instance;
	GthMetadataPrivate *priv;
};

struct _GthMetadataClass {
	GObjectClass parent_class;
};

GType             gth_metadata_get_type             (void);
GthMetadata *     gth_metadata_new                  (void);
GthMetadata *     gth_metadata_new_for_string_list  (GthStringList   *list);
GthMetadataType   gth_metadata_get_data_type        (GthMetadata     *metadata);
const char *      gth_metadata_get_id               (GthMetadata     *metadata);
const char *      gth_metadata_get_raw              (GthMetadata     *metadata);
GthStringList *   gth_metadata_get_string_list      (GthMetadata     *metadata);
const char *      gth_metadata_get_formatted        (GthMetadata     *metadata);
const char *      gth_metadata_get_value_type       (GthMetadata     *metadata);
GthMetadata *     gth_metadata_dup                  (GthMetadata     *metadata);
GthMetadataInfo * gth_metadata_info_dup             (GthMetadataInfo *info);
void              set_attribute_from_string         (GFileInfo       *info,
						     const char      *key,
						     const char      *raw,
						     const char      *formatted);

G_END_DECLS

#endif /* GTH_METADATA_H */
