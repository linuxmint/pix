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
#include <glib.h>
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "str-utils.h"


gboolean
_g_str_equal (const char *str1,
	      const char *str2)
{
	return g_strcmp0 (str1, str2) == 0;
}


gboolean
_g_str_n_equal (const char *str1,
		const char *str2,
		gsize       size)
{
	if ((str1 == NULL) && (str2 == NULL))
		return TRUE;
	if ((str1 == NULL) || (str2 == NULL))
		return FALSE;
	return strncmp (str1, str2, size);
}


gboolean
_g_str_empty (const char *str)
{
	return (str == NULL) || (str[0] == 0);
}


void
_g_str_set (char       **str,
	    const char  *value)
{
	if (*str == value)
		return;

	if (*str != NULL) {
		g_free (*str);
		*str = NULL;
	}

	if (value != NULL)
		*str = g_strdup (value);
}


char **
_g_strv_take_from_str_list (GList *str_list,
			    int    size)
{
	char  **str_v;
	GList  *scan;
	int     i;

	if (size < 0)
		size = g_list_length (str_list);

	str_v = g_new0 (char *, size + 1);
	for (scan = g_list_last (str_list), i = 0; scan && (i < size); scan = scan->prev, i++)
		str_v[i] = (char *) scan->data;
	str_v[i] = NULL;

	return str_v;
}


char *
_g_str_random (int len)
{
	static char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	static int   letters_only = 52;
	static int   whole_alphabet = 62;

	GRand       *generator;
	char        *s;
	int          i;

	generator = g_rand_new ();

	s = g_new (char, len + 1);
	for (i = 0; i < len; i++)
		s[i] = alphabet[g_rand_int_range (generator, 0, (i == 0) ? letters_only : whole_alphabet)];
	s[len] = 0;

	g_rand_free (generator);

	return s;
}


char *
_g_str_remove_suffix (const char *str,
		      const char *suffix)
{
	int s_len;
	int suffix_len;

	if (str == NULL)
		return NULL;

	if (suffix == NULL)
		return g_strdup (str);

	s_len = strlen (str);
	suffix_len = strlen (suffix);

	if (suffix_len >= s_len)
		return g_strdup ("");
	else
		return g_strndup (str, s_len - suffix_len);
}


GHashTable    *static_strings = NULL;
static GMutex  static_strings_mutex;


const char *
_g_str_get_static (const char *str)
{
	const char *result;

	if (str == NULL)
		return NULL;

	g_mutex_lock (&static_strings_mutex);

	if (static_strings == NULL)
		static_strings = g_hash_table_new_full (g_str_hash,
							g_str_equal,
							g_free,
							NULL);

	if (! g_hash_table_lookup_extended (static_strings,
					    str,
					    (gpointer) &result,
					    NULL))
	{
		result = g_strdup (str);
		g_hash_table_insert (static_strings,
				     (gpointer) result,
				     GINT_TO_POINTER (1));
	}

	g_mutex_unlock (&static_strings_mutex);

	return result;
}


GHashTable *
_g_str_split_as_hash_table (const char *str)
{
	GHashTable *htable;

	htable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	if (str != NULL) {
		char **strv;
		int    i;

		strv = g_strsplit (str, ",", -1);
		for (i = 0; strv[i] != NULL; i++)
			if (strv[i] != 0)
				g_hash_table_add (htable, g_strdup (strv[i]));

		g_strfreev (strv);
	}

	return htable;
}


/* StrV utils */


int
_g_strv_find (char       **strv,
	      const char  *str)
{
	int i;

	for (i = 0; strv[i] != NULL; i++) {
		if (strcmp (strv[i], str) == 0)
			return i;
	}

	return -1;
}


gboolean
_g_strv_contains (char       **strv,
		  const char  *str)
{
	return (_g_strv_find (strv, str) >= 0);
}


char **
_g_strv_prepend (char       **strv,
		 const char  *str)
{
	char **result;
	int    i;
	int    j;

	result = g_new (char *, g_strv_length (strv) + 1);
	i = 0;
	result[i++] = g_strdup (str);
	for (j = 0; strv[j] != NULL; j++)
		result[i++] = g_strdup (strv[j]);
	result[i] = NULL;

	return result;
}


