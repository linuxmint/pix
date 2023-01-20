/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2019 Free Software Foundation, Inc.
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
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "dlg-personalize-filters.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-auto-paned.h"
#include "gth-browser.h"
#include "gth-browser-actions-callbacks.h"
#include "gth-browser-actions-entries.h"
#include "gth-duplicable.h"
#include "gth-enum-types.h"
#include "gth-error.h"
#include "gth-file-list.h"
#include "gth-file-view.h"
#include "gth-file-selection.h"
#include "gth-file-source-vfs.h"
#include "gth-file-tool.h"
#include "gth-filter.h"
#include "gth-filterbar.h"
#include "gth-folder-tree.h"
#include "gth-grid-view.h"
#include "gth-icon-cache.h"
#include "gth-info-bar.h"
#include "gth-image-preloader.h"
#include "gth-location-bar.h"
#include "gth-location-chooser.h"
#include "gth-main.h"
#include "gth-marshal.h"
#include "gth-metadata-provider.h"
#include "gth-paned.h"
#include "gth-preferences.h"
#include "gth-progress-dialog.h"
#include "gth-sidebar.h"
#include "gth-statusbar.h"
#include "gth-toolbox.h"
#include "gth-user-dir.h"
#include "gth-viewer-page.h"
#include "gth-window.h"
#include "main.h"

#define GTH_BROWSER_CALLBACK(f) ((GthBrowserCallback) (f))
#define MAX_HISTORY_LENGTH 15
#define LOAD_FILE_DELAY 150
#define LOAD_METADATA_DELAY 150
#define HIDE_MOUSE_DELAY 1 /* in seconds */
#define MOTION_THRESHOLD 0
#define UPDATE_SELECTION_DELAY 200
#define MIN_SIDEBAR_SIZE 100
#define MIN_VIEWER_SIZE 256
#define STATUSBAR_SEPARATOR "  ·  "
#define FILE_PROPERTIES_MINIMUM_HEIGHT 100
#define HISTORY_FILE "history.xbel"
#define OVERLAY_MARGIN 10
#define AUTO_OPEN_FOLDER_DELAY 500


enum {
	LOCATION_READY,
	LAST_SIGNAL
};


typedef struct {
	gboolean        saved;
	GthBrowserPage  page;
	GFile          *location;
	GFile          *current_file;
	GList          *selected;
	double          vscroll;
} BrowserState;


struct _GthBrowserPrivate {
	/* UI staff */

	GtkWidget         *infobar;
	GtkWidget         *statusbar;
	GtkWidget         *browser_right_container;
	GtkWidget         *browser_left_container;
	GtkWidget         *browser_sidebar;
	GtkWidget         *folder_tree;
	GtkWidget         *folder_popup;
	GtkWidget         *file_list_popup;
	GtkWidget         *file_popup;
	GtkWidget         *filterbar;
	GtkWidget         *file_list;
	GtkWidget         *list_extra_widget_container;
	GtkWidget         *list_info_bar;
	GtkWidget         *location_bar;
	GtkWidget         *file_properties;
	GtkWidget         *header_sections[GTH_BROWSER_N_HEADER_SECTIONS];
	GtkWidget         *browser_status_commands;
	GtkWidget         *viewer_status_commands;
	GtkWidget         *show_progress_dialog_button;
	GtkWidget         *menu_button;
	GHashTable	  *menu_managers;

	GtkWidget         *thumbnail_list;

	GList             *viewer_pages;
	GtkOrientation     viewer_thumbnails_orientation;
	GtkWidget         *viewer_thumbnails_pane;
	GtkWidget         *viewer_sidebar_pane;
	GtkWidget         *viewer_sidebar_container;
	GtkWidget         *viewer_container;
	GthViewerPage     *viewer_page;
	GthImagePreloader *image_preloader;

	GtkWidget         *progress_dialog;

	GHashTable        *named_dialogs;
	GtkGesture        *gesture;

	/* Browser data */

	gulong             folder_changed_id;
	gulong             file_renamed_id;
	gulong             metadata_changed_id;
	gulong             emblems_changed_id;
	gulong             entry_points_changed_id;
	gulong             order_changed_id;
	gulong             shortcuts_changed_id;
	gulong             filters_changed_id;
	GthFileData       *location;
	GthFileData       *current_file;
	GthFileSource     *location_source;
	int                n_visibles;
	int                current_file_position;
	GFile             *monitor_location;
	gboolean           activity_ref;
	GthIconCache      *menu_icon_cache;
	GthFileDataSort   *current_sort_type;
	gboolean           current_sort_inverse;
	GthFileDataSort   *default_sort_type;
	gboolean           default_sort_inverse;
	gboolean           show_hidden_files;
	gboolean           fast_file_type;
	gboolean           closing;
	GthTask           *task;
	GthTask           *next_task;
	gulong             task_completed;
	gulong             task_progress;
	GList             *background_tasks;
	GList             *viewer_tasks;
	gboolean           close_with_task;
	GList             *load_data_queue;
	gpointer           last_folder_to_open;
	GList             *load_file_data_queue;
	guint              load_file_timeout;
	guint              load_metadata_timeout;
	char              *list_attributes;
	gboolean           constructed;
	guint              construct_step2_event;
	guint              selection_changed_event;
	GthFileData       *folder_popup_file_data;
	gboolean           properties_on_screen;
	char              *location_free_space;
	gboolean           recalc_location_free_space;
	gboolean           file_properties_on_the_right;
	GthSidebarState    viewer_sidebar;
	BrowserState       state;
	GthICCProfile     *screen_profile;
	GtkTreePath       *folder_tree_last_dest_row; /* used to open a folder during D&D */
	guint              folder_tree_open_folder_id;
	GtkWidget         *apply_editor_changes_button;
	gboolean           ask_to_save_modified_images;
	GthScrollAction    scroll_action;

	/* settings */

	GSettings         *browser_settings;
	GSettings         *messages_settings;
	GSettings         *desktop_interface_settings;

	/* fullscreen */

	gboolean           fullscreen;
	gboolean	   was_fullscreen;
	GtkWidget	  *fullscreen_toolbar;
	GtkWidget	  *fullscreen_headerbar;
	GtkWidget         *next_image_button;
	GtkWidget         *previous_image_button;
	GList             *viewer_controls;
	GList             *fixed_viewer_controls;
	gboolean           pointer_visible;
	guint              hide_mouse_timeout;
	guint              motion_signal;
	gdouble            last_mouse_x;
	gdouble            last_mouse_y;
	gboolean           view_files_in_fullscreen;
	gboolean           keep_mouse_visible;
	struct {
		int		page;
		gboolean	thumbnail_list;
		gboolean	browser_properties;
		GthSidebarState	viewer_sidebar;
	} before_fullscreen;
	struct {
		gboolean	thumbnail_list;
		GthSidebarState	sidebar;
	} fullscreen_state;

	/* history */

	GList             *history;
	GList             *history_current;
	GMenu             *history_menu;
};

static guint gth_browser_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthBrowser,
			 gth_browser,
			 GTH_TYPE_WINDOW,
			 G_ADD_PRIVATE (GthBrowser))


/* -- browser_state -- */


static void
browser_state_init (BrowserState *state)
{
	state->saved = FALSE;
	state->page = 0;
	state->location = NULL;
	state->current_file = NULL;
	state->selected = NULL;
}


static void
browser_state_free (BrowserState *state)
{
	if (! state->saved)
		return;

	_g_object_unref (state->location);
	_g_object_unref (state->current_file);
	g_list_foreach (state->selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (state->selected);

	state->location = NULL;
	state->current_file = NULL;
	state->selected = NULL;
	state->saved = FALSE;
}


/* -- monitor_event_data -- */


typedef struct {
	int              ref;
	GthFileSource   *file_source;
	GFile           *parent;
	int              position;
	GthMonitorEvent  event;
	GthBrowser      *browser;
	gboolean         update_file_list;
	gboolean         update_folder_tree;
} MonitorEventData;


static MonitorEventData *
monitor_event_data_new (void)
{
	MonitorEventData *monitor_data;

	monitor_data = g_new0 (MonitorEventData, 1);
	monitor_data->ref = 1;

	return monitor_data;
}


G_GNUC_UNUSED
static MonitorEventData *
monitor_event_data_ref (MonitorEventData *monitor_data)
{
	monitor_data->ref++;
	return monitor_data;
}


static void
monitor_event_data_unref (MonitorEventData *monitor_data)
{
	monitor_data->ref--;

	if (monitor_data->ref > 0)
		return;

	g_object_unref (monitor_data->file_source);
	g_object_unref (monitor_data->parent);
	g_free (monitor_data);
}


/* -- gth_browser -- */


static gboolean
gth_action_changes_folder (GthAction action)
{
	gboolean result = FALSE;

	switch (action) {
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
	case GTH_ACTION_GO_UP:
	case GTH_ACTION_TREE_OPEN:
		result = TRUE;
		break;

	case GTH_ACTION_TREE_LIST_CHILDREN:
		result = FALSE;
		break;
	}

	return result;
}


static void
_gth_browser_update_current_file_position (GthBrowser *browser)
{
	GthFileStore *file_store;

	file_store = gth_browser_get_file_store (browser);
	browser->priv->n_visibles = gth_file_store_n_visibles (file_store);
	browser->priv->current_file_position = -1;

	if (browser->priv->current_file != NULL) {
		int pos;

		pos = gth_file_store_get_pos (file_store, browser->priv->current_file->file);
		if (pos >= 0)
			browser->priv->current_file_position = pos;
	}
}


static gboolean
_gth_browser_file_tool_is_active (GthBrowser *browser)
{
	return  gth_toolbox_tool_is_active (GTH_TOOLBOX (gth_sidebar_get_toolbox (GTH_SIDEBAR (browser->priv->file_properties))));
}


void
gth_browser_update_title (GthBrowser *browser)
{
	GString    *title;
	const char *name = NULL;
	GList      *emblems = NULL;

	title = g_string_new (NULL);

	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		if (browser->priv->location != NULL)
			name = g_file_info_get_display_name (browser->priv->location->info);
		if (name != NULL)
			g_string_append (title, name);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		if (_gth_browser_file_tool_is_active (browser)) {
			GtkWidget *toolbox;
			GtkWidget *file_tool;

			toolbox = gth_sidebar_get_toolbox (GTH_SIDEBAR (browser->priv->file_properties));
			file_tool = gth_toolbox_get_active_tool (GTH_TOOLBOX (toolbox));
			g_string_append (title, gth_file_tool_get_options_title (GTH_FILE_TOOL (file_tool)));
		}
		else {
			if (browser->priv->current_file != NULL)
				name = g_file_info_get_display_name (browser->priv->current_file->info);
			if (name != NULL)
				g_string_append (title, name);

			if (gth_browser_get_file_modified (browser)) {
				g_string_append (title, " ");
				g_string_append (title, _("[modified]"));
			}

			if (browser->priv->current_file != NULL) {
				GthStringList *string_list;

				string_list = GTH_STRING_LIST (g_file_info_get_attribute_object (browser->priv->current_file->info, GTH_FILE_ATTRIBUTE_EMBLEMS));
				if (string_list != NULL)
					emblems = _g_string_list_dup (gth_string_list_get_list (string_list));
			}
		}
		break;
	}

	if (title->len == 0)
		g_string_append (title, _("gThumb"));

	gth_window_set_title (GTH_WINDOW (browser),
			      title->str,
			      emblems);

	g_string_free (title, TRUE);
	_g_string_list_free (emblems);
}


void
gth_browser_update_sensitivity (GthBrowser *browser)
{
	GFile    *parent;
	gboolean  parent_available;
	gboolean  viewer_can_save;
	gboolean  modified;
	int       n_selected;

	if (browser->priv->location != NULL)
		parent = g_file_get_parent (browser->priv->location->file);
	else
		parent = NULL;
	parent_available = (parent != NULL);
	_g_object_unref (parent);

	viewer_can_save = (browser->priv->location != NULL) && (browser->priv->viewer_page != NULL) && gth_viewer_page_can_save (GTH_VIEWER_PAGE (browser->priv->viewer_page));
	modified = gth_browser_get_file_modified (browser);
	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	gth_window_enable_action (GTH_WINDOW (browser), "show-thumbnail-list", gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER);
	gth_window_enable_action (GTH_WINDOW (browser), "show-sidebar", gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER);
	/* gth_window_enable_action (GTH_WINDOW (browser), "view-reload", gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER);*/ /* FIXME add view-reload */
	gth_window_enable_action (GTH_WINDOW (browser), "save", viewer_can_save && modified);
	gth_window_enable_action (GTH_WINDOW (browser), "save-as", viewer_can_save);
	gth_window_enable_action (GTH_WINDOW (browser), "revert-to-saved", viewer_can_save && modified);
	gth_window_enable_action (GTH_WINDOW (browser), "clear-history", browser->priv->history != NULL);
	gth_window_enable_action (GTH_WINDOW (browser), "go-up", parent_available);
	gth_window_enable_action (GTH_WINDOW (browser), "browser-edit-file", n_selected == 1);

	gth_sidebar_update_sensitivity (GTH_SIDEBAR (browser->priv->file_properties));

	if (browser->priv->viewer_page != NULL)
		gth_viewer_page_update_sensitivity (browser->priv->viewer_page);

	gth_hook_invoke ("gth-browser-update-sensitivity", browser);
}


void
gth_browser_update_extra_widget (GthBrowser *browser)
{
	GtkWidget *location_chooser;

	gtk_widget_show (browser->priv->location_bar);
	_gtk_container_remove_children (GTK_CONTAINER (gth_location_bar_get_action_area (GTH_LOCATION_BAR (browser->priv->location_bar))), NULL, NULL);

	location_chooser = gth_location_bar_get_chooser (GTH_LOCATION_BAR (browser->priv->location_bar));
	_g_signal_handlers_block_by_data (location_chooser, browser);
	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (location_chooser), browser->priv->location->file);
	_g_signal_handlers_unblock_by_data (location_chooser, browser);

	gth_hook_invoke ("gth-browser-update-extra-widget", browser);
}


static void
_gth_browser_set_sort_order (GthBrowser      *browser,
			     GthFileDataSort *sort_type,
			     gboolean         inverse,
			     gboolean         save,
			     gboolean         update_view);


static void
_gth_browser_set_location (GthBrowser  *browser,
			   GthFileData *location)
{
	GthFileDataSort *sort_type;
	gboolean         sort_inverse;

	if (location == NULL)
		return;

	if (location != browser->priv->location) {
		if (browser->priv->location != NULL)
			g_object_unref (browser->priv->location);
		browser->priv->location = gth_file_data_dup (location);
	}

	sort_type = gth_main_get_sort_type (g_file_info_get_attribute_string (browser->priv->location->info, "sort::type"));
	sort_inverse = g_file_info_get_attribute_boolean (browser->priv->location->info, "sort::inverse");
	if (sort_type == NULL) {
		sort_type = browser->priv->default_sort_type;
		sort_inverse = browser->priv->default_sort_inverse;
	}
	_gth_browser_set_sort_order (browser,
				     sort_type,
				     sort_inverse,
				     FALSE,
				     FALSE);
}


static void
_gth_browser_update_location (GthBrowser  *browser,
			      GthFileData *location)
{
	_gth_browser_set_location (browser, location);

	_gth_browser_update_current_file_position (browser);
	gth_browser_update_title (browser);
	gth_browser_update_sensitivity (browser);
	gth_browser_update_extra_widget (browser);
}


static void
_gth_browser_update_go_sensitivity (GthBrowser *browser)
{
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "go-back"),
		      "enabled",
		      (browser->priv->history_current != NULL) && (browser->priv->history_current->next != NULL),
		      NULL);
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "go-forward"),
		      "enabled",
		      (browser->priv->history_current != NULL) && (browser->priv->history_current->prev != NULL),
		      NULL);
}


#if 0
static void
_gth_browser_history_print (GthBrowser *browser)
{
	GList *scan;

	g_print ("history:\n");
	for (scan = browser->priv->history; scan; scan = scan->next) {
		GFile *file = scan->data;
		char  *uri;

		uri = g_file_get_uri (file);
		g_print (" %s%s\n", (browser->priv->history_current == scan) ? "*" : " ", uri);

		g_free (uri);
	}
}
#endif


static void
_gth_browser_history_menu (GthBrowser *browser)
{
	_gth_browser_update_go_sensitivity (browser);

	/* Update the history menu model for the headerbar button */

	g_menu_remove_all (browser->priv->history_menu);

	if (browser->priv->history != NULL) {
		GList *scan;
		int    i;

		for (i = 0, scan = browser->priv->history;
		     scan;
		     scan = scan->next, i++)
		{
			GFile     *file = scan->data;
			GMenuItem *item;
			char      *target;

			item = _g_menu_item_new_for_file (file, NULL);
			target = g_strdup_printf ("%d", i);
			g_menu_item_set_action_and_target (item, "win.go-to-history-position", "s", target);
			g_menu_append_item (browser->priv->history_menu, item);

			if (browser->priv->history_current == scan) {
				GAction *action = g_action_map_lookup_action (G_ACTION_MAP (browser), "go-to-history-position");
				g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_string (target));
			}

			g_free (target);
			g_object_unref (item);
		}
	}
}


static void
_gth_browser_history_add (GthBrowser *browser,
			  GFile      *file)
{
	if (file == NULL)
		return;

	if ((browser->priv->history_current == NULL)
		|| ! _g_file_equal (file, browser->priv->history_current->data))
	{
		GList *scan;

		/* remove all files after the current position */
		for (scan = browser->priv->history; scan && (scan != browser->priv->history_current); /* void */) {
			GList *next = scan->next;

			browser->priv->history = g_list_remove_link (browser->priv->history, scan);
			_g_object_list_unref (scan);

			scan = next;
		}

		browser->priv->history = g_list_prepend (browser->priv->history, g_object_ref (file));
		browser->priv->history_current = browser->priv->history;
	}
}


static void
_gth_browser_history_save (GthBrowser *browser)
{
	GSettings     *privacy_settings;
	gboolean       save_history;
	GBookmarkFile *bookmarks;
	GFile         *file;
	char          *filename;
	GList         *scan;
	int            n;

	privacy_settings = _g_settings_new_if_schema_installed ("org.gnome.desktop.privacy");
        save_history = (privacy_settings == NULL) || g_settings_get_boolean (privacy_settings, "remember-recent-files");
        _g_object_unref (privacy_settings);

	if (! save_history) {
		file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, HISTORY_FILE, NULL);
		g_file_delete (file, NULL, NULL);
		g_object_unref (file);
		return;
	}

	bookmarks = g_bookmark_file_new ();
	for (scan = browser->priv->history, n = 0; scan && (n < MAX_HISTORY_LENGTH); scan = scan->next, n++) {
		GFile *location = scan->data;
		char  *uri;

		uri = g_file_get_uri (location);
		_g_bookmark_file_add_uri (bookmarks, uri);

		g_free (uri);
	}
	file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, HISTORY_FILE, NULL);
	filename = g_file_get_path (file);
	g_bookmark_file_to_file (bookmarks, filename, NULL);

	g_free (filename);
	g_object_unref (file);
	g_bookmark_file_free (bookmarks);
}


static void
_gth_browser_history_load (GthBrowser *browser)
{
	GSettings     *privacy_settings;
	gboolean       load_history;
	GBookmarkFile *bookmarks;
	GFile         *file;
	char          *filename;

	_g_object_list_unref (browser->priv->history);
	browser->priv->history = NULL;
	browser->priv->history_current = NULL;

	privacy_settings = _g_settings_new_if_schema_installed ("org.gnome.desktop.privacy");
	load_history = (privacy_settings == NULL) || g_settings_get_boolean (privacy_settings, "remember-recent-files");
	_g_object_unref (privacy_settings);

	if (! load_history)
		return;

	bookmarks = g_bookmark_file_new ();
	file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, HISTORY_FILE, NULL);
	filename = g_file_get_path (file);
	if (g_bookmark_file_load_from_file (bookmarks, filename, NULL)) {
		char **uris;
		int    i;

		uris = g_bookmark_file_get_uris (bookmarks, NULL);
		for (i = 0; (uris[i] != NULL) && (i < MAX_HISTORY_LENGTH); i++)
			browser->priv->history = g_list_prepend (browser->priv->history, g_file_new_for_uri (uris[i]));
		browser->priv->history = g_list_reverse (browser->priv->history);

		g_strfreev (uris);
	}

	browser->priv->history_current = browser->priv->history;

	g_free (filename);
	g_object_unref (file);
	g_bookmark_file_free (bookmarks);
}


static void
_gth_browser_monitor_entry_points (GthBrowser *browser)
{
	GList *scan;

	for (scan = gth_main_get_all_file_sources (); scan; scan = scan->next) {
		GthFileSource *file_source = scan->data;
		gth_file_source_monitor_entry_points (file_source);
	}
}


static void
_gth_browser_update_entry_point_list (GthBrowser *browser)
{
	GList *entry_points;
	GFile *root;

	entry_points = gth_main_get_all_entry_points ();
	root = g_file_new_for_uri ("gthumb-vfs:///");
	gth_folder_tree_set_children (GTH_FOLDER_TREE (browser->priv->folder_tree), root, entry_points);

	g_object_unref (root);
	_g_object_list_unref (entry_points);
}


static GthTest *
_gth_browser_get_file_filter (GthBrowser *browser)
{
	GthTest *filterbar_test;
	GthTest *test;

	if (browser->priv->filterbar == NULL)
		return NULL;

	filterbar_test = gth_filterbar_get_test (GTH_FILTERBAR (browser->priv->filterbar));
	test = gth_main_add_general_filter (filterbar_test);

	_g_object_unref (filterbar_test);

	return test;
}


static gboolean
_gth_browser_get_fast_file_type (GthBrowser *browser,
				 GFile      *file)
{
	gboolean fast_file_type;

	fast_file_type = browser->priv->fast_file_type;

	/* Force the value to FALSE when browsing a cache or a temporary files
	 * directory.
	 * This is mainly used to browse the Firefox cache without changing the
	 * general preference each time, but can be useful for other caches
	 * as well. */
	if (g_file_has_uri_scheme (file, "file")) {
		char  *uri;
		char  *tmp_uri;
		char **uri_v;
		int    i;

		uri = g_file_get_uri (file);

		tmp_uri = g_filename_to_uri (g_get_tmp_dir (), NULL, NULL);
		if (tmp_uri != NULL) {
			gboolean is_tmp_dir;

			is_tmp_dir = (strcmp (uri, tmp_uri) == 0);
			g_free (tmp_uri);

			if (is_tmp_dir) {
				g_free (uri);
				return FALSE;
			}
		}

		uri_v = g_strsplit (uri, "/", -1);
		for (i = 0; uri_v[i] != NULL; i++)
			if (strstr (uri_v[i], "cache")
			    || strstr (uri_v[i], "CACHE")
			    || strstr (uri_v[i], "Cache"))
			{
				fast_file_type = FALSE;
				break;
			}

		 g_strfreev (uri_v);
		 g_free (uri);
	}

	return fast_file_type;
}


static void
_update_statusbar_list_info (GthBrowser *browser)
{
	GList   *file_list;
	int      n_total;
	goffset  size_total;
	GList   *scan;
	int      n_selected;
	goffset  size_selected;
	GList   *selected;
	char    *size_total_formatted;
	char    *size_selected_formatted;
	char    *text_total;
	char    *text_selected;
	GString *text;

	/* total */

	file_list = gth_file_store_get_visibles (gth_browser_get_file_store (browser));
	n_total = 0;
	size_total = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		n_total++;
		size_total += g_file_info_get_size (file_data->info);
	}
	_g_object_list_unref (file_list);

	/* selected */

	selected = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), selected);

	n_selected = 0;
	size_selected = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		n_selected++;
		size_selected += g_file_info_get_size (file_data->info);
	}
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (selected);

	/**/

	size_total_formatted = g_format_size (size_total);
	size_selected_formatted = g_format_size (size_selected);
	text_total = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", n_total), n_total, size_total_formatted);
	text_selected = g_strdup_printf (g_dngettext (NULL, "%d file selected (%s)", "%d files selected (%s)", n_selected), n_selected, size_selected_formatted);

	text = g_string_new (text_total);
	if (n_selected > 0) {
		g_string_append (text, STATUSBAR_SEPARATOR);
		g_string_append (text, text_selected);
	}
	if (browser->priv->location_free_space != NULL) {
		g_string_append (text, STATUSBAR_SEPARATOR);
		g_string_append (text, browser->priv->location_free_space);
	}
	gth_statusbar_set_list_info (GTH_STATUSBAR (browser->priv->statusbar), text->str);

	g_string_free (text, TRUE);
	g_free (text_selected);
	g_free (text_total);
	g_free (size_selected_formatted);
	g_free (size_total_formatted);
}


static void
get_free_space_ready_cb (GthFileSource *file_source,
			 guint64        total_size,
			 guint64        free_space,
			 GError        *error,
			 gpointer       data)
{
	GthBrowser *browser = data;
	char       *free_space_text;

	if (error == NULL) {
		char *free_space_formatted;

		free_space_formatted = g_format_size (free_space);
		free_space_text = g_strdup_printf (_("%s of free space"), free_space_formatted);

		g_free (free_space_formatted);
	}
	else
		free_space_text = NULL;

	g_free (browser->priv->location_free_space);
	browser->priv->location_free_space = NULL;
	if (free_space_text != NULL)
		browser->priv->location_free_space = g_strdup (free_space_text);
	g_free (free_space_text);

	browser->priv->recalc_location_free_space = FALSE;

	_update_statusbar_list_info (browser);
}


static void
_gth_browser_update_statusbar_list_info (GthBrowser *browser)
{
	if (browser->priv->recalc_location_free_space
	    && (browser->priv->location_source != NULL)
	    && (browser->priv->location != NULL))
	{
		gth_file_source_get_free_space (browser->priv->location_source,
						browser->priv->location->file,
						get_free_space_ready_cb,
						browser);
	}
	else {
		if ((browser->priv->location_source == NULL)
		    || (browser->priv->location == NULL))
		{
			g_free (browser->priv->location_free_space);
			browser->priv->location_free_space = NULL;
		}
		_update_statusbar_list_info (browser);
	}
}


typedef struct {
	GthBrowser    *browser;
	GthFileData   *requested_folder;
	GFile         *file_to_select;
	GList         *selected;
	double         vscroll;
	GthAction      action;
	gboolean       automatic;
	GList         *list;
	GList         *current;
	GFile         *entry_point;
	GthFileSource *file_source;
	GCancellable  *cancellable;
} LoadData;


static LoadData *
load_data_new (GthBrowser *browser,
	       GFile      *location,
	       GFile      *file_to_select,
	       GList      *selected,
	       double      vscroll,
	       GthAction   action,
	       gboolean    automatic,
	       GFile      *entry_point)
{
	LoadData *load_data;
	GFile    *file;

	load_data = g_new0 (LoadData, 1);
	load_data->browser = browser;
	load_data->requested_folder = gth_file_data_new (location, NULL);
	if (file_to_select != NULL)
		load_data->file_to_select = g_file_dup (file_to_select);
	else if (browser->priv->current_file != NULL)
		load_data->file_to_select = g_file_dup (browser->priv->current_file->file);
	load_data->selected = g_list_copy_deep (selected, (GCopyFunc) gtk_tree_path_copy, NULL);
	load_data->vscroll = vscroll;
	load_data->action = action;
	load_data->automatic = automatic;
	load_data->cancellable = g_cancellable_new ();

	browser->priv->load_data_queue = g_list_prepend (browser->priv->load_data_queue, load_data);
	if (gth_action_changes_folder (load_data->action))
		browser->priv->last_folder_to_open = load_data;

	if (entry_point == NULL)
		return load_data;

	load_data->entry_point = g_object_ref (entry_point);
	load_data->file_source = gth_main_get_file_source (load_data->requested_folder->file);

	file = g_object_ref (load_data->requested_folder->file);
	load_data->list = g_list_prepend (NULL, g_object_ref (file));
	while (! g_file_equal (load_data->entry_point, file)) {
		GFile *parent;

		parent = g_file_get_parent (file);
		g_object_unref (file);
		file = parent;

		load_data->list = g_list_prepend (load_data->list, g_object_ref (file));
	}
	g_object_unref (file);
	load_data->current = NULL;

	return load_data;
}


