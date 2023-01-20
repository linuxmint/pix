/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-search-source.h"


static void gth_search_source_dom_domizable_interface_init (DomDomizableInterface *iface);
static void gth_search_source_gth_duplicable_interface_init (GthDuplicableInterface *iface);


struct _GthSearchSourcePrivate {
	GFile    *folder;
	gboolean  recursive;
};


G_DEFINE_TYPE_WITH_CODE (GthSearchSource,
			 gth_search_source,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthSearchSource)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_search_source_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_search_source_gth_duplicable_interface_init))


static DomDomizableInterface  *dom_domizable_parent_iface = NULL;
static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;


static DomElement*
gth_search_source_real_create_element (DomDomizable *base,
				       DomDocument  *doc)
{
	GthSearchSource *self = GTH_SEARCH_SOURCE (base);
	char            *uri;
	DomElement      *element;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	uri = g_file_get_uri (self->priv->folder);
	element = dom_document_create_element (doc, "source",
					       "uri", uri,
					       "recursive", (self->priv->recursive ? "true" : "false"),
					       NULL);
	g_free (uri);

	return element;
}


static void
gth_search_source_real_load_from_element (DomDomizable *base,
					  DomElement   *element)
{
	GthSearchSource *self = GTH_SEARCH_SOURCE (base);
	GFile           *folder;

	g_return_if_fail (DOM_IS_ELEMENT (element));
	g_return_if_fail (g_strcmp0 (element->tag_name, "source") == 0);

	folder = g_file_new_for_uri (dom_element_get_attribute (element, "uri"));
	gth_search_source_set_folder (self, folder);
	g_object_unref (folder);

	gth_search_source_set_recursive (self, (g_strcmp0 (dom_element_get_attribute (element, "recursive"), "true") == 0));
}


static GObject *
gth_search_source_real_duplicate (GthDuplicable *duplicable)
{
	GthSearchSource *source = GTH_SEARCH_SOURCE (duplicable);
	GthSearchSource *new_source;

	new_source = gth_search_source_new ();

	gth_search_source_set_folder (new_source, gth_search_source_get_folder (source));
	gth_search_source_set_recursive (new_source, gth_search_source_is_recursive (source));

	return (GObject *) new_source;
}


static void
gth_search_source_finalize (GObject *object)
{
	GthSearchSource *source;

	source = GTH_SEARCH_SOURCE (object);

	if (source->priv->folder != NULL)
		g_object_unref (source->priv->folder);

	G_OBJECT_CLASS (gth_search_source_parent_class)->finalize (object);
}


static void
gth_search_source_class_init (GthSearchSourceClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_search_source_finalize;
}


static void
gth_search_source_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	dom_domizable_parent_iface = g_type_interface_peek_parent (iface);
	iface->create_element = gth_search_source_real_create_element;
	iface->load_from_element = gth_search_source_real_load_from_element;
}


static void
gth_search_source_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_search_source_real_duplicate;
}


static void
gth_search_source_init (GthSearchSource *source)
{
	source->priv = gth_search_source_get_instance_private (source);
	source->priv->folder = NULL;
	source->priv->recursive = FALSE;
}


GthSearchSource *
gth_search_source_new (void)
{
	return (GthSearchSource *) g_object_new (GTH_TYPE_SEARCH_SOURCE, NULL);
}


void
gth_search_source_set_folder (GthSearchSource *source,
			      GFile           *folder)
{
	_g_object_ref (folder);

	if (source->priv->folder != NULL) {
		g_object_unref (source->priv->folder);
		source->priv->folder = NULL;
	}

	if (folder != NULL)
		source->priv->folder = folder;
}


GFile *
gth_search_source_get_folder (GthSearchSource *source)
{
	return source->priv->folder;
}


void
gth_search_source_set_recursive (GthSearchSource *source,
				 gboolean         recursive)
{
	source->priv->recursive = recursive;
}


gboolean
gth_search_source_is_recursive (GthSearchSource *source)
{
	return source->priv->recursive;
}
