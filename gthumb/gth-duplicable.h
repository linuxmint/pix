/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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
 
#ifndef GTH_DUPLICABLE_H
#define GTH_DUPLICABLE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GTH_TYPE_DUPLICABLE               (gth_duplicable_get_type ())
#define GTH_DUPLICABLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_DUPLICABLE, GthDuplicable))
#define GTH_IS_DUPLICABLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_DUPLICABLE))
#define GTH_DUPLICABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_DUPLICABLE, GthDuplicableInterface))

typedef struct _GthDuplicable GthDuplicable;
typedef struct _GthDuplicableInterface GthDuplicableInterface;

struct _GthDuplicableInterface {
	GTypeInterface parent_iface;
	
	GObject * (*duplicate) (GthDuplicable *self);
};

GType      gth_duplicable_get_type   (void);
GObject *  gth_duplicable_duplicate  (GthDuplicable *self);

G_END_DECLS

#endif /* GTH_DUPLICABLE_H */
