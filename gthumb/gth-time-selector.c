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
#define _XOPEN_SOURCE /* glibc2 needs this */
#define _XOPEN_SOURCE_EXTENDED 1
#include <time.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-time-selector.h"


enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthTimeSelectorPrivate
{
	GthDateTime *date_time;
	GtkWidget   *date_entry;
	GtkWidget   *calendar_button;
	GtkWidget   *calendar;
	GtkWidget   *calendar_popup;
	GtkWidget   *use_time_checkbutton;
	GtkWidget   *time_combo_box;
	GtkWidget   *popup_box;
	GtkWidget   *now_button;
	gboolean     use_time;
	gboolean     optional_time;
	gulong       day_selected_event;
};


static guint gth_time_selector_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GthTimeSelector, gth_time_selector, GTK_TYPE_BOX);


static void
gth_time_selector_finalize (GObject *object)
{
	GthTimeSelector *self;

	self = GTH_TIME_SELECTOR (object);

	gtk_widget_destroy (self->priv->calendar_popup);
	gth_datetime_free (self->priv->date_time);

	G_OBJECT_CLASS (gth_time_selector_parent_class)->finalize (object);
}


static void
gth_time_selector_class_init (GthTimeSelectorClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (GthTimeSelectorPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_time_selector_finalize;

	gth_time_selector_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTimeSelectorClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_time_selector_init (GthTimeSelector *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TIME_SELECTOR, GthTimeSelectorPrivate);
	self->priv->date_time = gth_datetime_new ();
	self->priv->use_time = TRUE;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
}


static void
hide_calendar_popup (GthTimeSelector *self)
{
	gtk_grab_remove (self->priv->calendar_popup);
	/*gdk_keyboard_ungrab (GDK_CURRENT_TIME);*/
	gdk_device_ungrab (gdk_device_manager_get_client_pointer (gdk_display_get_device_manager (gdk_window_get_display (gtk_widget_get_window (self->priv->calendar_popup)))),
			   GDK_CURRENT_TIME);
	gtk_widget_hide (self->priv->calendar_popup);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->calendar_button), FALSE);
}


static void
show_calendar_popup (GthTimeSelector *self)
{
	GtkRequisition         popup_req;
	int                    x;
	int                    y;
	GtkAllocation          allocation;
	int                    selector_height;
	GdkScreen             *screen;
	gint                   monitor_num;
	cairo_rectangle_int_t  monitor;

	gtk_widget_get_preferred_size (self->priv->popup_box, &popup_req, NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->calendar_button), TRUE);

	gdk_window_get_position (gtk_widget_get_window (GTK_WIDGET (self)), &x, &y);
	gtk_widget_get_allocation (self->priv->date_entry, &allocation);
	x += allocation.x;
	y += allocation.y;
	selector_height = allocation.height;

	screen = gtk_widget_get_screen (GTK_WIDGET (self));
	monitor_num = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (GTK_WIDGET (self)));
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (x < monitor.x)
		x = monitor.x;
	else if (x + popup_req.width > monitor.x + monitor.width)
		x = monitor.x + monitor.width - popup_req.width;
	if (y + selector_height + popup_req.height > monitor.y + monitor.height)
		y = y - popup_req.height;
	else
		y = y + selector_height;

	gtk_window_move (GTK_WINDOW (self->priv->calendar_popup), x, y);
	gtk_widget_show (self->priv->calendar_popup);

	/*gdk_keyboard_grab (gtk_widget_get_window (self->priv->calendar_popup), TRUE, GDK_CURRENT_TIME);*/
	gtk_grab_add (self->priv->calendar_popup);
	gdk_device_grab (gdk_device_manager_get_client_pointer (gdk_display_get_device_manager (gdk_window_get_display (gtk_widget_get_window (self->priv->calendar_popup)))),
			 gtk_widget_get_window (self->priv->calendar_popup),
			 GDK_OWNERSHIP_WINDOW,
			 TRUE,
			 (GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK
			  | GDK_POINTER_MOTION_HINT_MASK
			  | GDK_BUTTON_MOTION_MASK),
			 NULL,
			 GDK_CURRENT_TIME);
	gtk_widget_grab_focus (self->priv->calendar);
}


static void
calendar_button_toggled_cb (GtkToggleButton *button,
			    gpointer         user_data)
{
	GthTimeSelector *self = user_data;

	if (gtk_toggle_button_get_active (button))
		show_calendar_popup (self);
	else
		hide_calendar_popup (self);
}


