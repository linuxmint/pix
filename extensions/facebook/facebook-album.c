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
#include <stdlib.h>
#include <string.h>
#include <gthumb.h>
#include "facebook-album.h"


G_DEFINE_TYPE (FacebookAlbum, facebook_album, G_TYPE_OBJECT)


enum {
        PROP_0,
        PROP_ID,
        PROP_NAME,
        PROP_DESCRIPTION,
        PROP_LINK,
        PROP_PRIVACY,
        PROP_COUNT,
        PROP_CAN_UPLOAD
};


static void
facebook_album_finalize (GObject *obj)
{
	FacebookAlbum *self;

	self = FACEBOOK_ALBUM (obj);

	g_free (self->id);
	g_free (self->name);
	g_free (self->description);
	g_free (self->link);
	g_free (self->privacy);

	G_OBJECT_CLASS (facebook_album_parent_class)->finalize (obj);
}


static void
facebook_album_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	FacebookAlbum *self;

        self = FACEBOOK_ALBUM (object);

	switch (property_id) {
	case PROP_ID:
		_g_strset (&self->id, g_value_get_string (value));
		break;
	case PROP_NAME:
		_g_strset (&self->name, g_value_get_string (value));
		break;
	case PROP_DESCRIPTION:
		_g_strset (&self->description, g_value_get_string (value));
		break;
	case PROP_LINK:
		_g_strset (&self->link, g_value_get_string (value));
		break;
	case PROP_PRIVACY:
		_g_strset (&self->privacy, g_value_get_string (value));
		break;
	case PROP_COUNT:
		self->count = g_value_get_int (value);
		break;
	case PROP_CAN_UPLOAD:
		self->can_upload = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}


static void
facebook_album_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	FacebookAlbum *self;

        self = FACEBOOK_ALBUM (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->id);
		break;
	case PROP_NAME:
		g_value_set_string (value, self->name);
		break;
	case PROP_DESCRIPTION:
		g_value_set_string (value, self->description);
		break;
	case PROP_LINK:
		g_value_set_string (value, self->link);
		break;
	case PROP_PRIVACY:
		g_value_set_string (value, self->privacy);
		break;
	case PROP_COUNT:
		g_value_set_int (value, self->count);
		break;
	case PROP_CAN_UPLOAD:
		 g_value_set_boolean (value, self->can_upload);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
facebook_album_class_init (FacebookAlbumClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = facebook_album_finalize;
	object_class->set_property = facebook_album_set_property;
	object_class->get_property = facebook_album_get_property;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
                                                              "ID",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
                                                              "Name",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DESCRIPTION,
					 g_param_spec_string ("description",
                                                              "Description",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LINK,
					 g_param_spec_string ("link",
                                                              "Link",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_PRIVACY,
					 g_param_spec_string ("privacy",
                                                              "Privacy",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COUNT,
					 g_param_spec_int ("count",
                                                           "Count",
                                                           "",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_CAN_UPLOAD,
					 g_param_spec_boolean ("can_upload",
                                                               "Can upload",
                                                               "",
                                                               FALSE,
                                                               G_PARAM_READWRITE));
}


static void
facebook_album_init (FacebookAlbum *self)
{
	self->id = NULL;
	self->name = NULL;
	self->description = NULL;
	self->link = NULL;
	self->privacy = NULL;
	self->count = 0;
	self->can_upload = FALSE;
}


FacebookAlbum *
facebook_album_new (void)
{
	return g_object_new (FACEBOOK_TYPE_ALBUM, NULL);
}
