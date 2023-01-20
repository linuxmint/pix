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
#include "gth-curve-editor.h"
#include "gth-curve-preset.h"
#include "gth-curve-preset-editor-dialog.h"
#include "gth-file-tool-curves.h"
#include "gth-curve.h"
#include "gth-points.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


struct _GthFileToolCurvesPrivate {
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GthTask            *image_task;
	guint               apply_event;
	GthImageViewerTool *preview_tool;
	GthHistogram       *histogram;
	gboolean            view_original;
	gboolean            apply_to_original;
	gboolean            closing;
	gboolean            apply_current_curve;
	GtkWidget          *curve_editor;
	GtkWidget          *preview_button;
	GtkWidget          *preview_channel_button;
	GtkWidget          *stack;
	GtkWidget          *show_presets_button;
	GtkWidget          *reset_button;
	GtkWidget          *add_preset_button;
	GthCurvePreset     *preset;
	GtkWidget          *filter_grid;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolCurves,
			 gth_file_tool_curves,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolCurves))


/* -- apply_changes -- */


typedef struct {
	long     *value_map[GTH_HISTOGRAM_N_CHANNELS];
	GthCurve *curve[GTH_HISTOGRAM_N_CHANNELS];
	int       current_curve;
	gboolean  apply_current_curve;
} TaskData;


static void
curves_setup (TaskData        *task_data,
	      cairo_surface_t *source)
{
	int c, v;

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++) {
		task_data->value_map[c] = g_new (long, 256);
		for (v = 0; v <= 255; v++) {
			double u;

			if ((c != task_data->current_curve) || task_data->apply_current_curve)
				u = gth_curve_eval (task_data->curve[c], v);
			else
				u = v;

			if (c > GTH_HISTOGRAM_CHANNEL_VALUE)
				u = task_data->value_map[GTH_HISTOGRAM_CHANNEL_VALUE][(int)u];

			task_data->value_map[c][v] = u;
		}
	}
}


static inline guchar
value_func (TaskData *task_data,
	    int       n_channel,
	    guchar    value)
{
	return (guchar) task_data->value_map[n_channel][value];
}


static gpointer
curves_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	TaskData        *task_data = user_data;
	cairo_surface_t *source;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	cairo_surface_t *destination;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	gboolean         cancelled;
	double           progress;
	int              x, y, temp;
	unsigned char    red, green, blue, alpha;

	/* initialize the extra data */

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	curves_setup (task_data, source);

	/* convert the image */

	format = cairo_image_surface_get_format (source);
	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (destination);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	p_destination_line = _cairo_image_surface_flush_and_get_data (destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled) {
			cairo_surface_destroy (destination);
			cairo_surface_destroy (source);
			return NULL;
		}

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, red, green, blue, alpha);
			red   = value_func (task_data, GTH_HISTOGRAM_CHANNEL_RED, red);
			green = value_func (task_data, GTH_HISTOGRAM_CHANNEL_GREEN, green);
			blue  = value_func (task_data, GTH_HISTOGRAM_CHANNEL_BLUE, blue);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (destination);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void apply_changes (GthFileToolCurves *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolCurves *self = user_data;
	GthImage	  *destination_image;

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


static TaskData *
task_data_new (GthPoints *points)
{
	TaskData *task_data;
	int       c;

	task_data = g_new (TaskData, 1);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		task_data->value_map[c] = NULL;
		task_data->curve[c] = gth_curve_new (GTH_TYPE_BEZIER, points + c);
	}

	return task_data;
}


static void
task_data_destroy (gpointer user_data)
{
	TaskData *task_data = user_data;
	int       c;

	if (task_data == NULL)
		return;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_object_unref (task_data->curve[c]);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_free (task_data->value_map[c]);
	g_free (task_data);
}


