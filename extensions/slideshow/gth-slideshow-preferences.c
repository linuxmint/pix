/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-slideshow-preferences.h"
#include "gth-transition.h"


enum {
	TRANSITION_COLUMN_ID,
	TRANSITION_COLUMN_DISPLAY_NAME
};


enum {
	FILE_COLUMN_ICON,
	FILE_COLUMN_NAME,
	FILE_COLUMN_URI
};


struct _GthSlideshowPreferencesPrivate {
	GtkBuilder *builder;
	GtkWidget  *transition_combobox;
};


G_DEFINE_TYPE_WITH_CODE (GthSlideshowPreferences,
			 gth_slideshow_preferences,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthSlideshowPreferences))


static void
gth_slideshow_preferences_finalize (GObject *object)
{
	GthSlideshowPreferences *self = GTH_SLIDESHOW_PREFERENCES (object);

	g_object_unref (self->priv->builder);
	G_OBJECT_CLASS (gth_slideshow_preferences_parent_class)->finalize (object);
}


static void
gth_slideshow_preferences_class_init (GthSlideshowPreferencesClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_slideshow_preferences_finalize;
}


static void
gth_slideshow_preferences_init (GthSlideshowPreferences *self)
{
	self->priv = gth_slideshow_preferences_get_instance_private (self);
	self->priv->builder = NULL;
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
}


static void
personalize_checkbutton_toggled_cb (GtkToggleButton *button,
				    gpointer         user_data)
{
	GthSlideshowPreferences *self = user_data;

	gtk_widget_set_sensitive (_gtk_builder_get_widget (self->priv->builder, "personalize_box"),
				  gtk_toggle_button_get_active (button));
}


static void
automatic_checkbutton_toggled_cb (GtkToggleButton *button,
				  gpointer         user_data)
{
	GthSlideshowPreferences *self = user_data;

	gtk_widget_set_sensitive (_gtk_builder_get_widget (self->priv->builder, "delay_options_box"),
				  gtk_toggle_button_get_active (button));
}


static void
remove_file_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	GthSlideshowPreferences *self = user_data;
	GtkTreeModel            *model;
	GtkTreeIter              iter;

	if (! gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (_gtk_builder_get_widget (self->priv->builder, "files_treeview"))), &model, &iter))
		return;

	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}


static void
file_chooser_dialog_response_cb (GtkDialog *dialog,
				 int        response,
				 gpointer   user_data)
{
	GthSlideshowPreferences *self = user_data;

	switch (response) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		{
			GSList       *files;
			GthIconCache *icon_cache;
			GtkListStore *list_store;
			GSList       *scan;

			files = gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog));
			icon_cache = gth_icon_cache_new_for_widget(GTK_WIDGET (self), GTK_ICON_SIZE_MENU);
			list_store = (GtkListStore *) gtk_builder_get_object (self->priv->builder, "files_liststore");
			for (scan = files; scan; scan = scan->next) {
				GFile       *file = scan->data;
				GIcon       *icon;
				GdkPixbuf   *pixbuf;
				char        *uri;
				char        *name;
				GtkTreeIter  iter;

				icon = g_content_type_get_icon ("audio");
				pixbuf = gth_icon_cache_get_pixbuf (icon_cache, icon);
				uri = g_file_get_uri (file);
				name = _g_file_get_display_name (file);

				gtk_list_store_append (list_store, &iter);
				gtk_list_store_set (list_store, &iter,
						    FILE_COLUMN_ICON, pixbuf,
						    FILE_COLUMN_NAME, name,
						    FILE_COLUMN_URI, uri,
						    -1);

				g_free (name);
				g_free (uri);
				g_object_unref (pixbuf);
			}

			gth_icon_cache_free (icon_cache);
			g_slist_foreach (files, (GFunc) g_object_unref, NULL);
			g_slist_free (files);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}


static void
add_file_button_clicked_cb (GtkButton *button,
			    gpointer   user_data)
{
	GthSlideshowPreferences *self = user_data;
	GtkWidget               *dialog;
	GtkFileFilter           *filter;

	dialog = gtk_file_chooser_dialog_new (_("Choose the files to play"),
					      _gtk_widget_get_toplevel_if_window (GTK_WIDGET (self)),
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
					      _GTK_LABEL_OK, GTK_RESPONSE_OK,
					      NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_user_special_dir (G_USER_DIRECTORY_MUSIC));
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Audio files"));
	gtk_file_filter_add_mime_type (filter, "audio/*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (file_chooser_dialog_response_cb),
			  self);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_widget_show (dialog);
}


