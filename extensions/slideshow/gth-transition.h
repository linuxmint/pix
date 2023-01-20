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

#ifndef GTH_TRANSITION_H
#define GTH_TRANSITION_H

#include <glib-object.h>
#include "gth-slideshow.h"

G_BEGIN_DECLS

#define GTH_TRANSITION_DURATION 650

#define GTH_TYPE_TRANSITION              (gth_transition_get_type ())
#define GTH_TRANSITION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TRANSITION, GthTransition))
#define GTH_TRANSITION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TRANSITION_TYPE, GthTransitionClass))
#define GTH_IS_TRANSITION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TRANSITION))
#define GTH_IS_TRANSITION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TRANSITION))
#define GTH_TRANSITION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TRANSITION, GthTransitionClass))

typedef struct _GthTransition         GthTransition;
typedef struct _GthTransitionClass    GthTransitionClass;
typedef struct _GthTransitionPrivate  GthTransitionPrivate;

typedef void (*FrameFunc) (GthSlideshow *slideshow, double progress);

struct _GthTransition
{
	GObject __parent;
	GthTransitionPrivate *priv;
};

struct _GthTransitionClass
{
	GObjectClass __parent_class;
};

GType             gth_transition_get_type         (void);
const char *      gth_transition_get_id           (GthTransition *self);
const char *      gth_transition_get_display_name (GthTransition *self);
void              gth_transition_frame            (GthTransition *self,
						   GthSlideshow  *slideshow,
						   double         progress);

G_END_DECLS

#endif /* GTH_TRANSITION_H */