static GthTask *
get_curves_task (GthPoints *points,
		 int        current_curve,
		 gboolean   apply_current_curve)
{
	TaskData *task_data;

	task_data = task_data_new (points);
	task_data->current_curve = current_curve;
	task_data->apply_current_curve = apply_current_curve;

	return  gth_image_task_new (_("Applying changes"),
				    NULL,
				    curves_exec,
				    NULL,
				    task_data,
				    task_data_destroy);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolCurves *self = user_data;
	GtkWidget         *window;
	GthPoints          points[GTH_HISTOGRAM_N_CHANNELS];

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return FALSE;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	gth_points_array_init (points);
	gth_curve_editor_get_points (GTH_CURVE_EDITOR (self->priv->curve_editor), points);
	self->priv->image_task = get_curves_task (points,
						  gth_curve_editor_get_current_channel (GTH_CURVE_EDITOR (self->priv->curve_editor)),
						  self->priv->apply_current_curve);
	gth_points_array_dispose (points);

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
apply_changes (GthFileToolCurves *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
reset_button_clicked_cb (GtkButton *button,
		  	 gpointer   user_data)
{
	GthFileToolCurves *self = user_data;
	gth_curve_editor_reset (GTH_CURVE_EDITOR (self->priv->curve_editor));
}


static void
add_to_presets_dialog_response_cb (GtkDialog *dialog,
				   int        response,
				   gpointer   user_data)
{
	GthFileToolCurves *self = user_data;
	char              *name;
	GthPoints          points[GTH_HISTOGRAM_N_CHANNELS];

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	name = gth_request_dialog_get_normalized_text (GTH_REQUEST_DIALOG (dialog));
	if (_g_utf8_all_spaces (name)) {
		g_free (name);
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("No name specified"));
		return;
	}

	if (g_regex_match_simple ("/", name, 0, 0)) {
		char *message;

		message = g_strdup_printf (_("Invalid name. The following characters are not allowed: %s"), "/");
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, message);

		g_free (message);
		g_free (name);

		return;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));

	gth_points_array_init (points);
	gth_curve_editor_get_points (GTH_CURVE_EDITOR (self->priv->curve_editor), points);
	gth_curve_preset_add (self->priv->preset, name, points);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->show_presets_button), TRUE);

	gth_points_array_dispose (points);
	g_free (name);
}


static void
add_to_presets_button_clicked_cb (GtkButton *button,
				  gpointer   user_data)
{
	GthFileToolCurves *self = user_data;
	GtkWidget         *dialog;

	dialog = gth_request_dialog_new (GTK_WINDOW (gth_file_tool_get_window (GTH_FILE_TOOL (self))),
					 GTK_DIALOG_MODAL,
					 _("Add to Presets"),
					 _("Enter the preset name:"),
					 _GTK_LABEL_CANCEL,
					 _GTK_LABEL_SAVE);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (add_to_presets_dialog_response_cb),
			  self);

	gtk_window_present (GTK_WINDOW (dialog));
}


static void
curve_editor_changed_cb (GthCurveEditor *curve_editor,
			 gpointer        user_data)
{
	GthFileToolCurves *self = user_data;

	apply_changes (self);
	if (g_strcmp0 (gtk_stack_get_visible_child_name (GTK_STACK (self->priv->stack)), "presets") != 0)
		gth_filter_grid_activate (GTH_FILTER_GRID (self->priv->filter_grid), GTH_FILTER_GRID_NO_FILTER);
}


static void
curve_editor_current_channel_changed_cb (GObject    *gobject,
					 GParamSpec *pspec,
					 gpointer    user_data)
{
	GthFileToolCurves *self = user_data;

	if (! self->priv->apply_current_curve)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->preview_channel_button), TRUE);
}


static void
_gth_file_tool_curves_set_view_original (GthFileToolCurves *self,
					 gboolean           view_original,
					 gboolean           update_image)
{
	self->priv->view_original = view_original;

	_g_signal_handlers_block_by_data (self->priv->preview_button, self);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->preview_button), ! self->priv->view_original);
	_g_signal_handlers_unblock_by_data (self->priv->preview_button, self);

	gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->priv->preview_channel_button), self->priv->view_original);
	gtk_widget_set_sensitive (self->priv->preview_channel_button, ! self->priv->view_original);

	if (update_image) {
		if (self->priv->view_original)
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
		else
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}
}


static void
preview_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
				gpointer         user_data)
{
	GthFileToolCurves *self = user_data;
	_gth_file_tool_curves_set_view_original (self, ! gtk_toggle_button_get_active (togglebutton), TRUE);
}


static void
preview_channel_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
	GthFileToolCurves *self = user_data;

	self->priv->apply_current_curve = gtk_toggle_button_get_active (togglebutton);
	apply_changes (self);
}


