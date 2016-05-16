/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "dom.h"

#define XML_DOCUMENT_TAG_NAME ("#document")
#define XML_TEXT_NODE_TAG_NAME ("#text")
#define XML_TAB_WIDTH (2)
#define XML_RC ("\n")


struct _DomDocumentPrivate {
	GQueue *open_nodes;
};


GQuark
dom_error_quark (void)
{
	return g_quark_from_static_string ("dom-error-quark");
}


static void
_g_list_free_g_object_unref (GList *self)
{
	g_list_foreach (self, ((GFunc) (g_object_unref)), NULL);
	g_list_free (self);
}


static void
_g_string_append_c_n_times (GString *string,
			    char     c,
		            int      times)
{
	int i;

	for (i = 1; i <= times; i++)
		g_string_append_c (string, c);
}


static char *
_g_xml_attribute_quote (char *value)
{
	char       *escaped;
	GString    *dest;
	const char *p;

	g_return_val_if_fail (value != NULL, NULL);

  	escaped = g_markup_escape_text (value, -1);

  	dest = g_string_new ("\"");
	for (p = escaped; (*p); p++) {
		if (*p == '"')
			g_string_append (dest, "\\\"");
		else
			g_string_append_c (dest, *p);
	}
	g_string_append_c (dest, '"');

  	g_free (escaped);

	return g_string_free (dest, FALSE);
}


static gboolean
_g_xml_tag_is_special (const char *tag_name)
{
	return (*tag_name == '#');
}


/* -- DomElement -- */


G_DEFINE_TYPE (DomElement, dom_element, G_TYPE_INITIALLY_UNOWNED)


static void
dom_attribute_dump (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
	GString *xml = user_data;
	char    *quoted_value;

	g_string_append_c (xml, ' ');
	g_string_append (xml, key);
	g_string_append_c (xml, '=');
	quoted_value = _g_xml_attribute_quote (value);
	g_string_append (xml, quoted_value);
	g_free (quoted_value);
}


static char *
dom_element_dump (DomElement *self,
		  int         level)
{
	return DOM_ELEMENT_GET_CLASS (self)->dump (self, level);
}


static char *
dom_element_real_dump (DomElement *self,
		       int         level)
{
	GString *xml;
	GList   *scan;

	xml = g_string_new ("");

	if ((self->parent_node != NULL) && (self != self->parent_node->first_child))
		 g_string_append (xml, XML_RC);

	/* start tag */

	if (! _g_xml_tag_is_special (self->tag_name)) {
		_g_string_append_c_n_times (xml, ' ', level * XML_TAB_WIDTH);
		g_string_append_c (xml, '<');
		g_string_append (xml, self->tag_name);
		g_hash_table_foreach (self->attributes, (GHFunc) dom_attribute_dump, xml);
		if (self->child_nodes != NULL) {
			g_string_append_c (xml, '>');
			if (! DOM_IS_TEXT_NODE (self->first_child))
				g_string_append (xml, XML_RC);
		}
		else {
			g_string_append (xml, "/>");
			return g_string_free (xml, FALSE);
		}
	}

	/* tag children */

	for (scan = self->child_nodes; scan != NULL; scan = scan->next) {
		DomElement *child = scan->data;
		char       *child_dump;

		child_dump = dom_element_dump (child, level + 1);
		g_string_append (xml, child_dump);
		g_free (child_dump);
        }

	/* end tag */

	if (! _g_xml_tag_is_special (self->tag_name)) {
		if ((self->child_nodes != NULL) && ! DOM_IS_TEXT_NODE (self->first_child)) {
			g_string_append (xml, XML_RC);
			_g_string_append_c_n_times (xml, ' ', level * XML_TAB_WIDTH);
		}
		g_string_append (xml, "</");
		g_string_append (xml, self->tag_name);
		g_string_append_c (xml, '>');
	}

	return g_string_free (xml, FALSE);
}


static void
dom_element_init (DomElement *self)
{
	self->tag_name = NULL;
	self->prefix = NULL;
	self->attributes = g_hash_table_new_full (g_str_hash,
						  g_str_equal,
						  (GDestroyNotify) g_free,
						  (GDestroyNotify )g_free);
	self->next_sibling = NULL;
	self->previous_sibling = NULL;
	self->parent_node = NULL;
	self->child_nodes = NULL;
	self->first_child = NULL;
	self->last_child = NULL;
}


static void
dom_element_finalize (GObject *obj)
{
	DomElement *self;

	self = DOM_ELEMENT (obj);
	g_free (self->tag_name);
	g_free (self->prefix);
	g_hash_table_unref (self->attributes);
	_g_list_free_g_object_unref (self->child_nodes);

	G_OBJECT_CLASS (dom_element_parent_class)->finalize (obj);
}