static void
update_date_from_view (GthTimeSelector *self)
{
	struct tm tm;

	strptime (gtk_entry_get_text (GTK_ENTRY (self->priv->date_entry)), "%x", &tm);
	if (self->priv->use_time)
		strptime (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (self->priv->time_combo_box)))), "%X", &tm);
	else {
		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
	}
	gth_datetime_from_struct_tm (self->priv->date_time, &tm);
}


static void
update_view_from_data (GthTimeSelector *self)
{
	if (self->priv->use_time) {
		if (gth_time_valid (self->priv->date_time->time)) {
			char      *text;
			GtkWidget *entry;

			text = gth_datetime_strftime (self->priv->date_time, "%X");
			entry = gtk_bin_get_child (GTK_BIN (self->priv->time_combo_box));
			gtk_entry_set_text (GTK_ENTRY (entry), text);

			g_free (text);
		}
		else {
			GtkWidget *entry;

			entry = gtk_bin_get_child (GTK_BIN (self->priv->time_combo_box));
			gtk_entry_set_text (GTK_ENTRY (entry), "");
		}
	}

	if (g_date_valid (self->priv->date_time->date)) {
		struct tm  tm;
		char      *text;

		g_date_to_struct_tm (self->priv->date_time->date, &tm);
		text = struct_tm_strftime (&tm, "%x");
		gtk_entry_set_text (GTK_ENTRY (self->priv->date_entry), text);

		if (gth_datetime_valid (self->priv->date_time)) {
			g_signal_handler_block (GTK_CALENDAR (self->priv->calendar), self->priv->day_selected_event);
			gtk_calendar_select_month (GTK_CALENDAR (self->priv->calendar),
						   g_date_get_month (self->priv->date_time->date) - 1,
						   g_date_get_year (self->priv->date_time->date));
			gtk_calendar_select_day (GTK_CALENDAR (self->priv->calendar),
						 g_date_get_day (self->priv->date_time->date));
			g_signal_handler_unblock (GTK_CALENDAR (self->priv->calendar), self->priv->day_selected_event);
		}
	}
	else
		gtk_entry_set_text (GTK_ENTRY (self->priv->date_entry), "");
}


static void
gth_time_selector_changed (GthTimeSelector *self)
{
	g_signal_emit (self, gth_time_selector_signals[CHANGED], 0);
}


static void
use_time_checkbutton_toggled_cb (GtkToggleButton *button,
				 gpointer         user_data)
{
	GthTimeSelector *self = user_data;

	self->priv->use_time = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->use_time_checkbutton));
	gtk_widget_set_sensitive (self->priv->time_combo_box, self->priv->use_time);
}


static void
today_button_clicked_cb (GtkButton *button,
			 gpointer   user_data)
{
	GthTimeSelector *self = user_data;
	GTimeVal         timeval;

	update_date_from_view (self);
	g_get_current_time (&timeval);
	g_date_set_time_val (self->priv->date_time->date, &timeval);
	update_view_from_data (self);
	gth_time_selector_changed (self);
	hide_calendar_popup (self);
}


static void
now_button_clicked_cb (GtkButton *button,
			 gpointer   user_data)
{
	GthTimeSelector *self = user_data;
	GTimeVal         timeval;
	struct tm       *tm;
	time_t           secs;

	update_date_from_view (self);
	g_get_current_time (&timeval);
	g_date_set_time_val (self->priv->date_time->date, &timeval);

	secs = timeval.tv_sec;
	tm = localtime (&secs);
	gth_time_set_hms (self->priv->date_time->time, tm->tm_hour, tm->tm_min, tm->tm_sec, timeval.tv_usec);

	update_view_from_data (self);
	hide_calendar_popup (self);
}


static gboolean
calendar_day_selected_double_click_cb (GtkCalendar *calendar,
				       gpointer     user_data)
{
	GthTimeSelector *self = user_data;
	guint            y, m, d;

	update_date_from_view (self);
	gtk_calendar_get_date (GTK_CALENDAR (self->priv->calendar), &y, &m, &d);
	g_date_set_dmy (self->priv->date_time->date, d, m + 1, y);
	update_view_from_data (self);
	gth_time_selector_changed (self);
	hide_calendar_popup (self);

	return FALSE;
}