static void
show_options_button_clicked_cb (GtkButton *button,
				gpointer   user_data)
{
	GthFileToolCurves *self = user_data;

	gtk_stack_set_visible_child_name (GTK_STACK (self->priv->stack), "options");

	g_signal_handlers_block_matched (self->priv->show_presets_button, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->show_presets_button), FALSE);
	g_signal_handlers_unblock_matched (self->priv->show_presets_button, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);

	gtk_widget_set_visible (self->priv->reset_button, TRUE);
	gtk_widget_set_visible (self->priv->add_preset_button, TRUE);
}


static void
edit_presets_button_clicked_cb (GtkButton *button,
				gpointer   user_data)
{
	GthFileToolCurves *self = user_data;
	GtkWidget         *dialog;

	dialog = gth_curve_preset_editor_dialog_new (GTK_WINDOW (gth_file_tool_get_window (GTH_FILE_TOOL (self))), self->priv->preset);
	gtk_widget_show (dialog);
}


static void
preset_changed_cb (GthCurvePreset	*preset,
		   GthPresetAction	 action,
		   int			 preset_id,
		   gpointer		 user_data)
{
	GthFileToolCurves *self = user_data;
	const char        *preset_name;
	GthPoints         *points;
	GError            *error = NULL;
	GList		  *order;

	if (! gth_curve_preset_save (self->priv->preset, &error)) {
		_gtk_error_dialog_from_gerror_show (NULL, _("Could not save the file"), error);
		g_clear_error (&error);
		return;
	}

	switch (action) {
	case GTH_PRESET_ACTION_ADDED:
		if (gth_curve_preset_get_by_id (preset, preset_id, &preset_name, &points)) {
			gth_filter_grid_add_filter (GTH_FILTER_GRID (self->priv->filter_grid),
						    preset_id,
						    get_curves_task (points, 0, TRUE),
						    preset_name,
						    NULL);
			gth_filter_grid_generate_preview (GTH_FILTER_GRID (self->priv->filter_grid),
							  preset_id,
							  self->priv->preview);
		}
		break;
	case GTH_PRESET_ACTION_REMOVED:
		gth_filter_grid_remove_filter (GTH_FILTER_GRID (self->priv->filter_grid), preset_id);
		break;
	case GTH_PRESET_ACTION_RENAMED:
		if (gth_curve_preset_get_by_id (preset, preset_id, &preset_name, NULL))
			gth_filter_grid_rename_filter (GTH_FILTER_GRID (self->priv->filter_grid), preset_id, preset_name);
		break;
	case GTH_PRESET_ACTION_CHANGED_ORDER:
		order = gth_curve_preset_get_order (preset);
		gth_filter_grid_change_order (GTH_FILTER_GRID (self->priv->filter_grid), order);
		g_list_free (order);
		break;
	}
}


static void
filter_grid_activated_cb (GthFilterGrid	*filter_grid,
			  int            filter_id,
			  gpointer       user_data)
{
	GthFileToolCurves *self = user_data;

	_gth_file_tool_curves_set_view_original (self, FALSE, FALSE);

	if (filter_id == GTH_FILTER_GRID_NO_FILTER) {
		if (g_strcmp0 (gtk_stack_get_visible_child_name (GTK_STACK (self->priv->stack)), "presets") == 0) {
			GthPoints points[GTH_HISTOGRAM_N_CHANNELS];
			int       c;

			/* reset the curve */

			for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
				GthPoints *p = points + c;
				gth_points_init (p, 2);
				gth_points_set_point (p, 0, 0, 0);
				gth_points_set_point (p, 1, 255, 255);
			}
			gth_curve_editor_set_points (GTH_CURVE_EDITOR (self->priv->curve_editor), points);
			gth_points_array_dispose (points);
		}
	}
	else {
		GthPoints *points;

		if (gth_curve_preset_get_by_id (GTH_CURVE_PRESET (self->priv->preset), filter_id, NULL, &points))
			gth_curve_editor_set_points (GTH_CURVE_EDITOR (self->priv->curve_editor), points);
	}
}


