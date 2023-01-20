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

#ifndef GTH_TIME_SELECTOR_H
#define GTH_TIME_SELECTOR_H

#include <gtk/gtk.h>
#include "gth-time.h"

G_BEGIN_DECLS

#define GTH_TYPE_TIME_SELECTOR         (gth_time_selector_get_type ())
#define GTH_TIME_SELECTOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TIME_SELECTOR, GthTimeSelector))
#define GTH_TIME_SELECTOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TIME_SELECTOR, GthTimeSelectorClass))
#define GTH_IS_TIME_SELECTOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TIME_SELECTOR))
#define GTH_IS_TIME_SELECTOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TIME_SELECTOR))
#define GTH_TIME_SELECTOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TIME_SELECTOR, GthTimeSelectorClass))

typedef struct _GthTimeSelector         GthTimeSelector;
typedef struct _GthTimeSelectorPrivate  GthTimeSelectorPrivate;
typedef struct _GthTimeSelectorClass    GthTimeSelectorClass;

struct _GthTimeSelector
{
	GtkBox __parent;
	GthTimeSelectorPrivate *priv;
};

struct _GthTimeSelectorClass
{
	GtkBoxClass __parent_class;

	/* -- Signals -- */

	void  (* changed) (GthTimeSelector *time_selector);
};

GType         gth_time_selector_get_type       (void) G_GNUC_CONST;
GtkWidget *   gth_time_selector_new            (void);
void          gth_time_selector_show_time      (GthTimeSelector *self,
						gboolean         show,
						gboolean         optional);
void          gth_time_selector_set_value      (GthTimeSelector *self,
					        GthDateTime     *date_time);
void          gth_time_selector_set_exif_date  (GthTimeSelector *self,
					        const char      *exif_date);
void          gth_time_selector_get_value      (GthTimeSelector *self,
						GthDateTime     *date_time);
void          gth_time_selector_focus          (GthTimeSelector *self);

G_END_DECLS

#endif /* GTH_TIME_SELECTOR_H */
