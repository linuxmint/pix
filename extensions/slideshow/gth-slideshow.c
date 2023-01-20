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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifdef HAVE_CLUTTER
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#endif /* HAVE_CLUTTER */
#if HAVE_GSTREAMER
#include <gst/gst.h>
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#endif /* HAVE_GSTREAMER */
#include "gth-slideshow.h"
#include "gth-transition.h"
#include "actions.h"

#define HIDE_CURSOR_DELAY 1
#define HIDE_PAUSED_SIGN_DELAY 1
#define DEFAULT_DELAY 2000
#define _GST_PLAY_FLAG_AUDIO (1 << 1)


typedef enum {
	GTH_SLIDESHOW_DIRECTION_FORWARD,
	GTH_SLIDESHOW_DIRECTION_BACKWARD
} GthSlideshowDirection;


struct _GthSlideshowPrivate {
	GthProjector          *projector;
	GthBrowser            *browser;
	GList                 *file_list; /* GthFileData */
	gboolean               automatic;
	gboolean               wrap_around;
	GList                 *current;
	GthImagePreloader     *preloader;
	GList                 *transitions; /* GthTransition */
	int                    n_transitions;
	GthTransition         *transition;
	GthSlideshowDirection  direction;
#if HAVE_CLUTTER
	ClutterTimeline       *timeline;
	ClutterActor          *image1;
	ClutterActor          *image2;
	ClutterActor          *paused_actor;
	guint32                last_button_event_time;
#endif
	GthImage              *current_image;
	GtkWidget             *viewer;
	guint                  next_event;
	guint                  delay;
	guint                  hide_cursor_event;
	GRand                 *rand;
	gboolean               first_show;
	gboolean               one_loaded;
	char                 **audio_files;
	gboolean               audio_loop;
#if HAVE_GSTREAMER
	int                    current_audio_file;
	GstElement            *playbin;
#endif
	GdkPixbuf             *pause_pixbuf;
	gboolean               paused;
	gboolean               paint_paused;
	guint                  hide_paused_sign;
	gboolean               animating;
	gboolean               random_order;
	GthScreensaver        *screensaver;
};


G_DEFINE_TYPE_WITH_CODE (GthSlideshow,
			 gth_slideshow,
			 GTH_TYPE_WINDOW,
			 G_ADD_PRIVATE (GthSlideshow))


static const GActionEntry actions[] = {
	{ "slideshow-close", gth_slideshow_activate_close },
	{ "slideshow-toggle-pause", gth_slideshow_activate_toggle_pause },
	{ "slideshow-next-image", gth_slideshow_activate_next_image },
	{ "slideshow-previous-image", gth_slideshow_activate_previous_image },
};


static int
shuffle_func (gconstpointer a,
	      gconstpointer b)
{
        return g_random_int_range (-1, 2);
}



static void
_gth_slideshow_reset_current (GthSlideshow *self)
{
	if (self->priv->random_order)
		self->priv->file_list = g_list_sort (self->priv->file_list, shuffle_func);

	if (self->priv->direction == GTH_SLIDESHOW_DIRECTION_FORWARD)
		self->priv->current = g_list_first (self->priv->file_list);
	else
		self->priv->current = g_list_last (self->priv->file_list);
}


static void
preloader_load_ready_cb (GObject	*source_object,
			 GAsyncResult	*result,
			 gpointer	 user_data)
{
	GthSlideshow *self = user_data;
	GthFileData  *requested;
	GthImage     *image;
	int	      requested_size;
	int	      original_width;
	int	      original_height;
	GError	     *error = NULL;

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
		gth_slideshow_load_next_image (self);
		return;
	}

	_g_object_unref (self->priv->current_image);
	self->priv->current_image = _g_object_ref (image);

	if (self->priv->current_image == NULL) {
		gth_slideshow_load_next_image (self);
		return;
	}

	self->priv->one_loaded = TRUE;
	self->priv->projector->image_ready (self, self->priv->current_image);

	_g_object_unref (requested);
	_g_object_unref (image);
}


