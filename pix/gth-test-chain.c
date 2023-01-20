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
#include "dom.h"
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-enum-types.h"
#include "gth-main.h"
#include "gth-test-chain.h"


struct _GthTestChainPrivate {
	GthMatchType  match_type;
	GList        *tests;
	GString      *attributes;
};


static DomDomizableInterface *dom_domizable_parent_iface = NULL;
static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;


static void gth_test_chain_dom_domizable_interface_init (DomDomizableInterface * iface);
static void gth_test_chain_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthTestChain,
			 gth_test_chain,
			 GTH_TYPE_TEST,
			 G_ADD_PRIVATE (GthTestChain)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_test_chain_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_test_chain_gth_duplicable_interface_init))


static void
gth_test_chain_finalize (GObject *object)
{
	GthTestChain *test;

	test = GTH_TEST_CHAIN (object);

	_g_object_list_unref (test->priv->tests);
	if (test->priv->attributes != NULL)
		g_string_free (test->priv->attributes, TRUE);

	G_OBJECT_CLASS (gth_test_chain_parent_class)->finalize (object);
}


static GthMatch
gth_test_chain_match (GthTest     *test,
		      GthFileData *file)
{
	GthTestChain *chain;
	GthMatch      match = GTH_MATCH_NO;
	GList        *scan;

        chain = GTH_TEST_CHAIN (test);

	if (chain->priv->match_type == GTH_MATCH_TYPE_NONE)
		return GTH_MATCH_YES;

	match = (chain->priv->match_type == GTH_MATCH_TYPE_ALL) ? GTH_MATCH_YES : GTH_MATCH_NO;
	for (scan = chain->priv->tests; scan; scan = scan->next) {
		GthTest *test = scan->data;

		if (gth_test_match (test, file)) {
			if (chain->priv->match_type == GTH_MATCH_TYPE_ANY) {
				match = GTH_MATCH_YES;
				break;
			}
		}
		else if (chain->priv->match_type == GTH_MATCH_TYPE_ALL) {
			match = GTH_MATCH_NO;
			break;
		}
	}

	return match;
}



static const char *
gth_test_chain_get_attributes (GthTest *test)
{
	GthTestChain *chain = GTH_TEST_CHAIN (test);
	GList        *scan;

	if (chain->priv->attributes != NULL)
		g_string_free (chain->priv->attributes, TRUE);
	chain->priv->attributes = g_string_new ("");

	for (scan = chain->priv->tests; scan; scan = scan->next) {
		GthTest    *test = scan->data;
		const char *child_attributes;

		child_attributes = gth_test_get_attributes (test);
		if ((child_attributes != NULL) && (child_attributes[0] != '\0')) {
			if (chain->priv->attributes->len > 0)
				g_string_append (chain->priv->attributes, ",");
			g_string_append (chain->priv->attributes, child_attributes);
		}
	}

	return chain->priv->attributes->str;
}


static void
gth_test_chain_class_init (GthTestChainClass *class)
{
	GObjectClass *object_class;
	GthTestClass *test_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_test_chain_finalize;

	test_class = (GthTestClass *) class;
	test_class->match = gth_test_chain_match;
	test_class->get_attributes = gth_test_chain_get_attributes;
}


static DomElement*
gth_test_chain_real_create_element (DomDomizable *base,
				    DomDocument  *doc)
{
	GthTestChain *self;
	DomElement   *element;
	GList        *scan;

	self = GTH_TEST_CHAIN (base);

	element = dom_document_create_element (doc, "tests",
					       "match", _g_enum_type_get_value (GTH_TYPE_MATCH_TYPE, self->priv->match_type)->value_nick,
					       NULL);
	for (scan = self->priv->tests; scan; scan = scan->next)
		dom_element_append_child (element, dom_domizable_create_element (DOM_DOMIZABLE (scan->data), doc));

	return element;
}


