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
#include <gio/gio.h>
#include "gth-edit-comment-dialog.h"
#include "gth-edit-general-page.h"
#include "utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


typedef enum {
	NO_DATE = 0,
	FOLLOWING_DATE,
	CURRENT_DATE,
	PHOTO_DATE,
	LAST_MODIFIED_DATE,
	CREATION_DATE,
	NO_CHANGE
} DateOption;


struct _GthEditGeneralPagePrivate {
	GFileInfo  *info;
	GtkBuilder *builder;
	GtkWidget  *date_combobox;
	GtkWidget  *date_selector;
	GtkWidget  *tags_entry;
	GTimeVal    current_date;
};


static void gth_edit_general_page_gth_edit_general_page_interface_init (GthEditCommentPageInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthEditGeneralPage,
			 gth_edit_general_page,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthEditGeneralPage)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_EDIT_COMMENT_PAGE,
						gth_edit_general_page_gth_edit_general_page_interface_init))


static void
gth_edit_general_page_real_set_file_list (GthEditCommentPage *base,
		 			  GList              *file_list)
{
	GthEditGeneralPage  *self;
	GtkTextBuffer       *buffer;
	GthMetadata         *metadata;
	GthMetadataProvider *provider;
	GHashTable          *common_tags;
	GHashTable          *no_common_tags;
	GList               *common_tags_list;
	GList               *no_common_tags_list;
	gboolean             no_provider;
	GthFileData         *file_data;
	const char          *mime_type;

	self = GTH_EDIT_GENERAL_PAGE (base);

	/* get the metadata common to the seleted files */

	_g_object_unref (self->priv->info);
	self->priv->info = gth_file_data_list_get_common_info (file_list, "general::description,general::title,general::location,general::datetime,general::tags,general::rating");

	/* description */

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (GET_WIDGET ("note_text")));
	metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "general::description");
	if (metadata != NULL) {
		GtkTextIter iter;

		gtk_text_buffer_set_text (buffer, gth_metadata_get_formatted (metadata), -1);
		gtk_text_buffer_get_iter_at_line (buffer, &iter, 0);
		gtk_text_buffer_place_cursor (buffer, &iter);
	}
	else
		gtk_text_buffer_set_text (buffer, "", -1);

	/* title */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "general::title");
	if (metadata != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("title_entry")), gth_metadata_get_formatted (metadata));
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("title_entry")), "");

	/* location */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "general::location");
	if (metadata != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("place_entry")), gth_metadata_get_formatted (metadata));
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("place_entry")), "");

	/* date */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "general::datetime");
	if (metadata != NULL) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->date_combobox), FOLLOWING_DATE);
		gtk_widget_set_sensitive (self->priv->date_combobox, TRUE);
		gth_time_selector_set_exif_date (GTH_TIME_SELECTOR (self->priv->date_selector), gth_metadata_get_raw (metadata));
	}
	else {
		if ((file_list != NULL) && (file_list->next == NULL))
			gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->date_combobox), NO_DATE);
		else
			gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->date_combobox), NO_CHANGE);
		gtk_widget_set_sensitive (self->priv->date_combobox, FALSE);
		gth_time_selector_set_exif_date (GTH_TIME_SELECTOR (self->priv->date_selector), "");
	}

	/* tags */

	utils_get_common_tags (file_list, &common_tags, &no_common_tags);
	common_tags_list = g_hash_table_get_keys (common_tags);
	no_common_tags_list = g_hash_table_get_keys (no_common_tags);
	gth_tags_entry_set_tag_list (GTH_TAGS_ENTRY (self->priv->tags_entry),
				     common_tags_list,
				     no_common_tags_list);

	g_list_free (no_common_tags_list);
	g_list_free (common_tags_list);
	g_hash_table_unref (no_common_tags);
	g_hash_table_unref (common_tags);

	/* rating */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "general::rating");
	if (metadata != NULL) {
		int v;

		sscanf (gth_metadata_get_raw (metadata), "%d", &v);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("rating_spinbutton")), v);
	}
	else
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("rating_spinbutton")), 0);

	gtk_widget_grab_focus (GET_WIDGET ("note_text"));

	/* set a widget insensitive if there is no way to save the relative
	 * metadata */

	no_provider = TRUE;

	if (file_list == NULL) {
		file_data = gth_file_data_new (NULL, NULL);
	}
	else if (file_list->next == NULL) {
		GthFileData *first = file_list->data;
		file_data = gth_file_data_new (first->file, first->info);
	}
	else
		file_data = gth_file_data_new (NULL, ((GthFileData *) file_list->data)->info);

	mime_type = gth_file_data_get_mime_type (file_data);

  	provider = gth_main_get_metadata_writer ("general::description", mime_type);
	gtk_widget_set_sensitive (GET_WIDGET ("note_text"), provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::location", mime_type);
	gtk_widget_set_sensitive (GET_WIDGET ("place_entry"), provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::datetime", mime_type);
	gtk_widget_set_sensitive (self->priv->date_combobox, provider != NULL);
	if (provider == NULL)
		gtk_widget_set_sensitive (self->priv->date_selector, FALSE);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::tags", mime_type);
	gtk_widget_set_sensitive (self->priv->tags_entry, provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::rating", mime_type);
	gtk_widget_set_sensitive (GET_WIDGET ("rating_spinbutton"), provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	/* hide the whole page if no metadata can be saved */

	if (no_provider)
		gtk_widget_hide (GTK_WIDGET (self));
	else
		gtk_widget_show (GTK_WIDGET (self));

	g_object_unref (file_data);
}


static char *
get_date_from_option (GthEditGeneralPage *self,
		      DateOption          option,
		      GFileInfo          *info)
{
	GTimeVal     timeval;
	GthDateTime *date_time;
	char        *exif_date;
	GthMetadata *metadata;

	_g_time_val_reset (&timeval);

	switch (option) {
	case NO_DATE:
		return g_strdup ("");

	case FOLLOWING_DATE:
		date_time = gth_datetime_new ();
		gth_time_selector_get_value (GTH_TIME_SELECTOR (self->priv->date_selector), date_time);
		exif_date = gth_datetime_to_exif_date (date_time);
		_g_time_val_from_exif_date (exif_date, &timeval);
		g_free (exif_date);
		gth_datetime_free (date_time);
		break;

	case CURRENT_DATE:
		g_get_current_time (&self->priv->current_date);
		timeval = self->priv->current_date;
		break;

	case PHOTO_DATE:
		metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "Embedded::Photo::DateTimeOriginal");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &timeval);
		else
			return g_strdup ("");
		break;

	case LAST_MODIFIED_DATE:
		timeval.tv_sec = g_file_info_get_attribute_uint64 (info, "time::modified");
		timeval.tv_usec = g_file_info_get_attribute_uint32 (info, "time::modified-usec");
		break;

	case CREATION_DATE:
		timeval.tv_sec = g_file_info_get_attribute_uint64 (info, "time::created");
		timeval.tv_usec = g_file_info_get_attribute_uint32 (info, "time::created-usec");
		break;

	case NO_CHANGE:
		metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "general::datetime");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &timeval);
		else
			return g_strdup ("");
		break;
	}

	return _g_time_val_to_exif_date (&timeval);
}


