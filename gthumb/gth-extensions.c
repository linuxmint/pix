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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-error.h"
#include "gth-extensions.h"


#define EXTENSION_SUFFIX ".extension"


/* -- gth_extension --  */


G_DEFINE_TYPE (GthExtension, gth_extension, G_TYPE_OBJECT)


static gboolean
gth_extension_base_open (GthExtension  *self,
			 GError       **error)
{
	g_return_val_if_fail (GTH_IS_EXTENSION (self), FALSE);
	return FALSE;
}


static void
gth_extension_base_close (GthExtension *self)
{
	g_return_if_fail (GTH_IS_EXTENSION (self));
}


static gboolean
gth_extension_base_activate (GthExtension  *self,
			     GError       **error)
{
	self->active = TRUE;
	return TRUE;
}


static gboolean
gth_extension_base_deactivate (GthExtension  *self,
			       GError       **error)
{
	self->active = FALSE;
	return TRUE;
}


static gboolean
gth_extension_base_is_configurable (GthExtension *self)
{
	g_return_val_if_fail (GTH_IS_EXTENSION (self), FALSE);
	return FALSE;
}


static void
gth_extension_base_configure (GthExtension *self,
			      GtkWindow    *parent)
{
	g_return_if_fail (GTH_IS_EXTENSION (self));
	g_return_if_fail (GTK_IS_WINDOW (parent));
}


static void
gth_extension_class_init (GthExtensionClass *klass)
{
	klass->open = gth_extension_base_open;
	klass->close = gth_extension_base_close;
	klass->activate = gth_extension_base_activate;
	klass->deactivate = gth_extension_base_deactivate;
	klass->is_configurable = gth_extension_base_is_configurable;
	klass->configure = gth_extension_base_configure;
}


static void
gth_extension_init (GthExtension *self)
{
	self->initialized = FALSE;
	self->active = FALSE;
}


gboolean
gth_extension_open (GthExtension  *self,
		    GError       **error)
{
	return GTH_EXTENSION_GET_CLASS (self)->open (self, error);
}


void
gth_extension_close (GthExtension *self)
{
	GTH_EXTENSION_GET_CLASS (self)->close (self);
}


gboolean
gth_extension_is_active (GthExtension *self)
{
	g_return_val_if_fail (GTH_IS_EXTENSION (self), FALSE);
	return self->active;
}


gboolean
gth_extension_activate (GthExtension  *self,
	                GError       **error)
{
	return GTH_EXTENSION_GET_CLASS (self)->activate (self, error);
}


gboolean
gth_extension_deactivate (GthExtension  *self,
	                  GError       **error)
{
	return GTH_EXTENSION_GET_CLASS (self)->deactivate (self, error);
}


gboolean
gth_extension_is_configurable (GthExtension *self)
{
	return GTH_EXTENSION_GET_CLASS (self)->is_configurable (self);
}


void
gth_extension_configure (GthExtension *self,
				GtkWindow          *parent)
{
	GTH_EXTENSION_GET_CLASS (self)->configure (self, parent);
}


/* -- gth_extension_module -- */


struct _GthExtensionModulePrivate {
	char    *module_name;
	GModule *module;
};


G_DEFINE_TYPE_WITH_CODE (GthExtensionModule,
			 gth_extension_module,
			 GTH_TYPE_EXTENSION,
			 G_ADD_PRIVATE (GthExtensionModule))


static gboolean
gth_extension_module_real_open (GthExtension  *base,
				GError       **error)
{
	GthExtensionModule *self;
	char               *file_name;

	self = GTH_EXTENSION_MODULE (base);

	if (self->priv->module != NULL)
		g_module_close (self->priv->module);

#ifdef RUN_IN_PLACE
	{
		char *extension_dir;

		extension_dir = g_build_filename (GTHUMB_EXTENSIONS_DIR, self->priv->module_name, NULL);
		file_name = g_module_build_path (extension_dir, self->priv->module_name);
		g_free (extension_dir);
	}
#else
	file_name = g_module_build_path (GTHUMB_EXTENSIONS_DIR, self->priv->module_name);
#endif
	self->priv->module = g_module_open (file_name, G_MODULE_BIND_LAZY);
	g_free (file_name);

	if (self->priv->module == NULL) {
		if (error != NULL)
			*error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, _("Could not open the module “%s”: %s"), self->priv->module_name, g_module_error ());
	}

	return self->priv->module != NULL;
}


static void
gth_extension_module_real_close (GthExtension *base)
{
	GthExtensionModule *self;

	self = GTH_EXTENSION_MODULE (base);

	if (self->priv->module != NULL) {
		g_module_close (self->priv->module);
		self->priv->module = NULL;
	}
}


