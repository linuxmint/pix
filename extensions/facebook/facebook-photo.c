/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010-2012 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include <gthumb.h>
#include "facebook-photo.h"


static void facebook_photo_json_serializable_interface_init (JsonSerializableIface *iface);


G_DEFINE_TYPE_WITH_CODE (FacebookPhoto,
			 facebook_photo,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
					 	facebook_photo_json_serializable_interface_init))


enum {
        PROP_0,
        PROP_ID,
        PROP_PICTURE,
        PROP_SOURCE,
        PROP_WIDTH,
        PROP_HEIGHT,
        PROP_LINK,
        PROP_CREATED_TIME,
        PROP_UPDATED_TIME,
        PROP_IMAGES
};


/* -- facebook_image -- */


static FacebookImage *
facebook_image_new (void)
{
	FacebookImage *image;

	image = g_new (FacebookImage, 1);
	image->source = NULL;
	image->width = 0;
	image->height = 0;

	return image;
}


static FacebookImage *
facebook_image_copy (FacebookImage *source)
{
	FacebookImage *dest;

	dest = facebook_image_new ();
	_g_strset (&dest->source, source->source);
	dest->width = source->width;
	dest->height = source->height;

	return dest;
}


static void
facebook_image_free (FacebookImage *image)
{
	g_free (image->source);
	g_free (image);
}


/* -- facebook_image_list -- */


#define FACEBOOK_TYPE_IMAGE_LIST (facebook_image_list_get_type ())


static GList *
facebook_image_list_copy (GList *source)
{
	return g_list_copy_deep (source, (GCopyFunc) facebook_image_copy, NULL);
}


static void
facebook_image_list_free (GList *images)
{
	g_list_foreach (images, (GFunc) facebook_image_free, NULL);
	g_list_free (images);
}


G_DEFINE_BOXED_TYPE (GList, facebook_image_list, facebook_image_list_copy, facebook_image_list_free)


/* -- facebook_photo -- */


static void
facebook_photo_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	FacebookPhoto *self;

	self = FACEBOOK_PHOTO (object);

	switch (property_id) {
	case PROP_ID:
		_g_strset (&self->id, g_value_get_string (value));
		break;
	case PROP_PICTURE:
		_g_strset (&self->picture, g_value_get_string (value));
		break;
	case PROP_SOURCE:
		_g_strset (&self->source, g_value_get_string (value));
		break;
	case PROP_WIDTH:
		self->width = g_value_get_int (value);
		break;
	case PROP_HEIGHT:
		self->height = g_value_get_int (value);
		break;
	case PROP_LINK:
		_g_strset (&self->link, g_value_get_string (value));
		break;
	case PROP_CREATED_TIME:
		gth_datetime_free (self->created_time);
		self->created_time = g_value_dup_boxed (value);
		break;
	case PROP_UPDATED_TIME:
		gth_datetime_free (self->updated_time);
		self->updated_time = g_value_dup_boxed (value);
		break;
	case PROP_IMAGES:
		facebook_image_list_free (self->images);
		self->images = g_value_dup_boxed (value);
		break;
	default:
		break;
	}
}


static void
facebook_photo_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	FacebookPhoto *self;

	self = FACEBOOK_PHOTO (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->id);
		break;
	case PROP_PICTURE:
		g_value_set_string (value, self->picture);
		break;
	case PROP_SOURCE:
		g_value_set_string (value, self->source);
		break;
	case PROP_WIDTH:
		g_value_set_int (value, self->width);
		break;
	case PROP_HEIGHT:
		g_value_set_int (value, self->height);
		break;
	case PROP_LINK:
		g_value_set_string (value, self->link);
		break;
	case PROP_CREATED_TIME:
		g_value_set_boxed (value, self->created_time);
		break;
	case PROP_UPDATED_TIME:
		g_value_set_boxed (value, self->updated_time);
		break;
	case PROP_IMAGES:
		g_value_set_boxed (value, self->images);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
facebook_photo_finalize (GObject *obj)
{
	FacebookPhoto *self;

	self = FACEBOOK_PHOTO (obj);

	g_free (self->id);
	g_free (self->picture);
	g_free (self->source);
	g_free (self->link);
	gth_datetime_free (self->created_time);
	gth_datetime_free (self->updated_time);
	facebook_image_list_free (self->images);

	G_OBJECT_CLASS (facebook_photo_parent_class)->finalize (obj);
}