static void
gth_edit_general_page_real_update_info (GthEditCommentPage *base,
					GFileInfo          *info,
					gboolean            only_modified_fields)
{
	GthEditGeneralPage  *self;
	GthFileData         *file_data;
	GtkTextBuffer       *buffer;
	GtkTextIter          start;
	GtkTextIter          end;
	char                *text;
	GthMetadata         *metadata;
	char                *s;

	self = GTH_EDIT_GENERAL_PAGE (base);

	file_data = gth_file_data_new (NULL, self->priv->info);

	/* caption */

	if (! only_modified_fields || ! gth_file_data_attribute_equal (file_data, "general::title", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))))) {
		metadata = g_object_new (GTH_TYPE_METADATA,
					 "id", "general::title",
					 "raw", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))),
					 "formatted", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))),
					 NULL);
		g_file_info_set_attribute_object (info, "general::title", G_OBJECT (metadata));
		g_object_unref (metadata);
	}

	/* comment */

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (GET_WIDGET ("note_text")));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	if (! only_modified_fields || ! gth_file_data_attribute_equal (file_data, "general::description", text)) {
		metadata = g_object_new (GTH_TYPE_METADATA,
					 "id", "general::description",
					 "raw", text,
					 "formatted", text,
					 NULL);
		g_file_info_set_attribute_object (info, "general::description", G_OBJECT (metadata));
		g_object_unref (metadata);
	}
	g_free (text);

	/* location */

	if (! only_modified_fields || ! gth_file_data_attribute_equal (file_data, "general::location", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("place_entry"))))) {
		metadata = g_object_new (GTH_TYPE_METADATA,
					 "id", "general::location",
					 "raw", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("place_entry"))),
					 "formatted", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("place_entry"))),
					 NULL);
		g_file_info_set_attribute_object (info, "general::location", G_OBJECT (metadata));
		g_object_unref (metadata);
	}

	/* date */

	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->date_combobox))) {
	case NO_CHANGE:
		break;

	case NO_DATE:
		g_file_info_remove_attribute (info, "general::datetime");
		break;

	default:
		{
			char *exif_date;

			if (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->date_combobox)) == CURRENT_DATE)
				exif_date = _g_time_val_to_exif_date (&self->priv->current_date);
			else
				exif_date = get_date_from_option (self, gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->date_combobox)), info);
			if (! only_modified_fields
			    || ! gth_file_data_attribute_equal (file_data, "general::datetime", exif_date))
			{
				metadata = g_object_new (GTH_TYPE_METADATA,
							 "id", "general::datetime",
							 "raw", exif_date,
							 "formatted", exif_date,
							 NULL);
				g_file_info_set_attribute_object (info, "general::datetime", G_OBJECT (metadata));
				g_object_unref (metadata);
			}

			g_free (exif_date);

			break;
		}
	}

	/* tags */

	if (only_modified_fields) {
		GList       *checked_tags;
		GList       *inconsistent_tags;
		GList       *new_tags;
		GHashTable  *old_tags;
		GList       *scan_tags;

		gth_tags_entry_get_tag_list (GTH_TAGS_ENTRY (self->priv->tags_entry),
					     TRUE,
					     &checked_tags,
					     &inconsistent_tags);

		new_tags = _g_string_list_dup (checked_tags);

		/* keep the inconsistent tags */

		metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "general::tags");
		old_tags = _g_hash_table_from_string_list (gth_metadata_get_string_list (metadata));
		for (scan_tags = inconsistent_tags; scan_tags; scan_tags = scan_tags->next) {
			char *inconsistent_tag = scan_tags->data;

			if (g_hash_table_lookup (old_tags, inconsistent_tag) != NULL)
				new_tags = g_list_prepend (new_tags, g_strdup (inconsistent_tag));
		}
		g_hash_table_unref (old_tags);

		/* update the general::tags attribute */

		if (new_tags != NULL) {
			GthStringList *file_tags;
			GthMetadata   *metadata;

			new_tags = g_list_sort (new_tags, (GCompareFunc) g_strcmp0);
			file_tags = gth_string_list_new (new_tags);
			metadata = gth_metadata_new_for_string_list (file_tags);
			g_file_info_set_attribute_object (info, "general::tags", G_OBJECT (metadata));

			g_object_unref (metadata);
			g_object_unref (file_tags);
			_g_string_list_free (new_tags);
		}
		else
			g_file_info_remove_attribute (info, "general::tags");

		g_list_free (inconsistent_tags);
		_g_string_list_free (checked_tags);
	}
	else {
		char          **tagv;
		GList          *tags;
		int             i;
		GthStringList  *string_list;

		tagv = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (self->priv->tags_entry), TRUE);
		tags = NULL;
		for (i = 0; tagv[i] != NULL; i++)
			tags = g_list_prepend (tags, tagv[i]);
		tags = g_list_reverse (tags);
		if (tags != NULL)
			string_list = gth_string_list_new (tags);
		else
			string_list = NULL;

		if (string_list != NULL) {
			metadata = gth_metadata_new_for_string_list (string_list);
			g_file_info_set_attribute_object (info, "general::tags", G_OBJECT (metadata));

			g_object_unref (metadata);
		}
		else
			g_file_info_remove_attribute (info, "general::tags");

		_g_object_unref (string_list);
		g_list_free (tags);
		g_strfreev (tagv);
	}

	/* rating */

	s = g_strdup_printf ("%d", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("rating_spinbutton"))));
	if (! only_modified_fields || ! gth_file_data_attribute_equal_int (file_data, "general::rating", s)) {
		if (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("rating_spinbutton"))) > 0) {
			metadata = g_object_new (GTH_TYPE_METADATA,
						 "id", "general::rating",
						 "raw", s,
						 "formatted", s,
						 NULL);
			g_file_info_set_attribute_object (info, "general::rating", G_OBJECT (metadata));
			g_object_unref (metadata);
		}
		else
			g_file_info_remove_attribute (info, "general::rating");
	}

	g_free (s);
	g_object_unref (file_data);
}


