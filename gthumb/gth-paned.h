/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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

#ifndef GTH_PANED_H
#define GTH_PANED_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_PANED            (gth_paned_get_type ())
#define GTH_PANED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PANED, GthPaned))
#define GTH_PANED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PANED, GthPanedClass))
#define GTH_IS_PANED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PANED))
#define GTH_IS_PANED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PANED))
#define GTH_PANED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_PANED, GthPanedClass))

typedef struct _GthPaned GthPaned;
typedef struct _GthPanedClass GthPanedClass;
typedef struct _GthPanedPrivate GthPanedPrivate;

struct _GthPaned {
	GtkPaned __parent;
	GthPanedPrivate *priv;
};

struct _GthPanedClass {
	GtkPanedClass __parent_class;
};

GType          gth_paned_get_type       (void);
GtkWidget *    gth_paned_new            (GtkOrientation  orientation);
void           gth_paned_set_position2  (GthPaned       *paned,
					 int             position);
int            gth_paned_get_position2  (GthPaned       *paned);

G_END_DECLS

#endif /* GTH_PANED_H */
