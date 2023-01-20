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
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "actions.h"
#include "gth-image-viewer-page.h"
#include "preferences.h"
#include "shortcuts.h"


#define UPDATE_QUALITY_DELAY 200
#define UPDATE_VISIBILITY_DELAY 100
#define N_HEADER_BAR_BUTTONS 7
#define HIDE_OVERVIEW_TIMEOUT 2 /* in seconds */
#define OVERLAY_MARGIN 10
#define ZOOM_BUTTON 2
#define APPLY_ICC_PROFILE_BUTTON 3
#define TOGGLE_ANIMATION_BUTTON 4
#define STEP_ANIMATION_BUTTON 5
#define TRANSPARENCY_STYLE_BUTTON 6
#undef ALWAYS_LOAD_ORIGINAL_SIZE
#define N_FORWARD_PRELOADERS 2
#define N_BACKWARD_PRELOADERS 2


static void gth_viewer_page_interface_init (GthViewerPageInterface *iface);


static const GActionEntry actions[] = {
	{ "image-zoom-in", gth_browser_activate_image_zoom_in },
	{ "image-zoom-out", gth_browser_activate_image_zoom_out },
	{ "image-zoom-100", gth_browser_activate_image_zoom_100 },
	{ "image-zoom-200", gth_browser_activate_image_zoom_200 },
	{ "image-zoom-300", gth_browser_activate_image_zoom_300 },
	{ "image-zoom-fit", gth_browser_activate_image_zoom_fit },
	{ "image-zoom-fit-if-larger", gth_browser_activate_image_zoom_fit_if_larger },
	{ "image-zoom-fit-width", gth_browser_activate_image_zoom_fit_width },
	{ "image-zoom-fit-width-if-larger", gth_browser_activate_image_zoom_fit_width_if_larger },
	{ "image-zoom-fit-height", gth_browser_activate_image_zoom_fit_height },
	{ "image-zoom-fit-height-if-larger", gth_browser_activate_image_zoom_fit_height_if_larger },
	{ "image-undo", gth_browser_activate_image_undo },
	{ "image-redo", gth_browser_activate_image_redo },
	{ "copy-image", gth_browser_activate_copy_image },
	{ "paste-image", gth_browser_activate_paste_image },
	{ "apply-icc-profile", toggle_action_activated, NULL, "true", gth_browser_activate_apply_icc_profile },
	{ "toggle-animation", toggle_action_activated, NULL, "true", gth_browser_activate_toggle_animation },
	{ "step-animation", gth_browser_activate_step_animation },
	{ "image-zoom", gth_browser_activate_image_zoom, "s", "''", NULL },
	{ "transparency-style", gth_browser_activate_transparency_style, "s", "''", NULL },
	{ "scroll-step-left", gth_browser_activate_scroll_step_left },
	{ "scroll-step-right", gth_browser_activate_scroll_step_right },
	{ "scroll-step-up", gth_browser_activate_scroll_step_up },
	{ "scroll-step-down", gth_browser_activate_scroll_step_down },
	{ "scroll-page-left", gth_browser_activate_scroll_page_left },
	{ "scroll-page-right", gth_browser_activate_scroll_page_right },
	{ "scroll-page-up", gth_browser_activate_scroll_page_up },
	{ "scroll-page-down", gth_browser_activate_scroll_page_down },
	{ "scroll-to-center", gth_browser_activate_scroll_to_center },
};


static const GthMenuEntry file_popup_entries[] = {
	{ N_("Copy Image"), "win.copy-image" },
	{ N_("Paste Image"), "win.paste-image" },
};


struct _GthImageViewerPagePrivate {
	GthBrowser        *browser;
	GSettings         *settings;
	GtkWidget         *image_navigator;
	GtkWidget         *overview_revealer;
	GtkWidget         *overview;
	GtkWidget         *viewer;
	GthImagePreloader *preloader;
	guint              file_popup_merge_id;
	GthImageHistory   *history;
	GthFileData       *file_data;
	GFileInfo         *updated_info;
	gboolean           active;
	gboolean           image_changed;
	gboolean           loading_image;
	GFile             *last_loaded;
	gboolean           can_paste;
	guint              update_quality_id;
	guint		   update_visibility_id;
	GtkWidget         *buttons[N_HEADER_BAR_BUTTONS];
	GtkBuilder        *builder;
	gboolean           pointer_on_viewer;
	gboolean           pointer_on_overview;
	guint              hide_overview_id;
	gboolean           apply_icc_profile;
	GthFileData       *next_file_data[N_FORWARD_PRELOADERS];
	GthFileData       *prev_file_data[N_BACKWARD_PRELOADERS];
	gulong             drag_data_get_event;
};


G_DEFINE_TYPE_WITH_CODE (GthImageViewerPage,
			 gth_image_viewer_page,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImageViewerPage)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_VIEWER_PAGE,
						gth_viewer_page_interface_init))


static void
gth_image_viewer_page_file_loaded (GthImageViewerPage *self,
				   gboolean            success)
{
	if (_g_file_equal (self->priv->last_loaded, self->priv->file_data->file))
		return;

	_g_object_unref (self->priv->last_loaded);
	self->priv->last_loaded = g_object_ref (self->priv->file_data->file);

	gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self),
				     self->priv->file_data,
				     self->priv->updated_info,
				     success);
}


static int
get_viewer_size (GthImageViewerPage *self)
{
	GtkAllocation allocation;
	int           size;

	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	size = MAX (allocation.width, allocation.height);
	if (size <= 1) {
		int window_width;
		int window_height;
		gtk_window_get_size (GTK_WINDOW (self->priv->browser),
				     &window_width,
				     &window_height);
		size = MAX (window_width, window_height);;
	}

	return size;
}


static int
_gth_image_preloader_get_requested_size_for_next_images (GthImageViewerPage *self)
{
	int requested_size;

	requested_size = -1;

	switch (gth_image_viewer_get_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer))) {
	case GTH_ZOOM_CHANGE_ACTUAL_SIZE:
		requested_size = -1;
		break;
	default:
		requested_size = (int) floor ((double) get_viewer_size (self) * 0.5);
		break;
	}

	return requested_size * gtk_widget_get_scale_factor (GTK_WIDGET (self->priv->viewer));
}


static int
_gth_image_preloader_get_requested_size_for_current_image (GthImageViewerPage *self)
{
	int	requested_size;
	double	zoom;

	requested_size = -1;

	switch (gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer))) {
	case GTH_FIT_NONE:
		zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));
		if (zoom < 1.0) {
			int original_width;
			int original_height;

			gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer), &original_width, &original_height);
			requested_size = MAX (original_width * zoom, original_height * zoom);
		}
		else
			requested_size = -1;
		break;

	default:
		requested_size = get_viewer_size (self);
		break;
	}

	return requested_size * gtk_widget_get_scale_factor (GTK_WIDGET (self->priv->viewer));
}


/* -- _gth_image_viewer_page_load_with_preloader -- */


typedef struct {
	GthImageViewerPage  *self;
	GthFileData         *file_data;
	int                  requested_size;
	GCancellable        *cancellable;
	GAsyncReadyCallback  callback;
	gpointer	     user_data;
} ProfileData;


static void
profile_data_free (ProfileData *profile_data)
{
	_g_object_unref (profile_data->cancellable);
	_g_object_unref (profile_data->file_data);
	_g_object_unref (profile_data->self);
	g_free (profile_data);
}


static void
_gth_image_viewer_page_load_with_preloader_step2 (GthImageViewerPage  *self,
						  GthFileData         *file_data,
						  int                  requested_size,
						  GCancellable        *cancellable,
						  GAsyncReadyCallback  callback,
						  gpointer	       user_data)
{
	g_object_ref (self);

	gth_image_preloader_load (self->priv->preloader,
				  file_data,
				  requested_size,
				  cancellable,
				  callback,
				  user_data,
				  N_FORWARD_PRELOADERS + N_BACKWARD_PRELOADERS,
				  self->priv->next_file_data[0],
				  self->priv->next_file_data[1],
				  self->priv->prev_file_data[0],
				  self->priv->prev_file_data[1]);
}


static void
profile_ready_cb (GObject      *source_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
	ProfileData        *profile_data = user_data;
	GthImageViewerPage *self = profile_data->self;

	if (self->priv->active && ! self->priv->image_changed && _g_file_equal (self->priv->file_data->file, profile_data->file_data->file)) {
		GthICCProfile *profile;

		profile = gth_color_manager_get_profile_finish (GTH_COLOR_MANAGER (source_object), res, NULL);
		if (profile == NULL)
			profile = _g_object_ref (gth_browser_get_monitor_profile (self->priv->browser));
		gth_image_preloader_set_out_profile (self->priv->preloader, profile);

		_gth_image_viewer_page_load_with_preloader_step2 (profile_data->self,
								  profile_data->file_data,
								  profile_data->requested_size,
								  profile_data->cancellable,
								  profile_data->callback,
								  profile_data->user_data);

		_g_object_unref (profile);
	}

	profile_data_free (profile_data);
}