char **
_g_strv_concat (char **strv1,
		char **strv2)
{
	char **result;
	int    i, j;

	result = g_new (char *, g_strv_length (strv1) + g_strv_length (strv2) + 1);
	i = 0;
	for (j = 0; strv1[j] != NULL; j++)
		result[i++] = g_strdup (strv1[j]);
	for (j = 0; strv2[j] != NULL; j++)
		result[i++] = g_strdup (strv2[j]);
	result[i] = NULL;

	return result;
}


gboolean
_g_strv_remove (char       **strv,
		const char  *str)
{
	int i;
	int j;

	if (str == NULL)
		return FALSE;

	for (i = 0; strv[i] != NULL; i++)
		if (strcmp (strv[i], str) == 0)
			break;

	if (strv[i] == NULL)
		return FALSE;

	for (j = i; strv[j] != NULL; j++)
		strv[j] = strv[j + 1];

	return TRUE;
}


/* UTF-8 utils */


char *
_g_utf8_strndup (const char *str,
		 gssize      size)
{
	char *new_str;

	if ((str == NULL) || (size == 0))
		return NULL;

	if (size < 0)
		size = g_utf8_strlen (str, -1);

	new_str = g_new (char, size * 4 + 1);
	g_utf8_strncpy (new_str, str, size);

	return new_str;
}


const char *
_g_utf8_find_str (const char *haystack,
		  const char *needle)
{
	glong haystack_len;
	glong needle_len;
	glong needle_size;
	glong i;

	if ((haystack == NULL) || (needle == NULL))
		return NULL;

	haystack_len = g_utf8_strlen (haystack, -1);
	needle_len = g_utf8_strlen (needle, -1);
	needle_size = strlen (needle);

	if (needle_len == 0)
		return NULL;

	for (i = 0; i <= haystack_len - needle_len; i++) {
		if (strncmp (haystack, needle, needle_size) == 0)
			return haystack;
		haystack = g_utf8_next_char (haystack);
	}

	return NULL;
}


char *
_g_utf8_find_expr (const char *str,
		   const char *pattern)
{
	GRegex     *regex;
	GMatchInfo *match_info;
	char       *value = NULL;

	if ((str == NULL) || _g_str_empty (pattern))
		return NULL;

	regex = g_regex_new (pattern, 0, 0, NULL);
	g_regex_match (regex, str, 0, &match_info);
	if (g_match_info_matches (match_info))
		value = g_match_info_fetch (match_info, 0);

	g_match_info_free (match_info);
	g_regex_unref (regex);

	return value;
}


/* -- _g_utf8_split -- */


char **
_g_utf8_split (const char *str,
	       const char *sep,
	       int         max_tokens)
{
	glong        sep_size;
	GList       *chunk_list;
	int          chunk_n;
	const char  *p;
	char       **chunk_v;

	sep_size = (sep != NULL) ? strlen (sep) : 0;
	chunk_list = NULL;
	chunk_n = 0;
	p = str;
	while ((p != NULL) && (max_tokens != 0) && (max_tokens != 1)) {
		const char *sep_p = _g_utf8_find_str (p, sep);
		char       *chunk;

		if (sep_p == NULL) {
			if ((p == str) && (*p == 0)) {
				/* Special case: when splitting an emtpy string
				 * return an emtpy string. */

				chunk = g_strdup ("");
				chunk_list = g_list_prepend (chunk_list, chunk);
				chunk_n++;
				if (max_tokens > 0) max_tokens--;
				p = NULL;
			}
			else if ((sep != NULL) && (sep_size == 0)) {

				/* Special case: when the separator is an
				 * empty string, split each character. */

				chunk = _g_utf8_strndup (p, 1);
				chunk_list = g_list_prepend (chunk_list, chunk);
				chunk_n++;
				if (max_tokens > 0) max_tokens--;
				p = g_utf8_next_char (p);
			}
			else {
				chunk = g_strdup (p);
				chunk_list = g_list_prepend (chunk_list, chunk);
				chunk_n++;
				if (max_tokens > 0) max_tokens--;
				p = NULL;
			}
		}
		else if (sep_p > p) {
			chunk = g_strndup (p, sep_p - p);
			chunk_list = g_list_prepend (chunk_list, chunk);
			chunk_n++;
			if (max_tokens > 0) max_tokens--;
			p = sep_p + sep_size;
		}
		else
			p = sep_p + sep_size;

		if ((p != NULL) && (g_utf8_get_char (p) == 0))
			break;
	}

	if ((p != NULL) && (max_tokens == 1)) {
		chunk_list = g_list_prepend (chunk_list, g_strdup (p));
		chunk_n++;
	}

	chunk_v = _g_strv_take_from_str_list (chunk_list, chunk_n);
	g_list_free (chunk_list);

	return chunk_v;
}


