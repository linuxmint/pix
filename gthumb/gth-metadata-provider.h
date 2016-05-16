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

#ifndef GTH_METADATA_PROVIDER_H
#define GTH_METADATA_PROVIDER_H

#include "gio-utils.h"
#include "gth-file-data.h"
#include "gth-metadata.h"
#include "typedefs.h"

G_BEGIN_DECLS

typedef enum {
	GTH_METADATA_WRITE_DEFAULT        = 0,
	GTH_METADATA_WRITE_FORCE_EMBEDDED = (1 << 0)
} GthMetadataWriteFlags;

#define GTH_TYPE_METADATA_PROVIDER (gth_metadata_provider_get_type ())
#define GTH_METADATA_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_METADATA_PROVIDER, GthMetadataProvider))
#define GTH_METADATA_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_METADATA_PROVIDER, GthMetadataProviderClass))
#define GTH_IS_METADATA_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_METADATA_PROVIDER))
#define GTH_IS_METADATA_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_METADATA_PROVIDER))
#define GTH_METADATA_PROVIDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_METADATA_PROVIDER, GthMetadataProviderClass))

typedef struct _GthMetadataProvider GthMetadataProvider;
typedef struct _GthMetadataProviderClass GthMetadataProviderClass;

struct _GthMetadataProvider {
	GObject parent_instance;
};

struct _GthMetadataProviderClass {
	GObjectClass parent_class;
	gboolean  (*can_read)  (GthMetadataProvider    *self,
			        const char             *mime_type,
			        char                  **attribute_v);
	gboolean  (*can_write) (GthMetadataProvider    *self,
			        const char             *mime_type,
			        char                  **attribute_v);
	void      (*read)      (GthMetadataProvider    *self,
		                GthFileData            *file_data,
		                const char             *attributes,
		                GCancellable           *cancellable);
	void      (*write)     (GthMetadataProvider    *self,
				GthMetadataWriteFlags   flags,
			        GthFileData            *file_data,
			        const char             *attributes,
			        GCancellable           *cancellable);
};

GType      gth_metadata_provider_get_type   (void);
gboolean   gth_metadata_provider_can_read   (GthMetadataProvider    *self,
					     const char             *mime_type,
					     char                  **attribute_v);
gboolean   gth_metadata_provider_can_write  (GthMetadataProvider    *self,
					     const char             *mime_type,
					     char                  **attribute_v);
void       gth_metadata_provider_read       (GthMetadataProvider    *self,
					     GthFileData            *file_data,
					     const char             *attributes,
					     GCancellable           *cancellable);
void       gth_metadata_provider_write      (GthMetadataProvider    *self,
					     GthMetadataWriteFlags   flags,
					     GthFileData            *file_data,
					     const char             *attributes,
					     GCancellable           *cancellable);
void       _g_query_metadata_async          (GList                  *files,       /* GthFileData * list */
					     const char             *attributes,
					     GCancellable           *cancellable,
					     GAsyncReadyCallback     callback,
					     gpointer                user_data);
GList *    _g_query_metadata_finish         (GAsyncResult           *result,
		  	  	  	     GError                **error);
void       _g_write_metadata_async          (GList                  *files, /* GthFileData * list */
					     GthMetadataWriteFlags   flags,
					     const char             *attributes,
					     GCancellable           *cancellable,
					     GAsyncReadyCallback     callback,
					     gpointer                user_data);
gboolean   _g_write_metadata_finish         (GAsyncResult           *result,
					     GError                **error);
void       _g_query_all_metadata_async      (GList                  *files, /* GFile * list */
					     GthListFlags            flags,
					     const char             *attributes,
					     GCancellable           *cancellable,
					     InfoReadyCallback       ready_func,
					     gpointer                user_data);

G_END_DECLS

#endif /* GTH_METADATA_PROVIDER_H */
