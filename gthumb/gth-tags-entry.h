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

#ifndef GTH_TAGS_ENTRY_H
#define GTH_TAGS_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	GTH_TAGS_ENTRY_MODE_POPUP,
	GTH_TAGS_ENTRY_MODE_INLINE
} GthTagsEntryMode;

#define GTH_TYPE_TAGS_ENTRY            (gth_tags_entry_get_type ())
#define GTH_TAGS_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TAGS_ENTRY, GthTagsEntry))
#define GTH_TAGS_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TAGS_ENTRY, GthTagsEntryClass))
#define GTH_IS_TAGS_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TAGS_ENTRY))
#define GTH_IS_TAGS_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TAGS_ENTRY))
#define GTH_TAGS_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TAGS_ENTRY, GthTagsEntryClass))

typedef struct _GthTagsEntry GthTagsEntry;
typedef struct _GthTagsEntryClass GthTagsEntryClass;
typedef struct _GthTagsEntryPrivate GthTagsEntryPrivate;

struct _GthTagsEntry {
	GtkBox parent_instance;
	GthTagsEntryPrivate *priv;
};

struct _GthTagsEntryClass {
	GtkBoxClass parent_class;

	/*< signals >*/

	void (*list_collapsed) (GthTagsEntry *self);
};

GType        gth_tags_entry_get_type             (void);
GtkWidget *  gth_tags_entry_new                  (GthTagsEntryMode	  mode);
void         gth_tags_entry_set_list_visible	 (GthTagsEntry		 *self,
					          gboolean		  visible);
gboolean     gth_tags_entry_get_list_visible     (GthTagsEntry		 *self);
void         gth_tags_entry_set_tags             (GthTagsEntry		 *self,
				                  char			**tags);
void         gth_tags_entry_set_tags_from_text   (GthTagsEntry		 *self,
				                  const char		 *text);
char **      gth_tags_entry_get_tags             (GthTagsEntry		 *self,
				                  gboolean		  update_globals);
void         gth_tags_entry_set_tag_list         (GthTagsEntry		 *self,
						  GList			 *checked,
						  GList 		 *inconsistent);
void         gth_tags_entry_get_tag_list         (GthTagsEntry		 *self,
						  gboolean		  update_globals,
						  GList			**checked,
						  GList			**inconsistent);

G_END_DECLS

#endif /* GTH_TAGS_ENTRY_H */
