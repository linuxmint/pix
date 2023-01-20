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
#include <glib/gi18n.h>
#include <glib.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-file-source-vfs.h"
#include "gth-filter.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"
#include "gth-user-dir.h"
#include "gth-preferences.h"
#include "gth-shortcut.h"
#include "gtk-utils.h"
#include "pixbuf-io.h"
#include "typedefs.h"


static GMutex register_mutex;


typedef struct {
	GType        object_type;
	guint        n_params;
	const char **names;
	GValue      *values;
} GthTypeSpec;


static GthTypeSpec *
gth_type_spec_new (GType object_type)
{
	GthTypeSpec *spec;

	spec = g_new0 (GthTypeSpec, 1);
	spec->object_type = object_type;
	spec->n_params = 0;
	spec->names = NULL;
	spec->values = NULL;

	return spec;
}


static void
gth_type_spec_free (GthTypeSpec *spec)
{
	while (spec->n_params--)
		g_value_unset (&spec->values[spec->n_params]);
	g_free (spec->names);
	g_free (spec->values);
	g_free (spec);
}


static GObject *
gth_type_spec_create_object (GthTypeSpec *spec,
			     const char  *object_id)
{
	GObject *object;

	object = g_object_new_with_properties (spec->object_type,
					       spec->n_params,
					       spec->names,
					       spec->values);
	if (g_object_class_find_property (G_OBJECT_GET_CLASS (object), "id"))
		g_object_set (object, "id", object_id, NULL);

	return object;
}


static GthMain *Main = NULL;


struct _GthMainPrivate {
	GList               *file_sources;
	GList               *metadata_provider;
	GPtrArray           *metadata_category;
	GPtrArray           *metadata_info;
	GHashTable          *metadata_info_hash;
	gboolean             metadata_info_sorted;
	GPtrArray           *shortcut_category_v;
	GHashTable          *shortcut_category_h;
	GHashTable          *sort_types;
	GHashTable          *image_loaders;
	GHashTable          *types;
	GHashTable          *classes;
	GHashTable          *objects_order;
	GBookmarkFile       *bookmarks;
	GthFilterFile       *filters;
	GthTagsFile         *tags;
	GthMonitor          *monitor;
	GthExtensionManager *extension_manager;
	GthColorManager     *color_manager;
};


G_DEFINE_TYPE_WITH_CODE (GthMain,
			 gth_main,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthMain))


static void
gth_main_finalize (GObject *object)
{
	GthMain *gth_main = GTH_MAIN (object);

	_g_object_list_unref (gth_main->priv->file_sources);

	g_hash_table_unref (gth_main->priv->metadata_info_hash);
	g_ptr_array_unref (gth_main->priv->metadata_category);
	g_ptr_array_unref (gth_main->priv->metadata_info);
	g_list_foreach (gth_main->priv->metadata_provider, (GFunc) g_object_unref, NULL);
	g_list_free (gth_main->priv->metadata_provider);

	g_ptr_array_unref (gth_main->priv->shortcut_category_v);
	g_hash_table_unref (gth_main->priv->shortcut_category_h);

	if (gth_main->priv->sort_types != NULL)
		g_hash_table_unref (gth_main->priv->sort_types);
	if (gth_main->priv->image_loaders != NULL)
		g_hash_table_unref (gth_main->priv->image_loaders);
	if (gth_main->priv->types != NULL)
		g_hash_table_unref (gth_main->priv->types);
	if (gth_main->priv->classes != NULL)
		g_hash_table_unref (gth_main->priv->classes);
	if (gth_main->priv->objects_order != NULL)
		g_hash_table_unref (gth_main->priv->objects_order);

	if (gth_main->priv->bookmarks != NULL)
		g_bookmark_file_free (gth_main->priv->bookmarks);

	_g_object_unref (gth_main->priv->monitor);
	_g_object_unref (gth_main->priv->extension_manager);
	_g_object_unref (gth_main->priv->color_manager);
	gth_filter_file_free (gth_main->priv->filters);
	gth_tags_file_free (gth_main->priv->tags);

	G_OBJECT_CLASS (gth_main_parent_class)->finalize (object);
}


static void
gth_main_class_init (GthMainClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_main_finalize;
}


static void
gth_main_init (GthMain *main)
{
	main->priv = gth_main_get_instance_private (main);
	main->priv->file_sources = NULL;
	main->priv->metadata_provider = NULL;
	main->priv->metadata_category = g_ptr_array_new ();
	main->priv->metadata_info = g_ptr_array_new ();
	main->priv->metadata_info_hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	main->priv->metadata_info_sorted = FALSE;
	main->priv->shortcut_category_v = g_ptr_array_new ();
	main->priv->shortcut_category_h = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	main->priv->sort_types = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	main->priv->image_loaders = g_hash_table_new_full (g_str_hash,
						           (GEqualFunc) g_content_type_equals,
						           g_free,
						           NULL);
	main->priv->types = NULL;
	main->priv->classes = NULL;
	main->priv->objects_order = NULL;
	main->priv->bookmarks = NULL;
	main->priv->filters = NULL;
	main->priv->tags = NULL;
	main->priv->monitor = NULL;
	main->priv->extension_manager = gth_extension_manager_new ();
	main->priv->color_manager = NULL;
}