char *
_g_utf8_replace_str (const char *str,
		     const char *old_str,
		     const char *new_str)
{
	GString     *result;
	size_t       old_str_size;
	const char  *p;

	if (str == NULL)
		return NULL;

	result = g_string_new ("");
	old_str_size = (old_str != NULL) ? strlen (old_str) : 0;
	p = str;
	while ((p != NULL) && (g_utf8_get_char (p) != 0)) {
		const char *sep = _g_utf8_find_str (p, old_str);

		if (sep == NULL) {
			g_string_append (result, p);
			p = NULL;
		}
		else {
			g_string_append_len (result, p, sep - p);
			if (new_str != NULL)
				g_string_append (result, new_str);
			p = sep + old_str_size;
		}
	}

	return g_string_free (result, FALSE);
}


char *
_g_utf8_replace_pattern (const char *str,
			 const char *pattern,
			 const char *replacement)
{
	GRegex *regex;
	char   *result;

	if (str == NULL)
		return NULL;

	regex = g_regex_new (pattern, 0, 0, NULL);
	if (regex == NULL)
		return NULL;

	result = g_regex_replace_literal (regex, str, -1, 0, replacement, 0, NULL);

	g_regex_unref (regex);

	return result;
}


char *
_g_utf8_last_char (const char *str,
		   glong      *p_size)
{
	glong len;

	if (str == NULL) {
		if (p_size) *p_size = 0;
		return NULL;
	}

	len = strlen (str);
	if (p_size) *p_size = len;

	if (len == 0)
		return NULL;

	return g_utf8_find_prev_char (str, str + len);
}


gboolean
_g_utf8_n_equal (const char  *str1,
		 const char  *str2,
		 glong        size)
{
	const char *p1;
	const char *p2;

	p1 = str1;
	p2 = str2;
	while ((size > 0) && (p1 != NULL) && (p2 != NULL)) {
		gunichar c1 = g_utf8_get_char (p1);
		gunichar c2 = g_utf8_get_char (p2);

		if ((c1 == 0) || (c2 == 0) || (c1 != c2))
			break;

		size--;
		p1 = g_utf8_next_char (p1);
		p2 = g_utf8_next_char (p2);
	}

	return size == 0;
}


const char *
_g_utf8_after_ascii_space (const char *str)
{
	while (str != NULL) {
		gunichar c = g_utf8_get_char (str);

		if (c == 0)
			break;

		if (c == ' ')
			return g_utf8_next_char (str);

		str = g_utf8_next_char (str);
	}

	return NULL;
}


gboolean
_g_utf8_has_prefix (const char *str,
		    const char *prefix)
{
	if (str == NULL)
		return FALSE;

	if (prefix == NULL)
		return FALSE;

	while ((str != NULL) && (prefix != NULL)) {
		gunichar str_ch = g_utf8_get_char (str);
		gunichar prefix_ch = g_utf8_get_char (prefix);

		if (prefix_ch == 0)
			return TRUE;

		if (str_ch == 0)
			return FALSE;

		if (str_ch != prefix_ch)
			return FALSE;

		str = g_utf8_next_char (str);
		prefix = g_utf8_next_char (prefix);
	}

	return FALSE;
}


gboolean
_g_utf8_all_spaces (const char *str)
{
	while (str != NULL) {
		gunichar ch = g_utf8_get_char (str);

		if (ch == 0)
			break;

		if (! g_unichar_isspace (ch))
			return FALSE;

		str = g_utf8_next_char (str);
	}

	return TRUE;
}


