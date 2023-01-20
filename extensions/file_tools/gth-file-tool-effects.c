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

#include <config.h>
#include <math.h>
#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>
#include "cairo-blur.h"
#include "cairo-effects.h"
#include "gth-curve.h"
#include "gth-file-tool-effects.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


struct _GthFileToolEffectsPrivate {
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GthTask            *image_task;
	GthImageViewerTool *preview_tool;
	guint               apply_event;
	gboolean            apply_to_original;
	gboolean            closing;
	gboolean            view_original;
	int                 method;
	int                 last_applied_method;
	GtkWidget          *filter_grid;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolEffects,
			 gth_file_tool_effects,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolEffects))


static void apply_changes (GthFileToolEffects *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolEffects *self = user_data;
	GthImage           *destination_image;

	g_signal_handlers_disconnect_matched (task, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, image_task_completed_cb, self);
	self->priv->image_task = NULL;

	if (self->priv->closing) {
		g_object_unref (task);
		gth_image_viewer_page_tool_reset_image (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
		return;
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			apply_changes (self);
		g_object_unref (task);
		return;
	}

	destination_image = gth_image_task_get_destination (GTH_IMAGE_TASK (task));
	if (destination_image == NULL) {
		g_object_unref (task);
		return;
	}

	cairo_surface_destroy (self->priv->destination);
	self->priv->destination = gth_image_get_cairo_surface (destination_image);
	self->priv->last_applied_method = self->priv->method;

	if (self->priv->apply_to_original) {
		if (self->priv->destination != NULL) {
			GtkWidget     *window;
			GthViewerPage *viewer_page;

			window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
			viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
			gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->destination, TRUE);
		}

		gth_file_tool_hide_options (GTH_FILE_TOOL (self));
	}
	else {
		if (! self->priv->view_original)
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}

	g_object_unref (task);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolEffects *self = user_data;
	GtkWidget          *window;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return FALSE;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	self->priv->image_task = gth_filter_grid_get_task (GTH_FILTER_GRID (self->priv->filter_grid), self->priv->method);
	if (self->priv->apply_to_original)
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->image_task), gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self)));
	else
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->image_task), self->priv->preview);
	g_signal_connect (self->priv->image_task,
			  "completed",
			  G_CALLBACK (image_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (window), self->priv->image_task, GTH_TASK_FLAGS_DEFAULT);

	return FALSE;
}


static void
apply_changes (GthFileToolEffects *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
gth_file_tool_effects_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolEffects *self = GTH_FILE_TOOL_EFFECTS (base);

	if (self->priv->image_task != NULL) {
		self->priv->closing = TRUE;
		return;
	}

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
filter_grid_activated_cb (GthFilterGrid	*filter_grid,
			  int            filter_id,
			  gpointer       user_data)
{
	GthFileToolEffects *self = user_data;

	self->priv->view_original = (filter_id == GTH_FILTER_GRID_NO_FILTER);
	if (self->priv->view_original) {
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	}
	else if (filter_id == self->priv->last_applied_method) {
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}
	else {
		self->priv->method = filter_id;
		apply_changes (self);
	}
}


static GtkWidget *
gth_file_tool_effects_get_options (GthFileTool *base)
{
	GthFileToolEffects *self;
	GtkWidget          *window;
	GthViewerPage      *viewer_page;
	GtkWidget          *viewer;
	cairo_surface_t    *source;
	GtkWidget          *options;
	int                 width, height;
	GtkAllocation       allocation;

	self = (GthFileToolEffects *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->destination);
	cairo_surface_destroy (self->priv->preview);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	if (scale_keeping_ratio (&width, &height, PREVIEW_SIZE * allocation.width, PREVIEW_SIZE * allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_fast (source, width, height);
	else
		self->priv->preview = cairo_surface_reference (source);

	self->priv->destination = cairo_surface_reference (self->priv->preview);
	self->priv->apply_to_original = FALSE;
	self->priv->closing = FALSE;

	self->priv->builder = _gtk_builder_new_from_file ("effects-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->filter_grid = gth_filter_grid_new ();
	gth_hook_invoke ("add-special-effect", self->priv->filter_grid);
	gtk_widget_show (self->priv->filter_grid);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filter_grid_box")), self->priv->filter_grid, TRUE, FALSE, 0);

	g_signal_connect (self->priv->filter_grid,
			  "activated",
			  G_CALLBACK (filter_grid_activated_cb),
			  self);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	gth_filter_grid_generate_previews (GTH_FILTER_GRID (self->priv->filter_grid), source);

	return options;
}


static void
gth_file_tool_effects_destroy_options (GthFileTool *base)
{
	GthFileToolEffects *self;
	GtkWidget          *window;
	GthViewerPage      *viewer_page;

	self = (GthFileToolEffects *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_viewer_page_update_sensitivity (viewer_page);

	_g_clear_object (&self->priv->builder);

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);
	self->priv->method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->last_applied_method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->view_original = TRUE;
}


static void
gth_file_tool_effects_apply_options (GthFileTool *base)
{
	GthFileToolEffects *self;

	self = (GthFileToolEffects *) base;

	if (! self->priv->view_original) {
		self->priv->apply_to_original = TRUE;
		apply_changes (self);
	}
}


static void
gth_file_tool_effects_finalize (GObject *object)
{
	GthFileToolEffects *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_EFFECTS (object));

	self = (GthFileToolEffects *) object;

	_g_clear_object (&self->priv->builder);
	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);

	G_OBJECT_CLASS (gth_file_tool_effects_parent_class)->finalize (object);
}


static void
gth_file_tool_effects_class_init (GthFileToolEffectsClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_effects_finalize;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->get_options = gth_file_tool_effects_get_options;
	file_tool_class->destroy_options = gth_file_tool_effects_destroy_options;
	file_tool_class->apply_options = gth_file_tool_effects_apply_options;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_effects_reset_image;
}


static void
gth_file_tool_effects_init (GthFileToolEffects *self)
{
	self->priv = gth_file_tool_effects_get_instance_private (self);
	self->priv->preview = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->last_applied_method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->view_original = TRUE;

	gth_file_tool_construct (GTH_FILE_TOOL (self),
				 "special-effects-symbolic",
				 _("Special Effects"),
				 GTH_TOOLBOX_SECTION_COLORS);
}


/* -- Warmer -- */


static gpointer
warmer_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 117,136, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 136,119, 255,255);
	if (cairo_image_surface_apply_curves (source, curve, task))
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
warmer_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, warmer_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of a filter that produces warmer colors */
				    _("Warmer"),
				    NULL);
}


