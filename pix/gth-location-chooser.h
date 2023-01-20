/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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

#ifndef GTH_LOCATION_CHOOSER_H
#define GTH_LOCATION_CHOOSER_H

#include <gtk/gtk.h>
#include "gth-file-source.h"

G_BEGIN_DECLS

#define GTH_TYPE_LOCATION_CHOOSER         (gth_location_chooser_get_type ())
#define GTH_LOCATION_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_LOCATION_CHOOSER, GthLocationChooser))
#define GTH_LOCATION_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_LOCATION_CHOOSER, GthLocationChooserClass))
#define GTH_IS_LOCATION_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_LOCATION_CHOOSER))
#define GTH_IS_LOCATION_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_LOCATION_CHOOSER))
#define GTH_LOCATION_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_LOCATION_CHOOSER, GthLocationChooserClass))

typedef struct _GthLocationChooser         GthLocationChooser;
typedef struct _GthLocationChooserPrivate  GthLocationChooserPrivate;
typedef struct _GthLocationChooserClass    GthLocationChooserClass;

struct _GthLocationChooser
{
	GtkBox __parent;
	GthLocationChooserPrivate *priv;
};

struct _GthLocationChooserClass
{
	GtkBoxClass __parent_class;

	/* -- Signals -- */

        void (* changed) (GthLocationChooser *loc);
};

GType            gth_location_chooser_get_type               (void) G_GNUC_CONST;
GtkWidget *      gth_location_chooser_new                    (void);
void             gth_location_chooser_set_relief             (GthLocationChooser *chooser,
							      GtkReliefStyle      value);
GtkReliefStyle   gth_location_chooser_get_relief             (GthLocationChooser *chooser);
void             gth_location_chooser_set_show_entry_points  (GthLocationChooser *chooser,
							      gboolean            value);
gboolean         gth_location_chooser_get_show_entry_points  (GthLocationChooser *chooser);
void             gth_location_chooser_set_current            (GthLocationChooser *chooser,
							      GFile              *location);
GFile *          gth_location_chooser_get_current            (GthLocationChooser *chooser);
void             gth_location_chooser_reload                 (GthLocationChooser *chooser);

G_END_DECLS

#endif /* GTH_LOCATION_CHOOSER_H */
