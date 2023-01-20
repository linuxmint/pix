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
#include <glib/gi18n.h>
#include "gth-slideshow-preferences.h"
#include "gth-transition.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define BROWSER_DATA_KEY "slideshow-preference-data"


typedef struct {
	GSettings *settings;
	GtkWidget *preferences_page;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->settings);
	g_free (data);
}


static void
transition_combobox_changed_cb (GtkComboBox *combo_box,
				BrowserData *data)
{
	char *transition_id;

	transition_id = gth_slideshow_preferences_get_transition_id (GTH_SLIDESHOW_PREFERENCES (data->preferences_page));
	if (transition_id != NULL)
		g_settings_set_string (data->settings, PREF_SLIDESHOW_TRANSITION, transition_id);

	g_free (transition_id);
}


static void
automatic_checkbutton_toggled_cb (GtkToggleButton *button,
				  BrowserData     *data)
{
	g_settings_set_boolean (data->settings, PREF_SLIDESHOW_AUTOMATIC, gtk_toggle_button_get_active (button));
}


static void
wrap_around_checkbutton_toggled_cb (GtkToggleButton *button,
				    BrowserData     *data)
{
	g_settings_set_boolean (data->settings, PREF_SLIDESHOW_WRAP_AROUND, gtk_toggle_button_get_active (button));
}


static void
random_order_checkbutton_toggled_cb (GtkToggleButton *button,
				     BrowserData     *data)
{
	g_settings_set_boolean (data->settings, PREF_SLIDESHOW_RANDOM_ORDER, gtk_toggle_button_get_active (button));
}


static void
change_delay_spinbutton_value_changed_cb (GtkSpinButton *spinbutton,
				          BrowserData   *data)
{
	g_settings_set_double (data->settings, PREF_SLIDESHOW_CHANGE_DELAY, gtk_spin_button_get_value (spinbutton));
}


void
ss__dlg_preferences_construct_cb (GtkWidget  *dialog,
				  GthBrowser *browser,
				  GtkBuilder *dialog_builder)
{
	BrowserData *data;
	GtkWidget   *notebook;
	char        *current_transition;
	GtkWidget   *label;

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	data = g_new0 (BrowserData, 1);
	data->settings = g_settings_new (GTHUMB_SLIDESHOW_SCHEMA);

	current_transition = g_settings_get_string (data->settings, PREF_SLIDESHOW_TRANSITION);
	data->preferences_page = gth_slideshow_preferences_new (current_transition,
							        g_settings_get_boolean (data->settings, PREF_SLIDESHOW_AUTOMATIC),
							        (int) (1000.0 * g_settings_get_double (data->settings, PREF_SLIDESHOW_CHANGE_DELAY)),
							        g_settings_get_boolean (data->settings, PREF_SLIDESHOW_WRAP_AROUND),
							        g_settings_get_boolean (data->settings, PREF_SLIDESHOW_RANDOM_ORDER));
	gtk_widget_show (data->preferences_page);
	g_free (current_transition);

#ifndef HAVE_CLUTTER
	gtk_widget_hide (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (data->preferences_page), "transition_box"));
#endif /* ! HAVE_CLUTTER */

	g_signal_connect (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (data->preferences_page), "transition_combobox"),
			  "changed",
			  G_CALLBACK (transition_combobox_changed_cb),
			  data);
	g_signal_connect (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (data->preferences_page), "automatic_checkbutton"),
			  "toggled",
			  G_CALLBACK (automatic_checkbutton_toggled_cb),
			  data);
	g_signal_connect (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (data->preferences_page), "wrap_around_checkbutton"),
			  "toggled",
			  G_CALLBACK (wrap_around_checkbutton_toggled_cb),
			  data);
	g_signal_connect (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (data->preferences_page), "random_order_checkbutton"),
			  "toggled",
			  G_CALLBACK (random_order_checkbutton_toggled_cb),
			  data);
	g_signal_connect (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (data->preferences_page), "change_delay_spinbutton"),
			  "value-changed",
			  G_CALLBACK (change_delay_spinbutton_value_changed_cb),
			  data);

	label = gtk_label_new (_("Presentation"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), data->preferences_page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
