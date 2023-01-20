/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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

#ifndef GTH_CURVE_EDITOR_H
#define GTH_CURVE_EDITOR_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-points.h"

G_BEGIN_DECLS

#define GTH_TYPE_CURVE_EDITOR (gth_curve_editor_get_type ())
#define GTH_CURVE_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CURVE_EDITOR, GthCurveEditor))
#define GTH_CURVE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CURVE_EDITOR, GthCurveEditorClass))
#define GTH_IS_CURVE_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CURVE_EDITOR))
#define GTH_IS_CURVE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CURVE_EDITOR))
#define GTH_CURVE_EDITOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CURVE_EDITOR, GthCurveEditorClass))

typedef struct _GthCurveEditor GthCurveEditor;
typedef struct _GthCurveEditorClass GthCurveEditorClass;
typedef struct _GthCurveEditorPrivate GthCurveEditorPrivate;

struct _GthCurveEditor {
	GtkBox parent_instance;
	GthCurveEditorPrivate *priv;
};

struct _GthCurveEditorClass {
	GtkBoxClass parent_class;

	/*< signals >*/

	void	(*changed)		(GthCurveEditor	*self);
};

GType              gth_curve_editor_get_type		(void);
GtkWidget *        gth_curve_editor_new			(GthHistogram		 *histogram);
void               gth_curve_editor_set_histogram	(GthCurveEditor		 *self,
							 GthHistogram		 *histogram);
GthHistogram *     gth_curve_editor_get_histogram	(GthCurveEditor		 *self);
void               gth_curve_editor_set_current_channel	(GthCurveEditor		 *self,
							 int			  n_channel);
int                gth_curve_editor_get_current_channel	(GthCurveEditor		 *self);
void               gth_curve_editor_set_scale_type	(GthCurveEditor		 *self,
							 GthHistogramScale	  scale);
GthHistogramScale  gth_curve_editor_get_scale_type	(GthCurveEditor		 *self);
void		   gth_curve_editor_set_points		(GthCurveEditor		 *self,
							 GthPoints		 *points);
void		   gth_curve_editor_get_points		(GthCurveEditor		 *self,
							 GthPoints		 *points);
void		   gth_curve_editor_reset		(GthCurveEditor		 *self);

G_END_DECLS

#endif /* GTH_CURVE_EDITOR_H */