static void
_gth_slideshow_load_current_image (GthSlideshow *self)
{
	GthFileData *requested_file;
	GthFileData *next_file;
	GthFileData *prev_file;
	int          screen_width;
	int          screen_height;

	if (self->priv->next_event != 0) {
		g_source_remove (self->priv->next_event);
		self->priv->next_event = 0;
	}

	if (self->priv->current == NULL) {
		if (! self->priv->one_loaded || ! self->priv->wrap_around) {
			gth_slideshow_close (self);
			return;
		}
		_gth_slideshow_reset_current (self);
	}

	requested_file = (GthFileData *) self->priv->current->data;
	if (self->priv->current->next != NULL)
		next_file = (GthFileData *) self->priv->current->next->data;
	else
		next_file = NULL;
	if (self->priv->current->prev != NULL)
		prev_file = (GthFileData *) self->priv->current->prev->data;
	else
		prev_file = NULL;

	_gtk_widget_get_screen_size (GTK_WIDGET (self), &screen_width, &screen_height);
	gth_image_preloader_load (self->priv->preloader,
				  requested_file,
				  MAX (screen_width, screen_height),
				  NULL,
				  preloader_load_ready_cb,
				  self,
				  2,
				  next_file,
				  prev_file);
}


static gboolean
next_image_cb (gpointer user_data)
{
	GthSlideshow *self = user_data;

	if (self->priv->next_event != 0) {
		g_source_remove (self->priv->next_event);
		self->priv->next_event = 0;
	}
	gth_slideshow_load_next_image (self);

	return FALSE;
}


static void
view_next_image_automatically (GthSlideshow *self)
{
	if (self->priv->automatic && ! self->priv->paused)
		gth_screensaver_inhibit (self->priv->screensaver,
					 GTK_WINDOW (self),
					 _("Playing a presentation"));
	else
		gth_screensaver_uninhibit (self->priv->screensaver);

	if (self->priv->automatic) {
		if (self->priv->next_event != 0)
			g_source_remove (self->priv->next_event);
		self->priv->next_event = g_timeout_add (self->priv->delay, next_image_cb, self);
	}
}


static void
gth_slideshow_finalize (GObject *object)
{
	GthSlideshow *self = GTH_SLIDESHOW (object);

	if (self->priv->next_event != 0)
		g_source_remove (self->priv->next_event);
	if (self->priv->hide_cursor_event != 0)
		g_source_remove (self->priv->hide_cursor_event);

	_g_object_unref (self->priv->pause_pixbuf);
	_g_object_unref (self->priv->current_image);
	_g_object_list_unref (self->priv->file_list);
	_g_object_unref (self->priv->browser);
	_g_object_unref (self->priv->preloader);
	_g_object_list_unref (self->priv->transitions);
	g_rand_free (self->priv->rand);
	g_strfreev (self->priv->audio_files);

#if HAVE_GSTREAMER
	if (self->priv->playbin != NULL) {
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
		self->priv->playbin = NULL;
	}
#endif

	if (self->priv->screensaver != NULL) {
		gth_screensaver_uninhibit (self->priv->screensaver);
		g_object_unref (self->priv->screensaver);
	}

	G_OBJECT_CLASS (gth_slideshow_parent_class)->finalize (object);
}


static void
gth_slideshow_class_init (GthSlideshowClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gth_slideshow_finalize;
}


static gboolean
hide_cursor_cb (gpointer data)
{
	GthSlideshow *self = data;

	g_source_remove (self->priv->hide_cursor_event);
	self->priv->hide_cursor_event = 0;
	self->priv->projector->hide_cursor (self);

	return FALSE;
}


#if HAVE_GSTREAMER


static gboolean
player_done_cb (gpointer user_data)
{
	GthSlideshow *self = user_data;

	self->priv->current_audio_file++;
	if ((self->priv->audio_files[self->priv->current_audio_file] == NULL)
	    && self->priv->audio_loop)
	{
		self->priv->current_audio_file = 0;
	}
	gst_element_set_state (self->priv->playbin, GST_STATE_READY);
	g_object_set (G_OBJECT (self->priv->playbin), "uri", self->priv->audio_files[self->priv->current_audio_file], NULL);
	gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);

	return FALSE;
}


static void
pipeline_eos_cb (GstBus     *bus,
                 GstMessage *message,
                 gpointer    user_data)
{
	g_idle_add (player_done_cb, user_data);
}


#endif