static char *
get_module_function_name (GthExtensionModule *self,
			  const char         *function_name)
{
	return g_strconcat ("gthumb_extension_", function_name, NULL);
}


static gboolean
gth_extension_module_exec_generic_func (GthExtensionModule   *self,
					const char           *name,
					GError              **error)
{
	char *function_name;
	void (*func) (void);

	function_name = get_module_function_name (self, name);
	if (g_module_symbol (self->priv->module, function_name, (gpointer *)&func))
		func();
	else
		*error = g_error_new_literal (GTH_ERROR, GTH_ERROR_GENERIC, g_module_error ());

	g_free (function_name);

	return (*error == NULL);
}


static gboolean
gth_extension_module_real_activate (GthExtension  *base,
				    GError       **error)
{
	GthExtensionModule *self;
	gboolean            success;

	self = GTH_EXTENSION_MODULE (base);

	if (base->active || base->initialized) {
		base->active = TRUE;
		return TRUE;
	}

	success = gth_extension_module_exec_generic_func (self, "activate", error);
	if (success) {
		base->active = TRUE;
		base->initialized = TRUE;
	}

	return success;
}


static gboolean
gth_extension_module_real_deactivate (GthExtension  *base,
				      GError       **error)
{
	GthExtensionModule *self;
	gboolean            success;

	self = GTH_EXTENSION_MODULE (base);

	if (! base->active)
		return TRUE;

	success = gth_extension_module_exec_generic_func (self, "deactivate", error);
	if (success)
		base->active = FALSE;

	return success;
}


static gboolean
gth_extension_module_real_is_configurable (GthExtension *base)
{
	GthExtensionModule *self;
	char               *function_name;
	gboolean            result = FALSE;
	gboolean (*is_configurable_func) (void);

	g_return_val_if_fail (GTH_IS_EXTENSION_MODULE (base), FALSE);

	self = GTH_EXTENSION_MODULE (base);

	function_name = get_module_function_name (self, "is_configurable");
	if (g_module_symbol (self->priv->module, function_name, (gpointer *)&is_configurable_func))
		result = is_configurable_func();
	g_free (function_name);

	return result;
}


static void
gth_extension_module_real_configure (GthExtension *base,
				     GtkWindow    *parent)
{
	GthExtensionModule *self;
	char               *function_name;
	void (*configure_func) (GtkWindow *);

	g_return_if_fail (GTK_IS_WINDOW (parent));

	self = GTH_EXTENSION_MODULE (base);

	function_name = get_module_function_name (self, "configure");
	if (g_module_symbol (self->priv->module, function_name, (gpointer *)&configure_func))
		configure_func (parent);

	g_free (function_name);
}


static void
gth_extension_module_finalize (GObject *obj)
{
	GthExtensionModule *self;

	self = GTH_EXTENSION_MODULE (obj);

	if (self->priv->module != NULL)
		g_module_close (self->priv->module);
	g_free (self->priv->module_name);

	G_OBJECT_CLASS (gth_extension_module_parent_class)->finalize (obj);
}


static void
gth_extension_module_class_init (GthExtensionModuleClass *klass)
{
	GthExtensionClass *elc;

	G_OBJECT_CLASS (klass)->finalize = gth_extension_module_finalize;

	elc = GTH_EXTENSION_CLASS (klass);
	elc->open = gth_extension_module_real_open;
	elc->close = gth_extension_module_real_close;
	elc->activate = gth_extension_module_real_activate;
	elc->deactivate = gth_extension_module_real_deactivate;
	elc->is_configurable = gth_extension_module_real_is_configurable;
	elc->configure = gth_extension_module_real_configure;
}


static void
gth_extension_module_init (GthExtensionModule *self)
{
	self->priv = gth_extension_module_get_instance_private (self);
}


GthExtension *
gth_extension_module_new (const char *module_name)
{
	GthExtension *loader;

	loader = g_object_new (GTH_TYPE_EXTENSION_MODULE, NULL);
	GTH_EXTENSION_MODULE (loader)->priv->module_name = g_strdup (module_name);

	return loader;
}


/* -- gth_extension_description -- */


struct _GthExtensionDescriptionPrivate {
	gboolean      opened;
	GthExtension *extension;
};


G_DEFINE_TYPE_WITH_CODE (GthExtensionDescription,
			 gth_extension_description,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthExtensionDescription))


