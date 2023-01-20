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
#include "glib-utils.h"
#include "uri-utils.h"


#define _G_URI_RESERVED_CHARS_IN_HOST "[]"


/* -- _g_uri_parse --
 *
 * References:
 *
 * https://tools.ietf.org/html/rfc3986
 * https://en.wikipedia.org/wiki/Uniform_Resource_Identifier#Generic_syntax
 */


typedef struct {
	const char *scheme;
	gsize       scheme_size;
	const char *userinfo;
	gsize       userinfo_size;
	const char *host;
	gsize       host_size;
	const char *port;
	gsize       port_size;
	const char *path;
	gsize       path_size;
	const char *query;
	gsize       query_size;
	const char *fragment;
	gsize       fragment_size;
} UriInfo;


typedef enum {
	URI_STATE_ERROR,
	URI_STATE_START,
	URI_STATE_SCHEME,
	URI_STATE_HIER,
	URI_STATE_AUTHORITY,
	URI_STATE_USERINFO_OR_HOST,
	URI_STATE_IP_LITERAL,
	URI_STATE_HOST,
	URI_STATE_PORT,
	URI_STATE_PATH,
	URI_STATE_QUERY,
	URI_STATE_FRAGMENT
} UriState;


static gboolean
_g_uri_parse (const char *uri,
	      UriInfo    *info)
{
	UriState    state;
	const char *p;
	gunichar    c;
	const char *part_begin = NULL;

	if (uri == NULL)
		return FALSE;

	if (! g_utf8_validate (uri, -1, NULL))
		return FALSE;

#define CODE(x) do { x; } while (FALSE)
#define PART_SET(dest, begin, size) CODE (info->dest = begin; info->dest##_size = size)
#define PART_INIT(dest) PART_SET(dest, NULL, 0)
#define PART_BEGIN(v) CODE (state = v; part_begin = p)
#define PART_CONTINUE(v) CODE (/* void */)
#define PART_END(dest) PART_SET(dest, part_begin, p - part_begin)
#define NEXT_CHAR CODE (p = next_p; next_p = g_utf8_next_char (p))

	PART_INIT (scheme);
	PART_INIT (userinfo);
	PART_INIT (host);
	PART_INIT (port);
	PART_INIT (path);
	PART_INIT (query);
	PART_INIT (fragment);

	state = URI_STATE_START;
	p = uri;
	while ((state != URI_STATE_ERROR) && (c = g_utf8_get_char (p)) != 0) {
		const char *next_p = g_utf8_next_char (p);
		gunichar    next_c = g_utf8_get_char (next_p);

		switch (state) {
		case URI_STATE_START:
			if (c == '/')
				PART_BEGIN (URI_STATE_PATH);
			else if (g_unichar_isalpha (c))
				PART_BEGIN (URI_STATE_SCHEME);
			else
				state = URI_STATE_ERROR;
			break;

		case URI_STATE_SCHEME:
			if (c == ':') {
				PART_END (scheme);
				state = URI_STATE_HIER;
			}
			else if (g_unichar_isalpha (c)
				|| g_unichar_isdigit (c)
				|| (c == '+')
				|| (c == '-')
				|| (c == '.'))
			{
				PART_CONTINUE (URI_STATE_SCHEME);
			}
			else
				state = URI_STATE_ERROR;
			break;

		case URI_STATE_HIER:
			if ((c == '/') && (next_c == '/')) {
				state = URI_STATE_AUTHORITY;
				NEXT_CHAR;
			}
			else
				PART_BEGIN (URI_STATE_PATH);
			break;

		case URI_STATE_AUTHORITY:
			if (c == '/')
				PART_BEGIN (URI_STATE_PATH);
			else if (c == '[')
				PART_BEGIN (URI_STATE_IP_LITERAL);
			else
				PART_BEGIN (URI_STATE_USERINFO_OR_HOST);
			break;

		case URI_STATE_IP_LITERAL:
			PART_CONTINUE (URI_STATE_IP_LITERAL);
			if (c == ']') {
				PART_END (host);

				if (next_c == '?') {
					NEXT_CHAR;
					NEXT_CHAR;
					PART_BEGIN (URI_STATE_QUERY);
				}
				else if (next_c == '#') {
					NEXT_CHAR;
					NEXT_CHAR;
					PART_BEGIN (URI_STATE_FRAGMENT);
				}
				else if (next_c == ':') {
					NEXT_CHAR;
					NEXT_CHAR;
					PART_BEGIN (URI_STATE_PORT);
				}
				else {
					NEXT_CHAR;
					PART_BEGIN (URI_STATE_PATH);
				}
			}
			break;

		case URI_STATE_USERINFO_OR_HOST:
		case URI_STATE_HOST:
			if (c == '@') {
				if (state == URI_STATE_HOST) {
					state = URI_STATE_ERROR;
				}
				else {
					PART_END (userinfo);

					/* After the userinfo the host is mandatory. */

					if ((next_c == '/')
						|| (next_c == '?')
						|| (next_c == '#')
						|| (next_c == ':'))
					{
						state = URI_STATE_ERROR;
					}
					else if (next_c == '[') {
						NEXT_CHAR;
						PART_BEGIN (URI_STATE_IP_LITERAL);
					}
					else {
						NEXT_CHAR;
						PART_BEGIN (URI_STATE_HOST);
					}
				}
			}
			else if (c == '/') {
				PART_END (host);
				PART_BEGIN (URI_STATE_PATH);
			}
			else if (c == '?') {
				PART_END (host);
				NEXT_CHAR;
				PART_BEGIN (URI_STATE_QUERY);
			}
			else if (c == '#') {
				PART_END (host);
				NEXT_CHAR;
				PART_BEGIN (URI_STATE_FRAGMENT);
			}
			else if (c == ':') {

				/* Possible ambiguity: the colon can be part of
				 * the userinfo as well as separate host and port. */

				if (state == URI_STATE_HOST) {
					PART_END (host);
					NEXT_CHAR;
					PART_BEGIN (URI_STATE_PORT);
				}
				else {
					gboolean    is_userinfo = TRUE;
					const char *forward_p = next_p;

					/* Search for @ until the end of the authority part. */

					while (is_userinfo) {
						gunichar forward_c = g_utf8_get_char (forward_p);

						if ((forward_c == 0)
							|| (forward_c == '/')
							|| (forward_c == '?')
							|| (forward_c == '#'))
						{
							is_userinfo = FALSE;
							break;
						}
						else if (forward_c == '@') {
							is_userinfo = TRUE;
							break;
						}

						forward_p = g_utf8_next_char (forward_p);
					}

					if (is_userinfo) {
						PART_CONTINUE (URI_STATE_USERINFO_OR_HOST);
					}
					else {
						PART_END (host);
						NEXT_CHAR;
						PART_BEGIN (URI_STATE_PORT);
					}
				}
			}
			else
				PART_CONTINUE (URI_STATE_USERINFO_OR_HOST);
			break;

		case URI_STATE_PORT:
			if (g_unichar_isdigit (c)) {
				PART_CONTINUE (URI_STATE_PORT);
			}
			else if (c == '/') {
				PART_END (port);
				PART_BEGIN (URI_STATE_PATH);
			}
			else
				state = URI_STATE_ERROR;
			break;

		case URI_STATE_PATH:
			if (c == '?') {
				PART_END (path);
				NEXT_CHAR;
				PART_BEGIN (URI_STATE_QUERY);
			}
			else if (c == '#') {
				PART_END (path);
				NEXT_CHAR;
				PART_BEGIN (URI_STATE_FRAGMENT);
			}
			else
				PART_CONTINUE (URI_STATE_PATH);
			break;

		case URI_STATE_QUERY:
			if (c == '#') {
				PART_END (query);
				NEXT_CHAR;
				PART_BEGIN (URI_STATE_FRAGMENT);
			}
			else
				PART_CONTINUE (URI_STATE_QUERY);
			break;

		case URI_STATE_FRAGMENT:
			PART_CONTINUE (URI_STATE_FRAGMENT);
			break;

		case URI_STATE_ERROR:
			break;
		}

		p = next_p;
	}

	switch (state) {
	case URI_STATE_HIER:
	case URI_STATE_AUTHORITY:
		break;

	case URI_STATE_USERINFO_OR_HOST:
	case URI_STATE_HOST:
		PART_END (host);
		break;

	case URI_STATE_PORT:
		PART_END (port);
		break;

	case URI_STATE_PATH:
		PART_END (path);
		break;

	case URI_STATE_QUERY:
		PART_END (query);
		break;

	case URI_STATE_FRAGMENT:
		PART_END (fragment);
		break;

	default:
		state = URI_STATE_ERROR;
		break;
	}

#undef NEXT_CHAR
#undef PART_END
#undef PART_CONTINUE
#undef PART_BEGIN
#undef PART_INIT
#undef PART_SET
#undef CODE

	return state != URI_STATE_ERROR;
}


