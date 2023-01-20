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
#include "cairo-scale.h"
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image-dragger.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define SIZE_TOO_BIG_FOR_SCALE_BILINEAR (3000 * 3000)
#define MAX_ZOOM_LEVEL_FOR_HIGH_QUALITY 3.0
#define FRAME_BORDER 15
#define DRAG_ICON_SIZE 128


/* Properties */
enum {
	PROP_0,
	PROP_SHOW_FRAME
};


struct _GthImageDraggerPrivate {
	GthImageViewer  *viewer;
	gboolean         scrollable;
	gboolean         show_frame;
	cairo_surface_t *scaled;
	double           scaled_zoom;
	GthTask         *scale_task;

	/* drag & drop */

	gboolean         dnd_source_enabled;
	GdkModifierType  dnd_start_button_mask;
	GtkTargetList   *dnd_target_list;
	GdkDragAction    dnd_actions;
	gboolean         dnd_started;
	gboolean         dnd_began;
};


static void gth_image_dragger_gth_image_tool_interface_init (GthImageViewerToolInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthImageDragger,
			 gth_image_dragger,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImageDragger)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_IMAGE_VIEWER_TOOL,
						gth_image_dragger_gth_image_tool_interface_init))


static void
gth_image_dragger_finalize (GObject *object)
{
	GthImageDragger *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_DRAGGER (object));

	self = GTH_IMAGE_DRAGGER (object);
	_cairo_clear_surface (&self->priv->scaled);
	if (self->priv->scale_task != NULL)
		gth_task_cancel (self->priv->scale_task);
	if (self->priv->dnd_target_list != NULL) {
		gtk_target_list_unref (self->priv->dnd_target_list);
		self->priv->dnd_target_list = NULL;
	}

	/* Chain up */
	G_OBJECT_CLASS (gth_image_dragger_parent_class)->finalize (object);
}


static void
gth_image_dragger_set_property (GObject      *object,
				guint         property_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	GthImageDragger *self;

        self = GTH_IMAGE_DRAGGER (object);

	switch (property_id) {
	case PROP_SHOW_FRAME:
		self->priv->show_frame = g_value_get_boolean (value);
		if (self->priv->viewer != NULL) {
			if (self->priv->show_frame)
				gth_image_viewer_show_frame (self->priv->viewer, FRAME_BORDER);
			else
				gth_image_viewer_hide_frame (self->priv->viewer);
		}
		break;
	default:
		break;
	}
}