static void
_gth_image_viewer_page_load_with_preloader (GthImageViewerPage  *self,
					    GthFileData         *file_data,
					    int                  requested_size,
					    GCancellable        *cancellable,
					    GAsyncReadyCallback  callback,
					    gpointer		 user_data)
{
	if ((file_data != NULL) && self->priv->apply_icc_profile) {
		char *monitor_name = NULL;

		if (_gtk_window_get_monitor_info (GTK_WINDOW (self->priv->browser), NULL, NULL, &monitor_name)) {
			ProfileData *profile_data;

			profile_data = g_new (ProfileData, 1);
			profile_data->self = g_object_ref (self);
			profile_data->file_data = g_object_ref (file_data);
			profile_data->requested_size = requested_size;
			profile_data->cancellable = _g_object_ref (cancellable);
			profile_data->callback = callback;
			profile_data->user_data = user_data;

			gth_color_manager_get_profile_async (gth_main_get_default_color_manager(),
							     monitor_name,
							     cancellable,
							     profile_ready_cb,
							     profile_data);

			return;
		}
	}

	gth_image_preloader_set_out_profile (self->priv->preloader, NULL);
	_gth_image_viewer_page_load_with_preloader_step2 (self, file_data, requested_size, cancellable, callback, user_data);
}


static gboolean
_gth_image_viewer_page_load_with_preloader_finish (GthImageViewerPage  *self)
{
	gboolean active = self->priv->active;
	g_object_unref (self);

	return active;
}


static void
different_quality_ready_cb (GObject		*source_object,
			    GAsyncResult	*result,
			    gpointer	 	 user_data)
{
	GthImageViewerPage *self = user_data;
	GthFileData	   *requested;
	GthImage	   *image;
	int		    requested_size;
	int		    original_width;
	int		    original_height;
	GError		   *error = NULL;
	cairo_surface_t    *s1 = NULL;
	cairo_surface_t    *s2;
	int                 w1, h1, w2, h2;
	gboolean            got_better_quality;

	if (! _gth_image_viewer_page_load_with_preloader_finish (self))
		return;

	if (! gth_image_preloader_load_finish (GTH_IMAGE_PRELOADER (source_object),
					       result,
					       &requested,
					       &image,
					       &requested_size,
					       &original_width,
					       &original_height,
					       &error))
	{
		g_clear_error (&error);
		return;
	}

	if (! (self->priv->image_changed && requested == NULL) && ! _g_file_equal (requested->file, self->priv->file_data->file))
		goto clear_data;

	if (image == NULL)
		goto clear_data;

	/* check whether the image is of different quality */

	s1 = gth_image_get_cairo_surface (image);
	if (s1 == NULL)
		goto clear_data;

	s2 = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	if (s2 == NULL) {
		got_better_quality = TRUE;
	}
	else {
		w1 = cairo_image_surface_get_width (s1);
		h1 = cairo_image_surface_get_height (s1);
		w2 = cairo_image_surface_get_width (s2);
		h2 = cairo_image_surface_get_height (s2);
		got_better_quality = ((w1 > w2) || (h1 > h2));
	}

	if (got_better_quality) {
		gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
		gth_image_viewer_set_better_quality (GTH_IMAGE_VIEWER (self->priv->viewer),
						     image,
						     original_width,
						     original_height);
		gth_image_viewer_set_requested_size (GTH_IMAGE_VIEWER (self->priv->viewer), requested_size);
		gtk_widget_queue_draw (self->priv->viewer);
	}

clear_data:

	if (s1 != NULL)
		cairo_surface_destroy (s1);
	_g_object_unref (requested);
	_g_object_unref (image);
	g_clear_error (&error);

	return;
}


static gboolean
_g_mime_type_can_load_different_quality (const char *mime_type)
{
	static const char *supported[] = {
		"image/jpeg",
		"image/x-portable-pixmap"
	};

	int i;
	for (i = 0; i < G_N_ELEMENTS (supported); i++)
		if (g_strcmp0 (mime_type, supported[i]) == 0)
			return TRUE;

	if (_g_mime_type_is_raw (mime_type))
		return TRUE;

	return FALSE;
}


typedef struct {
	GthImageViewerPage *self;
	GthFileData *file_data;
} UpdateQualityData;


static void
update_quality_data_free (UpdateQualityData *data)
{
	_g_object_unref (data->file_data);
	g_free (data);
}


static gboolean
update_quality_cb (gpointer user_data)
{
	UpdateQualityData  *data = user_data;
	GthImageViewerPage *self = data->self;
	gboolean            file_changed;

	if (! _gth_image_viewer_page_load_with_preloader_finish (self)) {
		update_quality_data_free (data);
		return FALSE;
	}

	if (self->priv->update_quality_id != 0) {
		g_source_remove (self->priv->update_quality_id);
		self->priv->update_quality_id = 0;
	}

	file_changed = ! _g_file_equal (data->file_data->file, self->priv->file_data->file);
	update_quality_data_free (data);

	if (file_changed)
		return FALSE;

	if (! self->priv->active)
		return FALSE;

	if (self->priv->viewer == NULL)
		return FALSE;

	if (self->priv->loading_image)
		return FALSE;

	if (! self->priv->image_changed && ! _g_mime_type_can_load_different_quality (gth_file_data_get_mime_type (self->priv->file_data)))
		return FALSE;

	_gth_image_viewer_page_load_with_preloader (self,
						    self->priv->image_changed ? GTH_MODIFIED_IMAGE : self->priv->file_data,
						    _gth_image_preloader_get_requested_size_for_current_image (self),
						    NULL,
						    different_quality_ready_cb,
						    self);

	return FALSE;
}


static void
update_image_quality_if_required (GthImageViewerPage *self)
{
#ifndef ALWAYS_LOAD_ORIGINAL_SIZE
	GthImage *image;

	if (self->priv->loading_image || gth_sidebar_tool_is_active (GTH_SIDEBAR (gth_browser_get_viewer_sidebar (self->priv->browser))))
		return;

	image = gth_image_viewer_get_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	if ((image != NULL) && (gth_image_get_is_zoomable (image) || gth_image_get_is_animation (image)))
		return;

	if (self->priv->update_quality_id != 0) {
		g_source_remove (self->priv->update_quality_id);
		self->priv->update_quality_id = 0;
	}

	UpdateQualityData *data;

	data = g_new0 (UpdateQualityData, 1);
	data->self = self;
	data->file_data = _g_object_ref (self->priv->file_data);

	_g_object_ref (self);
	self->priv->update_quality_id = g_timeout_add (UPDATE_QUALITY_DELAY,
						       update_quality_cb,
						       data);
#endif
}


static gboolean
hide_overview_after_timeout (gpointer data)
{
	GthImageViewerPage *self = data;

	if (self->priv->hide_overview_id != 0)
		g_source_remove (self->priv->hide_overview_id);
	self->priv->hide_overview_id = 0;

	if (! self->priv->pointer_on_overview)
		gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->overview_revealer), FALSE);

	return FALSE;
}


static gboolean
update_overview_visibility_now (gpointer user_data)
{
	GthImageViewerPage *self;
	gboolean            overview_visible;

	self = GTH_IMAGE_VIEWER_PAGE (user_data);

	if (self->priv->update_visibility_id != 0) {
		g_source_remove (self->priv->update_visibility_id);
		self->priv->update_visibility_id = 0;
	}

	if (! self->priv->active)
		return FALSE;

	overview_visible = self->priv->pointer_on_overview || (self->priv->pointer_on_viewer && gth_image_viewer_has_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer)));
	gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->overview_revealer), overview_visible);

	if (overview_visible) {
		if (self->priv->hide_overview_id != 0)
			g_source_remove (self->priv->hide_overview_id);
		self->priv->hide_overview_id = g_timeout_add_seconds (HIDE_OVERVIEW_TIMEOUT, hide_overview_after_timeout, self);
	}

	return FALSE;
}


static void
update_overview_visibility (GthImageViewerPage *self)
{
	if (self->priv->update_visibility_id != 0) {
		g_source_remove (self->priv->update_visibility_id);
		self->priv->update_visibility_id = 0;
	}

	self->priv->update_visibility_id = g_timeout_add (UPDATE_VISIBILITY_DELAY,
							  update_overview_visibility_now,
							  self);
}


#define MIN_ZOOM_LEVEL 0.3
#define MAX_ZOOM_LEVEL 3.0


static void
zoom_scale_value_changed_cb (GtkScale *scale,
			     gpointer  user_data)
{
	GthImageViewerPage *self = user_data;
	double              x, zoom;

	x = gtk_range_get_value (GTK_RANGE (scale));
	zoom = MIN_ZOOM_LEVEL + (x / 100.0 * (MAX_ZOOM_LEVEL - MIN_ZOOM_LEVEL));
	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (self->priv->viewer), zoom);
}


static void
zoom_button_toggled_cb (GtkToggleButton *togglebutton,
	                gpointer         user_data)
{
	GthImageViewerPage *self = user_data;

	if (! gtk_toggle_button_get_active (togglebutton))
		return;

	gth_browser_keep_mouse_visible (self->priv->browser, TRUE);
}


static void
zoom_popover_closed_cb (GtkPopover *popover,
			gpointer    user_data)
{
	GthImageViewerPage *self = user_data;

	gth_browser_keep_mouse_visible (self->priv->browser, FALSE);
	call_when_idle ((DataFunc) gth_viewer_page_focus, self);
}


#define ZOOM_EQUAL(a,b) (fabs (a - b) < 1e-3)