static void
gth_slideshow_show_cb (GtkWidget    *widget,
		       GthSlideshow *self)
{
	if (! self->priv->first_show)
		return;

	self->priv->first_show = FALSE;

#if HAVE_GSTREAMER
	if ((self->priv->audio_files != NULL)
	    && (self->priv->audio_files[0] != NULL)
	    && gstreamer_init ())
	{
		self->priv->current_audio_file = 0;
		if (self->priv->playbin == NULL) {
			GstBus *bus;

			self->priv->playbin = gst_element_factory_make ("playbin", "playbin");
			g_object_set (self->priv->playbin,
				      "flags", _GST_PLAY_FLAG_AUDIO,
				      "volume", 1.0,
				      NULL);
			bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->playbin));
			gst_bus_add_signal_watch (bus);
			g_signal_connect (bus, "message::eos", G_CALLBACK (pipeline_eos_cb), self);
		}
		else
			gst_element_set_state (self->priv->playbin, GST_STATE_READY);
		g_object_set (G_OBJECT (self->priv->playbin), "uri", self->priv->audio_files[self->priv->current_audio_file], NULL);
		gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
	}
#endif

	_gth_slideshow_reset_current (self);
	_gth_slideshow_load_current_image (self);
}


static gboolean
_gth_slideshow_key_press_cb (GthSlideshow *self,
			     GdkEventKey  *event,
			     gpointer      user_data)
{
	return gth_window_activate_shortcut (GTH_WINDOW (self),
					     GTH_SHORTCUT_CONTEXT_SLIDESHOW,
					     NULL,
					     event->keyval,
					     event->state);
}


static void
gth_slideshow_init (GthSlideshow *self)
{
	self->priv = gth_slideshow_get_instance_private (self);
	self->priv->file_list = NULL;
	self->priv->next_event = 0;
	self->priv->delay = DEFAULT_DELAY;
	self->priv->automatic = FALSE;
	self->priv->wrap_around = FALSE;
	self->priv->transitions = NULL;
	self->priv->n_transitions = 0;
	self->priv->rand = g_rand_new ();
	self->priv->first_show = TRUE;
	self->priv->audio_files = NULL;
	self->priv->paused = FALSE;
	self->priv->animating = FALSE;
	self->priv->direction = GTH_SLIDESHOW_DIRECTION_FORWARD;
	self->priv->random_order = FALSE;
	self->priv->current_image = NULL;
	self->priv->screensaver = gth_screensaver_new (NULL);
	self->priv->preloader = gth_image_preloader_new ();
}


static void
_gth_slideshow_construct (GthSlideshow *self,
			  GthProjector *projector,
			  GthBrowser   *browser,
			  GList        *file_list)
{
	self->priv->projector = projector;
	self->priv->browser = _g_object_ref (browser);
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->one_loaded = FALSE;
	self->priv->pause_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
							     "slideshow-pause",
							     100,
							     0,
							     NULL);
	if (self->priv->pause_pixbuf == NULL)
		self->priv->pause_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
								     "media-playback-pause-symbolic",
								     100,
								     0,
								     NULL);

	self->priv->projector->construct (self);

	g_action_map_add_action_entries (G_ACTION_MAP (self),
					 actions,
					 G_N_ELEMENTS (actions),
					 self);
	gth_window_copy_shortcuts (GTH_WINDOW (self),
				   GTH_WINDOW (self->priv->browser),
				   GTH_SHORTCUT_CONTEXT_SLIDESHOW);

	g_signal_connect (self, "show", G_CALLBACK (gth_slideshow_show_cb), self);

	g_signal_connect (self,
			  "key-press-event",
			  G_CALLBACK (_gth_slideshow_key_press_cb),
			  NULL);
}


GtkWidget *
gth_slideshow_new (GthProjector *projector,
		   GthBrowser   *browser,
		   GList        *file_list /* GthFileData */)
{
	GthSlideshow *window;

	g_return_val_if_fail (projector != NULL, NULL);

	window = (GthSlideshow *) g_object_new (GTH_TYPE_SLIDESHOW, NULL);
	_gth_slideshow_construct (window, projector, browser, file_list);

	return (GtkWidget*) window;
}


void
gth_slideshow_set_delay (GthSlideshow *self,
			 guint         msecs)
{
	self->priv->delay = msecs;
}


void
gth_slideshow_set_automatic (GthSlideshow *self,
			     gboolean      automatic)
{
	self->priv->automatic = automatic;
}


void
gth_slideshow_set_wrap_around (GthSlideshow *self,
			       gboolean      wrap_around)
{
	self->priv->wrap_around = wrap_around;
}


void
gth_slideshow_set_transitions (GthSlideshow *self,
			       GList        *transitions)
{
	_g_object_list_unref (self->priv->transitions);
	self->priv->transitions = _g_object_list_ref (transitions);
	self->priv->n_transitions = g_list_length (self->priv->transitions);
}