static gboolean
calendar_day_selected_cb (GtkCalendar *calendar,
		          gpointer     user_data)
{
	GthTimeSelector *self = user_data;
	guint            y, m, d;

	update_date_from_view (self);
	gtk_calendar_get_date (GTK_CALENDAR (self->priv->calendar), &y, &m, &d);
	g_date_set_dmy (self->priv->date_time->date, d, m + 1, y);
	update_view_from_data (self);
	gth_time_selector_changed (self);

	return FALSE;
}


static gboolean
calendar_popup_button_press_event_cb (GtkWidget      *widget,
				      GdkEventButton *event,
				      gpointer        user_data)
{
	GthTimeSelector       *self = user_data;
	cairo_rectangle_int_t  popup_area;

	gdk_window_get_geometry (gtk_widget_get_window (self->priv->calendar_popup),
				 &popup_area.x,
				 &popup_area.y,
				 &popup_area.width,
				 &popup_area.height);

	/*g_print ("(%.0f, %.0f) <==> (%d, %d)[%d, %d]\n", event->x_root, event->y_root,  popup_area.x,  popup_area.y, popup_area.width, popup_area.height);*/

	if ((event->x_root < popup_area.x)
	    || (event->x_root > popup_area.x + popup_area.width)
	    || (event->y_root < popup_area.y)
	    || (event->y_root > popup_area.y + popup_area.height))
	{
		hide_calendar_popup (self);
	}

	return FALSE;
}


static gboolean
calendar_popup_key_press_event_cb (GtkWidget   *widget,
				   GdkEventKey *event,
				   gpointer     user_data)
{
	GthTimeSelector *self = user_data;

	switch (event->keyval) {
	case GDK_KEY_Escape:
		hide_calendar_popup (self);
		break;

	default:
		break;
	}

	return FALSE;
}


static void
date_entry_activate_cb (GtkEntry        *entry,
			GthTimeSelector *self)
{
	update_date_from_view (self);

	if (gth_datetime_valid (self->priv->date_time)) {
		gtk_calendar_select_month (GTK_CALENDAR (self->priv->calendar),
					   g_date_get_month (self->priv->date_time->date) - 1,
					   g_date_get_year (self->priv->date_time->date));
		gtk_calendar_select_day (GTK_CALENDAR (self->priv->calendar),
					 g_date_get_day (self->priv->date_time->date));
	}
	gth_time_selector_changed (self);
}