/* -- Cooler -- */


static gpointer
cooler_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 136,119, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 117,136, 255,255);
	if (cairo_image_surface_apply_curves (source, curve, task))
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
cooler_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, cooler_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of a filter that produces cooler colors */
				    _("Cooler"),
				    NULL);
}


/* -- Soil -- */


static gpointer
soil_exec (GthAsyncTask *task,
	   gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 75,83, 198,185, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 70,63, 184,189, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 76,74, 191,176, 255,255);

	if (cairo_image_surface_apply_curves (source, curve, task)
	    && cairo_image_surface_apply_vignette (source, NULL, 127, task))
	{
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
soil_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, soil_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Soil"),
				    NULL);
}


/* -- Desert -- */


static gpointer
desert_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 115,145, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 71,66, 208,204, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 71,55, 200,206, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 232,185, 255,248);

	if (cairo_image_surface_apply_curves (source, curve, task)
	    && cairo_image_surface_apply_bcs (source, 0, 0, 20.0 / 100, task)
	    && cairo_image_surface_apply_vignette (source, NULL, 127, task))
	{
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
desert_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, desert_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Desert"),
				    NULL);
}


/* -- Artic -- */


static gpointer
artic_exec (GthAsyncTask *task,
	    gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 133,122, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 64,57, 176,186, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 180,207, 255,255);

	if (cairo_image_surface_apply_curves (source, curve, task)
	    && cairo_image_surface_apply_vignette (source, NULL, 127, task))
	{
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
artic_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, artic_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Arctic"),
				    NULL);
}


/* -- Mangos -- */


static gpointer
mangos_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 5, 0,0, 75,67, 135,155, 171,193, 252,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 84,65, 162,167, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 84,74, 160,150, 255,255);

	if (cairo_image_surface_apply_curves (source, curve, task)
	    && cairo_image_surface_apply_vignette (source, NULL, 127, task))
	{
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
mangos_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, mangos_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Mangos"),
				    NULL);
}


/* -- Fresh Blue -- */


static gpointer
fresh_blue_exec (GthAsyncTask *task,
		 gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 95,61, 116,71, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 81,94, 96,125, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 75,107, 86,129, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0,0, 39,139, 58,168, 255,255);

	if (cairo_image_surface_apply_curves (source, curve, task)
	    && cairo_image_surface_apply_vignette (source, NULL, 127, task))
	{
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
fresh_blue_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, fresh_blue_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Fresh Blue"),
				    NULL);
}


/* -- Cherry -- */