static void
gth_image_dragger_get_property (GObject    *object,
				guint       property_id,
				GValue     *value,
				GParamSpec *pspec)
{
	GthImageDragger *self;

        self = GTH_IMAGE_DRAGGER (object);

	switch (property_id) {
	case PROP_SHOW_FRAME:
		g_value_set_boolean (value, self->priv->show_frame);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_image_dragger_class_init (GthImageDraggerClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_dragger_finalize;
	gobject_class->set_property = gth_image_dragger_set_property;
	gobject_class->get_property = gth_image_dragger_get_property;

	/* properties */

	g_object_class_install_property (gobject_class,
					 PROP_SHOW_FRAME,
					 g_param_spec_boolean ("show-frame",
							       "Show Frame",
							       "Whether to show a frame around the image",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
gth_image_dragger_init (GthImageDragger *dragger)
{
	dragger->priv = gth_image_dragger_get_instance_private (dragger);
	dragger->priv->scaled = NULL;
	dragger->priv->scaled_zoom = 0;
	dragger->priv->scale_task = NULL;
	dragger->priv->viewer = NULL;
	dragger->priv->scrollable = FALSE;
	dragger->priv->show_frame = FALSE;

	dragger->priv->dnd_source_enabled = FALSE;
	dragger->priv->dnd_start_button_mask = 0;
	dragger->priv->dnd_target_list = NULL;
	dragger->priv->dnd_actions = 0;
	dragger->priv->dnd_started = FALSE;
	dragger->priv->dnd_began = FALSE;
}


static void
gth_image_dragger_set_viewer (GthImageViewerTool *base,
			      GthImageViewer     *image_viewer)
{
	GthImageDragger *self = GTH_IMAGE_DRAGGER (base);

	self->priv->viewer = image_viewer;
	g_object_add_weak_pointer (G_OBJECT (image_viewer), (gpointer *) &self->priv->viewer);
	if (self->priv->show_frame)
		gth_image_viewer_show_frame (self->priv->viewer, FRAME_BORDER);
}


static void
gth_image_dragger_unset_viewer (GthImageViewerTool *base,
		       	        GthImageViewer     *image_viewer)
{
	GthImageDragger *self = GTH_IMAGE_DRAGGER (base);

	if ((self->priv->viewer != NULL) && self->priv->show_frame)
		gth_image_viewer_hide_frame (self->priv->viewer);
	g_object_remove_weak_pointer (G_OBJECT (image_viewer),  (gpointer *) &self->priv->viewer);
	self->priv->viewer = NULL;
}


static void
gth_image_dragger_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_unrealize (GthImageViewerTool *base)
{
	/* void */
}


static void
_gth_image_dragger_update_cursor (GthImageDragger *self)
{
	GdkCursor *cursor;

	g_return_if_fail (self->priv->viewer != NULL);

	cursor = _gdk_cursor_new_for_widget (GTK_WIDGET (self->priv->viewer), GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);

	g_object_unref (cursor);
}


static void
gth_image_dragger_size_allocate (GthImageViewerTool *base,
				 GtkAllocation      *allocation)
{
	GthImageDragger *self;
	GthImageViewer  *viewer;
	double           h_page_size;
	double           v_page_size;
	double           h_upper;
	double           v_upper;

	self = (GthImageDragger *) base;
	viewer = (GthImageViewer *) self->priv->viewer;

	h_page_size = gtk_adjustment_get_page_size (viewer->hadj);
	v_page_size = gtk_adjustment_get_page_size (viewer->vadj);
	h_upper = gtk_adjustment_get_upper (viewer->hadj);
	v_upper = gtk_adjustment_get_upper (viewer->vadj);

	self->priv->scrollable = (h_page_size > 0) && (v_page_size > 0) && ((h_upper > h_page_size) || (v_upper > v_page_size));
	if (gtk_widget_get_realized (GTK_WIDGET (viewer)))
		_gth_image_dragger_update_cursor (self);
}


static void
gth_image_dragger_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_draw (GthImageViewerTool *self,
		        cairo_t            *cr)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	gth_image_viewer_paint_background (viewer, cr);

	if (gth_image_viewer_get_current_image (viewer) == NULL)
		return;

	gth_image_viewer_paint_frame (viewer, cr);

	if (dragger->priv->scaled != NULL) {
		gth_image_viewer_paint_region (viewer,
					       cr,
					       dragger->priv->scaled,
					       viewer->image_area.x,
					       viewer->image_area.y,
					       &viewer->visible_area,
					       CAIRO_FILTER_FAST);
	}
	else {
		double zoom = gth_image_viewer_get_zoom (viewer);
		int    screen_scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (viewer));

		gth_image_viewer_paint_region (viewer,
					       cr,
					       gth_image_viewer_get_current_image (viewer),
					       viewer->image_area.x,
					       viewer->image_area.y,
					       &viewer->visible_area,
					       (gth_image_viewer_get_zoom_quality (viewer) == GTH_ZOOM_QUALITY_HIGH) && (zoom * screen_scale_factor > 1.0) && (zoom <= MAX_ZOOM_LEVEL_FOR_HIGH_QUALITY) ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_FAST);
	}

	gth_image_viewer_apply_painters (viewer, cr);
}


static gboolean
gth_image_dragger_button_press (GthImageViewerTool *self,
				GdkEventButton     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;
	GtkWidget       *widget;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;
	widget = GTK_WIDGET (viewer);

	if (dragger->priv->scrollable
		&& ! viewer->dragging
		&& ((event->button == 1) || (event->button == 2)))
	{
		GdkCursor     *cursor;
		GdkGrabStatus  retval;

		cursor = gdk_cursor_new_from_name (gtk_widget_get_display (widget), "grabbing");
		retval = gdk_seat_grab (gdk_device_get_seat (gdk_event_get_device ((GdkEvent *) event)),
					gtk_widget_get_window (widget),
					GDK_SEAT_CAPABILITY_ALL_POINTING,
					TRUE,
					cursor,
					(GdkEvent *) event,
					NULL,
					NULL);

		if (cursor != NULL)
			g_object_unref (cursor);

		if (retval != GDK_GRAB_SUCCESS)
			return FALSE;

		viewer->pressed = TRUE;
		viewer->dragging = TRUE;

		return TRUE;
	}

	if (dragger->priv->dnd_source_enabled
		&& ! viewer->dragging
		&& (event->button == 1)
		&& (event->type == GDK_BUTTON_PRESS)
		&& ! (event->state & GDK_CONTROL_MASK)
		&& ! (event->state & GDK_SHIFT_MASK))
	{
		viewer->pressed = TRUE;
		viewer->dragging = TRUE;
		dragger->priv->dnd_began = FALSE;
	}

	return FALSE;
}


static gboolean
gth_image_dragger_button_release (GthImageViewerTool *self,
				  GdkEventButton     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (dragger->priv->scrollable && viewer->dragging)
		gdk_seat_ungrab (gdk_device_get_seat (event->device));

	if (dragger->priv->dnd_source_enabled && viewer->dragging)
		dragger->priv->dnd_began = FALSE;

	return FALSE;
}


static gboolean
gth_image_dragger_motion_notify (GthImageViewerTool *self,
				 GdkEventMotion     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (dragger->priv->scrollable && viewer->dragging) {
		gth_image_viewer_scroll_to (viewer,
					    viewer->drag_x_start - event->x,
					    viewer->drag_y_start - event->y);
		return TRUE;
	}

	if (dragger->priv->dnd_source_enabled
		&& viewer->dragging
		&& ! dragger->priv->dnd_began
		&& gtk_drag_check_threshold (GTK_WIDGET (viewer),
					     viewer->drag_x_start,
					     viewer->drag_y_start,
					     event->x,
					     event->y))
	{
		GdkDragContext  *context;
		cairo_surface_t *image;

		context = gtk_drag_begin_with_coordinates (GTK_WIDGET (viewer),
							   dragger->priv->dnd_target_list,
							   dragger->priv->dnd_actions,
							   1,
							   (GdkEvent *) event,
							   viewer->drag_x_start,
							   viewer->drag_y_start);

		image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
		if (image != NULL) {
			cairo_surface_t *icon = _cairo_create_dnd_icon (image, DRAG_ICON_SIZE, ITEM_STYLE_IMAGE, FALSE);
			gtk_drag_set_icon_surface (context, icon);
			cairo_surface_destroy (icon);
		}

		dragger->priv->dnd_began = TRUE;

		return TRUE;
	}

	return FALSE;
}


/* -- _gth_image_dragger_scale -- */


typedef struct {
	GthImageDragger *dragger;
	cairo_surface_t *image;
	int              new_width;
	int              new_height;
	cairo_surface_t *scaled;
	scale_filter_t   filter;
} ScaleData;


static void
scale_data_free (ScaleData *scale_data)
{
	if (scale_data == NULL)
		return;
	cairo_surface_destroy (scale_data->image);
	cairo_surface_destroy (scale_data->scaled);
	g_object_unref (scale_data->dragger);
	g_free (scale_data);
}


static gpointer
_gth_image_dragger_scale_exec (GthAsyncTask *task,
			       gpointer      user_data)
{
	ScaleData *scale_data = user_data;

	scale_data->scaled = _cairo_image_surface_scale (scale_data->image,
							 scale_data->new_width,
							 scale_data->new_height,
							 scale_data->filter,
							 task);

	return NULL;
}


static void
_gth_image_dragger_scale_after (GthAsyncTask *task,
				GError       *error,
				gpointer      user_data)
{
	ScaleData *scale_data = user_data;

	if (error == NULL) {
		GthImageDragger *dragger = scale_data->dragger;

		if ((scale_data->scaled != NULL) && (dragger->priv->viewer != NULL)) {
			_cairo_clear_surface (&dragger->priv->scaled);
			dragger->priv->scaled = cairo_surface_reference (scale_data->scaled);
			if (dragger->priv->viewer != NULL)
				gtk_widget_queue_draw (GTK_WIDGET (dragger->priv->viewer));
		}

		if (GTH_TASK (task) == dragger->priv->scale_task)
			dragger->priv->scale_task = NULL;
	}

	g_object_unref (task);
}


static void
_gth_image_dragger_create_scaled_high_quality (GthImageDragger *self,
					       cairo_surface_t *image,
					       int              new_width,
					       int              new_height,
					       scale_filter_t   filter)
{
	ScaleData *scale_data;

	if (self->priv->scale_task != NULL)
		gth_task_cancel (self->priv->scale_task);

	scale_data = g_new0 (ScaleData, 1);
	scale_data->dragger = g_object_ref (self);
	scale_data->image = cairo_surface_reference (image);
	scale_data->new_width = new_width;
	scale_data->new_height = new_height;
	scale_data->scaled = NULL;
	scale_data->filter = filter;

	self->priv->scale_task = gth_async_task_new (NULL,
						     _gth_image_dragger_scale_exec,
						     _gth_image_dragger_scale_after,
						     scale_data,
						     (GDestroyNotify) scale_data_free);

	gth_task_exec (self->priv->scale_task, NULL);
}


static void
_gth_image_dragger_update_scaled_image (GthImageDragger *self)
{
	cairo_surface_t *image;
	double           zoom_from_original;
	double           zoom_from_image;
	int              image_width, image_height;
	int              original_width, original_height;
	int              new_width, new_height;
	int              screen_scale_factor;
	scale_filter_t   filter;

	self->priv->scaled_zoom = 0;
	_cairo_clear_surface (&self->priv->scaled);

	if (self->priv->scale_task != NULL) {
		gth_task_cancel (self->priv->scale_task);
		self->priv->scale_task = NULL;
	}

	if (self->priv->viewer == NULL)
		return;

	if (gth_image_viewer_is_animation (self->priv->viewer)) {
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
		return;
	}

	image = gth_image_viewer_get_current_image (self->priv->viewer);
	if ((image == NULL) || (cairo_surface_status (image) != CAIRO_STATUS_SUCCESS)) {
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
		return;
	}

	screen_scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (self->priv->viewer));

	if (gth_image_viewer_get_zoom_quality_filter (self->priv->viewer) == CAIRO_FILTER_FAST) {
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
		return;
	}

	zoom_from_original = gth_image_viewer_get_zoom (self->priv->viewer);
	gth_image_viewer_get_original_size (self->priv->viewer, &original_width, &original_height);
	new_width = zoom_from_original * original_width * screen_scale_factor + 0.5;
	new_height = zoom_from_original * original_height * screen_scale_factor + 0.5;
	image_width = cairo_image_surface_get_width (image);
	image_height = cairo_image_surface_get_height (image);
	zoom_from_image = (double) new_width / image_width;

	if (zoom_from_image >= 1.0) {
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
		return;
	}

	if (image_width * image_height <= SIZE_TOO_BIG_FOR_SCALE_BILINEAR)
		filter = SCALE_FILTER_TRIANGLE;
	else
		filter = SCALE_FILTER_BOX;

	self->priv->scaled_zoom = zoom_from_original;
	_gth_image_dragger_create_scaled_high_quality (self, image, new_width, new_height, filter);
}


static void
gth_image_dragger_image_changed (GthImageViewerTool *base)
{
	_gth_image_dragger_update_scaled_image (GTH_IMAGE_DRAGGER (base));
}


static void
gth_image_dragger_zoom_changed (GthImageViewerTool *base)
{
	GthImageDragger *self = GTH_IMAGE_DRAGGER (base);

	if ((self->priv->viewer == NULL)
		|| (gth_image_viewer_get_zoom_quality (self->priv->viewer) == GTH_ZOOM_QUALITY_LOW)
		|| (self->priv->scaled_zoom != gth_image_viewer_get_zoom (self->priv->viewer)))
	{
		_gth_image_dragger_update_scaled_image (self);
	}
}


static void
gth_image_dragger_gth_image_tool_interface_init (GthImageViewerToolInterface *iface)
{
	iface->set_viewer = gth_image_dragger_set_viewer;
	iface->unset_viewer = gth_image_dragger_unset_viewer;
	iface->realize = gth_image_dragger_realize;
	iface->unrealize = gth_image_dragger_unrealize;
	iface->size_allocate = gth_image_dragger_size_allocate;
	iface->map = gth_image_dragger_map;
	iface->unmap = gth_image_dragger_unmap;
	iface->draw = gth_image_dragger_draw;
	iface->button_press = gth_image_dragger_button_press;
	iface->button_release = gth_image_dragger_button_release;
	iface->motion_notify = gth_image_dragger_motion_notify;
	iface->image_changed = gth_image_dragger_image_changed;
	iface->zoom_changed = gth_image_dragger_zoom_changed;
}


GthImageViewerTool *
gth_image_dragger_new (gboolean show_frame)
{
	return (GthImageViewerTool *) g_object_new (GTH_TYPE_IMAGE_DRAGGER,
						    "show-frame", show_frame,
						    NULL);
}


void
gth_image_dragger_enable_drag_source (GthImageDragger      *self,
				      GdkModifierType       start_button_mask,
				      const GtkTargetEntry *targets,
				      int                   n_targets,
				      GdkDragAction         actions)
{
	if (self->priv->dnd_target_list != NULL)
		gtk_target_list_unref (self->priv->dnd_target_list);

	self->priv->dnd_source_enabled = TRUE;
	self->priv->dnd_start_button_mask = start_button_mask;
	self->priv->dnd_target_list = gtk_target_list_new (targets, n_targets);
	self->priv->dnd_actions = actions;
}

void
gth_image_dragger_disable_drag_source (GthImageDragger      *self)
{
	self->priv->dnd_source_enabled = FALSE;
}