static void
load_data_free (LoadData *data)
{
	if (data->browser->priv->last_folder_to_open == data)
		data->browser->priv->last_folder_to_open = NULL;
	data->browser->priv->load_data_queue = g_list_remove (data->browser->priv->load_data_queue, data);

	g_object_unref (data->requested_folder);
	_g_object_unref (data->file_to_select);
	g_list_free_full (data->selected, (GDestroyNotify) gtk_tree_path_free);
	_g_object_unref (data->file_source);
	_g_object_list_unref (data->list);
	_g_object_unref (data->entry_point);
	g_object_unref (data->cancellable);
	g_free (data);
}


static void
_gth_browser_load (GthBrowser *browser,
		   GFile      *location,
		   GFile      *file_to_select,
		   GList      *selected,
		   double      vscroll,
		   GthAction   action,
		   gboolean    automatic);


static void
_gth_browser_show_error (GthBrowser *browser,
			 const char *title,
			 GError     *error)
{
	/* _gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), title, error); */

	gth_info_bar_set_primary_text (GTH_INFO_BAR (browser->priv->infobar), title);
	gth_info_bar_set_secondary_text (GTH_INFO_BAR (browser->priv->infobar), error->message);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (gtk_info_bar_get_action_area (GTK_INFO_BAR (browser->priv->infobar))), GTK_ORIENTATION_HORIZONTAL);
	gth_info_bar_set_icon_name (GTH_INFO_BAR (browser->priv->infobar), "dialog-error-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (browser->priv->infobar), GTK_MESSAGE_ERROR);
	_gtk_info_bar_clear_action_area (GTK_INFO_BAR (browser->priv->infobar));
	gtk_info_bar_add_buttons (GTK_INFO_BAR (browser->priv->infobar),
				  _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE,
				  NULL);
	gtk_widget_show (browser->priv->infobar);
}


static void
_gth_browser_update_activity (GthBrowser *browser,
			      gboolean    increment)
{
	if (increment) {
		browser->priv->activity_ref++;
		if (browser->priv->activity_ref == 1) {
			GdkCursor  *cursor = gdk_cursor_new_from_name (gtk_widget_get_display (GTK_WIDGET (browser)), "progress");
			gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (browser)), cursor);
			g_object_unref (cursor);
		}
	}
	else {
		browser->priv->activity_ref--;
		if (browser->priv->activity_ref == 0) {
			GdkCursor *cursor = gdk_cursor_new_from_name (gtk_widget_get_display (GTK_WIDGET (browser)), "default");
			gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (browser)), cursor);
			g_object_unref (cursor);
		}
	}
}


static void
_gth_browser_add_activity (GthBrowser *browser)
{
	_gth_browser_update_activity (browser, TRUE);
}


static void
_gth_browser_remove_activity (GthBrowser *browser)
{
	_gth_browser_update_activity (browser, FALSE);
}


static gboolean
load_data_is_still_relevant (LoadData *load_data)
{
	return ! gth_action_changes_folder (load_data->action)
		    || (load_data == load_data->browser->priv->last_folder_to_open);
}


static void
load_data_done (LoadData *load_data,
		GError   *error)
{
	GthBrowser *browser = load_data->browser;
	char       *title;

	{
		char *uri;

		uri = g_file_get_uri (load_data->requested_folder->file);
		debug (DEBUG_INFO, "LOAD READY: %s [%s]\n", uri, (error == NULL ? "Ok" : "Error"));
		performance (DEBUG_INFO, "load done for %s", uri);

		g_free (uri);
	}

	_gth_browser_remove_activity (browser);

	if (error == NULL)
		return;

	g_signal_emit (G_OBJECT (browser),
		       gth_browser_signals[LOCATION_READY],
		       0,
		       load_data->requested_folder->file,
		       TRUE);

	if (! load_data_is_still_relevant (load_data)
		|| g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
	{
		g_error_free (error);
		return;
	}

	if (gth_action_changes_folder (load_data->action)
		&& load_data->automatic)
	{
		GFile *parent;

		parent = g_file_get_parent (load_data->requested_folder->file);
		if (parent != NULL) {
			_gth_browser_load (load_data->browser,
					   parent,
					   NULL,
					   NULL,
					   0,
					   load_data->action,
					   TRUE);
			g_object_unref (parent);
			return;
		}
	}

	gth_browser_update_sensitivity (browser);
	title = _g_format_str_for_file (_("Could not load the position “%s”"),
					load_data->requested_folder->file);
	_gth_browser_show_error (browser, title, error);

	g_free (title);
	g_error_free (error);
}


static void
load_data_error (LoadData *load_data,
		 GError   *error)
{
	gth_folder_tree_set_children (GTH_FOLDER_TREE (load_data->browser->priv->folder_tree),
				      G_FILE (load_data->current->data),
				      NULL);

	load_data_done (load_data, error);
	load_data_free (load_data);
}


static void
load_data_cancelled (LoadData *load_data)
{
	load_data_done (load_data, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
	load_data_free (load_data);
}


/* -- _gth_browser_set_sort_order -- */


static const char *
_gth_browser_get_list_attributes (GthBrowser *browser,
				  gboolean    recalc)
{
	GString *attributes;
	GthTest *filter;
	char    *thumbnail_caption;

	if (recalc) {
		g_free (browser->priv->list_attributes);
		browser->priv->list_attributes = NULL;
	}

	if (browser->priv->list_attributes != NULL)
		return browser->priv->list_attributes;

	attributes = g_string_new ("");

	/* standard attributes */

	if (browser->priv->fast_file_type)
		g_string_append (attributes, GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE);
	else
		g_string_append (attributes, GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE);

	/* attributes required by the filter */

	filter = _gth_browser_get_file_filter (browser);
	if (filter != NULL) {
		const char *filter_attributes;

		filter_attributes = gth_test_get_attributes (GTH_TEST (filter));
		if (filter_attributes[0] != '\0') {
			g_string_append (attributes, ",");
			g_string_append (attributes, filter_attributes);
		}

		g_object_unref (filter);
	}

	/* attributes required for sorting */

	if ((browser->priv->current_sort_type != NULL) && (browser->priv->current_sort_type->required_attributes[0] != '\0')) {
		g_string_append (attributes, ",");
		g_string_append (attributes, browser->priv->current_sort_type->required_attributes);
	}

	/* attributes required for the thumbnail caption */

	thumbnail_caption = g_settings_get_string (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION);
	if ((thumbnail_caption[0] != '\0') && (strcmp (thumbnail_caption, "none") != 0)) {
		g_string_append (attributes, ",");
		g_string_append (attributes, thumbnail_caption);
		g_free (thumbnail_caption);
	}

	/* other attributes */

	g_string_append (attributes, ",");
	g_string_append (attributes, GTH_FILE_ATTRIBUTE_EMBLEMS);

	/* save in a variable to avoid recalculation */

	browser->priv->list_attributes = g_string_free (attributes, FALSE);

	return browser->priv->list_attributes;
}


static gboolean
_gth_browser_reload_required (GthBrowser *browser)
{
	char       *old_attributes;
	const char *new_attributes;
	gboolean    reload_required;

	old_attributes = g_strdup (_gth_browser_get_list_attributes (browser, FALSE));
	new_attributes = _gth_browser_get_list_attributes (browser, TRUE);
	reload_required = attribute_list_reload_required (old_attributes, new_attributes);

	g_free (old_attributes);

	return reload_required;
}


static void
write_sort_order_ready_cb (GObject  *source,
		           GError   *error,
		           gpointer  user_data)
{
	GthBrowser *browser = user_data;

	if (browser->priv->constructed && _gth_browser_reload_required (browser))
		gth_browser_reload (browser);
}


static void
_gth_browser_set_sort_order (GthBrowser      *browser,
			     GthFileDataSort *sort_type,
			     gboolean         inverse,
			     gboolean         save,
			     gboolean         update_view)
{
	g_return_if_fail (sort_type != NULL);

	browser->priv->current_sort_type = sort_type;
	browser->priv->current_sort_inverse = inverse;

	gth_file_list_set_sort_func (GTH_FILE_LIST (browser->priv->file_list),
				     sort_type->cmp_func,
				     inverse);

	if (! browser->priv->constructed || (browser->priv->location == NULL))
		return;

	g_file_info_set_attribute_string (browser->priv->location->info, "sort::type", (sort_type != NULL) ? sort_type->name : "general::unsorted");
	g_file_info_set_attribute_boolean (browser->priv->location->info, "sort::inverse", (sort_type != NULL) ? inverse : FALSE);

	if (update_view) {
		_gth_browser_update_current_file_position (browser);
		gth_browser_update_title (browser);
	}

	if (! save || (browser->priv->location_source == NULL))
		return;

	gth_file_source_write_metadata (browser->priv->location_source,
					browser->priv->location,
					"sort::type,sort::inverse",
					write_sort_order_ready_cb,
					browser);
}


static void _gth_browser_load_ready_cb (GthFileSource *file_source, GList *files, GError *error, gpointer user_data);


static void
requested_folder_attributes_ready_cb (GObject  *file_source,
				      GError   *error,
				      gpointer  user_data)
{
	LoadData   *load_data = user_data;
	GthBrowser *browser = load_data->browser;

	if (error != NULL) {
		load_data_error (load_data, error);
		return;
	}

	gth_file_source_list (load_data->file_source,
			      load_data->requested_folder->file,
			      _gth_browser_get_fast_file_type (browser, load_data->requested_folder->file) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE,
			      _gth_browser_load_ready_cb,
			      load_data);
}


static void
load_data_load_next_folder (LoadData *load_data)
{
	GthFolderTree *folder_tree;
	GFile         *folder_to_load = NULL;

	folder_tree = GTH_FOLDER_TREE (load_data->browser->priv->folder_tree);
	do {
		GtkTreePath *path;

		if (load_data->current == NULL)
			load_data->current = load_data->list;
		else
			load_data->current = load_data->current->next;
		folder_to_load = (GFile *) load_data->current->data;

		if (g_file_equal (folder_to_load, load_data->requested_folder->file))
			break;

		path = gth_folder_tree_get_path (folder_tree, folder_to_load);
		if (path == NULL)
			break;

		if (! gth_folder_tree_is_loaded (folder_tree, path)) {
			gtk_tree_path_free (path);
			break;
		}

		if (! g_file_equal (folder_to_load, load_data->requested_folder->file))
			gth_folder_tree_expand_row (folder_tree, path, FALSE);

		gtk_tree_path_free (path);
	}
	while (TRUE);

	g_assert (folder_to_load != NULL);

	if (gth_action_changes_folder (load_data->action)
		&& g_file_equal (folder_to_load, load_data->requested_folder->file))
	{
		gth_file_source_read_metadata (load_data->file_source,
					       load_data->requested_folder,
					       "*",
					       requested_folder_attributes_ready_cb,
					       load_data);
	}
	else
		gth_file_source_list (load_data->file_source,
				      folder_to_load,
				      GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
				      _gth_browser_load_ready_cb,
				      load_data);
}


static gboolean
_gth_browser_file_is_visible (GthBrowser  *browser,
				GthFileData *file_data)
{
	if (browser->priv->show_hidden_files)
		return TRUE;
	else
		return ! g_file_info_get_is_hidden (file_data->info);
}


static GList *
_gth_browser_get_visible_files (GthBrowser *browser,
			        GList      *list)
{
	GList *visible_list = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_gth_browser_file_is_visible (browser, file_data))
			visible_list = g_list_prepend (visible_list, g_object_ref (file_data));
	}

	return g_list_reverse (visible_list);
}


static gboolean _gth_browser_make_file_visible (GthBrowser  *browser,
						GthFileData *file_data);


static void
load_data_continue (LoadData *load_data,
		    GList    *loaded_files)
{
	GthBrowser  *browser = load_data->browser;
	GList       *files;
	GFile       *loaded_folder;
	gboolean     loaded_requested_folder;
	GtkTreePath *path;
	gboolean     changed_current_location;

	if (! load_data_is_still_relevant (load_data)) {
		load_data_cancelled (load_data);
		return;
	}

	loaded_folder = (GFile *) load_data->current->data;
	files = _gth_browser_get_visible_files (browser, loaded_files);
	gth_folder_tree_set_children (GTH_FOLDER_TREE (browser->priv->folder_tree), loaded_folder, files);

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (browser->priv->folder_tree), loaded_folder);
	loaded_requested_folder = _g_file_equal (loaded_folder, load_data->requested_folder->file);
	if ((path != NULL) && ! loaded_requested_folder)
		gth_folder_tree_expand_row (GTH_FOLDER_TREE (browser->priv->folder_tree), path, FALSE);

	if (! loaded_requested_folder) {
		load_data_load_next_folder (load_data);

		if (path != NULL)
			gtk_tree_path_free (path);
		_g_object_list_unref (files);
		return;
	}

	load_data_done (load_data, NULL);

	changed_current_location = gth_action_changes_folder (load_data->action);
	if (changed_current_location) {
		GthTest *filter;

		if ((browser->priv->location_source != NULL)
			&& (browser->priv->monitor_location != NULL))
		{
			gth_file_source_monitor_directory (browser->priv->location_source,
							   browser->priv->monitor_location,
							   FALSE);
			_g_clear_object (&browser->priv->monitor_location);
		}

		_g_object_unref (browser->priv->location_source);
		browser->priv->location_source = g_object_ref (load_data->file_source);

		browser->priv->recalc_location_free_space = TRUE;

		switch (load_data->action) {
		case GTH_ACTION_GO_TO:
		case GTH_ACTION_TREE_OPEN:
			_gth_browser_set_location (browser, load_data->requested_folder);
			if (browser->priv->location != NULL)
				_gth_browser_history_add (browser, browser->priv->location->file);
			_gth_browser_history_menu (browser);
			break;
		case GTH_ACTION_GO_BACK:
		case GTH_ACTION_GO_FORWARD:
			_gth_browser_set_location (browser, load_data->requested_folder);
			_gth_browser_history_menu (browser);
			break;
		default:
			break;
		}

		if (path != NULL) {
			GList    *entry_points;
			GList    *scan;
			gboolean  is_entry_point = FALSE;

			/* Collapse everything else after loading an entry point. */

			entry_points = gth_main_get_all_entry_points ();
			for (scan = entry_points; scan; scan = scan->next) {
				GthFileData *file_data = scan->data;

				if (g_file_equal (file_data->file, load_data->requested_folder->file)) {
					gth_folder_tree_collapse_all (GTH_FOLDER_TREE (browser->priv->folder_tree));
					is_entry_point = TRUE;
					break;
				}
			}

			gtk_tree_view_expand_row (GTK_TREE_VIEW (browser->priv->folder_tree), path, FALSE);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (browser->priv->folder_tree),
						      path,
						      NULL,
						      is_entry_point,
						      0.0,
						      0.0);
			gth_folder_tree_select_path (GTH_FOLDER_TREE (browser->priv->folder_tree), path);

			_g_object_list_unref (entry_points);
		}

		filter = _gth_browser_get_file_filter (browser);
		gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->file_list), filter);
		gth_file_list_set_files (GTH_FILE_LIST (browser->priv->file_list), files);
		g_object_unref (filter);

		if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER)
			gth_file_list_focus (GTH_FILE_LIST (browser->priv->file_list));

		if (load_data->file_to_select != NULL)
			gth_file_list_make_file_visible (GTH_FILE_LIST (browser->priv->file_list), load_data->file_to_select);
		else if ((load_data->selected != NULL) || (load_data->vscroll > 0))
			gth_file_list_restore_state (GTH_FILE_LIST (browser->priv->file_list), load_data->selected, load_data->vscroll);

		_gth_browser_update_statusbar_list_info (browser);

		g_assert (browser->priv->location_source != NULL);

		if (browser->priv->monitor_location != NULL)
			g_object_unref (browser->priv->monitor_location);
		browser->priv->monitor_location = g_file_dup (browser->priv->location->file);
		gth_file_source_monitor_directory (browser->priv->location_source,
						   browser->priv->monitor_location,
						   TRUE);

		if (browser->priv->current_file != NULL) {
			_gth_browser_update_current_file_position (browser);
			gth_browser_update_title (browser);
			gth_browser_update_statusbar_file_info (browser);
			if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER) {
				_gth_browser_make_file_visible (browser, browser->priv->current_file);
				if (browser->priv->viewer_page != NULL) {
					gth_viewer_page_update_info (browser->priv->viewer_page, browser->priv->current_file);
					gth_viewer_page_focus (browser->priv->viewer_page);
				}
			}
		}

		gth_browser_update_title (browser);
		gth_browser_update_extra_widget (browser);

		/* moving the "gth-browser-load-location-after" after the
		 * LOCATION_READY signal emition can brake the extensions */

		gth_hook_invoke ("gth-browser-load-location-after", browser, browser->priv->location);

		g_signal_emit (G_OBJECT (browser),
			       gth_browser_signals[LOCATION_READY],
			       0,
			       load_data->requested_folder->file,
			       FALSE);

		if (StartSlideshow) {
			StartSlideshow = FALSE;
			gth_hook_invoke ("slideshow", browser);
		}

		if (StartInFullscreen) {
			StartInFullscreen = FALSE;
			gth_browser_fullscreen (browser);
		}
	}

	gth_browser_update_sensitivity (browser);

	if (path != NULL)
		gtk_tree_path_free (path);
	_g_object_list_unref (files);
	load_data_free (load_data);
}


static void
metadata_ready_cb (GObject      *source_object,
		   GAsyncResult *result,
		   gpointer      user_data)
{
	LoadData *load_data = user_data;
	GList    *files;
	GError   *error = NULL;

	files = _g_query_metadata_finish (result, &error);
	if (error != NULL) {
		load_data_error (load_data, error);
		return;
	}

	load_data_continue (load_data, files);
}


static void
load_data_ready (LoadData *load_data,
		 GList    *files,
		 GError   *error)
{
	if (error != NULL) {
		load_data_error (load_data, error);
	}
	else if (gth_action_changes_folder (load_data->action)
		 && _g_file_equal ((GFile *) load_data->current->data, load_data->requested_folder->file))
	{
		_g_query_metadata_async (files,
					 _gth_browser_get_list_attributes (load_data->browser, TRUE),
					 load_data->cancellable,
					 metadata_ready_cb,
					 load_data);
	}
	else
		load_data_continue (load_data, files);
}


static void
_gth_browser_load_ready_cb (GthFileSource *file_source,
			    GList         *files,
			    GError        *error,
			    gpointer       user_data)
{
	load_data_ready ((LoadData *) user_data, files, error);
}


static void
mount_volume_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	if (! g_file_mount_enclosing_volume_finish (G_FILE (source_object),
						    result,
						    &error))
	{
		load_data_done (load_data, error);
		load_data_free (load_data);
		return;
	}

	gth_monitor_entry_points_changed (gth_main_get_default_monitor ());
	_gth_browser_update_entry_point_list (load_data->browser);

	/* try to load again */

	if (! load_data_is_still_relevant (load_data)) {
		load_data_cancelled (load_data);
		return;
	}

	_gth_browser_remove_activity (load_data->browser);

	_gth_browser_load (load_data->browser,
			   load_data->requested_folder->file,
			   load_data->file_to_select,
			   NULL,
			   0,
			   load_data->action,
			   load_data->automatic);

	load_data_free (load_data);
}


static void
mount_mountable_ready_cb (GObject      *source_object,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	if (! g_file_start_mountable_finish (G_FILE (source_object),
					     result,
					     &error))
	{
		load_data_done (load_data, error);
		load_data_free (load_data);
		return;
	}

	gth_monitor_entry_points_changed (gth_main_get_default_monitor ());
	_gth_browser_update_entry_point_list (load_data->browser);

	/* try to load again */

	if (! load_data_is_still_relevant (load_data)) {
		load_data_cancelled (load_data);
		return;
	}

	_gth_browser_remove_activity (load_data->browser);

	_gth_browser_load (load_data->browser,
			   load_data->requested_folder->file,
			   load_data->file_to_select,
			   NULL,
			   0,
			   load_data->action,
			   load_data->automatic);

	load_data_free (load_data);
}


static void
volume_mount_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	if (! g_volume_mount_finish (G_VOLUME (source_object),
				     result,
				     &error))
	{
		load_data_done (load_data, error);
		load_data_free (load_data);
		return;
	}

	gth_monitor_entry_points_changed (gth_main_get_default_monitor ());
	_gth_browser_update_entry_point_list (load_data->browser);

	/* try to load again */

	if (! load_data_is_still_relevant (load_data)) {
		load_data_cancelled (load_data);
		return;
	}

	_gth_browser_remove_activity (load_data->browser);

	_gth_browser_load (load_data->browser,
			   load_data->requested_folder->file,
			   load_data->file_to_select,
			   NULL,
			   0,
			   load_data->action,
			   load_data->automatic);

	load_data_free (load_data);
}


static void
_gth_browser_hide_infobar (GthBrowser *browser)
{
	if (gtk_widget_get_visible (browser->priv->infobar))
		gtk_info_bar_response (GTK_INFO_BAR (browser->priv->infobar), GTK_RESPONSE_CLOSE);
}


static void
_gth_browser_load (GthBrowser *browser,
		   GFile      *location,
		   GFile      *file_to_select,
		   GList      *selected,
		   double      vscroll,
		   GthAction   action,
		   gboolean    automatic)
{
	LoadData    *load_data;
	GthFileData *entry_point;

	if (! automatic)
		_gth_browser_hide_infobar (browser);

	if (gth_action_changes_folder (action) && (browser->priv->last_folder_to_open != NULL)) {
		LoadData *last_load_data = (LoadData *) browser->priv->last_folder_to_open;
		g_cancellable_cancel (last_load_data->cancellable);
	}

	entry_point = gth_main_get_nearest_entry_point (location);
	load_data = load_data_new (browser,
				   location,
				   file_to_select,
				   selected,
				   vscroll,
				   action,
				   automatic,
				   (entry_point != NULL) ? entry_point->file : NULL);

	_gth_browser_add_activity (browser);

	if (entry_point == NULL) {
		GMountOperation *mount_op;

		/* try to mount the enclosing volume */

		mount_op = gtk_mount_operation_new (GTK_WINDOW (browser));
		g_file_mount_enclosing_volume (location,
					       0,
					       mount_op,
					       load_data->cancellable,
					       mount_volume_ready_cb,
					       load_data);

		g_object_unref (mount_op);

		return;
	}
	else if (g_file_info_get_file_type (entry_point->info) == G_FILE_TYPE_MOUNTABLE) {
		GMountOperation *mount_op;
		GVolume         *volume;

		mount_op = gtk_mount_operation_new (GTK_WINDOW (browser));
		volume = (GVolume *) g_file_info_get_attribute_object (entry_point->info, GTH_FILE_ATTRIBUTE_VOLUME);
		if (volume != NULL)
			g_volume_mount (volume,
					0,
					mount_op,
					load_data->cancellable,
					volume_mount_ready_cb,
					load_data);
		else
			g_file_mount_mountable (entry_point->file,
						0,
						mount_op,
						load_data->cancellable,
						mount_mountable_ready_cb,
						load_data);

		g_object_unref (mount_op);

		return;
	}

	if (gth_action_changes_folder (load_data->action))
		gth_hook_invoke ("gth-browser-load-location-before", browser, load_data->requested_folder->file);

	if (entry_point == NULL) {
		GError *error;
		char   *uri;

		uri = g_file_get_uri (location);
		error = g_error_new (GTH_ERROR, 0, _("No suitable module found for %s"), uri);
		load_data_ready (load_data, NULL, error);

		g_free (uri);

		return;
	}

	if (action == GTH_ACTION_TREE_LIST_CHILDREN)
		gth_folder_tree_loading_children (GTH_FOLDER_TREE (browser->priv->folder_tree), location);

	if (load_data->file_source == NULL) {
		GError *error;
		char   *uri;

		uri = g_file_get_uri (load_data->requested_folder->file);
		error = g_error_new (GTH_ERROR, 0, _("No suitable module found for %s"), uri);
		load_data_ready (load_data, NULL, error);

		g_free (uri);

		return;
	}

	if (! gth_file_source_shows_extra_widget (load_data->file_source))
		gtk_widget_hide (browser->priv->list_info_bar);

	{
		char *uri;

		uri = g_file_get_uri (load_data->requested_folder->file);

		debug (DEBUG_INFO, "LOAD: %s\n", uri);
		performance (DEBUG_INFO, "loading %s", uri);

		g_free (uri);
	}

	load_data_load_next_folder (load_data);

	g_object_unref (entry_point);
}


/* -- gth_browser_ask_whether_to_save -- */


typedef struct {
	GthBrowser         *browser;
	GthBrowserCallback  callback;
	gpointer            user_data;
} AskSaveData;


static void
ask_whether_to_save__done (AskSaveData *data,
			   gboolean     cancelled)
{
	if (data->browser->priv->current_file != NULL)
		g_file_info_set_attribute_boolean (data->browser->priv->current_file->info, "gth::file::is-modified", cancelled);
	if (data->callback != NULL)
		(*data->callback) (data->browser, cancelled, data->user_data);
	g_free (data);
}


static void
ask_whether_to_save__file_saved_cb (GthViewerPage *viewer_page,
				    GthFileData   *file_data,
				    GError        *error,
				    gpointer       user_data)
{
	AskSaveData *data = user_data;
	gboolean     error_occurred;

	error_occurred = error != NULL;
	if (error != NULL)
		_gth_browser_show_error (data->browser, _("Could not save the file"), error);
	ask_whether_to_save__done (data, error_occurred);
}


enum {
	RESPONSE_SAVE,
	RESPONSE_NO_SAVE,
};


static void
ask_whether_to_save__response_cb (GtkWidget   *dialog,
				  int          response_id,
				  AskSaveData *data)
{
	gtk_widget_destroy (dialog);

	if ((response_id == RESPONSE_SAVE) && (data->browser->priv->viewer_page != NULL))
		gth_viewer_page_save (data->browser->priv->viewer_page,
				      NULL,
				      ask_whether_to_save__file_saved_cb,
				      data);
	else
		ask_whether_to_save__done (data, response_id != RESPONSE_NO_SAVE);
}


void
gth_browser_ask_whether_to_save (GthBrowser         *browser,
				 GthBrowserCallback  callback,
				 gpointer            user_data)
{
	AskSaveData *data;
	char        *title;
	GtkWidget   *d;

	data = g_new0 (AskSaveData, 1);
	data->browser = browser;
	data->callback = callback;
	data->user_data = user_data;

	gtk_window_present (GTK_WINDOW (browser));

	title = g_strdup_printf (_("Save changes to file “%s”?"), g_file_info_get_display_name (browser->priv->current_file->info));
	d = _gtk_message_dialog_new (GTK_WINDOW (browser),
				     GTK_DIALOG_MODAL,
				     _GTK_ICON_NAME_DIALOG_QUESTION,
				     title,
				     _("If you don’t save, changes to the file will be permanently lost."),
				     _("Do _Not Save"), RESPONSE_NO_SAVE,
				     _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				     _GTK_LABEL_SAVE, RESPONSE_SAVE,
				     NULL);
	g_signal_connect (G_OBJECT (d),
			  "response",
			  G_CALLBACK (ask_whether_to_save__response_cb),
			  data);

	gtk_widget_show (d);

	g_free (title);
}


