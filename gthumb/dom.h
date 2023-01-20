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

#ifndef DOM_H
#define DOM_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DOM_ERROR dom_error_quark ()
typedef enum {
	DOM_ERROR_FAILED,
	DOM_ERROR_INVALID_FORMAT
} DomErrorEnum;

#define DOM_TYPE_ELEMENT            (dom_element_get_type ())
#define DOM_ELEMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DOM_TYPE_ELEMENT, DomElement))
#define DOM_ELEMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_ELEMENT, DomElementClass))
#define DOM_IS_ELEMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DOM_TYPE_ELEMENT))
#define DOM_IS_ELEMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_ELEMENT))
#define DOM_ELEMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_ELEMENT, DomElementClass))

typedef struct _DomElement DomElement;
typedef struct _DomElementClass DomElementClass;
typedef struct _DomElementPrivate DomElementPrivate;

#define DOM_TYPE_TEXT_NODE            (dom_text_node_get_type ())
#define DOM_TEXT_NODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DOM_TYPE_TEXT_NODE, DomTextNode))
#define DOM_TEXT_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_TEXT_NODE, DomTextNodeClass))
#define DOM_IS_TEXT_NODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DOM_TYPE_TEXT_NODE))
#define DOM_IS_TEXT_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_TEXT_NODE))
#define DOM_TEXT_NODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_TEXT_NODE, DomTextNodeClass))

typedef struct _DomTextNode DomTextNode;
typedef struct _DomTextNodeClass DomTextNodeClass;
typedef struct _DomTextNodePrivate DomTextNodePrivate;

#define DOM_TYPE_DOCUMENT            (dom_document_get_type ())
#define DOM_DOCUMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DOM_TYPE_DOCUMENT, DomDocument))
#define DOM_DOCUMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_DOCUMENT, DomDocumentClass))
#define DOM_IS_DOCUMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DOM_TYPE_DOCUMENT))
#define DOM_IS_DOCUMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_DOCUMENT))
#define DOM_DOCUMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_DOCUMENT, DomDocumentClass))

typedef struct _DomDocument DomDocument;
typedef struct _DomDocumentClass DomDocumentClass;
typedef struct _DomDocumentPrivate DomDocumentPrivate;

#define DOM_TYPE_DOMIZABLE               (dom_domizable_get_type ())
#define DOM_DOMIZABLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), DOM_TYPE_DOMIZABLE, DomDomizable))
#define DOM_IS_DOMIZABLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DOM_TYPE_DOMIZABLE))
#define DOM_DOMIZABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), DOM_TYPE_DOMIZABLE, DomDomizableInterface))

typedef struct _DomDomizable DomDomizable;
typedef struct _DomDomizableInterface DomDomizableInterface;

struct _DomElement {
	GInitiallyUnowned parent_instance;
	DomElementPrivate *priv;

	char       *tag_name;
	char       *prefix;
	GHashTable *attributes;
	DomElement *next_sibling;
	DomElement *previous_sibling;
	DomElement *parent_node;
	GList      *child_nodes;
	DomElement *first_child;
	DomElement *last_child;
};

struct _DomElementClass {
	GInitiallyUnownedClass parent_class;

	char *		(*dump)		(DomElement *self,
					 int         level);
	gboolean	(*equal)	(DomElement *self,
					 DomElement *other);
};

struct _DomTextNode {
	DomElement parent_instance;
	DomTextNodePrivate *priv;

	char *data;
};

struct _DomTextNodeClass {
	DomElementClass parent_class;
};

struct _DomDocument {
	DomElement parent_instance;
	DomDocumentPrivate *priv;
};

struct _DomDocumentClass {
	DomElementClass parent_class;
};

struct _DomDomizableInterface {
	GTypeInterface parent_iface;

	DomElement * (*create_element)    (DomDomizable *self,
					   DomDocument  *doc);
	void         (*load_from_element) (DomDomizable *self,
					   DomElement   *e);
};

GQuark        dom_error_quark                       (void);

/* DomElement */

GType         dom_element_get_type                  (void);
void          dom_element_append_child              (DomElement   *self,
					             DomElement   *child);
const char *  dom_element_get_attribute             (DomElement   *self,
					             const char   *name);
int           dom_element_get_attribute_as_int      (DomElement   *self,
						     const char   *name);
gboolean      dom_element_has_attribute             (DomElement   *self,
					             const char   *name);
gboolean      dom_element_has_child_nodes           (DomElement   *self);
void          dom_element_remove_attribute          (DomElement   *self,
					             const char   *name);
DomElement *  dom_element_remove_child              (DomElement   *self,
					             DomElement   *node);
void          dom_element_replace_child             (DomElement   *self,
					             DomElement   *new_child,
					             DomElement   *old_child);
void          dom_element_set_attribute             (DomElement   *self,
					             const char   *name,
					             const char   *value);
const char *  dom_element_get_inner_text            (DomElement   *self);

/* DomTextNode */

GType         dom_text_node_get_type                (void);

/* DomDocument */

GType         dom_document_get_type                 (void);
DomDocument * dom_document_new                      (void);
DomElement *  dom_document_create_element           (DomDocument  *self,
					             const char   *tag_name,
					             ...) G_GNUC_NULL_TERMINATED;
DomElement *  dom_document_create_text_node         (DomDocument  *self,
					             const char   *data);
DomElement *  dom_document_create_element_with_text (DomDocument  *self,
						     const char   *text,
						     const char   *tag_name,
						     ...) G_GNUC_NULL_TERMINATED;
char *        dom_document_dump                     (DomDocument  *self,
					             gsize        *len);
gboolean      dom_document_load                     (DomDocument  *self,
					             const char   *xml,
					             gssize        len,
					             GError      **error);
gboolean      dom_document_equal                    (DomDocument   *a,
						     DomDocument   *b);

/* DomDomizable */

GType         dom_domizable_get_type                (void);
DomElement *  dom_domizable_create_element          (DomDomizable  *self,
					             DomDocument   *doc);
void          dom_domizable_load_from_element       (DomDomizable  *self,
					             DomElement    *e);

/* Utilities */

gboolean      dom_str_equal                         (const char    *a,
						     const char    *b);
int           dom_str_find			    (const char    *a,
						     const char    *b);

G_END_DECLS

#endif /* DOM_H */