void
gth_main_initialize (void)
{
	if (Main != NULL)
		return;

	Main = (GthMain*) g_object_new (GTH_TYPE_MAIN, NULL);
}


void
gth_main_release (void)
{
	if (Main != NULL)
		g_object_unref (Main);
}


void
gth_main_register_file_source (GType file_source_type)
{
	GObject *file_source;

	g_mutex_lock (&register_mutex);

	file_source = g_object_new (file_source_type, NULL);
	Main->priv->file_sources = g_list_append (Main->priv->file_sources, file_source);

	g_mutex_unlock (&register_mutex);
}


GthFileSource *
gth_main_get_file_source_for_uri (const char *uri)
{
	GthFileSource *file_source = NULL;
	GList         *scan;

	for (scan = Main->priv->file_sources; scan; scan = scan->next) {
		GthFileSource *registered_file_source = scan->data;

		if (gth_file_source_supports_scheme (registered_file_source, uri)) {
			file_source = g_object_new (G_OBJECT_TYPE (registered_file_source), NULL);
			break;
		}
	}

	if (file_source == NULL)
		file_source = g_object_new (GTH_TYPE_FILE_SOURCE_VFS, NULL);

	return file_source;
}


GthFileSource *
gth_main_get_file_source (GFile *file)
{
	char          *uri;
	GthFileSource *file_source;

	if (file == NULL)
		return NULL;

	uri = g_file_get_uri (file);
	file_source = gth_main_get_file_source_for_uri (uri);
	g_free (uri);

	return file_source;
}


char *
gth_main_get_source_scheme (const char *uri)
{
	GList *scan;

	for (scan = Main->priv->file_sources; scan; scan = scan->next) {
		GthFileSource *registered_file_source = scan->data;

		if (gth_file_source_supports_scheme (registered_file_source, uri))
			return _g_uri_get_scheme (uri);
	}

	return g_strdup ("file");
}


GList *
gth_main_get_all_file_sources (void)
{
	return Main->priv->file_sources;
}


#define MAX_ENTRY_POINTS_PER_SOURCE 1000


GList *
gth_main_get_all_entry_points (void)
{
	GList *list = NULL;
	int    sort_order = 0;
	GList *scan;

	for (scan = gth_main_get_all_file_sources (); scan; scan = scan->next) {
		GthFileSource *file_source = scan->data;
		GList         *entry_points;
		GList         *scan_entry;

		entry_points = gth_file_source_get_entry_points (file_source);
		for (scan_entry = entry_points; scan_entry; scan_entry = scan_entry->next) {
			GthFileData *file_data = scan_entry->data;
			g_file_info_set_sort_order (file_data->info, sort_order++);
		}

		list = g_list_concat (list, entry_points);
		sort_order += MAX_ENTRY_POINTS_PER_SOURCE;
	}

	return list;
}


GthFileData *
gth_main_get_nearest_entry_point (GFile *file)
{
	GList *list;
	GList *scan;
	GList *entries;
	GthFileData *nearest;
	char  *uri;
	int    file_uri_len;
	int    min_diff;

	entries = NULL;
	list = gth_main_get_all_entry_points ();
	for (scan = list; scan; scan = scan->next) {
		GthFileData *entry_point = scan->data;

		if (_g_file_is_parent (entry_point->file, file))
			entries = g_list_prepend (entries, g_object_ref (entry_point));
	}

	nearest = NULL;
	uri = g_file_get_uri (file);
	file_uri_len = strlen (uri);
	min_diff = 0;
	for (scan = entries; scan; scan = scan->next) {
		GthFileData *entry_point = scan->data;
		char *entry_uri;
		int   entry_len;
		int   diff;

		entry_uri = g_file_get_uri (entry_point->file);
		entry_len = strlen (entry_uri);
		diff = abs (entry_len - file_uri_len);
		if ((scan == entries) || (diff < min_diff)) {
			min_diff = diff;
			nearest = entry_point;
		}

		g_free (entry_uri);
	}
	g_free (uri);

	_g_object_ref (nearest);

	_g_object_list_unref (entries);
	_g_object_list_unref (list);

	return nearest;
}


char *
gth_main_get_gio_uri (const char *uri)
{
	GthFileSource *file_source;
	GFile         *file;
	GFile         *gio_file;
	char          *gio_uri;

	file_source = gth_main_get_file_source_for_uri (uri);
	file = g_file_new_for_uri (uri);
	gio_file = gth_file_source_to_gio_file (file_source, file);
	gio_uri = g_file_get_uri (gio_file);

	g_object_unref (gio_file);
	g_object_unref (file);
	g_object_unref (file_source);

	return gio_uri;
}


GFile *
gth_main_get_gio_file (GFile *file)
{
	GthFileSource *file_source;
	GFile         *gio_file;

	file_source = gth_main_get_file_source (file);
	gio_file = gth_file_source_to_gio_file (file_source, file);
	g_object_unref (file_source);

	return gio_file;
}