char *
_g_utf8_try_from_any (const char *str)
{
	char *utf8_str;

	if (str == NULL)
		return NULL;

	if (! g_utf8_validate (str, -1, NULL))
		utf8_str = g_locale_to_utf8 (str, -1, NULL, NULL, NULL);
	else
		utf8_str = g_strdup (str);

	return utf8_str;
}


char *
_g_utf8_from_any (const char *str)
{
	char *utf8_str;

	if (str == NULL)
		return NULL;

	utf8_str = _g_utf8_try_from_any (str);
	if (utf8_str == NULL)
		utf8_str = g_strdup (_("(invalid value)"));

	return utf8_str;
}


/* -- _g_utf8_strip_func -- */


typedef enum {
	STRIP_STATE_HEADING_SPACE,
	STRIP_STATE_REST
} StripState;


char *
_g_utf8_strip_func (const char *str,
		    StripFunc   is_space_func)
{
	const char *first_non_space = NULL;
	const char *last_non_space = NULL;
	StripState  state = STRIP_STATE_HEADING_SPACE;

	if (str == NULL)
		return NULL;

	if (is_space_func == NULL)
		return g_strdup ("");

	while (str != NULL) {
		gunichar ch = g_utf8_get_char (str);
		gboolean is_space = is_space_func (ch) || (ch == 0);

		switch (state) {
		case STRIP_STATE_HEADING_SPACE:
			if (! is_space) {
				state = STRIP_STATE_REST;
				first_non_space = last_non_space = str;
			}
			break;

		case STRIP_STATE_REST:
			if (! is_space)
				last_non_space = str;
			break;
		}

		if (ch == 0)
			break;

		str = g_utf8_next_char (str);
	}

	if (first_non_space == NULL)
		return g_strdup ("");

	g_assert (last_non_space != NULL);

	return g_strndup (first_non_space, g_utf8_next_char (last_non_space) - first_non_space);
}


char *
_g_utf8_strip (const char *str)
{
	return _g_utf8_strip_func (str, g_unichar_isspace);
}


/* -- _g_utf8_rstrip_func -- */


typedef enum {
	RSTRIP_STATE_ONLY_SPACES,	/* String contains only spaces. */
	RSTRIP_STATE_NON_SPACE,		/* Reading non space characters. */
	RSTRIP_STATE_TRAILING_SPACE	/* Reading possible trailing spaces. */
} RStripState;


char *
_g_utf8_rstrip_func (const char	 *str,
		     StripFunc	  is_space_func)
{
	const char  *first_trail_space;
	RStripState  state;
	const char  *p;

	if (str == NULL)
		return NULL;

	if (is_space_func == NULL)
		return g_strdup (str);

	first_trail_space = NULL;
	state = RSTRIP_STATE_ONLY_SPACES;
	p = str;
	while (p != NULL) {
		gunichar ch = g_utf8_get_char (p);
		gboolean is_space = is_space_func (ch);

		switch (state) {
		case RSTRIP_STATE_ONLY_SPACES:
			if (! is_space && (ch != 0))
				state = RSTRIP_STATE_NON_SPACE;
			break;

		case RSTRIP_STATE_NON_SPACE:
			if (is_space) {
				state = RSTRIP_STATE_TRAILING_SPACE;
				first_trail_space = p;
			}
			break;

		case RSTRIP_STATE_TRAILING_SPACE:
			if (! is_space && (ch != 0)) {
				state = RSTRIP_STATE_NON_SPACE;
				first_trail_space = NULL;
			}
			break;
		}

		if (ch == 0)
			break;

		p = g_utf8_next_char (p);
	}

	if (state == RSTRIP_STATE_ONLY_SPACES)
		return g_strdup ("");

	if (state == RSTRIP_STATE_NON_SPACE)
		return g_strdup (str);

	g_assert (first_trail_space != NULL);

	return g_strndup (str, first_trail_space - str);
}


char *
_g_utf8_rstrip (const char *str)
{
	return _g_utf8_rstrip_func (str, g_unichar_isspace);
}


/* -- _g_utf8_translate -- */


static gboolean
_g_unichar_equal (gconstpointer v1,
		  gconstpointer v2)
{
	return *((const gunichar*) v1) == *((const gunichar*) v2);
}


static guint
_g_unichar_hash (gconstpointer v)
{
	return (guint) *(const gunichar*) v;
}


