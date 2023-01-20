/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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

#ifndef GTH_METADATA_CHOOSER_H
#define GTH_METADATA_CHOOSER_H

#include <gtk/gtk.h>
#include "gth-metadata.h"

G_BEGIN_DECLS

#define GTH_TYPE_METADATA_CHOOSER            (gth_metadata_chooser_get_type ())
#define GTH_METADATA_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_METADATA_CHOOSER, GthMetadataChooser))
#define GTH_METADATA_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_METADATA_CHOOSER, GthMetadataChooserClass))
#define GTH_IS_METADATA_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_METADATA_CHOOSER))
#define GTH_IS_METADATA_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_METADATA_CHOOSER))
#define GTH_METADATA_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_METADATA_CHOOSER, GthMetadataChooserClass))

typedef struct _GthMetadataChooser GthMetadataChooser;
typedef struct _GthMetadataChooserClass GthMetadataChooserClass;
typedef struct _GthMetadataChooserPrivate GthMetadataChooserPrivate;

struct _GthMetadataChooser {
	GtkTreeView parent_instance;
	GthMetadataChooserPrivate *priv;
};

struct _GthMetadataChooserClass {
	GtkTreeViewClass parent_class;

	/*< signals >*/

	void  (*changed)  (GthMetadataChooser *self);
};

GType        gth_metadata_chooser_get_type      (void);
GtkWidget *  gth_metadata_chooser_new           (GthMetadataFlags    allowed_flags,
						 gboolean            reorderable);
void         gth_metadata_chooser_set_selection (GthMetadataChooser *self,
						 char               *ids);
char *       gth_metadata_chooser_get_selection (GthMetadataChooser *self);

G_END_DECLS

#endif /* GTH_METADATA_CHOOSER_H */