void
gth_main_register_metadata_category (GthMetadataCategory *metadata_category)
{
	int i;

	g_mutex_lock (&register_mutex);

	for (i = 0; metadata_category[i].id != NULL; i++)
		if (gth_main_get_metadata_category (metadata_category[i].id) == NULL)
			g_ptr_array_add (Main->priv->metadata_category, &metadata_category[i]);

	g_mutex_unlock (&register_mutex);
}


static GMutex metadata_info_mutex;


GthMetadataInfo *
gth_main_register_metadata_info (GthMetadataInfo *metadata_info)
{
	GthMetadataInfo *info;

	if ((metadata_info->display_name != NULL) && (strstr (metadata_info->display_name, "0x") != NULL))
		metadata_info->flags = GTH_METADATA_ALLOW_NOWHERE;

	g_mutex_lock (&metadata_info_mutex);

	info = gth_metadata_info_dup (metadata_info);
	g_ptr_array_add (Main->priv->metadata_info, info);
	g_hash_table_insert (Main->priv->metadata_info_hash, (gpointer) info->id, info);
	Main->priv->metadata_info_sorted = FALSE;

	g_mutex_unlock (&metadata_info_mutex);

	return info;
}


void
gth_main_register_metadata_info_v (GthMetadataInfo metadata_info[])
{
	int i;

	g_mutex_lock (&metadata_info_mutex);

	for (i = 0; metadata_info[i].id != NULL; i++)
		if ((metadata_info[i].display_name == NULL) || (strstr (metadata_info[i].display_name, "0x") == NULL)) {
			if (metadata_info[i].sort_order <= 0)
				metadata_info[i].sort_order = 500;
			g_ptr_array_add (Main->priv->metadata_info, &metadata_info[i]);
			g_hash_table_insert (Main->priv->metadata_info_hash, (gpointer) (&metadata_info[i])->id, &metadata_info[i]);
		}

	g_mutex_unlock (&metadata_info_mutex);
}


void
gth_main_register_metadata_provider (GType metadata_provider_type)
{
	GObject *metadata;

	g_mutex_lock (&register_mutex);

	metadata = g_object_new (metadata_provider_type, NULL);
	Main->priv->metadata_provider = g_list_append (Main->priv->metadata_provider, metadata);

	g_mutex_unlock (&register_mutex);
}


GList *
gth_main_get_all_metadata_providers (void)
{
	return Main->priv->metadata_provider;
}


static int
metadata_info_sort_func (gconstpointer a,
			 gconstpointer b)
{
	GthMetadataInfo *info_a = *((GthMetadataInfo **) a);
	GthMetadataInfo *info_b = *((GthMetadataInfo **) b);
	const char      *name_a;
	const char      *name_b;

	name_a = info_a->display_name;
	if (name_a == NULL)
		name_a = info_a->id;

	name_b = info_b->display_name;
	if (name_b == NULL)
		name_b = info_b->id;

	return g_utf8_collate (name_a, name_b);
}


char **
gth_main_get_metadata_attributes (const char *mask)
{
	GList                  *list = NULL;
	GFileAttributeMatcher  *matcher;
	int                     i;
	int                     n;
	GList                  *scan;
	char                  **values;

	g_mutex_lock (&metadata_info_mutex);

	if (! Main->priv->metadata_info_sorted) {
		g_ptr_array_sort (Main->priv->metadata_info, metadata_info_sort_func);
		Main->priv->metadata_info_sorted = TRUE;
	}

	matcher = g_file_attribute_matcher_new (mask);
	for (n = 0, i = 0; i < Main->priv->metadata_info->len; i++) {
		GthMetadataInfo *metadata_info = g_ptr_array_index (Main->priv->metadata_info, i);

		if (g_file_attribute_matcher_matches (matcher, metadata_info->id)) {
			list = g_list_prepend (list, (char *) metadata_info->id);
			n++;
		}
	}
	list = g_list_reverse (list);

	g_mutex_unlock (&metadata_info_mutex);

	values = g_new (char *, n + 1);
	for (i = 0, scan = list; scan; i++, scan = scan->next)
		values[i] = g_strdup (scan->data);
	values[n] = NULL;

	g_list_free (list);
	g_file_attribute_matcher_unref (matcher);

	return values;
}


GthMetadataProvider *
gth_main_get_metadata_reader (const char *id,
			      const char *mime_type)
{
	GthMetadataProvider *metadata = NULL;
	GList               *scan;
	const char          *attribute_v[] = { id, NULL };

	for (scan = Main->priv->metadata_provider; scan; scan = scan->next) {
		GthMetadataProvider *registered_metadata = scan->data;

		if (gth_metadata_provider_can_read (registered_metadata, NULL, mime_type, (char **)attribute_v)) {
			metadata = g_object_new (G_OBJECT_TYPE (registered_metadata), NULL);
			break;
		}
	}

	return metadata;
}


GthMetadataProvider *
gth_main_get_metadata_writer (const char *id,
			      const char *mime_type)
{
	GthMetadataProvider *metadata = NULL;
	GList               *scan;
	const char          *attribute_v[] = { id, NULL };

	for (scan = Main->priv->metadata_provider; scan; scan = scan->next) {
		GthMetadataProvider *registered_metadata = scan->data;

		if (gth_metadata_provider_can_write (registered_metadata, mime_type, (char **)attribute_v)) {
			metadata = g_object_new (G_OBJECT_TYPE (registered_metadata), NULL);
			break;
		}
	}

	return metadata;
}