static char *
_g_uri_info_unescape_part (const char *part,
			   gsize       part_size)
{
	if (part_size == 0)
		return NULL;

	return g_uri_unescape_segment (part,
				       part + part_size,
				       NULL);
}


gboolean
_g_uri_split (const char *uri,
	      UriParts   *parts)
{
	UriInfo info;

	if (! _g_uri_parse (uri, &info))
		return FALSE;

	parts->scheme = g_strndup (info.scheme, info.scheme_size);
	parts->userinfo = _g_uri_info_unescape_part (info.userinfo, info.userinfo_size);
	parts->host = _g_uri_info_unescape_part (info.host, info.host_size);
	parts->port = g_strndup (info.port, info.port_size);
	parts->path = _g_uri_info_unescape_part (info.path, info.path_size);
	parts->query = g_strndup (info.query, info.query_size);
	parts->fragment = g_strndup (info.fragment, info.fragment_size);

	return TRUE;
}


/* Format:
 *
 * URI = scheme:[//authority]path[?query][#fragment]
 * authority = [userinfo@]host[:port]
 */


static void
_g_string_append_uri_escaped (GString    *str,
			      const char *val,
			      const char *reserved_chars_allowed)
{
	char *esc_val;

	esc_val = g_uri_escape_string (val, reserved_chars_allowed, TRUE);
	g_string_append (str, esc_val);

	g_free (esc_val);
}


