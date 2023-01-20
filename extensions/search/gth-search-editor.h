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

#ifndef GTH_SEARCH_EDITOR_H
#define GTH_SEARCH_EDITOR_H

#include <gtk/gtk.h>
#include "gth-search.h"

#define GTH_TYPE_SEARCH_EDITOR            (gth_search_editor_get_type ())
#define GTH_SEARCH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SEARCH_EDITOR, GthSearchEditor))
#define GTH_SEARCH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SEARCH_EDITOR, GthSearchEditorClass))
#define GTH_IS_SEARCH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SEARCH_EDITOR))
#define GTH_IS_SEARCH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SEARCH_EDITOR))
#define GTH_SEARCH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SEARCH_EDITOR, GthSearchEditorClass))

typedef struct _GthSearchEditor        GthSearchEditor;
typedef struct _GthSearchEditorClass   GthSearchEditorClass;
typedef struct _GthSearchEditorPrivate GthSearchEditorPrivate;

struct _GthSearchEditor {
	GtkBox parent_instance;
	GthSearchEditorPrivate *priv;
};

struct _GthSearchEditorClass {
	GtkBoxClass parent_class;
};

GType       gth_search_editor_get_type   (void);
GtkWidget * gth_search_editor_new        (GthSearch        *search);
void        gth_search_editor_set_search (GthSearchEditor  *self,
				 	  GthSearch        *search);
GthSearch * gth_search_editor_get_search (GthSearchEditor  *self,
					  GError          **error);
void        gth_search_editor_focus_first_rule
					 (GthSearchEditor  *self);

#endif /* GTH_SEARCH_EDITOR_H */
