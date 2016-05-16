/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <extensions/oauth/oauth.h>
#include "flickr-account.h"


static void flickr_account_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (FlickrAccount,
			 flickr_account,
			 OAUTH_TYPE_ACCOUNT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        flickr_account_dom_domizable_interface_init))


static void
flickr_account_finalize (GObject *obj)
{
	FlickrAccount *self;

	self = FLICKR_ACCOUNT (obj);

	g_free (self->accountname);

	G_OBJECT_CLASS (flickr_account_parent_class)->finalize (obj);
}


static void
flickr_account_class_init (FlickrAccountClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = flickr_account_finalize;
}


static DomElement *
flickr_account_create_element (DomDomizable *self,
			       DomDocument  *doc)
{
	return oauth_account_create_element (self, doc);
}


static void
flickr_account_load_from_element (DomDomizable *base,
				  DomElement   *element)
{
	FlickrAccount *self = FLICKR_ACCOUNT (base);

	oauth_account_load_from_element (base, element);
	flickr_account_load_extra_data (self, element);
}


static void
flickr_account_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = flickr_account_create_element;
	iface->load_from_element = flickr_account_load_from_element;
}


static void
flickr_account_init (FlickrAccount *self)
{
	self->is_pro = FALSE;
	self->accountname = NULL;
	self->max_bandwidth = 0;
	self->used_bandwidth = 0;
	self->max_filesize = 0;
	self->max_videosize = 0;
	self->n_sets = 0;
	self->n_videos = 0;
}


OAuthAccount *
flickr_account_new (void)
{
	return g_object_new (FLICKR_TYPE_ACCOUNT, NULL);
}


void
flickr_account_set_is_pro (FlickrAccount *self,
			   const char    *value)
{
	self->is_pro = (g_strcmp0 (value, "1") == 0);
}


void
flickr_account_set_accountname (FlickrAccount *self,
				const char    *value)
{
	_g_strset (&self->accountname, value);
}


void
flickr_account_set_max_bandwidth (FlickrAccount *self,
				  const char    *value)
{
	self->max_bandwidth = g_ascii_strtoull (value, NULL, 10);
}


void
flickr_account_set_used_bandwidth (FlickrAccount *self,
				   const char    *value)
{
	self->used_bandwidth = g_ascii_strtoull (value, NULL, 10);
}


void
flickr_account_set_max_filesize (FlickrAccount *self,
				 const char    *value)
{
	self->max_filesize = g_ascii_strtoull (value, NULL, 10);
}


void
flickr_account_set_max_videosize (FlickrAccount *self,
				  const char    *value)
{
	self->max_videosize = g_ascii_strtoull (value, NULL, 10);
}


void
flickr_account_set_n_sets (FlickrAccount *self,
			   const char    *value)
{
	if (value != NULL)
		self->n_sets = atoi (value);
	else
		self->n_sets = 0;
}


void
flickr_account_set_n_videos (FlickrAccount *self,
			     const char    *value)
{
	if (value != NULL)
		self->n_videos = atoi (value);
	else
		self->n_videos = 0;
}


void
flickr_account_load_extra_data (FlickrAccount *self,
				DomElement    *element)
{
	DomElement *node;

	flickr_account_set_is_pro (self, dom_element_get_attribute (element, "ispro"));

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "accountname") == 0) {
			flickr_account_set_accountname (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "bandwidth") == 0) {
			flickr_account_set_max_bandwidth (self, dom_element_get_attribute (node, "maxbytes"));
			flickr_account_set_used_bandwidth (self, dom_element_get_attribute (node, "usedbytes"));
		}
		else if (g_strcmp0 (node->tag_name, "filesize") == 0) {
			flickr_account_set_max_filesize (self, dom_element_get_attribute (node, "maxbytes"));
		}
		else if (g_strcmp0 (node->tag_name, "videosize") == 0) {
			flickr_account_set_max_videosize (self, dom_element_get_attribute (node, "maxbytes"));
		}
		else if (g_strcmp0 (node->tag_name, "sets") == 0) {
			flickr_account_set_n_sets (self, dom_element_get_attribute (node, "created"));
		}
		else if (g_strcmp0 (node->tag_name, "videos") == 0) {
			flickr_account_set_n_videos (self, dom_element_get_attribute (node, "uploaded"));
		}
	}

}