char *
_g_uri_join (UriParts *parts)
{
	GString *uri;

	if (parts == NULL)
		return NULL;

	uri = g_string_new ("");

	if (parts->scheme != NULL) {
		g_string_append (uri, parts->scheme);
		g_string_append (uri, "://");

		if (parts->host != NULL) {
			if (parts->userinfo != NULL) {
				_g_string_append_uri_escaped (uri,
							      parts->userinfo,
							      G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO);
				g_string_append_c (uri, '@');
			}

			_g_string_append_uri_escaped (uri, parts->host, _G_URI_RESERVED_CHARS_IN_HOST);

			if (parts->port != NULL) {
				g_string_append_c (uri, ':');
				g_string_append (uri, parts->port);
			}
		}
	}

	if ((parts->path != NULL) && (*parts->path != 0))
		_g_string_append_uri_escaped (uri,
					      parts->path,
					      G_URI_RESERVED_CHARS_ALLOWED_IN_PATH);
	else
		g_string_append (uri, "/");

	if (parts->query != NULL) {
		g_string_append_c (uri, '?');
		g_string_append (uri, parts->query);
	}

	if (parts->fragment != NULL) {
		g_string_append_c (uri, '#');
		g_string_append (uri, parts->fragment);
	}

	return g_string_free (uri, FALSE);
}


void
_g_uri_parts_init (UriParts *parts)
{
	parts->scheme = NULL;
	parts->userinfo = NULL;
	parts->host = NULL;
	parts->port = NULL;
	parts->path = NULL;
	parts->query = NULL;
	parts->fragment = NULL;
}


void
_g_uri_parts_clear (UriParts *parts)
{
	g_free (parts->scheme);
	g_free (parts->userinfo);
	g_free (parts->host);
	g_free (parts->port);
	g_free (parts->path);
	g_free (parts->query);
	g_free (parts->fragment);
}


char *
_g_uri_get_part (const char  *uri,
		 UriPart      part)
{
	UriInfo     info;
	const char *begin = NULL;
	gsize       size = 0;

	if (! _g_uri_parse (uri, &info))
		return NULL;

	switch (part) {
	case URI_PART_SCHEME:
		begin = info.scheme;
		size = info.scheme_size;
		break;

	case URI_PART_USERINFO:
		return _g_uri_info_unescape_part (info.userinfo, info.userinfo_size);

	case URI_PART_HOST:
		begin = info.host;
		size = info.host_size;
		break;

	case URI_PART_PORT:
		begin = info.port;
		size = info.port_size;
		break;

	case URI_PART_PATH:
		return _g_uri_info_unescape_part (info.path, info.path_size);

	case URI_PART_QUERY:
		begin = info.query;
		size = info.query_size;
		break;

	case URI_PART_FRAGMENT:
		begin = info.fragment;
		size = info.fragment_size;
		break;
	}

	return g_strndup (begin, size);
}


