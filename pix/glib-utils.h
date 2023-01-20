/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2019 Free Software Foundation, Inc.
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

#ifndef _GLIB_UTILS_H
#define _GLIB_UTILS_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "typedefs.h"
#include "str-utils.h"
#include "uri-utils.h"

G_BEGIN_DECLS

/* Math */

#define GDOUBLE_ROUND_TO_INT(x) ((int) floor ((x) + 0.5))
#define SQR(x) ((x) * (x))
#define MIN3(x,y,z) ((y) <= (z) ? MIN ((x), (y)) : MIN ((x), (z)))
#define MAX3(x,y,z) ((y) >= (z) ? MAX ((x), (y)) : MAX ((x), (z)))

/* GFile attributes */

#define GFILE_NAME_TYPE_ATTRIBUTES "standard::name,standard::type"
#define GFILE_DISPLAY_ATTRIBUTES "standard::display-name,standard::icon,standard::symbolic-icon"
#define GFILE_BASIC_ATTRIBUTES GFILE_DISPLAY_ATTRIBUTES ",standard::name,standard::type"

#define DEFINE_STANDARD_ATTRIBUTES(rest) \
	"standard::type," \
	"standard::is-hidden," \
	"standard::is-backup," \
	"standard::name," \
	"standard::display-name," \
	"standard::edit-name," \
	"standard::icon," \
	"standard::symbolic-icon," \
	"standard::size," \
	"thumbnail::path" \
	"time::created," \
	"time::created-usec," \
	"time::modified," \
	"time::modified-usec," \
	"access::*" \
	rest

#define GFILE_STANDARD_ATTRIBUTES DEFINE_STANDARD_ATTRIBUTES("")
#define GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE DEFINE_STANDARD_ATTRIBUTES(",standard::fast-content-type")
#define GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE DEFINE_STANDARD_ATTRIBUTES(",standard::fast-content-type,standard::content-type")
#define GIO_ATTRIBUTES "standard::*,etag::*,id::*,access::*,mountable::*,time::*,unix::*,dos::*,owner::*,thumbnail::*,filesystem::*,gvfs::*,xattr::*,xattr-sys::*,selinux::*"
#define GTH_FILE_ATTRIBUTE_EMBLEMS "gth::file::emblems"
#define GTH_FILE_ATTRIBUTE_VOLUME "gth::volume"

#define GNOME_COPIED_FILES gdk_atom_intern_static_string ("x-special/gnome-copied-files")
#define IROUND(x) ((int)floor(((double)x) + 0.5))
#define FLOAT_EQUAL(a,b) (fabs (a - b) < 1e-6)
#define ID_LENGTH 8
#define G_TYPE_OBJECT_LIST g_object_list_get_type ()
#define G_TYPE_STRING_LIST g_string_list_get_type ()

#define DEFAULT_STRFTIME_FORMAT "%Y-%m-%d--%H.%M.%S"
#define DEF_ACTION_CALLBACK(name) void name (GSimpleAction *action, GVariant *parameter, gpointer user_data);

/* signals */

#define _g_signal_handlers_disconnect_by_data(instance, data) \
	g_signal_handlers_disconnect_matched ( \
		(instance), \
		G_SIGNAL_MATCH_DATA, \
		0, \
		0, \
		NULL, \
		NULL, \
		(data))

#define _g_signal_handlers_block_by_data(instance, data) \
	g_signal_handlers_block_matched ( \
		(instance), \
		G_SIGNAL_MATCH_DATA, \
		0, \
		0, \
		NULL, \
		NULL, \
		(data))

#define _g_signal_handlers_unblock_by_data(instance, data) \
	g_signal_handlers_unblock_matched ((instance), \
		G_SIGNAL_MATCH_DATA, \
		0, \
		0, \
		NULL, \
		NULL, \
		(data))

/* GObject utils */

gpointer	_g_object_ref			(gpointer	  object);
void		_g_object_unref		(gpointer	  object);
void		_g_clear_object		(gpointer	  object_p);

/* GObjectList */

