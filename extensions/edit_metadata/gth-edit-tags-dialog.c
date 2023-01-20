/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-edit-metadata-dialog.h"
#include "gth-edit-tags-dialog.h"
#include "utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthEditTagsDialogPrivate {
	GtkBuilder *builder;
	GtkWidget  *tags_entry;
};


static void gth_edit_tags_dialog_gth_edit_metadata_dialog_interface_init (GthEditMetadataDialogInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthEditTagsDialog,
			 gth_edit_tags_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthEditTagsDialog)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_EDIT_METADATA_DIALOG,
						gth_edit_tags_dialog_gth_edit_metadata_dialog_interface_init))


static void
gth_edit_tags_dialog_finalize (GObject *object)
{
	GthEditTagsDialog *self;

	self = GTH_EDIT_TAGS_DIALOG (object);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_edit_tags_dialog_parent_class)->finalize (object);
}


static void
gth_edit_tags_dialog_set_file_list (GthEditMetadataDialog *base,
				    GList                 *file_list)
{
	GthEditTagsDialog *self = GTH_EDIT_TAGS_DIALOG (base);
	GHashTable        *common_tags;
	GHashTable        *no_common_tags;
	GList             *common_tags_list;
	GList             *no_common_tags_list;

	/* update the tag entry */

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
}


static void
gth_edit_tags_dialog_update_info (GthEditMetadataDialog *base,
				  GList                 *file_list /* GthFileData list */)
{
	GthEditTagsDialog *self = GTH_EDIT_TAGS_DIALOG (base);
	GList             *checked_tags;
	GList             *inconsistent_tags;
	GList             *scan;

	gth_tags_entry_get_tag_list (GTH_TAGS_ENTRY (self->priv->tags_entry),
				     TRUE,
				     &checked_tags,
				     &inconsistent_tags);

	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GList       *new_tags;
		GthMetadata *metadata;
		GHashTable  *old_tags;
		GList       *scan_tags;

		new_tags = _g_string_list_dup (checked_tags);

		/* keep the inconsistent tags */

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::tags");
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
			g_file_info_set_attribute_object (file_data->info, "general::tags", G_OBJECT (metadata));

			g_object_unref (metadata);
			g_object_unref (file_tags);
			_g_string_list_free (new_tags);
		}
		else
			g_file_info_remove_attribute (file_data->info, "general::tags");
	}

	g_list_free (inconsistent_tags);
	_g_string_list_free (checked_tags);
}


static void
gth_edit_tags_dialog_gth_edit_metadata_dialog_interface_init (GthEditMetadataDialogInterface *iface)
{
	iface->set_file_list = gth_edit_tags_dialog_set_file_list;
	iface->update_info = gth_edit_tags_dialog_update_info;
}


static void
gth_edit_tags_dialog_class_init (GthEditTagsDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_edit_tags_dialog_finalize;
}


static void
gth_edit_tags_dialog_init (GthEditTagsDialog *self)
{
	self->priv = gth_edit_tags_dialog_get_instance_private (self);
	self->priv->builder = _gtk_builder_new_from_file ("tag-chooser.ui", "edit_metadata");

	gtk_window_set_title (GTK_WINDOW (self), _("Tags"));
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (self), -1, 500);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	self->priv->tags_entry = gth_tags_entry_new (GTH_TAGS_ENTRY_MODE_INLINE);
	gth_tags_entry_set_list_visible (GTH_TAGS_ENTRY (self->priv->tags_entry), TRUE);
	gtk_widget_set_size_request (self->priv->tags_entry, 400, -1);
	gtk_widget_show (self->priv->tags_entry);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("tag_entry_box")), self->priv->tags_entry, TRUE, TRUE, 0);

	gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), GET_WIDGET ("content"), TRUE, TRUE, 0);
}
