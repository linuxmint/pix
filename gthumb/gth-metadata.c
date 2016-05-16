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
#include "glib-utils.h"
#include "gth-metadata.h"


enum  {
	GTH_METADATA_DUMMY_PROPERTY,
	GTH_METADATA_ID,
	GTH_METADATA_DESCRIPTION,
	GTH_METADATA_RAW,
	GTH_METADATA_STRING_LIST,
	GTH_METADATA_FORMATTED,
	GTH_METADATA_VALUE_TYPE
};


struct _GthMetadataPrivate {
	GthMetadataType  data_type;
	char            *id;
	char            *description;
	char            *raw;
	GthStringList   *list;
	char            *formatted;
	char            *value_type;
};


G_DEFINE_TYPE (GthMetadata, gth_metadata, G_TYPE_OBJECT)


static void
gth_metadata_get_property (GObject    *object,
			   guint       property_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GthMetadata *self;

	self = GTH_METADATA (object);
	switch (property_id) {
	case GTH_METADATA_ID:
		g_value_set_string (value, self->priv->id);
		break;
	case GTH_METADATA_DESCRIPTION:
		g_value_set_string (value, self->priv->description);
		break;
	case GTH_METADATA_RAW:
		g_value_set_string (value, self->priv->raw);
		break;
	case GTH_METADATA_STRING_LIST:
		g_value_set_object (value, self->priv->list);
		break;
	case GTH_METADATA_FORMATTED:
		g_value_set_string (value, self->priv->formatted);
		break;
	case GTH_METADATA_VALUE_TYPE:
		g_value_set_string (value, self->priv->value_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_metadata_set_property (GObject      *object,
			   guint         property_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GthMetadata *self;

	self = GTH_METADATA (object);
	switch (property_id) {
	case GTH_METADATA_ID:
		_g_strset (&self->priv->id, g_value_get_string (value));
		break;
	case GTH_METADATA_DESCRIPTION:
		_g_strset (&self->priv->description, g_value_get_string (value));
		break;
	case GTH_METADATA_RAW:
		_g_strset (&self->priv->raw, g_value_get_string (value));
		break;
	case GTH_METADATA_STRING_LIST:
		_g_object_unref (self->priv->list);
		self->priv->list = gth_string_list_new (gth_string_list_get_list (GTH_STRING_LIST (g_value_get_object (value))));
		self->priv->data_type = (self->priv->list != NULL) ? GTH_METADATA_TYPE_STRING_LIST : GTH_METADATA_TYPE_STRING;
		break;
	case GTH_METADATA_FORMATTED:
		_g_strset (&self->priv->formatted, g_value_get_string (value));
		break;
	case GTH_METADATA_VALUE_TYPE:
		_g_strset (&self->priv->value_type, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_metadata_finalize (GObject *obj)
{
	GthMetadata *self;

	self = GTH_METADATA (obj);

	g_free (self->priv->id);
	g_free (self->priv->description);
	g_free (self->priv->raw);
	_g_object_unref (self->priv->list);
	g_free (self->priv->formatted);
	g_free (self->priv->value_type);

	G_OBJECT_CLASS (gth_metadata_parent_class)->finalize (obj);
}


static void
gth_metadata_class_init (GthMetadataClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthMetadataPrivate));

	G_OBJECT_CLASS (klass)->get_property = gth_metadata_get_property;
	G_OBJECT_CLASS (klass)->set_property = gth_metadata_set_property;
	G_OBJECT_CLASS (klass)->finalize = gth_metadata_finalize;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_ID,
					 g_param_spec_string ("id",
					 		      "ID",
					 		      "Metadata unique identifier",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_DESCRIPTION,
					 g_param_spec_string ("description",
					 		      "Description",
					 		      "Metadata description",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_RAW,
					 g_param_spec_string ("raw",
					 		      "Raw value",
					 		      "Metadata raw value",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_STRING_LIST,
					 g_param_spec_object ("string-list",
					 		      "String list",
					 		      "Metadata value as a list of strings (if available)",
					 		      GTH_TYPE_STRING_LIST,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_FORMATTED,
					 g_param_spec_string ("formatted",
					 		      "Formatted value",
					 		      "Metadata formatted value",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_VALUE_TYPE,
					 g_param_spec_string ("value-type",
					 		      "Type",
					 		      "Metadata type description",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
gth_metadata_init (GthMetadata *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_METADATA, GthMetadataPrivate);
	self->priv->data_type = GTH_METADATA_TYPE_STRING;
	self->priv->id = NULL;
	self->priv->description = NULL;
	self->priv->raw = NULL;
	self->priv->list = NULL;
	self->priv->formatted = NULL;
	self->priv->value_type = NULL;
}


GthMetadata *
gth_metadata_new (void)
{
	return g_object_new (GTH_TYPE_METADATA, NULL);
}


GthMetadata *
gth_metadata_new_for_string_list (GthStringList *list)
{
	return g_object_new (GTH_TYPE_METADATA, "string-list", list, NULL);
}


GthMetadataType
gth_metadata_get_data_type (GthMetadata *metadata)
{
	return metadata->priv->data_type;
}


const char *
gth_metadata_get_id (GthMetadata *metadata)
{
	return metadata->priv->id;
}


const char *
gth_metadata_get_raw (GthMetadata *metadata)
{
	return metadata->priv->raw;
}


GthStringList *
gth_metadata_get_string_list (GthMetadata *metadata)
{
	if (metadata == NULL)
		return NULL;
	if (metadata->priv->data_type == GTH_METADATA_TYPE_STRING_LIST)
		return metadata->priv->list;
	else
		return NULL;
}


const char *
gth_metadata_get_formatted (GthMetadata *metadata)
{
	return metadata->priv->formatted;
}


const char *
gth_metadata_get_value_type (GthMetadata *metadata)
{
	return metadata->priv->value_type;
}


GthMetadata *
gth_metadata_dup (GthMetadata *metadata)
{
	GthMetadata *new_metadata;

	new_metadata = gth_metadata_new ();
	g_object_set (new_metadata,
		      "id", metadata->priv->id,
		      "description", metadata->priv->description,
		      "raw", metadata->priv->raw,
		      "string-list", metadata->priv->list,
		      "formatted", metadata->priv->formatted,
		      "value-type", metadata->priv->value_type,
		      NULL);

	return new_metadata;
}


GthMetadataInfo *
gth_metadata_info_dup (GthMetadataInfo *info)
{
	GthMetadataInfo *new_info;

	new_info = g_new0 (GthMetadataInfo, 1);
	if (info->id != NULL)
		new_info->id = g_strdup (info->id);
	if (info->type != NULL)
		new_info->type = g_strdup (info->type);
	if (info->display_name != NULL)
		new_info->display_name = g_strdup (info->display_name);
	if (info->category != NULL)
		new_info->category = g_strdup (info->category);
	new_info->sort_order = info->sort_order;
	new_info->flags = info->flags;

	return new_info;
}


void
set_attribute_from_string (GFileInfo  *info,
			   const char *key,
			   const char *raw,
			   const char *formatted)
{
	GthMetadata *metadata;

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", key,
				 "raw", raw,
				 "formatted", (formatted != NULL ? formatted : raw),
				 NULL);
	g_file_info_set_attribute_object (info, key, G_OBJECT (metadata));

	g_object_unref (metadata);
}
