/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 The Free Software Foundation, Inc.
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
#include "dlg-change-date.h"
#include "gth-change-date-task.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define IS_ACTIVE(x) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (x)))
#define HOURS_TO_SECONDS(h) ((h) * 3600)
#define MINS_TO_SECONDS(h) ((h) * 60)


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GSettings  *settings;
	GtkWidget  *dialog;
	GtkWidget  *date_selector;
	GList      *file_list;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	_g_object_list_unref (data->file_list);
	g_object_unref (data->settings);
	g_object_unref (data->builder);
	g_free (data);
}


static void
ok_button_clicked (GtkWidget  *button,
		   DialogData *data)
{
	GthChangeFields  change_fields;
	GthChangeType    change_type;
	GthDateTime     *date_time;
	int              time_adjustment;
	GthTask         *task;

	date_time = NULL;

	change_fields = 0;
	if (IS_ACTIVE (GET_WIDGET ("change_last_modified_checkbutton")))
		change_fields |= GTH_CHANGE_LAST_MODIFIED_DATE;
	if (IS_ACTIVE (GET_WIDGET ("change_comment_checkbutton")))
		change_fields |= GTH_CHANGE_COMMENT_DATE;

	change_type = 0;
	time_adjustment = 0;
	if (IS_ACTIVE (GET_WIDGET ("to_following_date_radiobutton"))) {
		change_type = GTH_CHANGE_TO_FOLLOWING_DATE;
		date_time = gth_datetime_new ();
		gth_time_selector_get_value (GTH_TIME_SELECTOR (data->date_selector), date_time);
	}
	else if (IS_ACTIVE (GET_WIDGET ("to_last_modified_date_radiobutton")))
		change_type = GTH_CHANGE_TO_FILE_MODIFIED_DATE;
	else if (IS_ACTIVE (GET_WIDGET ("to_creation_date_radiobutton")))
		change_type = GTH_CHANGE_TO_FILE_CREATION_DATE;
	else if (IS_ACTIVE (GET_WIDGET ("to_photo_original_date_radiobutton")))
		change_type = GTH_CHANGE_TO_PHOTO_ORIGINAL_DATE;
	else if (IS_ACTIVE (GET_WIDGET ("adjust_time_radiobutton"))) {
		change_type = GTH_CHANGE_ADJUST_TIME;
		time_adjustment = (HOURS_TO_SECONDS (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_time_h_spinbutton"))))
				   + MINS_TO_SECONDS (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_time_m_spinbutton"))))
				   + gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_time_s_spinbutton"))));
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("adjust_sign_combobox"))) == 1)
			time_adjustment = -time_adjustment;
	}

	/* save the preferences */

	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_SET_LAST_MODIFIED_DATE, (change_fields & GTH_CHANGE_LAST_MODIFIED_DATE) == GTH_CHANGE_LAST_MODIFIED_DATE);
	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_SET_COMMENT_DATE, (change_fields & GTH_CHANGE_COMMENT_DATE) == GTH_CHANGE_COMMENT_DATE);

	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_TO_FOLLOWING_DATE, change_type == GTH_CHANGE_TO_FOLLOWING_DATE);
	if (change_type == GTH_CHANGE_TO_FOLLOWING_DATE) {
		char *s;
		s = gth_datetime_to_exif_date (date_time);
		g_settings_set_string (data->settings, PREF_CHANGE_DATE_DATE, s);
		g_free (s);
	}
	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_TO_FILE_MODIFIED_DATE, change_type == GTH_CHANGE_TO_FILE_MODIFIED_DATE);
	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_TO_FILE_CREATION_DATE, change_type == GTH_CHANGE_TO_FILE_CREATION_DATE);
	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_TO_PHOTO_ORIGINAL_DATE, change_type == GTH_CHANGE_TO_PHOTO_ORIGINAL_DATE);
	g_settings_set_boolean (data->settings, PREF_CHANGE_DATE_ADJUST_TIME, change_type == GTH_CHANGE_ADJUST_TIME);
	if (change_type == GTH_CHANGE_ADJUST_TIME)
		g_settings_set_int (data->settings, PREF_CHANGE_DATE_TIME_ADJUSTMENT, time_adjustment);

	/* exec the task */

	task = gth_change_date_task_new (gth_browser_get_location (data->browser),
					 data->file_list,
					 change_fields,
					 change_type,
					 date_time,
					 time_adjustment);
	gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);
	gtk_widget_destroy (data->dialog);

	g_object_unref (task);
	gth_datetime_free (date_time);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog),
					   GTK_RESPONSE_OK,
					   (IS_ACTIVE (GET_WIDGET ("change_last_modified_checkbutton"))
					    || IS_ACTIVE (GET_WIDGET ("change_comment_checkbutton"))));
	gtk_widget_set_sensitive (data->date_selector, IS_ACTIVE (GET_WIDGET ("to_following_date_radiobutton")));
	gtk_widget_set_sensitive (GET_WIDGET ("time_box"), IS_ACTIVE (GET_WIDGET ("adjust_time_radiobutton")));

	if (IS_ACTIVE (GET_WIDGET ("change_last_modified_checkbutton"))) {
		gtk_widget_set_sensitive (GET_WIDGET ("to_last_modified_date_radiobutton"), FALSE);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_last_modified_date_radiobutton"))))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_following_date_radiobutton")), TRUE);
	}
	else
		gtk_widget_set_sensitive (GET_WIDGET ("to_last_modified_date_radiobutton"), TRUE);
}