/* -- _gth_browser_close -- */


static void
_gth_browser_show_pointer_on_viewer (GthBrowser *browser,
				     gboolean    show)
{
	browser->priv->pointer_visible = show;
	if (browser->priv->viewer_page != NULL)
		gth_viewer_page_show_pointer (GTH_VIEWER_PAGE (browser->priv->viewer_page), show);
}


static void
_gth_browser_deactivate_viewer_page (GthBrowser *browser)
{
	if (browser->priv->viewer_page != NULL) {
		if (browser->priv->fullscreen)
			_gth_browser_show_pointer_on_viewer (browser, TRUE);
		gth_hook_invoke ("gth-browser-deactivate-viewer-page", browser);
		gth_viewer_page_deactivate (browser->priv->viewer_page);
		gth_browser_set_viewer_widget (browser, NULL);
		g_object_unref (browser->priv->viewer_page);
		browser->priv->viewer_page = NULL;
	}
}


static void
_gth_browser_close_final_step (gpointer user_data)
{
	GthBrowser *browser = user_data;
	gboolean    last_window;

	last_window = g_list_length (gtk_application_get_windows (gtk_window_get_application (GTK_WINDOW (browser)))) == 1;

	if (gtk_widget_get_realized (GTK_WIDGET (browser))) {
		gboolean        maximized;
		GtkAllocation   allocation;
		char          **sidebar_sections;

		/* Save visualization options only if the window is not maximized. */

		maximized = _gtk_window_get_is_maximized (GTK_WINDOW (browser));
		if (! maximized && ! browser->priv->fullscreen && gtk_widget_get_visible (GTK_WIDGET (browser))) {
			int width, height;
			int size_set = FALSE;

			if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER) {
				gtk_window_get_size (GTK_WINDOW (browser), &width, &height);
				size_set = TRUE;
			}
			else
				size_set = gth_window_get_page_size (GTH_WINDOW (browser),
								     GTH_BROWSER_PAGE_BROWSER,
								     &width,
								     &height);

			if (size_set) {
				g_settings_set_int (browser->priv->browser_settings, PREF_BROWSER_WINDOW_WIDTH, width);
				g_settings_set_int (browser->priv->browser_settings, PREF_BROWSER_WINDOW_HEIGHT, height);
			}
		}

		g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_WINDOW_MAXIMIZED, maximized);

		gtk_widget_get_allocation (browser->priv->browser_sidebar, &allocation);
		if (allocation.width > MIN_SIDEBAR_SIZE)
			g_settings_set_int (browser->priv->browser_settings, PREF_BROWSER_BROWSER_SIDEBAR_WIDTH, allocation.width);

		g_settings_set_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_SIDEBAR, browser->priv->viewer_sidebar);

		g_settings_set_enum (browser->priv->browser_settings, PREF_FULLSCREEN_SIDEBAR, browser->priv->fullscreen_state.sidebar);
		g_settings_set_boolean (browser->priv->browser_settings, PREF_FULLSCREEN_THUMBNAILS_VISIBLE, browser->priv->fullscreen_state.thumbnail_list);

		sidebar_sections = gth_sidebar_get_sections_status (GTH_SIDEBAR (browser->priv->file_properties));
		g_settings_set_strv (browser->priv->browser_settings, PREF_BROWSER_SIDEBAR_SECTIONS, (const char **) sidebar_sections);
		g_strfreev (sidebar_sections);
	}

	/**/

	_gth_browser_deactivate_viewer_page (browser);

	gth_hook_invoke ("gth-browser-close", browser);

	if (gtk_widget_get_realized (GTK_WIDGET (browser)) && last_window) {
		if (g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_GO_TO_LAST_LOCATION)
		    && (browser->priv->location != NULL))
		{
			char *uri;

			uri = g_file_get_uri (browser->priv->location->file);
			_g_settings_set_uri (browser->priv->browser_settings, PREF_BROWSER_STARTUP_LOCATION, uri);
			g_free (uri);

			if (browser->priv->current_file != NULL) {
				uri = g_file_get_uri (browser->priv->current_file->file);
				_g_settings_set_uri (browser->priv->browser_settings, PREF_BROWSER_STARTUP_CURRENT_FILE, uri);
				g_free (uri);
			}
			else
				_g_settings_set_uri (browser->priv->browser_settings, PREF_BROWSER_STARTUP_CURRENT_FILE, "");
		}

		if (browser->priv->default_sort_type != NULL) {
			g_settings_set_string (browser->priv->browser_settings, PREF_BROWSER_SORT_TYPE, browser->priv->default_sort_type->name);
			g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_SORT_INVERSE, browser->priv->default_sort_inverse);
		}

		gth_filterbar_save_filter (GTH_FILTERBAR (browser->priv->filterbar), "active_filter.xml");

		_gth_browser_history_save (browser);

		gth_hook_invoke ("gth-browser-close-last-window", browser);
	}

	if (browser->priv->progress_dialog != NULL) {
		g_object_remove_weak_pointer (G_OBJECT (browser->priv->progress_dialog), (gpointer*) &(browser->priv->progress_dialog));
		gtk_widget_destroy (browser->priv->progress_dialog);
	}

	g_settings_sync ();

	gtk_widget_destroy (GTK_WIDGET (browser));
}


static void
_gth_browser_close_step4 (gpointer user_data)
{
	GthBrowser *browser = user_data;

	gth_file_list_cancel (GTH_FILE_LIST (browser->priv->thumbnail_list),
			      _gth_browser_close_final_step,
			      browser);
}


static void
_gth_browser_close_step3 (gpointer user_data)
{
	GthBrowser *browser = user_data;

	gth_file_list_cancel (GTH_FILE_LIST (browser->priv->file_list),
			      _gth_browser_close_step4,
			      browser);
}


static void _gth_browser_cancel (GthBrowser *browser,
				 DataFunc    done_func,
				 gpointer    user_data);


static void
_gth_browser_real_close (GthBrowser *browser)
{
	if (browser->priv->closing)
		return;

	browser->priv->closing = TRUE;

	/* disconnect from the settings */

	_g_signal_handlers_disconnect_by_data (browser->priv->browser_settings, browser);
	_g_signal_handlers_disconnect_by_data (browser->priv->messages_settings, browser);
	_g_signal_handlers_disconnect_by_data (browser->priv->desktop_interface_settings, browser);

	/* disconnect from the monitor */

	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->folder_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->file_renamed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->metadata_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->emblems_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->entry_points_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->order_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->shortcuts_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (),
				     browser->priv->filters_changed_id);

	/* remove timeouts */

	if (browser->priv->motion_signal != 0) {
		g_signal_handler_disconnect (browser, browser->priv->motion_signal);
		browser->priv->motion_signal = 0;
	}

	if (browser->priv->hide_mouse_timeout != 0) {
		g_source_remove (browser->priv->hide_mouse_timeout);
		browser->priv->hide_mouse_timeout = 0;
	}

	if (browser->priv->construct_step2_event != 0) {
		g_source_remove (browser->priv->construct_step2_event);
		browser->priv->construct_step2_event = 0;
	}

	if (browser->priv->selection_changed_event != 0) {
		g_source_remove (browser->priv->selection_changed_event);
		browser->priv->selection_changed_event = 0;
	}

	if (browser->priv->folder_tree_open_folder_id != 0) {
		g_source_remove (browser->priv->folder_tree_open_folder_id);
		browser->priv->folder_tree_open_folder_id = 0;
	}

	/* cancel async operations */

	_gth_browser_cancel (browser, _gth_browser_close_step3, browser);
}


static void
close__file_saved_cb (GthBrowser *browser,
		      gboolean    cancelled,
		      gpointer    user_data)
{
	if (! cancelled)
		_gth_browser_real_close (browser);
}


static void
_gth_browser_close (GthWindow *window)
{
	GthBrowser *browser = (GthBrowser *) window;

	if (browser->priv->background_tasks != NULL) {
		gtk_window_present (GTK_WINDOW (browser->priv->progress_dialog));
		return;
	}

	if (g_settings_get_boolean (browser->priv->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE)
	    && gth_browser_get_file_modified (browser))
	{
		gth_browser_ask_whether_to_save (browser,
						 close__file_saved_cb,
						 NULL);
	}
	else
		_gth_browser_real_close (browser);
}


/* --- _gth_browser_set_current_page --- */


static GtkWidget *
_gth_browser_get_browser_file_properties_container (GthBrowser *browser)
{
	if (browser->priv->file_properties_on_the_right)
		return browser->priv->browser_right_container;
	else
		return browser->priv->browser_sidebar;
}


static void
_gth_browser_update_header_section_visibility (GthBrowser              *browser,
					       GthBrowserHeaderSection  section,
					       gboolean                 visible)
{
	GtkWidget *header_section;

	header_section = browser->priv->header_sections[section];
	gtk_widget_set_visible (header_section, visible);
}


/* -- hide_mouse_pointer_after_delay -- */


typedef struct {
	GthBrowser *browser;
	GdkDevice  *device;
} HideMouseData;


static gboolean
pointer_on_widget (GtkWidget *widget,
		   GdkDevice *device)
{
	GdkWindow *widget_win;
	int        px, py, w, h;

	if (! gtk_widget_get_visible (widget) || ! gtk_widget_get_realized (widget))
		return FALSE;

	widget_win = gtk_widget_get_window (widget);
	gdk_window_get_device_position (widget_win,
					device,
					&px,
					&py,
					0);
	w = gdk_window_get_width (widget_win);
	h = gdk_window_get_height (widget_win);

	return ((px >= 0) && (px <= w) && (py >= 0) && (py <= h));
}


static gboolean
pointer_on_control (HideMouseData *hmdata,
		    GList         *controls)
{
	GList *scan;

	if (hmdata->device == NULL)
		return FALSE;

	for (scan = controls; scan; scan = scan->next)
		if (pointer_on_widget ((GtkWidget *) scan->data, hmdata->device))
			return TRUE;

	return FALSE;
}


static gboolean
hide_mouse_pointer_cb (gpointer data)
{
	HideMouseData *hmdata = data;
	GthBrowser    *browser = hmdata->browser;

	browser->priv->hide_mouse_timeout = 0;

	/* do not hide the pointer if it's over a viewer control */

	if (browser->priv->keep_mouse_visible
	    || pointer_on_control (hmdata, browser->priv->fixed_viewer_controls)
	    || pointer_on_control (hmdata, browser->priv->viewer_controls)
	    || pointer_on_widget (browser->priv->viewer_sidebar_container, hmdata->device))
	{
		return FALSE;
	}

	browser->priv->pointer_visible = FALSE;
	gtk_widget_hide (browser->priv->fullscreen_toolbar);
	g_list_foreach (browser->priv->fixed_viewer_controls, (GFunc) gtk_widget_hide, NULL);
	_gth_browser_show_pointer_on_viewer (browser, FALSE);

	return FALSE;
}


static void
hide_mouse_pointer_after_delay (GthBrowser *browser,
				GdkDevice  *device)
{
	HideMouseData *hmdata;

	if (browser->priv->hide_mouse_timeout != 0)
		g_source_remove (browser->priv->hide_mouse_timeout);

	hmdata = g_new0 (HideMouseData, 1);
	hmdata->browser = browser;
	hmdata->device = (device != NULL) ? device : _gtk_widget_get_client_pointer (GTK_WIDGET (browser));
	browser->priv->hide_mouse_timeout = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT,
									HIDE_MOUSE_DELAY,
									hide_mouse_pointer_cb,
									hmdata,
									g_free);
}


static gboolean
_gth_browser_can_change_image (GthBrowser *browser)
{
	return ! gth_sidebar_tool_is_active (GTH_SIDEBAR (browser->priv->file_properties));
}


static void
show_fixed_viewer_control (GtkWidget  *control,
		           GthBrowser *browser)
{
	if (! _gth_browser_can_change_image (browser)) {
		if (control == browser->priv->next_image_button)
			return;
		if (control == browser->priv->previous_image_button)
			return;
	}

	gtk_widget_show (control);
}


static gboolean
viewer_motion_notify_event_cb (GtkWidget      *widget,
			       GdkEventMotion *event,
			       gpointer        data)
{
	GthBrowser *browser = data;

	if (! pointer_on_widget (browser->priv->viewer_container, event->device))
		return FALSE;

	if (browser->priv->last_mouse_x == 0.0)
		browser->priv->last_mouse_x = event->x;
	if (browser->priv->last_mouse_y == 0.0)
		browser->priv->last_mouse_y = event->y;

	if ((abs (browser->priv->last_mouse_x - event->x) > MOTION_THRESHOLD)
	    || (abs (browser->priv->last_mouse_y - event->y) > MOTION_THRESHOLD))
	{
		if (! browser->priv->pointer_visible) {
			browser->priv->pointer_visible = TRUE;
			if (browser->priv->fullscreen)
				gtk_widget_show (browser->priv->fullscreen_toolbar);
			g_list_foreach (browser->priv->fixed_viewer_controls, (GFunc) show_fixed_viewer_control, browser);
		}
		_gth_browser_show_pointer_on_viewer (browser, TRUE);
	}

	hide_mouse_pointer_after_delay (browser, event->device);

	browser->priv->last_mouse_x = event->x;
	browser->priv->last_mouse_y = event->y;

	return FALSE;
}


static void
_gth_browser_update_header_bar_content (GthBrowser *browser)
{
	int      page;
	gboolean active_tool;
	gboolean section_visible;

	page = gth_window_get_current_page (GTH_WINDOW (browser));

        section_visible = (page == GTH_BROWSER_PAGE_BROWSER);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_NAVIGATION, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_COMMANDS, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_VIEW, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_METADATA_TOOLS, section_visible);

	active_tool = _gth_browser_file_tool_is_active (browser);
	section_visible = (page == GTH_BROWSER_PAGE_VIEWER) && ! active_tool;
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_NAVIGATION, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_VIEW, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT_METADATA, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW, section_visible);

	section_visible = (page == GTH_BROWSER_PAGE_VIEWER) && active_tool;
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_EDITOR_NAVIGATION, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_EDITOR_VIEW, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS, section_visible);
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_EDITOR_APPLY, section_visible);

	section_visible = (page == GTH_BROWSER_PAGE_VIEWER);
	if (active_tool) {
		GtkWidget *toolbox;
		GtkWidget *file_tool;

		toolbox = gth_sidebar_get_toolbox (GTH_SIDEBAR (browser->priv->file_properties));
		file_tool = gth_toolbox_get_active_tool (GTH_TOOLBOX (toolbox));
		section_visible = gth_file_tool_get_zoomable (GTH_FILE_TOOL (file_tool));
		gtk_button_set_label (GTK_BUTTON (browser->priv->apply_editor_changes_button),
				      gth_file_tool_get_changes_image (GTH_FILE_TOOL (file_tool)) ? _("Accept") : _("_Close"));
	}
	_gth_browser_update_header_section_visibility (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM, section_visible);

	gtk_widget_set_visible (browser->priv->menu_button, ! ((page == GTH_BROWSER_PAGE_VIEWER) && active_tool));

	gth_browser_update_title (browser);
}


static void
_gth_browser_real_set_current_page (GthWindow *window,
				    int        page)
{
	GthBrowser *browser = (GthBrowser *) window;
	int         prev_page;
	int         width;
	int         height;

	prev_page = gth_window_get_current_page (window);
	if (page == prev_page)
		return;

	GTH_WINDOW_CLASS (gth_browser_parent_class)->set_current_page (window, page);

	/* update the ui commands */

	gth_statusbar_show_section (GTH_STATUSBAR (browser->priv->statusbar), (page == GTH_BROWSER_PAGE_BROWSER) ? GTH_STATUSBAR_SECTION_FILE_LIST : GTH_STATUSBAR_SECTION_FILE);
	_gth_browser_hide_infobar (browser);

	if (page == GTH_BROWSER_PAGE_VIEWER) {
		if (browser->priv->viewer_page != NULL) {
			gth_viewer_page_show (browser->priv->viewer_page);
			_gth_browser_show_pointer_on_viewer (browser, FALSE);
			hide_mouse_pointer_after_delay (browser, NULL);

			browser->priv->last_mouse_x = 0.0;
			browser->priv->last_mouse_y = 0.0;
			if (browser->priv->motion_signal == 0)
				browser->priv->motion_signal = g_signal_connect (browser,
										 "motion_notify_event",
										 G_CALLBACK (viewer_motion_notify_event_cb),
										 browser);
		}
	}
	else {
		if (browser->priv->viewer_page != NULL)
			gth_viewer_page_hide (browser->priv->viewer_page);

		if (browser->priv->motion_signal != 0) {
			g_signal_handler_disconnect (browser, browser->priv->motion_signal);
			browser->priv->motion_signal = 0;
		}
		if (browser->priv->hide_mouse_timeout != 0) {
			g_source_remove (browser->priv->hide_mouse_timeout);
			browser->priv->hide_mouse_timeout = 0;
		}
	}

	_gth_browser_update_header_bar_content (browser);
        gtk_widget_set_visible (browser->priv->browser_status_commands, page == GTH_BROWSER_PAGE_BROWSER);
	gtk_widget_set_visible (browser->priv->viewer_status_commands, page == GTH_BROWSER_PAGE_VIEWER);

	/* move the sidebar from the browser to the viewer and vice-versa */

	gtk_widget_unrealize (browser->priv->file_properties);
	if (page == GTH_BROWSER_PAGE_BROWSER) {
		GtkWidget *file_properties_parent;

		file_properties_parent = _gth_browser_get_browser_file_properties_container (browser);
		_gtk_widget_reparent (browser->priv->file_properties, file_properties_parent);
		/* restore the child properties that gtk_widget_reparent doesn't preserve. */
		gtk_container_child_set (GTK_CONTAINER (file_properties_parent),
					 browser->priv->file_properties,
					 "resize", ! browser->priv->file_properties_on_the_right,
					 "shrink", FALSE,
					 NULL);
	}
	else
		_gtk_widget_reparent (browser->priv->file_properties, browser->priv->viewer_sidebar_container);

	/* update the sidebar state depending on the current visible page */

	if (page == GTH_BROWSER_PAGE_BROWSER) {
		gth_sidebar_show_properties (GTH_SIDEBAR (browser->priv->file_properties));
		if (gth_window_get_action_state (GTH_WINDOW (browser), "browser-properties"))
			gth_browser_show_file_properties (browser);
		else
			gth_browser_hide_sidebar (browser);
	}
	else if (page == GTH_BROWSER_PAGE_VIEWER) {
		if (browser->priv->viewer_sidebar == GTH_SIDEBAR_STATE_PROPERTIES)
			gth_browser_show_file_properties (browser);
		else if (browser->priv->viewer_sidebar == GTH_SIDEBAR_STATE_TOOLS)
			gth_browser_show_viewer_tools (browser);
		else
			gth_browser_hide_sidebar (browser);
	}

	/* save the browser window size */

	if ((prev_page == GTH_BROWSER_PAGE_BROWSER) && ! (browser->priv->fullscreen || browser->priv->was_fullscreen)) {
		gtk_window_get_size (GTK_WINDOW (browser), &width, &height);
		gth_window_save_page_size (GTH_WINDOW (browser), prev_page, width, height);
	}

	/* restore the browser window size */

	if ((page == GTH_BROWSER_PAGE_BROWSER) && ! (browser->priv->fullscreen || browser->priv->was_fullscreen))
		gth_window_apply_saved_size (GTH_WINDOW (window), page);

	/* set the focus */

	if (page == GTH_BROWSER_PAGE_BROWSER)
		gth_file_list_focus (GTH_FILE_LIST (browser->priv->file_list));
	else if (page == GTH_BROWSER_PAGE_VIEWER)
		_gth_browser_make_file_visible (browser, browser->priv->current_file);

	/* exit from fullscreen after switching to the browser */

	if ((page == GTH_BROWSER_PAGE_BROWSER) && gth_browser_get_is_fullscreen (browser))
		gth_browser_unfullscreen (browser);

	/* extension hook */

	gth_hook_invoke ("gth-browser-set-current-page", browser);

	/* final updates */

	gth_browser_update_title (browser);
	gth_browser_update_sensitivity (browser);
}


static void
set_current_page__file_saved_cb (GthBrowser *browser,
				 gboolean    cancelled,
				 gpointer    user_data)
{
	if (cancelled)
		return;

	if (browser->priv->current_file != NULL)
		gth_viewer_page_revert (browser->priv->viewer_page);
	_gth_browser_real_set_current_page (GTH_WINDOW (browser), GPOINTER_TO_INT (user_data));
}


static void
_gth_browser_set_current_page (GthWindow *window,
			       int        page)
{
	GthBrowser *browser = GTH_BROWSER (window);

	if (page == gth_window_get_current_page (window))
		return;

	if (g_settings_get_boolean (browser->priv->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE)
	    && gth_browser_get_file_modified (browser))
	{
		gth_browser_ask_whether_to_save (browser,
						 set_current_page__file_saved_cb,
						 GINT_TO_POINTER (page));
	}
	else
		_gth_browser_real_set_current_page (window, page);
}


static void
gth_browser_finalize (GObject *object)
{
	GthBrowser *browser = GTH_BROWSER (object);

	g_list_free (browser->priv->fixed_viewer_controls);
	g_hash_table_destroy (browser->priv->menu_managers);
	browser_state_free (&browser->priv->state);
	_g_object_unref (browser->priv->browser_settings);
	_g_object_unref (browser->priv->messages_settings);
	_g_object_unref (browser->priv->desktop_interface_settings);
	g_free (browser->priv->location_free_space);
	_g_object_unref (browser->priv->location_source);
	_g_object_unref (browser->priv->monitor_location);
	_g_object_unref (browser->priv->location);
	_g_object_unref (browser->priv->current_file);
	_g_object_unref (browser->priv->viewer_page);
	_g_object_unref (browser->priv->image_preloader);
	_g_object_list_unref (browser->priv->viewer_pages);
	_g_object_list_unref (browser->priv->history);
	gth_icon_cache_free (browser->priv->menu_icon_cache);
	g_hash_table_unref (browser->priv->named_dialogs);
	g_object_unref (browser->priv->gesture);
	g_free (browser->priv->list_attributes);
	_g_object_unref (browser->priv->folder_popup_file_data);
	_g_object_unref (browser->priv->history_menu);
	_g_object_unref (browser->priv->screen_profile);
	gtk_tree_path_free (browser->priv->folder_tree_last_dest_row);

	G_OBJECT_CLASS (gth_browser_parent_class)->finalize (object);
}


static void
gth_browser_class_init (GthBrowserClass *klass)
{
	GObjectClass   *gobject_class;
	GthWindowClass *window_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gth_browser_finalize;

	window_class = GTH_WINDOW_CLASS (klass);
	window_class->close = _gth_browser_close;
	window_class->set_current_page = _gth_browser_set_current_page;

	/* signals */

	gth_browser_signals[LOCATION_READY] =
		g_signal_new ("location-ready",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthBrowserClass, location_ready),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_BOOLEAN,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_BOOLEAN);
}


static gboolean
viewer_container_get_child_position_cb (GtkOverlay   *overlay,
					GtkWidget    *widget,
					GdkRectangle *allocation,
					gpointer      user_data)
{
	GthBrowser     *browser = user_data;
	GtkAllocation   main_alloc;
	gboolean        allocation_filled = FALSE;
	gboolean	rtl;

	rtl = gtk_widget_get_direction (GTK_WIDGET (overlay)) == GTK_TEXT_DIR_RTL;

	gtk_widget_get_allocation (gtk_bin_get_child (GTK_BIN (overlay)), &main_alloc);
	gtk_widget_get_preferred_width (widget, NULL, &allocation->width);
	gtk_widget_get_preferred_height (widget, NULL, &allocation->height);

	if (widget == browser->priv->previous_image_button) {
		allocation->x = rtl ? main_alloc.width - allocation->width - OVERLAY_MARGIN :
				OVERLAY_MARGIN;
		allocation->y = (main_alloc.height - allocation->height) / 2;
		allocation_filled = TRUE;
	}
	else if (widget == browser->priv->next_image_button) {
		allocation->x = rtl ? OVERLAY_MARGIN :
				main_alloc.width - allocation->width - OVERLAY_MARGIN;
		allocation->y = (main_alloc.height - allocation->height) / 2;
		allocation_filled = TRUE;
	}

	return allocation_filled;
}


static gboolean
folder_tree_open_folder_cb (gpointer user_data)
{
	GthBrowser *browser = user_data;

	if (browser->priv->folder_tree_open_folder_id != 0) {
		g_source_remove (browser->priv->folder_tree_open_folder_id);
		browser->priv->folder_tree_open_folder_id = 0;
	}

	gtk_tree_view_expand_row (GTK_TREE_VIEW (browser->priv->folder_tree),
				  browser->priv->folder_tree_last_dest_row,
				  FALSE);

	return FALSE;
}


static gboolean
folder_tree_drag_motion_cb (GtkWidget      *file_view,
			    GdkDragContext *context,
			    gint            x,
			    gint            y,
			    guint           time,
			    gpointer        user_data)
{
	GthBrowser              *browser = user_data;
	GtkTreePath             *path;
	GtkTreeViewDropPosition  pos;

	if (gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK) {
		gdk_drag_status (context, GDK_ACTION_ASK, time);
		return FALSE;
	}

	if (! gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (file_view),
					         x,
					         y,
					         &path,
					         &pos))
	{
		gtk_tree_path_free (browser->priv->folder_tree_last_dest_row);
		browser->priv->folder_tree_last_dest_row = NULL;

		if (browser->priv->folder_tree_open_folder_id != 0) {
			g_source_remove (browser->priv->folder_tree_open_folder_id);
			browser->priv->folder_tree_open_folder_id = 0;
		}

		gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (file_view), NULL, 0);
		gdk_drag_status (context, 0, time);
		return TRUE;
	}

	if (pos == GTK_TREE_VIEW_DROP_BEFORE)
		pos = GTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
	if (pos == GTK_TREE_VIEW_DROP_AFTER)
		pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;

	gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (file_view), path, pos);

	if ((browser->priv->folder_tree_last_dest_row == NULL) || gtk_tree_path_compare (path, browser->priv->folder_tree_last_dest_row) != 0) {
		gtk_tree_path_free (browser->priv->folder_tree_last_dest_row);
		browser->priv->folder_tree_last_dest_row = gtk_tree_path_copy (path);

		if (browser->priv->folder_tree_open_folder_id != 0)
			g_source_remove (browser->priv->folder_tree_open_folder_id);
		browser->priv->folder_tree_open_folder_id = g_timeout_add (AUTO_OPEN_FOLDER_DELAY, folder_tree_open_folder_cb, browser);
	}

	gdk_drag_status (context, GDK_ACTION_MOVE, time);
	gtk_tree_path_free (path);

	return TRUE;
}