static void
facebook_photo_class_init (FacebookPhotoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = facebook_photo_finalize;
	object_class->set_property = facebook_photo_set_property;
	object_class->get_property = facebook_photo_get_property;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
                                                              "ID",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_PICTURE,
					 g_param_spec_string ("picture",
                                                              "Picture",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
				 	 PROP_SOURCE,
					 g_param_spec_string ("source",
                                                              "Source",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_WIDTH,
					 g_param_spec_int ("width",
                                                           "Width",
                                                           "",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_HEIGHT,
					 g_param_spec_int ("height",
                                                           "Height",
                                                           "",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
				 	 PROP_LINK,
					 g_param_spec_string ("link",
                                                              "Link",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
				 	 PROP_CREATED_TIME,
					 g_param_spec_boxed ("created-time",
                                                             "Created time",
                                                             "",
                                                             GTH_TYPE_DATETIME,
                                                             G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
				 	 PROP_UPDATED_TIME,
				 	 g_param_spec_boxed ("updated-time",
				 			     "Updated time",
				 			     "",
				 			     GTH_TYPE_DATETIME,
				 			     G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
				 	 PROP_IMAGES,
				 	 g_param_spec_boxed ("images",
				 			     "Images",
				 			     "",
				 			     FACEBOOK_TYPE_IMAGE_LIST,
				 			     G_PARAM_READWRITE));
}


static gboolean
facebook_photo_deserialize_property (JsonSerializable *serializable,
                                     const gchar      *property_name,
                                     GValue           *value,
                                     GParamSpec       *pspec,
                                     JsonNode         *property_node)
{
	FacebookPhoto *self = FACEBOOK_PHOTO (serializable);

	if (pspec->value_type == GTH_TYPE_DATETIME) {
		GTimeVal timeval;

		if (g_time_val_from_iso8601 (json_node_get_string (property_node), &timeval)) {
			GthDateTime *datetime;

			datetime = gth_datetime_new ();
			gth_datetime_from_timeval (datetime, &timeval);
			g_object_set (self, property_name, datetime, NULL);

			gth_datetime_free (datetime);

			return TRUE;
		}

		return FALSE;
	}

	if (pspec->value_type == FACEBOOK_TYPE_IMAGE_LIST) {
		GList     *images = NULL;
		JsonArray *array;
		int        i;

		array = json_node_get_array (property_node);
		for (i = 0; i < json_array_get_length (array); i++) {
			JsonObject *image_obj;

			image_obj = json_array_get_object_element (array, i);
			if (image_obj != NULL) {
				FacebookImage *image;

				image = facebook_image_new ();
				_g_strset (&image->source, json_object_get_string_member (image_obj, "source"));
				image->width = json_object_get_int_member (image_obj, "width");
				image->height = json_object_get_int_member (image_obj, "height");

				images = g_list_prepend (images, image);
			}
		}

		images = g_list_reverse (images);
		g_object_set (self, property_name, images, NULL);

		facebook_image_list_free (images);

		return TRUE;
	}

	return json_serializable_default_deserialize_property (serializable,
							       property_name,
							       value,
							       pspec,
							       property_node);
}


static void
facebook_photo_json_serializable_interface_init (JsonSerializableIface *iface)
{
	iface->deserialize_property = facebook_photo_deserialize_property;
}


static void
facebook_photo_init (FacebookPhoto *self)
{
	self->id = NULL;
	self->picture = NULL;
	self->source = NULL;
	self->width = 0;
	self->height = 0;
	self->link = NULL;
	self->created_time = NULL;
	self->updated_time = NULL;
	self->images = NULL;
}


FacebookPhoto *
facebook_photo_new (void)
{
	return g_object_new (FACEBOOK_TYPE_PHOTO, NULL);
}


const char *
facebook_photo_get_original_url	(FacebookPhoto *photo)
{
	char   *url;
	GList  *scan;
	glong   max_size;

	url = photo->source;
	max_size = photo->width * photo->height;

	for (scan = photo->images; scan; scan = scan->next) {
		FacebookImage *image = scan->data;
		glong          image_size;

		image_size = image->width * image->height;
		if (image_size > max_size) {
			max_size = image_size;
			url = image->source;
		}
	}

	return url;
}


const char *
facebook_photo_get_thumbnail_url (FacebookPhoto *photo,
				  int            requested_size)
{
	char   *url;
	GList  *scan;
	glong   min_delta;

	url = photo->picture;
	requested_size = requested_size * requested_size;
	min_delta = 0;

	for (scan = photo->images; scan; scan = scan->next) {
		FacebookImage *image = scan->data;
		glong          image_delta;

		image_delta = labs ((image->width * image->height) - requested_size);
		if ((scan == photo->images) || (image_delta < min_delta)) {
			min_delta = image_delta;
			url = image->source;
		}
	}

	return url;
}