char *
_g_uri_get_path (const char *uri)
{
	UriInfo info;

	if (! _g_uri_parse (uri, &info))
		return NULL;

	return _g_uri_info_unescape_part (info.path, info.path_size);
}


char *
_g_uri_get_basename (const char *uri)
{
	char *path;
	char *result;

	path = _g_uri_get_path (uri);
	result = g_strdup (_g_path_get_basename (path));

	g_free (path);

	return result;
}


char *
_g_uri_get_extension (const char *uri)
{
	char *path;
	char *result;

	path = _g_uri_get_path (uri);
	result = g_strdup (_g_path_get_extension (path));

	g_free (path);

	return result;
}


char *
_g_uri_get_scheme (const char *uri)
{
	UriInfo info;

	if (_g_uri_parse (uri, &info))
		return _g_utf8_strndup (info.scheme, info.scheme_size);
	else
		return NULL;
}


char *
_g_uri_get_relative_path (const char *uri,
			  const char *base)
{
	UriInfo  uri_info;
	UriInfo  base_info;
	char    *uri_path;
	char    *base_path;
	char    *rel_uri;

	if (! _g_uri_parse (uri, &uri_info))
		return NULL;

	if (! _g_uri_parse (base, &base_info))
		return g_strdup (uri);

	if ((uri_info.scheme_size != base_info.scheme_size)
		|| ! _g_utf8_n_equal (uri_info.scheme,
				      base_info.scheme,
				      uri_info.scheme_size))
	{
		return g_strdup (uri);
	}

	uri_path = _g_utf8_strndup (uri_info.path, uri_info.path_size);
	base_path = _g_utf8_strndup (base_info.path, base_info.path_size);
	rel_uri = _g_path_get_relative (uri_path, base_path);

	g_free (uri_path);
	g_free (base_path);

	return rel_uri;
}


gboolean
_g_uri_is_parent (const char *uri,
		  const char *file)
{
	UriInfo   uri_info;
	UriInfo   file_info;
	char     *path;
	char     *file_path;
	gboolean  result;

	if (! _g_uri_parse (uri, &uri_info))
		return FALSE;

	if (! _g_uri_parse (file, &file_info))
		return FALSE;

	if ((uri_info.scheme_size != file_info.scheme_size)
		|| ! _g_utf8_n_equal (uri_info.scheme,
				      file_info.scheme,
				      uri_info.scheme_size))
	{
		return FALSE;
	}

	path = _g_utf8_strndup (uri_info.path, uri_info.path_size);
	file_path = _g_utf8_strndup (file_info.path, file_info.path_size);
	result = _g_path_is_parent (path, file_path);

	g_free (file_path);
	g_free (path);

	return result;
}


char *
_g_uri_get_parent (const char *uri)
{
	UriParts  parts;
	char     *new_path;
	char     *new_uri;

	if (! _g_uri_split (uri, &parts))
		return NULL;

	new_path = _g_path_get_parent (parts.path);
	_g_str_set (&parts.path, new_path);
	_g_str_set (&parts.query, NULL);
	_g_str_set (&parts.fragment, NULL);
	new_uri = _g_uri_join (&parts);

	_g_uri_parts_clear (&parts);
	g_free (new_path);

	return new_uri;
}


char *
_g_uri_remove_extension (const char *uri)
{
	UriParts  parts;
	char     *new_path;
	char     *new_uri;

	if (! _g_uri_split (uri, &parts))
		return NULL;

	new_path = _g_path_remove_extension (parts.path);
	_g_str_set (&parts.path, new_path);
	_g_str_set (&parts.query, NULL);
	_g_str_set (&parts.fragment, NULL);
	new_uri = _g_uri_join (&parts);

	_g_uri_parts_clear (&parts);
	g_free (new_path);

	return new_uri;
}