static void
update_zoom_info (GthImageViewerPage *self)
{
	double  zoom;
	char   *text;
	double  x;

	/* status bar */

	zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));
	text = g_strdup_printf ("  %d%%  ", (int) (zoom * 100));
	gth_statusbar_set_secondary_text (GTH_STATUSBAR (gth_browser_get_statusbar (self->priv->browser)), text);
	g_free (text);

	/* zoom menu */

	gboolean    zoom_enabled;
	GthFit      fit_mode;
	GAction    *action;
	const char *state;

	zoom_enabled = gth_image_viewer_get_zoom_enabled (GTH_IMAGE_VIEWER (self->priv->viewer));
	fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer));

	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "image-zoom", zoom_enabled);

	state = "";
	if (fit_mode == GTH_FIT_SIZE)
		state = "fit";
	else if (fit_mode == GTH_FIT_WIDTH)
		state = "fit-width";
	else if (fit_mode == GTH_FIT_HEIGHT)
		state = "fit-height";
	else if (fit_mode == GTH_FIT_SIZE_IF_LARGER)
		state = "automatic";
	else if (ZOOM_EQUAL (zoom, 0.5))
		state = "50";
	else if (ZOOM_EQUAL (zoom, 1.0))
		state = "100";
	else if (ZOOM_EQUAL (zoom, 2.0))
		state = "200";
	else if (ZOOM_EQUAL (zoom, 3.0))
		state = "300";

	action = g_action_map_lookup_action (G_ACTION_MAP (self->priv->browser), "image-zoom");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_string (state));

	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "image-zoom-100", ! ZOOM_EQUAL (zoom, 1.0));
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "image-zoom-fit-if-larger", fit_mode != GTH_FIT_SIZE_IF_LARGER);

	/* zoom menu scale */

	GtkWidget *scale = _gtk_builder_get_widget (self->priv->builder, "zoom_level_scale");

	_g_signal_handlers_block_by_data (scale, self);
	x = (zoom - MIN_ZOOM_LEVEL) / (MAX_ZOOM_LEVEL - MIN_ZOOM_LEVEL) * 100.0;
	gtk_range_set_value (GTK_RANGE (scale), CLAMP (x, 0, 100));
	_g_signal_handlers_unblock_by_data (scale, self);
}


static void
viewer_zoom_changed_cb (GtkWidget          *widget,
			GthImageViewerPage *self)
{
	update_image_quality_if_required (self);
	self->priv->pointer_on_viewer = TRUE;
	update_overview_visibility (self);
	update_zoom_info (self);
}


static void
viewer_image_changed_cb (GtkWidget          *widget,
			 GthImageViewerPage *self)
{
	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
	update_image_quality_if_required (self);
	update_overview_visibility (self);
	update_zoom_info (self);
}


static gboolean
viewer_button_press_event_cb (GtkWidget          *widget,
			      GdkEventButton     *event,
			      GthImageViewerPage *self)
{
	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static gboolean
viewer_popup_menu_cb (GtkWidget          *widget,
		      GthImageViewerPage *self)
{
	gth_browser_file_menu_popup (self->priv->browser, NULL);
	return TRUE;
}


static gboolean
viewer_scroll_event_cb (GtkWidget 	   *widget,
		        GdkEventScroll     *event,
		        GthImageViewerPage *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
viewer_image_map_event_cb (GtkWidget          *widget,
			   GdkEvent           *event,
			   GthImageViewerPage *self)
{
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
	return FALSE;
}


static void
clipboard_targets_received_cb (GtkClipboard *clipboard,
			       GdkAtom      *atoms,
                               int           n_atoms,
                               gpointer      user_data)
{
	GthImageViewerPage *self = user_data;
	int                 i;

	self->priv->can_paste = FALSE;
	for (i = 0; ! self->priv->can_paste && (i < n_atoms); i++)
		if (atoms[i] == gdk_atom_intern_static_string ("image/png"))
			self->priv->can_paste = TRUE;

	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "paste-image", self->priv->can_paste);

	g_object_unref (self);
}


static void
_gth_image_viewer_page_update_paste_command_sensitivity (GthImageViewerPage *self,
							 GtkClipboard       *clipboard)
{
	self->priv->can_paste = FALSE;
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "paste-image", self->priv->can_paste);

	if (clipboard == NULL)
		clipboard = gtk_widget_get_clipboard (GTK_WIDGET (self->priv->viewer), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_targets (clipboard,
				       clipboard_targets_received_cb,
				       g_object_ref (self));
}


static void
clipboard_owner_change_cb (GtkClipboard *clipboard,
                           GdkEvent     *event,
                           gpointer      user_data)
{
	_gth_image_viewer_page_update_paste_command_sensitivity ((GthImageViewerPage *) user_data, clipboard);
}


static void
viewer_realize_cb (GtkWidget *widget,
                   gpointer   user_data)
{
	GthImageViewerPage *self = user_data;
	GtkClipboard       *clipboard;

	clipboard = gtk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
	g_signal_connect (clipboard,
	                  "owner_change",
	                  G_CALLBACK (clipboard_owner_change_cb),
	                  self);
}


static void
viewer_unrealize_cb (GtkWidget *widget,
		     gpointer   user_data)
{
	GthImageViewerPage *self = user_data;
	GtkClipboard       *clipboard;

	clipboard = gtk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
	g_signal_handlers_disconnect_by_func (clipboard,
	                                      G_CALLBACK (clipboard_owner_change_cb),
	                                      self);
}


static gboolean
image_navigator_get_child_position_cb	(GtkOverlay   *overlay,
					 GtkWidget    *widget,
					 GdkRectangle *allocation,
					 gpointer      user_data)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (user_data);
	GtkAllocation       main_alloc;
	gboolean            allocation_filled = FALSE;

	gtk_widget_get_allocation (gtk_bin_get_child (GTK_BIN (overlay)), &main_alloc);
	gtk_widget_get_preferred_width (widget, NULL, &allocation->width);
	gtk_widget_get_preferred_height (widget, NULL, &allocation->height);

	if (widget == self->priv->overview_revealer) {
		allocation->x = main_alloc.width - allocation->width - OVERLAY_MARGIN;
		allocation->y = OVERLAY_MARGIN;
		if (gth_browser_get_is_fullscreen (self->priv->browser))
			allocation->y += gtk_widget_get_allocated_height (gth_browser_get_fullscreen_headerbar (self->priv->browser));
		allocation_filled = TRUE;
	}

	return allocation_filled;
}


static gboolean
overview_motion_notify_event_cb (GtkWidget      *widget,
				 GdkEventMotion *event,
				 gpointer        data)
{
	GthImageViewerPage *self = data;

	if (self->priv->hide_overview_id != 0) {
		g_source_remove (self->priv->hide_overview_id);
		self->priv->hide_overview_id = 0;
	}

	self->priv->pointer_on_viewer = TRUE;
	if (widget == self->priv->overview)
		self->priv->pointer_on_overview = TRUE;
	update_overview_visibility (data);

	return FALSE;
}


static gboolean
overview_leave_notify_event_cb (GtkWidget *widget,
				GdkEvent  *event,
				gpointer   data)
{
	GthImageViewerPage *self = data;

	if (widget == self->priv->overview)
		self->priv->pointer_on_overview = gth_image_overview_get_scrolling_is_active (GTH_IMAGE_OVERVIEW (self->priv->overview));

	return FALSE;
}


static void
pref_zoom_quality_changed (GSettings *settings,
			   char      *key,
			   gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	if (! self->priv->active || (self->priv->viewer == NULL))
		return;

	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer),
					   g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_zoom_change_changed (GSettings *settings,
		   	  char      *key,
		   	  gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	if (! self->priv->active || (self->priv->viewer == NULL))
		return;

	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer),
					  g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_reset_scrollbars_changed (GSettings *settings,
		   	       char      *key,
		   	       gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	if (! self->priv->active || (self->priv->viewer == NULL))
		return;

	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer),
					       g_settings_get_boolean (self->priv->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS));
}


static void
pref_transparency_style_changed (GSettings *settings,
				 char      *key,
				 gpointer   user_data)
{
	GthImageViewerPage   *self = user_data;
	GthTransparencyStyle  style;
	GAction              *action;
	const char           *state;

	if (! self->priv->active || (self->priv->viewer == NULL))
		return;

	style = g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_TRANSPARENCY_STYLE);
	state = "";
	switch (style) {
	case GTH_TRANSPARENCY_STYLE_CHECKERED:
		state = "checkered";
		break;
	case GTH_TRANSPARENCY_STYLE_WHITE:
		state = "white";
		break;
	case GTH_TRANSPARENCY_STYLE_GRAY:
		state = "gray";
		break;
	case GTH_TRANSPARENCY_STYLE_BLACK:
		state = "black";
		break;
	}
	action = g_action_map_lookup_action (G_ACTION_MAP (self->priv->browser), "transparency-style");
	if (action != NULL)
		g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_string (state));

	gth_image_viewer_set_transparency_style (GTH_IMAGE_VIEWER (self->priv->viewer), style);
}