GList *	_g_object_list_ref		(GList		 *list);
void		_g_object_list_unref		(GList		 *list);
GType		g_object_list_get_type		(void);

/* Enum type utils*/

GEnumValue *	_g_enum_type_get_value		(GType		  enum_type,
						 int		  value);
GEnumValue *	_g_enum_type_get_value_by_nick	(GType		  enum_type,
						 const char	 *nick);

/* idle callback */

typedef struct {
	DataFunc func;
	gpointer data;
} IdleCall;


IdleCall *	idle_call_new			(DataFunc	  func,
						 gpointer	  data);
void		idle_call_free			(IdleCall	 *call);
guint		idle_call_exec			(IdleCall	 *call,
						 gboolean	  use_idle_cb);
guint		call_when_idle			(DataFunc	  func,
						 gpointer	  data);
void		object_ready_with_error	(gpointer	  object,
						 ReadyCallback	  ready_func,
						 gpointer	  user_data,
						 GError	 *error);
void		ready_with_error		(ReadyFunc	  ready_func,
						 gpointer	  user_data,
						 GError	 *error);

/* debug */

void		debug				(const char	 *file,
						 int		  line,
						 const char	 *function,
						 const char	 *format,
						 ...) G_GNUC_PRINTF (4, 5);
void		performance			(const char	 *file,
						 int		  line,
						 const char	 *function,
						 const char	 *format,
						 ...) G_GNUC_PRINTF (4, 5);

#define DEBUG_INFO __FILE__, __LINE__, G_STRFUNC

/* GTimeVal utils */

char *		_g_struct_tm_strftime		(struct tm	 *tm,
						 const char	 *format);
int		_g_time_val_cmp		(GTimeVal	 *a,
	 					 GTimeVal	 *b);
void		_g_time_val_reset		(GTimeVal	 *time_);
gboolean	_g_time_val_from_exif_date	(const char	 *exif_date,
						 GTimeVal	 *time_);
char *		_g_time_val_to_exif_date	(GTimeVal	 *time_);
char *		_g_time_val_to_xmp_date	(GTimeVal	 *time_);
char *		_g_time_val_strftime		(GTimeVal	 *time_,
						 const char	 *format);

/* Bookmark file utils */

void		_g_bookmark_file_clear		(GBookmarkFile	 *bookmark);
void		_g_bookmark_file_add_uri	(GBookmarkFile	 *bookmark,
						 const char	 *uri);
void		_g_bookmark_file_set_uris	(GBookmarkFile	 *bookmark,
						 GList		 *uri_list);

/* GList utils */

GList *	_g_list_prepend_link		(GList		 *list,
						 GList		 *link);
GList *	_g_list_insert_list_before	(GList		 *list1,
						 GList		 *sibling,
						 GList		 *list2);
void		_g_list_reorder		(GList		 *all_files,
						 GList		 *visible_files,
						 GList		 *files_to_move,
						 int		  dest_pos,
						 int		**new_order_p,
						 GList		**new_file_list_p);

/* GStringList */

void		_g_string_list_free		(GList		 *string_list);
GList *	_g_string_list_dup		(GList		 *string_list);
char **	_g_string_list_to_strv		(GList		 *string_list);
GType		g_string_list_get_type		(void);

/* Array utils */

char *		_g_string_array_join		(GPtrArray	 *array,
						 const char	 *separator);
GPtrArray *	_g_ptr_array_dup		(GPtrArray	 *array,
						 GCopyFunc	  copy_func,
						 GDestroyNotify   free_func);

/* Regexp utils */

GRegex **	_g_regex_v_from_pattern	(const char	 *pattern_string,
						 GRegexCompileFlags
								  compile_options);
gboolean	_g_regex_v_match		(GRegex	**regexps,
						 const char	 *string,
						 GRegexMatchFlags
								  match_options);
void		_g_regex_v_free		(GRegex	**regexps);


/* URI utils */

gboolean	_g_uri_query_is_file		(const char	 *uri);
gboolean	_g_uri_query_is_dir		(const char	 *uri);
char *		_g_filename_clear_for_file	(const char	 *display_name);

/* GFile utils */