void
gth_slideshow_set_playlist (GthSlideshow  *self,
			    char         **files)
{
	self->priv->audio_files = g_strdupv (files);
	self->priv->audio_loop = TRUE;
}


void
gth_slideshow_set_random_order (GthSlideshow *self,
				gboolean      random)
{
	self->priv->random_order = random;
}


void
gth_slideshow_toggle_pause (GthSlideshow *self)
{
	g_return_if_fail (GTH_IS_SLIDESHOW (self));

	self->priv->paused = ! self->priv->paused;
	if (self->priv->paused) {
		self->priv->projector->paused (self);
#if HAVE_GSTREAMER
		if (self->priv->playbin != NULL)
			gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);
#endif
	}
	else { /* resume */
		gth_slideshow_load_next_image (self);
#if HAVE_GSTREAMER
		if (self->priv->playbin != NULL)
			gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
#endif
	}
}


void
gth_slideshow_load_next_image (GthSlideshow *self)
{
	g_return_if_fail (GTH_IS_SLIDESHOW (self));

	self->priv->projector->load_next_image (self);
	self->priv->direction = GTH_SLIDESHOW_DIRECTION_FORWARD;

	if (self->priv->paused)
		return;

	self->priv->current = self->priv->current->next;
	_gth_slideshow_load_current_image (self);
}


void
gth_slideshow_load_prev_image (GthSlideshow *self)
{
	g_return_if_fail (GTH_IS_SLIDESHOW (self));

	self->priv->projector->load_prev_image (self);
	self->priv->direction = GTH_SLIDESHOW_DIRECTION_BACKWARD;

	if (self->priv->paused)
		return;

	self->priv->current = self->priv->current->prev;
	_gth_slideshow_load_current_image (self);
}


void
gth_slideshow_next_image_or_resume (GthSlideshow *self)
{
	g_return_if_fail (GTH_IS_SLIDESHOW (self));

	if (self->priv->paused)
		gth_slideshow_toggle_pause (self);
	else
		gth_slideshow_load_next_image (self);
}


static void
_gth_slideshow_close_cb (gpointer user_data)
{
	GthSlideshow *self = user_data;
	gboolean      close_browser;
	GthBrowser   *browser;

	browser = self->priv->browser;
	close_browser = ! gtk_widget_get_visible (GTK_WIDGET (browser));
	self->priv->projector->show_cursor (self);
	self->priv->projector->finalize (self);
	gtk_widget_destroy (GTK_WIDGET (self));

	if (close_browser)
		gth_window_close (GTH_WINDOW (browser));
}


void
gth_slideshow_close (GthSlideshow *self)
{
	call_when_idle (_gth_slideshow_close_cb, self);
}


/* -- default projector -- */


static void
default_projector_load_next_image (GthSlideshow *self)
{
	/* void */
}


static void
default_projector_load_prev_image (GthSlideshow *self)
{
	/* void */
}


static void
default_projector_image_ready (GthSlideshow *self,
			       GthImage     *image)
{
	gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->viewer), image, -1, -1);
	view_next_image_automatically (self);
}


static void
default_projector_finalize (GthSlideshow *self)
{
	if (self->priv->hide_paused_sign != 0) {
		g_source_remove (self->priv->hide_paused_sign);
		self->priv->hide_paused_sign = 0;
	}
}


