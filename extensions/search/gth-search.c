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
#include <gthumb.h>
#include "gth-search.h"


#define SEARCH_FORMAT "1.0"


static void gth_search_dom_domizable_interface_init (DomDomizableInterface *iface);
static void gth_search_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthSearch,
			 gth_search,
			 GTH_TYPE_CATALOG,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        gth_search_dom_domizable_interface_init)
		         G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
		        		        gth_search_gth_duplicable_interface_init))


struct _GthSearchPrivate {
	GFile        *folder;
	gboolean      recursive;
	GthTestChain *test;	
};


static DomDomizableInterface  *dom_domizable_parent_iface = NULL;
static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;


static DomElement *
gth_search_create_root (GthCatalog  *catalog,
			DomDocument *doc)
{
	return dom_document_create_element (doc, "search",
					    "version", SEARCH_FORMAT,
					    NULL);
}


static void
gth_search_read_from_doc (GthCatalog *base,
			  DomElement *root)
{
	GthSearch  *self;
	DomElement *node;

	g_return_if_fail (DOM_IS_ELEMENT (root));

	self = GTH_SEARCH (base);
	GTH_CATALOG_CLASS (gth_search_parent_class)->read_from_doc (GTH_CATALOG (self), root);

	gth_search_set_test (self, NULL);
	for (node = root->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "folder") == 0) {
			GFile *folder;

			folder = g_file_new_for_uri (dom_element_get_attribute (node, "uri"));
			gth_search_set_folder (self, folder);
			g_object_unref (folder);

			gth_search_set_recursive (self, (g_strcmp0 (dom_element_get_attribute (node, "recursive"), "true") == 0));
		}
		else if (g_strcmp0 (node->tag_name, "tests") == 0) {
			GthTest *test;

			test = gth_test_chain_new (GTH_MATCH_TYPE_NONE, NULL);
			dom_domizable_load_from_element (DOM_DOMIZABLE (test), node);
			gth_search_set_test (self, GTH_TEST_CHAIN (test));
		}
	}
}


static void
_gth_search_write_to_doc (GthSearch   *self,
			  DomDocument *doc,
			  DomElement  *root)
{
	char *uri;

	uri = g_file_get_uri (self->priv->folder);
	dom_element_append_child (root,
				  dom_document_create_element (doc, "folder",
							       "uri", uri,
							       "recursive", (self->priv->recursive ? "true" : "false"),
							       NULL));
	g_free (uri);

	dom_element_append_child (root, dom_domizable_create_element (DOM_DOMIZABLE (self->priv->test), doc));
}


static void
gth_search_write_to_doc (GthCatalog  *catalog,
			 DomDocument *doc,
			 DomElement  *root)
{
	GTH_CATALOG_CLASS (gth_search_parent_class)->write_to_doc (catalog, doc, root);
	_gth_search_write_to_doc (GTH_SEARCH (catalog), doc, root);
}


static DomElement*
gth_search_real_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	GthSearch  *self;
	DomElement *element;
	
	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_SEARCH (base);
	element = gth_search_create_root (GTH_CATALOG (self), doc);
	gth_search_write_to_doc (GTH_CATALOG (self), doc, element);
	
	return element;
}


static void
gth_search_real_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	gth_search_read_from_doc (GTH_CATALOG (base), element);
}


GObject *
gth_search_real_duplicate (GthDuplicable *duplicable)
{
	GthSearch *search = GTH_SEARCH (duplicable);
	GthSearch *new_search;
	GList     *file_list;
	GList     *new_file_list = NULL;
	GList     *scan;

	new_search = gth_search_new ();
	
	gth_search_set_folder (new_search, gth_search_get_folder (search));
	gth_search_set_recursive (new_search, gth_search_is_recursive (search));
	
	if (search->priv->test != NULL)
		new_search->priv->test = (GthTestChain*) gth_duplicable_duplicate (GTH_DUPLICABLE (search->priv->test));

	file_list = gth_catalog_get_file_list (GTH_CATALOG (search));
	for (scan = file_list; scan; scan = scan->next) {
		GFile *file = scan->data;		
		new_file_list = g_list_prepend (new_file_list, g_file_dup (file));
	}
	gth_catalog_set_file_list (GTH_CATALOG (new_search), new_file_list);
	
	_g_object_list_unref (new_file_list);
	
	return (GObject *) new_search;
}