static void
folder_tree_drag_data_received (GtkWidget        *tree_view,
				GdkDragContext   *context,
				int               x,
				int               y,
				GtkSelectionData *selection_data,
				guint             info,
				guint             time,
				gpointer          user_data)
{
	GthBrowser     *browser = user_data;
	gboolean        success = FALSE;
	GdkDragAction   suggested_action;
	GtkTreePath    *path;
	GthFileData    *destination;
	char          **uris;
	GList          *file_list;

	suggested_action = gdk_drag_context_get_suggested_action (context);

	if ((suggested_action == GDK_ACTION_COPY)
	    || (suggested_action == GDK_ACTION_MOVE)
	    || (suggested_action == GDK_ACTION_ASK))
	{
		success = TRUE;
	}

	if (! gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (browser->priv->folder_tree),
						 x, y,
						 &path,
						 NULL))
	{
		success = FALSE;
	}

	if (success && (suggested_action == GDK_ACTION_ASK)) {
		GdkDragAction action = _gtk_menu_ask_drag_drop_action (tree_view, gdk_drag_context_get_actions (context));
		gdk_drag_status (context, action, time);
		success = gdk_drag_context_get_selected_action (context) != 0;
	}

	if (! success) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	destination = gth_folder_tree_get_file (GTH_FOLDER_TREE (browser->priv->folder_tree), path);
	uris = gtk_selection_data_get_uris (selection_data);
	file_list = _g_file_list_new_from_uriv (uris);
	if (file_list != NULL)
		gth_hook_invoke ("gth-browser-folder-tree-drag-data-received",
				 browser,
				 destination,
				 file_list,
				 gdk_drag_context_get_selected_action (context));

	gtk_drag_finish (context, TRUE, FALSE, time);

	_g_object_list_unref (file_list);
	g_strfreev (uris);
	_g_object_unref (destination);
}


static void
folder_tree_drag_data_get_cb (GtkWidget        *widget,
			      GdkDragContext   *drag_context,
			      GtkSelectionData *selection_data,
			      guint             info,
			      guint             time,
			      gpointer          user_data)
{
	GthBrowser     *browser = user_data;
	GthFileData    *file_data;
	GthFileSource  *file_source;
	char          **uris;

	file_data = gth_folder_tree_get_selected (GTH_FOLDER_TREE (browser->priv->folder_tree));
	if (file_data == NULL)
		return;

	file_source = gth_main_get_file_source (file_data->file);
	if (file_source == NULL)
		return;

	if (gdk_drag_context_get_actions (drag_context) & GDK_ACTION_MOVE) {
		GdkDragAction action = gth_file_source_can_cut (file_source) ? GDK_ACTION_MOVE : GDK_ACTION_COPY;
		gdk_drag_status (drag_context, action, time);
	}

	uris = g_new (char *, 2);
	uris[0] = g_file_get_uri (file_data->file);
	uris[1] = NULL;
	gtk_selection_data_set_uris (selection_data, uris);

	g_strfreev (uris);
	g_object_unref (file_source);
	g_object_unref (file_data);
}


static void
folder_tree_open_cb (GthFolderTree *folder_tree,
		     GFile         *file,
		     GthBrowser    *browser)
{
	_gth_browser_load (browser, file, NULL, NULL, 0, GTH_ACTION_TREE_OPEN, FALSE);
}


static void
folder_tree_open_parent_cb (GthFolderTree *folder_tree,
			    GthBrowser    *browser)
{
	gth_browser_go_up (browser, 1);
}


static void
folder_tree_list_children_cb (GthFolderTree *folder_tree,
			      GFile         *file,
			      GthBrowser    *browser)
{
	_gth_browser_load (browser, file, NULL, NULL, 0, GTH_ACTION_TREE_LIST_CHILDREN, FALSE);
}


static void
folder_tree_folder_popup_cb (GthFolderTree *folder_tree,
			     GthFileData   *file_data,
			     guint          time,
			     gpointer       user_data)
{
	GthBrowser    *browser = user_data;
	GthFileSource *file_source;

	gth_window_enable_action (GTH_WINDOW (browser), "open-folder-in-new-window", (file_data != NULL));

	_g_object_unref (browser->priv->folder_popup_file_data);
	browser->priv->folder_popup_file_data = _g_object_ref (file_data);

	if (file_data != NULL)
		file_source = gth_main_get_file_source (file_data->file);
	else
		file_source = NULL;
	gth_hook_invoke ("gth-browser-folder-tree-popup-before", browser, file_source, file_data);

	gtk_menu_popup_at_pointer (GTK_MENU (browser->priv->folder_popup), NULL);

	if (file_data != NULL) {
		GtkTreePath *path;

		path = gth_folder_tree_get_path (GTH_FOLDER_TREE (browser->priv->folder_tree), file_data->file);
		gth_folder_tree_select_path (GTH_FOLDER_TREE (browser->priv->folder_tree), path);

		gtk_tree_path_free (path);
	}

	_g_object_unref (file_source);
}


static void
folder_popup_hide_cb (GtkWidget *widget,
		      gpointer   user_data)
{
	GthBrowser  *browser = user_data;
	GtkTreePath *path;

	if (browser->priv->location == NULL)
		return;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (browser->priv->folder_tree), browser->priv->location->file);
	if (path != NULL) {
		gth_folder_tree_select_path (GTH_FOLDER_TREE (browser->priv->folder_tree), path);
		gtk_tree_path_free (path);
	}
}


GthFileData *
gth_browser_get_folder_popup_file_data (GthBrowser *browser)
{
	return _g_object_ref (browser->priv->folder_popup_file_data);
}


static void
file_source_rename_ready_cb (GObject  *object,
			     GError   *error,
			     gpointer  user_data)
{
	GthBrowser *browser = user_data;

	g_object_unref (object);

	if (error != NULL)
		_gth_browser_show_error (browser, _("Could not change name"), error);
}


static void
folder_tree_rename_cb (GthFolderTree *folder_tree,
		       GFile         *file,
		       const char    *new_name,
		       GthBrowser    *browser)
{
	GthFileSource *file_source;

	file_source = gth_main_get_file_source (file);
	gth_file_source_rename (file_source,
				file,
				new_name,
				file_source_rename_ready_cb,
				browser);
}


static void
toolbox_options_visibility_cb (GthToolbox *toolbox,
			       gboolean    toolbox_options_visible,
			       GthBrowser *browser)
{
	_gth_browser_update_header_bar_content (browser);

	if (toolbox_options_visible) {
		GtkWidget *file_tool;

		gth_browser_show_viewer_tools (browser);

		gtk_widget_hide (browser->priv->next_image_button);
		gtk_widget_hide (browser->priv->previous_image_button);
		browser->priv->pointer_visible = FALSE;

		file_tool = gth_toolbox_get_active_tool (toolbox);
		if (file_tool != NULL)
			gth_file_tool_populate_headerbar (GTH_FILE_TOOL (file_tool), browser);
	}
	else {
		if (browser->priv->pointer_visible) {
			gtk_widget_show (browser->priv->next_image_button);
			gtk_widget_show (browser->priv->previous_image_button);
		}
		_gtk_container_remove_children (GTK_CONTAINER (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS]), NULL, NULL);
	}
}


static void
filterbar_changed_cb (GthFilterbar *filterbar,
		      GthBrowser   *browser)
{
	GthTest *filter;

	filter = _gth_browser_get_file_filter (browser);
	gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->file_list), filter);
	g_object_unref (filter);

	_gth_browser_update_statusbar_list_info (browser);
	gth_browser_update_sensitivity (browser);

	if (_gth_browser_reload_required (browser))
		gth_browser_reload (browser);
	else if (browser->priv->current_file != NULL)
		gth_file_list_make_file_visible (GTH_FILE_LIST (browser->priv->file_list), browser->priv->current_file->file);
}


static void
filterbar_personalize_cb (GthFilterbar *filterbar,
			  GthBrowser   *browser)
{
	dlg_personalize_filters (browser);
}


static void
_gth_browser_change_file_list_order (GthBrowser *browser,
				     int        *new_order)
{
	g_file_info_set_attribute_string (browser->priv->location->info, "sort::type", "general::unsorted");
	g_file_info_set_attribute_boolean (browser->priv->location->info, "sort::inverse", FALSE);
	gth_file_store_reorder (GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (gth_browser_get_file_list_view (browser)))), new_order);
	_gth_browser_update_current_file_position (browser);
	gth_browser_update_title (browser);
}


static void
file_attributes_ready_cb (GthFileSource *file_source,
			  GList         *files,
			  GError        *error,
			  gpointer       user_data)
{
	MonitorEventData *monitor_data = user_data;
	GthBrowser       *browser = monitor_data->browser;
	GList            *visible_folders;

	if (error != NULL) {
		monitor_event_data_unref (monitor_data);
		g_clear_error (&error);
		return;
	}

	visible_folders = _gth_browser_get_visible_files (browser, files);

	if (monitor_data->event == GTH_MONITOR_EVENT_CREATED) {
		if (monitor_data->update_folder_tree)
			gth_folder_tree_add_children (GTH_FOLDER_TREE (browser->priv->folder_tree), monitor_data->parent, visible_folders);
		if (monitor_data->update_file_list) {
			if (monitor_data->position >= 0)
				_gth_browser_set_sort_order (browser,
							     gth_main_get_sort_type ("general::unsorted"),
							     FALSE,
							     FALSE,
							     TRUE);
			gth_file_list_add_files (GTH_FILE_LIST (browser->priv->file_list), files, monitor_data->position);
			gth_file_list_update_files (GTH_FILE_LIST (browser->priv->file_list), files);
		}
	}
	else if (monitor_data->event == GTH_MONITOR_EVENT_CHANGED) {
		if (monitor_data->update_folder_tree)
			gth_folder_tree_update_children (GTH_FOLDER_TREE (browser->priv->folder_tree), monitor_data->parent, visible_folders);
		if (monitor_data->update_file_list)
			gth_file_list_update_files (GTH_FILE_LIST (browser->priv->file_list), files);
	}

	if (browser->priv->current_file != NULL) {
		GList *link;

		link = gth_file_data_list_find_file (files, browser->priv->current_file->file);
		if (link != NULL) {
			GthFileData *file_data = link->data;
			gth_browser_load_file (browser, file_data, FALSE);
		}
	}

	_gth_browser_update_current_file_position (browser);
	gth_browser_update_title (browser);
	gth_browser_update_sensitivity (browser);
	_gth_browser_update_statusbar_list_info (browser);

	_g_object_list_unref (visible_folders);
	monitor_event_data_unref (monitor_data);
}


static gboolean
_g_file_list_only_contains (GList *l,
			    GFile *file)
{
	return (l != NULL) && (l->next == NULL) && _g_file_equal (file, G_FILE (l->data));
}


static GList *
_g_file_list_find_file_or_ancestor (GList *l,
				    GFile *file)
{
	GList *link = NULL;
	GList *scan;

	for (scan = l; (link == NULL) && scan; scan = scan->next) {
		GFile *parent;

		parent = g_object_ref (file);
		while ((parent != NULL) && ! g_file_equal (parent, (GFile *) scan->data)) {
			GFile *tmp;

			tmp = g_file_get_parent (parent);
			g_object_unref (parent);
			parent = tmp;
		}

		if (parent != NULL) {
			link = scan;
			g_object_unref (parent);
		}
	}

	return link;
}


static void _gth_browser_load_file_more_options (GthBrowser  *browser,
						 GthFileData *file_data,
						 gboolean     view,
						 gboolean     fullscreen,
						 gboolean     no_delay);


static void
folder_changed_cb (GthMonitor      *monitor,
		   GFile           *parent,
		   GList           *list,
		   int              position,
		   GthMonitorEvent  event,
		   GthBrowser      *browser)
{
	GtkTreePath *path;
	gboolean     update_folder_tree;
	gboolean     update_file_list;

	if (browser->priv->location == NULL)
		return;

	if ((event == GTH_MONITOR_EVENT_DELETED)
		&& _g_file_list_only_contains (list, browser->priv->location->file))
	{
		/* current location deleted -> load the previous location in
		 * the folder tree. */

		GtkTreePath *location_path;

		location_path = gth_folder_tree_get_path (GTH_FOLDER_TREE (browser->priv->folder_tree), browser->priv->location->file);
		if (location_path != NULL) {
			GtkTreePath *prev_path;

			prev_path = _gtk_tree_path_get_previous_or_parent (location_path);
			if (prev_path != NULL) {
				GthFileData *prev_file;

				prev_file = gth_folder_tree_get_file (GTH_FOLDER_TREE (browser->priv->folder_tree), prev_path);
				if (prev_file != NULL) {
					_gth_browser_load (browser, prev_file->file, NULL, NULL, 0, GTH_ACTION_GO_TO, TRUE);
					_g_object_unref (prev_file);
				}

				gtk_tree_path_free (prev_path);
			}

			gtk_tree_path_free (location_path);
		}
	}
	else if ((event == GTH_MONITOR_EVENT_DELETED)
		&& (_g_file_list_find_file_or_ancestor (list, browser->priv->location->file) != NULL))
	{
		_gth_browser_load (browser, parent, NULL, NULL, 0, GTH_ACTION_GO_TO, TRUE);
	}
	else if ((event == GTH_MONITOR_EVENT_CHANGED)
		&& (_g_file_list_find_file_or_ancestor (list, browser->priv->location->file) != NULL))
	{
		_gth_browser_load (browser, browser->priv->location->file, NULL, NULL, 0, GTH_ACTION_GO_TO, TRUE);
	}

#if 0
{
	GList *scan;
	g_print ("folder changed: %s [%s]\n", g_file_get_uri (parent), _g_enum_type_get_value (GTH_TYPE_MONITOR_EVENT, event)->value_nick);
	for (scan = list; scan; scan = scan->next)
		g_print ("   %s\n", g_file_get_uri (scan->data));
}
#endif

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (browser->priv->folder_tree), parent);
	update_folder_tree = (g_file_equal (parent, gth_folder_tree_get_root (GTH_FOLDER_TREE (browser->priv->folder_tree)))
			      || ((path != NULL) && gth_folder_tree_is_loaded (GTH_FOLDER_TREE (browser->priv->folder_tree), path)));

	update_file_list = g_file_equal (parent, browser->priv->location->file);
	if (! update_file_list && (event != GTH_MONITOR_EVENT_CREATED)) {
		GthFileStore *file_store;
		GList        *scan;

		file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list))));
		for (scan = list; scan; scan = scan->next) {
			if (gth_file_store_find_visible (file_store, (GFile *) scan->data, NULL)) {
				update_file_list = TRUE;
				break;
			}
		}
	}

	if ((event == GTH_MONITOR_EVENT_CREATED)
		&& _g_file_list_only_contains (list, browser->priv->location->file))
	{
		gth_file_list_set_files (GTH_FILE_LIST (browser->priv->file_list), NULL);
	}
	else if (update_folder_tree || update_file_list) {
		MonitorEventData *monitor_data;
		gboolean          current_file_deleted = FALSE;
		GthFileData      *new_file = NULL;

		browser->priv->recalc_location_free_space = TRUE;

		switch (event) {
		case GTH_MONITOR_EVENT_CREATED:
		case GTH_MONITOR_EVENT_CHANGED:
			monitor_data = monitor_event_data_new ();
			monitor_data->file_source = gth_main_get_file_source (parent);
			monitor_data->parent = g_file_dup (parent);
			monitor_data->position = position;
			monitor_data->event = event;
			monitor_data->browser = browser;
			monitor_data->update_file_list = update_file_list;
			monitor_data->update_folder_tree = update_folder_tree;
			gth_file_source_read_attributes (monitor_data->file_source,
						 	 list,
						 	 _gth_browser_get_list_attributes (browser, FALSE),
						 	 file_attributes_ready_cb,
						 	 monitor_data);
			break;

		case GTH_MONITOR_EVENT_REMOVED:
		case GTH_MONITOR_EVENT_DELETED:
			if ((event == GTH_MONITOR_EVENT_REMOVED) && ! g_file_equal (parent, browser->priv->location->file))
				break;

			if (browser->priv->current_file != NULL) {
				GList *link;

				link = _g_file_list_find_file (list, browser->priv->current_file->file);
				if (link != NULL) {
					GthFileStore *file_store;
					GtkTreeIter   iter;
					gboolean      found = FALSE;

					current_file_deleted = TRUE;

					file_store = gth_browser_get_file_store (browser);
					if (gth_file_store_find_visible (file_store, browser->priv->current_file->file, &iter)) {
						if (gth_file_store_get_next_visible (file_store, &iter))
							found = TRUE;
						else if (gth_file_store_get_prev_visible (file_store, &iter))
							found = TRUE;
					}

					if (found)
						new_file = g_object_ref (gth_file_store_get_file (file_store, &iter));
				}
			}

			if (update_folder_tree)
				gth_folder_tree_delete_children (GTH_FOLDER_TREE (browser->priv->folder_tree), parent, list);

			if (update_file_list) {
				if (current_file_deleted)
					_g_signal_handlers_block_by_data (gth_browser_get_file_list_view (browser), browser);
				gth_file_list_delete_files (GTH_FILE_LIST (browser->priv->file_list), list);
				if (event == GTH_MONITOR_EVENT_DELETED)
					gth_file_source_deleted_from_disk (browser->priv->location_source, browser->priv->location, list);
				if (current_file_deleted)
					_g_signal_handlers_unblock_by_data (gth_browser_get_file_list_view (browser), browser);
			}

			if (current_file_deleted && ! gth_browser_get_file_modified (browser)) {
				g_file_info_set_attribute_boolean (browser->priv->current_file->info, "gth::file::is-modified", FALSE);

				if (new_file != NULL) {
					if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER)
						gth_browser_load_file (browser, new_file, FALSE);
					else if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER)
						gth_file_list_make_file_visible (GTH_FILE_LIST (browser->priv->file_list), new_file->file);

					_g_object_unref (new_file);
				}
				else {
					gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
					_gth_browser_load_file_more_options (browser, NULL, FALSE, FALSE, FALSE);
				}
			}

			_gth_browser_update_statusbar_list_info (browser);
			gth_browser_update_sensitivity (browser);
			break;

		default:
			break;
		}
	}

	gtk_tree_path_free (path);
}


typedef struct {
	GthBrowser    *browser;
	GthFileSource *file_source;
	GFile         *file;
	GFile         *new_file;
} RenameData;


static void
rename_data_free (RenameData *rename_data)
{
	g_object_unref (rename_data->file_source);
	g_object_unref (rename_data->file);
	g_object_unref (rename_data->new_file);
	g_free (rename_data);
}


static void
renamed_file_attributes_ready_cb (GthFileSource *file_source,
				  GList         *files,
				  GError        *error,
				  gpointer       user_data)
{
	RenameData  *rename_data = user_data;
	GthBrowser  *browser = rename_data->browser;
	GthFileData *file_data;

	if (error != NULL) {
		rename_data_free (rename_data);
		g_clear_error (&error);
		return;
	}

	file_data = files->data;

	gth_folder_tree_update_child (GTH_FOLDER_TREE (browser->priv->folder_tree), rename_data->file, file_data);
	gth_file_list_rename_file (GTH_FILE_LIST (browser->priv->file_list), rename_data->file, file_data);

	if (g_file_equal (rename_data->file, browser->priv->location->file)) {
		GthFileData *new_location;
		GFileInfo   *new_info;

		new_location = gth_file_data_new (rename_data->new_file, browser->priv->location->info);
		new_info = gth_file_source_get_file_info (rename_data->file_source, new_location->file, GFILE_DISPLAY_ATTRIBUTES);
		_g_file_info_update (new_location->info, new_info);

		_gth_browser_update_location (browser, new_location);

		g_object_unref (new_info);
		g_object_unref (new_location);
	}
	else if ((browser->priv->current_file != NULL) && g_file_equal (rename_data->file, browser->priv->current_file->file) && ! gth_browser_get_file_modified (browser))
		gth_browser_load_file (browser, file_data, FALSE);

	rename_data_free (rename_data);
}


static void
file_renamed_cb (GthMonitor *monitor,
		 GFile      *file,
		 GFile      *new_file,
		 GthBrowser *browser)
{
	RenameData *rename_data;
	GList      *list;

	gth_hook_invoke ("gth-browser-file-renamed", browser, file, new_file);

	rename_data = g_new0 (RenameData, 1);
	rename_data->browser = browser;
	rename_data->file_source = gth_main_get_file_source (new_file);
	rename_data->file = g_file_dup (file);
	rename_data->new_file = g_file_dup (new_file);

	list = g_list_prepend (NULL, rename_data->new_file);
	gth_file_source_read_attributes (rename_data->file_source,
				 	 list,
				 	 _gth_browser_get_list_attributes (browser, FALSE),
				 	 renamed_file_attributes_ready_cb,
				 	 rename_data);

	g_list_free (list);
}


void
gth_browser_update_statusbar_file_info (GthBrowser *browser)
{
	const char  *extra_info;
	const char  *image_size;
	const char  *file_size;
	GString     *status;

	if (browser->priv->current_file == NULL) {
		gth_statusbar_set_primary_text (GTH_STATUSBAR (browser->priv->statusbar), "");
		gth_statusbar_set_secondary_text (GTH_STATUSBAR (browser->priv->statusbar), "");
		return;
	}

	extra_info = g_file_info_get_attribute_string (browser->priv->current_file->info, "gthumb::statusbar-extra-info");
	image_size = g_file_info_get_attribute_string (browser->priv->current_file->info, "general::dimensions");
	file_size = g_file_info_get_attribute_string (browser->priv->current_file->info, "gth::file::display-size");

	status = g_string_new ("");

	if (browser->priv->current_file_position >= 0)
		g_string_append_printf (status, "%d/%d", browser->priv->current_file_position + 1, browser->priv->n_visibles);

	if (image_size != NULL) {
		g_string_append (status, STATUSBAR_SEPARATOR);
		g_string_append (status, image_size);
	}

	if (gth_browser_get_file_modified (browser)) {
		g_string_append (status, STATUSBAR_SEPARATOR);
		g_string_append (status, _("Modified"));
	}
	else {
		if (file_size != NULL) {
			g_string_append (status, STATUSBAR_SEPARATOR);
			g_string_append (status, file_size);
		}
	}

	if (extra_info != NULL) {
		g_string_append (status, STATUSBAR_SEPARATOR);
		g_string_append (status, extra_info);
	}

	gth_statusbar_set_primary_text (GTH_STATUSBAR (browser->priv->statusbar), status->str);

	g_string_free (status, TRUE);
}


static void
metadata_changed_cb (GthMonitor  *monitor,
		     GthFileData *file_data,
		     GthBrowser  *browser)
{
	if ((browser->priv->location != NULL) && g_file_equal (browser->priv->location->file, file_data->file)) {
		GtkWidget *location_chooser;

		if (file_data->info != browser->priv->location->info)
			g_file_info_copy_into (file_data->info, browser->priv->location->info);

		location_chooser = gth_location_bar_get_chooser (GTH_LOCATION_BAR (browser->priv->location_bar));
		gth_location_chooser_reload (GTH_LOCATION_CHOOSER (location_chooser));

		_gth_browser_update_location (browser, browser->priv->location);
	}

	if ((browser->priv->current_file != NULL) && g_file_equal (browser->priv->current_file->file, file_data->file)) {
		if (file_data->info != browser->priv->current_file->info)
			g_file_info_copy_into (file_data->info, browser->priv->current_file->info);

		gth_sidebar_set_file (GTH_SIDEBAR (browser->priv->file_properties), browser->priv->current_file);

		gth_browser_update_statusbar_file_info (browser);
		gth_browser_update_title (browser);
		gth_browser_update_sensitivity (browser);
	}

	gth_folder_tree_update_child (GTH_FOLDER_TREE (browser->priv->folder_tree), file_data->file, file_data);
}


typedef struct {
	GthBrowser    *browser;
	GthFileSource *file_source;
	GList         *files;
} EmblemsData;


static void
emblems_data_free (EmblemsData *data)
{
	_g_object_list_unref (data->files);
	g_object_unref (data->file_source);
	g_free (data);
}


static void
emblems_attributes_ready_cb (GthFileSource *file_source,
			     GList         *files,
			     GError        *error,
			     gpointer       user_data)
{
	EmblemsData *data = user_data;

	if (error == NULL) {
		GthBrowser *browser = data->browser;

		gth_file_list_update_emblems (GTH_FILE_LIST (browser->priv->file_list), files);

		if (browser->priv->current_file != NULL) {
			GList *link;

			link = gth_file_data_list_find_file (files, browser->priv->current_file->file);
			if (link != NULL) {
				GthFileData *current_file_data = link->data;
				GObject     *emblems;

				emblems = g_file_info_get_attribute_object (current_file_data->info, GTH_FILE_ATTRIBUTE_EMBLEMS);
				g_file_info_set_attribute_object (browser->priv->current_file->info, GTH_FILE_ATTRIBUTE_EMBLEMS, emblems);
				gth_browser_update_title (browser);
			}
		}
	}

	emblems_data_free (data);
}


static void
emblems_changed_cb (GthMonitor  *monitor,
		    GList       *files, /* GFile list */
		    GthBrowser  *browser)
{
	EmblemsData *data;

	if (browser->priv->location_source == NULL)
		return;

	data = g_new0 (EmblemsData, 1);
	data->browser = browser;
	data->file_source = g_object_ref (browser->priv->location_source);
	data->files = _g_object_list_ref (files);

	gth_file_source_read_attributes (browser->priv->location_source,
					 files,
					 GTH_FILE_ATTRIBUTE_EMBLEMS,
					 emblems_attributes_ready_cb,
					 data);
}


static void
entry_points_changed_cb (GthMonitor *monitor,
			 GthBrowser *browser)
{
	call_when_idle ((DataFunc) _gth_browser_update_entry_point_list, browser);
}


static void
order_changed_cb (GthMonitor *monitor,
		  GFile      *file,
		  int        *new_order,
		  GthBrowser *browser)
{
	if ((browser->priv->location != NULL) && g_file_equal (file, browser->priv->location->file))
		_gth_browser_change_file_list_order (browser, new_order);
}


static void
shortcuts_changed_cb (GthMonitor *monitor,
		      GthBrowser *browser)
{
	gth_window_load_shortcuts (GTH_WINDOW (browser));
}


static void
pref_general_filter_changed (GSettings  *settings,
			     const char *key,
			     gpointer    user_data)
{
	GthBrowser *browser = user_data;
	GthTest    *filter;

	filter = _gth_browser_get_file_filter (browser);
	gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->file_list), filter);

	g_object_unref (filter);
}


static void
gth_file_list_popup_menu (GthBrowser     *browser,
			  GdkEventButton *event)
{
	gth_hook_invoke ("gth-browser-file-list-popup-before", browser);
	gtk_menu_popup_at_pointer (GTK_MENU (browser->priv->file_list_popup), (GdkEvent *) event);
}


static void
location_chooser_changed_cb (GthLocationChooser *chooser,
			     gpointer            user_data)
{
	gth_browser_go_to (GTH_BROWSER (user_data), gth_location_chooser_get_current (chooser), NULL);
}


static gboolean
gth_file_list_button_press_cb  (GtkWidget      *widget,
				GdkEventButton *event,
				gpointer        user_data)
{
	GthBrowser *browser = user_data;

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		GtkWidget *file_view;
		int        pos;

		file_view = gth_browser_get_file_list_view (browser);
		pos = gth_file_view_get_at_position (GTH_FILE_VIEW (file_view), event->x, event->y);
		if ((pos >= 0) && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (file_view), pos)) {
			gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_view));
			gth_file_selection_select (GTH_FILE_SELECTION (file_view), pos);
			gth_file_view_set_cursor (GTH_FILE_VIEW (file_view), pos);
		}
		gth_file_list_popup_menu (browser, event);

		return FALSE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 2)) {
		GtkWidget *file_view;
		int        pos;

		file_view = gth_browser_get_file_list_view (browser);
		pos = gth_file_view_get_at_position (GTH_FILE_VIEW (file_view), event->x, event->y);
		if ((pos >= 0) && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (file_view), pos)) {
			gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_view));
			gth_file_selection_select (GTH_FILE_SELECTION (file_view), pos);
			gth_file_view_set_cursor (GTH_FILE_VIEW (file_view), pos);
		}
		return FALSE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1) && (event->state & GDK_MOD1_MASK)) {
		GtkWidget *file_view;
		int        pos;

		file_view = gth_browser_get_file_list_view (browser);
		pos = gth_file_view_get_at_position (GTH_FILE_VIEW (file_view), event->x, event->y);
		if (pos >= 0) {
			GtkTreeModel *file_store = gth_file_view_get_model (GTH_FILE_VIEW (file_view));
			GtkTreeIter   iter;

			if (gth_file_store_get_nth_visible (GTH_FILE_STORE (file_store), pos, &iter)) {
				GthFileData *file_data;

				file_data = gth_file_store_get_file (GTH_FILE_STORE (file_store), &iter);
				_gth_browser_load_file_more_options (browser, file_data, TRUE, ! browser->priv->view_files_in_fullscreen, TRUE);
			}
		}
	}
	else if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 2)) {
		gth_browser_fullscreen (browser);
		return TRUE;
	}

	return FALSE;
}