GthMetadataCategory *
gth_main_get_metadata_category (const char *id)
{
	int i;

	if (id == NULL)
		return NULL;

	for (i = 0; i < Main->priv->metadata_category->len; i++) {
		GthMetadataCategory *category;

		category = g_ptr_array_index (Main->priv->metadata_category, i);
		if (strcmp (category->id, id) == 0)
			return category;
	}

	return NULL;
}


GthMetadataInfo *
gth_main_get_metadata_info (const char *id)
{
	GthMetadataInfo *info;

	if (id == NULL)
		return NULL;

	info = g_hash_table_lookup (Main->priv->metadata_info_hash, id);

	return info;
}


GList *
gth_main_get_all_metadata_info (void)
{
	GList *list = NULL;
	int    i;

	g_mutex_lock (&metadata_info_mutex);

	for (i = 0; i < Main->priv->metadata_info->len; i++) {
		GthMetadataInfo *metadata_info = g_ptr_array_index (Main->priv->metadata_info, i);

		list = g_list_prepend (list, metadata_info);
	}
	list = g_list_reverse (list);

	g_mutex_unlock (&metadata_info_mutex);

	return list;
}


GthShortcutCategory *
gth_main_get_shortcut_category (const char *id)
{
	return g_hash_table_lookup (Main->priv->shortcut_category_h, id);
}


GPtrArray *
gth_main_get_shortcut_categories (void)
{
	return Main->priv->shortcut_category_v;
}


void
gth_main_register_shortcut_category (GthShortcutCategory *shortcut_category,
				     int                  n_categories)
{
	int i;

	g_mutex_lock (&register_mutex);
	for (i = 0; i < n_categories; i++) {
		if (gth_main_get_shortcut_category (shortcut_category[i].id) == NULL) {
			g_ptr_array_add (Main->priv->shortcut_category_v, &shortcut_category[i]);
			g_hash_table_insert (Main->priv->shortcut_category_h, shortcut_category[i].id, &shortcut_category[i]);
		}
	}
	g_mutex_unlock (&register_mutex);
}


void
gth_main_register_sort_type (GthFileDataSort *sort_type)
{
	g_mutex_lock (&register_mutex);
	g_hash_table_insert (Main->priv->sort_types, (gpointer) sort_type->name, sort_type);
	g_mutex_unlock (&register_mutex);
}


GthFileDataSort *
gth_main_get_sort_type (const char *name)
{
	GthFileDataSort *retval =  NULL;

	if (name == NULL)
		return NULL;

	retval = g_hash_table_lookup (Main->priv->sort_types, name);

	if (retval != NULL)
		return retval;
	else
		return g_hash_table_lookup (Main->priv->sort_types, "file::name");
}


static void
collect_sort_types (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
	GList           **sort_types = user_data;
	GthFileDataSort  *sort_type = value;

	*sort_types = g_list_prepend (*sort_types, sort_type);
}


static gboolean
campare_sort_types (GthFileDataSort *type1,
		    GthFileDataSort *type2)
{
	return g_utf8_collate (type1->display_name, type2->display_name);
}


GList *
gth_main_get_all_sort_types (void)
{
	GList *sort_types = NULL;

	g_hash_table_foreach (Main->priv->sort_types, collect_sort_types, &sort_types);
	return g_list_sort (sort_types, (GCompareFunc) campare_sort_types);
}


static GthTypeSpec *
_gth_main_create_type_spec (GType       object_type,
			    va_list     var_args)
{
	GObject     *object;
	GthTypeSpec *type_spec;
	const char  *name;
	guint        max_params = 16;

	type_spec = gth_type_spec_new (object_type);

    	object = g_object_new (object_type, NULL);

  	while ((name = va_arg (var_args, char *)) != NULL) {
		GParamSpec *pspec;
		char       *error = NULL;

		if (type_spec->n_params == max_params - 1) {
			g_warning ("%s: too many params (max: %d)", G_STRFUNC, max_params);
			break;
		}

		if (type_spec->names == NULL) {
			int i;

			type_spec->names = g_new (const char *, max_params);
			for (i = 0; i < max_params; i++)
				type_spec->names[i] = NULL;
		}

		if (type_spec->values == NULL) {
			int i;

			type_spec->values = g_new (GValue, max_params);
			for (i = 0; i < max_params; i++) {
				type_spec->values[i].g_type = 0;
			}
		}

		pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), name);
		if (pspec == NULL) {
			g_warning ("%s: object class `%s' has no property named `%s'",
				   G_STRFUNC,
				   g_type_name (object_type),
				   name);
			break;
		}
		type_spec->names[type_spec->n_params] = name;
		g_value_init (&type_spec->values[type_spec->n_params], G_PARAM_SPEC_VALUE_TYPE (pspec));
		G_VALUE_COLLECT (&type_spec->values[type_spec->n_params], var_args, 0, &error);
		if (error != NULL) {
			g_warning ("%s: %s", G_STRFUNC, error);
			g_free (error);
			g_value_unset (&type_spec->values[type_spec->n_params]);
			break;
		}
		type_spec->n_params++;
	}

	g_object_unref (object);

	return type_spec;
}