static void
gth_extension_description_finalize (GObject *obj)
{
	GthExtensionDescription *self;

	self = GTH_EXTENSION_DESCRIPTION (obj);

	g_free (self->id);
	g_free (self->name);
	g_free (self->description);
	g_strfreev (self->authors);
	g_free (self->copyright);
	g_free (self->version);
	g_free (self->icon_name);
	g_free (self->url);
	g_free (self->category);
	g_free (self->loader_type);
	g_strfreev (self->loader_requires);
	g_strfreev (self->loader_after);
	_g_object_unref (self->priv->extension);

	G_OBJECT_CLASS (gth_extension_description_parent_class)->finalize (obj);
}


static void
gth_extension_description_class_init (GthExtensionDescriptionClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_extension_description_finalize;
}


static void
gth_extension_description_init (GthExtensionDescription *self)
{
	self->priv = gth_extension_description_get_instance_private (self);
	self->priv->opened = FALSE;
	self->priv->extension = NULL;
}


static gboolean
gth_extension_description_load_from_file (GthExtensionDescription *desc,
					  GFile                   *file)
{
	GKeyFile *key_file;
	char     *file_path;
	char     *api;
	char     *basename;

	key_file = g_key_file_new ();
	file_path = g_file_get_path (file);
	if (! g_key_file_load_from_file (key_file, file_path, G_KEY_FILE_NONE, NULL)) {
		g_free (file_path);
		g_key_file_free (key_file);
		return FALSE;
	}

	api = g_key_file_get_string (key_file, "Loader", "API", NULL);
	if (g_strcmp0 (api, GTHUMB_API_VERSION) != 0) {
		g_free (api);
		g_free (file_path);
		g_key_file_free (key_file);
		return FALSE;
	}

	basename = g_file_get_basename (file);
	desc->id = _g_path_remove_extension (basename);
	desc->name = g_key_file_get_locale_string (key_file, "Extension", "Name", NULL, NULL);
	desc->description = g_key_file_get_locale_string (key_file, "Extension", "Comment", NULL, NULL);
	if (desc->description == NULL)
		desc->description = g_key_file_get_locale_string (key_file, "Extension", "Description", NULL, NULL);
	desc->version = g_key_file_get_locale_string (key_file, "Extension", "Version", NULL, NULL);
	desc->authors = g_key_file_get_locale_string_list (key_file, "Extension", "Authors", NULL, NULL, NULL);
	desc->copyright = g_key_file_get_locale_string (key_file, "Extension", "Copyright", NULL, NULL);
	desc->icon_name = g_key_file_get_string (key_file, "Extension", "Icon", NULL);
	desc->url = g_key_file_get_string (key_file, "Extension", "URL", NULL);
	desc->category = g_key_file_get_string (key_file, "Extension", "Category", NULL);
	desc->mandatory = g_key_file_get_boolean (key_file, "Extension", "Mandatory", NULL);
	desc->hidden = g_key_file_get_boolean (key_file, "Extension", "Hidden", NULL);
	desc->loader_type = g_key_file_get_string (key_file, "Loader", "Type", NULL);
	desc->loader_requires = g_key_file_get_string_list (key_file, "Loader", "Requires", NULL, NULL);
	desc->loader_after = g_key_file_get_string_list (key_file, "Loader", "After", NULL, NULL);

	g_free (basename);
	g_free (api);
	g_free (file_path);
	g_key_file_free (key_file);

	return TRUE;
}


GthExtensionDescription *
gth_extension_description_new (GFile *file)
{
	GthExtensionDescription *desc;

	desc = g_object_new (GTH_TYPE_EXTENSION_DESCRIPTION, NULL);
	if (! gth_extension_description_load_from_file (desc, file)) {
		g_object_unref (desc);
		desc = NULL;
	}

	return desc;
}


gboolean
gth_extension_description_is_active (GthExtensionDescription *desc)
{
	return (desc->priv->extension != NULL) && desc->priv->extension->active;
}


GthExtension *
gth_extension_description_get_extension (GthExtensionDescription *desc)
{
	return desc->priv->extension;
}


/* -- gth_extension_manager --  */


struct _GthExtensionManagerPrivate {
	GHashTable *extensions;
};


G_DEFINE_TYPE_WITH_CODE (GthExtensionManager,
			 gth_extension_manager,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthExtensionManager))


static void
gth_extension_manager_finalize (GObject *obj)
{
	GthExtensionManager *self;

	self = GTH_EXTENSION_MANAGER (obj);

	if (self->priv->extensions != NULL)
		g_hash_table_destroy (self->priv->extensions);

	G_OBJECT_CLASS (gth_extension_manager_parent_class)->finalize (obj);
}


static void
gth_extension_manager_class_init (GthExtensionManagerClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_extension_manager_finalize;
}


