/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "actions.h"
#include "gth-image-viewer-page.h"
#include "preferences.h"



static GthImageViewerPage *
get_image_viewer_page (GthBrowser *browser)
{
	GthViewerPage *viewer_page = gth_browser_get_viewer_page (browser);

	if ((viewer_page != NULL) && GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return GTH_IMAGE_VIEWER_PAGE (viewer_page);
	else
		return NULL;
}


void
gth_browser_activate_image_zoom_in (GSimpleAction	*action,
				    GVariant		*parameter,
				    gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_zoom_in (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)));
}


void
gth_browser_activate_image_zoom_out (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_zoom_out (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)));
}


void
gth_browser_activate_image_zoom_100 (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), 1.0);
}


void
gth_browser_activate_image_zoom_200 (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), 2.0);
}


void
gth_browser_activate_image_zoom_300 (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), 3.0);
}


void
gth_browser_activate_image_zoom_fit (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), GTH_FIT_SIZE);
}


void
gth_browser_activate_image_zoom_fit_if_larger (GSimpleAction	*action,
					       GVariant		*parameter,
					       gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), GTH_FIT_SIZE_IF_LARGER);
}


void
gth_browser_activate_image_zoom_fit_width (GSimpleAction	*action,
					   GVariant		*parameter,
					   gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), GTH_FIT_WIDTH);
}


void
gth_browser_activate_image_zoom_fit_width_if_larger (GSimpleAction	*action,
						     GVariant		*parameter,
						     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), GTH_FIT_WIDTH_IF_LARGER);
}


void
gth_browser_activate_image_zoom_fit_height (GSimpleAction	*action,
					    GVariant		*parameter,
					    gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), GTH_FIT_HEIGHT);
}


void
gth_browser_activate_image_zoom_fit_height_if_larger (GSimpleAction	*action,
						      GVariant		*parameter,
						      gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page)), GTH_FIT_HEIGHT_IF_LARGER);
}


void
gth_browser_activate_image_undo (GSimpleAction	*action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_page_undo (viewer_page);
}


void
gth_browser_activate_image_redo (GSimpleAction	*action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_page_redo (viewer_page);
}


void
gth_browser_activate_copy_image (GSimpleAction	*action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_page_copy_image (viewer_page);
}


void
gth_browser_activate_paste_image (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	gth_image_viewer_page_paste_image (viewer_page);
}


void
gth_browser_activate_apply_icc_profile  (GSimpleAction	*action,
					 GVariant	*state,
					 gpointer	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);

	if (viewer_page == NULL)
		return;

	g_simple_action_set_state (action, state);
	gth_image_viewer_page_apply_icc_profile (viewer_page, g_variant_get_boolean (state));
}


void
gth_browser_activate_transparency_style (GSimpleAction	*action,
					 GVariant	*parameter,
					 gpointer	 user_data)
{
	GthBrowser           *browser = user_data;
	GthImageViewerPage   *viewer_page = get_image_viewer_page (browser);
	const char           *state;
	GthTransparencyStyle  style;
	GSettings            *settings;

	if (viewer_page == NULL)
		return;

	state = g_variant_get_string (parameter, NULL);
	if (state == NULL)
		return;

	g_simple_action_set_state (action, g_variant_new_string (state));

	if (strcmp (state, "white") == 0)
		style = GTH_TRANSPARENCY_STYLE_WHITE;
	else if (strcmp (state, "gray") == 0)
		style = GTH_TRANSPARENCY_STYLE_GRAY;
	else if (strcmp (state, "black") == 0)
		style = GTH_TRANSPARENCY_STYLE_BLACK;
	else
		style = GTH_TRANSPARENCY_STYLE_CHECKERED;

	settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	g_settings_set_enum (settings, PREF_IMAGE_VIEWER_TRANSPARENCY_STYLE, style);

	g_object_unref (settings);
}


void
gth_browser_activate_image_zoom  (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	const char         *state;
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	state = g_variant_get_string (parameter, NULL);
	g_simple_action_set_state (action, g_variant_new_string (state));

	if (state == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	if (strcmp (state, "automatic") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_SIZE_IF_LARGER);
	else if (strcmp (state, "fit") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_SIZE);
	else if (strcmp (state, "fit-width") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_WIDTH);
	else if (strcmp (state, "fit-height") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_HEIGHT);
	else if (strcmp (state, "50") == 0)
		gth_image_viewer_set_zoom (image_viewer, 0.5);
	else if (strcmp (state, "100") == 0)
		gth_image_viewer_set_zoom (image_viewer, 1.0);
	else if (strcmp (state, "200") == 0)
		gth_image_viewer_set_zoom (image_viewer, 2.0);
	else if (strcmp (state, "300") == 0)
		gth_image_viewer_set_zoom (image_viewer, 3.0);
}


void
gth_browser_activate_toggle_animation (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	g_simple_action_set_state (action, state);

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));

	if (gth_image_viewer_is_playing_animation (image_viewer))
		gth_image_viewer_stop_animation (image_viewer);
	else
		gth_image_viewer_start_animation (image_viewer);
	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (viewer_page));
}


void
gth_browser_activate_step_animation (GSimpleAction	*action,
				     GVariant		*state,
				     gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_step_animation (image_viewer);
}


void
gth_browser_activate_scroll_step_left (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_step_x (image_viewer, FALSE);
}


void
gth_browser_activate_scroll_step_right (GSimpleAction	*action,
				        GVariant		*state,
				        gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_step_x (image_viewer, TRUE);
}


void
gth_browser_activate_scroll_step_up (GSimpleAction	*action,
				     GVariant		*state,
				     gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_step_y (image_viewer, FALSE);
}

void
gth_browser_activate_scroll_step_down (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_step_y (image_viewer, TRUE);
}

void
gth_browser_activate_scroll_page_left (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_page_x (image_viewer, FALSE);
}

void
gth_browser_activate_scroll_page_right (GSimpleAction	*action,
				        GVariant		*state,
				        gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_page_x (image_viewer, TRUE);
}

void
gth_browser_activate_scroll_page_up (GSimpleAction	*action,
				     GVariant		*state,
				     gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_page_y (image_viewer, FALSE);
}

void
gth_browser_activate_scroll_page_down (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_page_y (image_viewer, TRUE);
}


void
gth_browser_activate_scroll_to_center (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser         *browser = user_data;
	GthImageViewerPage *viewer_page = get_image_viewer_page (browser);
	GthImageViewer     *image_viewer;

	if (viewer_page == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (viewer_page));
	gth_image_viewer_scroll_to_center (image_viewer);
}