/* Substitute each occurrence of a character with a string. */
char *
_g_utf8_translate (const char *str,
		   ...)
{
	va_list     args;
	GHashTable *translation;
	const char *arg;
	GString    *regexp;

	if (str == NULL)
		return NULL;

	translation = g_hash_table_new_full (_g_unichar_hash, _g_unichar_equal, NULL, g_free);
	va_start (args, str);
	while ((arg = va_arg (args, const char *)) != NULL) {
		gunichar    from_ch;
		const char *to_str;

		from_ch = g_utf8_get_char (arg);
		to_str = va_arg (args, const char *);
		if (to_str == NULL)
			break;

		g_hash_table_insert (translation, &from_ch, g_strdup (to_str));
	}
	va_end (args);

	if (g_hash_table_size (translation) == 0) {
		g_hash_table_unref (translation);
		return g_strdup (str);
	}

	regexp = g_string_new ("");
	while (str != NULL) {
		gunichar  ch = g_utf8_get_char (str);
		char     *replacement;

		if (ch == 0)
			break;

		replacement = g_hash_table_lookup (translation, &ch);
		if (replacement != NULL)
			g_string_append (regexp, replacement);
		else
			g_string_append_unichar (regexp, ch);

		str = g_utf8_next_char (str);
	}

	g_hash_table_unref (translation);

	return g_string_free (regexp, FALSE);
}


/* -- _g_utf8_text_escape_xml -- */


typedef enum {
	XML_ESCAPE_DEFAULT	= 1 << 0,
	XML_ESCAPE_TEXT		= 1 << 1
} XmlEscFlags;


static char *
_g_utf8_escape_xml_flags (const char  *text,
			  XmlEscFlags  flags)
{
	GString  *str;
	gboolean  for_text;

	if (text == NULL)
		return NULL;

	str = g_string_sized_new (strlen (text));
	for_text = (flags & XML_ESCAPE_TEXT) != 0;

	while (text != NULL) {
		gunichar ch = g_utf8_get_char (text);

		if (ch == 0)
			break;

		switch (ch) {
		case '&':
			g_string_append (str, "&amp;");
			break;

		case '<':
			g_string_append (str, "&lt;");
			break;

		case '>':
			g_string_append (str, "&gt;");
			break;

		case '\'':
			g_string_append (str, "&apos;");
			break;

		case '"':
			g_string_append (str, "&quot;");
			break;

		default:
			if (for_text && (ch == '\n'))
				g_string_append (str, "<br>");
			else if ((ch > 127) || ! g_ascii_isprint ((char) ch))
				g_string_append_printf (str, "&#%d;", ch);
			else
				g_string_append_unichar (str, ch);
			break;
		}

		text = g_utf8_next_char (text);
	}

	return g_string_free (str, FALSE);
}


char *
_g_utf8_escape_xml (const char *str)
{
	return _g_utf8_escape_xml_flags (str, XML_ESCAPE_DEFAULT);
}


char *
_g_utf8_text_escape_xml	(const char *str)
{
	return _g_utf8_escape_xml_flags (str, XML_ESCAPE_TEXT);
}


char *
_g_utf8_remove_string_properties (const char *str)
{
	const char *known_properties[] = { "lang=", "charset=" };
	const char *after_properties = str;
	int         i;

	for (i = 0; i < G_N_ELEMENTS (known_properties); /* void */) {
		if (_g_utf8_has_prefix (after_properties, known_properties[i])) {
			after_properties = _g_utf8_after_ascii_space (after_properties);
			i = 0;
		}
		else
			i++;
	}

	return (after_properties != NULL) ? g_strdup (after_properties) : NULL;
}


/* -- Template utils -- */


#undef DEBUG_TOKENIZER


typedef enum {
	TOKENIZER_STATE_START,
	TOKENIZER_STATE_ENUMERATOR,
	TOKENIZER_STATE_CODE,
	TOKENIZER_STATE_ARG,
	TOKENIZER_STATE_TEXT
} TokenizerState;


#if DEBUG_TOKENIZER
static char *TokenizerStateName[] = {
	"TOKENIZER_STATE_START",
	"TOKENIZER_STATE_ENUMERATOR",
	"TOKENIZER_STATE_CODE",
	"TOKENIZER_STATE_ARG",
	"TOKENIZER_STATE_TEXT"
};
#endif


