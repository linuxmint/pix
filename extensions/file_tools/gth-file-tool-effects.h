/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_TOOL_EFFECTS_H
#define GTH_FILE_TOOL_EFFECTS_H

#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_TOOL_EFFECTS (gth_file_tool_effects_get_type ())
#define GTH_FILE_TOOL_EFFECTS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_TOOL_EFFECTS, GthFileToolEffects))
#define GTH_FILE_TOOL_EFFECTS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_TOOL_EFFECTS, GthFileToolEffectsClass))
#define GTH_IS_FILE_TOOL_EFFECTS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_TOOL_EFFECTS))
#define GTH_IS_FILE_TOOL_EFFECTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_TOOL_EFFECTS))
#define GTH_FILE_TOOL_EFFECTS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILE_TOOL_EFFECTS, GthFileToolEffectsClass))

typedef struct _GthFileToolEffects GthFileToolEffects;
typedef struct _GthFileToolEffectsClass GthFileToolEffectsClass;
typedef struct _GthFileToolEffectsPrivate GthFileToolEffectsPrivate;

struct _GthFileToolEffects {
	GthImageViewerPageTool parent_instance;
	GthFileToolEffectsPrivate *priv;
};

struct _GthFileToolEffectsClass {
	GthImageViewerPageToolClass parent_class;
};

GType  gth_file_tool_effects_get_type  (void);

void warmer_add_to_special_effects		(GthFilterGrid *);
void cooler_add_to_special_effects		(GthFilterGrid *);
void soil_add_to_special_effects		(GthFilterGrid *);
void desert_add_to_special_effects		(GthFilterGrid *);
void artic_add_to_special_effects		(GthFilterGrid *);
void mangos_add_to_special_effects		(GthFilterGrid *);
void fresh_blue_add_to_special_effects		(GthFilterGrid *);
void cherry_add_to_special_effects		(GthFilterGrid *);
void vintage_add_to_special_effects		(GthFilterGrid *);
void blurred_edges_add_to_special_effects	(GthFilterGrid *);
void vignette_add_to_special_effects		(GthFilterGrid *);

G_END_DECLS

#endif /* GTH_FILE_TOOL_EFFECTS_H */