static gboolean
gth_file_list_popup_menu_cb (GtkWidget *widget,
			     gpointer   user_data)
{
	gth_file_list_popup_menu (GTH_BROWSER (user_data), NULL);
	return TRUE;
}


static gboolean
update_selection_cb (gpointer user_data)
{
	GthBrowser *browser = user_data;
	int         n_selected;

	g_source_remove (browser->priv->selection_changed_event);
	browser->priv->selection_changed_event = 0;

	if (browser->priv->closing)
		return FALSE;

	gth_browser_update_sensitivity (browser);
	_gth_browser_update_statusbar_list_info (browser);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	gth_hook_invoke ("gth-browser-selection-changed", browser, n_selected);

	if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_BROWSER_PAGE_BROWSER)
		return FALSE;

	if (n_selected == 1) {
		GList       *items;
		GList       *file_list;
		GthFileData *selected_file_data;

		items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
		file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
		selected_file_data = (GthFileData *) file_list->data;
		gth_browser_load_file (browser, selected_file_data, FALSE);

		_g_object_list_unref (file_list);
		_gtk_tree_path_list_free (items);
	}
	else
		gth_browser_load_file (browser, NULL, FALSE);

	return FALSE;
}


static void
gth_file_view_selection_changed_cb (GtkIconView *iconview,
				    gpointer     user_data)
{
	GthBrowser *browser = user_data;

	if (browser->priv->closing)
		return;

	if (browser->priv->selection_changed_event != 0)
		g_source_remove (browser->priv->selection_changed_event);

	browser->priv->selection_changed_event = g_timeout_add (UPDATE_SELECTION_DELAY,
								update_selection_cb,
								browser);
}


static void
gth_thumbnail_view_selection_changed_cb (GtkIconView *iconview,
					 gpointer     user_data)
{
	GthBrowser *browser = user_data;
	int         n_selected;

	if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_BROWSER_PAGE_VIEWER)
		return;

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_thumbnail_list_view (browser)));
	if (n_selected == 1) {
		GList       *items;
		GList       *file_list;
		GthFileData *selected_file_data;

		items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_thumbnail_list_view (browser)));
		file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
		selected_file_data = (GthFileData *) file_list->data;
		gth_browser_load_file (browser, selected_file_data, FALSE);

		_g_object_list_unref (file_list);
		_gtk_tree_path_list_free (items);
	}
}


static void
gth_file_view_file_activated_cb (GthFileView *file_view,
				 GtkTreePath *path,
				 gpointer     user_data)
{
	GthBrowser   *browser = user_data;
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_store = gth_browser_get_file_store (browser);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (file_store), &iter, path)) {
		GthFileData *file_data;

		file_data = gth_file_store_get_file (file_store, &iter);
		if (file_data != NULL)
			_gth_browser_load_file_more_options (browser, file_data, TRUE, browser->priv->view_files_in_fullscreen, TRUE);
	}
}


static gboolean
gth_browser_file_list_key_press_cb (GthBrowser  *browser,
			            GdkEventKey *event)
{
	gboolean activated;

	activated = gth_window_activate_shortcut (GTH_WINDOW (browser),
						  GTH_SHORTCUT_CONTEXT_BROWSER,
						  NULL,
						  event->keyval,
						  event->state);

	if (! activated)
		activated = gth_hook_invoke_get ("gth-browser-file-list-key-press", browser, event) != NULL;

	return activated;
}


typedef struct {
	GthBrowser *browser;
	GFile      *location;
	GFile      *file_to_select;
} NewWindowData;


static void
new_window_data_free (gpointer user_data)
{
	NewWindowData *data = user_data;

	_g_object_unref (data->location);
	_g_object_unref (data->file_to_select);
	g_free (data);
}


static void
_gth_browser_construct_step2 (gpointer user_data)
{
	NewWindowData *data = user_data;
	GthBrowser    *browser = data->browser;

	browser->priv->construct_step2_event = 0;

	_gth_browser_update_entry_point_list (browser);
	_gth_browser_monitor_entry_points (browser);

	gth_hook_invoke ("gth-browser-construct-idle-callback", browser);
	gth_hook_invoke ("gth-browser-selection-changed", browser, 0);

	if (data->file_to_select != NULL)
		gth_browser_go_to (browser, data->location, data->file_to_select);
	else
		gth_browser_load_location (browser, data->location);

	new_window_data_free (data);
}


static void
pref_browser_properties_on_the_right_changed (GSettings  *settings,
					      const char *key,
					      gpointer    user_data)
{
	GthBrowser *browser = user_data;
	GtkWidget  *old_parent;
	GtkWidget  *new_parent;

	old_parent = _gth_browser_get_browser_file_properties_container (browser);
	browser->priv->file_properties_on_the_right = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT);
	new_parent = _gth_browser_get_browser_file_properties_container (browser);

	if (old_parent == new_parent)
		return;

	if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_BROWSER_PAGE_BROWSER)
		return;

	gtk_widget_unrealize (browser->priv->file_properties);
	_gtk_widget_reparent (browser->priv->file_properties, new_parent);
	/* restore the child properties that gtk_widget_reparent doesn't preserve. */
	gtk_container_child_set (GTK_CONTAINER (new_parent),
				 browser->priv->file_properties,
				 "resize", ! browser->priv->file_properties_on_the_right,
				 "shrink", FALSE,
				 NULL);
}


static void
pref_ui_viewer_thumbnails_orient_changed (GSettings  *settings,
					  const char *key,
					  gpointer    user_data)
{
	GthBrowser     *browser = user_data;
	GtkOrientation  viewer_thumbnails_orientation;
	GtkWidget      *viewer_thumbnails_pane;
	GtkWidget      *child1;
	GtkWidget      *child2;

	viewer_thumbnails_orientation = g_settings_get_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT);
	if (viewer_thumbnails_orientation == browser->priv->viewer_thumbnails_orientation)
		return;
	browser->priv->viewer_thumbnails_orientation = viewer_thumbnails_orientation;
	if (viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		viewer_thumbnails_pane = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
	else
		viewer_thumbnails_pane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

	child1 = gtk_paned_get_child1 (GTK_PANED (browser->priv->viewer_thumbnails_pane));
	child2 = gtk_paned_get_child2 (GTK_PANED (browser->priv->viewer_thumbnails_pane));

	g_object_ref (child1);
	gtk_widget_set_visible (child1, FALSE);
	gtk_widget_unrealize (child1);
	gtk_container_remove (GTK_CONTAINER (browser->priv->viewer_thumbnails_pane), child1);

	g_object_ref (child2);
	gtk_widget_set_visible (child2, FALSE);
	gtk_widget_unrealize (child2);
	gtk_container_remove (GTK_CONTAINER (browser->priv->viewer_thumbnails_pane), child2);

	if (viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL) {
		gtk_paned_pack1 (GTK_PANED (viewer_thumbnails_pane), browser->priv->viewer_sidebar_pane, TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (viewer_thumbnails_pane), browser->priv->thumbnail_list, FALSE, FALSE);
	}
	else {
		gtk_paned_pack1 (GTK_PANED (viewer_thumbnails_pane), browser->priv->thumbnail_list, FALSE, FALSE);
		gtk_paned_pack2 (GTK_PANED (viewer_thumbnails_pane), browser->priv->viewer_sidebar_pane, TRUE, FALSE);
	}

	gtk_widget_set_visible (child1, TRUE);
	gtk_widget_set_visible (child2, TRUE);

	g_object_notify (G_OBJECT (child1), "parent");
	g_object_unref (child1);

	g_object_notify (G_OBJECT (child2), "parent");
	g_object_unref (child2);

	gtk_widget_destroy (browser->priv->viewer_thumbnails_pane);
	browser->priv->viewer_thumbnails_pane = viewer_thumbnails_pane;

	if (viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		gth_file_list_set_mode (GTH_FILE_LIST (browser->priv->thumbnail_list), GTH_FILE_LIST_MODE_H_SIDEBAR);
	else
		gth_file_list_set_mode (GTH_FILE_LIST (browser->priv->thumbnail_list), GTH_FILE_LIST_MODE_V_SIDEBAR);

	gth_window_attach_content (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER, browser->priv->viewer_thumbnails_pane);

	if (gth_window_get_action_state (GTH_WINDOW (browser), "show-thumbnail-list"))
		gtk_widget_show (browser->priv->thumbnail_list);
	gtk_widget_show (browser->priv->viewer_sidebar_pane);
	gtk_widget_show (browser->priv->viewer_thumbnails_pane);
}


static void
_gth_browser_set_statusbar_visibility (GthBrowser *browser,
				       gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	gth_window_change_action_state (GTH_WINDOW (browser), "show-statusbar", visible);
	if (browser->priv->fullscreen)
		return;

	if (visible)
		gtk_widget_show (browser->priv->statusbar);
	else
		gtk_widget_hide (browser->priv->statusbar);
}


static void
pref_ui_statusbar_visible_changed (GSettings  *settings,
				   const char *key,
				   gpointer    user_data)
{
	GthBrowser *browser = user_data;
	_gth_browser_set_statusbar_visibility (browser, g_settings_get_boolean (settings, key));
}


static void
_gth_browser_set_sidebar_visibility  (GthBrowser *browser,
				      gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	gth_window_change_action_state (GTH_WINDOW (browser), "show-sidebar", visible);
	if (visible) {
		gtk_widget_show (browser->priv->browser_sidebar);
		gtk_paned_set_position (GTK_PANED (browser->priv->browser_left_container),
				        g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_BROWSER_SIDEBAR_WIDTH));
	}
	else
		gtk_widget_hide (browser->priv->browser_sidebar);
}


static void
pref_ui_sidebar_visible_changed (GSettings  *settings,
				 const char *key,
				 gpointer    user_data)
{
	GthBrowser *browser = user_data;
	_gth_browser_set_sidebar_visibility (browser, g_settings_get_boolean (settings, key));
}


static gboolean
_gth_browser_make_file_visible (GthBrowser  *browser,
				GthFileData *file_data)
{
	int        file_pos;
	GtkWidget *view;

	if (file_data == NULL)
		return FALSE;

	file_pos = gth_file_store_get_pos (GTH_FILE_STORE (gth_browser_get_file_store (browser)), file_data->file);
	if (file_pos < 0)
		return FALSE;

	/* the main file list */

	view = gth_browser_get_file_list_view (browser);
	g_signal_handlers_block_by_func (view, gth_file_view_selection_changed_cb, browser);
	gth_file_selection_unselect_all (GTH_FILE_SELECTION (view));
	gth_file_selection_select (GTH_FILE_SELECTION (view), file_pos);
	gth_file_view_set_cursor (GTH_FILE_VIEW (view), file_pos);
	g_signal_handlers_unblock_by_func (view, gth_file_view_selection_changed_cb, browser);

	/* the thumbnail list in viewer mode */

	view = gth_browser_get_thumbnail_list_view (browser);
	g_signal_handlers_block_by_func (view, gth_thumbnail_view_selection_changed_cb, browser);
	gth_file_selection_unselect_all (GTH_FILE_SELECTION (view));
	gth_file_selection_select (GTH_FILE_SELECTION (view), file_pos);
	gth_file_view_set_cursor (GTH_FILE_VIEW (view), file_pos);
	g_signal_handlers_unblock_by_func (view, gth_thumbnail_view_selection_changed_cb, browser);

	return TRUE;
}


static void
_gth_browser_set_thumbnail_list_visibility (GthBrowser *browser,
					    gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	gth_window_change_action_state (GTH_WINDOW (browser), "show-thumbnail-list", visible);
	if (visible) {
		gtk_widget_show (browser->priv->thumbnail_list);
		_gth_browser_make_file_visible (browser, browser->priv->current_file);
	}
	else
		gtk_widget_hide (browser->priv->thumbnail_list);
}


static void
pref_ui_thumbnail_list_visible_changed (GSettings  *settings,
					const char *key,
					gpointer    user_data)
{
	GthBrowser *browser = user_data;
	_gth_browser_set_thumbnail_list_visibility (browser, g_settings_get_boolean (settings, key));
}


static void
pref_show_hidden_files_changed (GSettings  *settings,
				const char *key,
				gpointer    user_data)
{
	GthBrowser *browser = user_data;
	gboolean    show_hidden_files;

	show_hidden_files = g_settings_get_boolean (settings, key);
	if (show_hidden_files == browser->priv->show_hidden_files)
		return;

	gth_window_change_action_state (GTH_WINDOW (browser), "show-hidden-files", show_hidden_files);
	browser->priv->show_hidden_files = show_hidden_files;
	gth_folder_tree_reset_loaded (GTH_FOLDER_TREE (browser->priv->folder_tree));
	gth_browser_reload (browser);
}


static void
pref_fast_file_type_changed (GSettings  *settings,
			     const char *key,
			     gpointer    user_data)
{
	GthBrowser *browser = user_data;

	browser->priv->fast_file_type = g_settings_get_boolean (settings, key);
	gth_browser_reload (browser);
}


static void
pref_thumbnail_size_changed (GSettings  *settings,
			     const char *key,
			     gpointer    user_data)
{
	GthBrowser *browser = user_data;

	gth_file_list_set_thumb_size (GTH_FILE_LIST (browser->priv->file_list), g_settings_get_int (settings, key));
	gth_file_list_set_thumb_size (GTH_FILE_LIST (browser->priv->thumbnail_list), g_settings_get_int (settings, key));
	gth_browser_reload (browser);
}


static void
pref_thumbnail_caption_changed (GSettings  *settings,
				const char *key,
				gpointer    user_data)
{
	GthBrowser *browser = user_data;
	char       *caption;

	caption = g_settings_get_string (settings, key);
	gth_file_list_set_caption (GTH_FILE_LIST (browser->priv->file_list), caption);

	if (_gth_browser_reload_required (browser))
		gth_browser_reload (browser);

	g_free (caption);
}


static void
pref_single_click_activation_changed (GSettings  *settings,
				      const char *key,
				      gpointer    user_data)
{
	GthBrowser *browser = user_data;
	gboolean    single_click = g_settings_get_boolean (settings, key);

	gth_file_view_set_activate_on_single_click (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list))), single_click);
}


static void
pref_open_files_in_fullscreen_changed (GSettings  *settings,
				       const char *key,
				       gpointer    user_data)
{
	GthBrowser *browser = user_data;
	browser->priv->view_files_in_fullscreen = g_settings_get_boolean (settings, key);
}


static void
pref_msg_save_modified_image_changed (GSettings  *settings,
				      const char *key,
				      gpointer    user_data)
{
	GthBrowser *browser = user_data;
	browser->priv->ask_to_save_modified_images = g_settings_get_boolean (settings, key);
}


static void
pref_scroll_action_changed (GSettings  *settings,
			    const char *key,
			    gpointer    user_data)
{
	GthBrowser *browser = user_data;
	browser->priv->scroll_action = g_settings_get_enum (settings, key);
}


static gboolean
_gth_browser_realize (GtkWidget *browser,
		      gpointer  *data)
{
	gth_hook_invoke ("gth-browser-realize", browser);

	return FALSE;
}


static gboolean
_gth_browser_unrealize (GtkWidget *browser,
			gpointer  *data)
{
	gth_hook_invoke ("gth-browser-unrealize", browser);

	return FALSE;
}


static void
_gth_browser_register_fixed_viewer_control (GthBrowser *browser,
					    GtkWidget  *widget)
{
	browser->priv->fixed_viewer_controls = g_list_prepend (browser->priv->fixed_viewer_controls, widget);
}


static void
browser_gesture_pressed_cb (GtkGestureMultiPress *gesture,
			    int                   n_press,
			    double                x,
			    double                y,
			    gpointer              user_data)
{
	GthBrowser *browser = user_data;
	guint       button;

	button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
	switch (button) {
	case 8: /* Back button */
		switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
		case GTH_BROWSER_PAGE_VIEWER:
			gth_window_activate_action (GTH_WINDOW (browser), "browser-mode", NULL);
			break;
		case GTH_BROWSER_PAGE_BROWSER:
			gth_browser_go_back (browser, 1);
			break;
		default:
			break;
		}
		break;

	case 9: /* Forward button */
		switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
		case GTH_BROWSER_PAGE_BROWSER:
			gth_browser_go_forward (browser, 1);
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}


#define FILTERS_GROUP "filters"


static void
_gth_browser_update_filter_list (GthBrowser *browser)
{
	GHashTable *accels;
	GList      *filters;
	GList      *scan;

	accels = gth_window_get_accels_for_group (GTH_WINDOW (browser), FILTERS_GROUP);
	gth_window_remove_shortcuts (GTH_WINDOW (browser), FILTERS_GROUP);

	filters = gth_main_get_all_filters ();
	for (scan = filters; scan; scan = scan->next) {
		GthTest     *test = scan->data;
		GthShortcut *shortcut;

		if (! gth_test_is_visible (test))
			continue;

		shortcut = gth_shortcut_new ("set-filter", g_variant_new_string (gth_test_get_id (test)));
		shortcut->description = g_strdup (gth_test_get_display_name (test));
		shortcut->context = GTH_SHORTCUT_CONTEXT_BROWSER;
		shortcut->category = GTH_SHORTCUT_CATEGORY_FILTERS;
		gth_shortcut_set_accelerator (shortcut, g_hash_table_lookup (accels, shortcut->detailed_action));
		shortcut->default_accelerator = g_strdup ("");

		gth_window_add_removable_shortcut (GTH_WINDOW (browser),
						   FILTERS_GROUP,
						   shortcut);

		gth_shortcut_free (shortcut);
	}
	gth_filterbar_set_filter_list (GTH_FILTERBAR (browser->priv->filterbar), filters);

	_g_object_list_unref (filters);
	g_hash_table_unref (accels);
}


static void
filters_changed_cb (GthMonitor *monitor,
		    GthBrowser *browser)
{
	_gth_browser_update_filter_list (browser);
}


static gboolean
browser_key_press_cb (GthBrowser  *browser,
		      GdkEventKey *event)
{
	GtkWidget *focus_widget;

	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_VIEWER:
		if (! _gth_browser_file_tool_is_active (browser))
			return gth_browser_viewer_key_press_cb (browser, event);
		break;

	case GTH_BROWSER_PAGE_BROWSER:
		focus_widget = gtk_window_get_focus (GTK_WINDOW (browser));
		if (! GTK_IS_ENTRY (focus_widget) && ! GTK_IS_TREE_VIEW (focus_widget))
			return gth_browser_file_list_key_press_cb (browser, event);
		break;

	default:
		break;
	}
	return FALSE;
}


static void
gth_browser_init (GthBrowser *browser)
{
	int             window_width;
	int             window_height;
	char          **sidebar_sections;
	GtkWidget      *vbox;
	GtkWidget      *scrolled_window;
	char           *sort_type;
	char           *caption;
	int             i;

	g_object_set (browser,
		      "n-pages", GTH_BROWSER_N_PAGES,
		      "use-header-bar", TRUE,
		      NULL);

	gtk_widget_add_events (GTK_WIDGET (browser), GDK_POINTER_MOTION_HINT_MASK);

	browser->priv = gth_browser_get_instance_private (browser);
	browser->priv->viewer_pages = NULL;
	browser->priv->viewer_page = NULL;
	browser->priv->image_preloader = gth_image_preloader_new ();
	browser->priv->progress_dialog = NULL;
	browser->priv->named_dialogs = g_hash_table_new (g_str_hash, g_str_equal);
	browser->priv->location = NULL;
	browser->priv->current_file = NULL;
	browser->priv->location_source = NULL;
	browser->priv->n_visibles = 0;
	browser->priv->current_file_position = -1;
	browser->priv->monitor_location = NULL;
	browser->priv->activity_ref = 0;
	browser->priv->menu_icon_cache = gth_icon_cache_new_for_widget (GTK_WIDGET (browser), GTK_ICON_SIZE_MENU);
	browser->priv->current_sort_type = NULL;
	browser->priv->current_sort_inverse = FALSE;
	browser->priv->default_sort_type = NULL;
	browser->priv->default_sort_inverse = FALSE;
	browser->priv->show_hidden_files = FALSE;
	browser->priv->fast_file_type = FALSE;
	browser->priv->closing = FALSE;
	browser->priv->task = NULL;
	browser->priv->task_completed = 0;
	browser->priv->task_progress = 0;
	browser->priv->next_task = NULL;
	browser->priv->background_tasks = NULL;
	browser->priv->viewer_tasks = NULL;
	browser->priv->close_with_task = FALSE;
	browser->priv->load_data_queue = NULL;
	browser->priv->last_folder_to_open = NULL;
	browser->priv->load_file_data_queue = NULL;
	browser->priv->load_file_timeout = 0;
	browser->priv->load_metadata_timeout = 0;
	browser->priv->list_attributes = NULL;
	browser->priv->constructed = FALSE;
	browser->priv->selection_changed_event = 0;
	browser->priv->folder_popup_file_data = NULL;
	browser->priv->properties_on_screen = FALSE;
	browser->priv->location_free_space = NULL;
	browser->priv->recalc_location_free_space = TRUE;
	browser->priv->fullscreen = FALSE;
	browser->priv->fullscreen_headerbar = NULL;
	browser->priv->was_fullscreen = FALSE;
	browser->priv->viewer_controls = NULL;
	browser->priv->fixed_viewer_controls = NULL;
	browser->priv->pointer_visible = TRUE;
	browser->priv->hide_mouse_timeout = 0;
	browser->priv->motion_signal = 0;
	browser->priv->last_mouse_x = 0.0;
	browser->priv->last_mouse_y = 0.0;
	browser->priv->history = NULL;
	browser->priv->history_current = NULL;
	browser->priv->browser_settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	browser->priv->messages_settings = g_settings_new (GTHUMB_MESSAGES_SCHEMA);
	browser->priv->desktop_interface_settings = g_settings_new (GNOME_DESKTOP_INTERFACE_SCHEMA);
	browser->priv->file_properties_on_the_right = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT);
	browser->priv->menu_managers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	browser->priv->screen_profile = NULL;
	browser->priv->folder_tree_last_dest_row = NULL;
	browser->priv->folder_tree_open_folder_id = 0;
	browser->priv->view_files_in_fullscreen = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_OPEN_FILES_IN_FULLSCREEN);;
	browser->priv->keep_mouse_visible = FALSE;
	browser->priv->fullscreen_state.sidebar = g_settings_get_enum (browser->priv->browser_settings, PREF_FULLSCREEN_SIDEBAR);
	browser->priv->fullscreen_state.thumbnail_list = g_settings_get_boolean (browser->priv->browser_settings, PREF_FULLSCREEN_THUMBNAILS_VISIBLE);

	browser_state_init (&browser->priv->state);

	/* find a suitable size for the window */

	window_width = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_WINDOW_WIDTH);
	window_height = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_WINDOW_HEIGHT);

	if ((window_width == 0) || (window_height == 0)) {
		int max_width;
		int max_height;
		int sidebar_width;
		int thumb_size;
		int thumb_spacing;
		int default_columns_of_thumbnails;
		int n_cols;

		gtk_widget_realize (GTK_WIDGET (browser));
		_gtk_widget_get_screen_size (GTK_WIDGET (browser), &max_width, &max_height);
		max_width = max_width * 5 / 6;
		max_height = max_height * 3 / 4;

		sidebar_width = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_BROWSER_SIDEBAR_WIDTH) + 10;
		thumb_size = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE);
		thumb_spacing = 40;
		default_columns_of_thumbnails = 6;

		for (n_cols = default_columns_of_thumbnails; n_cols >= 1; n_cols--) {
			window_width = sidebar_width + (thumb_spacing + 20) + (n_cols * (thumb_size + thumb_spacing));
			if (window_width < max_width)
				break;
		}
		if (n_cols == 0)
			window_width = max_width;
		window_height = max_height;
	}
	gtk_window_set_default_size (GTK_WINDOW (browser), window_width, window_height);
	if (g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_WINDOW_MAXIMIZED))
		gtk_window_maximize(GTK_WINDOW (browser));

	/* realize the widget before adding the ui to get the icons from the icon theme */

	g_signal_connect (browser, "realize", G_CALLBACK (_gth_browser_realize), NULL);
	g_signal_connect (browser, "unrealize", G_CALLBACK (_gth_browser_unrealize), NULL);
	gtk_widget_realize (GTK_WIDGET (browser));

	g_signal_connect (browser,
			  "key-press-event",
			  G_CALLBACK (browser_key_press_cb),
			  browser);

	browser->priv->gesture = gtk_gesture_multi_press_new (GTK_WIDGET (browser));
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (browser->priv->gesture), 0);
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (browser->priv->gesture), GTK_PHASE_CAPTURE);
	g_signal_connect (browser->priv->gesture,
			  "pressed",
			  G_CALLBACK (browser_gesture_pressed_cb),
			  browser);

	/* ui actions */

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 gth_browser_actions,
					 G_N_ELEMENTS (gth_browser_actions),
					 browser);
	gth_window_add_accelerators (GTH_WINDOW (browser),
				     gth_browser_accelerators,
				     G_N_ELEMENTS (gth_browser_accelerators));

	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  gth_browser_shortcuts,
				  G_N_ELEMENTS (gth_browser_shortcuts));

	/* -- image page -- */

	/* content */

	browser->priv->viewer_thumbnails_orientation = g_settings_get_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT);
	if (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		browser->priv->viewer_thumbnails_pane = gth_paned_new (GTK_ORIENTATION_VERTICAL);
	else
		browser->priv->viewer_thumbnails_pane = gth_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show (browser->priv->viewer_thumbnails_pane);
	gth_window_attach_content (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER, browser->priv->viewer_thumbnails_pane);

	browser->priv->viewer_sidebar_pane = gth_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->viewer_sidebar_pane), GTK_STYLE_CLASS_SIDEBAR);
	gtk_widget_set_size_request (browser->priv->viewer_sidebar_pane, -1, MIN_VIEWER_SIZE);
	gtk_widget_show (browser->priv->viewer_sidebar_pane);
	if (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_paned_pack1 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->viewer_sidebar_pane, TRUE, FALSE);
	else
		gtk_paned_pack2 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->viewer_sidebar_pane, TRUE, FALSE);

	browser->priv->viewer_container = gtk_overlay_new ();
	gtk_widget_set_size_request (browser->priv->viewer_container, MIN_VIEWER_SIZE, -1);
	gtk_widget_show (browser->priv->viewer_container);

	g_signal_connect (browser->priv->viewer_container,
			  "get-child-position",
			  G_CALLBACK (viewer_container_get_child_position_cb),
			  browser);

	gtk_paned_pack1 (GTK_PANED (browser->priv->viewer_sidebar_pane), browser->priv->viewer_container, TRUE, FALSE);
	browser->priv->viewer_sidebar_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_vexpand (browser->priv->viewer_sidebar_container, TRUE);
	gtk_paned_pack2 (GTK_PANED (browser->priv->viewer_sidebar_pane), browser->priv->viewer_sidebar_container, FALSE, FALSE);

	browser->priv->thumbnail_list = gth_file_list_new (gth_grid_view_new (), (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL) ? GTH_FILE_LIST_MODE_H_SIDEBAR : GTH_FILE_LIST_MODE_V_SIDEBAR, TRUE);
	gth_file_list_set_caption (GTH_FILE_LIST (browser->priv->thumbnail_list), "none");
	gth_grid_view_set_cell_spacing (GTH_GRID_VIEW (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->thumbnail_list))), 0);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (browser->priv->thumbnail_list), g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE));

	if (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_paned_pack2 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->thumbnail_list, FALSE, FALSE);
	else
		gtk_paned_pack1 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->thumbnail_list, FALSE, FALSE);
	_gth_browser_set_thumbnail_list_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE));

	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->thumbnail_list)),
			  "file-selection-changed",
			  G_CALLBACK (gth_thumbnail_view_selection_changed_cb),
			  browser);

	/* -- browser page -- */

	/* dynamic header sections */

	for (i = 0; i < GTH_BROWSER_N_HEADER_SECTIONS; i++) {
		gboolean separated_buttons;

		separated_buttons = (/*(i == GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS)*/
				     /*|| (i == GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS)*/
				     /*|| (i == GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR)*/
				     (i == GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW)
				     /*|| (i == GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT)*/
				     || (i == GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS));

		browser->priv->header_sections[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, separated_buttons ? 6 : 0);
		gtk_widget_set_valign (browser->priv->header_sections[i], GTK_ALIGN_CENTER);
		if (! separated_buttons)
			gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->header_sections[i]), GTK_STYLE_CLASS_LINKED);
	}

	/* window header bar */

	{
		GtkWidget  *header_bar;
		GtkBuilder *builder;
		GMenuModel *menu;
		GtkWidget  *button;

		header_bar = gth_window_get_header_bar (GTH_WINDOW (browser));

		gtk_widget_set_margin_start (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_COMMANDS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_start (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_COMMANDS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_VIEW], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_start (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_start (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_start (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_start (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_VIEW], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_VIEW], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);
		gtk_widget_set_margin_end (browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS], GTH_BROWSER_HEADER_BAR_BIG_MARGIN);

		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_NAVIGATION]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_COMMANDS]);

		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_NAVIGATION]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW]);

		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_NAVIGATION]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_VIEW]);

		/* GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM is shared by the viewer and the editor */
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_VIEW]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS]);
		gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS]);

		/* gears menu button */

		builder = _gtk_builder_new_from_resource ("gears-menu.ui");
		menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
		browser->priv->menu_button = _gtk_menu_button_new_for_header_bar ("open-menu-symbolic");
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (browser->priv->menu_button), menu);
		gtk_widget_set_halign (GTK_WIDGET (gtk_menu_button_get_popup (GTK_MENU_BUTTON (browser->priv->menu_button))), GTK_ALIGN_END);
		gtk_widget_show_all (browser->priv->menu_button);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->menu_button);

		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_GEARS, G_MENU (menu));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_GEARS_FOLDER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "folder-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_GEARS_OTHER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "other-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_GEARS_APP_ACTIONS, G_MENU (gtk_builder_get_object (builder, "app-actions")));
		_gtk_window_add_accelerators_from_menu ((GTK_WINDOW (browser)), menu);
		g_object_unref (builder);

		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS]);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_METADATA_TOOLS]);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_BROWSER_VIEW]);

		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT]);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT_METADATA]);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR]);

		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_APPLY]);
		gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), browser->priv->header_sections[GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS]);

		/* browser navigation */

		gth_browser_add_header_bar_button (browser,
						   GTH_BROWSER_HEADER_SECTION_BROWSER_NAVIGATION,
						   "go-previous-symbolic",
						   _("Go to the previous visited location"),
						   "win.go-back",
						   NULL);
		gth_browser_add_header_bar_button (browser,
						   GTH_BROWSER_HEADER_SECTION_BROWSER_NAVIGATION,
						   "go-next-symbolic",
						   _("Go to the next visited location"),
						   "win.go-forward",
						   NULL);

		/* history menu button */

		builder = _gtk_builder_new_from_resource ("history-menu.ui");
		button = _gtk_menu_button_new_for_header_bar ("document-open-recent-symbolic");
		gtk_widget_set_tooltip_text (button, _("History"));

		browser->priv->history_menu = g_object_ref (G_MENU (gtk_builder_get_object (builder, "visited-locations")));
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), G_MENU_MODEL (gtk_builder_get_object (builder, "menu")));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_NAVIGATION)), button, FALSE, FALSE, 0);

		g_object_unref (builder);

		/* viewer navigation */

		gth_browser_add_header_bar_button (browser,
						   GTH_BROWSER_HEADER_SECTION_VIEWER_NAVIGATION,
						   "go-previous-symbolic",
						   _("View the folders"),
						   "win.browser-mode",
						   NULL);

		/* viewer edit */

		gth_browser_add_header_bar_toggle_button (browser,
							  GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR,
							  "dialog-information-symbolic",
							  _("Properties"),
							  "win.viewer-properties",
							  NULL);
		gth_browser_add_header_bar_toggle_button (browser,
							  GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR,
							  "palette-symbolic",
							  _("Edit file"),
							  "win.viewer-edit-file",
							  NULL);

		/* viewer view */

		gth_browser_add_header_bar_button (browser,
						   GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW,
						   "view-fullscreen-symbolic",
						   _("Fullscreen"),
						   "win.fullscreen",
						   NULL);

		/* editor navigation */

		gth_browser_add_header_bar_button (browser,
						   GTH_BROWSER_HEADER_SECTION_EDITOR_NAVIGATION,
						   "go-previous-symbolic",
						   NULL,
						   "win.browser-mode",
						   NULL);

		/* editor view */

		gth_browser_add_header_bar_button (browser,
						   GTH_BROWSER_HEADER_SECTION_EDITOR_VIEW,
						   "view-fullscreen-symbolic",
						   _("Fullscreen"),
						   "win.fullscreen",
						   NULL);

		/* editor commands */

		button = gth_browser_add_header_bar_label_button (browser,
								  GTH_BROWSER_HEADER_SECTION_EDITOR_APPLY,
								  _("Accept"),
								  NULL,
								  "win.apply-editor-changes",
								  NULL);
