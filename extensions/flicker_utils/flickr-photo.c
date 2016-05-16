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
#include "flickr-photo.h"


char *FlickrUrlSuffix[] = {
	"_sq",
	"_s",
	"_t",
	"_m",
	"_z",
	"_b",
	"_o"
};


struct _FlickrPhotoPrivate {
	FlickrServer *server;
};


static void flickr_photo_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (FlickrPhoto,
			 flickr_photo,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        flickr_photo_dom_domizable_interface_init))


static void
flickr_photo_finalize (GObject *obj)
{
	FlickrPhoto *self;
	int          i;

	self = FLICKR_PHOTO (obj);

	g_free (self->id);
	g_free (self->secret);
	g_free (self->server);
	g_free (self->farm);
	g_free (self->title);
	for (i = 0; i < FLICKR_URLS; i++)
		g_free (self->url[i]);
	g_free (self->original_format);
	g_free (self->mime_type);

	G_OBJECT_CLASS (flickr_photo_parent_class)->finalize (obj);
}


static void
flickr_photo_class_init (FlickrPhotoClass *klass)
{
	g_type_class_add_private (klass, sizeof (FlickrPhotoPrivate));
	G_OBJECT_CLASS (klass)->finalize = flickr_photo_finalize;
}


static DomElement*
flickr_photo_create_element (DomDomizable *base,
			     DomDocument  *doc)
{
	FlickrPhoto *self;
	DomElement  *element;

	self = FLICKR_PHOTO (base);

	element = dom_document_create_element (doc, "photo", NULL);
	if (self->id != NULL)
		dom_element_set_attribute (element, "id", self->id);
	if (self->secret != NULL)
		dom_element_set_attribute (element, "secret", self->secret);
	if (self->server != NULL)
		dom_element_set_attribute (element, "server", self->server);
	if (self->title != NULL)
		dom_element_set_attribute (element, "title", self->title);
	if (self->is_primary)
		dom_element_set_attribute (element, "isprimary", "1");

	return element;
}


static void
flickr_photo_load_from_element (DomDomizable *base,
				DomElement   *element)
{
	FlickrPhoto *self;

	if ((element == NULL) || (g_strcmp0 (element->tag_name, "photo") != 0))
		return;

	self = FLICKR_PHOTO (base);

	flickr_photo_set_id (self, dom_element_get_attribute (element, "id"));
	flickr_photo_set_secret (self, dom_element_get_attribute (element, "secret"));
	flickr_photo_set_server (self, dom_element_get_attribute (element, "server"));
	flickr_photo_set_farm (self, dom_element_get_attribute (element, "farm"));
	flickr_photo_set_title (self, dom_element_get_attribute (element, "title"));
	flickr_photo_set_is_primary (self, dom_element_get_attribute (element, "isprimary"));
	flickr_photo_set_original_format (self, dom_element_get_attribute (element, "originalformat"));
	flickr_photo_set_original_secret (self, dom_element_get_attribute (element, "originalsecret"));

	flickr_photo_set_url (self, FLICKR_URL_SQ, dom_element_get_attribute (element, "url_sq"));
	flickr_photo_set_url (self, FLICKR_URL_S, dom_element_get_attribute (element, "url_s"));
	flickr_photo_set_url (self, FLICKR_URL_T, dom_element_get_attribute (element, "url_t"));
	flickr_photo_set_url (self, FLICKR_URL_M, dom_element_get_attribute (element, "url_m"));
	flickr_photo_set_url (self, FLICKR_URL_Z, dom_element_get_attribute (element, "url_z"));
	flickr_photo_set_url (self, FLICKR_URL_B, dom_element_get_attribute (element, "url_b"));
	flickr_photo_set_url (self, FLICKR_URL_O, dom_element_get_attribute (element, "url_o"));
}


static void
flickr_photo_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = flickr_photo_create_element;
	iface->load_from_element = flickr_photo_load_from_element;
}


static void
flickr_photo_init (FlickrPhoto *self)
{
	int i;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FLICKR_TYPE_PHOTO, FlickrPhotoPrivate);
	self->priv->server = NULL;

	self->id = NULL;
	self->secret = NULL;
	self->server = NULL;
	self->farm = NULL;
	self->title = NULL;
	for (i = 0; i < FLICKR_URLS; i++)
		self->url[i] = NULL;
	self->original_format = NULL;
	self->mime_type = NULL;
}


FlickrPhoto *
flickr_photo_new (FlickrServer *server)
{
	FlickrPhoto *self;

	self = g_object_new (FLICKR_TYPE_PHOTO, NULL);
	self->priv->server = server;

	return self;
}


void
flickr_photo_set_id (FlickrPhoto *self,
		     const char  *value)
{
	_g_strset (&self->id, value);
}


void
flickr_photo_set_secret (FlickrPhoto *self,
			 const char  *value)
{
	_g_strset (&self->secret, value);
}


void
flickr_photo_set_server (FlickrPhoto *self,
			 const char  *value)
{
	_g_strset (&self->server, value);
}


void
flickr_photo_set_farm (FlickrPhoto *self,
		       const char  *value)
{
	_g_strset (&self->farm, value);
}


void
flickr_photo_set_title (FlickrPhoto *self,
			const char  *value)
{
	_g_strset (&self->title, value);
}


void
flickr_photo_set_is_primary (FlickrPhoto *self,
			     const char  *value)
{
	self->is_primary = (g_strcmp0 (value, "1") == 0);
}


static char *
flickr_get_static_url (FlickrPhoto *self,
		       FlickrUrl    size)
{

	const char *ext;
	const char *secret;

	if ((self->priv->server == NULL) || ! self->priv->server->automatic_urls)
		return NULL;

	secret = self->secret;
	if (size == FLICKR_URL_O) {
		if (self->original_secret != NULL)
			secret = self->original_secret;
	}

	ext = "jpg";
	if (size == FLICKR_URL_O) {
		if (self->original_format != NULL)
			ext = self->original_format;
	}

	if (self->farm != NULL)
		return g_strdup_printf ("http://farm%s.%s/%s/%s_%s%s.%s",
					self->farm,
					self->priv->server->static_url,
					self->server,
					self->id,
					secret,
					FlickrUrlSuffix[size],
					ext);
	else
		return g_strdup_printf ("http://%s/%s/%s_%s%s.%s",
					self->priv->server->static_url,
					self->server,
					self->id,
					secret,
					FlickrUrlSuffix[size],
					ext);
}


void
flickr_photo_set_url (FlickrPhoto *self,
		      FlickrUrl    size,
		      const char  *value)
{
	_g_strset (&(self->url[size]), value);
	if (self->url[size] == NULL)
		self->url[size] = flickr_get_static_url (self, size);

	if ((size == FLICKR_URL_O) && (self->url[size] == NULL)) {
		int other_size;
		for (other_size = FLICKR_URL_O - 1; other_size >= 0; other_size--) {
			if (self->url[other_size] != NULL) {
				_g_strset (&(self->url[size]), self->url[other_size]);
				break;
			}
		}
	}
}


void
flickr_photo_set_original_format (FlickrPhoto *self,
				  const char  *value)
{
	_g_strset (&self->original_format, value);

	g_free (self->mime_type);
	self->mime_type = NULL;
	if (self->original_format != NULL)
		self->mime_type = g_strconcat ("image/", self->original_format, NULL);
}


void
flickr_photo_set_original_secret (FlickrPhoto *self,
				  const char  *value)
{
	_g_strset (&self->original_secret, value);
}