static void
default_projector_show_cursor (GthSlideshow *self)
{
	gth_image_viewer_show_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
default_projector_hide_cursor (GthSlideshow *self)
{
	gth_image_viewer_hide_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
default_projector_paused (GthSlideshow *self)
{
	if (self->priv->hide_paused_sign != 0) {
		g_source_remove (self->priv->hide_paused_sign);
		self->priv->hide_paused_sign = 0;
	}
	self->priv->paint_paused = TRUE;
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
viewer_event_cb (GtkWidget    *widget,
	         GdkEvent     *event,
	         GthSlideshow *self)
{
	if (event->type == GDK_MOTION_NOTIFY) {
		gth_image_viewer_show_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
		if (self->priv->hide_cursor_event != 0)
			g_source_remove (self->priv->hide_cursor_event);
		self->priv->hide_cursor_event = g_timeout_add_seconds (HIDE_CURSOR_DELAY, hide_cursor_cb, self);
	}
	else if (event->type == GDK_BUTTON_PRESS) {
		switch (((GdkEventButton *) event)->button) {
		case 1:
			gth_slideshow_load_next_image (self);
			break;
		case 3:
			gth_slideshow_load_prev_image (self);
			break;
		default:
			break;
		}
	}
}


static gboolean
hide_paused_sign_cb (gpointer user_data)
{
	GthSlideshow *self = user_data;

	g_source_remove (self->priv->hide_paused_sign);
	self->priv->hide_paused_sign = 0;

	self->priv->paint_paused = FALSE;
	gtk_widget_queue_draw (self->priv->viewer);

	return FALSE;
}


static void
default_projector_pause_painter (GthImageViewer *image_viewer,
				 cairo_t        *cr,
				 gpointer        user_data)
{
	GthSlideshow *self = user_data;
	int           screen_width;
	int           screen_height;
	double        dest_x;
	double        dest_y;

	if (! self->priv->paused || ! self->priv->paint_paused || (self->priv->pause_pixbuf == NULL))
		return;

	if (! _gtk_widget_get_screen_size (GTK_WIDGET (image_viewer), &screen_width, &screen_height))
		return;

	dest_x = (screen_width - gdk_pixbuf_get_width (self->priv->pause_pixbuf)) / 2.0;
	dest_y = (screen_height - gdk_pixbuf_get_height (self->priv->pause_pixbuf)) / 2.0;
	gdk_cairo_set_source_pixbuf (cr, self->priv->pause_pixbuf, dest_x, dest_y);
	cairo_rectangle (cr, dest_x, dest_y, gdk_pixbuf_get_width (self->priv->pause_pixbuf), gdk_pixbuf_get_height (self->priv->pause_pixbuf));
	cairo_fill (cr);

	if (self->priv->hide_paused_sign != 0)
		g_source_remove (self->priv->hide_paused_sign);
	self->priv->hide_paused_sign = g_timeout_add_seconds (HIDE_PAUSED_SIGN_DELAY, hide_paused_sign_cb, self);
}


static void
default_projector_construct (GthSlideshow *self)
{
	self->priv->hide_paused_sign = 0;
	self->priv->paint_paused = FALSE;

	self->priv->viewer = gth_image_viewer_new ();
	gth_image_viewer_hide_frame (GTH_IMAGE_VIEWER (self->priv->viewer));
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE);
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_ZOOM_CHANGE_FIT_SIZE);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_ZOOM_QUALITY_LOW);
	gth_image_viewer_add_painter (GTH_IMAGE_VIEWER (self->priv->viewer), default_projector_pause_painter, self);
	gth_image_viewer_set_transparency_style (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_TRANSPARENCY_STYLE_CHECKERED);

	g_signal_connect (self->priv->viewer, "button-press-event", G_CALLBACK (viewer_event_cb), self);
	g_signal_connect (self->priv->viewer, "motion-notify-event", G_CALLBACK (viewer_event_cb), self);
	g_signal_connect (self->priv->viewer, "key-press-event", G_CALLBACK (viewer_event_cb), self);
	g_signal_connect (self->priv->viewer, "key-release-event", G_CALLBACK (viewer_event_cb), self);

	gtk_widget_show (self->priv->viewer);
	gtk_container_add (GTK_CONTAINER (self), self->priv->viewer);
}


GthProjector default_projector = {
	default_projector_construct,
	default_projector_paused,
	default_projector_show_cursor,
	default_projector_hide_cursor,
	default_projector_finalize,
	default_projector_image_ready,
	default_projector_load_prev_image,
	default_projector_load_next_image
};


#ifdef HAVE_CLUTTER


/* -- clutter projector -- */


static void
_gth_slideshow_swap_current_and_next (GthSlideshow *self)
{
	ClutterGeometry tmp_geometry;

	self->current_image = self->next_image;
	if (self->current_image == self->priv->image1)
		self->next_image = self->priv->image2;
	else
		self->next_image = self->priv->image1;

	tmp_geometry = self->current_geometry;
	self->current_geometry = self->next_geometry;
	self->next_geometry = tmp_geometry;
}


static void
reset_texture_transformation (GthSlideshow *self,
			      ClutterActor *texture)
{
	float stage_w, stage_h;

	if (texture == NULL)
		return;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_pivot_point (texture, stage_w / 2.0, stage_h / 2.0);
	clutter_actor_set_opacity (texture, 255);
	clutter_actor_set_rotation_angle (texture, CLUTTER_X_AXIS, 0.0);
	clutter_actor_set_rotation_angle (texture, CLUTTER_Y_AXIS, 0.0);
	clutter_actor_set_rotation_angle (texture, CLUTTER_Z_AXIS, 0.0);
	clutter_actor_set_scale (texture, 1.0, 1.0);
}


