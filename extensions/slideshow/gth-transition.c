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

#include <config.h>
#include "gth-transition.h"


/* Properties */
enum {
	PROP_0,
	PROP_ID,
	PROP_DISPLAY_NAME,
	PROP_FRAME_FUNC
};


struct _GthTransitionPrivate {
	char      *id;
	char      *display_name;
	FrameFunc  frame_func;
};


G_DEFINE_TYPE_WITH_CODE (GthTransition,
			 gth_transition,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthTransition))


static void
gth_transition_finalize (GObject *object)
{
	GthTransition *self = GTH_TRANSITION (object);

	g_free (self->priv->id);
	g_free (self->priv->display_name);

	G_OBJECT_CLASS (gth_transition_parent_class)->finalize (object);
}


static void
gth_transition_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GthTransition *self = GTH_TRANSITION (object);

	switch (property_id) {
	case PROP_ID:
		g_free (self->priv->id);
		self->priv->id = g_value_dup_string (value);
		if (self->priv->id == NULL)
			self->priv->id = g_strdup ("");
		break;

	case PROP_DISPLAY_NAME:
		g_free (self->priv->display_name);
		self->priv->display_name = g_value_dup_string (value);
		if (self->priv->display_name == NULL)
			self->priv->display_name = g_strdup ("");
		break;

	case PROP_FRAME_FUNC:
		self->priv->frame_func = g_value_get_pointer (value);
		break;

	default:
		break;
	}
}


static void
gth_transition_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GthTransition *self = GTH_TRANSITION (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->priv->id);
		break;

	case PROP_DISPLAY_NAME:
		g_value_set_string (value, self->priv->display_name);
		break;

	case PROP_FRAME_FUNC:
		g_value_set_pointer (value, self->priv->frame_func);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
gth_transition_class_init (GthTransitionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->get_property = gth_transition_get_property;
	object_class->set_property = gth_transition_set_property;
	object_class->finalize = gth_transition_finalize;

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
                                                              "ID",
                                                              "The object id",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DISPLAY_NAME,
					 g_param_spec_string ("display-name",
                                                              "Display name",
                                                              "The user visible name",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FRAME_FUNC,
					 g_param_spec_pointer ("frame-func",
							       "Frame Function",
							       "The function used to set the current frame",
							       G_PARAM_READWRITE));
}


static void
gth_transition_init (GthTransition *self)
{
	self->priv = gth_transition_get_instance_private (self);
	self->priv->id = g_strdup ("");
	self->priv->display_name = g_strdup ("");
	self->priv->frame_func = NULL;
}


const char *
gth_transition_get_id (GthTransition *self)
{
	return self->priv->id;
}


const char *
gth_transition_get_display_name (GthTransition *self)
{
	return self->priv->display_name;
}


void
gth_transition_frame (GthTransition *self,
		      GthSlideshow  *slideshow,
		      double         progress)
{
	if (self->priv->frame_func != NULL)
		self->priv->frame_func (slideshow, progress);
}
