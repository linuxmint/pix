/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2009 Free Software Foundation, Inc.
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

#ifndef GTH_FILTERBAR_H
#define GTH_FILTERBAR_H

#include <gtk/gtk.h>
#include "gth-test.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILTERBAR         (gth_filterbar_get_type ())
#define GTH_FILTERBAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILTERBAR, GthFilterbar))
#define GTH_FILTERBAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILTERBAR, GthFilterbarClass))
#define GTH_IS_FILTERBAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILTERBAR))
#define GTH_IS_FILTERBAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILTERBAR))
#define GTH_FILTERBAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILTERBAR, GthFilterbarClass))

typedef struct _GthFilterbar         GthFilterbar;
typedef struct _GthFilterbarPrivate  GthFilterbarPrivate;
typedef struct _GthFilterbarClass    GthFilterbarClass;

struct _GthFilterbar
{
	GtkBox __parent;
	GthFilterbarPrivate *priv;
};

struct _GthFilterbarClass
{
	GtkBoxClass __parent_class;

	/* -- Signals -- */

	void (* changed)              (GthFilterbar *filterbar);
	void (* personalize)          (GthFilterbar *filterbar);
	void (* close_button_clicked) (GthFilterbar *filterbar);
};

GType         gth_filterbar_get_type    (void) G_GNUC_CONST;
GtkWidget *   gth_filterbar_new         (const char   *selected_filter);
GthTest *     gth_filterbar_get_test    (GthFilterbar *filterbar);
void          gth_filterbar_save_filter (GthFilterbar *filterbar,
					 const char   *filename);
void          gth_filterbar_load_filter (GthFilterbar *filterbar,
					 const char   *filename);

G_END_DECLS

#endif /* GTH_FILTERBAR_H */