static void
dom_element_class_init (DomElementClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = dom_element_finalize;
	DOM_ELEMENT_CLASS (klass)->dump = dom_element_real_dump;
}


static DomElement *
dom_element_new (const char *a_tag_name)
{
	DomElement *self;

	g_return_val_if_fail (a_tag_name != NULL, NULL);

	self = g_object_newv (DOM_TYPE_ELEMENT, 0, NULL);
	self->tag_name = (a_tag_name == NULL) ? NULL : g_strdup (a_tag_name);

	return self;
}


void
dom_element_append_child (DomElement *self,
			  DomElement *child)
{
	GList *child_link;

	g_return_if_fail (DOM_IS_ELEMENT (self));
	g_return_if_fail (DOM_IS_ELEMENT (child));

	self->child_nodes = g_list_append (self->child_nodes, g_object_ref_sink (child));

	/* update child attributes */

	child_link = g_list_find (self->child_nodes, child);
	child->parent_node = self;
	if (child_link->prev != NULL) {
		DomElement *prev_sibling;

		prev_sibling = child_link->prev->data;
		child->previous_sibling = prev_sibling;
		prev_sibling->next_sibling = child;
	}
	else
		child->previous_sibling = NULL;
	child->next_sibling = NULL;

	/* update self attributes */

	self->first_child = (self->first_child == NULL) ? child_link->data : self->first_child;
	self->last_child = child;
}


