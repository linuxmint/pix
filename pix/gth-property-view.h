/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

#ifndef GTH_PROPERTY_VIEW_H
#define GTH_PROPERTY_VIEW_H

#include <gtk/gtk.h>
#include "gth-file-data.h"

G_BEGIN_DECLS

#define GTH_TYPE_PROPERTY_VIEW               (gth_property_view_get_type ())
#define GTH_PROPERTY_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PROPERTY_VIEW, GthPropertyView))
#define GTH_IS_PROPERTY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PROPERTY_VIEW))
#define GTH_PROPERTY_VIEW_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_PROPERTY_VIEW, GthPropertyViewInterface))

typedef struct _GthPropertyView GthPropertyView;
typedef struct _GthPropertyViewInterface GthPropertyViewInterface;

struct _GthPropertyViewInterface {
	GTypeInterface parent_iface;
	const char *	(*get_name)	(GthPropertyView *self);
	const char *	(*get_icon)	(GthPropertyView *self);
	gboolean	(*can_view)	(GthPropertyView *self,
					 GthFileData     *file_data);
	void		(*set_file)	(GthPropertyView *self,
					 GthFileData     *file_data);
};

GType          gth_property_view_get_type      (void);
const char *   gth_property_view_get_name      (GthPropertyView *self);
const char *   gth_property_view_get_icon      (GthPropertyView *self);
gboolean       gth_property_view_can_view      (GthPropertyView *self,
						GthFileData     *file_data);
void           gth_property_view_set_file      (GthPropertyView *self,
						GthFileData     *file_data);

G_END_DECLS

#endif /* GTH_PROPERTY_VIEW_H */