#ifdef GTK_STYLE_CLASS_SUGGESTED_ACTION
		gtk_style_context_add_class (gtk_widget_get_style_context (button), GTK_STYLE_CLASS_SUGGESTED_ACTION);
#endif
		browser->priv->apply_editor_changes_button = button;
	}

	/* fullscreen toolbar */

	browser->priv->fullscreen_toolbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->fullscreen_toolbar), GTK_STYLE_CLASS_BACKGROUND);
	gth_window_add_overlay (GTH_WINDOW (browser), browser->priv->fullscreen_toolbar);

	/* overlay commands */

	{
		/* show next image */

		browser->priv->next_image_button = gtk_button_new_from_icon_name ("go-next-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (browser->priv->next_image_button), "win.show-next-image");
		gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->next_image_button), GTK_STYLE_CLASS_OSD);
		gtk_widget_set_can_focus (browser->priv->next_image_button, FALSE);
		gtk_overlay_add_overlay (GTK_OVERLAY (browser->priv->viewer_container), browser->priv->next_image_button);
		_gth_browser_register_fixed_viewer_control (browser, browser->priv->next_image_button);

		/* show previous image */

		browser->priv->previous_image_button = gtk_button_new_from_icon_name ("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (browser->priv->previous_image_button), "win.show-previous-image");
		gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->previous_image_button), GTK_STYLE_CLASS_OSD);
		gtk_widget_set_can_focus (browser->priv->previous_image_button, FALSE);
		gtk_overlay_add_overlay (GTK_OVERLAY (browser->priv->viewer_container), browser->priv->previous_image_button);
		_gth_browser_register_fixed_viewer_control (browser, browser->priv->previous_image_button);
	}

	/* infobar */

	browser->priv->infobar = gth_info_bar_new ();
	gth_window_attach (GTH_WINDOW (browser), browser->priv->infobar, GTH_WINDOW_INFOBAR);

	/* statusbar */

	browser->priv->statusbar = gth_statusbar_new ();
	_gth_browser_set_statusbar_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_STATUSBAR_VISIBLE));
	gth_window_attach (GTH_WINDOW (browser), browser->priv->statusbar, GTH_WINDOW_STATUSBAR);

	{
		GtkWidget *button;

		/* statusbar commands available in all modes */

		browser->priv->show_progress_dialog_button = button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("system-run-symbolic", GTK_ICON_SIZE_MENU));
		gtk_widget_set_tooltip_text (button, _("Operations"));
		gtk_widget_show_all (button);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.show-progress-dialog");
		gtk_box_pack_start (GTK_BOX (gth_statubar_get_action_area (GTH_STATUSBAR (browser->priv->statusbar))), button, FALSE, FALSE, 0);
		gtk_widget_hide (button);

		/* statusbar commands in browser mode */

		browser->priv->browser_status_commands = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (browser->priv->browser_status_commands);
		gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->browser_status_commands), GTK_STYLE_CLASS_LINKED);
		gtk_box_pack_start (GTK_BOX (gth_statubar_get_action_area (GTH_STATUSBAR (browser->priv->statusbar))), browser->priv->browser_status_commands, FALSE, FALSE, 0);

		button = gtk_toggle_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("dialog-information-symbolic", GTK_ICON_SIZE_MENU));
		gtk_widget_set_tooltip_text (button, _("Properties"));
		gtk_widget_show_all (button);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.browser-properties");
		gtk_box_pack_start (GTK_BOX (browser->priv->browser_status_commands), button, FALSE, FALSE, 0);

		/* statusbar commands in viewer mode */

		browser->priv->viewer_status_commands = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
		gtk_widget_show (browser->priv->viewer_status_commands);
		/*gtk_style_context_add_class (gtk_widget_get_style_context (browser->priv->viewer_status_commands), GTK_STYLE_CLASS_LINKED);*/
		gtk_box_pack_start (GTK_BOX (gth_statubar_get_action_area (GTH_STATUSBAR (browser->priv->statusbar))), browser->priv->viewer_status_commands, FALSE, FALSE, 0);

		button = gtk_toggle_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("view-grid-symbolic", GTK_ICON_SIZE_MENU));
		gtk_widget_show_all (button);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.show-thumbnail-list");
		gtk_box_pack_end (GTK_BOX (browser->priv->viewer_status_commands), button, FALSE, FALSE, 0);
	}

	/* main content */

	browser->priv->browser_right_container = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show (browser->priv->browser_right_container);
	gth_window_attach_content (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER, browser->priv->browser_right_container);

	browser->priv->browser_left_container = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_position (GTK_PANED (browser->priv->browser_left_container), g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_BROWSER_SIDEBAR_WIDTH));
	gtk_widget_show (browser->priv->browser_left_container);
	gtk_paned_pack1 (GTK_PANED (browser->priv->browser_right_container), browser->priv->browser_left_container, TRUE, TRUE);

	/* the browser sidebar */

	browser->priv->browser_sidebar = gth_auto_paned_new (GTK_ORIENTATION_VERTICAL);
	gtk_paned_pack1 (GTK_PANED (browser->priv->browser_left_container), browser->priv->browser_sidebar, FALSE, FALSE);

	/* the box that contains the folder list.  */

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (vbox), GTK_STYLE_CLASS_SIDEBAR);
	gtk_widget_set_size_request (vbox, -1, FILE_PROPERTIES_MINIMUM_HEIGHT);
	gtk_widget_show (vbox);
	gtk_paned_pack1 (GTK_PANED (browser->priv->browser_sidebar), vbox, TRUE, FALSE);

	/* the folder list */

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_NONE);
	gtk_widget_show (scrolled_window);

	browser->priv->folder_tree = gth_folder_tree_new ("gthumb-vfs:///");
	gtk_widget_show (browser->priv->folder_tree);

	gtk_container_add (GTK_CONTAINER (scrolled_window), browser->priv->folder_tree);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

	{
		GtkTargetList  *target_list;
		GtkTargetEntry *targets;
		int             n_targets;

		target_list = gtk_target_list_new (NULL, 0);
		gtk_target_list_add_uri_targets (target_list, 0);
		gtk_target_list_add_text_targets (target_list, 0);
		targets = gtk_target_table_new_from_list (target_list, &n_targets);

		gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (browser->priv->folder_tree),
						      targets,
						      n_targets,
						      GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_ASK);
		gth_folder_tree_enable_drag_source (GTH_FOLDER_TREE (browser->priv->folder_tree),
						    GDK_BUTTON1_MASK,
						    targets,
						    n_targets,
						    GDK_ACTION_MOVE | GDK_ACTION_COPY);
		g_signal_connect (browser->priv->folder_tree,
				  "drag_motion",
				  G_CALLBACK (folder_tree_drag_motion_cb),
				  browser);
		g_signal_connect (browser->priv->folder_tree,
	                          "drag-data-received",
	                          G_CALLBACK (folder_tree_drag_data_received),
	                          browser);
		g_signal_connect (browser->priv->folder_tree,
				  "drag-data-get",
				  G_CALLBACK (folder_tree_drag_data_get_cb),
				  browser);

		gtk_target_list_unref (target_list);
		gtk_target_table_free (targets, n_targets);
	}

	g_signal_connect (browser->priv->folder_tree,
			  "open",
			  G_CALLBACK (folder_tree_open_cb),
			  browser);
	g_signal_connect (browser->priv->folder_tree,
			  "open_parent",
			  G_CALLBACK (folder_tree_open_parent_cb),
			  browser);
	g_signal_connect (browser->priv->folder_tree,
			  "list_children",
			  G_CALLBACK (folder_tree_list_children_cb),
			  browser);
	g_signal_connect (browser->priv->folder_tree,
			  "folder_popup",
			  G_CALLBACK (folder_tree_folder_popup_cb),
			  browser);
	g_signal_connect (browser->priv->folder_tree,
			  "rename",
			  G_CALLBACK (folder_tree_rename_cb),
			  browser);

	/* the file property box */

	sidebar_sections = g_settings_get_strv (browser->priv->browser_settings, PREF_BROWSER_SIDEBAR_SECTIONS);
	browser->priv->file_properties = gth_sidebar_new (sidebar_sections);
	gtk_widget_set_size_request (browser->priv->file_properties, -1, FILE_PROPERTIES_MINIMUM_HEIGHT);
	gtk_widget_hide (browser->priv->file_properties);
	gtk_paned_pack2 (GTK_PANED (_gth_browser_get_browser_file_properties_container (browser)),
			 browser->priv->file_properties,
			 ! browser->priv->file_properties_on_the_right,
			 FALSE);
	g_strfreev (sidebar_sections);

	g_signal_connect (gth_sidebar_get_toolbox (GTH_SIDEBAR (browser->priv->file_properties)),
			  "options-visibility",
			  G_CALLBACK (toolbox_options_visibility_cb),
			  browser);

	/* the box that contains the file list and the filter bar.  */

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	gtk_paned_pack2 (GTK_PANED (browser->priv->browser_left_container), vbox, TRUE, TRUE);

	/* the list extra widget container */

	browser->priv->list_extra_widget_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (browser->priv->list_extra_widget_container);
	gtk_box_pack_start (GTK_BOX (vbox), browser->priv->list_extra_widget_container, FALSE, FALSE, 0);

	/* location bar */

	browser->priv->location_bar = gth_location_bar_new ();
	gtk_widget_show (browser->priv->location_bar);
	gtk_box_pack_start (GTK_BOX (browser->priv->list_extra_widget_container), browser->priv->location_bar, TRUE, TRUE, 0);
	g_signal_connect (gth_location_bar_get_chooser (GTH_LOCATION_BAR (browser->priv->location_bar)),
			  "changed",
			  G_CALLBACK (location_chooser_changed_cb),
			  browser);

	/* list info bar */

	browser->priv->list_info_bar = gth_info_bar_new ();
	gtk_box_pack_start (GTK_BOX (browser->priv->list_extra_widget_container), browser->priv->list_info_bar, TRUE, TRUE, 0);

	/* the file list */

	browser->priv->file_list = gth_file_list_new (gth_grid_view_new (), GTH_FILE_LIST_MODE_NORMAL, TRUE);
	sort_type = g_settings_get_string (browser->priv->browser_settings, PREF_BROWSER_SORT_TYPE);
	gth_browser_set_sort_order (browser,
				    gth_main_get_sort_type (sort_type),
				    g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SORT_INVERSE));
	gth_file_list_set_thumb_size (GTH_FILE_LIST (browser->priv->file_list),
				      g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE));
	caption = g_settings_get_string (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION);
	gth_file_list_set_caption (GTH_FILE_LIST (browser->priv->file_list), caption);
	gth_file_view_set_activate_on_single_click (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list))), g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SINGLE_CLICK_ACTIVATION));

	g_free (caption);
	g_free (sort_type);

	gth_file_list_set_model (GTH_FILE_LIST (browser->priv->thumbnail_list),
				 gth_file_list_get_model (GTH_FILE_LIST (browser->priv->file_list)));

	gtk_widget_show (browser->priv->file_list);
	gtk_box_pack_start (GTK_BOX (vbox), browser->priv->file_list, TRUE, TRUE, 0);

	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list)),
			  "button-press-event",
			  G_CALLBACK (gth_file_list_button_press_cb),
			  browser);
	g_signal_connect (gth_file_list_get_empty_view (GTH_FILE_LIST (browser->priv->file_list)),
			  "button-press-event",
			  G_CALLBACK (gth_file_list_button_press_cb),
			  browser);
	g_signal_connect (browser->priv->file_list,
			  "popup-menu",
			  G_CALLBACK (gth_file_list_popup_menu_cb),
			  browser);
	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list)),
			  "file-selection-changed",
			  G_CALLBACK (gth_file_view_selection_changed_cb),
			  browser);
	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list)),
			  "file-activated",
			  G_CALLBACK (gth_file_view_file_activated_cb),
			  browser);

	/* the filter bar */

	browser->priv->filterbar = gth_filterbar_new ();
	gtk_widget_show (browser->priv->filterbar);
	_gth_browser_update_filter_list (browser);
	gth_filterbar_load_filter (GTH_FILTERBAR (browser->priv->filterbar), "active_filter.xml");
	gtk_box_pack_end (GTK_BOX (vbox), browser->priv->filterbar, FALSE, FALSE, 0);

	g_signal_connect (browser->priv->filterbar,
			  "changed",
			  G_CALLBACK (filterbar_changed_cb),
			  browser);
	g_signal_connect (browser->priv->filterbar,
			  "personalize",
			  G_CALLBACK (filterbar_personalize_cb),
			  browser);
	browser->priv->filters_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "filters-changed",
				  G_CALLBACK (filters_changed_cb),
				  browser);

	/* monitor signals */

	browser->priv->folder_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "folder-changed",
				  G_CALLBACK (folder_changed_cb),
				  browser);
	browser->priv->file_renamed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "file-renamed",
				  G_CALLBACK (file_renamed_cb),
				  browser);
	browser->priv->metadata_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "metadata-changed",
				  G_CALLBACK (metadata_changed_cb),
				  browser);
	browser->priv->emblems_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "emblems-changed",
				  G_CALLBACK (emblems_changed_cb),
				  browser);
	browser->priv->entry_points_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "entry-points-changed",
				  G_CALLBACK (entry_points_changed_cb),
				  browser);
	browser->priv->order_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "order-changed",
				  G_CALLBACK (order_changed_cb),
				  browser);
	browser->priv->shortcuts_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "shortcuts-changed",
				  G_CALLBACK (shortcuts_changed_cb),
				  browser);

	/* init browser data */

	/* file popup menu */
	{
		GtkBuilder *builder;
		GMenuModel *menu;

		builder = _gtk_builder_new_from_resource ("file-menu.ui");
		menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
		browser->priv->file_popup = gtk_menu_new_from_model (menu);
		gtk_menu_attach_to_widget (GTK_MENU (browser->priv->file_popup), GTK_WIDGET (browser), NULL);

		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE, G_MENU (menu));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_SCREEN_ACTIONS, G_MENU (gtk_builder_get_object (builder, "screen-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_OPEN_ACTIONS, G_MENU (gtk_builder_get_object (builder, "open-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_EDIT_ACTIONS, G_MENU (gtk_builder_get_object (builder, "edit-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_FILE_ACTIONS, G_MENU (gtk_builder_get_object (builder, "file-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_DELETE_ACTIONS, G_MENU (gtk_builder_get_object (builder, "delete-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_FOLDER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "folder-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_OTHER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "other-actions")));

		g_object_unref (builder);
	}

	/* file list popup menu */
	{
		GtkBuilder *builder;
		GMenuModel *menu;

		builder = _gtk_builder_new_from_resource ("file-list-menu.ui");
		menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
		browser->priv->file_list_popup = gtk_menu_new_from_model (menu);
		gtk_menu_attach_to_widget (GTK_MENU (browser->priv->file_list_popup), GTK_WIDGET (browser), NULL);

		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST, G_MENU (menu));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_SCREEN_ACTIONS, G_MENU (gtk_builder_get_object (builder, "screen-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS, G_MENU (gtk_builder_get_object (builder, "open-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_EDIT_ACTIONS, G_MENU (gtk_builder_get_object (builder, "edit-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_FILE_ACTIONS, G_MENU (gtk_builder_get_object (builder, "file-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_DELETE_ACTIONS, G_MENU (gtk_builder_get_object (builder, "delete-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_FOLDER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "folder-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OTHER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "other-actions")));

		g_object_unref (builder);
	}

	/* folder popup menu */
	{
		GtkBuilder *builder;
		GMenuModel *menu;

		builder = _gtk_builder_new_from_resource ("folder-menu.ui");
		menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
		browser->priv->folder_popup = gtk_menu_new_from_model (menu);
		gtk_menu_attach_to_widget (GTK_MENU (browser->priv->folder_popup), GTK_WIDGET (browser), NULL);
		g_signal_connect (browser->priv->folder_popup,
				  "hide",
				  G_CALLBACK (folder_popup_hide_cb),
				  browser);

		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FOLDER, G_MENU (menu));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OPEN_ACTIONS, G_MENU (gtk_builder_get_object (builder, "open-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_CREATE_ACTIONS, G_MENU (gtk_builder_get_object (builder, "create-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_EDIT_ACTIONS, G_MENU (gtk_builder_get_object (builder, "edit-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_FOLDER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "folder-actions")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OTHER_ACTIONS, G_MENU (gtk_builder_get_object (builder, "other-actions")));

		g_object_unref (builder);
	}

	_gth_browser_set_sidebar_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SIDEBAR_VISIBLE));

	browser->priv->show_hidden_files = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SHOW_HIDDEN_FILES);
	gth_window_change_action_state (GTH_WINDOW (browser), "show-hidden-files", browser->priv->show_hidden_files);

	if (g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_VISIBLE))
		gth_browser_show_file_properties (browser);
	else
		gth_browser_hide_sidebar (browser);

	browser->priv->viewer_sidebar = g_settings_get_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_SIDEBAR);
	browser->priv->fast_file_type = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_FAST_FILE_TYPE);
	browser->priv->ask_to_save_modified_images = g_settings_get_boolean (browser->priv->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE);
	browser->priv->scroll_action = g_settings_get_enum (browser->priv->browser_settings, PREF_VIEWER_SCROLL_ACTION);

	/* load the history only for the first window */
	{
		GList * windows = gtk_application_get_windows (Main_Application);
		if ((windows == NULL) || (windows->next == NULL))
			_gth_browser_history_load (browser);
	}

	gtk_widget_realize (browser->priv->file_list);
	gth_hook_invoke ("gth-browser-construct", browser);
	gth_window_load_shortcuts (GTH_WINDOW (browser));

	performance (DEBUG_INFO, "window initialized");

	/* settings notifications */

	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_GENERAL_FILTER,
			  G_CALLBACK (pref_general_filter_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT,
			  G_CALLBACK (pref_ui_viewer_thumbnails_orient_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_PROPERTIES_ON_THE_RIGHT,
			  G_CALLBACK (pref_browser_properties_on_the_right_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_STATUSBAR_VISIBLE,
			  G_CALLBACK (pref_ui_statusbar_visible_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_SIDEBAR_VISIBLE,
			  G_CALLBACK (pref_ui_sidebar_visible_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_THUMBNAIL_LIST_VISIBLE,
			  G_CALLBACK (pref_ui_thumbnail_list_visible_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_SHOW_HIDDEN_FILES,
			  G_CALLBACK (pref_show_hidden_files_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_FAST_FILE_TYPE,
			  G_CALLBACK (pref_fast_file_type_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_THUMBNAIL_SIZE,
			  G_CALLBACK (pref_thumbnail_size_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_THUMBNAIL_CAPTION,
			  G_CALLBACK (pref_thumbnail_caption_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_SINGLE_CLICK_ACTIVATION,
			  G_CALLBACK (pref_single_click_activation_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_OPEN_FILES_IN_FULLSCREEN,
			  G_CALLBACK (pref_open_files_in_fullscreen_changed),
			  browser);
	g_signal_connect (browser->priv->messages_settings,
			  "changed::" PREF_MSG_SAVE_MODIFIED_IMAGE,
			  G_CALLBACK (pref_msg_save_modified_image_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_VIEWER_SCROLL_ACTION,
			  G_CALLBACK (pref_scroll_action_changed),
			  browser);

	browser->priv->constructed = TRUE;
}


GtkWidget *
gth_browser_new (GFile *location,
		 GFile *file_to_select)
{
	GthBrowser    *browser;
	NewWindowData *data;

	browser = (GthBrowser*) g_object_new (GTH_TYPE_BROWSER, NULL);

	data = g_new0 (NewWindowData, 1);
	data->browser = browser;
	data->location = _g_object_ref (location);
	if (data->location == NULL)
		data->location = g_file_new_for_uri (gth_pref_get_startup_location ());
	data->file_to_select = _g_object_ref (file_to_select);

	browser->priv->construct_step2_event = call_when_idle (_gth_browser_construct_step2, data);

	return (GtkWidget*) browser;
}


GFile *
gth_browser_get_location (GthBrowser *browser)
{
	if (browser->priv->location != NULL)
		return browser->priv->location->file;
	else
		return NULL;
}


GthFileData *
gth_browser_get_location_data (GthBrowser *browser)
{
	return browser->priv->location;
}


GthFileData *
gth_browser_get_current_file (GthBrowser *browser)
{
	return browser->priv->current_file;
}


gboolean
gth_browser_get_file_modified (GthBrowser *browser)
{
	if (browser->priv->current_file != NULL)
		return g_file_info_get_attribute_boolean (browser->priv->current_file->info, "gth::file::is-modified");
	else
		return FALSE;
}


void
gth_browser_go_to (GthBrowser *browser,
		   GFile      *location,
		   GFile      *file_to_select)
{
	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
	_gth_browser_load (browser, location, file_to_select, NULL, 0, GTH_ACTION_GO_TO, FALSE);
}


static void
gth_browser_go_to_with_state (GthBrowser  *browser,
			      GFile       *location,
			      GList       *selected,
			      double       vscroll)
{
	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
	_gth_browser_load (browser, location, NULL, selected, vscroll, GTH_ACTION_GO_TO, FALSE);
}


void
gth_browser_go_back (GthBrowser *browser,
		     int         steps)
{
	GList *new_current;

	new_current = browser->priv->history_current;
	while ((new_current != NULL) && (steps-- > 0))
		new_current = new_current->next;

	if (new_current == NULL)
		return;

	browser->priv->history_current = new_current;
	_gth_browser_load (browser, (GFile*) browser->priv->history_current->data, NULL, NULL, 0, GTH_ACTION_GO_BACK, FALSE);
}


void
gth_browser_go_forward (GthBrowser *browser,
			int         steps)
{
	GList *new_current;

	new_current = browser->priv->history_current;
	while ((new_current != NULL) && (steps-- > 0))
		new_current = new_current->prev;

	if (new_current == NULL)
		return;

	browser->priv->history_current = new_current;
	_gth_browser_load (browser, (GFile *) browser->priv->history_current->data, NULL, NULL, 0, GTH_ACTION_GO_FORWARD, FALSE);
}


void
gth_browser_go_to_history_pos (GthBrowser *browser,
			       int         pos)
{
	GList *new_current;

	new_current = g_list_nth (browser->priv->history, pos);
	if (new_current == NULL)
		return;

	if (new_current == browser->priv->history_current)
		return;

	browser->priv->history_current = new_current;
	_gth_browser_load (browser, (GFile*) browser->priv->history_current->data, NULL, NULL, 0, GTH_ACTION_GO_BACK, FALSE);
}


void
gth_browser_go_up (GthBrowser *browser,
		   int         steps)
{
	GFile *parent;

	if (browser->priv->location == NULL)
		return;

	parent = g_object_ref (browser->priv->location->file);
	while ((steps-- > 0) && (parent != NULL)) {
		GFile *parent_parent;

		parent_parent = g_file_get_parent (parent);
		g_object_unref (parent);
		parent = parent_parent;
	}

	if (parent != NULL) {
		gth_browser_go_to (browser, parent, NULL);
		g_object_unref (parent);
	}
}


void
gth_browser_go_home (GthBrowser *browser)
{
	GFile *location;

	if (g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION))
		location = g_file_new_for_uri (gth_pref_get_startup_location ());
	else
		location = g_file_new_for_uri (_g_uri_get_home ());

	gth_browser_go_to (browser, location, NULL);

	g_object_unref (location);
}


void
gth_browser_clear_history (GthBrowser *browser)
{
	_g_object_list_unref (browser->priv->history);
	browser->priv->history = NULL;
	browser->priv->history_current = NULL;

	if (browser->priv->location != NULL)
		_gth_browser_history_add (browser, browser->priv->location->file);
	_gth_browser_history_menu (browser);
}


void
gth_browser_set_dialog (GthBrowser *browser,
			const char *dialog_name,
			GtkWidget  *dialog)
{
	if (dialog != NULL)
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	g_hash_table_insert (browser->priv->named_dialogs, (gpointer) dialog_name, dialog);
}


GtkWidget *
gth_browser_get_dialog (GthBrowser *browser,
			const char *dialog_name)
{
	return g_hash_table_lookup (browser->priv->named_dialogs, dialog_name);
}


GthIconCache *
gth_browser_get_menu_icon_cache (GthBrowser *browser)
{
	return browser->priv->menu_icon_cache;
}


GtkWidget *
gth_browser_get_infobar (GthBrowser *browser)
{
	return browser->priv->infobar;
}


GtkWidget *
gth_browser_get_statusbar (GthBrowser *browser)
{
	return browser->priv->statusbar;
}


GtkWidget *
gth_browser_get_filterbar (GthBrowser *browser)
{
	return browser->priv->filterbar;
}


GtkWidget *
gth_browser_get_headerbar_section (GthBrowser			*browser,
				   GthBrowserHeaderSection	 section)
{
	return browser->priv->header_sections[section];
}


static void
_gth_browser_setup_header_bar_button (GthBrowser			*browser,
				      GthBrowserHeaderSection		 section,
				      const char			*tooltip,
				      const char 			*action_name,
				      const char			*accelerator,
				      GtkWidget 			*button)
{
	if (action_name != NULL)
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), action_name);
	if (tooltip != NULL)
		gtk_widget_set_tooltip_text (button, tooltip);
	if ((action_name != NULL) && (accelerator != NULL))
		_gtk_window_add_accelerator_for_action (GTK_WINDOW (browser),
							gth_window_get_accel_group (GTH_WINDOW (browser)),
							action_name,
							accelerator,
							NULL);
	gtk_box_pack_start (GTK_BOX (gth_browser_get_headerbar_section (browser, section)), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}


GtkWidget *
gth_browser_add_header_bar_button (GthBrowser			*browser,
				   GthBrowserHeaderSection	 section,
				   const char			*icon_name,
				   const char			*tooltip,
				   const char 			*action_name,
				   const char			*accelerator)
{
	GtkWidget *button;

	g_return_val_if_fail (icon_name != NULL, NULL);

	button = _gtk_image_button_new_for_header_bar (icon_name);
	_gth_browser_setup_header_bar_button (browser, section, tooltip, action_name, accelerator, button);

	return button;
}


GtkWidget *
gth_browser_add_header_bar_toggle_button (GthBrowser			*browser,
					  GthBrowserHeaderSection	 section,
					  const char			*icon_name,
					  const char			*tooltip,
					  const char 			*action_name,
					  const char			*accelerator)
{
	GtkWidget *button;

	g_return_val_if_fail (icon_name != NULL, NULL);

	button = _gtk_toggle_image_button_new_for_header_bar (icon_name);
	_gth_browser_setup_header_bar_button (browser, section, tooltip, action_name, accelerator, button);

	return button;
}


GtkWidget *
gth_browser_add_header_bar_label_button (GthBrowser			*browser,
					 GthBrowserHeaderSection	 section,
					 const char			*label,
					 const char			*tooltip,
					 const char 			*action_name,
					 const char			*accelerator)
{
	GtkWidget *button;

	g_return_val_if_fail (label != NULL, NULL);

	button = gtk_button_new_with_label (label);
	gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
	_gth_browser_setup_header_bar_button (browser, section, tooltip, action_name, accelerator, button);

	return button;
}


GtkWidget *
gth_browser_add_header_bar_menu_button (GthBrowser		*browser,
					GthBrowserHeaderSection	 section,
					const char              *icon_name,
					const char              *tooltip,
					GtkWidget               *popover)
{
	GtkWidget *button;

	g_return_val_if_fail (icon_name != NULL, NULL);

	button = _gtk_menu_button_new_for_header_bar (icon_name);
	gtk_menu_button_set_popover (GTK_MENU_BUTTON (button), popover);
	_gth_browser_setup_header_bar_button (browser, section, tooltip, NULL, NULL, button);

	return button;
}


void
gth_browser_add_menu_manager_for_menu (GthBrowser *browser,
				       const char *menu_id,
				       GMenu	  *menu)
{
	g_hash_table_insert (browser->priv->menu_managers, g_strdup (menu_id), gth_menu_manager_new (menu));
}


GthMenuManager *
gth_browser_get_menu_manager (GthBrowser *browser,
			      const char *menu_id)
{
	return g_hash_table_lookup (browser->priv->menu_managers, menu_id);
}


GtkWidget *
gth_browser_get_file_list (GthBrowser *browser)
{
	return browser->priv->file_list;
}


GtkWidget *
gth_browser_get_file_list_view (GthBrowser *browser)
{
	return gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list));
}


GtkWidget *
gth_browser_get_thumbnail_list (GthBrowser *browser)
{
	return browser->priv->thumbnail_list;
}


GtkWidget *
gth_browser_get_thumbnail_list_view (GthBrowser *browser)
{
	return gth_file_list_get_view (GTH_FILE_LIST (browser->priv->thumbnail_list));
}


GthFileSource *
gth_browser_get_location_source (GthBrowser *browser)
{
	return browser->priv->location_source;
}


GthFileStore *
gth_browser_get_file_store (GthBrowser *browser)
{
	return GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (gth_browser_get_file_list_view (browser))));
}


GtkWidget *
gth_browser_get_folder_tree (GthBrowser *browser)
{
	return browser->priv->folder_tree;
}


void
gth_browser_get_sort_order (GthBrowser        *browser,
			    GthFileDataSort **sort_type,
			    gboolean         *inverse)
{
	if (sort_type != NULL)
		*sort_type = browser->priv->current_sort_type;
	if (inverse != NULL)
		*inverse = browser->priv->current_sort_inverse;
}


void
gth_browser_set_sort_order (GthBrowser      *browser,
			    GthFileDataSort *sort_type,
			    gboolean         sort_inverse)
{
	g_return_if_fail (sort_type != NULL);

	browser->priv->default_sort_type = sort_type;
	browser->priv->default_sort_inverse = sort_inverse;
	_gth_browser_set_sort_order (browser, sort_type, sort_inverse, TRUE, TRUE);
}


void
gth_browser_get_file_list_info (GthBrowser *browser,
				int        *current_position,
				int        *n_visibles)
{
	if (current_position != NULL)
		*current_position = browser->priv->current_file_position;
	if (n_visibles != NULL)
		*n_visibles = browser->priv->n_visibles;
}


void
gth_browser_stop (GthBrowser *browser)
{
	_gth_browser_cancel (browser, NULL, NULL);
}


void
gth_browser_reload (GthBrowser *browser)
{
	if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_BROWSER_PAGE_BROWSER)
		return;
	gth_browser_go_to (browser, browser->priv->location->file, NULL);
}


typedef struct {
	GthBrowser   *browser;
	GthTask      *task;
	GthTaskFlags  flags;
	gulong        completed_event;
} TaskData;


static void
task_data_free (TaskData *task_data)
{
	g_object_unref (task_data->task);
	g_object_unref (task_data->browser);
	g_free (task_data);
}


static void
background_task_completed_cb (GthTask  *task,
			      GError   *error,
			      gpointer  user_data)
{
	TaskData   *task_data = user_data;
	GthBrowser *browser = task_data->browser;

	_gth_browser_remove_activity (browser);

	if (gth_task_get_for_viewer (task_data->task))
		browser->priv->viewer_tasks = g_list_remove (browser->priv->viewer_tasks, task_data);
	else
		browser->priv->background_tasks = g_list_remove (browser->priv->background_tasks, task_data);
	g_signal_handler_disconnect (task, task_data->completed_event);
	task_data_free (task_data);

	gtk_widget_set_visible (browser->priv->show_progress_dialog_button, (browser->priv->viewer_tasks != NULL) || (browser->priv->background_tasks != NULL));

	if (error == NULL)
		return;

	if ((error->domain == G_IO_ERROR)
	    && ! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)
	    && ((task_data->flags & GTH_TASK_FLAGS_IGNORE_ERROR) == 0))
	{
		_gth_browser_show_error (browser, _("Could not perform the operation"), error);
	}
}


static TaskData *
task_data_new (GthBrowser   *browser,
	       GthTask      *task,
	       GthTaskFlags  flags)
{
	TaskData *task_data;

	task_data = g_new0 (TaskData, 1);
	task_data->browser = g_object_ref (browser);
	task_data->task = g_object_ref (task);
	task_data->flags = flags;
	task_data->completed_event = g_signal_connect (task_data->task,
						       "completed",
						       G_CALLBACK (background_task_completed_cb),
						       task_data);
	return task_data;
}


static void
_gth_browser_exec_foreground_task (GthBrowser   *browser,
				   GthTask      *task);


static void
foreground_task_completed_cb (GthTask    *task,
			      GError     *error,
			      GthBrowser *browser)
{
	g_return_if_fail (task == browser->priv->task);

	_gth_browser_remove_activity (browser);

	g_signal_handler_disconnect (browser->priv->task, browser->priv->task_completed);
	g_signal_handler_disconnect (browser->priv->task, browser->priv->task_progress);

	gth_browser_update_sensitivity (browser);
	if ((error != NULL) && ! g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED))
		_gth_browser_show_error (browser, _("Could not perform the operation"), error);

	g_object_unref (browser->priv->task);
	browser->priv->task = NULL;

	if (browser->priv->next_task != NULL) {
		_gth_browser_exec_foreground_task (browser, browser->priv->next_task);
		_g_clear_object (&browser->priv->next_task);
	}
}


static void
foreground_task_progress_cb (GthTask    *task,
			     const char *description,
			     const char *details,
			     gboolean    pulse,
			     double      fraction,
			     GthBrowser *browser)
{
	/* void */
}


static void
_gth_browser_exec_foreground_task (GthBrowser   *browser,
				   GthTask      *task)
{
	g_return_if_fail (browser->priv->task == NULL);

	browser->priv->task = g_object_ref (task);
	browser->priv->task_completed = g_signal_connect (task,
							  "completed",
							  G_CALLBACK (foreground_task_completed_cb),
							  browser);
	browser->priv->task_progress = g_signal_connect (task,
							 "progress",
							 G_CALLBACK (foreground_task_progress_cb),
							 browser);
	_gth_browser_add_activity (browser);
	gth_browser_update_sensitivity (browser);
	gth_task_exec (browser->priv->task, NULL);
}


void
gth_browser_exec_task (GthBrowser   *browser,
		       GthTask      *task,
		       GthTaskFlags  flags)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));
	g_return_if_fail (task != NULL);

	if ((flags & GTH_TASK_FLAGS_FOREGROUND) == 0) {
		TaskData *task_data;

		_gth_browser_add_activity (browser);

		task_data = task_data_new (browser, task, flags);
		if (gth_task_get_for_viewer (task))
			browser->priv->viewer_tasks = g_list_prepend (browser->priv->viewer_tasks, task_data);
		else
			browser->priv->background_tasks = g_list_prepend (browser->priv->background_tasks, task_data);

		if (browser->priv->progress_dialog == NULL) {
			browser->priv->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (browser));
			g_object_add_weak_pointer (G_OBJECT (browser->priv->progress_dialog), (gpointer*) &(browser->priv->progress_dialog));
		}
		gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (browser->priv->progress_dialog), task, flags);
		gtk_widget_show (browser->priv->show_progress_dialog_button);

		return;
	}

	/* foreground task */

	if (browser->priv->task != NULL) {
		_g_object_unref (browser->priv->next_task);
		browser->priv->next_task = g_object_ref (task);
		gth_task_cancel (task);
		return;
	}

	_gth_browser_exec_foreground_task (browser, task);
}