static GtkWidget *
gth_file_tool_curves_get_options (GthFileTool *base)
{
	GthFileToolCurves *self;
	GthViewerPage     *viewer_page;
	GtkWidget         *viewer;
	cairo_surface_t   *source;
	GtkWidget         *options;
	int                width, height;
	GtkAllocation      allocation;
	GtkWidget         *container;

	self = (GthFileToolCurves *) base;

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (viewer_page == NULL)
		return NULL;

	_cairo_clear_surface (&self->priv->destination);
	_cairo_clear_surface (&self->priv->preview);

	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	if (scale_keeping_ratio (&width, &height, PREVIEW_SIZE * allocation.width, PREVIEW_SIZE * allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_fast (source, width, height);
	else
		self->priv->preview = cairo_surface_reference (source);

	self->priv->destination = cairo_surface_reference (self->priv->preview);
	self->priv->apply_to_original = FALSE;
	self->priv->view_original = FALSE;
	self->priv->closing = FALSE;

	container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	self->priv->stack = gtk_stack_new ();
	gtk_stack_set_transition_type (GTK_STACK (self->priv->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
	gtk_box_pack_start (GTK_BOX (container), self->priv->stack, FALSE, FALSE, 0);
	gtk_widget_show (self->priv->stack);

	self->priv->builder = _gtk_builder_new_from_file ("curves-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);
	gtk_stack_add_named (GTK_STACK (self->priv->stack), options, "options");

	self->priv->curve_editor = gth_curve_editor_new (self->priv->histogram);
	gtk_widget_show (self->priv->curve_editor);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("curves_box")), self->priv->curve_editor, TRUE, TRUE, 0);

	g_signal_connect (self->priv->curve_editor,
			  "changed",
			  G_CALLBACK (curve_editor_changed_cb),
			  self);
	g_signal_connect (self->priv->curve_editor,
			  "notify::current-channel",
			  G_CALLBACK (curve_editor_current_channel_changed_cb),
			  self);

	self->priv->preview_button = GET_WIDGET ("preview_checkbutton");
	g_signal_connect (self->priv->preview_button,
			  "toggled",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
			  self);

	self->priv->preview_channel_button = GET_WIDGET ("preview_channel_checkbutton");
	g_signal_connect (self->priv->preview_channel_button,
			  "toggled",
			  G_CALLBACK (preview_channel_checkbutton_toggled_cb),
			  self);

	{
		GtkWidget *header_bar;
		GtkWidget *button;
		GFile     *file;
		int        i;
		GtkWidget *presets;

		header_bar = gtk_header_bar_new ();
		gtk_header_bar_set_title (GTK_HEADER_BAR (header_bar), _("Presets"));

		button = gtk_button_new_from_icon_name ("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
		g_signal_connect (button,
				  "clicked",
				  G_CALLBACK (show_options_button_clicked_cb),
				  self);
		gtk_widget_show (button);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), button);

		button = gtk_button_new_from_icon_name ("edit-symbolic", GTK_ICON_SIZE_BUTTON);
		g_signal_connect (button,
				  "clicked",
				  G_CALLBACK (edit_presets_button_clicked_cb),
				  self);
		gtk_widget_show (button);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), button);

		gtk_widget_show (header_bar);

		file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, "gthumb", "curves.xml", NULL);
		self->priv->preset = gth_curve_preset_new_from_file (file);
		g_object_unref (file);

		g_signal_connect (self->priv->preset,
				  "preset_changed",
				  G_CALLBACK (preset_changed_cb),
				  self);

		self->priv->filter_grid = gth_filter_grid_new ();
		for (i = 0; i < gth_curve_preset_get_size (self->priv->preset); i++) {
			GthPoints  *points;
			const char *name;
			int         id;

			if (gth_curve_preset_get_nth (self->priv->preset, i, &id, &name, &points))
				gth_filter_grid_add_filter (GTH_FILTER_GRID (self->priv->filter_grid),
						            id,
							    get_curves_task (points, 0, TRUE),
							    name,
							    NULL);
		}

		g_signal_connect (self->priv->filter_grid,
				  "activated",
				  G_CALLBACK (filter_grid_activated_cb),
				  self);

		gtk_widget_show (self->priv->filter_grid);

		presets = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
		gtk_box_pack_start (GTK_BOX (presets), header_bar, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (presets), self->priv->filter_grid, FALSE, FALSE, 0);
		gtk_widget_show (presets);
		gtk_stack_add_named (GTK_STACK (self->priv->stack), presets, "presets");

		gth_filter_grid_generate_previews (GTH_FILTER_GRID (self->priv->filter_grid), self->priv->preview);
	}

	gtk_stack_set_visible_child_name (GTK_STACK (self->priv->stack), "options");
	gtk_widget_show_all (container);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	gth_histogram_calculate_for_image (self->priv->histogram, self->priv->preview);
	apply_changes (self);

	return container;
}