static gboolean
_g_template_valid_special_code (gunichar ch)
{
	return (ch != '%') && (ch != '{') && ! g_unichar_isspace (ch) && (ch != 0);
}


char **
_g_template_tokenize (const char    *template,
		      TemplateFlags  flags)
{
	gboolean         split_enumerator;
	TokenizerState   state;
	GList           *token_list;
	int              token_n;
	const char      *p;
	const char      *token_start;
	glong            token_size;
	int              n_open_brackets;
	char           **token_v;

#if DEBUG_TOKENIZER
	g_print ("\n> _g_template_tokenize: '%s'\n", template);
#endif

	split_enumerator = (flags & TEMPLATE_FLAGS_NO_ENUMERATOR) == 0;

	state = TOKENIZER_STATE_START;
	token_list = NULL;
	token_n = 0;
	token_start = NULL;
	token_size = 0;
	n_open_brackets = 0;
	p = template;
	while (p != NULL) {
		gboolean    add_to_token;
		gboolean    save_token;
		gunichar    ch;
		const char *next_p;
		gunichar    next_ch;
		gboolean    code_prefix;
		gboolean    enumerator_prefix;

		add_to_token = FALSE;
		save_token = FALSE;
		ch = (p != NULL) ? g_utf8_get_char (p) : 0;
		next_p = (p != NULL) ? g_utf8_next_char (p) : NULL;
		next_ch = (next_p != NULL) ? g_utf8_get_char (next_p) : 0;
		code_prefix = (ch == '%') && _g_template_valid_special_code (next_ch);
		enumerator_prefix = split_enumerator && (ch == '#');

#if DEBUG_TOKENIZER
		{
			char str[7] = { 0, 0, 0, 0, 0, 0, 0 };
			g_unichar_to_utf8 (ch, str);
			g_print ("> ch: '%s'\n", str);
		}
#endif

		switch (state) {
		case TOKENIZER_STATE_START:
			token_start = p;
			token_size = 0;
			add_to_token = TRUE;
			if (enumerator_prefix)
				state = TOKENIZER_STATE_ENUMERATOR;
			else if (code_prefix)
				state = TOKENIZER_STATE_CODE;
			else
				state = TOKENIZER_STATE_TEXT;
			break;

		case TOKENIZER_STATE_ENUMERATOR:
			if (ch != '#') {
				add_to_token = FALSE;
				save_token = TRUE;
				state = TOKENIZER_STATE_START;
			}
			else
				add_to_token = TRUE;
			break;

		case TOKENIZER_STATE_CODE:
			if (next_ch == '{') {
				add_to_token = TRUE;
				state = TOKENIZER_STATE_ARG;
				n_open_brackets = 0;
			}
			else {
				add_to_token = TRUE;
				save_token = TRUE;
				state = TOKENIZER_STATE_START;
			}
			break;

		case TOKENIZER_STATE_ARG:
			if (ch == '{')
				n_open_brackets++;
			else if (ch == '}')
				n_open_brackets--;
			if ((n_open_brackets == 0) && (ch == '}') && (next_ch != '{')) {
				add_to_token = TRUE;
				save_token = TRUE;
				state = TOKENIZER_STATE_START;
			}
			else
				add_to_token = TRUE;
			break;

		case TOKENIZER_STATE_TEXT:
			if ((ch == 0) || enumerator_prefix || code_prefix) {
				add_to_token = FALSE;
				save_token = TRUE;
				state = TOKENIZER_STATE_START;
			}
			else
				add_to_token = TRUE;
			break;
		}

#if DEBUG_TOKENIZER
		g_print ("  new state: %s\n", TokenizerStateName[state]);
		g_print ("  save_token: %d\n", save_token);
		g_print ("  add_to_token: %d\n", add_to_token);
		g_print ("  token_size: %ld\n", token_size);
		g_print ("  n_open_brackets: %d\n", n_open_brackets);
#endif

		if (add_to_token) {
			token_size++;
			p = next_p;
		}

		if (save_token) {
			token_list = g_list_prepend (token_list, _g_utf8_strndup (token_start, token_size));
			token_n++;
		}

		if (ch == 0)
			break;
	}

	token_v = _g_strv_take_from_str_list (token_list, token_n);
	g_list_free (token_list);

	return token_v;
}