static void
_register_image_loader_func (GthImageLoaderFunc  loader,
			     GthImageFormat      native_format,
			     const char         *mime_type)
{
	char *key;

	key = g_strdup_printf ("%s-%d", mime_type, native_format);
	g_hash_table_insert (Main->priv->image_loaders, (gpointer) key, loader);
}


void
gth_main_register_image_loader_func (GthImageLoaderFunc  loader,
				     GthImageFormat      native_format,
				     ...)
{
	va_list     var_args;
	const char *mime_type;

	g_mutex_lock (&register_mutex);

	va_start (var_args, native_format);
  	while ((mime_type = va_arg (var_args, const char *)) != NULL)
  		_register_image_loader_func (loader, native_format, mime_type);
	va_end (var_args);

	g_mutex_unlock (&register_mutex);
}


void
gth_main_register_image_loader_func_v (GthImageLoaderFunc   loader,
				       GthImageFormat       native_format,
				       const char         **mime_types)
{
	int i;

	g_mutex_lock (&register_mutex);

	for (i = 0; mime_types[i] != NULL; i++)
		_register_image_loader_func (loader, native_format, mime_types[i]);

	g_mutex_unlock (&register_mutex);
}


GthImageLoaderFunc
gth_main_get_image_loader_func (const char     *mime_type,
				GthImageFormat  preferred_format)
{
	GthImageLoaderFunc  loader;
	char               *key;
	int                 format;

	/* give priority to the preferred format */

	key = g_strdup_printf ("%s-%d", mime_type, preferred_format);
	loader = g_hash_table_lookup (Main->priv->image_loaders, key);

	/* if the preferred format is not available, search another
	 * format. */

	for (format = 0; (loader == NULL) && (format < GTH_IMAGE_N_FORMATS); format++) {
		if (format == preferred_format)
			continue;
		g_free (key);
		key = g_strdup_printf ("%s-%d", mime_type, format);
		loader = g_hash_table_lookup (Main->priv->image_loaders, key);
	}

	g_free (key);

	return loader;
}


GthImageSaver *
gth_main_get_image_saver (const char *mime_type)
{
	GArray *savers;
	int     i;

	savers = gth_main_get_type_set ("image-saver");
	if (savers == NULL)
		return NULL;

	for (i = 0; i < savers->len; i++) {
		GType           saver_type;
		GthImageSaver *saver;

		saver_type = g_array_index (savers, GType, i);
		saver = g_object_new (saver_type, NULL);
		if (gth_image_saver_can_save (saver, mime_type))
			return saver;

		g_object_unref (saver);
	}

	return NULL;
}


GthTest *
gth_main_get_general_filter (void)
{
	GSettings *settings;
	char      *filter_name;
	GthTest   *filter;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	filter_name = g_settings_get_string (settings, PREF_BROWSER_GENERAL_FILTER);
	filter = gth_main_get_registered_object (GTH_TYPE_TEST, filter_name);

	g_free (filter_name);
	g_object_unref (settings);

	return filter;
}


GthTest *
gth_main_add_general_filter (GthTest *original_filter)
{
	GthTest *test;

	if (original_filter == NULL)
		test = gth_main_get_general_filter ();
	else
		test = (GthTest *) gth_duplicable_duplicate (GTH_DUPLICABLE (original_filter));

	if (! GTH_IS_FILTER (test)) {
		GthTest   *new_chain;
		GthFilter *filter;

		filter = gth_filter_new ();

		new_chain = gth_test_chain_new (GTH_MATCH_TYPE_ALL, NULL);
		gth_test_chain_add_test (GTH_TEST_CHAIN (new_chain), test);

		if (strncmp (gth_test_get_id (test), "file::type::", 12) != 0) {
			GthTest *file_type_filter;

			file_type_filter = gth_main_get_general_filter ();
			gth_test_chain_add_test (GTH_TEST_CHAIN (new_chain), file_type_filter);
			g_object_unref (file_type_filter);
		}
		gth_filter_set_test (filter, GTH_TEST_CHAIN (new_chain));
		g_object_unref (new_chain);

		g_object_unref (test);
		test = (GthTest*) filter;
	}
	else {
		GthFilter    *filter;
		GthTestChain *filter_test;

		filter = (GthFilter *) gth_duplicable_duplicate (GTH_DUPLICABLE (test));
		filter_test = gth_filter_get_test (filter);
		if ((filter_test == NULL) || ! gth_test_chain_has_type_test (filter_test)) {
			GthTest *new_filter_test;
			GthTest *general_filter;

			new_filter_test = gth_test_chain_new (GTH_MATCH_TYPE_ALL, NULL);
			if (filter_test != NULL)
				gth_test_chain_add_test (GTH_TEST_CHAIN (new_filter_test), GTH_TEST (filter_test));

			general_filter = gth_main_get_general_filter ();
			gth_test_chain_add_test (GTH_TEST_CHAIN (new_filter_test), general_filter);
			g_object_unref (general_filter);

			gth_filter_set_test (filter, GTH_TEST_CHAIN (new_filter_test));
			g_object_unref (new_filter_test);
		}

		if (filter_test != NULL)
			g_object_unref (filter_test);
		g_object_unref (test);

		test = (GthTest*) filter;
	}

	return test;
}