GthTask *
gth_browser_get_foreground_task (GthBrowser *browser)
{
	return browser->priv->task;
}


void
gth_browser_set_close_with_task (GthBrowser *browser,
				 gboolean    value)
{
	browser->priv->close_with_task = value;
}


gboolean
gth_browser_get_close_with_task (GthBrowser *browser)
{
	return browser->priv->close_with_task;
}


GtkWidget *
gth_browser_get_location_bar (GthBrowser *browser)
{
	return browser->priv->location_bar;
}


GtkWidget *
gth_browser_get_list_info_bar (GthBrowser *browser)
{
	return browser->priv->list_info_bar;
}


gboolean
gth_browser_viewer_button_press_cb (GthBrowser     *browser,
				    GdkEventButton *event)
{
	g_return_val_if_fail (event != NULL, FALSE);

	if (event->button == 3) {
		gth_browser_file_menu_popup (browser, event);
		return TRUE;
	}

	return FALSE;
}


gboolean
gth_browser_viewer_scroll_event_cb (GthBrowser     *browser,
				    GdkEventScroll *event)
{
	gboolean handled;

	g_return_val_if_fail (event != NULL, FALSE);

	if (! _gth_browser_can_change_image (browser))
		return FALSE;

	if (event->state & GDK_SHIFT_MASK)
		return FALSE;

	if (event->state & GDK_CONTROL_MASK)
		return FALSE;

	if ((event->direction != GDK_SCROLL_UP) && (event->direction != GDK_SCROLL_DOWN))
		return FALSE;

	handled = FALSE;
	switch (browser->priv->scroll_action) {
	case GTH_SCROLL_ACTION_CHANGE_FILE:
		if (event->direction == GDK_SCROLL_UP)
			gth_browser_show_prev_image (browser, FALSE, FALSE);
		else
			gth_browser_show_next_image (browser, FALSE, FALSE);
		handled = TRUE;
		break;

	case GTH_SCROLL_ACTION_ZOOM:
		handled = gth_viewer_page_zoom_from_scroll (browser->priv->viewer_page, event);
		break;

	default:
		break;
	}

	return handled;
}


static void
gth_browser_toggle_properties_on_screen (GthBrowser *browser)
{
	gboolean on_screen;

	on_screen = browser->priv->fullscreen && (GTH_VIEWER_PAGE_GET_INTERFACE (browser->priv->viewer_page)->show_properties != NULL);
	if (on_screen) {
		browser->priv->properties_on_screen = ! browser->priv->properties_on_screen;
		gth_viewer_page_show_properties (browser->priv->viewer_page, browser->priv->properties_on_screen);
	}
	else if (browser->priv->viewer_sidebar != GTH_SIDEBAR_STATE_PROPERTIES)
		gth_browser_show_file_properties (browser);
	else
		gth_browser_hide_sidebar (browser);
}


gboolean
gth_browser_viewer_key_press_cb (GthBrowser  *browser,
				 GdkEventKey *event)
{
	gboolean activated;

	g_return_val_if_fail (event != NULL, FALSE);

	activated = gth_window_activate_shortcut (GTH_WINDOW (browser),
						  GTH_SHORTCUT_CONTEXT_VIEWER,
						  gth_viewer_page_get_shortcut_context (browser->priv->viewer_page),
						  event->keyval,
						  event->state);

	if (! activated && gtk_widget_get_realized (browser->priv->file_list))
		activated = gth_hook_invoke_get ("gth-browser-file-list-key-press", browser, event) != NULL;

	return activated;
}


void
gth_browser_set_viewer_widget (GthBrowser *browser,
			       GtkWidget  *widget)
{
	GtkWidget *child;

	child = gth_browser_get_viewer_widget (browser);
	if (child != NULL)
		gtk_widget_destroy (child);
	if (widget != NULL)
		gtk_container_add (GTK_CONTAINER (browser->priv->viewer_container), widget);
}


GtkWidget *
gth_browser_get_viewer_widget (GthBrowser *browser)
{
	return gtk_bin_get_child (GTK_BIN (browser->priv->viewer_container));
}


GthViewerPage *
gth_browser_get_viewer_page (GthBrowser *browser)
{
	return browser->priv->viewer_page;
}


GtkWidget *
gth_browser_get_viewer_sidebar (GthBrowser *browser)
{
	return browser->priv->file_properties;
}


static gboolean
view_focused_image (GthBrowser *browser)
{
	GthFileView *view;
	int          n;
	GtkTreeIter  iter;
	GthFileData *focused_file = NULL;

	if (browser->priv->current_file == NULL)
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));
	n = gth_file_view_get_cursor (view);
	if (n == -1)
		return FALSE;

	if (! gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), n, &iter))
		return FALSE;

	focused_file = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
	if (focused_file == NULL)
		return FALSE;

	return ! g_file_equal (browser->priv->current_file->file, focused_file->file);
}


static void _gth_browser_load_file_keep_view (GthBrowser  *browser,
					      GthFileData *file_data,
					      gboolean     view,
					      gboolean     no_delay);


gboolean
gth_browser_show_next_image (GthBrowser *browser,
			     gboolean    skip_broken,
			     gboolean    only_selected)
{
	GthFileView *view;
	int          pos;

	if (! _gth_browser_can_change_image (browser))
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));

	if (browser->priv->current_file == NULL) {
		pos = gth_file_list_next_file (GTH_FILE_LIST (browser->priv->file_list), -1, skip_broken, only_selected, TRUE);
	}
	else if (view_focused_image (browser)) {
		pos = gth_file_view_get_cursor (view);
		if (pos < 0)
			pos = gth_file_list_next_file (GTH_FILE_LIST (browser->priv->file_list), -1, skip_broken, only_selected, TRUE);
	}
	else {
		pos = gth_file_store_get_pos (gth_browser_get_file_store (browser), browser->priv->current_file->file);
		pos = gth_file_list_next_file (GTH_FILE_LIST (browser->priv->file_list), pos, skip_broken, only_selected, FALSE);
	}

	if (pos >= 0) {
		GtkTreeIter  iter;

		if (gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), pos, &iter)) {
			GthFileData *file_data;

			file_data = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
			_gth_browser_load_file_keep_view (browser, file_data, TRUE, TRUE);
		}
	}
	else
		gdk_window_beep (gtk_widget_get_window (GTK_WIDGET (browser)));

	return (pos >= 0);
}


gboolean
gth_browser_show_prev_image (GthBrowser *browser,
			     gboolean    skip_broken,
			     gboolean    only_selected)
{
	GthFileView *view;
	int          pos;

	if (! _gth_browser_can_change_image (browser))
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));

	if (browser->priv->current_file == NULL) {
		pos = gth_file_list_prev_file (GTH_FILE_LIST (browser->priv->file_list), -1, skip_broken, only_selected, TRUE);
	}
	else if (view_focused_image (browser)) {
		pos = gth_file_view_get_cursor (view);
		if (pos < 0)
			pos = gth_file_list_prev_file (GTH_FILE_LIST (browser->priv->file_list), -1, skip_broken, only_selected, TRUE);
	}
	else {
		pos = gth_file_store_get_pos (gth_browser_get_file_store (browser), browser->priv->current_file->file);
		pos = gth_file_list_prev_file (GTH_FILE_LIST (browser->priv->file_list), pos, skip_broken, only_selected, FALSE);
	}

	if (pos >= 0) {
		GtkTreeIter iter;

		if (gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), pos, &iter)) {
			GthFileData *file_data;

			file_data = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
			_gth_browser_load_file_keep_view (browser, file_data, TRUE, TRUE);
		}
	}
	else
		gdk_window_beep (gtk_widget_get_window (GTK_WIDGET (browser)));

	return (pos >= 0);
}


gboolean
gth_browser_show_first_image (GthBrowser *browser,
			      gboolean    skip_broken,
			      gboolean    only_selected)
{
	int          pos;
	GthFileView *view;
	GtkTreeIter  iter;
	GthFileData *file_data;

	if (! _gth_browser_can_change_image (browser))
		return FALSE;

	pos = gth_file_list_first_file (GTH_FILE_LIST (browser->priv->file_list), skip_broken, only_selected);
	if (pos < 0)
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));

	if (! gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), pos, &iter))
		return FALSE;

	file_data = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
	_gth_browser_load_file_keep_view (browser, file_data, TRUE, TRUE);

	return TRUE;
}


gboolean
gth_browser_show_last_image (GthBrowser *browser,
			     gboolean    skip_broken,
			     gboolean    only_selected)
{
	int          pos;
	GthFileView *view;
	GtkTreeIter  iter;
	GthFileData *file_data;

	if (! _gth_browser_can_change_image (browser))
		return FALSE;

	pos = gth_file_list_last_file (GTH_FILE_LIST (browser->priv->file_list), skip_broken, only_selected);
	if (pos < 0)
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));

	if (! gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), pos, &iter))
		return FALSE;

	file_data = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
	_gth_browser_load_file_keep_view (browser, file_data, TRUE, TRUE);

	return TRUE;
}


/* -- gth_browser_load_file -- */


typedef struct {
	int           ref;
	GthBrowser   *browser;
	GthFileData  *file_data;
	gboolean      view;
	gboolean      fullscreen;
	GCancellable *cancellable;
} LoadFileData;


static void
cancel_all_metadata_operations (GthBrowser  *browser)
{
	GList *scan;

	for (scan = browser->priv->load_file_data_queue; scan; scan = scan->next) {
		LoadFileData *data = scan->data;
		g_cancellable_cancel (data->cancellable);
	}
}


static LoadFileData *
load_file_data_new (GthBrowser  *browser,
		    GthFileData *file_data,
		    gboolean     view,
		    gboolean     fullscreen)
{
	LoadFileData *data;

	data = g_new0 (LoadFileData, 1);
	data->ref = 1;
	data->browser = g_object_ref (browser);
	if (file_data != NULL)
		data->file_data = gth_file_data_dup (file_data);
	data->view = view;
	data->fullscreen = fullscreen;
	data->cancellable = g_cancellable_new ();

	cancel_all_metadata_operations (browser);
	browser->priv->load_file_data_queue = g_list_prepend (browser->priv->load_file_data_queue, data);

	return data;
}


static void
load_file_data_ref (LoadFileData *data)
{
	data->ref++;
}


static void
load_file_data_unref (LoadFileData *data)
{
	if (--data->ref != 0)
		return;

	data->browser->priv->load_file_data_queue = g_list_remove (data->browser->priv->load_file_data_queue, data);
	_g_object_unref (data->file_data);
	_g_object_unref (data->browser);
	_g_object_unref (data->cancellable);
	g_free (data);
}


static void
gth_viewer_page_file_loaded_cb (GthViewerPage *viewer_page,
				GthFileData   *file_data,
				GFileInfo     *updated_metadata,
				gboolean       success,
				gpointer       user_data);


static void
_gth_browser_set_current_viewer_page (GthBrowser    *browser,
				      GthViewerPage *registered_viewer_page)
{
	if ((browser->priv->viewer_page != NULL) && (G_OBJECT_TYPE (registered_viewer_page) != G_OBJECT_TYPE (browser->priv->viewer_page)))
		_gth_browser_deactivate_viewer_page (browser);

	if (browser->priv->viewer_page == NULL) {
		browser->priv->viewer_page = g_object_new (G_OBJECT_TYPE (registered_viewer_page), NULL);
		gth_viewer_page_activate (browser->priv->viewer_page, browser);
		gth_hook_invoke ("gth-browser-activate-viewer-page", browser);
		_gth_browser_show_pointer_on_viewer (browser, FALSE);

		g_signal_connect (browser->priv->viewer_page,
				  "file-loaded",
				  G_CALLBACK (gth_viewer_page_file_loaded_cb),
				  browser);
	}
}


static void
file_metadata_ready_cb (GList    *files,
			GError   *error,
			gpointer  user_data)
{
	LoadFileData *data = user_data;
	GthBrowser   *browser = data->browser;
	GthFileData  *file_data;
	gboolean      different_mime_type;

	if ((error != NULL) || (files == NULL)) {
		load_file_data_unref (data);
		return;
	}

	file_data = files->data;
	if ((browser->priv->current_file == NULL) || ! _g_file_equal (file_data->file, browser->priv->current_file->file)) {
		load_file_data_unref (data);
		return;
	}

	/* the mime type can be different for example when a jpeg image has a .png extension */
	different_mime_type = ! g_str_equal (gth_file_data_get_mime_type (browser->priv->current_file), gth_file_data_get_mime_type (file_data));

	_g_file_info_update (browser->priv->current_file->info, file_data->info);

	gth_browser_update_title (browser);
	gth_browser_update_statusbar_file_info (browser);
	gth_sidebar_set_file (GTH_SIDEBAR (browser->priv->file_properties), browser->priv->current_file);
	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER) {
		_gth_browser_make_file_visible (browser, browser->priv->current_file);
		_gth_browser_update_statusbar_list_info (browser);
	}
	gth_browser_update_sensitivity (browser);
	if (browser->priv->viewer_page != NULL) {
		GthViewerPage *basic_viewer_page;

		/* The basic viewer is registered before any other viewer, so
		 * it's the last one in the viewer_pages list. */

		basic_viewer_page = g_list_last (browser->priv->viewer_pages)->data;

		/* If after reading the metadata we got a different mime type
		 * and the current viewer is the default file viewer it's likely
		 * that we have an image with a wrong extension.  Try to
		 * load the file again to see if the mime type can be viewed by
		 * a different viewer_page. */

		if (different_mime_type && (G_OBJECT_TYPE (browser->priv->viewer_page) == G_OBJECT_TYPE (basic_viewer_page)))
			_gth_browser_load_file_more_options (browser, file_data, data->view, data->fullscreen, data->view);
		else
			gth_viewer_page_update_info (browser->priv->viewer_page, browser->priv->current_file);
	}

	/* location is NULL if the file has been loaded because requested
	 * from the command line */
	if (browser->priv->location == NULL) {
		GFile *parent;

		parent = g_file_get_parent (file_data->file);
		_gth_browser_load (browser, parent, file_data->file, NULL, 0, GTH_ACTION_GO_TO, FALSE);
		g_object_unref (parent);
	}

	load_file_data_unref (data);
}