static void
paint_comment_over_image_func (GthImageViewer *image_viewer,
			       cairo_t        *cr,
			       gpointer        user_data)
{
	GthImageViewerPage *self = user_data;
	GthFileData        *file_data = self->priv->file_data;
	GString            *file_info;
	char               *comment;
	const char         *file_date;
	const char         *file_size;
	int                 current_position;
	int                 n_visibles;
	int                 width;
	int                 height;
	GthMetadata        *metadata;
	PangoLayout        *layout;
	PangoAttrList      *attr_list = NULL;
	GError             *error = NULL;
	char               *text;
	static GdkPixbuf   *icon = NULL;
	int                 icon_width;
	int                 icon_height;
	int                 image_width;
	int                 image_height;
	const int           x_padding = 20;
	const int           y_padding = 20;
	int                 max_text_width;
	PangoRectangle      bounds;
	int                 text_x;
	int                 text_y;
	int                 icon_x;
	int                 icon_y;

	file_info = g_string_new ("");

	comment = gth_file_data_get_attribute_as_string (file_data, "general::description");
	if (comment != NULL) {
		g_string_append_printf (file_info, "<b>%s</b>\n\n", comment);
		g_free (comment);
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL)
		file_date = gth_metadata_get_formatted (metadata);
	else
		file_date = g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime");
	file_size = g_file_info_get_attribute_string (file_data->info, "gth::file::display-size");

	gth_browser_get_file_list_info (self->priv->browser, &current_position, &n_visibles);
	gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer), &width, &height);

	g_string_append_printf (file_info,
			        "<small><i>%s - %dx%d (%d%%) - %s</i>\n<tt>%d/%d - %s</tt></small>",
			        file_date,
			        width,
			        height,
			        (int) (gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer)) * 100),
			        file_size,
			        current_position + 1,
			        n_visibles,
				g_file_info_get_attribute_string (file_data->info, "standard::display-name"));

	layout = gtk_widget_create_pango_layout (GTK_WIDGET (self->priv->viewer), NULL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

	if (! pango_parse_markup (file_info->str,
			          -1,
				  0,
				  &attr_list,
				  &text,
				  NULL,
				  &error))
	{
		g_warning ("Failed to set text from markup due to error parsing markup: %s\nThis is the text that caused the error: %s",  error->message, file_info->str);
		g_error_free (error);
		g_object_unref (layout);
		g_string_free (file_info, TRUE);
		return;
	}

	pango_layout_set_attributes (layout, attr_list);
        pango_layout_set_text (layout, text, strlen (text));

        if (icon == NULL) {
        	GIcon *gicon;

        	gicon = g_themed_icon_new ("dialog-information-symbolic");
        	icon = _g_icon_get_pixbuf (gicon, 24, _gtk_widget_get_icon_theme (GTK_WIDGET (image_viewer)));

        	g_object_unref (gicon);
        }
	icon_width = gdk_pixbuf_get_width (icon);
	icon_height = gdk_pixbuf_get_height (icon);

	image_width = gdk_window_get_width (gtk_widget_get_window (self->priv->viewer));
	image_height = gdk_window_get_height (gtk_widget_get_window (self->priv->viewer));
	max_text_width = ((image_width * 3 / 4) - icon_width - (x_padding * 3) - (x_padding * 2));

	pango_layout_set_width (layout, max_text_width * PANGO_SCALE);
	pango_layout_get_pixel_extents (layout, NULL, &bounds);

	bounds.width += (2 * x_padding) + (icon_width + x_padding);
	bounds.height = MIN (image_height - icon_height - (y_padding * 2), bounds.height + (2 * y_padding));
	bounds.x = MAX ((image_width - bounds.width) / 2, 0);
	bounds.y = MAX (image_height - bounds.height - (y_padding * 3), 0);

	text_x = bounds.x + x_padding + icon_width + x_padding;
	text_y = bounds.y + y_padding;
	icon_x = bounds.x + x_padding;
	icon_y = bounds.y + (bounds.height - icon_height) / 2;

	cairo_save (cr);

	/* background */

	_cairo_draw_rounded_box (cr, bounds.x, bounds.y, bounds.width, bounds.height, 8.0);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.80);
	cairo_fill (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke (cr);

	/* icon */

	gdk_cairo_set_source_pixbuf (cr, icon, icon_x, icon_y);
	cairo_rectangle (cr, icon_x, icon_y, icon_width, icon_height);
	cairo_fill (cr);

	/* text */

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	pango_cairo_update_layout (cr, layout);
	cairo_move_to (cr, text_x, text_y);
	pango_cairo_show_layout (cr, layout);

	cairo_restore (cr);

        g_free (text);
        pango_attr_list_unref (attr_list);
	g_object_unref (layout);
	g_string_free (file_info, TRUE);
}