static void
_g_destroy_array (GArray *array)
{
	g_array_free (array, TRUE);
}


void
gth_main_register_type (const char *set_name,
			GType       object_type)
{
	GArray *set;

	g_mutex_lock (&register_mutex);

	if (Main->priv->types == NULL)
		Main->priv->types = g_hash_table_new_full (g_str_hash,
							   g_str_equal,
							   (GDestroyNotify) g_free,
							   (GDestroyNotify) _g_destroy_array);

	set = g_hash_table_lookup (Main->priv->types, set_name);
	if (set == NULL) {
		set = g_array_new (FALSE, FALSE, sizeof (GType));
		g_hash_table_insert (Main->priv->types, g_strdup (set_name), set);
	}

	g_array_append_val (set, object_type);

	g_mutex_unlock (&register_mutex);
}


GArray *
gth_main_get_type_set (const char *set_name)
{
	return g_hash_table_lookup (Main->priv->types, set_name);
}


static void
g_ptr_array_destroy (gpointer array)
{
	g_ptr_array_unref ((GPtrArray *) array);
}


void
gth_main_register_object (GType       superclass_type,
			  const char *object_id,
			  GType       object_type,
			  ...)
{
	const char  *superclass_name;
	GHashTable  *object_hash;
	GPtrArray   *object_order;
	va_list      var_args;
	GthTypeSpec *spec;
	char        *id;

	g_mutex_lock (&register_mutex);

	if (object_id == NULL)
		object_id = g_type_name (object_type);

	if (Main->priv->classes == NULL) {
		Main->priv->classes = g_hash_table_new_full (g_str_hash,
							     g_str_equal,
							     (GDestroyNotify) g_free,
							     (GDestroyNotify) g_hash_table_destroy);
		Main->priv->objects_order = g_hash_table_new_full (g_str_hash,
							           g_str_equal,
							           (GDestroyNotify) g_free,
							           g_ptr_array_destroy);
	}

	superclass_name = g_type_name (superclass_type);
	object_hash = g_hash_table_lookup (Main->priv->classes, superclass_name);
	object_order = g_hash_table_lookup (Main->priv->objects_order, superclass_name);

	if (object_hash == NULL) {
		object_hash = g_hash_table_new_full (g_str_hash,
						     g_str_equal,
						     (GDestroyNotify) g_free,
						     (GDestroyNotify) gth_type_spec_free);
		g_hash_table_insert (Main->priv->classes, g_strdup (superclass_name), object_hash);

		object_order = g_ptr_array_new ();
		g_hash_table_insert (Main->priv->objects_order, g_strdup (superclass_name), object_order);
	}

	va_start (var_args, object_type);
	spec = _gth_main_create_type_spec (object_type, var_args);
	va_end (var_args);

	id = g_strdup (object_id);
	g_hash_table_insert (object_hash, id, spec);
	g_ptr_array_add (object_order, id);

	g_mutex_unlock (&register_mutex);
}


GList *
gth_main_get_registered_objects (GType superclass_type)
{
	GList      *objects = NULL;
	GHashTable *object_hash;
	GPtrArray  *object_order;
	int         i;

	object_hash = g_hash_table_lookup (Main->priv->classes, g_type_name (superclass_type));
	if (object_hash == NULL)
		return NULL;
	object_order = g_hash_table_lookup (Main->priv->objects_order, g_type_name (superclass_type));

	for (i = object_order->len - 1; i >= 0; i--) {
		char        *object_id;
		GthTypeSpec *spec;

		object_id = g_ptr_array_index (object_order, i);
		spec = g_hash_table_lookup (object_hash, object_id);
		objects = g_list_prepend (objects, gth_type_spec_create_object (spec, object_id));
	}

	return objects;
}


GList *
gth_main_get_registered_objects_id (GType superclass_type)
{
	GList      *objects = NULL;
	GHashTable *object_hash;
	GPtrArray  *object_order;
	int         i;

	object_hash = g_hash_table_lookup (Main->priv->classes, g_type_name (superclass_type));
	if (object_hash == NULL)
		return NULL;
	object_order = g_hash_table_lookup (Main->priv->objects_order, g_type_name (superclass_type));

	for (i = object_order->len - 1; i >= 0; i--) {
		char *object_id;

		object_id = g_ptr_array_index (object_order, i);
		objects = g_list_prepend (objects, g_strdup (object_id));
	}

	return objects;
}


gpointer
gth_main_get_registered_object (GType       superclass_type,
				const char *object_id)
{
	GHashTable  *object_hash;
	GthTypeSpec *spec;

	object_hash = g_hash_table_lookup (Main->priv->classes, g_type_name (superclass_type));
	if (object_hash == NULL)
		return NULL;

	spec = g_hash_table_lookup (object_hash, object_id);
	if (spec == NULL)
		return NULL;

	return (gpointer) gth_type_spec_create_object (spec, object_id);
}