gunichar
_g_template_get_token_code (const char *token)
{
	gunichar ch;

	if (token == NULL)
		return 0;

	ch = g_utf8_get_char (token);
	if (ch != '%')
		return 0;

	token = g_utf8_next_char (token);
	if (token == NULL)
		return 0;

	ch = g_utf8_get_char (token);

	return _g_template_valid_special_code (ch) ? ch : 0;
}


char **
_g_template_get_token_args (const char *token)
{
	TokenizerState   state;
	GList           *token_list;
	int              token_n;
	const char      *p;
	const char      *token_start;
	glong            token_size;
	int              n_open_brackets;
	char           **token_v;

#if DEBUG_TOKENIZER
	g_print ("\n> _g_template_get_token_args: '%s'\n", token);
#endif

	state = TOKENIZER_STATE_START;
	token_list = NULL;
	token_n = 0;
	token_start = NULL;
	token_size = 0;
	n_open_brackets = 0;
	p = token;
	while (p != NULL) {
		gboolean    save_token = FALSE;
		gunichar    ch;
		const char *next_p;
		gunichar    next_ch;

		ch = (p != NULL) ? g_utf8_get_char (p) : 0;
		next_p = (p != NULL) ? g_utf8_next_char (p) : NULL;
		next_ch = (next_p != NULL) ? g_utf8_get_char (next_p) : 0;

#if DEBUG_TOKENIZER
		{
			char str[7] = { 0, 0, 0, 0, 0, 0, 0 };
			g_unichar_to_utf8 (ch, str);
			g_print ("> ch: '%s'\n", str);
		}
#endif

		switch (state) {
		case TOKENIZER_STATE_START:
			if ((ch == '%') && _g_template_valid_special_code (next_ch)) {
				/* Skip the code. */
				p = next_p;
				next_p = (p != NULL) ? g_utf8_next_char (p) : NULL;
				state = TOKENIZER_STATE_CODE;
			}
			else
				ch = 0; /* Not a special code. */
			break;

		case TOKENIZER_STATE_CODE:
			if (ch == '{') {
				n_open_brackets = 1;
				token_start = next_p;
				token_size = 0;
				state = TOKENIZER_STATE_ARG;
			}
			else
				ch = 0; /* No arguments for this code. */
			break;

		case TOKENIZER_STATE_ARG:
			if (ch == '}') {
				n_open_brackets--;
				if (n_open_brackets == 0) {
					save_token = TRUE;
					state = TOKENIZER_STATE_CODE;
				}
				else
					token_size++;
			}
			else {
				token_size++;
				if (ch == '{')
					n_open_brackets++;
			}
			break;

		default:
			g_assert_not_reached ();
			break;
		}

#if DEBUG_TOKENIZER
		g_print ("  new state: %s\n", TokenizerStateName[state]);
		g_print ("  save_token: %d\n", save_token);
		g_print ("  token_size: %ld\n", token_size);
		g_print ("  n_open_brackets: %d\n", n_open_brackets);
#endif

		if (save_token) {
			char *token;

			token = _g_utf8_strndup (token_start, token_size);
			token_list = g_list_prepend (token_list, _g_utf8_strip (token));
			token_n++;

			g_free (token);
		}

		if (ch == 0)
			break;

		p = next_p;
	}

	token_v = _g_strv_take_from_str_list (token_list, token_n);
	g_list_free (token_list);

	return token_v;
}


gboolean
_g_template_token_is (const char *token,
		      gunichar    code)
{
	return _g_template_get_token_code (token) == code;
}


/* -- _g_template_for_each_token -- */