static void
gth_extension_manager_init (GthExtensionManager *self)
{
	self->priv = gth_extension_manager_get_instance_private (self);
	self->priv->extensions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}


static void
gth_extension_manager_load_extensions (GthExtensionManager *self)
{
	GFile           *extensions_dir;
	GFileEnumerator *enumerator;
	GFileInfo       *info;

	g_return_if_fail (GTH_IS_EXTENSION_MANAGER (self));

#ifdef G_PLATFORM_WIN32
	{
		char *path;

		path = g_getenv ("PATH");
		path = g_strconcat (path, G_SEARCHPATH_SEPARATOR_S GTHUMB_EXTENSIONS_DIR, NULL);
		g_setenv ("PATH", path, TRUE);
		g_free (path);
	}
#endif

	extensions_dir = g_file_new_for_path (GTHUMB_EXTENSIONS_DIR);
	enumerator = g_file_enumerate_children (extensions_dir, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, NULL);
	if (enumerator == NULL) {
		g_critical ("Could not find the extensions folder: %s", g_file_get_uri (extensions_dir));
		abort ();
	}

	while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL) {
		const char              *name;
		GFile                   *ext_file;
		char                    *ext_name;
		GthExtensionDescription *ext_desc;

		name = g_file_info_get_name (info);

#ifdef RUN_IN_PLACE
	{
		char *basename;
		char *path;

		basename = g_strconcat (name, EXTENSION_SUFFIX, NULL);
		path = g_build_filename (GTHUMB_EXTENSIONS_DIR, name, basename, NULL);
		ext_file = g_file_new_for_path (path);
		ext_name = g_strdup (name);

		g_free (path);
		g_free (basename);
	}
#else
		if (! g_str_has_suffix (name, EXTENSION_SUFFIX))
			continue;
		ext_file = g_file_get_child (extensions_dir, name);
		ext_name = _g_str_remove_suffix (name, EXTENSION_SUFFIX);
#endif

		ext_desc = gth_extension_description_new (ext_file);
		if (ext_desc != NULL)
			g_hash_table_insert (self->priv->extensions, g_strdup (ext_name), ext_desc);

		g_free (ext_name);
		g_object_unref (ext_file);
		g_object_unref (info);
	}

	g_object_unref (enumerator);
	g_object_unref (extensions_dir);
}


GthExtensionManager *
gth_extension_manager_new (void)
{
	GthExtensionManager *manager;

	manager = (GthExtensionManager *) g_object_new (GTH_TYPE_EXTENSION_MANAGER, NULL);
	gth_extension_manager_load_extensions (manager);

	return manager;
}


gboolean
gth_extension_manager_open (GthExtensionManager  *manager,
			    const char           *extension_name,
			    GError              **error)
{
	GthExtensionDescription *description;

	description = g_hash_table_lookup (manager->priv->extensions, extension_name);
	if (description == NULL) {
		*error = g_error_new_literal (GTH_ERROR, GTH_ERROR_EXTENSION_DEPENDENCY, "Extension not found");
		return FALSE;
	}

	if (description->priv->opened)
		return TRUE;

	g_return_val_if_fail (description->loader_type != NULL, FALSE);

	if (strcmp (description->loader_type, "module") == 0)
		description->priv->extension = gth_extension_module_new (extension_name);

	g_return_val_if_fail (description->priv->extension != NULL, FALSE);

	description->priv->opened = gth_extension_open (description->priv->extension, error);

	if (! description->priv->opened) {
		g_object_unref (description->priv->extension);
		description->priv->extension = NULL;
	}

	return description->priv->opened;
}


gboolean
gth_extension_manager_activate (GthExtensionManager  *manager,
				const char           *extension_name,
				GError              **error)
{
	GthExtensionDescription *description;

	if (! gth_extension_manager_open (manager, extension_name, error))
		return FALSE;

	description = g_hash_table_lookup (manager->priv->extensions, extension_name);
	if (description->loader_requires != NULL) {
		int i;

		for (i = 0; description->loader_requires[i] != NULL; i++)
			if (! gth_extension_manager_activate (manager, description->loader_requires[i], error))
				return FALSE;
	}

	return gth_extension_activate (description->priv->extension, error);
}


static GList *
get_extension_dependencies (GthExtensionManager     *manager,
			    GthExtensionDescription *description)
{
	GList *dependencies = NULL;
	GList *names;
	GList *scan;

	names = g_hash_table_get_keys (manager->priv->extensions);
	for (scan = names; scan; scan = scan->next) {
		const char              *extension_name = scan->data;
		GthExtensionDescription *other_description;

		other_description = g_hash_table_lookup (manager->priv->extensions, extension_name);
		if (other_description->loader_requires != NULL) {
			int i;

			for (i = 0; other_description->loader_requires[i] != NULL; i++)
				if (strcmp (description->id, other_description->loader_requires[i]) == 0)
					dependencies = g_list_prepend (dependencies, other_description);
		}
	}

	g_list_free (names);

	return g_list_reverse (dependencies);
}