GBookmarkFile *
gth_main_get_default_bookmarks (void)
{
	GFile *file;
	char  *filename;

	if (Main->priv->bookmarks != NULL)
		return Main->priv->bookmarks;

	Main->priv->bookmarks = g_bookmark_file_new ();

	file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, BOOKMARKS_FILE, NULL);
	filename = g_file_get_path (file);
	g_bookmark_file_load_from_file (Main->priv->bookmarks, filename, NULL);

	g_free (filename);
	g_object_unref (file);

	return Main->priv->bookmarks;
}


void
gth_main_bookmarks_changed (void)
{
	GFile *file;
	char  *filename;

	file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, BOOKMARKS_FILE, NULL);
	filename = g_file_get_path (file);
	g_bookmark_file_to_file (Main->priv->bookmarks, filename, NULL);

	g_free (filename);
	g_object_unref (file);

	gth_monitor_bookmarks_changed (gth_main_get_default_monitor ());
}


void
gth_main_shortcuts_changed (GPtrArray *shortcuts_v)
{
	if (gth_shortcuts_write_to_file (shortcuts_v, NULL))
		gth_monitor_shortcuts_changed (gth_main_get_default_monitor ());
}


GthFilterFile *
gth_main_get_default_filter_file (void)
{
	GFile *file;

	if (Main->priv->filters != NULL)
		return Main->priv->filters;

	Main->priv->filters = gth_filter_file_new ();
	file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, FILTERS_FILE, NULL);
	gth_filter_file_load_from_file (Main->priv->filters, file, NULL);

	g_object_unref (file);

	return Main->priv->filters;
}


GList *
gth_main_get_all_filters (void)
{
	GthFilterFile *filter_file;
	GList         *filters;
	GList         *registered_tests;
	GList         *scan;
	gboolean       changed = FALSE;

	filter_file = gth_main_get_default_filter_file ();
	filters = gth_filter_file_get_tests (filter_file);

	registered_tests = gth_main_get_registered_objects_id (GTH_TYPE_TEST);
	for (scan = registered_tests; scan; scan = scan->next) {
		const char *registered_test_id = scan->data;
		gboolean    test_present = FALSE;
		GList      *scan2;

		for (scan2 = filters; ! test_present && scan2; scan2 = scan2->next) {
			GthTest *test = scan2->data;

			if (g_strcmp0 (gth_test_get_id (test), registered_test_id) == 0)
				test_present = TRUE;
		}

		if (! test_present) {
			GthTest *registered_test;

			registered_test = gth_main_get_registered_object (GTH_TYPE_TEST, registered_test_id);
			filters = g_list_append (filters, registered_test);
			gth_filter_file_add (filter_file, registered_test);
			changed = TRUE;
		}
	}
	_g_string_list_free (registered_tests);

	if (changed)
		gth_main_filters_changed ();

	return filters;
}


void
gth_main_filters_changed (void)
{
	GFile *file;

	file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, FILTERS_FILE, NULL);
	gth_filter_file_to_file (Main->priv->filters, file, NULL);
	gth_monitor_filters_changed (gth_main_get_default_monitor ());

	g_object_unref (file);
}


GthTagsFile *
gth_main_get_default_tag_file (void)
{
	GFile *file;

	if (Main->priv->tags != NULL)
		return Main->priv->tags;

	Main->priv->tags = gth_tags_file_new ();
	file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, TAGS_FILE, NULL);
	gth_tags_file_load_from_file (Main->priv->tags, file, NULL);

	g_object_unref (file);

	return Main->priv->tags;
}


char **
gth_main_get_all_tags (void)
{
	return gth_tags_file_get_tags (gth_main_get_default_tag_file ());
}


void
gth_main_tags_changed (void)
{
	GFile *file;

	file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, TAGS_FILE, NULL);
	gth_tags_file_to_file (Main->priv->tags, file, NULL);
	gth_monitor_tags_changed (gth_main_get_default_monitor ());

	g_object_unref (file);
}


GthMonitor *
gth_main_get_default_monitor (void)
{
	if (G_LIKELY (Main->priv->monitor != NULL))
		return Main->priv->monitor;

	Main->priv->monitor = gth_monitor_new ();

	return Main->priv->monitor;
}


GthExtensionManager *
gth_main_get_default_extension_manager (void)
{
	return Main->priv->extension_manager;
}


GthColorManager *
gth_main_get_default_color_manager (void)
{
	if (Main->priv->color_manager == NULL)
		Main->priv->color_manager = gth_color_manager_new ();
	return Main->priv->color_manager;
}


