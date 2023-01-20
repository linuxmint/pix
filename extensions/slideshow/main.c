/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "callbacks.h"
#include "gth-transition.h"
#include "preferences.h"
#include "shortcuts.h"


#ifdef HAVE_CLUTTER

#define VALUE_AT_PROGRESS(v, t)((double) (v) * (double) (t))


static void
no_transition (GthSlideshow *self,
	       double        progress)
{
	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_hide (self->current_image);
		clutter_actor_show (self->next_image);
	}
}


static void
push_from_right_transition (GthSlideshow *self,
			    double        progress)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_x (self->next_image, (float) VALUE_AT_PROGRESS (stage_w, 1.0 - progress) + self->next_geometry.x);
	if (self->current_image != NULL)
		clutter_actor_set_x (self->current_image, (float) VALUE_AT_PROGRESS (- stage_w, progress) + self->current_geometry.x);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_show (self->current_image);
		clutter_actor_show (self->next_image);
	}
}


static void
push_from_bottom_transition (GthSlideshow *self,
			     double        progress)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_y (self->next_image, (float) VALUE_AT_PROGRESS (stage_h, 1.0 - progress) + self->next_geometry.y);
	if (self->current_image != NULL)
		clutter_actor_set_y (self->current_image, (float) VALUE_AT_PROGRESS (- stage_h, progress) + self->current_geometry.y);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_show (self->current_image);
		clutter_actor_show (self->next_image);
	}
}


static void
slide_from_right_transition (GthSlideshow *self,
			     double        progress)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	clutter_actor_set_x (self->next_image, (float) VALUE_AT_PROGRESS (stage_w, 1.0 - progress) + self->next_geometry.x);

	if (self->current_image != NULL)
		clutter_actor_set_opacity (self->current_image, (int) VALUE_AT_PROGRESS (255.0, 1.0 - progress));
	clutter_actor_set_opacity (self->next_image, (int) VALUE_AT_PROGRESS (255.0, progress));

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_show (self->current_image);
			clutter_actor_set_child_above_sibling (self->stage, self->next_image, self->current_image);
		}
		clutter_actor_show (self->next_image);
	}
}


static void
slide_from_bottom_transition (GthSlideshow *self,
			      double        progress)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	clutter_actor_set_y (self->next_image, (float) VALUE_AT_PROGRESS (stage_h, 1.0 - progress) + self->next_geometry.y);

	if (self->current_image != NULL)
		clutter_actor_set_opacity (self->current_image, (int) VALUE_AT_PROGRESS (255.0, 1.0 - progress));
	clutter_actor_set_opacity (self->next_image, (int) VALUE_AT_PROGRESS (255.0, progress));

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_show (self->current_image);
			clutter_actor_set_child_above_sibling (self->stage, self->next_image, self->current_image);
		}
		clutter_actor_show (self->next_image);
	}
}


static void
fade_transition (GthSlideshow *self,
		 double        progress)
{
	if (self->current_image != NULL)
		clutter_actor_set_opacity (self->current_image, (int) VALUE_AT_PROGRESS (255.0, 1.0 - progress));
	clutter_actor_set_opacity (self->next_image, (int) VALUE_AT_PROGRESS (255.0, progress));

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_show (self->current_image);
			clutter_actor_set_child_above_sibling (self->stage, self->next_image, self->current_image);
		}
		clutter_actor_show (self->next_image);
	}
}


static void
_clutter_actor_set_rotation (ClutterActor *self,
			     ClutterRotateAxis axis,
			     gdouble angle,
			     gfloat x,
			     gfloat y,
			     gfloat z)
{
	clutter_actor_set_pivot_point (self, x, y);
	clutter_actor_set_pivot_point_z (self, z);
	clutter_actor_set_rotation_angle (self, axis, angle);
}


static void
flip_transition (GthSlideshow *self,
		 double        progress)
{
	if (progress >= 0.5) {
		clutter_actor_show (self->next_image);
		if (self->current_image != NULL)
			clutter_actor_hide (self->current_image);
	}
	else {
		clutter_actor_hide (self->next_image);
		if (self->current_image != NULL)
			clutter_actor_show (self->current_image);
	}

	_clutter_actor_set_rotation (self->next_image,
				     CLUTTER_Y_AXIS,
				     VALUE_AT_PROGRESS (180.0, 1.0 - progress),
				     0.5,
				     0.5,
				     0.0);
	if (self->current_image != NULL)
		_clutter_actor_set_rotation (self->current_image,
					     CLUTTER_Y_AXIS,
					     VALUE_AT_PROGRESS (180.0, - progress),
					     0.5,
					     0.5,
					     0.0);

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_set_child_above_sibling (self->stage, self->next_image, self->current_image);
			clutter_actor_set_pivot_point (self->current_image, 0.5, 0.5);
		}
		clutter_actor_set_pivot_point (self->next_image, 0.5, 0.5);
	}
}