static void
_gth_slideshow_reset_textures_position (GthSlideshow *self)
{
	if (self->next_image != NULL) {
		clutter_actor_set_size (self->next_image, (float) self->next_geometry.width, (float) self->next_geometry.height);
		clutter_actor_set_position (self->next_image, (float) self->next_geometry.x, (float) self->next_geometry.y);
	}

	if (self->current_image != NULL) {
		clutter_actor_set_size (self->current_image, (float) self->current_geometry.width, (float) self->current_geometry.height);
		clutter_actor_set_position (self->current_image, (float) self->current_geometry.x, (float) self->current_geometry.y);
	}

	if ((self->current_image != NULL) && (self->next_image != NULL)) {
		clutter_actor_set_child_above_sibling (CLUTTER_ACTOR (self->stage), self->current_image, self->next_image);
		clutter_actor_hide (self->next_image);
	}

	if (self->current_image != NULL)
		clutter_actor_show (self->current_image);

	reset_texture_transformation (self, self->next_image);
	reset_texture_transformation (self, self->current_image);
}


static void
_gth_slideshow_animation_completed (GthSlideshow *self)
{
	self->priv->animating = FALSE;
	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_FORWARD)
		_gth_slideshow_swap_current_and_next (self);
	_gth_slideshow_reset_textures_position (self);
}


static void
clutter_projector_load_next_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline)) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_FORWARD);
}


static void
clutter_projector_load_prev_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline)) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_BACKWARD);
}


static void
animation_completed_cb (ClutterTimeline *timeline,
			GthSlideshow    *self)
{
	_gth_slideshow_animation_completed (self);
	view_next_image_automatically (self);
}


static void
animation_frame_cb (ClutterTimeline *timeline,
		    int              msecs,
		    GthSlideshow    *self)
{
	if (self->priv->transition != NULL)
		gth_transition_frame (self->priv->transition,
				      self,
				      clutter_timeline_get_progress (self->priv->timeline));

	if (self->first_frame)
		self->first_frame = FALSE;
}


static void
animation_started_cb (ClutterTimeline *timeline,
		      GthSlideshow    *self)
{
	self->priv->animating = TRUE;
	self->first_frame = TRUE;
}


static GthTransition *
_gth_slideshow_get_transition (GthSlideshow *self)
{
	if (self->priv->transitions == NULL)
		return NULL;
	else if (self->priv->transitions->next == NULL)
		return self->priv->transitions->data;
	else
		return g_list_nth_data (self->priv->transitions,
					g_rand_int_range (self->priv->rand, 0, self->priv->n_transitions));
}


static void
clutter_projector_image_ready (GthSlideshow *self,
			       GthImage     *image_data)
{
	GdkPixbuf    *pixbuf;
	GdkPixbuf    *image;
	ClutterActor *texture;
	int           pixbuf_w, pixbuf_h;
	float         stage_w, stage_h;
	int           pixbuf_x, pixbuf_y;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	if ((stage_w == 0) || (stage_h == 0))
		return;

	pixbuf = gth_image_get_pixbuf (image_data);
	image = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
				FALSE,
				gdk_pixbuf_get_bits_per_sample (pixbuf),
				stage_w,
				stage_h);
	gdk_pixbuf_fill (image, 0x000000ff);

	pixbuf_w = gdk_pixbuf_get_width (pixbuf);
	pixbuf_h = gdk_pixbuf_get_height (pixbuf);
	scale_keeping_ratio (&pixbuf_w, &pixbuf_h, (int) stage_w, (int) stage_h, TRUE);
	pixbuf_x = (stage_w - pixbuf_w) / 2;
	pixbuf_y = (stage_h - pixbuf_h) / 2;

	gdk_pixbuf_composite (pixbuf,
			      image,
			      pixbuf_x,
			      pixbuf_y,
			      pixbuf_w,
			      pixbuf_h,
			      pixbuf_x,
			      pixbuf_y,
			      (double) pixbuf_w / gdk_pixbuf_get_width (pixbuf),
			      (double) pixbuf_h / gdk_pixbuf_get_height (pixbuf),
			      GDK_INTERP_BILINEAR,
			      255);

	if (self->next_image == self->priv->image1)
		texture = self->priv->image1;
	else
		texture = self->priv->image2;
	gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (texture), image, NULL);

	self->next_geometry.x = 0;
	self->next_geometry.y = 0;
	self->next_geometry.width = stage_w;
	self->next_geometry.height = stage_h;

	_gth_slideshow_reset_textures_position (self);
	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_BACKWARD)
		_gth_slideshow_swap_current_and_next (self);

	self->priv->transition = _gth_slideshow_get_transition (self);
	clutter_timeline_rewind (self->priv->timeline);
	clutter_timeline_start (self->priv->timeline);
	if (self->current_image == NULL)
		clutter_timeline_advance (self->priv->timeline, GTH_TRANSITION_DURATION);

	g_object_unref (image);
	g_object_unref (pixbuf);
}