static void
radio_button_clicked (GtkWidget  *button,
		      DialogData *data)
{
	update_sensitivity (data);
}


void
dlg_change_date (GthBrowser *browser,
		 GList      *file_list)
{
	DialogData  *data;
	GTimeVal     timeval;
	GthDateTime *datetime;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("change-date.ui", "change_date");
	data->settings = g_settings_new (GTHUMB_CHANGE_DATE_SCHEMA);

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Change Date"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_EXECUTE, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	data->date_selector = gth_time_selector_new ();
	gth_time_selector_show_time (GTH_TIME_SELECTOR (data->date_selector), TRUE, TRUE);
	gtk_widget_show (data->date_selector);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_selector_box")), data->date_selector, TRUE, TRUE, 0);

	/* Set widgets data. */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("change_last_modified_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_SET_LAST_MODIFIED_DATE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("change_comment_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_SET_COMMENT_DATE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_following_date_radiobutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_TO_FOLLOWING_DATE));

	datetime = gth_datetime_new ();
	g_get_current_time (&timeval);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_following_date_radiobutton")))) {
		char *s;
		s = g_settings_get_string (data->settings, PREF_CHANGE_DATE_DATE);
		if (strcmp (s, "") != 0)
			gth_datetime_from_exif_date (datetime, s);
		else
			gth_datetime_from_timeval (datetime, &timeval);
		g_free (s);
	}
	else
		gth_datetime_from_timeval (datetime, &timeval);
	gth_time_selector_set_value (GTH_TIME_SELECTOR (data->date_selector), datetime);
	gth_datetime_free (datetime);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_last_modified_date_radiobutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_TO_FILE_MODIFIED_DATE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_creation_date_radiobutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_TO_FILE_CREATION_DATE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_photo_original_date_radiobutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_TO_PHOTO_ORIGINAL_DATE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adjust_time_radiobutton")),
				      g_settings_get_boolean (data->settings, PREF_CHANGE_DATE_ADJUST_TIME));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adjust_time_radiobutton")))) {
		int adjustement;
		int sign;
		int hours;
		int minutes;
		int seconds;

		adjustement = g_settings_get_int (data->settings, PREF_CHANGE_DATE_TIME_ADJUSTMENT);
		if (adjustement < 0) {
			sign = -1;
			adjustement = - adjustement;
		}
		else
			sign = 1;

		hours = adjustement / 3600;
		adjustement = adjustement % 3600;

		minutes = adjustement / 60;
		adjustement = adjustement % 60;

		seconds = adjustement;

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_time_h_spinbutton")), hours);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_time_m_spinbutton")), minutes);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_time_s_spinbutton")), seconds);
		gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("adjust_sign_combobox")), (sign >= 0) ? 0 : 1);
	}

	update_sensitivity (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("change_last_modified_checkbutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("change_comment_checkbutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_following_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_last_modified_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_creation_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_photo_original_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("adjust_time_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