static void
cube_from_right_transition (GthSlideshow *self,
			    double        progress)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	if (self->current_image != NULL) {
		if (progress >= 0.5)
			clutter_actor_set_child_above_sibling (self->stage, self->next_image, self->current_image);
		else
			clutter_actor_set_child_above_sibling (self->stage, self->current_image, self->next_image);
	}

	_clutter_actor_set_rotation (self->next_image,
				     CLUTTER_Y_AXIS,
				     VALUE_AT_PROGRESS (90.0, - progress) - 270.0,
				     0.5,
				     0.5,
				     - stage_w / 2.0);
	if (self->current_image != NULL)
		_clutter_actor_set_rotation (self->current_image,
					     CLUTTER_Y_AXIS,
					     VALUE_AT_PROGRESS (90.0, - progress),
					     0.5,
					     0.5,
					     - stage_w / 2.0);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_set_pivot_point (self->current_image, 0.5, 0.5);
		clutter_actor_show (self->next_image);
		clutter_actor_set_pivot_point (self->next_image, 0.5, 0.5);
	}
}


static void
cube_from_bottom_transition (GthSlideshow *self,
			     double        progress)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	if (self->current_image != NULL) {
		if (progress >= 0.5)
			clutter_actor_set_child_above_sibling (self->stage, self->next_image, self->current_image);
		else
			clutter_actor_set_child_above_sibling (self->stage, self->current_image, self->next_image);
	}

	_clutter_actor_set_rotation (self->next_image,
				     CLUTTER_X_AXIS,
				     VALUE_AT_PROGRESS (90.0, progress) + 270.0,
				     0.5,
				     0.5,
				     - stage_w / 2.0);
	if (self->current_image != NULL)
		_clutter_actor_set_rotation (self->current_image,
					     CLUTTER_X_AXIS,
					     VALUE_AT_PROGRESS (90.0, progress),
					     0.5,
					     0.5,
					     - stage_w / 2.0);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_set_pivot_point (self->current_image, 0.5, 0.5);
		clutter_actor_show (self->next_image);
		clutter_actor_set_pivot_point (self->next_image, 0.5, 0.5);
	}
}


#endif /* HAVE_CLUTTER */


static GthShortcutCategory shortcut_categories[] = {
	{ GTH_SHORTCUT_CATEGORY_SLIDESHOW, N_("Presentation"), 40 },
};


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
#ifdef HAVE_CLUTTER
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "none",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("None"),
				  "frame-func", no_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "push-from-right",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Push from right"),
				  "frame-func", push_from_right_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "push-from-bottom",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Push from bottom"),
				  "frame-func", push_from_bottom_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "slide-from-right",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Slide from right"),
				  "frame-func", slide_from_right_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "slide-from-bottom",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Slide from bottom"),
				  "frame-func", slide_from_bottom_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "fade",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Fade in"),
				  "frame-func", fade_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "flip",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Flip page"),
				  "frame-func", flip_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "cube-from-right",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Cube from right"),
				  "frame-func", cube_from_right_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "cube-from-bottom",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Cube from bottom"),
				  "frame-func", cube_from_bottom_transition,
				  NULL);
#endif /* HAVE_CLUTTER */

	gth_main_register_shortcut_category (shortcut_categories, G_N_ELEMENTS (shortcut_categories));
	gth_hook_add_callback ("slideshow", 10, G_CALLBACK (ss__slideshow_cb), NULL);
	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (ss__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-sensitivity", 10, G_CALLBACK (ss__gth_browser_update_sensitivity_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 20, G_CALLBACK (ss__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("gth-catalog-read-metadata", 10, G_CALLBACK (ss__gth_catalog_read_metadata), NULL);
	gth_hook_add_callback ("gth-catalog-write-metadata", 10, G_CALLBACK (ss__gth_catalog_write_metadata), NULL);
	gth_hook_add_callback ("gth-catalog-read-from-doc", 10, G_CALLBACK (ss__gth_catalog_read_from_doc), NULL);
	gth_hook_add_callback ("gth-catalog-write-to-doc", 10, G_CALLBACK (ss__gth_catalog_write_to_doc), NULL);
	gth_hook_add_callback ("dlg-catalog-properties", 10, G_CALLBACK (ss__dlg_catalog_properties), NULL);
	gth_hook_add_callback ("dlg-catalog-properties-save", 10, G_CALLBACK (ss__dlg_catalog_properties_save), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