static void
gth_search_finalize (GObject *object)
{
	GthSearch *search;

	search = GTH_SEARCH (object);

	if (search->priv != NULL) {
		if (search->priv->folder != NULL)
			g_object_unref (search->priv->folder);
		if (search->priv->test != NULL)
			g_object_unref (search->priv->test);
		g_free (search->priv);
		search->priv = NULL;
	}

	G_OBJECT_CLASS (gth_search_parent_class)->finalize (object);
}


static void
gth_search_class_init (GthSearchClass *class)
{
	GObjectClass    *object_class;
	GthCatalogClass *catalog_class;
	
	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_search_finalize;
	
	catalog_class = GTH_CATALOG_CLASS (class);
	catalog_class->create_root = gth_search_create_root;
	catalog_class->read_from_doc = gth_search_read_from_doc;
	catalog_class->write_to_doc = gth_search_write_to_doc;
}


static void
gth_search_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	dom_domizable_parent_iface = g_type_interface_peek_parent (iface);
	iface->create_element = gth_search_real_create_element;
	iface->load_from_element = gth_search_real_load_from_element;
}


static void
gth_search_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_search_real_duplicate;
}


static void
gth_search_init (GthSearch *search)
{
	search->priv = g_new0 (GthSearchPrivate, 1);
}


GthSearch *
gth_search_new (void)
{
	return (GthSearch *) g_object_new (GTH_TYPE_SEARCH, NULL);
}


GthSearch*
gth_search_new_from_data (void    *buffer, 
		   	  gsize    count,
		   	  GError **error)
{
	DomDocument *doc;
	DomElement  *root;
	GthSearch   *search;
			
	doc = dom_document_new ();
	if (! dom_document_load (doc, (const char *) buffer, count, error)) 
		return NULL;
	
	root = DOM_ELEMENT (doc)->first_child;
	if (g_strcmp0 (root->tag_name, "search") != 0) {
		*error = g_error_new_literal (DOM_ERROR, DOM_ERROR_INVALID_FORMAT, _("Invalid file format"));
		return NULL;
	}
	
	search = gth_search_new ();
	dom_domizable_load_from_element (DOM_DOMIZABLE (search), root);
	
	g_object_unref (doc);
	
	return search;
}


void
gth_search_set_folder (GthSearch *search,
		       GFile     *folder)
{
	if (search->priv->folder != NULL) {
		g_object_unref (search->priv->folder);
		search->priv->folder = NULL;
	}
	
	if (folder != NULL)
		search->priv->folder = g_object_ref (folder);
}


GFile *
gth_search_get_folder (GthSearch *search)
{
	return search->priv->folder;
}


void
gth_search_set_recursive (GthSearch *search,
			  gboolean   recursive)
{
	search->priv->recursive = recursive;
}


gboolean
gth_search_is_recursive (GthSearch *search)
{
	return search->priv->recursive;
}


void
gth_search_set_test (GthSearch    *search,
		     GthTestChain *test)
{
	if (test == search->priv->test)
		return;
	if (search->priv->test != NULL) {
		g_object_unref (search->priv->test);
		search->priv->test = NULL;
	}
	if (test != NULL) 
		search->priv->test = g_object_ref (test);
}


GthTestChain *
gth_search_get_test (GthSearch *search)
{
	if (search->priv->test != NULL)
		return search->priv->test;
	else
		return NULL;
}


static DomDocument *
_create_fake_document (GthSearch *self)
{
	DomDocument *doc;
	DomElement  *root;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "search", NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	_gth_search_write_to_doc (self, doc, root);

	return doc;
}


gboolean
gth_search_equal (GthSearch *a,
		  GthSearch *b)
{
	DomDocument *doc_a;
	DomDocument *doc_b;
	char        *xml_a;
	gsize        size_a;
	char        *xml_b;
	gsize        size_b;
	gboolean     equal;

	doc_a = _create_fake_document (a);
	doc_b = _create_fake_document (b);
	xml_a = dom_document_dump (doc_a, &size_a);
	xml_b = dom_document_dump (doc_b, &size_b);
	equal = (size_a == size_b) && (g_strcmp0 (xml_a, xml_b) == 0);

	g_free (xml_a);
	g_free (xml_b);

	return equal;
}