gboolean
gth_extension_manager_deactivate (GthExtensionManager  *manager,
				  const char           *extension_name,
				  GError              **error)
{
	GthExtensionDescription *description;
	GList                   *required_by;
	GList                   *scan;

	if (! gth_extension_manager_open (manager, extension_name, error))
		return TRUE;

	description = g_hash_table_lookup (manager->priv->extensions, extension_name);
	if (! gth_extension_description_is_active (description))
		return TRUE;

	required_by = get_extension_dependencies (manager, description);
	for (scan = required_by; scan; scan = scan->next) {
		GthExtensionDescription *child_description = scan->data;

		if (gth_extension_description_is_active (child_description)) {
			*error = g_error_new (GTH_ERROR, GTH_ERROR_EXTENSION_DEPENDENCY, _("The extension “%1$s” is required by the extension “%2$s”"), description->name, child_description->name);
			break;
		}
	}

	g_list_free (required_by);

	if (*error == NULL)
		return gth_extension_deactivate (description->priv->extension, error);
	else
		return FALSE;
}


gboolean
gth_extension_manager_is_active (GthExtensionManager *manager,
				 const char          *extension_name)
{
	GthExtensionDescription *description;

	description = g_hash_table_lookup (manager->priv->extensions, extension_name);
	if (description != NULL)
		return gth_extension_description_is_active (description);
	else
		return FALSE;
}


GList *
gth_extension_manager_get_extensions (GthExtensionManager *manager)
{
	return g_hash_table_get_keys (manager->priv->extensions);
}


GthExtensionDescription *
gth_extension_manager_get_description (GthExtensionManager *manager,
				       const char          *extension_name)
{
	return g_hash_table_lookup (manager->priv->extensions, extension_name);
}


static GList *
get_extension_optional_dependencies (GthExtensionManager     *manager,
				     GthExtensionDescription *description)
{
	GList *dependencies = NULL;
	int    i;

	if (description->loader_after == NULL)
		return NULL;

	for (i = 0; description->loader_after[i] != NULL; i++) {
		char                    *extension_name = description->loader_after[i];
		GthExtensionDescription *other_description;

		dependencies = g_list_prepend (dependencies, extension_name);

		other_description = g_hash_table_lookup (manager->priv->extensions, extension_name);
		if (other_description != NULL)
			dependencies = g_list_concat (get_extension_optional_dependencies (manager, other_description), dependencies);
	}

	return dependencies;
}


GList *
gth_extension_manager_order_extensions (GthExtensionManager  *manager,
					char                **extensions)
{
	GList *extension_list;
	int    i;
	GList *ordered_by_dependency;
	GList *scan;

	extension_list = NULL;
	for (i = 0; extensions[i] != NULL; i++)
		if (g_hash_table_lookup (manager->priv->extensions, extensions[i]) != NULL)
			extension_list = g_list_prepend (extension_list, g_strdup (extensions[i]));
	extension_list = g_list_reverse (extension_list);

	ordered_by_dependency = NULL;
	for (scan = extension_list; scan; /* void */) {
		GList                   *next;
		char                    *ext_name;
		GthExtensionDescription *ext_description;
		GList                   *dependencies;
		GList                   *scan_dep;

		next = extension_list->next;
		ext_name = extension_list->data;
		ext_description = g_hash_table_lookup (manager->priv->extensions, ext_name);
		g_assert (ext_description != NULL);

		dependencies = get_extension_optional_dependencies (manager, ext_description);
		for (scan_dep = dependencies; scan_dep; scan_dep = scan_dep->next) {
			char  *dep_name = scan_dep->data;
			GList *link;

			link = g_list_find_custom (extension_list, dep_name, (GCompareFunc) strcmp);
			if (link != NULL) {
				if (link == next)
					next = next->next;

				/* prepend the extension dependency to the ordered list */
				extension_list = g_list_remove_link (extension_list, link);
				ordered_by_dependency = _g_list_prepend_link (ordered_by_dependency, link);
			}
		}
		g_list_free (dependencies);

		/* prepend the extension to the ordered list */
		extension_list = g_list_remove_link (extension_list, scan);
		ordered_by_dependency = _g_list_prepend_link (ordered_by_dependency, scan);

		scan = next;
	}

	return g_list_reverse (ordered_by_dependency);
}
