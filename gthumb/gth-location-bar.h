/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef GTH_LOCATION_BAR_H
#define GTH_LOCATION_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_LOCATION_BAR         (gth_location_bar_get_type ())
#define GTH_LOCATION_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_LOCATION_BAR, GthLocationBar))
#define GTH_LOCATION_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_LOCATION_BAR, GthLocationBarClass))
#define GTH_IS_LOCATION_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_LOCATION_BAR))
#define GTH_IS_LOCATION_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_LOCATION_BAR))
#define GTH_LOCATION_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_LOCATION_BAR, GthLocationBarClass))

typedef struct _GthLocationBar         GthLocationBar;
typedef struct _GthLocationBarPrivate  GthLocationBarPrivate;
typedef struct _GthLocationBarClass    GthLocationBarClass;

struct _GthLocationBar
{
	GtkBox __parent;
	GthLocationBarPrivate *priv;
};

struct _GthLocationBarClass
{
	GtkBoxClass __parent_class;
};

GType		gth_location_bar_get_type		(void) G_GNUC_CONST;
GtkWidget *	gth_location_bar_new			(void);
GtkWidget *	gth_location_bar_get_chooser		(GthLocationBar		*dialog);
GtkWidget *	gth_location_bar_get_action_area	(GthLocationBar		*self);

G_END_DECLS

#endif /* GTH_LOCATION_BAR_H */