static void
gth_slideshow_preferences_construct (GthSlideshowPreferences *self,
				     const char              *current_transition,
				     gboolean                 automatic,
				     int                      delay,
				     gboolean                 wrap_around,
				     gboolean                 random_order)
{
	GtkListStore    *model;
	GtkCellRenderer *renderer;
	GList           *transitions;
	int              i, i_active;
	GList           *scan;
	GtkTreeIter      iter;

	self->priv->builder = _gtk_builder_new_from_file ("slideshow-preferences.ui", "slideshow");
	gtk_container_add (GTK_CONTAINER (self), _gtk_builder_get_widget (self->priv->builder, "preferences_page"));

	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	self->priv->transition_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->transition_combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->priv->transition_combobox),
					renderer,
					"text", TRANSITION_COLUMN_DISPLAY_NAME,
					NULL);

	transitions = gth_main_get_registered_objects (GTH_TYPE_TRANSITION);
	for (i = 0, i_active = 0, scan = transitions; scan; scan = scan->next, i++) {
		GthTransition *transition = scan->data;

		if (g_strcmp0 (gth_transition_get_id (transition), current_transition) == 0)
			i_active = i;

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    TRANSITION_COLUMN_ID, gth_transition_get_id (transition),
				    TRANSITION_COLUMN_DISPLAY_NAME, gth_transition_get_display_name (transition),
				    -1);
	}

	if (g_strcmp0 ("random", current_transition) == 0)
		i_active = i;
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    TRANSITION_COLUMN_ID, "random",
			    TRANSITION_COLUMN_DISPLAY_NAME, _("Random"),
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->transition_combobox), i_active);
	gtk_widget_show (self->priv->transition_combobox);
	gtk_box_pack_start (GTK_BOX (_gtk_builder_get_widget (self->priv->builder, "transition_box")), self->priv->transition_combobox, FALSE, FALSE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "automatic_checkbutton")), automatic);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "change_delay_spinbutton")), ((float) delay) / 1000.0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "wrap_around_checkbutton")), wrap_around);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "random_order_checkbutton")), random_order);

	/* signals */

	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "personalize_checkbutton"),
			  "toggled",
			  G_CALLBACK (personalize_checkbutton_toggled_cb),
			  self);
	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "automatic_checkbutton"),
			  "toggled",
			  G_CALLBACK (automatic_checkbutton_toggled_cb),
			  self);
	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "remove_file_button"),
			  "clicked",
			  G_CALLBACK (remove_file_button_clicked_cb),
			  self);
	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "add_file_button"),
			  "clicked",
			  G_CALLBACK (add_file_button_clicked_cb),
			  self);
}


GtkWidget *
gth_slideshow_preferences_new (const char *transition,
			       gboolean    automatic,
			       int         delay,
			       gboolean    wrap_around,
			       gboolean    random_order)
{
	GtkWidget *widget;

	widget = g_object_new (GTH_TYPE_SLIDESHOW_PREFERENCES, NULL);
	gth_slideshow_preferences_construct (GTH_SLIDESHOW_PREFERENCES (widget),
					     transition,
					     automatic,
					     delay,
					     wrap_around,
					     random_order);

	return widget;
}


void
gth_slideshow_preferences_set_audio (GthSlideshowPreferences  *self,
				     char                    **files)
{
	GthIconCache *icon_cache;
	GtkListStore *list_store;
	int           i;

	icon_cache = gth_icon_cache_new_for_widget(GTK_WIDGET (self), GTK_ICON_SIZE_MENU);
	list_store = (GtkListStore *) gtk_builder_get_object (self->priv->builder, "files_liststore");
	gtk_list_store_clear (list_store);
	for (i = 0; files[i] != NULL; i++) {
		GIcon       *icon;
		GdkPixbuf   *pixbuf;
		GFile       *file;
		char        *name;
		GtkTreeIter  iter;

		icon = g_content_type_get_icon ("audio");
		pixbuf = gth_icon_cache_get_pixbuf (icon_cache, icon);
		file = g_file_new_for_uri (files[i]);
		name = _g_file_get_display_name (file);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    FILE_COLUMN_ICON, pixbuf,
				    FILE_COLUMN_NAME, name,
				    FILE_COLUMN_URI, files[i],
				    -1);

		g_free (name);
		g_object_unref (file);
		g_object_unref (pixbuf);
	}

	gth_icon_cache_free (icon_cache);
}


GtkWidget *
gth_slideshow_preferences_get_widget (GthSlideshowPreferences *self,
				      const char              *name)
{
	if (strcmp (name, "transition_combobox") == 0)
		return self->priv->transition_combobox;
	else
		return _gtk_builder_get_widget (self->priv->builder, name);
}


gboolean
gth_slideshow_preferences_get_personalize (GthSlideshowPreferences *self)
{
	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder, "personalize_checkbutton")));
}


char *
gth_slideshow_preferences_get_transition_id (GthSlideshowPreferences *self)
{
	GtkTreeIter   iter;
	GtkTreeModel *tree_model;
	char         *transition_id;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (self->priv->transition_combobox), &iter))
		return NULL;

	tree_model = gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->transition_combobox));
	gtk_tree_model_get (tree_model, &iter, TRANSITION_COLUMN_ID, &transition_id, -1);

	return transition_id;
}

gboolean
gth_slideshow_preferences_get_automatic (GthSlideshowPreferences *self)
{
	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder, "automatic_checkbutton")));
}


int
gth_slideshow_preferences_get_delay (GthSlideshowPreferences *self)
{
	return (int) (1000.0 * gtk_spin_button_get_value (GTK_SPIN_BUTTON (gtk_builder_get_object (self->priv->builder, "change_delay_spinbutton"))));
}


gboolean
gth_slideshow_preferences_get_wrap_around (GthSlideshowPreferences *self)
{
	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder, "wrap_around_checkbutton")));
}


gboolean
gth_slideshow_preferences_get_random_order (GthSlideshowPreferences *self)
{
	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder, "random_order_checkbutton")));
}


char **
gth_slideshow_preferences_get_audio_files (GthSlideshowPreferences *self)
{
	GtkTreeModel  *tree_model;
	GtkTreeIter    iter;
	char         **files_v;
	GList         *files = NULL;

	tree_model = (GtkTreeModel *) gtk_builder_get_object (self->priv->builder, "files_liststore");
	if (gtk_tree_model_get_iter_first (tree_model, &iter)) {
		do {
			char *uri;

			gtk_tree_model_get (tree_model, &iter,
					    FILE_COLUMN_URI, &uri,
					    -1);
			files = g_list_prepend (files, uri);
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}
	files = g_list_reverse (files);
	files_v = _g_string_list_to_strv (files);

	_g_string_list_free (files);

	return files_v;
}
