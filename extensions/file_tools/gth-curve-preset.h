/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
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

#ifndef GTH_CURVE_PRESET_H
#define GTH_CURVE_PRESET_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-points.h"

G_BEGIN_DECLS

typedef enum {
	GTH_PRESET_ACTION_ADDED,
	GTH_PRESET_ACTION_RENAMED,
	GTH_PRESET_ACTION_REMOVED,
	GTH_PRESET_ACTION_CHANGED_ORDER
} GthPresetAction;

#define GTH_TYPE_CURVE_PRESET (gth_curve_preset_get_type ())
#define GTH_CURVE_PRESET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CURVE_PRESET, GthCurvePreset))
#define GTH_CURVE_PRESET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CURVE_PRESET, GthCurvePresetClass))
#define GTH_IS_CURVE_PRESET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CURVE_PRESET))
#define GTH_IS_CURVE_PRESET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CURVE_PRESET))
#define GTH_CURVE_PRESET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CURVE_PRESET, GthCurvePresetClass))

typedef struct _GthCurvePresetPrivate GthCurvePresetPrivate;

typedef struct {
	GObject parent_instance;
	GthCurvePresetPrivate *priv;
} GthCurvePreset;

typedef struct {
	GObjectClass parent_class;

	/*< signals >*/

	void	(*changed)		(GthCurvePreset		*self);
	void	(*preset_changed)	(GthCurvePreset		*self,
					 int             	 id,
					 GthPresetAction	 action);
} GthCurvePresetClass;

GType			gth_curve_preset_get_type		(void);
GthCurvePreset *	gth_curve_preset_new_from_file		(GFile		 *file);
int			gth_curve_preset_get_size		(GthCurvePreset  *self);
gboolean		gth_curve_preset_get_nth		(GthCurvePreset  *self,
								 int		  n,
								 int		 *id,
								 const char	**name,
								 GthPoints	**points);
gboolean		gth_curve_preset_get_by_id		(GthCurvePreset  *self,
								 int		  id,
								 const char	**name,
								 GthPoints	**points);
int			gth_curve_preset_get_pos		(GthCurvePreset  *self,
								 int		  id);
int			gth_curve_preset_add			(GthCurvePreset  *self,
								 const char	 *name,
								 GthPoints	 *points);
void			gth_curve_preset_remove			(GthCurvePreset  *self,
								 int              id);
void			gth_curve_preset_rename			(GthCurvePreset  *self,
								 int              id,
								 const char      *new_name);
void			gth_curve_preset_change_order		(GthCurvePreset  *self,
								 GList		 *id_list);
GList *			gth_curve_preset_get_order		(GthCurvePreset  *self);
gboolean		gth_curve_preset_save			(GthCurvePreset  *self,
								 GError		**error);

#endif /* GTH_CURVE_PRESET_H */
