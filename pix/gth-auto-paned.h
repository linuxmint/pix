/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_AUTO_PANED_H
#define GTH_AUTO_PANED_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_AUTO_PANED            (gth_auto_paned_get_type ())
#define GTH_AUTO_PANED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_AUTO_PANED, GthAutoPaned))
#define GTH_AUTO_PANED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_AUTO_PANED, GthAutoPanedClass))
#define GTH_IS_AUTO_PANED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_AUTO_PANED))
#define GTH_IS_AUTO_PANED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_AUTO_PANED))
#define GTH_AUTO_PANED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_AUTO_PANED, GthAutoPanedClass))

typedef struct _GthAutoPaned GthAutoPaned;
typedef struct _GthAutoPanedClass GthAutoPanedClass;
typedef struct _GthAutoPanedPrivate GthAutoPanedPrivate;

struct _GthAutoPaned {
	GtkPaned parent_instance;
	GthAutoPanedPrivate *priv;
};

struct _GthAutoPanedClass {
	GtkPanedClass parent_class;
};

GType        gth_auto_paned_get_type (void);
GtkWidget *  gth_auto_paned_new      (GtkOrientation orientation);

G_END_DECLS

#endif /* GTH_AUTO_PANED_H */