static gpointer
cherry_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 5, 0,12, 74,79, 134,156, 188,209, 239,255);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 5, 12,0, 78,67, 138,140, 189,189, 252,233);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 5, 0,8, 77,100, 139,140, 202,186, 255,244);
	if (cairo_image_surface_apply_curves (source, curve, task)
	    && cairo_image_surface_apply_vignette (source, NULL, 127, task))
	{
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
cherry_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, cherry_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Cherry"),
				    NULL);
}


/* -- Vintage -- */


static gpointer
vintage_exec (GthAsyncTask *task,
	 gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 76,173, 255,255);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);

	if (cairo_image_surface_colorize (source, 112, 66, 20, 255, task)
	    && cairo_image_surface_apply_bcs (source, 0, -20 / 100, -20 / 100, task)
	    && cairo_image_surface_apply_vignette (source, curve, 255, task))
	{
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), source);
	}

	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_BLUE]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_GREEN]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_RED]);
	g_object_unref (curve[GTH_HISTOGRAM_CHANNEL_VALUE]);
	cairo_surface_destroy (source);
	cairo_surface_destroy (original);

	return NULL;
}


void
vintage_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, vintage_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Vintage"),
				    NULL);
}


/* -- Blurred Edges -- */


static gpointer
blurred_edges_exec (GthAsyncTask *task,
		    gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	cairo_surface_t *blurred;
	int              blurred_stride;
	double           center_x, center_y, min_radius, d, max_radius, max_distance;
	int              width;
	int              height;
	cairo_format_t   format;
	int              source_stride;
	cairo_surface_t *destination;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_blurred_line;
	int              x, y, temp;
	double           progress;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	unsigned char   *p_blurred;
	unsigned char    image_red, image_green, image_blue, image_alpha;
	unsigned char    layer_red, layer_green, layer_blue, layer_alpha;
	gboolean         cancelled = FALSE;

	gimp_op_init ();

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	blurred = _cairo_image_surface_copy (source);
	blurred_stride = cairo_image_surface_get_stride (blurred);
	if (! _cairo_image_surface_blur (blurred, 2, task)) {
		cairo_surface_destroy (blurred);
		cairo_surface_destroy (source);
		return NULL;
	}

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	format = cairo_image_surface_get_format (source);
	source_stride = cairo_image_surface_get_stride (source);

	center_x = width / 2.0;
	center_y = height / 2.0;
	min_radius = MIN (width, height) / 2.0;
	min_radius -= min_radius * 0.5;
	max_radius = MAX (width, height) / 2.0 /*sqrt (SQR (center_x) + SQR (center_y))*/;
	max_distance = max_radius - min_radius;

	destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (destination);

	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	p_blurred_line = _cairo_image_surface_flush_and_get_data (blurred);
	p_destination_line = _cairo_image_surface_flush_and_get_data (destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_blurred = p_blurred_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			d = sqrt (SQR (x - center_x) + SQR (y - center_y));
			d = CLAMP_PIXEL (d < min_radius ? 0 : d > max_radius ? 255 : (255.0 * ((d - min_radius) / max_distance)));

			CAIRO_GET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);

			/* blurred image layer with a radial mask (to blur the edges) */

			CAIRO_GET_RGBA (p_blurred, layer_red, layer_green, layer_blue, layer_alpha);
			layer_alpha = (guchar) (d);
			/*layer_red = layer_green = layer_blue = 0;*/
			p_destination[CAIRO_RED] = GIMP_OP_NORMAL (layer_red, image_red, layer_alpha);
			p_destination[CAIRO_GREEN] = GIMP_OP_NORMAL (layer_green, image_green, layer_alpha);
			p_destination[CAIRO_BLUE] = GIMP_OP_NORMAL (layer_blue, image_blue, layer_alpha);
			p_destination[CAIRO_ALPHA] = 255;

			p_source += 4;
			p_blurred += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_blurred_line += blurred_stride;
		p_destination_line += destination_stride;
	}

	if (! cancelled) {
		cairo_surface_mark_dirty (destination);
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);
	}

	cairo_surface_destroy (destination);
	cairo_surface_destroy (blurred);
	cairo_surface_destroy (source);

	return NULL;
}


void
blurred_edges_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, blurred_edges_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter */
				    _("Blurred Edges"),
				    NULL);
}


/* -- Vignette -- */


static gpointer
vignette_exec (GthAsyncTask *task,
	       gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *destination;

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = _cairo_image_surface_copy (original);

	if (cairo_image_surface_apply_vignette (destination, NULL, 127, task))
			gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (original);

	return NULL;
}


void
vignette_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), NULL, vignette_exec, NULL, NULL, NULL),
				    /* Translators: this is the name of an image filter that produces darker edges */
				    _("Vignette"),
				    NULL);
}