static const char *
gth_edit_general_page_real_get_name (GthEditCommentPage *self)
{
	return _("General");
}


static void
gth_edit_general_page_finalize (GObject *object)
{
	GthEditGeneralPage *self;

	self = GTH_EDIT_GENERAL_PAGE (object);

	_g_object_unref (self->priv->info);
	g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_edit_general_page_parent_class)->finalize (object);
}


static void
gth_edit_general_page_class_init (GthEditGeneralPageClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_edit_general_page_finalize;
}


static void
date_combobox_changed_cb (GtkComboBox *widget,
			  gpointer     user_data)
{
	GthEditGeneralPage *self = user_data;
	char               *value;

	value = get_date_from_option (self, gtk_combo_box_get_active (widget), self->priv->info);
	gth_time_selector_set_exif_date (GTH_TIME_SELECTOR (self->priv->date_selector), value);
	gtk_widget_set_sensitive (self->priv->date_selector, ! g_str_equal (value, ""));

	g_free (value);
}


static void
tags_entry_list_collapsed_cb (GthTagsEntry *widget,
			      gpointer      user_data)
{
	GtkWindow *toplevel;
	int        width;

	/* collapse the dialog height */

	toplevel = _gtk_widget_get_toplevel_if_window (GTK_WIDGET (widget));
	if (toplevel == NULL)
		return;

	gtk_window_get_size (toplevel, &width, NULL);
	gtk_window_resize (toplevel, width, 1);
}


