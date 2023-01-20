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


static void
compare_loaded_and_dumped_xml (DomDocument *doc)
{
	char        *xml;
	gsize        len;
	DomDocument *loaded_doc;

	xml = dom_document_dump (doc, &len);
	loaded_doc = dom_document_new ();
	dom_document_load (loaded_doc, xml, len, NULL);
	g_assert_true (dom_document_equal (doc, loaded_doc));

	g_object_unref (loaded_doc);
	g_free (xml);
}


static void
check_dumped_xml (DomDocument *doc,
		  const char  *expected_xml)
{
	char *xml;

	xml = dom_document_dump (doc, NULL);
	g_assert_cmpstr (xml, ==, expected_xml);
	g_free (xml);
}


static void
test_dom_1 (void)
{
	DomDocument *doc;

	doc = dom_document_new ();

	compare_loaded_and_dumped_xml (doc);
	check_dumped_xml (doc, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

	g_object_unref (doc);
}


static void
test_dom_2 (void)
{
	DomDocument *doc;
	DomElement  *filters;

	doc = dom_document_new ();
	filters = dom_document_create_element (doc, "filters", "version", "1.0", NULL);
	dom_element_append_child (DOM_ELEMENT (doc), filters);

	compare_loaded_and_dumped_xml (doc);
	check_dumped_xml (doc,  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<filters version=\"1.0\"/>\n");

	g_object_unref (doc);
}


static void
test_dom_3 (void)
{
	DomDocument *doc;
	DomElement  *filters;
	DomElement  *filter;
	DomElement  *match;
	DomElement  *test;
	DomElement  *limit;

	doc = dom_document_new ();
	filters = dom_document_create_element (doc, "filters", NULL);
	dom_element_set_attribute (filters, "version", "1.0");
	dom_element_append_child (DOM_ELEMENT (doc), filters);

	filter = dom_document_create_element (doc, "filter", NULL);
	dom_element_set_attribute (filter, "name", "test1");
	dom_element_append_child (filters, filter);

	match = dom_document_create_element (doc, "match", NULL);
	dom_element_set_attribute (match, "type", "all");
	dom_element_append_child (filter, match);

	test = dom_document_create_element (doc, "test", NULL);
	dom_element_set_attribute (test, "id", "::filesize");
	dom_element_set_attribute (test, "op", "lt");
	dom_element_set_attribute (test, "value", "10");
	dom_element_set_attribute (test, "unit", "kB");
	dom_element_append_child (match, test);

	test = dom_document_create_element (doc, "test", NULL);
	dom_element_set_attribute (test, "id", "::filename");
	dom_element_set_attribute (test, "op", "contains");
	dom_element_set_attribute (test, "value", "logo");
	dom_element_append_child (match, test);

	limit = dom_document_create_element (doc, "limit", NULL);
	dom_element_set_attribute (limit, "value", "25");
	dom_element_set_attribute (limit, "type", "files");
	dom_element_set_attribute (limit, "selected_by", "more_recent");
	dom_element_append_child (filter, limit);

	filter = dom_document_create_element (doc, "filter", NULL);
	dom_element_set_attribute (filter, "name", "test2");
	dom_element_append_child (filters, filter);

	limit = dom_document_create_element (doc, "limit", NULL);
	dom_element_set_attribute (limit, "value", "25");
	dom_element_set_attribute (limit, "type", "files");
	dom_element_set_attribute (limit, "selected_by", "more_recent");
	dom_element_append_child (filter, limit);

	compare_loaded_and_dumped_xml (doc);

	check_dumped_xml (doc,  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			        "<filters version=\"1.0\">\n"
				"  <filter name=\"test1\">\n"
				"    <match type=\"all\">\n"
				"      <test id=\"::filesize\" op=\"lt\" unit=\"kB\" value=\"10\"/>\n"
				"      <test id=\"::filename\" op=\"contains\" value=\"logo\"/>\n"
				"    </match>\n"
				"    <limit selected_by=\"more_recent\" type=\"files\" value=\"25\"/>\n"
				"  </filter>\n"
				"  <filter name=\"test2\">\n"
				"    <limit selected_by=\"more_recent\" type=\"files\" value=\"25\"/>\n"
				"  </filter>\n"
				"</filters>\n");

	g_object_unref (doc);
}


int
main (int   argc,
      char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/dom/1", test_dom_1);
	g_test_add_func ("/dom/2", test_dom_2);
	g_test_add_func ("/dom/3", test_dom_3);

	return g_test_run ();
}