static void
_for_each_token (const char          *tmpl,
		 TemplateFlags        flags,
		 gunichar             parent_node,
		 TemplateForEachFunc  for_each,
		 gpointer             user_data)
{
	char     **tokenv;
	int        i;
	gboolean   with_enumerator;
	gboolean   stop;

	if ((tmpl == NULL) || (for_each == NULL))
		return;

	with_enumerator = (flags & TEMPLATE_FLAGS_NO_ENUMERATOR) == 0;
	stop = FALSE;
	tokenv = _g_template_tokenize (tmpl, flags);
	for (i = 0; ! stop && (tokenv[i] != NULL); i++) {
		char     *token = tokenv[i];
		gunichar  code;

		code = _g_template_get_token_code (token);
		if (code != 0) {
			char **args;
			int    j;

			args = _g_template_get_token_args (token);
			for (j = 0; args[j] != NULL; j++)
				_for_each_token (args[j], flags, code, for_each, user_data);

			stop = for_each (parent_node, code, args, user_data);

			g_strfreev (args);
		}
		else if (with_enumerator && (token[0] == '#')) {
			char *args[] = { token, NULL };
			stop = for_each (parent_node, '#', args, user_data);
		}
		else {
			char *args[] = { token, NULL };
			stop = for_each (parent_node, code, args, user_data);
		}
	}

	g_strfreev (tokenv);
}


void
_g_template_for_each_token (const char          *tmpl,
			    TemplateFlags        flags,
			    TemplateForEachFunc  for_each,
			    gpointer             user_data)
{
	_for_each_token (tmpl, flags, 0, for_each, user_data);
}


/* -- _g_template_eval -- */


static gchar *
_eval_template (const char       *tmpl,
		TemplateFlags     flags,
		gunichar          parent_code,
		TemplateEvalFunc  eval,
		gpointer          user_data)
{
	GString   *result;
	char     **tokenv;
	int        i;
	gboolean   with_enumerator;
	gboolean   stop;

	if (tmpl == NULL)
		return NULL;

	result = g_string_new ("");
	with_enumerator = (flags & TEMPLATE_FLAGS_NO_ENUMERATOR) == 0;
	stop = FALSE;
	tokenv = _g_template_tokenize (tmpl, flags);
	for (i = 0; ! stop && (tokenv[i] != NULL); i++) {
		char     *token = tokenv[i];
		gunichar  code;

		code = _g_template_get_token_code (token);
		if (code != 0) {
			char **args;
			int    j;

			if (eval == NULL)
				continue;

			args = _g_template_get_token_args (token);
			for (j = 0; args[j] != NULL; j++)
				args[j] = _eval_template (args[j], flags, code, eval, user_data);

			stop = eval (flags, parent_code, code, args, result, user_data);

			g_strfreev (args);
		}
		else if (with_enumerator && (token[0] == '#')) {
			char *args[] = { token, NULL };

			if (eval == NULL)
				continue;

			stop = eval (flags, parent_code, '#', args, result, user_data);
		}
		else { /* Literal string. */
			if (flags & TEMPLATE_FLAGS_PREVIEW)
				_g_string_append_markup_escaped (result, "%s", token);
			else
				g_string_append (result, token);
		}
	}

	g_strfreev (tokenv);

	return g_string_free (result, FALSE);
}


char *
_g_template_eval (const char       *tmpl,
		  TemplateFlags     flags,
		  TemplateEvalFunc  eval,
		  gpointer          user_data)
{
	return _eval_template (tmpl,
			       flags,
			       0,
			       eval,
			       user_data);
}


void
_g_string_append_template_code (GString   *str,
				gunichar   code,
				char     **args)
{
	if (code == 0) {
		/* Literal string, not a special code. */
		if (args[0] != NULL)
			g_string_append (str, args[0]);
	}
	else {
		g_string_append_c (str, '%');
		g_string_append_unichar (str, code);

		if (args != NULL) {
			int i;

			for (i = 0; args[i] != NULL; i++)
				g_string_append_printf (str, "{ %s }", args[i]);
		}
	}
}


char *
_g_template_replace_enumerator (const char *token,
				int         n)
{
	long  length;
	char *format;
	char *result;

	length = 0;
	while (token != NULL) {
		gunichar ch = g_utf8_get_char (token);
		if (ch == 0)
			break;
		if (ch != '#')
			return NULL; /* Syntax error. */
		length++;
		token = g_utf8_next_char (token);
	}

	if (length == 0)
		return NULL;

	format = g_strdup_printf ("%%0" "%ld" "d", length);
	result = g_strdup_printf (format, n);

	g_free (format);

	return result;
}