static void
gth_edit_general_page_init (GthEditGeneralPage *self)
{
	self->priv = gth_edit_general_page_get_instance_private (self);
	self->priv->info = NULL;

	gtk_container_set_border_width (GTK_CONTAINER (self), 12);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

	self->priv->builder = _gtk_builder_new_from_file ("edit-comment-page.ui", "edit_metadata");
	gtk_box_pack_start (GTK_BOX (self), _gtk_builder_get_widget (self->priv->builder, "content"), TRUE, TRUE, 0);

	self->priv->date_combobox = gtk_combo_box_text_new ();
	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->date_combobox),
				     _("No date"),
				     _("The following date"),
				     _("Current date"),
				     _("Date photo was taken"),
				     _("Last modified date"),
				     _("File creation date"),
				     _("Do not modify"),
				     NULL);
	gtk_widget_show (self->priv->date_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_combobox_container")), self->priv->date_combobox, TRUE, TRUE, 0);

	g_signal_connect (self->priv->date_combobox,
			  "changed",
			  G_CALLBACK (date_combobox_changed_cb),
			  self);

	self->priv->date_selector = gth_time_selector_new ();
	gtk_widget_show (self->priv->date_selector);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_selector_container")), self->priv->date_selector, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("date_label")), self->priv->date_combobox);

	self->priv->tags_entry = gth_tags_entry_new (GTH_TAGS_ENTRY_MODE_POPUP);
	gtk_widget_show (self->priv->tags_entry);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("tags_entry_container")), self->priv->tags_entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("tags_label")), self->priv->tags_entry);

	g_signal_connect (self->priv->tags_entry,
			  "list-collapsed",
			  G_CALLBACK (tags_entry_list_collapsed_cb),
			  self);
}


static void
gth_edit_general_page_gth_edit_general_page_interface_init (GthEditCommentPageInterface *iface)
{
	iface->set_file_list = gth_edit_general_page_real_set_file_list;
	iface->update_info = gth_edit_general_page_real_update_info;
	iface->get_name = gth_edit_general_page_real_get_name;
}