static void
gth_image_viewer_page_real_activate (GthViewerPage *base,
				     GthBrowser    *browser)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;

	self->priv->browser = browser;
	self->priv->active = TRUE;

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	self->priv->buttons[0] =
			gth_browser_add_header_bar_button (browser,
							   GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM,
							   "view-zoom-original-symbolic",
							   _("Set to actual size"),
							   "win.image-zoom-100",
							   NULL);
	self->priv->buttons[1] =
			gth_browser_add_header_bar_button (browser,
							   GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM,
							   "view-zoom-fit-symbolic",
							   _("Fit to window if larger"),
							   "win.image-zoom-fit-if-larger",
							   NULL);

	self->priv->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/image_viewer/data/ui/toolbar-zoom-menu.ui");
	self->priv->buttons[ZOOM_BUTTON] =
			gth_browser_add_header_bar_menu_button (browser,
								GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM,
								"view-zoom-in-symbolic",
								NULL,
								_gtk_builder_get_widget (self->priv->builder, "zoom_popover"));
	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "zoom_level_scale"),
			  "value-changed",
			  G_CALLBACK (zoom_scale_value_changed_cb),
			  self);
	g_signal_connect (self->priv->buttons[ZOOM_BUTTON],
			  "toggled",
			  G_CALLBACK (zoom_button_toggled_cb),
			  self);
	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "zoom_popover"),
			  "closed",
			  G_CALLBACK (zoom_popover_closed_cb),
			  self);

	self->priv->buttons[APPLY_ICC_PROFILE_BUTTON] =
			gth_browser_add_header_bar_toggle_button (browser,
							   	  GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS,
								  "color-profile",
								  _("Apply the embedded color profile"),
								  "win.apply-icc-profile",
								  NULL);

	self->priv->buttons[TOGGLE_ANIMATION_BUTTON] =
			gth_browser_add_header_bar_toggle_button (browser,
							   	  GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
								  "media-playback-start-symbolic",
								  _("Play"),
								  "win.toggle-animation",
								  NULL);
	self->priv->buttons[STEP_ANIMATION_BUTTON] =
			gth_browser_add_header_bar_button (browser,
							   GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
							   "media-skip-forward-symbolic",
							   _("Next frame"),
							   "win.step-animation",
							   NULL);
	self->priv->buttons[TRANSPARENCY_STYLE_BUTTON] =
			gth_browser_add_header_bar_menu_button (browser,
								GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS,
								"transparency-symbolic",
								_("Transparency"),
								_gtk_builder_get_widget (self->priv->builder, "transparency_popover"));

	self->priv->preloader = gth_browser_get_image_preloader (browser);

	self->priv->viewer = gth_image_viewer_new ();
	g_object_add_weak_pointer (G_OBJECT (self->priv->viewer), (gpointer *) &self->priv->viewer);
	gtk_widget_add_events (self->priv->viewer, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
	gth_image_viewer_page_reset_viewer_tool (self);
	gtk_widget_show (self->priv->viewer);

	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "zoom-changed",
			  G_CALLBACK (viewer_zoom_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "image-changed",
			  G_CALLBACK (viewer_image_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "popup-menu",
			  G_CALLBACK (viewer_popup_menu_cb),
			  self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"button_press_event",
				G_CALLBACK (viewer_button_press_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"scroll_event",
				G_CALLBACK (viewer_scroll_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"map_event",
				G_CALLBACK (viewer_image_map_event_cb),
				self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "realize",
			  G_CALLBACK (viewer_realize_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "unrealize",
			  G_CALLBACK (viewer_unrealize_cb),
			  self);

	self->priv->image_navigator = gtk_overlay_new ();
	g_signal_connect (self->priv->image_navigator,
			  "get-child-position",
			  G_CALLBACK (image_navigator_get_child_position_cb),
			  self);
	gtk_container_add (GTK_CONTAINER (self->priv->image_navigator), self->priv->viewer);
	gtk_widget_show (self->priv->image_navigator);

	self->priv->overview_revealer = gtk_revealer_new ();
	gtk_revealer_set_transition_duration (GTK_REVEALER (self->priv->overview_revealer), 200);
	gtk_revealer_set_transition_type (GTK_REVEALER (self->priv->overview_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
	gtk_widget_show (self->priv->overview_revealer);
	gtk_overlay_add_overlay (GTK_OVERLAY (self->priv->image_navigator), self->priv->overview_revealer);

	self->priv->overview = gth_image_overview_new (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_widget_add_events (self->priv->overview, GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK);
	gtk_widget_show (self->priv->overview);
	gtk_container_add (GTK_CONTAINER (self->priv->overview_revealer), self->priv->overview);

	g_signal_connect_after (G_OBJECT (self->priv->overview),
				"motion-notify-event",
				G_CALLBACK (overview_motion_notify_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->overview),
				"leave-notify-event",
				G_CALLBACK (overview_leave_notify_event_cb),
				self);

	gth_browser_set_viewer_widget (browser, self->priv->image_navigator);
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
}


static void
gth_image_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthImageViewerPage *self;
	int                 i;

	self = (GthImageViewerPage*) base;

	for (i = 0; i < N_HEADER_BAR_BUTTONS; i++) {
		if (self->priv->buttons[i] != NULL) {
			gtk_widget_destroy (self->priv->buttons[i]);
			self->priv->buttons[i] = NULL;
		}
	}

	_g_object_unref (self->priv->builder);
	self->priv->builder = NULL;
	_g_object_unref (self->priv->preloader);
	self->priv->preloader = NULL;
	self->priv->active = FALSE;

	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static void
gth_image_viewer_page_real_show (GthViewerPage *base)
{
	GthImageViewerPage *self = (GthImageViewerPage*) base;

	if (self->priv->file_popup_merge_id == 0)
		self->priv->file_popup_merge_id =
				gth_menu_manager_append_entries (gth_browser_get_menu_manager (self->priv->browser, GTH_BROWSER_MENU_MANAGER_FILE_EDIT_ACTIONS),
								 file_popup_entries,
								 G_N_ELEMENTS (file_popup_entries));
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
}


static void
gth_image_viewer_page_real_hide (GthViewerPage *base)
{
	GthImageViewerPage *self = (GthImageViewerPage*) base;

	gth_menu_manager_remove_entries (gth_browser_get_menu_manager (self->priv->browser, GTH_BROWSER_MENU_MANAGER_FILE_EDIT_ACTIONS), self->priv->file_popup_merge_id);
	self->priv->file_popup_merge_id = 0;
}


static gboolean
gth_image_viewer_page_real_can_view (GthViewerPage *base,
				     GthFileData   *file_data)
{
	g_return_val_if_fail (file_data != NULL, FALSE);
	return _g_mime_type_is_image (gth_file_data_get_mime_type (file_data));
}


static void
preloader_load_ready_cb (GObject	*source_object,
			 GAsyncResult	*result,
			 gpointer	 user_data)
{
	GthImageViewerPage *self = user_data;
	GthFileData	   *requested;
	GthImage	   *image;
	int		    requested_size;
	int		    original_width;
	int		    original_height;
	GError		   *error = NULL;

	self->priv->loading_image = FALSE;
	if (! _gth_image_viewer_page_load_with_preloader_finish (self))
		return;

	if (! gth_image_preloader_load_finish (GTH_IMAGE_PRELOADER (source_object),
					       result,
					       &requested,
					       &image,
					       &requested_size,
					       &original_width,
					       &original_height,
					       &error))
	{
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			gth_image_viewer_page_file_loaded (self, FALSE);
		g_clear_error (&error);
		return;
	}

	if (! _g_file_equal (requested->file, self->priv->file_data->file))
		goto clear_data;

	if (image == NULL) {
		gth_image_viewer_page_file_loaded (self, FALSE);
		goto clear_data;
	}

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->viewer),
				    image,
				    original_width,
				    original_height);
	gth_image_viewer_set_requested_size (GTH_IMAGE_VIEWER (self->priv->viewer), requested_size);
	gtk_widget_queue_draw (self->priv->viewer);

	gth_image_history_clear (self->priv->history);
	gth_image_history_add_image (self->priv->history,
				     image,
				     requested_size,
				     FALSE);

	if ((original_width == -1) || (original_height == -1))
		/* In this case the image was loaded at its original size,
		 * so we get the original size from the image surface size. */
		gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer),
						    &original_width,
						    &original_height);
	g_file_info_set_attribute_int32 (self->priv->updated_info,
					 "frame::width",
					 original_width);
	g_file_info_set_attribute_int32 (self->priv->updated_info,
					 "frame::height",
					 original_height);

	{
		GthICCProfile *profile;

		profile = gth_image_get_icc_profile (image);
		if (profile != NULL) {
			char *desc = gth_icc_profile_get_description (profile);

			if (desc != NULL) {
				g_file_info_set_attribute_string (self->priv->updated_info,
								  "Loaded::Image::ColorProfile",
								  desc);
				g_free (desc);
			}
		}
	}

	gth_image_viewer_page_file_loaded (self, TRUE);
	update_image_quality_if_required (self);

clear_data:

	_g_object_unref (requested);
	_g_object_unref (image);
	g_clear_error (&error);

	return;
}


static void
_gth_image_viewer_page_load (GthImageViewerPage *self,
		             GthFileData        *file_data)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	int           i;

	if (self->priv->file_data != file_data) {
		_g_object_unref (self->priv->file_data);
		self->priv->file_data = gth_file_data_dup (file_data);
		_g_object_unref (self->priv->updated_info);
		self->priv->updated_info = g_file_info_new ();
	}
	self->priv->image_changed = FALSE;
	self->priv->loading_image = TRUE;

	for (i = 0; i < N_FORWARD_PRELOADERS; i++)
		_g_clear_object (&self->priv->next_file_data[i]);
	for (i = 0; i < N_BACKWARD_PRELOADERS; i++)
		_g_clear_object (&self->priv->prev_file_data[i]);

	file_store = gth_browser_get_file_store (self->priv->browser);
	if (gth_file_store_find_visible (file_store, self->priv->file_data->file, &iter)) {
		GtkTreeIter next_iter;

		next_iter = iter;
		for (i = 0; i < N_FORWARD_PRELOADERS; i++) {
			if (! gth_file_store_get_next_visible (file_store, &next_iter))
				break;
			self->priv->next_file_data[i] = g_object_ref (gth_file_store_get_file (file_store, &next_iter));
		}

		next_iter = iter;
		for (i = 0; i < N_BACKWARD_PRELOADERS; i++) {
			if (! gth_file_store_get_prev_visible (file_store, &next_iter))
				break;
			self->priv->prev_file_data[i] = g_object_ref (gth_file_store_get_file (file_store, &next_iter));
		}

		gth_image_viewer_set_void (GTH_IMAGE_VIEWER (self->priv->viewer));
	}

	_gth_image_viewer_page_load_with_preloader (self,
						    self->priv->file_data,
#ifdef ALWAYS_LOAD_ORIGINAL_SIZE
						    GTH_ORIGINAL_SIZE,
#else
						    _gth_image_preloader_get_requested_size_for_next_images (self),
#endif
						    NULL,
						    preloader_load_ready_cb,
						    self);
}


static void
gth_image_viewer_page_real_view (GthViewerPage *base,
				 GthFileData   *file_data)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;
	g_return_if_fail (file_data != NULL);
	g_return_if_fail (self->priv->active);

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	_g_clear_object (&self->priv->last_loaded);

	if ((self->priv->file_data != NULL)
	    && g_file_equal (file_data->file, self->priv->file_data->file)
	    && (gth_file_data_get_mtime (file_data) == gth_file_data_get_mtime (self->priv->file_data))
	    && ! self->priv->image_changed)
	{
		gth_image_viewer_page_file_loaded (self, TRUE);
		return;
	}

	_gth_image_viewer_page_load (self, file_data);
}


static void
gth_image_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_IMAGE_VIEWER_PAGE (base)->priv->viewer;
	if (gtk_widget_get_realized (widget) && gtk_widget_get_mapped (widget))
		gtk_widget_grab_focus (widget);
}


static void
gth_image_viewer_page_real_fullscreen (GthViewerPage *base,
				       gboolean       active)
{
	GthImageViewerPage *self;
	GthImageViewerTool *tool;

	self = (GthImageViewerPage *) base;
	tool = gth_image_viewer_get_tool (GTH_IMAGE_VIEWER (self->priv->viewer));

	if (! GTH_IS_IMAGE_DRAGGER (tool))
		return;

	g_object_set (tool, "show-frame", ! active, NULL);
}


static void
gth_image_viewer_page_real_show_pointer (GthViewerPage *base,
				         gboolean       show)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage *) base;

	if (show)
		gth_image_viewer_show_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
	else if (gth_browser_get_is_fullscreen (self->priv->browser))
		gth_image_viewer_hide_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));

	if (self->priv->hide_overview_id != 0) {
		g_source_remove (self->priv->hide_overview_id);
		self->priv->hide_overview_id = 0;
	}

	self->priv->pointer_on_viewer = show;
	update_overview_visibility (self);
}


static void
gth_image_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
	GthImageViewerPage *self;
	GthImage           *image;
	gboolean            enable_action;
	gboolean            is_animation;

	self = (GthImageViewerPage*) base;

	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "image-undo", gth_image_history_can_undo (self->priv->history));
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "image-redo", gth_image_history_can_redo (self->priv->history));

	image = gth_image_viewer_get_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	enable_action = (image != NULL) && (gth_image_get_icc_profile (image) != NULL);
	gtk_widget_set_visible (self->priv->buttons[APPLY_ICC_PROFILE_BUTTON], enable_action);
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "apply-icc-profile", enable_action);

	enable_action = (self->priv->file_data != NULL) && _g_mime_type_has_transparency (gth_file_data_get_mime_type (self->priv->file_data));
	gtk_widget_set_visible (self->priv->buttons[TRANSPARENCY_STYLE_BUTTON], enable_action);
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "transparency-style", enable_action);

	is_animation = gth_image_viewer_is_animation (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_widget_set_visible (self->priv->buttons[TOGGLE_ANIMATION_BUTTON], is_animation);
	gtk_widget_set_visible (self->priv->buttons[STEP_ANIMATION_BUTTON], is_animation);
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "step-animation", ! gth_image_viewer_is_playing_animation (GTH_IMAGE_VIEWER (self->priv->viewer)));

	_gth_image_viewer_page_update_paste_command_sensitivity (self, NULL);

	update_zoom_info (self);
}


