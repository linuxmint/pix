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
#include <extensions/edit_metadata/gth-edit-comment-dialog.h>
#include "exiv2-utils.h"
#include "gth-edit-iptc-page.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthEditIptcPagePrivate {
	GtkBuilder *builder;
	gboolean    supported;
	GFileInfo  *info;
};


static void gth_edit_iptc_page_gth_edit_comment_page_interface_init (GthEditCommentPageInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthEditIptcPage,
			 gth_edit_iptc_page,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthEditIptcPage)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_EDIT_COMMENT_PAGE,
						gth_edit_iptc_page_gth_edit_comment_page_interface_init))


static void
set_entry_value (GthEditIptcPage *self,
		 GFileInfo        *info,
		 const char       *attribute,
		 const char       *widget_id)
{
	GthMetadata *metadata;

	metadata = (GthMetadata *) g_file_info_get_attribute_object (info, attribute);
	if (metadata != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (widget_id)), gth_metadata_get_formatted (metadata));
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (widget_id)), "");
}


static void
gth_edit_iptc_page_real_set_file_list (GthEditCommentPage *base,
		 		        GList              *file_data_list)
{
	GthEditIptcPage *self;
	GList            *scan;
	GthMetadata      *metadata;

	self = GTH_EDIT_IPTC_PAGE (base);

	self->priv->supported = TRUE;
	for (scan = file_data_list; self->priv->supported && scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		self->priv->supported = exiv2_supports_writes (gth_file_data_get_mime_type (file_data));
	}

	if (! self->priv->supported) {
		gtk_widget_hide (GTK_WIDGET (base));
		return;
	}

	_g_object_unref (self->priv->info);
	self->priv->info = gth_file_data_list_get_common_info (file_data_list, "Iptc::Application2::Copyright,Iptc::Application2::Credit,Iptc::Application2::Byline,Iptc::Application2::BylineTitle,Iptc::Application2::CountryName,Iptc::Application2::CountryCode,Iptc::Application2::City,Iptc::Application2::Language,Iptc::Application2::ObjectName,Iptc::Application2::Source,Iptc::Envelope::Destination,Iptc::Application2::Urgency,Iptc::Application2:ProvinceState");

	set_entry_value (self, self->priv->info, "Iptc::Application2::Copyright", "copyright_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::Credit", "credit_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::Byline", "byline_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::BylineTitle", "byline_title_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::CountryName", "country_name_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::CountryCode", "country_code_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::ProvinceState", "state_name_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::City", "city_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::Language", "language_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::ObjectName", "object_name_entry");
	set_entry_value (self, self->priv->info, "Iptc::Application2::Source", "source_entry");
	set_entry_value (self, self->priv->info, "Iptc::Envelope::Destination", "destination_entry");

	metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "Iptc::Application2::Urgency");
	if (metadata != NULL) {
		float v;

		if (sscanf (gth_metadata_get_formatted (metadata), "%f", &v) == 1)
			gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")), (double) v);
		else
			gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")), 0.0);
	}
	else
		gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")), 0.0);

	gtk_widget_show (GTK_WIDGET (base));
}


static void
set_attribute_from_entry (GthEditIptcPage *self,
			  GFileInfo        *info,
			  GthFileData      *file_data,
			  gboolean          only_modified_fields,
			  const char       *attribute,
			  const char       *widget_id)
{
	GthMetadata *metadata;
	const char  *value;

	value = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET (widget_id)));
	if (only_modified_fields && gth_file_data_attribute_equal (file_data, attribute, value))
		return;

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", attribute,
				 "raw", value,
				 "formatted", value,
				 NULL);
	g_file_info_set_attribute_object (info, attribute, G_OBJECT (metadata));
	g_object_unref (metadata);
}


static void
gth_edit_iptc_page_real_update_info (GthEditCommentPage *base,
				      GFileInfo          *info,
				      gboolean            only_modified_fields)
{
	GthEditIptcPage *self;
	GthFileData      *file_data;
	double            v;
	char             *s;

	self = GTH_EDIT_IPTC_PAGE (base);

	if (! self->priv->supported)
		return;

	file_data = gth_file_data_new (NULL, self->priv->info);

	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::Copyright", "copyright_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::Credit", "credit_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::Byline", "byline_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::BylineTitle", "byline_title_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::CountryName", "country_name_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::CountryCode", "country_code_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::City", "city_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::Language", "language_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::ObjectName", "object_name_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::Source", "source_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Envelope::Destination", "destination_entry");
	set_attribute_from_entry (self, info, file_data, only_modified_fields, "Iptc::Application2::ProvinceState", "state_name_entry");

	/* urgency */

	v = gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")));
	s = g_strdup_printf ("%1.g", v);
	if (! only_modified_fields || ! gth_file_data_attribute_equal_int (file_data, "Iptc::Application2::Urgency", s)) {
		GthMetadata *metadata;

		metadata = g_object_new (GTH_TYPE_METADATA,
					 "id", "Iptc::Application2::Urgency",
					 "raw", s,
					 "formatted", s,
					 NULL);
		g_file_info_set_attribute_object (info, "Iptc::Application2::Urgency", G_OBJECT (metadata));
		g_object_unref (metadata);
	}
	g_free (s);

	g_object_unref (file_data);
}


static const char *
gth_edit_iptc_page_real_get_name (GthEditCommentPage *self)
{
	return _("Other");
}


static void
gth_edit_iptc_page_finalize (GObject *object)
{
	GthEditIptcPage *self;

	self = GTH_EDIT_IPTC_PAGE (object);

	_g_object_unref (self->priv->info);
	g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_edit_iptc_page_parent_class)->finalize (object);
}


static void
gth_edit_iptc_page_class_init (GthEditIptcPageClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_edit_iptc_page_finalize;
}


static void
gth_edit_iptc_page_init (GthEditIptcPage *self)
{
	self->priv = gth_edit_iptc_page_get_instance_private (self);
	self->priv->info = NULL;

	gtk_container_set_border_width (GTK_CONTAINER (self), 12);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

	self->priv->builder = _gtk_builder_new_from_file ("edit-exiv2-page.ui", "exiv2_tools");
  	gtk_box_pack_start (GTK_BOX (self), _gtk_builder_get_widget (self->priv->builder, "content"), TRUE, TRUE, 0);
}


static void
gth_edit_iptc_page_gth_edit_comment_page_interface_init (GthEditCommentPageInterface *iface)
{
	iface->set_file_list = gth_edit_iptc_page_real_set_file_list;
	iface->update_info = gth_edit_iptc_page_real_update_info;
	iface->get_name = gth_edit_iptc_page_real_get_name;
}
