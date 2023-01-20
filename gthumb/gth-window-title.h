/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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

#ifndef GTH_WINDOW_TITLE_H
#define GTH_WINDOW_TITLE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_WINDOW_TITLE            (gth_window_title_get_type ())
#define GTH_WINDOW_TITLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_WINDOW_TITLE, GthWindowTitle))
#define GTH_WINDOW_TITLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_WINDOW_TITLE, GthWindowTitleClass))
#define GTH_IS_WINDOW_TITLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_WINDOW_TITLE))
#define GTH_IS_WINDOW_TITLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_WINDOW_TITLE))
#define GTH_WINDOW_TITLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_WINDOW_TITLE, GthWindowTitleClass))

typedef struct _GthWindowTitle GthWindowTitle;
typedef struct _GthWindowTitleClass GthWindowTitleClass;
typedef struct _GthWindowTitlePrivate GthWindowTitlePrivate;

struct _GthWindowTitle {
	GtkBox parent_instance;
	GthWindowTitlePrivate *priv;
};

struct _GthWindowTitleClass {
	GtkBoxClass parent_class;
};

GType            gth_window_title_get_type	(void);
GtkWidget *      gth_window_title_new		(void);
void             gth_window_title_set_title	(GthWindowTitle	*window_title,
						 const char	*title);
void             gth_window_title_set_emblems	(GthWindowTitle	*window_title,
						 GList		*emblems);

G_END_DECLS

#endif /* GTH_WINDOW_TITLE_H */