typedef struct {
	GthImageViewerPage *self;
	GthFileData        *file_to_save;
	GthFileData        *original_file;
	FileSavedFunc       func;
	gpointer            user_data;
} SaveData;


static void
save_data_free (SaveData *data)
{
	g_object_unref (data->file_to_save);
	g_object_unref (data->original_file);
	g_free (data);
}


static void
save_image_task_completed_cb (GthTask *task,
			      GError      *error,
			      gpointer     user_data)
{
	SaveData           *data = user_data;
	GthImageViewerPage *self = data->self;
	gboolean            error_occurred;

	error_occurred = error != NULL;

	if (error_occurred) {
		gth_file_data_set_file (data->file_to_save, data->original_file->file);
		g_file_info_set_attribute_boolean (data->file_to_save->info, "gth::file::is-modified", FALSE);
	}

	if (data->func != NULL)
		(data->func) ((GthViewerPage *) self, data->file_to_save, error, data->user_data);
	else if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not save the file"), error);

	if (! error_occurred) {
		GFile *folder;
		GList *file_list;

		folder = g_file_get_parent (data->file_to_save->file);
		file_list = g_list_prepend (NULL, g_object_ref (data->file_to_save->file));
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    folder,
					    file_list,
					    GTH_MONITOR_EVENT_CHANGED);

		_g_object_list_unref (file_list);
		g_object_unref (folder);
	}

	save_data_free (data);
	_g_object_unref (task);
}


static void
_gth_image_viewer_page_real_save (GthViewerPage *base,
				  GFile         *file,
				  const char    *mime_type,
				  FileSavedFunc  func,
				  gpointer       user_data)
{
	GthImageViewerPage *self;
	SaveData           *data;
	GthFileData        *current_file;
	GthTask            *task;

	self = (GthImageViewerPage *) base;

	data = g_new0 (SaveData, 1);
	data->self = self;
	data->func = func;
	data->user_data = user_data;

	if (mime_type == NULL)
		mime_type = gth_file_data_get_mime_type (self->priv->file_data);

	current_file = gth_browser_get_current_file (self->priv->browser);
	if (current_file == NULL)
		return;

	data->file_to_save = g_object_ref (current_file);
	data->original_file = gth_file_data_dup (current_file);
	if (file != NULL)
		gth_file_data_set_file (data->file_to_save, file);

	/* save the value of 'gth::file::is-modified' into 'gth::file::image-changed'
	 * to allow the exiv2 metadata writer to not change some fields if the
	 * content wasn't modified. */
	g_file_info_set_attribute_boolean (data->file_to_save->info,
					   "gth::file::image-changed",
					   g_file_info_get_attribute_boolean (data->file_to_save->info, "gth::file::is-modified"));

	/* the 'gth::file::is-modified' attribute must be set to false before
	 * saving the file to avoid a scenario where the user is asked whether
	 * he wants to save the file after saving it.
	 * This is because when a file is modified in the current folder the
	 * folder_changed_cb function in gth-browser.c is called automatically
	 * and if the current file has been modified it is reloaded
	 * (see file_attributes_ready_cb in gth-browser.c) and if it has been
	 * modified ('gth::file::is-modified' is TRUE) the user is asked if he
	 * wants to save (see load_file_delayed_cb in gth-browser.c). */
	g_file_info_set_attribute_boolean (data->file_to_save->info, "gth::file::is-modified", FALSE);

	task = gth_image_task_chain_new (_("Saving"),
					 gth_original_image_task_new (self),
					 gth_save_image_task_new (NULL, mime_type, data->file_to_save, GTH_OVERWRITE_RESPONSE_YES),
					 NULL);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (save_image_task_completed_cb),
			  data);
	gth_browser_exec_task (GTH_BROWSER (self->priv->browser), task, GTH_TASK_FLAGS_DEFAULT);
}


static gboolean
gth_image_viewer_page_real_can_save (GthViewerPage *base)
{
	GArray *savers;

	savers = gth_main_get_type_set ("image-saver");
	return (savers != NULL) && (savers->len > 0);
}


static void
gth_image_viewer_page_real_save (GthViewerPage *base,
				 GFile         *file,
				 FileSavedFunc  func,
				 gpointer       user_data)
{
	_gth_image_viewer_page_real_save (base, file, NULL, func, user_data);
}


/* -- gth_image_viewer_page_real_save_as -- */


typedef struct {
	GthImageViewerPage *self;
	FileSavedFunc       func;
	gpointer            user_data;
	GthFileData        *file_data;
	GtkWidget          *file_sel;
} SaveAsData;


static void
save_as_destroy_cb (GtkWidget  *w,
		    SaveAsData *data)
{
	g_object_unref (data->file_data);
	g_free (data);
}


static void
save_as_response_cb (GtkDialog  *file_sel,
		     int         response,
		     SaveAsData *data)
{
	GFile      *file;
	const char *mime_type;

	if (response != GTK_RESPONSE_OK) {
		if (data->func != NULL) {
			(*data->func) ((GthViewerPage *) data->self,
				       data->file_data,
				       g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""),
				       data->user_data);
		}
		gtk_widget_destroy (GTK_WIDGET (file_sel));
		return;
	}

	if (! gth_file_chooser_dialog_get_file (GTH_FILE_CHOOSER_DIALOG (file_sel), &file, &mime_type))
		return;

	gtk_widget_hide (GTK_WIDGET (data->file_sel));

	gth_file_data_set_file (data->file_data, file);
	_gth_image_viewer_page_real_save ((GthViewerPage *) data->self,
					  file,
					  mime_type,
					  data->func,
					  data->user_data);

	gtk_widget_destroy (GTK_WIDGET (data->file_sel));

	g_object_unref (file);
}


static void
gth_image_viewer_page_real_save_as (GthViewerPage *base,
				    FileSavedFunc  func,
				    gpointer       user_data)
{
	GthImageViewerPage *self;
	GtkWidget          *file_sel;
	char               *uri;
	SaveAsData         *data;

	self = GTH_IMAGE_VIEWER_PAGE (base);
	file_sel = gth_file_chooser_dialog_new (_("Save Image"),
						GTK_WINDOW (self->priv->browser),
						"image-saver");
	_gtk_dialog_add_class_to_response (GTK_DIALOG (file_sel), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	uri = g_file_get_uri (self->priv->file_data->file);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (file_sel), uri);

	data = g_new0 (SaveAsData, 1);
	data->self = self;
	data->func = func;
	data->file_data = gth_file_data_dup (self->priv->file_data);
	data->user_data = user_data;
	data->file_sel = file_sel;

	g_signal_connect (GTK_DIALOG (file_sel),
			  "response",
			  G_CALLBACK (save_as_response_cb),
			  data);
	g_signal_connect (G_OBJECT (file_sel),
			  "destroy",
			  G_CALLBACK (save_as_destroy_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);

	g_free (uri);
}


static void
_gth_image_viewer_page_set_image (GthImageViewerPage *self,
				  GthImage           *image,
				  int                 requested_size,
				  gboolean            modified)
{
	GthFileData *file_data;
	int          width;
	int          height;

	if (image == NULL)
		return;

	if (modified)
		gth_image_preloader_set_modified_image (self->priv->preloader, image);
	gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->viewer), image, -1, -1);
	gth_image_viewer_set_requested_size (GTH_IMAGE_VIEWER (self->priv->viewer), requested_size);

	file_data = gth_browser_get_current_file (GTH_BROWSER (self->priv->browser));

	self->priv->image_changed = modified;
	g_file_info_set_attribute_boolean (file_data->info, "gth::file::is-modified", modified);

	if (gth_image_get_original_size (image, &width, &height)) {
		char *size;

		g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
		g_file_info_set_attribute_int32 (file_data->info, "image::height", height);

		size = g_strdup_printf (_("%d × %d"), width, height);
		g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);
		g_free (size);
	}

	gth_monitor_metadata_changed (gth_main_get_default_monitor (), file_data);

	update_image_quality_if_required (self);
}


static void
_gth_image_viewer_page_set_surface (GthImageViewerPage *self,
				    cairo_surface_t    *surface,
				    int                 requested_size,
				    gboolean            modified)
{
	GthImage *image;

	image = gth_image_new_for_surface (surface);
	_gth_image_viewer_page_set_image (self, image, requested_size, modified);

	g_object_unref (image);
}


static void
gth_image_viewer_page_real_revert (GthViewerPage *base)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);
	GthImageData       *last_saved;

	last_saved = gth_image_history_revert (self->priv->history);
	if (last_saved == NULL)
		return;

	gth_image_history_add_image (self->priv->history,
				     last_saved->image,
				     last_saved->requested_size,
				     last_saved->unsaved);
	_gth_image_viewer_page_set_image (self,
					  last_saved->image,
					  last_saved->requested_size,
					  last_saved->unsaved);

	gth_image_data_unref (last_saved);
}


static void
gth_image_viewer_page_real_update_info (GthViewerPage *base,
					GthFileData   *file_data)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);

	if (! _g_file_equal (self->priv->file_data->file, file_data->file))
		return;
	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);

	if (self->priv->viewer == NULL)
		return;

	gtk_widget_queue_draw (self->priv->viewer);
}