static void
clutter_projector_finalize (GthSlideshow *self)
{
	_g_object_unref (self->priv->timeline);
}


static void
clutter_projector_show_cursor (GthSlideshow *self)
{
	clutter_stage_show_cursor (CLUTTER_STAGE (self->stage));
}


static void
clutter_projector_hide_cursor (GthSlideshow *self)
{
	clutter_stage_hide_cursor (CLUTTER_STAGE (self->stage));
}


static void
clutter_projector_paused (GthSlideshow *self)
{
	float stage_w;
	float stage_h;

	if (self->priv->animating) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	clutter_actor_set_position (self->priv->paused_actor, stage_w / 2.0, stage_h / 2.0);
	clutter_actor_set_pivot_point (self->priv->paused_actor, 0.5, 0.5);
	clutter_actor_set_scale (self->priv->paused_actor, 1.0, 1.0);
	clutter_actor_set_opacity (self->priv->paused_actor, 255);
	clutter_actor_set_child_above_sibling (CLUTTER_ACTOR (self->stage), self->priv->paused_actor, NULL);
	clutter_actor_show (self->priv->paused_actor);

	clutter_actor_save_easing_state (self->priv->paused_actor);
	clutter_actor_set_easing_mode (self->priv->paused_actor, CLUTTER_LINEAR);
	clutter_actor_set_easing_duration (self->priv->paused_actor, 500);
	clutter_actor_set_scale (self->priv->paused_actor, 3.0, 3.0);
	clutter_actor_set_opacity (self->priv->paused_actor, 0);
	clutter_actor_restore_easing_state (self->priv->paused_actor);
}


static void
stage_input_cb (ClutterStage *stage,
	        ClutterEvent *event,
	        GthSlideshow *self)
{
	if (event->type == CLUTTER_MOTION) {
		clutter_stage_show_cursor (CLUTTER_STAGE (self->stage));
		if (self->priv->hide_cursor_event != 0)
			g_source_remove (self->priv->hide_cursor_event);
		self->priv->hide_cursor_event = g_timeout_add (HIDE_CURSOR_DELAY, hide_cursor_cb, self);
	}
	else if (event->type == CLUTTER_BUTTON_PRESS) {
		guint32 event_time;

		/* avoid a double button_press emission that seems to be a
		 * clutter bug */
		event_time = ((ClutterButtonEvent *)event)->time;
		if (self->priv->last_button_event_time == event_time)
			return;
		self->priv->last_button_event_time = event_time;

		switch (clutter_event_get_button (event)) {
		case 1:
			gth_slideshow_load_next_image (self);
			break;
		case 3:
			gth_slideshow_load_prev_image (self);
			break;
		default:
			break;
		}
	}
}