static gboolean
load_metadata_cb (gpointer user_data)
{
	LoadFileData *data = user_data;
	GthBrowser   *browser = data->browser;
	GList        *files;

	if (browser->priv->load_metadata_timeout != 0) {
		g_source_remove (browser->priv->load_metadata_timeout);
		browser->priv->load_metadata_timeout = 0;
	}

	if ((browser->priv->current_file == NULL) ||
	    ! _g_file_equal (data->file_data->file, browser->priv->current_file->file))
	{
		return FALSE;
	}

	load_file_data_ref (data);
	files = g_list_prepend (NULL, data->file_data->file);
	_g_query_all_metadata_async (files,
				     GTH_LIST_DEFAULT,
				     "*",
				     data->cancellable,
				     file_metadata_ready_cb,
				     data);

	g_list_free (files);

	return FALSE;
}


static void
gth_viewer_page_file_loaded_cb (GthViewerPage *viewer_page,
				GthFileData   *file_data,
				GFileInfo     *updated_metadata,
				gboolean       success,
				gpointer       user_data)
{
	GthBrowser   *browser = user_data;
	LoadFileData *data;

	if ((browser->priv->current_file == NULL) || ! g_file_equal (file_data->file, browser->priv->current_file->file))
		return;

	if (! success) {
		GthViewerPage *basic_viewer_page;

		/* Use the basic viewer if the default viewer failed.  The
		 * basic viewer is registered before any other viewer, so it's
		 * the last one in the viewer_pages list. */

		basic_viewer_page = g_list_last (browser->priv->viewer_pages)->data;
		_gth_browser_set_current_viewer_page (browser, basic_viewer_page);
		if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER)
			gth_viewer_page_show (browser->priv->viewer_page);
		gth_viewer_page_view (browser->priv->viewer_page, browser->priv->current_file);

		return;
	}

	_g_file_info_update (browser->priv->current_file->info, updated_metadata);
	g_file_info_set_attribute_boolean (browser->priv->current_file->info, "gth::file::is-modified", FALSE);

	if (browser->priv->load_metadata_timeout != 0)
		g_source_remove (browser->priv->load_metadata_timeout);

	data = load_file_data_new (browser, browser->priv->current_file, FALSE, FALSE);
	browser->priv->load_metadata_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT,
								   LOAD_METADATA_DELAY,
								   load_metadata_cb,
								   data,
								   (GDestroyNotify) load_file_data_unref);
}


static void
_gth_browser_load_file (GthBrowser  *browser,
			GthFileData *file_data,
			gboolean     view,
			gboolean     fullcreen)
{
	GList *scan;

	if (file_data == NULL) {
		_gth_browser_deactivate_viewer_page (browser);
		_g_object_unref (browser->priv->current_file);
		browser->priv->current_file = NULL;

		gth_sidebar_set_file (GTH_SIDEBAR (browser->priv->file_properties), NULL);

		gth_browser_update_statusbar_file_info (browser);
		gth_browser_update_title (browser);
		gth_browser_update_sensitivity (browser);

		return;
	}

	_g_object_unref (browser->priv->current_file);
	browser->priv->current_file = gth_file_data_dup (file_data);

	_gth_browser_update_current_file_position (browser);
	gth_browser_update_statusbar_file_info (browser);

	if (browser->priv->viewer_pages == NULL)
		browser->priv->viewer_pages = g_list_reverse (gth_main_get_registered_objects (GTH_TYPE_VIEWER_PAGE));

	for (scan = browser->priv->viewer_pages; scan; scan = scan->next) {
		GthViewerPage *registered_viewer_page = scan->data;

		if (gth_viewer_page_can_view (registered_viewer_page, browser->priv->current_file)) {
			_gth_browser_set_current_viewer_page (browser, registered_viewer_page);
			break;
		}
	}

	if (view) {
		if (fullcreen) {
			if (! gth_browser_get_is_fullscreen (browser))
				gth_browser_fullscreen (browser);
		}
		else
			gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
	}

	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER) {
		gth_viewer_page_show (browser->priv->viewer_page);
		if (browser->priv->fullscreen) {
			gth_viewer_page_fullscreen (browser->priv->viewer_page, TRUE);
			_gth_browser_show_pointer_on_viewer (browser, FALSE);
		}
		_gth_browser_make_file_visible (browser, browser->priv->current_file);
	}

	if (browser->priv->viewer_page != NULL)
		gth_viewer_page_view (browser->priv->viewer_page, browser->priv->current_file);
	else
		gth_viewer_page_file_loaded_cb (NULL, browser->priv->current_file, NULL, FALSE, browser);
}


static void
load_file__previuos_file_saved_cb (GthBrowser *browser,
				   gboolean    cancelled,
				   gpointer    user_data)
{
	LoadFileData *data = user_data;

	if (! cancelled)
		_gth_browser_load_file (data->browser, data->file_data, data->view, data->fullscreen);

	load_file_data_unref (data);
}


static gboolean
load_file_delayed_cb (gpointer user_data)
{
	LoadFileData *data = user_data;
	GthBrowser   *browser = data->browser;

	load_file_data_ref (data);

	if (browser->priv->load_file_timeout != 0) {
		g_source_remove (browser->priv->load_file_timeout);
		browser->priv->load_file_timeout = 0;
	}

	if (browser->priv->ask_to_save_modified_images && gth_browser_get_file_modified (browser)) {
		load_file_data_ref (data);
		gth_browser_ask_whether_to_save (browser,
						 load_file__previuos_file_saved_cb,
						 data);
	}
	else
		_gth_browser_load_file (data->browser, data->file_data, data->view, data->fullscreen);

	load_file_data_unref (data);

	return FALSE;
}


static void
_gth_browser_load_file_more_options (GthBrowser  *browser,
				     GthFileData *file_data,
				     gboolean     view,
				     gboolean     fullscreen,
				     gboolean     no_delay)
{
	LoadFileData *data;

	_gth_browser_hide_infobar (browser);

	if (browser->priv->load_file_timeout != 0) {
		g_source_remove (browser->priv->load_file_timeout);
		browser->priv->load_file_timeout = 0;
	}

	if (browser->priv->load_metadata_timeout != 0) {
		g_source_remove (browser->priv->load_metadata_timeout);
		browser->priv->load_metadata_timeout = 0;
	}

	data = load_file_data_new (browser, file_data, view, fullscreen);
	if (no_delay) {
		load_file_delayed_cb (data);
		load_file_data_unref (data);
	}
	else
		browser->priv->load_file_timeout =
				g_timeout_add_full (G_PRIORITY_DEFAULT,
						    LOAD_FILE_DELAY,
						    load_file_delayed_cb,
						    data,
						    (GDestroyNotify) load_file_data_unref);
}


static void
_gth_browser_load_file_keep_view (GthBrowser  *browser,
				  GthFileData *file_data,
				  gboolean     view,
				  gboolean     no_delay)
{
	if (browser->priv->view_files_in_fullscreen && gth_browser_get_is_fullscreen (browser))
		view = FALSE;
	else if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER)
		view = FALSE;
	_gth_browser_load_file_more_options (browser, file_data, view, browser->priv->view_files_in_fullscreen, no_delay);
}


void
gth_browser_load_file (GthBrowser  *browser,
		       GthFileData *file_data,
		       gboolean     view)
{
	_gth_browser_load_file_keep_view (browser, file_data, view, view);
}


void
gth_browser_show_file_properties (GthBrowser *browser)
{
	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
	case GTH_WINDOW_PAGE_UNDEFINED: /* --> when called from gth_browser_init */
		g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_VISIBLE, TRUE);
		gth_window_change_action_state (GTH_WINDOW (browser), "browser-properties", TRUE);
		if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_WINDOW_PAGE_UNDEFINED)
			gtk_widget_show (browser->priv->file_properties);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		gth_window_change_action_state (GTH_WINDOW (browser), "viewer-edit-file", FALSE);
		browser->priv->viewer_sidebar = GTH_SIDEBAR_STATE_PROPERTIES;
		gth_window_change_action_state (GTH_WINDOW (browser), "viewer-properties", TRUE);
		gtk_widget_show (browser->priv->viewer_sidebar_container);
		gtk_widget_show (browser->priv->file_properties);
		gth_sidebar_show_properties (GTH_SIDEBAR (browser->priv->file_properties));
		break;
	}
}


void
gth_browser_show_viewer_tools (GthBrowser *browser)
{
	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);

	gth_window_change_action_state (GTH_WINDOW (browser), "viewer-properties", FALSE);
	browser->priv->viewer_sidebar = GTH_SIDEBAR_STATE_TOOLS;
	gth_window_change_action_state (GTH_WINDOW (browser), "viewer-edit-file", TRUE);
	gtk_widget_show (browser->priv->viewer_sidebar_container);
	gtk_widget_show (browser->priv->file_properties);
	gth_sidebar_show_tools (GTH_SIDEBAR (browser->priv->file_properties));
}


void
gth_browser_toggle_file_properties (GthBrowser  *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		if (gth_window_get_action_state (GTH_WINDOW (browser), "browser-properties"))
			gth_browser_hide_sidebar (browser);
		else
			gth_browser_show_file_properties (browser);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		gth_browser_toggle_properties_on_screen (browser);
		break;

	default:
		break;
	}
}


void
gth_browser_toggle_viewer_tools (GthBrowser  *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		if (browser->priv->viewer_page != NULL)
			gth_browser_show_viewer_tools (GTH_BROWSER (browser));
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		if (browser->priv->viewer_sidebar != GTH_SIDEBAR_STATE_TOOLS)
			gth_browser_show_viewer_tools (browser);
		else
			gth_browser_hide_sidebar (browser);
		break;

	default:
		break;
	}
}


void
gth_browser_hide_sidebar (GthBrowser *browser)
{
	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_VISIBLE, FALSE);
		gth_window_change_action_state (GTH_WINDOW (browser), "browser-properties", FALSE);
		gtk_widget_hide (browser->priv->file_properties);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		if (! gth_sidebar_tool_is_active (GTH_SIDEBAR (browser->priv->file_properties))) {
			if (browser->priv->viewer_sidebar == GTH_SIDEBAR_STATE_PROPERTIES)
				gth_window_change_action_state (GTH_WINDOW (browser), "viewer-properties", FALSE);
			else if (browser->priv->viewer_sidebar == GTH_SIDEBAR_STATE_TOOLS)
				gth_window_change_action_state (GTH_WINDOW (browser), "viewer-edit-file", FALSE);
			browser->priv->viewer_sidebar = GTH_SIDEBAR_STATE_HIDDEN;
			gtk_widget_hide (browser->priv->viewer_sidebar_container);
		}
		break;
	}
}


/* -- gth_browser_load_location -- */


typedef struct {
	GthFileSource *file_source;
	GthFileData   *location_data;
	GthBrowser    *browser;
} LoadLocationData;


static void
load_location_data_free (LoadLocationData *data)
{
	g_object_unref (data->location_data);
	g_object_unref (data->file_source);
	g_free (data);
}


static void
load_file_attributes_ready_cb (GObject  *object,
			       GError   *error,
			       gpointer  user_data)
{
	LoadLocationData *data = user_data;
	GthBrowser       *browser = data->browser;

	if (error == NULL) {
		if (g_file_info_get_file_type (data->location_data->info) == G_FILE_TYPE_REGULAR) {
			GFile *parent;

			parent = g_file_get_parent (data->location_data->file);
			if ((browser->priv->location != NULL) && ! g_file_equal (parent, browser->priv->location->file)) {
				/* set location to NULL to force a folder reload */
				_g_object_unref (browser->priv->location);
				browser->priv->location = NULL;
			}

			gth_browser_load_file (browser, data->location_data, TRUE);

			g_object_unref (parent);
		}
		else if (g_file_info_get_file_type (data->location_data->info) == G_FILE_TYPE_DIRECTORY) {
			gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
			gth_browser_go_to (browser, data->location_data->file, NULL);
		}
		else {
			char   *title;
			GError *error;

			title = _g_format_str_for_file (_("Could not load the position “%s”"), data->location_data->file);
			error = g_error_new (GTH_ERROR, 0, _("File type not supported"));
			_gth_browser_show_error (browser, title, error);
			g_clear_error (&error);

			g_free (title);
		}
	}
	else if (browser->priv->location == NULL) {
		GFile *home;

		home = g_file_new_for_uri (_g_uri_get_home ());
		gth_browser_load_location (browser, home);

		g_object_unref (home);
	}
	else {
		char *title;

		title = _g_format_str_for_file (_("Could not load the position “%s”"), data->location_data->file);
		_gth_browser_show_error (browser, title, error);

		g_free (title);
	}

	load_location_data_free (data);
}


void
gth_browser_load_location (GthBrowser *browser,
		  	   GFile      *location)
{
	LoadLocationData *data;

	data = g_new0 (LoadLocationData, 1);
	data->browser = browser;
	data->location_data = gth_file_data_new (location, NULL);
	data->file_source = gth_main_get_file_source (data->location_data->file);
	if (data->file_source == NULL) {
		char   *title;
		GError *error;

		title = _g_format_str_for_file (_("Could not load the position “%s”"), data->location_data->file);
		error = g_error_new (GTH_ERROR, 0, _("No suitable module found"));
		_gth_browser_show_error (browser, title, error);
		g_clear_error (&error);

		g_free (title);
	}

	gth_file_source_read_metadata (data->file_source,
				       data->location_data,
				       GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
				       load_file_attributes_ready_cb,
				       data);
}


/* -- _gth_browser_cancel -- */


#define CHECK_CANCELLABLE_INTERVAL 100


typedef struct {
	GthBrowser *browser;
	DataFunc    done_func;
	gpointer    user_data;
	gulong      check_id;
} CancelData;


static void
cancel_data_unref (CancelData *cancel_data)
{
	g_object_unref (cancel_data->browser);
	g_free (cancel_data);
}


static gboolean
check_cancellable_cb (gpointer user_data)
{
	CancelData *cancel_data = user_data;
	GthBrowser *browser = cancel_data->browser;

	if ((browser->priv->load_data_queue == NULL)
	    && (browser->priv->load_file_data_queue == NULL)
	    && (browser->priv->task == NULL)
	    && (browser->priv->viewer_tasks == NULL))
	{
		g_source_remove (cancel_data->check_id);
		cancel_data->check_id = 0;

		if (cancel_data->done_func != NULL)
			cancel_data->done_func (cancel_data->user_data);
		cancel_data_unref (cancel_data);

		return FALSE;
	}

	return TRUE;
}


static void
_gth_browser_cancel (GthBrowser *browser,
		     DataFunc    done_func,
		     gpointer    user_data)
{
	CancelData *cancel_data;
	GList      *scan;

	cancel_data = g_new0 (CancelData, 1);
	cancel_data->browser = g_object_ref (browser);
	cancel_data->done_func = done_func;
	cancel_data->user_data = user_data;

	if (browser->priv->load_file_timeout != 0) {
		g_source_remove (browser->priv->load_file_timeout);
		browser->priv->load_file_timeout = 0;
	}

	if (browser->priv->load_metadata_timeout != 0) {
		g_source_remove (browser->priv->load_metadata_timeout);
		browser->priv->load_metadata_timeout = 0;
	}

	for (scan = browser->priv->load_data_queue; scan; scan = scan->next) {
		LoadData *data = scan->data;

		if (data->file_source != NULL)
			gth_file_source_cancel (data->file_source);
		g_cancellable_cancel (data->cancellable);
	}

	for (scan = browser->priv->load_file_data_queue; scan; scan = scan->next) {
		LoadFileData *data = scan->data;
		g_cancellable_cancel (data->cancellable);
	}

	for (scan = browser->priv->viewer_tasks; scan; scan = scan->next) {
		TaskData *data = scan->data;
		if (gth_task_is_running (data->task))
			gth_task_cancel (data->task);
	}

	if ((browser->priv->task != NULL) && gth_task_is_running (browser->priv->task)) {
		if (browser->priv->next_task != NULL)
			_g_clear_object (&browser->priv->next_task);
		gth_task_cancel (browser->priv->task);
	}

	cancel_data->check_id = g_timeout_add (CHECK_CANCELLABLE_INTERVAL,
					       check_cancellable_cb,
					       cancel_data);
}


gpointer
gth_browser_get_image_preloader (GthBrowser *browser)
{
	return g_object_ref (browser->priv->image_preloader);
}


void
gth_browser_register_viewer_control (GthBrowser *browser,
				     GtkWidget  *widget)
{
	browser->priv->viewer_controls = g_list_prepend (browser->priv->viewer_controls, widget);
}


void
gth_browser_unregister_viewer_control (GthBrowser *browser,
				       GtkWidget  *widget)
{
	browser->priv->viewer_controls = g_list_remove (browser->priv->viewer_controls, widget);
}


void
gth_browser_fullscreen (GthBrowser *browser)
{
	if (browser->priv->fullscreen) {
		gth_browser_unfullscreen (browser);
		return;
	}

	if (browser->priv->current_file == NULL) {
		if (! gth_browser_show_first_image (browser, FALSE, FALSE)) {
			browser->priv->fullscreen = FALSE;
			return;
		}
	}

	browser->priv->was_fullscreen = FALSE;
	browser->priv->fullscreen = TRUE;

	browser->priv->before_fullscreen.page = gth_window_get_current_page (GTH_WINDOW (browser));
	browser->priv->before_fullscreen.thumbnail_list = gth_window_get_action_state (GTH_WINDOW (browser), "show-thumbnail-list");
	browser->priv->before_fullscreen.browser_properties = gth_window_get_action_state (GTH_WINDOW (browser), "browser-properties");
	browser->priv->before_fullscreen.viewer_sidebar = browser->priv->viewer_sidebar;

	if (browser->priv->fullscreen_headerbar == NULL) {
		browser->priv->fullscreen_headerbar = gth_window_get_header_bar (GTH_WINDOW (browser));

		gtk_widget_set_margin_top (browser->priv->viewer_sidebar_container,
					   gtk_widget_get_allocated_height (browser->priv->fullscreen_headerbar));

		g_object_ref (browser->priv->fullscreen_headerbar);
		gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (browser->priv->fullscreen_headerbar)), browser->priv->fullscreen_headerbar);
		gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (browser->priv->fullscreen_headerbar), FALSE);
		gtk_box_pack_start (GTK_BOX (browser->priv->fullscreen_toolbar), browser->priv->fullscreen_headerbar, TRUE, TRUE, 0);
		g_object_unref (browser->priv->fullscreen_headerbar);
	}

	g_list_free (browser->priv->viewer_controls);
	browser->priv->viewer_controls = g_list_append (NULL, browser->priv->fullscreen_toolbar);

	gtk_window_fullscreen (GTK_WINDOW (browser));

	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
	if (browser->priv->fullscreen_state.sidebar == GTH_SIDEBAR_STATE_PROPERTIES)
		gth_browser_show_file_properties (browser);
	else if (browser->priv->fullscreen_state.sidebar == GTH_SIDEBAR_STATE_TOOLS)
		gth_browser_show_viewer_tools (browser);
	else
		gth_browser_hide_sidebar (browser);

	_gth_browser_set_thumbnail_list_visibility (browser, browser->priv->fullscreen_state.thumbnail_list);

	gth_window_show_only_content (GTH_WINDOW (browser), TRUE);

	browser->priv->properties_on_screen = FALSE;

	if (browser->priv->viewer_page != NULL) {
		gth_viewer_page_show_properties (browser->priv->viewer_page, browser->priv->properties_on_screen);
		gth_viewer_page_fullscreen (browser->priv->viewer_page, TRUE);
		_gth_browser_show_pointer_on_viewer (browser, FALSE);
	}

	gth_browser_update_sensitivity (browser);
	browser->priv->was_fullscreen = browser->priv->fullscreen;
}


void
gth_browser_unfullscreen (GthBrowser *browser)
{
	browser->priv->was_fullscreen = TRUE;
	browser->priv->fullscreen = FALSE;

	if (browser->priv->fullscreen_headerbar != NULL) {
		g_object_ref (browser->priv->fullscreen_headerbar);
		gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (browser->priv->fullscreen_headerbar)), browser->priv->fullscreen_headerbar);
		gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (browser->priv->fullscreen_headerbar), TRUE);
		gth_window_set_header_bar (GTH_WINDOW (browser), browser->priv->fullscreen_headerbar);
		g_object_unref (browser->priv->fullscreen_headerbar);
		browser->priv->fullscreen_headerbar = NULL;
	}
	gtk_widget_set_margin_top (browser->priv->viewer_sidebar_container, 0);
	gtk_widget_hide (browser->priv->fullscreen_toolbar);

	gth_window_show_only_content (GTH_WINDOW (browser), FALSE);

	if (! gth_sidebar_tool_is_active (GTH_SIDEBAR (browser->priv->file_properties)))
		browser->priv->fullscreen_state.sidebar = browser->priv->viewer_sidebar;
	browser->priv->fullscreen_state.thumbnail_list = gth_window_get_action_state (GTH_WINDOW (browser), "show-thumbnail-list");

	if (browser->priv->before_fullscreen.page < 0)
		browser->priv->before_fullscreen.page = GTH_BROWSER_PAGE_BROWSER;
	gth_window_set_current_page (GTH_WINDOW (browser), browser->priv->before_fullscreen.page);

	_gth_browser_set_thumbnail_list_visibility (browser, browser->priv->before_fullscreen.thumbnail_list);

	if (browser->priv->before_fullscreen.page == GTH_BROWSER_PAGE_BROWSER) {
		browser->priv->viewer_sidebar = browser->priv->before_fullscreen.viewer_sidebar;
		if (browser->priv->before_fullscreen.browser_properties)
			gth_browser_show_file_properties (browser);
		else
			gth_browser_hide_sidebar (browser);
	}
	else if (browser->priv->before_fullscreen.page == GTH_BROWSER_PAGE_VIEWER) {
		if (browser->priv->before_fullscreen.viewer_sidebar == GTH_SIDEBAR_STATE_PROPERTIES)
			gth_browser_show_file_properties (browser);
		else if (browser->priv->before_fullscreen.viewer_sidebar == GTH_SIDEBAR_STATE_TOOLS)
			gth_browser_show_viewer_tools (browser);
		else
			gth_browser_hide_sidebar (browser);
	}

	gtk_window_unfullscreen (GTK_WINDOW (browser));

	browser->priv->properties_on_screen = FALSE;
	if (browser->priv->viewer_page != NULL) {
		if (GTH_VIEWER_PAGE_GET_INTERFACE (browser->priv->viewer_page)->show_properties != NULL)
			gth_viewer_page_show_properties (browser->priv->viewer_page, FALSE);
		gth_viewer_page_fullscreen (browser->priv->viewer_page, FALSE);
	}
	_gth_browser_show_pointer_on_viewer (browser, TRUE);
	g_list_free (browser->priv->viewer_controls);
	browser->priv->viewer_controls = NULL;

	gth_browser_update_sensitivity (browser);
	browser->priv->was_fullscreen = browser->priv->fullscreen;
}


gboolean
gth_browser_get_is_fullscreen (GthBrowser *browser)
{
	return browser->priv->fullscreen;
}


void
gth_browser_file_menu_popup (GthBrowser     *browser,
			     GdkEventButton *event)
{
	gth_hook_invoke ("gth-browser-file-popup-before", browser);
	gtk_menu_popup_at_pointer (GTK_MENU (browser->priv->file_popup), (GdkEvent *) event);
}


void
gth_browser_save_state (GthBrowser *browser)
{
	browser_state_free (&browser->priv->state);

	browser->priv->state.saved = TRUE;
	browser->priv->state.page = gth_window_get_current_page (GTH_WINDOW (browser));
	if (browser->priv->location != NULL)
		browser->priv->state.location = g_object_ref (browser->priv->location->file);
	if (browser->priv->current_file != NULL)
		browser->priv->state.current_file = g_object_ref (browser->priv->current_file->file);
	browser->priv->state.selected = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->file_list))));
	browser->priv->state.vscroll = gtk_adjustment_get_value (gth_file_list_get_vadjustment (GTH_FILE_LIST (browser->priv->file_list)));
}


gboolean
gth_browser_restore_state (GthBrowser *browser)
{
	if (! browser->priv->state.saved)
		return FALSE;

	switch (browser->priv->state.page) {
	case GTH_BROWSER_PAGE_BROWSER:
		if (browser->priv->state.current_file != NULL) {
			gth_browser_go_to (browser,
					   browser->priv->state.location,
					   browser->priv->state.current_file);
		}
		else {
			_gth_browser_load_file_more_options (browser, NULL, FALSE, FALSE, FALSE);
			gth_browser_go_to_with_state (browser,
						      browser->priv->state.location,
						      browser->priv->state.selected,
						      browser->priv->state.vscroll);
		}
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		gth_browser_load_location (browser, browser->priv->state.current_file);
		break;

	default:
		break;
	}

	return TRUE;
}


void
gth_browser_apply_editor_changes (GthBrowser *browser)
{
	GtkWidget *toolbox;
	GtkWidget *file_tool;

	toolbox = gth_sidebar_get_toolbox (GTH_SIDEBAR (browser->priv->file_properties));
	file_tool = gth_toolbox_get_active_tool (GTH_TOOLBOX (toolbox));
	if (file_tool != NULL)
		gth_file_tool_apply_options (GTH_FILE_TOOL (file_tool));
}


GthICCProfile *
gth_browser_get_monitor_profile (GthBrowser *browser)
{
#if HAVE_LCMS2
	if (browser->priv->screen_profile == NULL) {
		int monitor_num;

		if (_gtk_window_get_monitor_info (GTK_WINDOW (browser), NULL, &monitor_num, NULL)) {
			char      *atom_name;
			GdkAtom    type    = GDK_NONE;
			int        format  = 0;
			int        nitems  = 0;
			guchar    *data    = NULL;

			if (monitor_num > 0)
				atom_name = g_strdup_printf ("_ICC_PROFILE_%d", monitor_num);
			else
				atom_name = g_strdup ("_ICC_PROFILE");

			if (gdk_property_get (gdk_screen_get_root_window (gtk_widget_get_screen (GTK_WIDGET (browser))),
			                      gdk_atom_intern (atom_name, FALSE),
					      GDK_NONE,
					      0, 64 * 1024 * 1024, FALSE,
					      &type, &format, &nitems, &data) && nitems > 0)
			{
				GthCMSProfile cms_profile;

				cms_profile = (GthCMSProfile) cmsOpenProfileFromMem (data, nitems);
				if (cms_profile != NULL) {
					char *id = g_strdup_printf ("%s%d", GTH_ICC_PROFILE_FROM_PROPERTY, monitor_num);
					browser->priv->screen_profile = gth_icc_profile_new (id, cms_profile);
					g_free (id);
				}

				g_free (data);
			}

			g_free (atom_name);
		}
	}
#endif

	if (browser->priv->screen_profile == NULL)
		browser->priv->screen_profile = gth_icc_profile_new_srgb();

	return browser->priv->screen_profile;
}


GtkWidget *
gth_browser_get_fullscreen_headerbar (GthBrowser *browser)
{
	return browser->priv->fullscreen_headerbar;
}


void
gth_browser_keep_mouse_visible (GthBrowser *browser,
			        gboolean    value)
{
	browser->priv->keep_mouse_visible = value;
}


void
gth_browser_show_menu (GthBrowser *browser)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (browser->priv->menu_button), TRUE);
}


void
gth_browser_show_progress_dialog (GthBrowser *browser)
{
	if ((browser->priv->background_tasks != NULL) && (browser->priv->progress_dialog != NULL))
		gtk_window_present (GTK_WINDOW (browser->priv->progress_dialog));
}