static void
gth_test_chain_real_load_from_element (DomDomizable *base,
				       DomElement   *element)
{
	GthTestChain *chain;
	GEnumValue   *enum_value;
	DomElement   *node;

	chain = GTH_TEST_CHAIN (base);

	enum_value = _g_enum_type_get_value_by_nick (GTH_TYPE_MATCH_TYPE, dom_element_get_attribute (element, "match"));
	if (enum_value != NULL)
		chain->priv->match_type = enum_value->value;

	gth_test_chain_clear_tests (chain);
	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "test") == 0) {
			GthTest *test;

			test = gth_main_get_registered_object (GTH_TYPE_TEST, dom_element_get_attribute (node, "id"));
			if (test == NULL)
				continue;

			dom_domizable_load_from_element (DOM_DOMIZABLE (test), node);
			gth_test_chain_add_test (chain, test);
			g_object_unref (test);
		}
		else if (g_strcmp0 (node->tag_name, "tests") == 0) {
			GthTest *sub_chain;

			sub_chain = gth_test_chain_new (GTH_MATCH_TYPE_NONE, NULL);
			dom_domizable_load_from_element (DOM_DOMIZABLE (sub_chain), node);
			gth_test_chain_add_test (chain, sub_chain);
			g_object_unref (sub_chain);
		}
	}
}


static void
gth_test_chain_dom_domizable_interface_init (DomDomizableInterface * iface)
{
	dom_domizable_parent_iface = g_type_interface_peek_parent (iface);
	iface->create_element = gth_test_chain_real_create_element;
	iface->load_from_element = gth_test_chain_real_load_from_element;
}


static GObject *
gth_test_chain_real_duplicate (GthDuplicable *duplicable)
{
	GthTestChain *chain;
	GthTest      *new_chain;
	GList        *tests, *scan;

	chain = GTH_TEST_CHAIN (duplicable);

	new_chain = gth_test_chain_new (chain->priv->match_type, NULL);
	tests = gth_test_chain_get_tests (chain);
	for (scan = tests; scan; scan = scan->next)
		gth_test_chain_add_test (GTH_TEST_CHAIN (new_chain), scan->data);
	_g_object_list_unref (tests);

	return G_OBJECT (new_chain);
}


static void
gth_test_chain_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_test_chain_real_duplicate;
}


static void
gth_test_chain_init (GthTestChain *test)
{
	test->priv = gth_test_chain_get_instance_private (test);
	test->priv->match_type = GTH_MATCH_TYPE_NONE;
	test->priv->tests = NULL;
	test->priv->attributes = NULL;
}


GthTest *
gth_test_chain_new (GthMatchType  match_type,
		    ...)
{
	GthTestChain *chain;
	va_list       args;
	GthTest      *test;

	chain = g_object_new (GTH_TYPE_TEST_CHAIN, NULL);
	chain->priv->match_type = match_type;

	va_start (args, match_type);
	while ((test = va_arg (args, GthTest *)) != NULL) {
		gth_test_chain_add_test (chain, g_object_ref (test));
	}
	va_end (args);

	return (GthTest *) chain;
}


void
gth_test_chain_set_match_type (GthTestChain *chain,
			       GthMatchType  match_type)
{
	chain->priv->match_type = match_type;
}


GthMatchType
gth_test_chain_get_match_type (GthTestChain *chain)
{
	return chain->priv->match_type;
}


void
gth_test_chain_clear_tests (GthTestChain *chain)
{
	if (chain->priv->tests == NULL)
		return;
	_g_object_list_unref (chain->priv->tests);
	chain->priv->tests = NULL;
	g_object_set (chain, "attributes", "", NULL);
}


void
gth_test_chain_add_test (GthTestChain *chain,
			 GthTest      *test)
{
	const char *old_attributes;
	const char *test_attributes;
	gboolean    separator;
	char       *attributes;

	g_object_set (test, "visible", TRUE, NULL);
	chain->priv->tests = g_list_append (chain->priv->tests, g_object_ref (test));

	old_attributes = gth_test_get_attributes (GTH_TEST (chain));
	test_attributes = gth_test_get_attributes (test);
	if (old_attributes == NULL) {
		old_attributes = test_attributes;
		test_attributes = NULL;
	}
	separator = (old_attributes != NULL) && (test_attributes != NULL) && (old_attributes[0] != '\0') && (test_attributes[0] != '\0');
	attributes = g_strconcat (old_attributes,
			          (separator ? "," : ""),
			          test_attributes,
			          NULL);
	g_object_set (chain, "attributes", attributes, NULL);

	g_free (attributes);
}


GList *
gth_test_chain_get_tests (GthTestChain *chain)
{
	return _g_object_list_ref (chain->priv->tests);
}


gboolean
gth_test_chain_has_type_test (GthTestChain *chain)
{
	gboolean  result = FALSE;
	GList    *scan;

	for (scan = chain->priv->tests; scan; scan = scan->next) {
		GthTest *single_test = scan->data;

		if (strncmp (gth_test_get_id (single_test), "file::type::", 12) == 0) {
			result = TRUE;
			break;
		}
	}

	return result;
}