char *
_g_uri_from_path (const char *path)
{
	char *escaped;
	char *uri;

	escaped = g_uri_escape_string (path, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
	uri = g_strconcat ("file://", escaped, NULL);

	g_free (escaped);

	return uri;
}


const char *
_g_uri_get_home (void)
{
	static char *home = NULL;

	if (home == NULL)
		home = _g_uri_from_path (g_get_home_dir ());

	return home;
}


char *
_g_uri_append_path (const char *uri,
		    const char *path)
{
	UriParts  parts;
	char     *new_path;
	char     *new_uri;

	if (! _g_uri_split (uri, &parts))
		return NULL;

	new_path = _g_path_join ((parts.path != NULL) ? parts.path : "/", path, NULL);

	g_free (parts.path);
	parts.path = new_path;
	new_uri = _g_uri_join (&parts);

	_g_uri_parts_clear (&parts);

	return new_uri;
}


/* -- _g_path_split_components -- */


typedef enum {
	COMP_STATE_START,
	COMP_STATE_NEXT_NON_SEP_STARTS_COMP,
	COMP_STATE_NEXT_SEP_ENDS_COMP,
} CompState;


char **
_g_path_split_components (const char *path,
			  int        *size)
{
	GList       *comp_list;
	int          comp_n;
	const char  *comp_begin;
	CompState    state;
	char       **comp_v;

	comp_list = NULL;
	comp_n = 0;
	comp_begin = NULL;
	state = COMP_STATE_START;
	while (path != NULL) {
		gunichar ch = g_utf8_get_char (path);
		gboolean is_sep = (ch == '/') || (ch == 0);

		switch (state) {
		case COMP_STATE_START:
			if (ch == 0) {
				break;
			}
			else if (is_sep) {
				state = COMP_STATE_NEXT_NON_SEP_STARTS_COMP;
			}
			else {
				comp_begin = path;
				state = COMP_STATE_NEXT_SEP_ENDS_COMP;
			}
			break;

		case COMP_STATE_NEXT_NON_SEP_STARTS_COMP:
			if (! is_sep) {
				comp_begin = path;
				state = COMP_STATE_NEXT_SEP_ENDS_COMP;
			}
			break;

		case COMP_STATE_NEXT_SEP_ENDS_COMP:
			if (is_sep) {
				comp_list = g_list_prepend (comp_list, g_strndup (comp_begin, path - comp_begin));
				comp_n++;
				state = COMP_STATE_START;
			}
			break;
		}

		if (ch == 0)
			break;
		path = g_utf8_next_char (path);
	}

	if ((comp_n == 0) && (path != NULL)) {
		const char *str = (state == COMP_STATE_START) ? "" : "/";
		comp_list = g_list_prepend (comp_list, g_strdup (str));
		comp_n++;
	}

	comp_v = _g_strv_take_from_str_list (comp_list, comp_n);
	if (size != NULL)
		*size = comp_n;

	g_list_free (comp_list);

	return comp_v;
}


/* -- _g_path_join_components -- */


static gboolean
_g_unichar_is_separator (gunichar ch)
{
	return ch == '/';
}


char *
_g_path_join_components (char **comp_v)
{
	GString *path;
	int      i;

	if (comp_v == NULL)
		return NULL;

	path = g_string_new ("");
	for (i = 0; comp_v[i] != NULL; i++) {
		const char *next = comp_v[i + 1];
		char       *comp;

		/* If the next component starts with a separator
		 * trim the trailing separators from this component. */

		if ((next != NULL) && (*next == '/'))
			comp = _g_utf8_rstrip_func (comp_v[i], _g_unichar_is_separator);
		else
			comp = g_strdup (comp_v[i]);
		g_string_append (path, comp);

		if ((comp != NULL) && (next != NULL) && (*next != '/')) {
			const char *last_ch = _g_utf8_last_char (comp, NULL);
			if ((last_ch != NULL) && (g_utf8_get_char (last_ch) != '/'))
				g_string_append_c (path, '/');
		}

		g_free (comp);
	}

	if ((path->len == 0) && (i > 1))
		g_string_append_c (path, '/');

	return g_string_free (path, FALSE);
}


char *
_g_path_join (const char *first, ...)
{
	va_list      args;
	const char  *arg;
	GList       *arg_list;
	int          n_comp;
	char       **argv;
	char        *result;

	arg_list = g_list_prepend (NULL, g_strdup (first));
	n_comp = 1;

	va_start (args, first);
	while ((arg = va_arg (args, const char *)) != NULL) {
		arg_list = g_list_prepend (arg_list, g_strdup (arg));
		n_comp++;
	}
	va_end (args);

	argv = _g_strv_take_from_str_list (arg_list, n_comp);
	result = _g_path_join_components (argv);

	g_strfreev (argv);
	g_list_free (arg_list);

	return result;
}


const char *
_g_path_get_basename (const char *path)
{
	const char *last_non_sep;
	const char *p;

	if (path == NULL)
		return NULL;

	last_non_sep = NULL;
	p = _g_utf8_last_char (path, NULL);
	while (p != NULL) {
		if (g_utf8_get_char (p) == '/')
			break;
		last_non_sep = p;
		p = g_utf8_find_prev_char (path, p);
	}

	return last_non_sep;
}


char *
_g_path_get_parent (const char *path)
{
	const char *p;

	p = _g_utf8_last_char (path, NULL);
	while (p != NULL) {
		if (g_utf8_get_char (p) == '/')
			break;
		p = g_utf8_find_prev_char (path, p);
	}

	if (p == NULL)
		return NULL;

	if (p == path)
		return g_strdup ("/");

	return g_strndup (path, p - path);
}


const char *
_g_path_get_extension (const char *path)
{
	char *p;
	char *p2;

	if (path == NULL)
		return NULL;

	p = g_utf8_strrchr (path, -1, '.');
	if (p == NULL)
		return NULL;

	p2 = g_utf8_find_prev_char (path, p);
	while (p2 != NULL) {
		if (g_utf8_get_char (p2) == '.')
			break;
		p2 = g_utf8_find_prev_char (path, p2);
	}

	if (_g_utf8_n_equal (p2, ".tar.", 5))
		p = p2;

	return p;
}


char *
_g_path_remove_extension (const char *path)
{
	char *new_path;
	char *ext;

	if (path == NULL)
		return NULL;

	new_path = g_strdup (path);
	ext = (char *) _g_path_get_extension (new_path);
	if (ext != NULL)
		*ext = 0;

	return new_path;
}


gboolean
_g_path_is_parent (const char *dir,
		   const char *file)
{
	const char *dir_p = NULL;
	const char *file_p = NULL;
	const char *p;
	gunichar    dir_ch[3];
	gunichar    file_ch[2];

	if ((dir == NULL) || (file == NULL))
		return FALSE;

	while ((dir != NULL) && (file != NULL)) {
		gunichar c1 = g_utf8_get_char (dir);
		gunichar c2 = g_utf8_get_char (file);

		if ((c1 == 0) || (c2 == 0))
			break;

		if (c1 != c2)
			break;

		dir_p = dir;
		file_p = file;

		dir = g_utf8_next_char (dir);
		file = g_utf8_next_char (file);
	}

	/* dir and file have the same content until dir_p and file_p. */

	/* dir_ch contains the last same char and the next two in dir. */

	p = dir_p;
	dir_ch[0] = (p != NULL) ? g_utf8_get_char (p) : 0;

	p = g_utf8_next_char (p);
	dir_ch[1] = ((dir_ch[0] != 0) && (p != NULL)) ? g_utf8_get_char (p) : 0;

	p = g_utf8_next_char (p);
	dir_ch[2] = ((dir_ch[1] != 0) && (p != NULL)) ? g_utf8_get_char (p) : 0;

	/* file_ch contains the last same char and the next one in file. */

	p = file_p;
	file_ch[0] = (p != NULL) ? g_utf8_get_char (p) : 0;

	p = g_utf8_next_char (p);
	file_ch[1] = ((file_ch[0] != 0) && (p != NULL)) ? g_utf8_get_char (p) : 0;

	g_assert (dir_ch[0] == file_ch[0]);

	if ((dir_ch[1] == 0) && (file_ch[1] == 0))
		return TRUE;

	if ((dir_ch[1] == 0) && (file_ch[1] == '/'))
		return TRUE;

	if ((dir_ch[1] == '/') && (dir_ch[2] == 0) && (file_ch[1] == 0))
		return TRUE;

	if ((dir_ch[0] == '/') && (dir_ch[1] == 0))
		return TRUE;

	return FALSE;
}


char *
_g_path_get_relative (const char *path,
		      const char *base)
{
	GString  *rel_path;
	char    **dir_v;
	char    **base_v;
	int       i;
	int       j;

	rel_path = g_string_new (NULL);
	dir_v = _g_path_split_components (path, NULL);
	base_v = _g_path_split_components (base, NULL);

	i = 0;
	while ((dir_v[i] != NULL)
		&& (base_v[i] != NULL)
		&& (strcmp (dir_v[i], base_v[i]) == 0))
	{
		i++;
	}

	j = i;

	while (base_v[i++] != NULL)
		g_string_append (rel_path, "../");

	while (dir_v[j] != NULL) {
		g_string_append (rel_path, dir_v[j]);
		if (dir_v[j + 1] != NULL)
			g_string_append_c (rel_path, '/');
		j++;
	}

	if (rel_path->len == 0)
		g_string_append (rel_path, "./");

	g_strfreev (base_v);
	g_strfreev (dir_v);

	return g_string_free (rel_path, FALSE);
}