static void
adapt_image_size_to_stage_size (GthSlideshow *self)
{
	gfloat          stage_w, stage_h;
	GdkPixbuf      *pixbuf;
	GdkPixbuf      *image;
	int             pixbuf_w, pixbuf_h;
	int             pixbuf_x, pixbuf_y;
	ClutterActor   *texture;

	if (self->current_image == NULL)
		return;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	if ((stage_w == 0) || (stage_h == 0))
		return;

	if (self->priv->current_image == NULL)
		return;

	pixbuf = gth_image_get_pixbuf (self->priv->current_image);
	image = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
				FALSE,
				gdk_pixbuf_get_bits_per_sample (pixbuf),
				stage_w,
				stage_h);
	gdk_pixbuf_fill (image, 0x000000ff);

	pixbuf_w = gdk_pixbuf_get_width (pixbuf);
	pixbuf_h = gdk_pixbuf_get_height (pixbuf);
	scale_keeping_ratio (&pixbuf_w, &pixbuf_h, (int) stage_w, (int) stage_h, TRUE);
	pixbuf_x = (stage_w - pixbuf_w) / 2;
	pixbuf_y = (stage_h - pixbuf_h) / 2;

	gdk_pixbuf_composite (pixbuf,
			      image,
			      pixbuf_x,
			      pixbuf_y,
			      pixbuf_w,
			      pixbuf_h,
			      pixbuf_x,
			      pixbuf_y,
			      (double) pixbuf_w / gdk_pixbuf_get_width (pixbuf),
			      (double) pixbuf_h / gdk_pixbuf_get_height (pixbuf),
			      GDK_INTERP_BILINEAR,
			      255);

	if (self->current_image == self->priv->image1)
		texture = self->priv->image1;
	else
		texture = self->priv->image2;
	gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (texture), image, NULL);

	self->current_geometry.x = 0;
	self->current_geometry.y = 0;
	self->current_geometry.width = stage_w;
	self->current_geometry.height = stage_h;
	_gth_slideshow_reset_textures_position (self);

	g_object_unref (image);
	g_object_unref (pixbuf);
}


static void
gth_slideshow_size_allocate_cb (GtkWidget     *widget,
				GtkAllocation *allocation,
				gpointer       user_data)
{
	adapt_image_size_to_stage_size (GTH_SLIDESHOW (user_data));
}


static void
clutter_projector_construct (GthSlideshow *self)
{
	GtkWidget    *embed;
	ClutterColor  background_color = { 0x0, 0x0, 0x0, 0xff };

	embed = gtk_clutter_embed_new ();
	self->stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));
	clutter_stage_hide_cursor (CLUTTER_STAGE (self->stage));
	clutter_actor_set_background_color (CLUTTER_ACTOR (self->stage), &background_color);

	self->priv->last_button_event_time = 0;
	g_signal_connect (self->stage, "button-press-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->stage, "motion-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->stage, "key-press-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->stage, "key-release-event", G_CALLBACK (stage_input_cb), self);

	self->priv->image1 = gtk_clutter_texture_new ();
	clutter_actor_hide (self->priv->image1);
	clutter_actor_add_child (CLUTTER_ACTOR (self->stage), self->priv->image1);

	self->priv->image2 = gtk_clutter_texture_new ();
	clutter_actor_hide (self->priv->image2);
	clutter_actor_add_child (CLUTTER_ACTOR (self->stage), self->priv->image2);

	self->current_image = NULL;
	self->next_image = self->priv->image1;

	self->priv->timeline = clutter_timeline_new (GTH_TRANSITION_DURATION);
	clutter_timeline_set_progress_mode (self->priv->timeline, CLUTTER_EASE_IN_OUT_SINE);
	g_signal_connect (self->priv->timeline, "completed", G_CALLBACK (animation_completed_cb), self);
	g_signal_connect (self->priv->timeline, "new-frame", G_CALLBACK (animation_frame_cb), self);
	g_signal_connect (self->priv->timeline, "started", G_CALLBACK (animation_started_cb), self);

	self->priv->paused_actor = gtk_clutter_texture_new ();
	if (self->priv->pause_pixbuf != NULL)
		gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (self->priv->paused_actor),
				 	 	     self->priv->pause_pixbuf,
				 	 	     NULL);
	else
		gtk_clutter_texture_set_from_icon_name (GTK_CLUTTER_TEXTURE (self->priv->paused_actor),
						    	GTK_WIDGET (self),
						    	"media-playback-pause-symbolic",
						    	GTK_ICON_SIZE_DIALOG,
						    	NULL);
	clutter_actor_hide (self->priv->paused_actor);
	clutter_actor_add_child (CLUTTER_ACTOR (self->stage), self->priv->paused_actor);

	g_signal_connect (self, "size-allocate", G_CALLBACK (gth_slideshow_size_allocate_cb), self);

	gtk_widget_show (embed);
	gtk_container_add (GTK_CONTAINER (self), embed);
}


GthProjector clutter_projector = {
	clutter_projector_construct,
	clutter_projector_paused,
	clutter_projector_show_cursor,
	clutter_projector_hide_cursor,
	clutter_projector_finalize,
	clutter_projector_image_ready,
	clutter_projector_load_prev_image,
	clutter_projector_load_next_image
};


#endif /* HAVE_CLUTTER*/
