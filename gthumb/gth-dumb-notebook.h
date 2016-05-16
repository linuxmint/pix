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
 
#ifndef GTH_DUMB_NOTEBOOK_H
#define GTH_DUMB_NOTEBOOK_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_DUMB_NOTEBOOK            (gth_dumb_notebook_get_type ())
#define GTH_DUMB_NOTEBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_DUMB_NOTEBOOK, GthDumbNotebook))
#define GTH_DUMB_NOTEBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_DUMB_NOTEBOOK, GthDumbNotebookClass))
#define GTH_IS_DUMB_NOTEBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_DUMB_NOTEBOOK))
#define GTH_IS_DUMB_NOTEBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_DUMB_NOTEBOOK))
#define GTH_DUMB_NOTEBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_DUMB_NOTEBOOK, GthDumbNotebookClass))

typedef struct _GthDumbNotebook GthDumbNotebook;
typedef struct _GthDumbNotebookClass GthDumbNotebookClass;
typedef struct _GthDumbNotebookPrivate GthDumbNotebookPrivate;

struct _GthDumbNotebook {
	GtkContainer parent_instance;
	GthDumbNotebookPrivate *priv;
};

struct _GthDumbNotebookClass {
	GtkContainerClass parent_class;
};

GType        gth_dumb_notebook_get_type      (void);
GtkWidget *  gth_dumb_notebook_new           (void);
void         gth_dumb_notebook_show_child    (GthDumbNotebook *notebook,
					      int              pos);

G_END_DECLS

#endif /* GTH_DUMB_NOTEBOOK_H */