static void
gth_time_selector_construct (GthTimeSelector *self)
{
	GtkWidget   *frame;
	GtkWidget   *box;
	GtkWidget   *button_box;
	GtkWidget   *time_box;
	GtkWidget   *button;
	GthDateTime *dt;
	guint8       h;

	g_object_set (self, "spacing", 6, NULL);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (self), box, FALSE, FALSE, 0);

	self->priv->date_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (self->priv->date_entry), 10);
	gtk_editable_set_editable (GTK_EDITABLE (self->priv->date_entry), TRUE);
	gtk_widget_show (self->priv->date_entry);
	gtk_box_pack_start (GTK_BOX (box), self->priv->date_entry, FALSE, FALSE, 0);

	self->priv->calendar_button = gtk_toggle_button_new ();
	gtk_container_add (GTK_CONTAINER (self->priv->calendar_button), gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
	gtk_widget_show_all (self->priv->calendar_button);
	gtk_box_pack_start (GTK_BOX (box), self->priv->calendar_button, FALSE, FALSE, 0);
	g_signal_connect (self->priv->calendar_button,
			  "toggled",
			  G_CALLBACK (calendar_button_toggled_cb),
			  self);

	self->priv->calendar_popup = gtk_window_new (GTK_WINDOW_POPUP);
	g_signal_connect (self->priv->calendar_popup,
			  "button-press-event",
			  G_CALLBACK (calendar_popup_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->calendar_popup,
			  "key-press-event",
			  G_CALLBACK (calendar_popup_key_press_event_cb),
			  self);

	self->priv->popup_box = frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (self->priv->calendar_popup), frame);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);
	gtk_widget_show (box);
	gtk_container_add (GTK_CONTAINER (frame), box);

	self->priv->calendar = gtk_calendar_new ();
	gtk_widget_show (self->priv->calendar);
	gtk_box_pack_start (GTK_BOX (box), self->priv->calendar, FALSE, FALSE, 0);
	g_signal_connect (self->priv->calendar,
			  "day-selected-double-click",
			  G_CALLBACK (calendar_day_selected_double_click_cb),
			  self);
	self->priv->day_selected_event =
			g_signal_connect (self->priv->calendar,
					  "day-selected",
					  G_CALLBACK (calendar_day_selected_cb),
					  self);

	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_set_homogeneous (GTK_BOX (button_box), TRUE);
	gtk_widget_show (button_box);
	gtk_box_pack_start (GTK_BOX (box), button_box, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Today"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (today_button_clicked_cb),
			  self);

	self->priv->now_button = button = gtk_button_new_with_label (_("Now"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (now_button_clicked_cb),
			  self);

	time_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (time_box);
	gtk_box_pack_start (GTK_BOX (self), time_box, FALSE, FALSE, 0);

	self->priv->use_time_checkbutton = gtk_check_button_new ();
	gtk_widget_show (self->priv->use_time_checkbutton);
	gtk_box_pack_start (GTK_BOX (time_box), self->priv->use_time_checkbutton, FALSE, FALSE, 0);

	self->priv->time_combo_box = gtk_combo_box_text_new_with_entry ();
	gtk_entry_set_width_chars (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (self->priv->time_combo_box))), 10);
	gtk_widget_show (self->priv->time_combo_box);
	gtk_box_pack_start (GTK_BOX (time_box), self->priv->time_combo_box, FALSE, FALSE, 0);

	dt = gth_datetime_new ();
	g_date_set_dmy (dt->date, 1, 1, 2000);
	for (h = 0; h < 24; h++) {
		char *text;

		gth_time_set_hms (dt->time, h, 0, 0, 0);
		text = gth_datetime_strftime (dt, "%X");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->time_combo_box), text);
		g_free (text);

		gth_time_set_hms (dt->time, h, 30, 0, 0);
		text = gth_datetime_strftime (dt, "%X");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->time_combo_box), text);
		g_free (text);
	}

	gth_time_selector_show_time (self, TRUE, FALSE);

	g_signal_connect (self->priv->date_entry,
			  "activate",
			  G_CALLBACK (date_entry_activate_cb),
			  self);
	g_signal_connect_swapped (self->priv->time_combo_box,
				  "changed",
				  G_CALLBACK (gth_time_selector_changed),
				  self);
	g_signal_connect (self->priv->use_time_checkbutton,
			  "toggled",
			  G_CALLBACK (use_time_checkbutton_toggled_cb),
			  self);

	gth_datetime_free (dt);
}


GtkWidget *
gth_time_selector_new (void)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_TIME_SELECTOR, NULL));
	gth_time_selector_construct (GTH_TIME_SELECTOR (widget));

	return widget;
}


void
gth_time_selector_show_time (GthTimeSelector *self,
			     gboolean         show,
			     gboolean         optional)
{
	self->priv->use_time = show;
	self->priv->optional_time = optional;

	if (show || optional) {
		gtk_widget_show (self->priv->time_combo_box);
		gtk_widget_show (self->priv->now_button);
	}
	else {
		gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (self->priv->time_combo_box))), "00:00:00");
		gtk_widget_hide (self->priv->time_combo_box);
		gtk_widget_hide (self->priv->now_button);
	}

	if (self->priv->optional_time)
		gtk_widget_show (self->priv->use_time_checkbutton);
	else
		gtk_widget_hide (self->priv->use_time_checkbutton);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->use_time_checkbutton), self->priv->use_time);
	gtk_widget_set_sensitive (self->priv->time_combo_box, self->priv->use_time);
}


void
gth_time_selector_set_value (GthTimeSelector *self,
			     GthDateTime     *date_time)
{
	*self->priv->date_time->date = *date_time->date;
	if (self->priv->use_time)
		*self->priv->date_time->time = *date_time->time;
	else
		gth_time_clear (self->priv->date_time->time);
	update_view_from_data (self);
}


void
gth_time_selector_set_exif_date (GthTimeSelector *self,
				 const char      *exif_date)
{
	GthDateTime *date_time;

	date_time = gth_datetime_new ();
	if (! gth_datetime_from_exif_date (date_time, exif_date))
		gth_datetime_clear (date_time);
	gth_time_selector_set_value (self, date_time);

	gth_datetime_free (date_time);
}


void
gth_time_selector_get_value (GthTimeSelector *self,
			     GthDateTime     *date_time)
{
	g_return_if_fail (date_time != NULL);

	update_date_from_view (self);
	*date_time->date = *self->priv->date_time->date;
	if (self->priv->use_time)
		*date_time->time = *self->priv->date_time->time;
	else
		gth_time_clear (date_time->time);
}
