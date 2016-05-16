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
#include "picasa-web-album.h"


static void picasa_web_album_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (PicasaWebAlbum,
			 picasa_web_album,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        picasa_web_album_dom_domizable_interface_init))


static void
picasa_web_album_finalize (GObject *obj)
{
	PicasaWebAlbum *self;

	self = PICASA_WEB_ALBUM (obj);

	g_free (self->id);
	g_free (self->title);
	g_free (self->summary);
	g_free (self->edit_url);

	G_OBJECT_CLASS (picasa_web_album_parent_class)->finalize (obj);
}


static void
picasa_web_album_class_init (PicasaWebAlbumClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = picasa_web_album_finalize;
}


static DomElement*
picasa_web_album_create_element (DomDomizable *base,
				 DomDocument  *doc)
{
	PicasaWebAlbum *self;
	DomElement     *element;
	char           *value;

	self = PICASA_WEB_ALBUM (base);

	element = dom_document_create_element (doc, "entry",
					       "xmlns", "http://www.w3.org/2005/Atom",
					       "xmlns:media", "http://search.yahoo.com/mrss/",
					       "xmlns:gphoto", "http://schemas.google.com/photos/2007",
					       NULL);
	if (self->id != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->id, "id", NULL));
	if (self->title != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->title, "title", "type", "text", NULL));
	if (self->summary != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->summary, "summary", "type", "text", NULL));
	if (self->location != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->location, "gphoto:location", NULL));

	switch (self->access) {
	case PICASA_WEB_ACCESS_ALL:
		value = "all";
		break;
	case PICASA_WEB_ACCESS_PRIVATE:
	default:
		value = "private";
		break;
	case PICASA_WEB_ACCESS_PUBLIC:
		value = "public";
		break;
	case PICASA_WEB_ACCESS_VISIBLE:
		value = "visible";
		break;
	}
	dom_element_append_child (element, dom_document_create_element_with_text (doc, value, "gphoto:access", NULL));

	dom_element_append_child (element,
				  dom_document_create_element (doc, "category",
							       "scheme", "http://schemas.google.com/g/2005#kind",
							       "term", "http://schemas.google.com/photos/2007#album",
							       NULL));

	return element;
}


static void
picasa_web_album_load_from_element (DomDomizable *base,
				    DomElement   *element)
{
	PicasaWebAlbum *self;
	DomElement     *node;

	self = PICASA_WEB_ALBUM (base);

	picasa_web_album_set_id (self, NULL);
	picasa_web_album_set_title (self, NULL);
	picasa_web_album_set_summary (self, NULL);
	picasa_web_album_set_alternate_url (self, NULL);
	picasa_web_album_set_edit_url (self, NULL);
	picasa_web_album_set_access (self, NULL);
	self->n_photos = 0;
	self->n_photos_remaining = 0;
	self->used_bytes = 0;

	picasa_web_album_set_etag (self, dom_element_get_attribute (element, "gd:etag"));
	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "gphoto:id") == 0) {
			picasa_web_album_set_id (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "title") == 0) {
			picasa_web_album_set_title (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "summary") == 0) {
			picasa_web_album_set_summary (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:location") == 0) {
			picasa_web_album_set_location (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "link") == 0) {
			if (g_strcmp0 (dom_element_get_attribute (node, "rel"), "edit") == 0)
				picasa_web_album_set_edit_url (self, dom_element_get_attribute (node, "href"));
			else if (g_strcmp0 (dom_element_get_attribute (node, "rel"), "alternate") == 0)
				picasa_web_album_set_alternate_url (self, dom_element_get_attribute (node, "href"));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:access") == 0) {
			picasa_web_album_set_access (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:numphotos") == 0) {
			picasa_web_album_set_n_photos (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:numphotosremaining") == 0) {
			picasa_web_album_set_n_photos_remaining (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:bytesUsed") == 0) {
			picasa_web_album_set_used_bytes (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "media:group") == 0) {
			DomElement *child;

			for (child = node->first_child; child; child = child->next_sibling) {
				if (g_strcmp0 (child->tag_name, "media:keywords") == 0) {
					picasa_web_album_set_keywords (self, dom_element_get_inner_text (child));
					break;
				}
			}
		}
	}
}


static void
picasa_web_album_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = picasa_web_album_create_element;
	iface->load_from_element = picasa_web_album_load_from_element;
}


static void
picasa_web_album_init (PicasaWebAlbum *self)
{
	/* void */
}


PicasaWebAlbum *
picasa_web_album_new (void)
{
	return g_object_new (PICASA_WEB_TYPE_ALBUM, NULL);
}


void
picasa_web_album_set_etag (PicasaWebAlbum *self,
			   const char     *value)
{
	_g_strset (&self->etag, value);
}


void
picasa_web_album_set_id (PicasaWebAlbum *self,
			 const char     *value)
{
	_g_strset (&self->id, value);
}


void
picasa_web_album_set_title (PicasaWebAlbum *self,
			    const char     *value)
{
	_g_strset (&self->title, value);
}


void
picasa_web_album_set_summary (PicasaWebAlbum *self,
			      const char     *value)
{
	_g_strset (&self->summary, value);
}


void
picasa_web_album_set_location (PicasaWebAlbum *self,
			       const char     *value)
{
	_g_strset (&self->location, value);
}


void
picasa_web_album_set_alternate_url (PicasaWebAlbum *self,
				    const char     *value)
{
	_g_strset (&self->alternate_url, value);
}


void
picasa_web_album_set_edit_url (PicasaWebAlbum *self,
			       const char     *value)
{
	_g_strset (&self->edit_url, value);
}


void
picasa_web_album_set_access (PicasaWebAlbum *self,
			     const char     *value)
{
	if (value == NULL)
		self->access = PICASA_WEB_ACCESS_PRIVATE;
	else if (strcmp (value, "all") == 0)
		self->access = PICASA_WEB_ACCESS_ALL;
	else if (strcmp (value, "private") == 0)
		self->access = PICASA_WEB_ACCESS_PRIVATE;
	else if (strcmp (value, "public") == 0)
		self->access = PICASA_WEB_ACCESS_PUBLIC;
	else if (strcmp (value, "visible") == 0)
		self->access = PICASA_WEB_ACCESS_VISIBLE;
	else
		self->access = PICASA_WEB_ACCESS_PRIVATE;
}


void
picasa_web_album_set_used_bytes (PicasaWebAlbum *self,
				 const char     *value)
{
	self->used_bytes = g_ascii_strtoull (value, NULL, 10);
}


void
picasa_web_album_set_n_photos (PicasaWebAlbum *self,
			       const char     *value)
{
	if (value != NULL)
		self->n_photos = atoi (value);
	else
		self->n_photos = 0;
}


void
picasa_web_album_set_n_photos_remaining (PicasaWebAlbum *self,
					 const char     *value)
{
	if (value != NULL)
		self->n_photos_remaining = atoi (value);
	else
		self->n_photos_remaining = 0;
}


void
picasa_web_album_set_keywords (PicasaWebAlbum *self,
			       const char     *value)
{
	_g_strset (&self->keywords, value);
}