const char *
dom_element_get_attribute (DomElement *self,
			   const char *name)
{
	g_return_val_if_fail (DOM_IS_ELEMENT (self), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return (const char*) g_hash_table_lookup (self->attributes, name);
}


int
dom_element_get_attribute_as_int (DomElement *self,
				  const char *name)
{
	const char *value;
	int         i;

	i = 0;
	value = dom_element_get_attribute (self, name);
	if (value != NULL)
		i = atoi (value);

	return i;
}


gboolean
dom_element_has_attribute (DomElement *self,
			   const char *name)
{
	g_return_val_if_fail (DOM_IS_ELEMENT (self), FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	return ((const char*) (g_hash_table_lookup (self->attributes, name))) != NULL;
}


gboolean
dom_element_has_child_nodes (DomElement *self)
{
	g_return_val_if_fail (DOM_IS_ELEMENT (self), FALSE);

	return self->child_nodes != NULL;
}


void
dom_element_remove_attribute (DomElement *self,
			      const char *name)
{
	g_return_if_fail (DOM_IS_ELEMENT (self));
	g_return_if_fail (name != NULL);

	g_hash_table_remove (self->attributes, name);
}


DomElement *
dom_element_remove_child (DomElement *self,
			  DomElement *node)
{
	g_return_val_if_fail (DOM_IS_ELEMENT (self), NULL);
	g_return_val_if_fail (DOM_IS_ELEMENT (node), NULL);

	self->child_nodes = g_list_remove (self->child_nodes, node);
	return node;
}


void
dom_element_replace_child (DomElement *self,
			   DomElement *new_child,
			   DomElement *old_child)
{
	GList *link;

	g_return_if_fail (DOM_IS_ELEMENT (self));
	g_return_if_fail (DOM_IS_ELEMENT (new_child));
	g_return_if_fail (DOM_IS_ELEMENT (old_child));

	link = g_list_find (self->child_nodes, old_child);
	if (link == NULL)
		return;
	g_object_unref (link->data);
	link->data = g_object_ref (new_child);
}


void
dom_element_set_attribute (DomElement *self,
			   const char *name,
			   const char *value)
{
	g_return_if_fail (DOM_IS_ELEMENT (self));
	g_return_if_fail (name != NULL);

	if (value == NULL)
		g_hash_table_remove (self->attributes, name);
	else
		g_hash_table_insert (self->attributes, g_strdup (name), g_strdup (value));
}


const char *
dom_element_get_inner_text (DomElement *self)
{
	if (! DOM_IS_TEXT_NODE (self->first_child))
		return NULL;
	else
		return DOM_TEXT_NODE (self->first_child)->data;
}


/* -- DomTextNode -- */


G_DEFINE_TYPE (DomTextNode, dom_text_node, DOM_TYPE_ELEMENT)


static char *
dom_text_node_real_dump (DomElement *base,
			 int         level)
{
	DomTextNode *self;

	self = DOM_TEXT_NODE (base);
	return (self->data == NULL ? g_strdup ("") : g_markup_escape_text (self->data, -1));
}


static void
dom_text_node_finalize (GObject *obj)
{
	DomTextNode *self;

	self = DOM_TEXT_NODE (obj);
	g_free (self->data);

	G_OBJECT_CLASS (dom_text_node_parent_class)->finalize (obj);
}


static void
dom_text_node_class_init (DomTextNodeClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = dom_text_node_finalize;
	DOM_ELEMENT_CLASS (klass)->dump = dom_text_node_real_dump;
}


static void
dom_text_node_init (DomTextNode *self)
{
	DOM_ELEMENT (self)->tag_name = g_strdup (XML_TEXT_NODE_TAG_NAME);
	self->data = NULL;
}


static DomTextNode *
dom_text_node_new (const char *a_data)
{
	DomTextNode *self;

	self = g_object_newv (DOM_TYPE_TEXT_NODE, 0, NULL);
	self->data = (a_data == NULL ? g_strdup ("") : g_strdup (a_data));

	return self;
}


/* -- DomDocument -- */


G_DEFINE_TYPE (DomDocument, dom_document, DOM_TYPE_ELEMENT)


static void
dom_document_finalize (GObject *obj)
{
	DomDocument *self;

	self = DOM_DOCUMENT (obj);

	if (self->priv != NULL) {
		g_queue_free (self->priv->open_nodes);
		g_free (self->priv);
	}

	G_OBJECT_CLASS (dom_document_parent_class)->finalize (obj);
}


static void
dom_document_class_init (DomDocumentClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = dom_document_finalize;
}


static void
dom_document_init (DomDocument *self)
{
	DOM_ELEMENT (self)->tag_name = g_strdup (XML_DOCUMENT_TAG_NAME);

	self->priv = g_new0 (DomDocumentPrivate, 1);
	self->priv->open_nodes = g_queue_new ();
}


DomDocument *
dom_document_new (void)
{
	DomDocument *self;

	self = g_object_newv (DOM_TYPE_DOCUMENT, 0, NULL);
	return g_object_ref_sink (self);
}


DomElement *
dom_document_create_element (DomDocument *self,
			     const char  *tag_name,
			     const char  *first_attr,
			     ...)
{
	DomElement *element;
	va_list     var_args;
	const char  *attr;
	const char *value;

	g_return_val_if_fail (DOM_IS_DOCUMENT (self), NULL);
	g_return_val_if_fail (tag_name != NULL, NULL);

	element = dom_element_new (tag_name);
	va_start (var_args, first_attr);
	attr = first_attr;
        while (attr != NULL) {
        	value = va_arg (var_args, const char *);
		dom_element_set_attribute (element, attr, value);
                attr = va_arg (var_args, const char *);
        }
	va_end (var_args);

	return element;
}


DomElement *
dom_document_create_text_node (DomDocument *self,
			       const char  *data)
{
	g_return_val_if_fail (DOM_IS_DOCUMENT (self), NULL);
	g_return_val_if_fail (data != NULL, NULL);

	return (DomElement *) dom_text_node_new (data);
}


DomElement *
dom_document_create_element_with_text (DomDocument  *self,
				       const char   *text_data,
				       const char   *tag_name,
				       const char   *first_attr,
				       ...)
{
	DomElement *element;
	va_list     var_args;
	const char  *attr;
	const char *value;

	g_return_val_if_fail (DOM_IS_DOCUMENT (self), NULL);
	g_return_val_if_fail (tag_name != NULL, NULL);

	element = dom_element_new (tag_name);
	va_start (var_args, first_attr);
	attr = first_attr;
        while (attr != NULL) {
        	value = va_arg (var_args, const char *);
		dom_element_set_attribute (element, attr, value);
                attr = va_arg (var_args, const char *);
        }
	va_end (var_args);

	if (text_data != NULL)
		dom_element_append_child (element, dom_document_create_text_node (self, text_data));

	return element;
}


char *
dom_document_dump (DomDocument *self,
		   gsize       *len)
{
	GString *xml;
	char    *body;

	xml = g_string_sized_new (4096);
	g_string_append (xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	g_string_append (xml, XML_RC);

	body = dom_element_dump (DOM_ELEMENT (self), -1);
	g_string_append (xml, body);
	g_free (body);

	if (dom_element_has_child_nodes (DOM_ELEMENT (self)))
		g_string_append (xml, XML_RC);

	if (len != NULL)
		*len = xml->len;

	return g_string_free (xml, FALSE);
}


static void
start_element_cb (GMarkupParseContext  *context,
                  const char           *element_name,
                  const char          **attribute_names,
                  const char          **attribute_values,
                  gpointer              user_data,
                  GError              **error)
{
	DomDocument *doc = user_data;
	DomElement  *node;
	int          i;

	node = dom_document_create_element (doc, element_name, NULL);
	for (i = 0; attribute_names[i]; i++)
		dom_element_set_attribute (node, attribute_names[i], attribute_values[i]);
	dom_element_append_child (DOM_ELEMENT (g_queue_peek_head (doc->priv->open_nodes)), node);

	g_queue_push_head (doc->priv->open_nodes, node);
}


static void
end_element_cb (GMarkupParseContext  *context,
		const char           *element_name,
		gpointer              user_data,
		GError              **error)
{
	DomDocument *doc = user_data;

	g_queue_pop_head (doc->priv->open_nodes);
}


static void
text_cb (GMarkupParseContext  *context,
	 const char           *text,
	 gsize                 text_len,
	 gpointer              user_data,
	 GError              **error)
{
	DomDocument *doc = user_data;
	char        *data;

	data = g_strndup (text, text_len);
	dom_element_append_child (DOM_ELEMENT (g_queue_peek_head (doc->priv->open_nodes)),
				  dom_document_create_text_node (doc, data));
	g_free (data);
}


static const GMarkupParser markup_parser = {
	start_element_cb,     /* start_element */
	end_element_cb,       /* end_element */
	text_cb,              /* text */
	NULL,                 /* passthrough */
	NULL                  /* error */
};


gboolean
dom_document_load (DomDocument  *self,
		   const char   *xml,
		   gssize        len,
                   GError      **error)
{
	GMarkupParseContext *context;

	g_return_val_if_fail (DOM_IS_DOCUMENT (self), FALSE);
	g_return_val_if_fail (xml != NULL, FALSE);

	g_queue_clear (self->priv->open_nodes);
	g_queue_push_head (self->priv->open_nodes, self);

	context = g_markup_parse_context_new (&markup_parser, 0, self, NULL);
	if (! g_markup_parse_context_parse (context, xml, (len < 0) ? strlen (xml) : len, error))
		return FALSE;
	if (! g_markup_parse_context_end_parse (context, error))
		return FALSE;
	g_markup_parse_context_free (context);

	return TRUE;
}


/* -- DomDomizable -- */


G_DEFINE_INTERFACE (DomDomizable, dom_domizable, 0)


static void
dom_domizable_default_init (DomDomizableInterface *iface)
{
	/* void */
}


DomElement *
dom_domizable_create_element (DomDomizable *self,
			      DomDocument  *doc)
{
	return DOM_DOMIZABLE_GET_INTERFACE (self)->create_element (self, doc);
}


void
dom_domizable_load_from_element (DomDomizable *self,
				 DomElement   *e)
{
	DOM_DOMIZABLE_GET_INTERFACE (self)->load_from_element (self, e);
}


/* -- Utilities -- */


/* GMarkupParser converts \r into \n, this function compares two strings
 * treating \r characters as they were equal to \n */
gboolean
dom_str_equal (const char *a,
	       const char *b)
{
	const char *ai, *bi;

	if ((a == NULL) && (b == NULL))
		return TRUE;

	if ((a == NULL) || (b == NULL))
		return FALSE;

	ai = a;
	bi = b;
	while ((*ai != '\0') && (*bi != '\0')) {
		if (*ai != *bi) {
			/* \r equal to \n */

			if (! (((*ai == '\r') && (*bi == '\n'))
			       || ((*ai == '\n') && (*bi == '\r'))))
			{
				return FALSE;
			}

			/* \r\n equal to \n */

			if ((*ai == '\r') && (*(ai + 1) == '\n') && (*bi == '\n') && (*(bi + 1) != '\n'))
				ai++;
			if ((*bi == '\r') && (*(bi + 1) == '\n') && (*ai == '\n') && (*(ai + 1) != '\n'))
				ai++;
		}
		ai++;
		bi++;
	}

	/* 'end of string' equal to \n and to \r\n */

	return (((*ai == '\0') && (*bi == '\0'))
		|| ((*ai == '\0') && (*bi == '\n') && (*(bi + 1) == '\0'))
		|| ((*bi == '\0') && (*ai == '\n') && (*(ai + 1) == '\0'))
		|| ((*ai == '\0') && (*bi == '\r') && (*(bi + 1) == '\n') && (*(bi + 2) == '\0'))
		|| ((*bi == '\0') && (*ai == '\r') && (*(ai + 1) == '\n') && (*(ai + 2) == '\0')));
}