static void
gth_file_tool_curves_destroy_options (GthFileTool *base)
{
	GthFileToolCurves *self;
	GthViewerPage     *viewer_page;

	self = (GthFileToolCurves *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_viewer_page_update_sensitivity (viewer_page);

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);
	_g_clear_object (&self->priv->builder);
}


static void
gth_file_tool_curves_apply_options (GthFileTool *base)
{
	GthFileToolCurves *self;

	self = (GthFileToolCurves *) base;
	self->priv->apply_to_original = TRUE;
	apply_changes (self);
}


static void
presets_toggled_cb (GtkToggleButton   *button,
		    GthFileToolCurves * self)
{
	gboolean show_presets;

	show_presets = gtk_toggle_button_get_active (button);
	gtk_stack_set_visible_child_name (GTK_STACK (self->priv->stack), show_presets ? "presets" : "options");
	gtk_widget_set_visible (self->priv->reset_button, ! show_presets);
	gtk_widget_set_visible (self->priv->add_preset_button, ! show_presets);
}


static void
gth_file_tool_curves_populate_headerbar (GthFileTool *base,
					 GthBrowser  *browser)
{
	GthFileToolCurves *self;
	GtkWidget         *button;

	self = (GthFileToolCurves *) base;

	/* reset button */

	button = gth_browser_add_header_bar_button (browser,
						    GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
						    "edit-undo-symbolic",
						    _("Reset"),
						    NULL,
						    NULL);
	self->priv->reset_button = button;
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (reset_button_clicked_cb),
			  self);

	/* add to presets */

	button = gth_browser_add_header_bar_button (browser,
						    GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
						    "list-add-symbolic",
						    _("Add to presets"),
						    NULL,
						    NULL);
	self->priv->add_preset_button = button;
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (add_to_presets_button_clicked_cb),
			  self);

	/* presets */

	button = gth_browser_add_header_bar_toggle_button (browser,
							   GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
							   "presets-symbolic",
							   _("Presets"),
							   NULL,
							   NULL);
	gtk_widget_set_margin_start (button, GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
	self->priv->show_presets_button = button;
	g_signal_connect (button,
			  "toggled",
			  G_CALLBACK (presets_toggled_cb),
			  self);
}


static void
gth_file_tool_sharpen_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolCurves *self = (GthFileToolCurves *) base;

	if (self->priv->image_task != NULL) {
		self->priv->closing = TRUE;
		gth_task_cancel (self->priv->image_task);
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
gth_file_tool_curves_init (GthFileToolCurves *self)
{
	self->priv = gth_file_tool_curves_get_instance_private (self);
	self->priv->preview = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->image_task = NULL;
	self->priv->view_original = FALSE;
	self->priv->apply_current_curve = TRUE;
	self->priv->histogram = gth_histogram_new ();

	gth_file_tool_construct (GTH_FILE_TOOL (self), "curves-symbolic", _("Color Curves"), GTH_TOOLBOX_SECTION_COLORS);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Adjust color curves"));
}


static void
gth_file_tool_curves_finalize (GObject *object)
{
	GthFileToolCurves *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_CURVES (object));

	self = (GthFileToolCurves *) object;

	cairo_surface_destroy (self->priv->preview);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->histogram);

	G_OBJECT_CLASS (gth_file_tool_curves_parent_class)->finalize (object);
}


static void
gth_file_tool_curves_class_init (GthFileToolCurvesClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_curves_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_curves_get_options;
	file_tool_class->destroy_options = gth_file_tool_curves_destroy_options;
	file_tool_class->apply_options = gth_file_tool_curves_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_curves_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_sharpen_reset_image;
}
