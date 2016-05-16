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
#include "photobucket-photo.h"


static void photobucket_photo_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (PhotobucketPhoto,
			 photobucket_photo,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        photobucket_photo_dom_domizable_interface_init))


static void
photobucket_photo_finalize (GObject *obj)
{
	PhotobucketPhoto *self;

	self = PHOTOBUCKET_PHOTO (obj);

	g_free (self->name);
	g_free (self->browse_url);
	g_free (self->url);
	g_free (self->thumb_url);
	g_free (self->description);
	g_free (self->title);

	G_OBJECT_CLASS (photobucket_photo_parent_class)->finalize (obj);
}


static void
photobucket_photo_class_init (PhotobucketPhotoClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = photobucket_photo_finalize;
}


static DomElement*
photobucket_photo_create_element (DomDomizable *base,
				  DomDocument  *doc)
{
	PhotobucketPhoto *self;
	DomElement  *element;

	self = PHOTOBUCKET_PHOTO (base);

	element = dom_document_create_element (doc, "photo", NULL);
	if (self->name != NULL)
		dom_element_set_attribute (element, "name", self->name);

	return element;
}


static void
photobucket_photo_load_from_element (DomDomizable *base,
				     DomElement   *element)
{
	PhotobucketPhoto *self;
	DomElement       *node;

	if ((element == NULL) || (g_strcmp0 (element->tag_name, "photo") != 0))
		return;

	self = PHOTOBUCKET_PHOTO (base);

	photobucket_photo_set_name (self, dom_element_get_attribute (element, "name"));
	photobucket_photo_set_is_public (self, dom_element_get_attribute (element, "public"));

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "browse_url") == 0) {
			photobucket_photo_set_browse_url (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "url") == 0) {
			photobucket_photo_set_url (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "thumb_url") == 0) {
			photobucket_photo_set_thumb_url (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "description") == 0) {
			photobucket_photo_set_description (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "title") == 0) {
			photobucket_photo_set_title (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "is_sponsored") == 0) {
			photobucket_photo_set_is_sponsored (self, dom_element_get_inner_text (node));
		}
	}
}


static void
photobucket_photo_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = photobucket_photo_create_element;
	iface->load_from_element = photobucket_photo_load_from_element;
}


static void
photobucket_photo_init (PhotobucketPhoto *self)
{
	/* void */
}


PhotobucketPhoto *
photobucket_photo_new (void)
{
	return g_object_new (PHOTOBUCKET_TYPE_PHOTO, NULL);
}


void
photobucket_photo_set_name (PhotobucketPhoto *self,
			    const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_is_public (PhotobucketPhoto *self,
			         const char       *value)
{
	self->is_public = (g_strcmp0 (value, "1") == 0);
}


void
photobucket_photo_set_browse_url (PhotobucketPhoto *self,
			          const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_url (PhotobucketPhoto *self,
			   const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_thumb_url (PhotobucketPhoto *self,
			         const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_description (PhotobucketPhoto *self,
			           const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_title (PhotobucketPhoto *self,
			     const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_is_sponsored (PhotobucketPhoto *self,
			            const char       *value)
{
	self->is_sponsored = (g_strcmp0 (value, "1") == 0);
}