void
gth_main_activate_extensions (void)
{
	const char *mandatory_extensions[] = {
		"file_viewer", /* keep the file viewer before any other viewer (see comment in gth-browser:file_metadata_ready_cb). */
#ifdef HAVE_LIBJPEG
		"jpeg_utils",  /* mandatory if jpeg support is activated at compile time */
#endif
		"cairo_io",
		"image_viewer",
		"file_tools",
		NULL
	};
	const char *default_extensions[] = {
		"23hq",
		"bookmarks",
		"burn_disc",
		"catalogs",
		"change_date",
		"comments",
		"contact_sheet",
		"convert_format",
		"desktop_background",
		"edit_metadata",
		"exiv2_tools",
		"file_manager",
		"find_duplicates",
		"flicker",
		"gstreamer_tools",
		"gstreamer_utils",
		"image_print",
		"image_rotation",
		"importer",
		"jpeg_utils",
		"list_tools",
		"oauth",
		"photo_importer",
		"raw_files",
		"red_eye_removal",
		"rename_series",
		"resize_images",
		"search",
		"selections",
		"slideshow",
		"terminal",
		"webalbums",
		NULL
	};
	GSettings            *settings;
	char                **user_actived_extensions;
	char                **actived_extensions;
	GList                *ordered_extensions;
	GthExtensionManager  *manager;
	GList                *scan;

	if (Main->priv->extension_manager == NULL)
		Main->priv->extension_manager = gth_extension_manager_new ();

	settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	user_actived_extensions = g_settings_get_strv (settings, PREF_GENERAL_ACTIVE_EXTENSIONS);
	if ((user_actived_extensions != NULL)
	     && (user_actived_extensions[1] == NULL)
	     && (g_strcmp0 (user_actived_extensions[0], "default") == 0))
	{
		g_strfreev (user_actived_extensions);
		user_actived_extensions = g_strdupv ((char **) default_extensions);
		g_settings_set_strv (settings, PREF_GENERAL_ACTIVE_EXTENSIONS, (const char *const *) user_actived_extensions);
	}
	actived_extensions = _g_strv_concat ((char **) mandatory_extensions, user_actived_extensions);
	ordered_extensions = gth_extension_manager_order_extensions (Main->priv->extension_manager, actived_extensions);

	manager = gth_main_get_default_extension_manager ();
	for (scan = ordered_extensions; scan; scan = scan->next) {
		char                    *name = scan->data;
		gboolean                 mandatory;
		GthExtensionDescription *description;
		GError                  *error = NULL;

		mandatory = _g_strv_contains ((char **) mandatory_extensions, name);
		description = gth_extension_manager_get_description (manager, name);
		if (! mandatory && (description != NULL) && description->hidden)
			continue;

		if (! gth_extension_manager_activate (Main->priv->extension_manager, name, &error)) {
			if (mandatory) {
				g_critical ("Could not load the mandatory extension '%s': %s", name, error->message);
				abort ();
			}
			else
				g_warning ("Could not load the '%s' extension: %s", name, error->message);
			g_clear_error (&error);
		}
	}

	_g_string_list_free (ordered_extensions);
	g_strfreev (actived_extensions);
	g_strfreev (user_actived_extensions);
}


gboolean
gth_main_extension_is_active (const char *extension_name)
{
	if (Main->priv->extension_manager == NULL)
		return FALSE;
	else
		return gth_extension_manager_is_active (Main->priv->extension_manager, extension_name);
}


/* utilities */


gboolean
attribute_list_reload_required (const char *old_attributes,
				const char *new_attributes)
{
	char     **old_attributes_v;
	char     **new_attributes_v;
	int        new_attributes_len;
	int        i;
	gboolean   reload_required;

	if (old_attributes == NULL)
		return TRUE;

	old_attributes_v = g_strsplit (old_attributes, ",", -1);
	new_attributes_v = g_strsplit (new_attributes, ",", -1);
	new_attributes_len = g_strv_length (new_attributes_v);

	for (i = 0; i < new_attributes_len; i++) {
		if (_g_utf8_all_spaces (new_attributes_v[i]) || _g_file_attributes_matches_any (new_attributes_v[i], GIO_ATTRIBUTES)) {
			g_free (new_attributes_v[i]);
			new_attributes_v[i] = NULL;
		}
	}

	for (i = 0; (old_attributes_v[i] != NULL); i++) {
		GthMetadataProvider *provider;
		int                  j;

		provider = gth_main_get_metadata_reader (old_attributes_v[i], "*");
		if (provider == NULL)
			continue;

		for (j = 0; j < new_attributes_len; j++)
			if ((new_attributes_v[j] != NULL)
			    && (new_attributes_v[j][0] != '\0')
			    && (strcmp (new_attributes_v[j], "none") != 0))
			{
				char *attr_v[2];

				attr_v[0] = new_attributes_v[j];
				attr_v[1] = NULL;
				if (gth_metadata_provider_can_read (provider, NULL, "*", attr_v)) {
					g_free (new_attributes_v[j]);
					new_attributes_v[j] = NULL;
				}
			}

		g_object_unref (provider);
	}

	/*
	g_print ("old attributes: %s\n", old_attributes);
	g_print ("new attributes: %s\n", new_attributes);
	g_print ("attributes not available: \n");
	*/

	reload_required = FALSE;
	for (i = 0; ! reload_required && (i < new_attributes_len); i++)
		if ((new_attributes_v[i] != NULL)
		    && (new_attributes_v[i][0] != '\0')
		    && (strcmp (new_attributes_v[i], "none") != 0))
		{
			reload_required = TRUE;
			/* g_print ("\t%s\n", new_attributes_v[i]); */
		}

	/* g_print ("reload required: %d\n", reload_required); */

	g_strfreev (new_attributes_v);
	g_strfreev (old_attributes_v);

	return reload_required;
}