static gboolean
gth_image_viewer_page_real_zoom_from_scroll (GthViewerPage  *base,
					     GdkEventScroll *event)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);
	return gth_image_viewer_zoom_from_scroll (GTH_IMAGE_VIEWER (self->priv->viewer), event);
}


static void
gth_image_viewer_page_real_show_properties (GthViewerPage *base,
					    gboolean       show)
{
	GthImageViewerPage *self;

	self = GTH_IMAGE_VIEWER_PAGE (base);

	if (show)
		gth_image_viewer_add_painter (GTH_IMAGE_VIEWER (self->priv->viewer), paint_comment_over_image_func, self);
	else
		gth_image_viewer_remove_painter (GTH_IMAGE_VIEWER (self->priv->viewer), paint_comment_over_image_func, self);
	gtk_widget_queue_draw (self->priv->viewer);
}


static const char *
gth_image_viewer_page_shortcut_context (GthViewerPage *base)
{
	return GTH_SHORTCUT_VIEWER_CONTEXT_IMAGE;
}


static void
gth_image_viewer_page_finalize (GObject *obj)
{
	GthImageViewerPage *self;
	int                 i;

	self = GTH_IMAGE_VIEWER_PAGE (obj);

	if (self->priv->update_quality_id != 0) {
		g_source_remove (self->priv->update_quality_id);
		self->priv->update_quality_id = 0;
	}
	if (self->priv->update_visibility_id != 0) {
		g_source_remove (self->priv->update_visibility_id);
		self->priv->update_visibility_id = 0;
	}
	if (self->priv->hide_overview_id != 0) {
		g_source_remove (self->priv->hide_overview_id);
		self->priv->hide_overview_id = 0;
	}

	g_object_unref (self->priv->settings);
	g_object_unref (self->priv->history);
	_g_object_unref (self->priv->file_data);
	_g_object_unref (self->priv->last_loaded);

	for (i = 0; i < N_FORWARD_PRELOADERS; i++)
		_g_clear_object (&self->priv->next_file_data[i]);
	for (i = 0; i < N_BACKWARD_PRELOADERS; i++)
		_g_clear_object (&self->priv->prev_file_data[i]);

	G_OBJECT_CLASS (gth_image_viewer_page_parent_class)->finalize (obj);
}


static void
gth_image_viewer_page_class_init (GthImageViewerPageClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_image_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageInterface *iface)
{
	iface->activate = gth_image_viewer_page_real_activate;
	iface->deactivate = gth_image_viewer_page_real_deactivate;
	iface->show = gth_image_viewer_page_real_show;
	iface->hide = gth_image_viewer_page_real_hide;
	iface->can_view = gth_image_viewer_page_real_can_view;
	iface->view = gth_image_viewer_page_real_view;
	iface->focus = gth_image_viewer_page_real_focus;
	iface->fullscreen = gth_image_viewer_page_real_fullscreen;
	iface->show_pointer = gth_image_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_image_viewer_page_real_update_sensitivity;
	iface->can_save = gth_image_viewer_page_real_can_save;
	iface->save = gth_image_viewer_page_real_save;
	iface->save_as = gth_image_viewer_page_real_save_as;
	iface->revert = gth_image_viewer_page_real_revert;
	iface->update_info = gth_image_viewer_page_real_update_info;
	iface->zoom_from_scroll = gth_image_viewer_page_real_zoom_from_scroll;
	iface->show_properties = gth_image_viewer_page_real_show_properties;
	iface->shortcut_context = gth_image_viewer_page_shortcut_context;
}


static void
gth_image_viewer_page_init (GthImageViewerPage *self)
{
	int i;

	self->priv = gth_image_viewer_page_get_instance_private (self);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	self->priv->preloader = NULL;
	self->priv->file_popup_merge_id = 0;
	self->priv->history = gth_image_history_new ();
	self->priv->file_data = NULL;
	self->priv->updated_info = NULL;
	self->priv->active = FALSE;
	self->priv->image_changed = FALSE;
	self->priv->loading_image = FALSE;
	self->priv->last_loaded = NULL;
	self->priv->can_paste = FALSE;
	self->priv->update_quality_id = 0;
	self->priv->update_visibility_id = 0;
	for (i = 0; i < N_HEADER_BAR_BUTTONS; i++)
		self->priv->buttons[i] = NULL;
	self->priv->builder = NULL;
	self->priv->pointer_on_overview = FALSE;
	self->priv->pointer_on_viewer = FALSE;
	self->priv->hide_overview_id = 0;
	self->priv->apply_icc_profile = TRUE;

	for (i = 0; i < N_FORWARD_PRELOADERS; i++)
		self->priv->next_file_data[i] = NULL;
	for (i = 0; i < N_BACKWARD_PRELOADERS; i++)
		self->priv->prev_file_data[i] = NULL;

	self->priv->drag_data_get_event = 0;

	/* settings notifications */

	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_ZOOM_QUALITY,
			  G_CALLBACK (pref_zoom_quality_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_ZOOM_CHANGE,
			  G_CALLBACK (pref_zoom_change_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_RESET_SCROLLBARS,
			  G_CALLBACK (pref_reset_scrollbars_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_TRANSPARENCY_STYLE,
			  G_CALLBACK (pref_transparency_style_changed),
			  self);
}


GtkWidget *
gth_image_viewer_page_get_image_viewer (GthImageViewerPage *self)
{
	return self->priv->viewer;
}


GdkPixbuf *
gth_image_viewer_page_get_pixbuf (GthImageViewerPage *self)
{
	return gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer));
}


void
gth_image_viewer_page_set_pixbuf (GthImageViewerPage *self,
				  GdkPixbuf          *pixbuf,
				  gboolean            add_to_history)
{
	cairo_surface_t *image;

	image = _cairo_image_surface_create_from_pixbuf (pixbuf);
	gth_image_viewer_page_set_image (self, image, add_to_history);

	cairo_surface_destroy (image);
}


cairo_surface_t *
gth_image_viewer_page_get_current_image (GthImageViewerPage *self)
{
	return gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
}


cairo_surface_t *
gth_image_viewer_page_get_modified_image (GthImageViewerPage *self)
{
	return gth_image_preloader_get_modified_image (self->priv->preloader);
}


void
gth_image_viewer_page_set_image (GthImageViewerPage *self,
			 	 cairo_surface_t    *image,
			 	 gboolean            add_to_history)
{
	if (gth_image_viewer_page_get_current_image (self) == image)
		return;

	if (add_to_history)
		gth_image_history_add_surface (self->priv->history, image, -1, TRUE);

	_gth_image_viewer_page_set_surface (self, image, -1, TRUE);

	if (add_to_history)
		gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
}


void
gth_image_viewer_page_undo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_undo (self->priv->history);
	if (idata != NULL)
		_gth_image_viewer_page_set_image (self, idata->image, idata->requested_size, idata->unsaved);
}


void
gth_image_viewer_page_redo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_redo (self->priv->history);
	if (idata != NULL)
		_gth_image_viewer_page_set_image (self, idata->image, idata->requested_size, idata->unsaved);
}


GthImageHistory *
gth_image_viewer_page_get_history (GthImageViewerPage *self)
{
	return self->priv->history;
}


void
gth_image_viewer_page_reset (GthImageViewerPage *self)
{
	GthImageData *last_image;

	last_image = gth_image_history_get_last (self->priv->history);
	if (last_image == NULL)
		return;

	_gth_image_viewer_page_set_image (self, last_image->image, last_image->requested_size, last_image->unsaved);
}


static void
viewer_drag_data_get_cb (GtkWidget        *widget,
			 GdkDragContext   *context,
			 GtkSelectionData *data,
			 guint             info,
			 guint             time,
			 gpointer          user_data)
{
	GthImageViewerPage *self = user_data;
	char               *uris[2];

	if (self->priv->file_data == NULL)
		return;

	uris[0] = g_file_get_uri (self->priv->file_data->file);
	uris[1] = NULL;
	gtk_selection_data_set_uris (data, (char **) uris);

	g_free (uris[0]);
}


static void
_gth_image_viewer_page_enable_drag_source (GthImageViewerPage *self,
					   gboolean            enable)
{
	GthImageViewerTool *dragger;
	GtkTargetList      *source_target_list;
	GtkTargetEntry     *source_targets;
	int                 n_source_targets;

	dragger = gth_image_viewer_get_tool (GTH_IMAGE_VIEWER (self->priv->viewer));
	if (! GTH_IS_IMAGE_DRAGGER (dragger))
		return;

	if (! enable) {
		if (self->priv->drag_data_get_event > 0) {
			g_signal_handler_disconnect (self->priv->viewer, self->priv->drag_data_get_event);
			self->priv->drag_data_get_event = 0;
		}
		gth_image_dragger_disable_drag_source (GTH_IMAGE_DRAGGER (dragger));
		return;
	}

	source_target_list = gtk_target_list_new (NULL, 0);
	gtk_target_list_add_uri_targets (source_target_list, 0);
	gtk_target_list_add_text_targets (source_target_list, 0);
	source_targets = gtk_target_table_new_from_list (source_target_list, &n_source_targets);
	gth_image_dragger_enable_drag_source (GTH_IMAGE_DRAGGER (dragger),
					      GDK_BUTTON1_MASK,
					      source_targets,
					      n_source_targets,
					      GDK_ACTION_COPY | GDK_ACTION_MOVE);
	gtk_target_table_free (source_targets, n_source_targets);
	gtk_target_list_unref (source_target_list);

	if (self->priv->drag_data_get_event == 0)
		self->priv->drag_data_get_event = g_signal_connect (self->priv->viewer,
				  "drag-data-get",
				  G_CALLBACK (viewer_drag_data_get_cb),
				  self);
}