GFile *	_g_file_new_for_display_name	(const char	 *base_uri,
						 const char	 *display_name,
						 const char	 *extension);
gboolean	_g_file_equal			(GFile		 *file1,
						 GFile		 *file2);
char *		_g_file_get_display_name	(GFile		 *file);
GFileType	_g_file_query_standard_type	(GFile		 *file);
GFile *	_g_file_get_destination	(GFile		 *source,
						 GFile		 *source_base,
						 GFile		 *destination_folder);
GFile *	_g_file_get_duplicated		(GFile		 *file);
GFile *	_g_file_get_child		(GFile		 *file,
						 ...) G_GNUC_NULL_TERMINATED;
GList *	_g_file_list_dup		(GList		 *l);
void		_g_file_list_free		(GList		 *l);
GList *	_g_file_list_new_from_uriv	(char		**uris);
GList *	_g_file_list_find_file		(GList		 *l,
						 GFile		 *file);
const char *	_g_file_query_mime_type	(GFile		 *file,
						 gboolean	  fast_file_type);
int		_g_file_cmp_uris		(GFile		 *a,
						 GFile		 *b);
gboolean	_g_file_has_scheme		(GFile		 *file,
						 const char	 *scheme);
gboolean	_g_file_is_parent		(GFile		 *dir,
						 GFile		 *file);
GFile *	_g_file_append_path		(GFile		 *file,
						 const char	 *path);
gboolean	_g_file_attributes_matches_all	(const char	 *attributes,
						 const char	 *mask);
gboolean	_g_file_attributes_matches_any	(const char	 *attributes,
						 const char	 *mask);
gboolean	_g_file_attributes_matches_any_v(const char	 *attributes,
						 char		**attribute_v);
void		_g_file_info_swap_attributes	(GFileInfo	 *info,
						 const char	 *attr1,
						 const char	 *attr2);
void		_g_file_info_set_secondary_sort_order
						(GFileInfo	 *info,
						 gint32	  sort_order);
gint32		_g_file_info_get_secondary_sort_order
						(GFileInfo	 *info);
void		_g_file_info_update		(GFileInfo	 *dest_info,
						 GFileInfo	 *src_info);

/* MIME type utils */

const char *	_g_content_type_guess_from_name (const char	 *filename);
gboolean	_g_content_type_is_a		(const char	 *type,
						 const char	 *supertype);
const char *	_g_content_type_get_from_stream (GInputStream	 *istream,
						 GFile		 *file, /* optional */
						 GCancellable	 *cancellable,
						 GError	**error);
gboolean	_g_mime_type_is_image		(const char	 *mime_type);
gboolean	_g_mime_type_is_raw		(const char	 *mime_type);
gboolean	_g_mime_type_is_video		(const char	 *mime_type);
gboolean	_g_mime_type_is_audio		(const char	 *mime_type);
gboolean	_g_mime_type_has_transparency	(const char	 *mime_type);

/* GSettings utils */

char *		_g_settings_get_uri		(GSettings	 *settings,
						 const char	 *key);
char *		_g_settings_get_uri_or_special_dir
						(GSettings	 *settings,
						 const char	 *key,
						 GUserDirectory  directory);
void		_g_settings_set_uri		(GSettings	 *settings,
						 const char	 *key,
						 const char	 *uri);
void		_g_settings_set_string_list	(GSettings	 *settings,
						 const char	 *key,
						 GList		 *list);
GList *	_g_settings_get_string_list	(GSettings	 *settings,
						 const char	 *key);
GSettings *	_g_settings_new_if_schema_installed
						(const char	 *schema_name);

/* Other */

char *		_g_format_duration_for_display	(gint64	  msecs);
char *		_g_format_str_for_file		(const char	 *format,
						 GFile		 *file);
void		_g_error_free			(GError	 *error);
GString *	_g_string_append_markup_escaped (GString	 *string,
						 const char	 *format,
						 ...) G_GNUC_PRINTF (2, 3);

DEF_ACTION_CALLBACK (toggle_action_activated)

G_END_DECLS

#endif /* _GLIB_UTILS_H */
