/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_EMPTY_LIST_H
#define GTH_EMPTY_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

G_BEGIN_DECLS

#define GTH_TYPE_EMPTY_LIST            (gth_empty_list_get_type ())
#define GTH_EMPTY_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EMPTY_LIST, GthEmptyList))
#define GTH_EMPTY_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EMPTY_LIST, GthEmptyListClass))
#define GTH_IS_EMPTY_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EMPTY_LIST))
#define GTH_IS_EMPTY_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EMPTY_LIST))
#define GTH_EMPTY_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EMPTY_LIST, GthEmptyListClass))

typedef struct _GthEmptyList GthEmptyList;
typedef struct _GthEmptyListClass GthEmptyListClass;
typedef struct _GthEmptyListPrivate GthEmptyListPrivate;

struct _GthEmptyList {
	GtkWidget parent_instance;
	GthEmptyListPrivate * priv;
};

struct _GthEmptyListClass {
	GtkWidgetClass parent_class;
};

GType        gth_empty_list_get_type  (void);
GtkWidget *  gth_empty_list_new       (const char   *text);
void         gth_empty_list_set_text  (GthEmptyList *self,
				       const char   *text);

G_END_DECLS

#endif /* GTH_EMPTY_LIST_H */
