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
#include <string.h>
#include <locale.h>
#include "glib-utils.h"


static void
test_g_rand_string (void)
{
	char *id;

	id = _g_str_random (8);
	g_assert_cmpint (strlen (id), == , 8);
	g_free (id);

	id = _g_str_random (16);
	g_assert_cmpint (strlen (id), == , 16);
	g_free (id);
}


static void
test_regexp (void)
{
	GRegex   *re;
	char    **a;
	int       i, n, j;
	char    **b;
	char     *attributes;

	re = g_regex_new ("%prop\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, "thunderbird -compose %prop{ Exif::Image::DateTime } %ask %prop{ File::Size }", 0);
	for (i = 0, n = 0; a[i] != NULL; i += 2)
		n++;

	b = g_new (char *, n + 1);
	for (i = 1, j = 0; a[i] != NULL; i += 2) {
		b[j++] = g_strstrip (a[i]);
	}
	b[j] = NULL;
	attributes = g_strjoinv (",", b);

	g_assert_cmpstr (attributes, ==, "Exif::Image::DateTime,File::Size");

	g_free (attributes);
	g_free (b);
	g_strfreev (a);
	g_regex_unref (re);
}


static void
test_g_utf8_has_prefix (void)
{
	g_assert_false (_g_utf8_has_prefix (NULL, NULL));
	g_assert_false (_g_utf8_has_prefix (NULL, ""));
	g_assert_false (_g_utf8_has_prefix ("", NULL));
	g_assert_true (_g_utf8_has_prefix ("", ""));
	g_assert_true (_g_utf8_has_prefix ("日", ""));
	g_assert_false (_g_utf8_has_prefix ("", "日"));
	g_assert_true (_g_utf8_has_prefix ("日本語", "日"));
	g_assert_false (_g_utf8_has_prefix ("日", "日本"));
	g_assert_false (_g_utf8_has_prefix ("日", "日本語"));
	g_assert_true (_g_utf8_has_prefix ("lang=正體字/繁體字 中华人民共和国", "lang="));
}


static void
test_g_utf8_after_ascii_space (void)
{
	g_assert_cmpstr (_g_utf8_after_ascii_space (NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_after_ascii_space (""), ==, NULL);
	g_assert_cmpstr (_g_utf8_after_ascii_space ("正體字 "), ==, "");
	g_assert_cmpstr (_g_utf8_after_ascii_space ("lang=FR langue d’oïl"), ==, "langue d’oïl");
	g_assert_cmpstr (_g_utf8_after_ascii_space ("正體字"), ==, NULL);
	g_assert_cmpstr (_g_utf8_after_ascii_space ("lang=正體字/繁體字 中华人民共和国"), ==, "中华人民共和国");
}


static void
test_g_utf8_all_spaces_space (void)
{
	g_assert_true (_g_utf8_all_spaces (NULL));
	g_assert_true (_g_utf8_all_spaces (""));
	g_assert_true (_g_utf8_all_spaces (" "));
	g_assert_false (_g_utf8_all_spaces ("."));
	g_assert_false (_g_utf8_all_spaces ("正"));
}


static void
test_g_utf8_escape_xml_space (void)
{
	g_assert_cmpstr (_g_utf8_escape_xml (NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_escape_xml (""), ==, "");
	g_assert_cmpstr (_g_utf8_escape_xml ("ascii"), ==, "ascii");
	g_assert_cmpstr (_g_utf8_escape_xml ("正"), ==, "&#27491;");
	g_assert_cmpstr (_g_utf8_escape_xml ("體"), ==, "&#39636;");
	g_assert_cmpstr (_g_utf8_escape_xml ("字"), ==, "&#23383;");
	g_assert_cmpstr (_g_utf8_escape_xml ("正體字"), ==, "&#27491;&#39636;&#23383;");
	g_assert_cmpstr (_g_utf8_escape_xml ("正\\<體"), ==, "&#27491;\\&lt;&#39636;");
	g_assert_cmpstr (_g_utf8_escape_xml ("正>體"), ==, "&#27491;&gt;&#39636;");
	g_assert_cmpstr (_g_utf8_escape_xml ("正'體"), ==, "&#27491;&apos;&#39636;");
	g_assert_cmpstr (_g_utf8_escape_xml ("正\"體"), ==, "&#27491;&quot;&#39636;");
	g_assert_cmpstr (_g_utf8_escape_xml ("正&體"), ==, "&#27491;&amp;&#39636;");
	g_assert_cmpstr (_g_utf8_text_escape_xml ("\n"), ==, "<br>");
	g_assert_cmpstr (_g_utf8_text_escape_xml ("正\n體"), ==, "&#27491;<br>&#39636;");
}


static void
test_g_utf8_find_str (void)
{
	g_assert_cmpstr (_g_utf8_find_str ("正體字", "正"), ==, "正體字");
	g_assert_cmpstr (_g_utf8_find_str ("正體字", "體"), ==, "體字");
	g_assert_cmpstr (_g_utf8_find_str ("正體字", "字"), ==, "字");
	g_assert_cmpstr (_g_utf8_find_str ("正體", "字"), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_str ("正體", NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_str (NULL, NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_str (NULL, "字"), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_str ("正體", ""), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_str ("", ""), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_str ("", "字"), ==, NULL);
}


static void
test_remove_properties_from_utf8_string (const char *value,
					 const char *expected)
{
	char *result = NULL;

	result = _g_utf8_remove_string_properties (value);
	g_assert_cmpstr (result, ==, expected);

	g_free (result);
}


static void
test_g_utf8_remove_string_properties (void)
{
	test_remove_properties_from_utf8_string (NULL, NULL);
	test_remove_properties_from_utf8_string ("", "");
	test_remove_properties_from_utf8_string ("正體字", "正體字");
	test_remove_properties_from_utf8_string ("langue d’oïl", "langue d’oïl");
	test_remove_properties_from_utf8_string ("lang=FR langue d’oïl", "langue d’oïl");
	test_remove_properties_from_utf8_string ("lang=正體字/繁體字 中华人民共和国", "中华人民共和国");
	test_remove_properties_from_utf8_string ("charset=UTF-8 langue d’oïl", "langue d’oïl");
	test_remove_properties_from_utf8_string ("lang=FR charset=UTF-8 langue d’oïl", "langue d’oïl");
	test_remove_properties_from_utf8_string ("charset=UTF-8 lang=FR langue d’oïl", "langue d’oïl");
	test_remove_properties_from_utf8_string ("lang=FR charset=UTF-8 field=value langue d’oïl", "field=value langue d’oïl");
	test_remove_properties_from_utf8_string ("lang=FR field=value charset=UTF-8 langue d’oïl", "field=value charset=UTF-8 langue d’oïl");
	test_remove_properties_from_utf8_string ("field=value lang=FR charset=UTF-8 langue d’oïl", "field=value lang=FR charset=UTF-8 langue d’oïl");
}


static void
test_g_utf8_find_expr (void)
{
	g_assert_cmpstr (_g_utf8_find_expr (NULL, NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr ("", NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr (NULL, ""), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr ("", ""), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr ("日", ""), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr ("日", "日"), ==, "日");
	g_assert_cmpstr (_g_utf8_find_expr ("日本語", "日"), ==, "日");
	g_assert_cmpstr (_g_utf8_find_expr ("日本語", "語"), ==, "語");
	g_assert_cmpstr (_g_utf8_find_expr ("日本語", "本本"), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr ("日本語", "[0-9]+"), ==, NULL);
	g_assert_cmpstr (_g_utf8_find_expr ("日本1234語", "[0-9]+"), ==, "1234");
	g_assert_cmpstr (_g_utf8_find_expr ("日1本9876語", "[0-9]+"), ==, "1");
	g_assert_cmpstr (_g_utf8_find_expr ("日1234本9876語", "[0-9]+"), ==, "1234");
}


static void
test_g_path_get_parent_all (void)
{
	g_assert_cmpstr (_g_path_get_parent (NULL), ==, NULL);
	g_assert_cmpstr (_g_path_get_parent (""), ==, NULL);
	g_assert_cmpstr (_g_path_get_parent ("/"), ==, "/");
	g_assert_cmpstr (_g_path_get_parent ("/日"), ==, "/");
	g_assert_cmpstr (_g_path_get_parent ("/日/"), ==, "/日");
	g_assert_cmpstr (_g_path_get_parent ("/日/本"), ==, "/日");
	g_assert_cmpstr (_g_path_get_parent ("/日/本/"), ==, "/日/本");
	g_assert_cmpstr (_g_path_get_parent ("/日/本/語.txt"), ==, "/日/本");
	g_assert_cmpstr (_g_path_get_parent ("日"), ==, NULL);
	g_assert_cmpstr (_g_path_get_parent ("日/"), ==, "日");
	g_assert_cmpstr (_g_path_get_parent ("日/本"), ==, "日");
	g_assert_cmpstr (_g_path_get_parent ("日/本/"), ==, "日/本");
	g_assert_cmpstr (_g_path_get_parent ("日/本/語.txt"), ==, "日/本");
}


static void
test_g_path_get_basename_all (void)
{
	g_assert_cmpstr (_g_path_get_basename (NULL), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename (""), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("/"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("//"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("///"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("////"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("//a//"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("/日"), ==, "日");
	g_assert_cmpstr (_g_path_get_basename ("/日/"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("/日/本"), ==, "本");
	g_assert_cmpstr (_g_path_get_basename ("/日/本/語.txt"), ==, "語.txt");
	g_assert_cmpstr (_g_path_get_basename ("日"), ==, "日");
	g_assert_cmpstr (_g_path_get_basename ("日/"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("日/本"), ==, "本");
	g_assert_cmpstr (_g_path_get_basename ("日/本/"), ==, NULL);
	g_assert_cmpstr (_g_path_get_basename ("日/本/語.txt"), ==, "語.txt");
}


static void
test_g_uri_append_path_all (void)
{
	g_assert_cmpstr (_g_uri_append_path (NULL, NULL), ==, NULL);
	g_assert_cmpstr (_g_uri_append_path (NULL, ""), ==, NULL);
	g_assert_cmpstr (_g_uri_append_path (NULL, "/"), ==, NULL);
	g_assert_cmpstr (_g_uri_append_path (NULL, "/日"), ==, NULL);

	g_assert_cmpstr (_g_uri_append_path ("", NULL), ==, NULL);
	g_assert_cmpstr (_g_uri_append_path ("", ""), ==, NULL);
	g_assert_cmpstr (_g_uri_append_path ("", "/"), ==, NULL);
	g_assert_cmpstr (_g_uri_append_path ("", "/日"), ==, NULL);

	g_assert_cmpstr (_g_uri_append_path ("file:", NULL), ==, "file:///");
	g_assert_cmpstr (_g_uri_append_path ("file:", ""), ==, "file:///");
	g_assert_cmpstr (_g_uri_append_path ("file:", "/"), ==, "file:///");
	g_assert_cmpstr (_g_uri_append_path ("file://", "/"), ==, "file:///");
	g_assert_cmpstr (_g_uri_append_path ("file:///", "/"), ==, "file:///");

	g_assert_cmpstr (_g_uri_append_path ("file:", "日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_append_path ("file://", "日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_append_path ("file:///", "日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_append_path ("file:///", "日/本/語"), ==, "file:///日/本/語");
	g_assert_cmpstr (_g_uri_append_path ("catalog:///", "Tags"), ==, "catalog:///Tags");

	g_assert_cmpstr (_g_uri_append_path ("file:", "/日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_append_path ("file://", "/日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_append_path ("file:///", "/日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_append_path ("file:///", "/日/本/語"), ==, "file:///日/本/語");
}


static void
test_g_uri_from_path_all (void)
{
	g_assert_cmpstr (_g_uri_from_path ("/"), ==, "file:///");
	g_assert_cmpstr (_g_uri_from_path ("/日"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_from_path ("/日/"), ==, "file:///日/");
	g_assert_cmpstr (_g_uri_from_path ("/日/本/語"), ==, "file:///日/本/語");
	g_assert_cmpstr (_g_uri_from_path ("/日 本"), ==, "file:///日%20本");
}


static void
test_g_uri_get_basename_all (void)
{
	g_assert_cmpstr (_g_uri_get_basename ("file:///home/paolo/"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:///home/paolo/file.txt"), ==, "file.txt");
	g_assert_cmpstr (_g_uri_get_basename ("file:///file.txt"), ==, "file.txt");
	g_assert_cmpstr (_g_uri_get_basename ("file://file.txt"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:file.txt"), ==, "file.txt");
	g_assert_cmpstr (_g_uri_get_basename ("file.txt"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("/file.txt"), ==, "file.txt");

	g_assert_cmpstr (_g_uri_get_basename (NULL), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename (""), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:/"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file://"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:///"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:///%E6%97%A5"), ==, "日");
	g_assert_cmpstr (_g_uri_get_basename ("file:///%E6%97%A5/"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_basename ("file:///%E6%97%A5/%E6%9C%AC/%E8%AA%9E"), ==, "語");
}


static void
test_g_uri_get_parent_all (void)
{
	g_assert_cmpstr (_g_uri_get_parent ("file"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_parent ("file:"), ==, "file:///");
	g_assert_cmpstr (_g_uri_get_parent ("file://"), ==, "file:///");
	g_assert_cmpstr (_g_uri_get_parent ("file:///"), ==, "file:///");
	g_assert_cmpstr (_g_uri_get_parent ("file:///日"), ==, "file:///");
	g_assert_cmpstr (_g_uri_get_parent ("file:///日/本"), ==, "file:///日");
	g_assert_cmpstr (_g_uri_get_parent ("file:///日/本/語.txt"), ==, "file:///日/本");
	g_assert_cmpstr (_g_uri_get_parent ("file:///%E6%97%A5/%E6%9C%AC/%E8%AA%9E"), ==, "file:///日/本");
	g_assert_cmpstr (_g_uri_get_parent ("file://日/本/語.txt"), ==, "file://日/本");
	g_assert_cmpstr (_g_uri_get_parent ("file://諸星@日:123/本/語.txt"), ==, "file://諸星@日:123/本");
	g_assert_cmpstr (_g_uri_get_parent ("file://諸星@日:123/本"), ==, "file://諸星@日:123/");
	g_assert_cmpstr (_g_uri_get_parent ("file://諸星@日:123/"), ==, "file://諸星@日:123/");
}


static void
test_g_file_get_display_name (const char *uri,
			      const char *expected)
{
	GFile *file;
	char  *name;

	file = g_file_new_for_uri (uri);
	name = _g_file_get_display_name (file);
	g_assert_cmpstr (name, ==, expected);

	g_free (name);
	g_object_unref (file);
}


static void
test_g_file_get_display_name_all (void)
{
	test_g_file_get_display_name ("sftp:///", "/");
	test_g_file_get_display_name ("sftp://日本語", "日本語");
	test_g_file_get_display_name ("sftp://日本語/", "日本語");
	test_g_file_get_display_name ("sftp://日本語/諸星.txt", "諸星.txt");
	test_g_file_get_display_name ("file:///日/本/諸星.txt", "諸星.txt");
	test_g_file_get_display_name ("file:///日本語/諸星", "諸星");
	test_g_file_get_display_name ("file:///", "/");
}


static void
test_g_utf8_n_equal_all (void)
{
	g_assert_true (_g_utf8_n_equal (NULL, NULL, 0));
	g_assert_true (_g_utf8_n_equal ("", "", 0));
	g_assert_true (_g_utf8_n_equal ("日本語", "日", 1));
	g_assert_true (_g_utf8_n_equal ("日", "日本語", 1));
	g_assert_true (_g_utf8_n_equal (".tar.日", ".tar.", 5));
	g_assert_false (_g_utf8_n_equal ("日本", "日本語", 3));
	g_assert_false (_g_utf8_n_equal ("日", "日本語", 3));
	g_assert_false (_g_utf8_n_equal ("", "日本語", 3));
	g_assert_false (_g_utf8_n_equal ("日", "日本語", 2));
	g_assert_false (_g_utf8_n_equal ("日", "日本", 2));
	g_assert_false (_g_utf8_n_equal ("日", "日", 2));
	g_assert_true (_g_utf8_n_equal ("日", "日", 1));
	g_assert_true (_g_utf8_n_equal ("本", "日", 0));
	g_assert_false (_g_utf8_n_equal ("本", "日", 1));
	g_assert_true (_g_utf8_n_equal ("日本", "日", 1));
}


static void
test_g_utf8_last_char_all (void)
{
	g_assert_cmpstr (_g_utf8_last_char (NULL, NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_last_char ("", NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_last_char ("日", NULL), ==, "日");
	g_assert_cmpstr (_g_utf8_last_char ("日本", NULL), ==, "本");
	g_assert_cmpstr (_g_utf8_last_char ("日本語", NULL), ==, "語");
}


static void
test_g_utf8_replace_str_all (void)
{
	g_assert_cmpstr (_g_utf8_replace_str (NULL, NULL, NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_replace_str (NULL, "の", "-"), ==, NULL);
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", NULL, NULL), ==, "正の體の字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "の", NULL), ==, "正體字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "の", ""), ==, "正體字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "", NULL), ==, "正の體の字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "", ""), ==, "正の體の字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "", "-"), ==, "正の體の字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "の", "-"), ==, "正-體-字");
	g_assert_cmpstr (_g_utf8_replace_str ("正の體の字", "-", "の"), ==, "正の體の字");
}


static void
_g_assert_strv_equal (char **strv, ...)
{
	va_list     args;
	int         i;
	const char *str;

	va_start (args, strv);
	i = 0;
	while ((str = va_arg (args, const char *)) != NULL) {
		g_assert_cmpstr (strv[i], ==, str);
		i++;
	}
	va_end (args);

	g_assert_cmpstr (strv[i], ==, NULL);
}


static void
test_g_utf8_split_all (void)
{
	char **strv;

	strv = _g_utf8_split (NULL, NULL, -1);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", NULL, -1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split (NULL, "の", -1);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", "の", -1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split (NULL, "", -1);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("", "", -1);
	_g_assert_strv_equal (strv, "", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("", "の", -1);
	_g_assert_strv_equal (strv, "", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の", "の", -1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體", "の", -1);
	_g_assert_strv_equal (strv, "正", "體", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の", "の", -1);
	_g_assert_strv_equal (strv, "正", "體", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の字", "の", -1);
	_g_assert_strv_equal (strv, "正", "體", "字", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正體字", "", -1);
	_g_assert_strv_equal (strv, "正", "體", "字", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split (NULL, NULL, 0);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("", "", 0);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", NULL, 0);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split (NULL, NULL, 1);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("", "", 1);
	_g_assert_strv_equal (strv, "", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", NULL, 1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", "", 1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", "正", 1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", "の", 1);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の", "の", 1);
	_g_assert_strv_equal (strv, "正の", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體", "の", 1);
	_g_assert_strv_equal (strv, "正の體", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の", "の", 1);
	_g_assert_strv_equal (strv, "正の體の", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正", "の", 2);
	_g_assert_strv_equal (strv, "正", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體", "の", 2);
	_g_assert_strv_equal (strv, "正", "體", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の", "の", 2);
	_g_assert_strv_equal (strv, "正", "體の", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の字", "の", 2);
	_g_assert_strv_equal (strv, "正", "體の字", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の字", "の", 3);
	_g_assert_strv_equal (strv, "正", "體", "字", NULL);
	g_strfreev (strv);

	strv = _g_utf8_split ("正の體の字", "の", 4);
	_g_assert_strv_equal (strv, "正", "體", "字", NULL);
	g_strfreev (strv);
}


static void
test_g_template_tokenize_all (void)
{
	char **strv;

	strv = _g_template_tokenize ("", 0);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("xxx##yy#", 0);
	_g_assert_strv_equal (strv, "xxx", "##", "yy", "#", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("日", 0);
	_g_assert_strv_equal (strv, "日", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("日本", 0);
	_g_assert_strv_equal (strv, "日本", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("#", 0);
	_g_assert_strv_equal (strv, "#", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("##", 0);
	_g_assert_strv_equal (strv, "##", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("#日", 0);
	_g_assert_strv_equal (strv, "#", "日", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("日#", 0);
	_g_assert_strv_equal (strv, "日", "#", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("日#本", 0);
	_g_assert_strv_equal (strv, "日", "#", "本", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("日#本#", 0);
	_g_assert_strv_equal (strv, "日", "#", "本", "#", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("日#本#語", 0);
	_g_assert_strv_equal (strv, "日", "#", "本", "#", "語", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日", 0);
	_g_assert_strv_equal (strv, "%日", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日%本", 0);
	_g_assert_strv_equal (strv, "%日", "%本", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日-%本", 0);
	_g_assert_strv_equal (strv, "%日", "-", "%本", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日-%本-", 0);
	_g_assert_strv_equal (strv, "%日", "-", "%本", "-", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("-%日-%本", 0);
	_g_assert_strv_equal (strv, "-", "%日", "-", "%本", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }日本語", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", "日本語", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }%本", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", "%本", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }%正{ 體字 }", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", "%正{ 體字 }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }%正{ 體字 }{ 本語 }", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", "%正{ 體字 }{ 本語 }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }-%正{ 體字 }{ 本語 }", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", "-", "%正{ 體字 }{ 本語 }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%日{ 本語 }-%日{ 本 }{ 語 }-", 0);
	_g_assert_strv_equal (strv, "%日{ 本語 }", "-", "%日{ 本 }{ 語 }", "-", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ %A{ Prompt: } }", 0);
	_g_assert_strv_equal (strv, "%Q{ %A{ Prompt: } }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ { %A } }", 0);
	_g_assert_strv_equal (strv, "%Q{ { %A } }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ {} }", 0);
	_g_assert_strv_equal (strv, "%Q{ {} }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ { }", 0);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ %A{} }", 0);
	_g_assert_strv_equal (strv, "%Q{ %A{} }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ %A{}{} }", 0);
	_g_assert_strv_equal (strv, "%Q{ %A{}{} }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%Q{ %A{ Prompt: }{ 12 } }", 0);
	_g_assert_strv_equal (strv, "%Q{ %A{ Prompt: }{ 12 } }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%a{ %x }%b{ %y }", 0);
	_g_assert_strv_equal (strv, "%a{ %x }", "%b{ %y }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%a{ %x }%b{ %y }%c{ %z }", 0);
	_g_assert_strv_equal (strv, "%a{ %x }", "%b{ %y }", "%c{ %z }", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%a{%x}%b{%y}%c{%z}", 0);
	_g_assert_strv_equal (strv, "%a{%x}", "%b{%y}", "%c{%z}", NULL);
	g_strfreev (strv);

	strv = _g_template_tokenize ("%a{%x} %b{%y} %c{%z}", 0);
	_g_assert_strv_equal (strv, "%a{%x}", " ", "%b{%y}", " ", "%c{%z}", NULL);
	g_strfreev (strv);
}


static void
test_g_template_get_token_code_all (void)
{
	g_assert_true (_g_template_get_token_code (NULL) == 0);
	g_assert_true (_g_template_get_token_code ("") == 0);
	g_assert_true (_g_template_get_token_code ("a") == 0);
	g_assert_true (_g_template_get_token_code ("日") == 0);
	g_assert_true (_g_template_get_token_code ("%a") == 'a');
	g_assert_true (_g_template_get_token_code ("% a") == 0);
	g_assert_true (_g_template_get_token_code ("%日") == g_utf8_get_char ("日"));
	g_assert_true (_g_template_get_token_code ("%日 %字") == g_utf8_get_char ("日"));
	g_assert_true (_g_template_get_token_code ("% 日") == 0);
	g_assert_true (_g_template_get_token_code (" %日") == 0);
	g_assert_true (_g_template_get_token_code (" % 日") == 0);
	g_assert_true (_g_template_get_token_code ("%{}") == 0);
	g_assert_true (_g_template_get_token_code ("%{ text }") == 0);
	g_assert_true (_g_template_get_token_code ("%{ %字 }") == 0);
	g_assert_true (_g_template_get_token_code ("%日{}") == g_utf8_get_char ("日"));
	g_assert_true (_g_template_get_token_code ("%日{%字}") == g_utf8_get_char ("日"));
	g_assert_true (_g_template_get_token_code ("%日{ }") == g_utf8_get_char ("日"));
	g_assert_true (_g_template_get_token_code ("%日{ %字 }") == g_utf8_get_char ("日"));
}


static void
test_g_template_get_token_args_all (void)
{
	char **strv;

	strv = _g_template_get_token_args ("");
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%F");
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("{ xxx }{ yyy }");
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%A%F{ xxx }{ yyy }");
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%F{ xxx }{ yyy }");
	_g_assert_strv_equal (strv, "xxx", "yyy", NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%日{ 本語 }");
	_g_assert_strv_equal (strv, "本語", NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%日{ 本語 }{ 體字 }");
	_g_assert_strv_equal (strv, "本語", "體字", NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%Q{ %A }");
	_g_assert_strv_equal (strv, "%A", NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%Q{ %A{ Prompt: } }");
	_g_assert_strv_equal (strv, "%A{ Prompt: }", NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%Q{ %A{ Prompt: }{ 10 } }");
	_g_assert_strv_equal (strv, "%A{ Prompt: }{ 10 }", NULL);
	g_strfreev (strv);

	strv = _g_template_get_token_args ("%Q{ %A{ Prompt: }{ 10 } %A{ Prompt2: }{ 20 } }");
	_g_assert_strv_equal (strv, "%A{ Prompt: }{ 10 } %A{ Prompt2: }{ 20 }", NULL);
	g_strfreev (strv);
}


static void
test_g_template_token_is_all (void)
{
	g_assert_true (_g_template_token_is (NULL, 0));
	g_assert_true (_g_template_token_is ("", 0));

	g_assert_true (_g_template_token_is ("a", 0));
	g_assert_true (! _g_template_token_is ("a", 'a'));
	g_assert_true (! _g_template_token_is ("a", 'b'));

	g_assert_true (! _g_template_token_is ("%a", 0));
	g_assert_true (_g_template_token_is ("%a", 'a'));
	g_assert_true (_g_template_token_is ("%abc", 'a'));
	g_assert_true (_g_template_token_is ("%a{}", 'a'));
	g_assert_true (_g_template_token_is ("%a{ xyz }", 'a'));

	g_assert_true (_g_template_token_is ("%日", g_utf8_get_char ("日")));
	g_assert_true (_g_template_token_is ("%日{}", g_utf8_get_char ("日")));
	g_assert_true (_g_template_token_is ("%日{ 日本語 }", g_utf8_get_char ("日")));
}


static gboolean
eval_template_cb (TemplateFlags   flags,
		  gunichar        parent_code,
		  gunichar        code,
		  char          **args,
		  GString        *result,
		  gpointer        user_data)
{
	int i;

	switch (code) {
	case 'A':
		g_string_append (result, "a");
		break;
	case 'B':
		for (i = 0; args[i] != NULL; i++)
			g_string_append (result, args[i]);
		break;
	case 'C':
		break;
	case '#':
		g_string_append (result, _g_template_replace_enumerator (args[0], 1));
		break;
	default:
		g_string_append_unichar (result, code);
		break;
	}
	return FALSE;
}


static void
test_g_template_eval_all (void)
{
	g_assert_cmpstr (_g_template_eval (NULL, 0, eval_template_cb, NULL), ==, NULL);
	g_assert_cmpstr (_g_template_eval ("", 0, eval_template_cb, NULL), ==, "");
	g_assert_cmpstr (_g_template_eval ("日本語", 0, eval_template_cb, NULL), ==, "日本語");
	g_assert_cmpstr (_g_template_eval ("%A", 0, eval_template_cb, NULL), ==, "a");
	g_assert_cmpstr (_g_template_eval ("%A{}", 0, eval_template_cb, NULL), ==, "a");
	g_assert_cmpstr (_g_template_eval ("%A{ xyz }", 0, eval_template_cb, NULL), ==, "a");
	g_assert_cmpstr (_g_template_eval ("%A{ %B }", 0, eval_template_cb, NULL), ==, "a");
	g_assert_cmpstr (_g_template_eval ("%A{ %A }", 0, eval_template_cb, NULL), ==, "a");
	g_assert_cmpstr (_g_template_eval ("%B{x}", 0, eval_template_cb, NULL), ==, "x");
	g_assert_cmpstr (_g_template_eval ("%B{%B{x}}", 0, eval_template_cb, NULL), ==, "x");
	g_assert_cmpstr (_g_template_eval ("%B{%B{x}y}z", 0, eval_template_cb, NULL), ==, "xyz");
	g_assert_cmpstr (_g_template_eval ("%B{%B{x}%B{y}}", 0, eval_template_cb, NULL), ==, "xy");
	g_assert_cmpstr (_g_template_eval ("%A%B%C", 0, eval_template_cb, NULL), ==, "a");
	g_assert_cmpstr (_g_template_eval ("%A{}%B{ x }%C{ xyz }", 0, eval_template_cb, NULL), ==, "ax");
	g_assert_cmpstr (_g_template_eval ("%A%B%C日本語", 0, eval_template_cb, NULL), ==, "a日本語");
	g_assert_cmpstr (_g_template_eval ("日本語%A%B{日}%C", 0, eval_template_cb, NULL), ==, "日本語a日");

	g_assert_cmpstr (_g_template_eval ("#", 0, eval_template_cb, NULL), ==, "1");
	g_assert_cmpstr (_g_template_eval ("##", 0, eval_template_cb, NULL), ==, "01");
	g_assert_cmpstr (_g_template_eval ("日##", 0, eval_template_cb, NULL), ==, "日01");
	g_assert_cmpstr (_g_template_eval ("日#本##語###", 0, eval_template_cb, NULL), ==, "日1本01語001");
	g_assert_cmpstr (_g_template_eval ("#日##本###語", 0, eval_template_cb, NULL), ==, "1日01本001語");
	g_assert_cmpstr (_g_template_eval ("%A#%B#%C##", 0, eval_template_cb, NULL), ==, "a1101");
	g_assert_cmpstr (_g_template_eval ("日本語%A%B{日}%C###", 0, eval_template_cb, NULL), ==, "日本語a日001");

	g_assert_cmpstr (_g_template_eval ("#", TEMPLATE_FLAGS_NO_ENUMERATOR, eval_template_cb, NULL), ==, "#");
	g_assert_cmpstr (_g_template_eval ("%A#%B{#}%C##", TEMPLATE_FLAGS_NO_ENUMERATOR, eval_template_cb, NULL), ==, "a####");
}


static void
test_g_template_replace_enumerator_all (void)
{
	g_assert_cmpstr (_g_template_replace_enumerator (NULL, 1), ==, NULL);
	g_assert_cmpstr (_g_template_replace_enumerator ("", 1), ==, NULL);
	g_assert_cmpstr (_g_template_replace_enumerator ("X", 1), ==, NULL);
	g_assert_cmpstr (_g_template_replace_enumerator ("日", 1), ==, NULL);
	g_assert_cmpstr (_g_template_replace_enumerator ("日#", 1), ==, NULL);
	g_assert_cmpstr (_g_template_replace_enumerator ("#日", 1), ==, NULL);
	g_assert_cmpstr (_g_template_replace_enumerator ("#", 1), ==, "1");
	g_assert_cmpstr (_g_template_replace_enumerator ("##", 1), ==, "01");
	g_assert_cmpstr (_g_template_replace_enumerator ("###", 1), ==, "001");
	g_assert_cmpstr (_g_template_replace_enumerator ("###", 10), ==, "010");
	g_assert_cmpstr (_g_template_replace_enumerator ("###", 1000), ==, "1000");
}


static void
test_g_utf8_rstrip_all (void)
{
	g_assert_cmpstr (_g_utf8_rstrip (NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_rstrip (""), ==, "");
	g_assert_cmpstr (_g_utf8_rstrip (" "), ==, "");
	g_assert_cmpstr (_g_utf8_rstrip ("  "), ==, "");
	g_assert_cmpstr (_g_utf8_rstrip ("日"), ==, "日");
	g_assert_cmpstr (_g_utf8_rstrip (" 日"), ==, " 日");
	g_assert_cmpstr (_g_utf8_rstrip ("日 "), ==, "日");
	g_assert_cmpstr (_g_utf8_rstrip ("日  "), ==, "日");
	g_assert_cmpstr (_g_utf8_rstrip (" 日 "), ==, " 日");
	g_assert_cmpstr (_g_utf8_rstrip (" 日  "), ==, " 日");
	g_assert_cmpstr (_g_utf8_rstrip ("日 本"), ==, "日 本");
	g_assert_cmpstr (_g_utf8_rstrip (" 日 本"), ==, " 日 本");
	g_assert_cmpstr (_g_utf8_rstrip ("  日 本"), ==, "  日 本");
	g_assert_cmpstr (_g_utf8_rstrip ("日 本 "), ==, "日 本");
	g_assert_cmpstr (_g_utf8_rstrip ("日 本  "), ==, "日 本");
	g_assert_cmpstr (_g_utf8_rstrip ("日  本"), ==, "日  本");
	g_assert_cmpstr (_g_utf8_rstrip ("日  本 "), ==, "日  本");
	g_assert_cmpstr (_g_utf8_rstrip ("日  本  "), ==, "日  本");
}


static void
test_g_utf8_strip_all (void)
{
	g_assert_cmpstr (_g_utf8_strip (NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_strip (""), ==, "");
	g_assert_cmpstr (_g_utf8_strip (" "), ==, "");
	g_assert_cmpstr (_g_utf8_strip ("  "), ==, "");
	g_assert_cmpstr (_g_utf8_strip ("日"), ==, "日");
	g_assert_cmpstr (_g_utf8_strip (" 日"), ==, "日");
	g_assert_cmpstr (_g_utf8_strip ("日 "), ==, "日");
	g_assert_cmpstr (_g_utf8_strip (" 日 "), ==, "日");
	g_assert_cmpstr (_g_utf8_strip ("日 本"), ==, "日 本");
	g_assert_cmpstr (_g_utf8_strip (" 日 本"), ==, "日 本");
	g_assert_cmpstr (_g_utf8_strip ("日 本 "), ==, "日 本");
}


static void
test_g_utf8_translate_all (void)
{
	g_assert_cmpstr (_g_utf8_translate (NULL, NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_translate (NULL, "*", ".*", NULL), ==, NULL);
	g_assert_cmpstr (_g_utf8_translate ("*", "*", "", NULL), ==, "");
	g_assert_cmpstr (_g_utf8_translate ("**", "*", "", NULL), ==, "");
	g_assert_cmpstr (_g_utf8_translate ("日", "*", "", NULL), ==, "日");
	g_assert_cmpstr (_g_utf8_translate ("*日*", NULL), ==, "*日*");
	g_assert_cmpstr (_g_utf8_translate ("*日*", "*", ".*", NULL), ==, ".*日.*");
	g_assert_cmpstr (_g_utf8_translate ("*日*", "*", "", NULL), ==, "日");
	g_assert_cmpstr (_g_utf8_translate ("*日*本.語", ".", "\\.", "*", ".*", NULL), ==, ".*日.*本\\.語");
}


static void
test_g_path_get_extension_all (void)
{
	g_assert_cmpstr (_g_path_get_extension ("日本.tar"), ==, ".tar");
	g_assert_cmpstr (_g_path_get_extension ("日本.tar.xz"), ==, ".tar.xz");
	g_assert_cmpstr (_g_path_get_extension ("日本.xz"), ==, ".xz");
	g_assert_null (_g_path_get_extension ("日本"));
}


static void
test_g_path_remove_extension_all (void)
{
	g_assert_cmpstr (_g_path_remove_extension ("日本.tar"), ==, "日本");
	g_assert_cmpstr (_g_path_remove_extension ("日本.tar.xz"), ==, "日本");
	g_assert_cmpstr (_g_path_remove_extension ("日本"), ==, "日本");
}


static void
test_g_path_is_parent_all (void)
{
	g_assert_true (_g_path_is_parent ("/日", "/日"));
	g_assert_true (_g_path_is_parent ("/日", "/日/"));
	g_assert_true (_g_path_is_parent ("/日/", "/日"));

	g_assert_true (_g_path_is_parent ("/", "/日/本"));
	g_assert_true (_g_path_is_parent ("/日", "/日/本"));
	g_assert_true (_g_path_is_parent ("/日/", "/日/本"));

	g_assert_false (_g_path_is_parent ("/日/本", "/日"));
	g_assert_false (_g_path_is_parent ("/日/本", "/日/"));
	g_assert_false (_g_path_is_parent ("/日/本/語", "/日/本"));
	g_assert_false (_g_path_is_parent ("/日/本/語", "/日/本/"));

	g_assert_false (_g_path_is_parent ("/日/", "/日本"));
	g_assert_false (_g_path_is_parent ("/日", "/日本"));

	g_assert_false (_g_path_is_parent ("/本", "/日本"));
	g_assert_false (_g_path_is_parent ("/本", "/日"));
	g_assert_false (_g_path_is_parent ("/本/", "/日"));
	g_assert_false (_g_path_is_parent ("/本", "/日/"));
	g_assert_false (_g_path_is_parent ("/本/", "/日/"));
}


static void
test_g_path_join_components (const char *expected, ...)
{
	va_list      args;
	const char  *str;
	GList       *str_list;
	char       **strv;
	char        *result;

	str_list = NULL;
	va_start (args, expected);
	while ((str = va_arg (args, const char *)) != NULL)
		str_list = g_list_prepend (str_list, g_strdup (str));
	va_end (args);
	str_list = g_list_reverse (str_list);

	strv = _g_string_list_to_strv (str_list);
	result = _g_path_join_components (strv);
	g_assert_cmpstr (result, ==, expected);

	g_free (result);
	g_strfreev (strv);
	_g_string_list_free (str_list);
}


static void
test_g_path_join_components_all (void)
{
	test_g_path_join_components ("", NULL);
	test_g_path_join_components ("", "", NULL);
	test_g_path_join_components ("/", "", "", NULL);
	test_g_path_join_components ("/", "", "/", NULL);

	test_g_path_join_components ("/", "/", NULL);
	test_g_path_join_components ("/", "/", "", NULL);
	test_g_path_join_components ("/", "/", "/", NULL);

	test_g_path_join_components ("本", "本", NULL);
	test_g_path_join_components ("/本", "/本", NULL);
	test_g_path_join_components ("本/", "本/", NULL);
	test_g_path_join_components ("/本/", "/本/", NULL);

	test_g_path_join_components ("本/", "本", "", NULL);
	test_g_path_join_components ("/本/", "/本", "", NULL);
	test_g_path_join_components ("本/", "本/", "", NULL);
	test_g_path_join_components ("/本/", "/本/", "", NULL);

	test_g_path_join_components ("本/", "本", "/", NULL);
	test_g_path_join_components ("/本/", "/本", "/", NULL);
	test_g_path_join_components ("本/", "本/", "/", NULL);
	test_g_path_join_components ("/本/", "/本/", "/", NULL);

	test_g_path_join_components ("/本/", "/本", "", NULL);
	test_g_path_join_components ("/本/", "/本", "", "", NULL);
	test_g_path_join_components ("/本/", "/本", "/", NULL);
	test_g_path_join_components ("/本/", "/本", "/", "/", NULL);

	test_g_path_join_components ("/本", "/", "本", NULL);
	test_g_path_join_components ("/本", "/", "/本", NULL);
	test_g_path_join_components ("/本/", "/", "/本/", NULL);
	test_g_path_join_components ("/本", "/", "/", "本", NULL);
	test_g_path_join_components ("/本", "/", "/", "/本", NULL);
	test_g_path_join_components ("/本/", "/", "/", "/本/", NULL);
}


static void
test_g_path_split_components_all (void)
{
	char **strv;

	strv = _g_path_split_components (NULL, NULL);
	_g_assert_strv_equal (strv, NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("", NULL);
	_g_assert_strv_equal (strv, "", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/", NULL);
	_g_assert_strv_equal (strv, "/", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("//", NULL);
	_g_assert_strv_equal (strv, "/", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("///", NULL);
	_g_assert_strv_equal (strv, "/", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本", NULL);
	_g_assert_strv_equal (strv, "本", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("//本", NULL);
	_g_assert_strv_equal (strv, "本", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本/", NULL);
	_g_assert_strv_equal (strv, "本", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本//", NULL);
	_g_assert_strv_equal (strv, "本", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("//本//", NULL);
	_g_assert_strv_equal (strv, "本", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本/日", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本/日/", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("//本/日/", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本//日/", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("/本/日//", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("本", NULL);
	_g_assert_strv_equal (strv, "本", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("本日", NULL);
	_g_assert_strv_equal (strv, "本日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("本/日", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("本//日", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("本//日/", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);

	strv = _g_path_split_components ("本//日//", NULL);
	_g_assert_strv_equal (strv, "本", "日", NULL);
	g_strfreev (strv);
}


static void
test_g_path_get_relative_all (void)
{
	g_assert_cmpstr (_g_path_get_relative ("/日/本/語", "/日/諸星"), ==, "../本/語");
	g_assert_cmpstr (_g_path_get_relative ("/日/本/語", "/日"), ==, "本/語");
	g_assert_cmpstr (_g_path_get_relative ("/日/本/語", "/日/"), ==, "本/語");
	g_assert_cmpstr (_g_path_get_relative ("/日/本", "/日/本"), ==, "./");
	g_assert_cmpstr (_g_path_get_relative ("/日", "/日"), ==, "./");
	g_assert_cmpstr (_g_path_get_relative ("/", "/"), ==, "./");
	g_assert_cmpstr (_g_path_get_relative ("/日/本語", "/日/本"), ==, "../本語");
	g_assert_cmpstr (_g_path_get_relative ("/日/本", "/日/本語"), ==, "../本");
}


static void
test_g_uri_get_relative_all (void)
{
	g_assert_cmpstr (_g_uri_get_relative_path ("file:///日/本/語", "file:///日/諸星"), ==, "../本/語");
	g_assert_cmpstr (_g_uri_get_relative_path ("file:///日/本/語", "file:///日"), ==, "本/語");
	g_assert_cmpstr (_g_uri_get_relative_path ("smb:///日/本/語", "file:///日/諸星"), ==, "smb:///日/本/語");
	g_assert_cmpstr (_g_uri_get_relative_path ("file:///日/本", "file:///日/本"), ==, "./");
}


static void
test_g_uri_get_scheme_all (void)
{
	g_assert_cmpstr (_g_uri_get_scheme (NULL), ==, NULL);
	g_assert_cmpstr (_g_uri_get_scheme (""), ==, NULL);
	g_assert_cmpstr (_g_uri_get_scheme ("/"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_scheme ("file:"), ==, "file");
	g_assert_cmpstr (_g_uri_get_scheme ("file://"), ==, "file");
	g_assert_cmpstr (_g_uri_get_scheme ("file:///"), ==, "file");
	g_assert_cmpstr (_g_uri_get_scheme ("file:///日/本/語"), ==, "file");
	g_assert_cmpstr (_g_uri_get_scheme ("sftp+file:///日/本/語"), ==, "sftp+file");
}


static void
test_g_uri_get_path_all (void)
{
	g_assert_cmpstr (_g_uri_get_path (NULL), ==, NULL);
	g_assert_cmpstr (_g_uri_get_path (""), ==, NULL);
	g_assert_cmpstr (_g_uri_get_path ("file:"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_path ("file:/"), ==, "/");
	g_assert_cmpstr (_g_uri_get_path ("file://"), ==, NULL);
	g_assert_cmpstr (_g_uri_get_path ("file:///"), ==, "/");
	g_assert_cmpstr (_g_uri_get_path ("file:///%E6%97%A5"), ==, "/日");
	g_assert_cmpstr (_g_uri_get_path ("file:///%E6%97%A5/"), ==, "/日/");
	g_assert_cmpstr (_g_uri_get_path ("file:///%E6%97%A5/%E6%9C%AC/%E8%AA%9E"), ==, "/日/本/語");
}


static void
test_g_uri_is_parent_all (void)
{
	g_assert_false (_g_uri_is_parent (NULL, NULL));
	g_assert_false (_g_uri_is_parent ("", ""));
	g_assert_false (_g_uri_is_parent ("file:", "file:"));
	g_assert_true (_g_uri_is_parent ("file:/日", "file:/日/本"));
	g_assert_true (_g_uri_is_parent ("file:///日", "file:///日/本"));
	g_assert_true (_g_uri_is_parent ("file:///日/本", "file:///日/本"));
	g_assert_true (_g_uri_is_parent ("file:///日/本/", "file:///日/本"));
	g_assert_true (_g_uri_is_parent ("file:///日/本", "file:///日/本/"));
	g_assert_true (_g_uri_is_parent ("file:///日/本/", "file:///日/本/"));
	g_assert_false (_g_uri_is_parent ("file:/日", "smb:/日/本"));
}


static void
test_g_uri_remove_extension_all (void)
{
	g_assert_cmpstr (_g_uri_remove_extension ("file:///日本"), ==, "file:///日本");
	g_assert_cmpstr (_g_uri_remove_extension ("file:///日本.tar"), ==, "file:///日本");
	g_assert_cmpstr (_g_uri_remove_extension ("file:///日本.tar.xz"), ==, "file:///日本");
	g_assert_cmpstr (_g_uri_remove_extension ("file://諸:星@日:123/日本.tar.xz"), ==, "file://諸:星@日:123/日本");
}


int
main (int   argc,
      char *argv[])
{
	setlocale (LC_ALL, "");
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/glib-utils/_g_utf8_after_ascii_space", test_g_utf8_after_ascii_space);
	g_test_add_func ("/glib-utils/_g_utf8_all_spaces", test_g_utf8_all_spaces_space);
	g_test_add_func ("/glib-utils/_g_utf8_escape_xml", test_g_utf8_escape_xml_space);
	g_test_add_func ("/glib-utils/_g_utf8_find_str", test_g_utf8_find_str);
	g_test_add_func ("/glib-utils/_g_utf8_has_prefix", test_g_utf8_has_prefix);
	g_test_add_func ("/glib-utils/_g_utf8_last_char", test_g_utf8_last_char_all);
	g_test_add_func ("/glib-utils/_g_utf8_n_equal", test_g_utf8_n_equal_all);
	g_test_add_func ("/glib-utils/_g_utf8_replace_str", test_g_utf8_replace_str_all);
	g_test_add_func ("/glib-utils/_g_utf8_rstrip", test_g_utf8_rstrip_all);
	g_test_add_func ("/glib-utils/_g_utf8_split", test_g_utf8_split_all);
	g_test_add_func ("/glib-utils/_g_template_tokenize", test_g_template_tokenize_all);
	g_test_add_func ("/glib-utils/_g_template_get_token_code", test_g_template_get_token_code_all);
	g_test_add_func ("/glib-utils/_g_template_get_token_args", test_g_template_get_token_args_all);
	g_test_add_func ("/glib-utils/_g_template_token_is", test_g_template_token_is_all);
	g_test_add_func ("/glib-utils/_g_template_eval", test_g_template_eval_all);
	g_test_add_func ("/glib-utils/_g_template_replace_enumerator", test_g_template_replace_enumerator_all);
	g_test_add_func ("/glib-utils/_g_utf8_strip", test_g_utf8_strip_all);
	g_test_add_func ("/glib-utils/_g_utf8_translate", test_g_utf8_translate_all);
	g_test_add_func ("/glib-utils/_g_utf8_remove_string_properties", test_g_utf8_remove_string_properties);
	g_test_add_func ("/glib-utils/_g_utf8_find_expr", test_g_utf8_find_expr);

	g_test_add_func ("/glib-utils/_g_path_get_basename", test_g_path_get_basename_all);
	g_test_add_func ("/glib-utils/_g_path_get_extension", test_g_path_get_extension_all);
	g_test_add_func ("/glib-utils/_g_path_get_parent", test_g_path_get_parent_all);
	g_test_add_func ("/glib-utils/_g_path_get_relative", test_g_path_get_relative_all);
	g_test_add_func ("/glib-utils/_g_path_is_parent", test_g_path_is_parent_all);
	g_test_add_func ("/glib-utils/_g_path_join_components", test_g_path_join_components_all);
	g_test_add_func ("/glib-utils/_g_path_remove_extension", test_g_path_remove_extension_all);
	g_test_add_func ("/glib-utils/_g_path_split_components", test_g_path_split_components_all);

	g_test_add_func ("/glib-utils/_g_uri_append_path", test_g_uri_append_path_all);
	g_test_add_func ("/glib-utils/_g_uri_from_path", test_g_uri_from_path_all);
	g_test_add_func ("/glib-utils/_g_uri_get_basename", test_g_uri_get_basename_all);
	g_test_add_func ("/glib-utils/_g_uri_get_parent", test_g_uri_get_parent_all);
	g_test_add_func ("/glib-utils/_g_uri_get_relative_path", test_g_uri_get_relative_all);
	g_test_add_func ("/glib-utils/_g_uri_get_scheme", test_g_uri_get_scheme_all);
	g_test_add_func ("/glib-utils/_g_uri_get_path", test_g_uri_get_path_all);
	g_test_add_func ("/glib-utils/_g_uri_is_parent", test_g_uri_is_parent_all);
	g_test_add_func ("/glib-utils/_g_uri_remove_extension", test_g_uri_remove_extension_all);

	g_test_add_func ("/glib-utils/_g_file_get_display_name", test_g_file_get_display_name_all);
	g_test_add_func ("/glib-utils/_g_rand_string", test_g_rand_string);
	g_test_add_func ("/glib-utils/regex", test_regexp);

	return g_test_run ();
}
