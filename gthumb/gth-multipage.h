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

#ifndef GTH_MULTIPAGE_H
#define GTH_MULTIPAGE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_MULTIPAGE              (gth_multipage_get_type ())
#define GTH_MULTIPAGE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MULTIPAGE, GthMultipage))
#define GTH_MULTIPAGE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MULTIPAGE, GthMultipageClass))
#define GTH_IS_MULTIPAGE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MULTIPAGE))
#define GTH_IS_MULTIPAGE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_MULTIPAGE))
#define GTH_MULTIPAGE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_MULTIPAGE, GthMultipageClass))

#define GTH_TYPE_MULTIPAGE_CHILD               (gth_multipage_child_get_type ())
#define GTH_MULTIPAGE_CHILD(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MULTIPAGE_CHILD, GthMultipageChild))
#define GTH_IS_MULTIPAGE_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MULTIPAGE_CHILD))
#define GTH_MULTIPAGE_CHILD_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_MULTIPAGE_CHILD, GthMultipageChildInterface))

typedef struct _GthMultipage        GthMultipage;
typedef struct _GthMultipageClass   GthMultipageClass;
typedef struct _GthMultipagePrivate GthMultipagePrivate;

struct _GthMultipage
{
	GtkBox __parent;
	GthMultipagePrivate *priv;
};

struct _GthMultipageClass
{
	GtkBoxClass __parent_class;

	void  (*changed) (GthMultipage *self);
};

typedef struct _GthMultipageChild GthMultipageChild;
typedef struct _GthMultipageChildInterface GthMultipageChildInterface;

struct _GthMultipageChildInterface {
	GTypeInterface parent_iface;
	const char *  (*get_name)  (GthMultipageChild *self);
	const char *  (*get_icon)  (GthMultipageChild *self);
};

GType          gth_multipage_get_type         (void);
GtkWidget *    gth_multipage_new              (void);
void           gth_multipage_add_child        (GthMultipage      *multipage,
					       GthMultipageChild *child);
GList *        gth_multipage_get_children     (GthMultipage      *multipage);
void           gth_multipage_set_current      (GthMultipage      *multipage,
					       int                index_);
int            gth_multipage_get_current      (GthMultipage      *multipage);

GType          gth_multipage_child_get_type   (void);
const char *   gth_multipage_child_get_name   (GthMultipageChild *self);
const char *   gth_multipage_child_get_icon   (GthMultipageChild *self);

G_END_DECLS

#endif /* GTH_MULTIPAGE_H */