void
gth_image_viewer_page_reset_viewer_tool	(GthImageViewerPage *self)
{
	GthImageViewerTool *dragger;

	dragger = gth_image_dragger_new (TRUE);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (self->priv->viewer), dragger);
	g_object_unref (dragger);

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE_IF_LARGER);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer),
					   g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer),
					  g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE));
	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer),
					       g_settings_get_boolean (self->priv->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS));
	gth_image_viewer_enable_key_bindings (GTH_IMAGE_VIEWER (self->priv->viewer), FALSE);
	pref_transparency_style_changed (self->priv->settings, NULL, self);
	_gth_image_viewer_page_enable_drag_source (self, TRUE);
}


gboolean
gth_image_viewer_page_get_is_modified (GthImageViewerPage *self)
{
	return self->priv->image_changed;
}


/* -- gth_image_viewer_page_copy_image -- */


static void
copy_image_original_image_ready_cb (GthTask  *task,
				    GError   *error,
				    gpointer  user_data)
{
	GthImageViewerPage *self = user_data;
	cairo_surface_t    *image;

	image = gth_original_image_task_get_image (task);
	if (image != NULL) {
		GtkClipboard *clipboard;
		GdkPixbuf    *pixbuf;

		clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (self->priv->viewer), GDK_SELECTION_CLIPBOARD);
		pixbuf = _gdk_pixbuf_new_from_cairo_surface (image);
		gtk_clipboard_set_image (clipboard, pixbuf);

		g_object_unref (pixbuf);
	}

	cairo_surface_destroy (image);
	g_object_unref (task);
}


void
gth_image_viewer_page_copy_image (GthImageViewerPage *self)
{
	GthTask *task;

	task = gth_original_image_task_new (self);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (copy_image_original_image_ready_cb),
			  self);
	gth_browser_exec_task (self->priv->browser, task, GTH_TASK_FLAGS_DEFAULT);
}


static void
clipboard_image_received_cb (GtkClipboard *clipboard,
			     GdkPixbuf    *pixbuf,
			     gpointer      user_data)
{
	GthImageViewerPage *self = user_data;

	if (pixbuf != NULL)
		gth_image_viewer_page_set_pixbuf (self, pixbuf, TRUE);
	g_object_unref (self);
}


void
gth_image_viewer_page_paste_image (GthImageViewerPage *self)
{
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (self->priv->viewer), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_image (clipboard,
				     clipboard_image_received_cb,
				     g_object_ref (self));
}


/* -- gth_image_viewer_page_get_original -- */


typedef struct {
	GthImageViewerPage *viewer_page;
	GTask              *task;
	GCancellable       *cancellable;
} OriginalImageData;


static OriginalImageData *
get_original_data_new (void)
{
	OriginalImageData *data;

	data = g_new0 (OriginalImageData, 1);
	data->cancellable = NULL;
	data->task = NULL;

	return data;
}


static void
get_original_data_free (OriginalImageData *data)
{
	if (data == NULL)
		return;

	_g_object_unref (data->viewer_page);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->task);
	g_free (data);
}


static void
original_image_ready_cb (GObject	*source_object,
			 GAsyncResult	*result,
			 gpointer	 user_data)
{
	OriginalImageData *data = user_data;
	GthImage          *image = NULL;
	GError            *error = NULL;

	if (! _gth_image_viewer_page_load_with_preloader_finish (data->viewer_page)) {
		g_task_return_error (data->task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
		get_original_data_free (data);
		return;
	}

	if (! gth_image_preloader_load_finish (GTH_IMAGE_PRELOADER (source_object),
					       result,
					       NULL,
					       &image,
					       NULL,
					       NULL,
					       NULL,
					       &error))
	{
		g_task_return_error (data->task, error);
	}
	else
		g_task_return_pointer (data->task, image, (GDestroyNotify) g_object_unref);

	get_original_data_free (data);
}


void
gth_image_viewer_page_get_original (GthImageViewerPage	 *self,
				    GCancellable	 *cancellable,
				    GAsyncReadyCallback	  ready_callback,
				    gpointer		  user_data)
{
	OriginalImageData *data;

	data = get_original_data_new ();
	data->viewer_page = g_object_ref (self);
	data->cancellable = (cancellable != NULL) ? g_object_ref (cancellable) : g_cancellable_new ();
	data->task = g_task_new (G_OBJECT (self),
				 data->cancellable,
				 ready_callback,
				 user_data);

	if (gth_image_viewer_is_animation (GTH_IMAGE_VIEWER (self->priv->viewer))) {
		GthImage *image;

		image = gth_image_new_for_surface (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)));
		g_task_return_pointer (data->task, image, (GDestroyNotify) g_object_unref);

		get_original_data_free (data);
	}
	else {
		_gth_image_viewer_page_load_with_preloader (self,
							    self->priv->image_changed ? GTH_MODIFIED_IMAGE : self->priv->file_data,
							    GTH_ORIGINAL_SIZE,
							    data->cancellable,
							    original_image_ready_cb,
							    data);
	}
}


gboolean
gth_image_viewer_page_get_original_finish (GthImageViewerPage	 *self,
					   GAsyncResult		 *result,
					   cairo_surface_t	**image_p,
					   GError		**error)
{
	GthImage *image;

	g_return_val_if_fail (g_task_is_valid (G_TASK (result), G_OBJECT (self)), FALSE);

	image = g_task_propagate_pointer (G_TASK (result), error);
	if (image == NULL)
		return FALSE;

	if (image_p != NULL)
		*image_p = gth_image_get_cairo_surface (image);

	g_object_unref (image);

	return TRUE;
}


/* -- GthOriginalImageTask -- */


#define GTH_TYPE_ORIGINAL_IMAGE_TASK	(gth_original_image_task_get_type ())
#define GTH_ORIGINAL_IMAGE_TASK(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_ORIGINAL_IMAGE_TASK, GthOriginalImageTask))


typedef struct _GthOriginalImageTask        GthOriginalImageTask;
typedef struct _GthOriginalImageTaskClass   GthOriginalImageTaskClass;


struct _GthOriginalImageTask {
	GthImageTask __parent;
	GthImageViewerPage *viewer_page;
};


struct _GthOriginalImageTaskClass {
	GthImageTaskClass __parent;
};

GType gth_original_image_task_get_type (void);

G_DEFINE_TYPE (GthOriginalImageTask, gth_original_image_task, GTH_TYPE_IMAGE_TASK)


static void
get_original_image_ready_cb (GObject		*source_object,
			    GAsyncResult	*result,
			    gpointer		 user_data)
{
	GthOriginalImageTask *self = user_data;
	cairo_surface_t      *image = NULL;
	GError               *error = NULL;

	if (gth_image_viewer_page_get_original_finish (self->viewer_page,
						       result,
						       &image,
						       &error))
	{
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (self), image);
	}
	gth_task_completed (GTH_TASK (self), error);

	cairo_surface_destroy (image);
	_g_error_free (error);
}


static void
gth_original_image_task_exec (GthTask *base)
{
	GthOriginalImageTask *self = GTH_ORIGINAL_IMAGE_TASK (base);

	gth_task_progress (base, _("Loading the original image"), NULL, TRUE, 0.0);
	gth_image_viewer_page_get_original (self->viewer_page,
					    gth_task_get_cancellable (base),
					    get_original_image_ready_cb,
					    self);
}


static void
gth_original_image_task_class_init (GthOriginalImageTaskClass *class)
{
	GthTaskClass *task_class;

	task_class = GTH_TASK_CLASS (class);
	task_class->exec = gth_original_image_task_exec;
}


static void
gth_original_image_task_init (GthOriginalImageTask *self)
{
	self->viewer_page = NULL;
	gth_task_set_for_viewer (GTH_TASK (self), TRUE);
}


GthTask *
gth_original_image_task_new (GthImageViewerPage *self)
{
	GthOriginalImageTask *task;

	task = g_object_new (GTH_TYPE_ORIGINAL_IMAGE_TASK, NULL);
	task->viewer_page = self;

	return GTH_TASK (task);
}


cairo_surface_t *
gth_original_image_task_get_image (GthTask *task)
{
	return gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
}


void
gth_image_viewer_page_apply_icc_profile	(GthImageViewerPage *self,
					 gboolean            apply)
{
	GthFileData *file_data;

	g_return_if_fail (self->priv->active);

	self->priv->apply_icc_profile = apply;
	gth_image_preloader_clear_cache (self->priv->preloader);

	file_data = gth_browser_get_current_file (self->priv->browser);
	if (file_data == NULL)
		return;

	/* force a complete reload */
	_g_object_unref (self->priv->last_loaded);
	self->priv->last_loaded = NULL;

	g_object_ref (file_data);
	_gth_image_viewer_page_load (self, file_data);

	g_object_unref (file_data);
}
