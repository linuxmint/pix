/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#include "dlg-personalize-filters.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-auto-paned.h"
#include "gth-browser.h"
#include "gth-browser-actions-callbacks.h"
#include "gth-browser-actions-entries.h"
#include "gth-browser-ui.h"
#include "gth-duplicable.h"
#include "gth-embedded-dialog.h"
#include "gth-enum-types.h"
#include "gth-error.h"
#include "gth-file-list.h"
#include "gth-file-view.h"
#include "gth-file-selection.h"
#include "gth-filter.h"
#include "gth-filterbar.h"
#include "gth-folder-tree.h"
#include "gth-grid-view.h"
#include "gth-icon-cache.h"
#include "gth-info-bar.h"
#include "gth-image-preloader.h"
#include "gth-location-chooser.h"
#include "gth-main.h"
#include "gth-marshal.h"
#include "gth-menu-action.h"
#include "gth-metadata-provider.h"
#include "gth-paned.h"
#include "gth-preferences.h"
#include "gth-progress-dialog.h"
#include "gth-sidebar.h"
#include "gth-statusbar.h"
#include "gth-toggle-menu-tool-button.h"
#include "gth-user-dir.h"
#include "gth-viewer-page.h"
#include "gth-window.h"
#include "gth-window-actions-callbacks.h"
#include "gth-window-actions-entries.h"
#include "main.h"

#define GTH_BROWSER_CALLBACK(f) ((GthBrowserCallback) (f))
#define MAX_HISTORY_LENGTH 15
#define LOAD_FILE_DELAY 150
#define HIDE_MOUSE_DELAY 1000
#define MOTION_THRESHOLD 0
#define UPDATE_SELECTION_DELAY 200
#define MIN_SIDEBAR_SIZE 100
#define MIN_VIEWER_SIZE 256
#define STATUSBAR_SEPARATOR " · "
#define SHIRNK_WRAP_WIDTH_OFFSET 100
#define SHIRNK_WRAP_HEIGHT_OFFSET 125
#define FILE_PROPERTIES_MINIMUM_HEIGHT 100
#define HISTORY_FILE "history.xbel"

G_DEFINE_TYPE (GthBrowser, gth_browser, GTH_TYPE_WINDOW)


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

	GtkUIManager      *ui;
	GtkActionGroup    *actions;
	GtkWidget         *infobar;
	GtkWidget         *statusbar;
	GtkWidget         *browser_toolbar;
	GtkWidget         *browser_right_container;
	GtkWidget         *browser_left_container;
	GtkWidget         *browser_sidebar;
	GtkWidget         *folder_tree;
	GtkWidget         *history_list_popup_menu;
	GtkWidget         *folder_popup;
	GtkWidget         *file_list_popup;
	GtkWidget         *file_popup;
	GtkWidget         *filterbar;
	GtkWidget         *file_list;
	GtkWidget         *list_extra_widget_container;
	GtkWidget         *list_extra_widget;
	GtkWidget         *file_properties;

	GtkWidget         *thumbnail_list;

	GList             *viewer_pages;
	GtkOrientation     viewer_thumbnails_orientation;
	GtkWidget         *viewer_thumbnails_pane;
	GtkWidget         *viewer_sidebar_pane;
	GtkWidget         *viewer_sidebar_alignment;
	GtkWidget         *viewer_container;
	GtkWidget         *viewer_toolbar;
	GthViewerPage     *viewer_page;
	GthImagePreloader *image_preloader;

	GtkWidget         *progress_dialog;

	GHashTable        *named_dialogs;
	GList             *toolbar_menu_buttons[GTH_BROWSER_N_PAGES];

	guint              browser_ui_merge_id;
	guint              viewer_ui_merge_id;

	/* Browser data */

	guint              help_message_cid;
	gulong             folder_changed_id;
	gulong             file_renamed_id;
	gulong             metadata_changed_id;
	gulong             emblems_changed_id;
	gulong             entry_points_changed_id;
	gulong             order_changed_id;
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
	gulong             task_completed;
	gulong             task_progress;
	GList             *background_tasks;
	GList             *load_data_queue;
	GList             *load_file_data_queue;
	guint              load_file_timeout;
	char              *list_attributes;
	gboolean           constructed;
	guint              construct_step2_event;
	guint              selection_changed_event;
	GthFileData       *folder_popup_file_data;
	gboolean           properties_on_screen;
	char              *location_free_space;
	gboolean           recalc_location_free_space;
	gboolean           shrink_wrap_viewer;
	gboolean           file_properties_on_the_right;
	GthSidebarState    viewer_sidebar;
	BrowserState       state;

	/* settings */

	GSettings         *browser_settings;
	GSettings         *messages_settings;
	GSettings         *desktop_interface_settings;

	/* fulscreen */

	gboolean           fullscreen;
	GtkWidget         *fullscreen_toolbar;
	GList             *fullscreen_controls;
	guint              hide_mouse_timeout;
	guint              motion_signal;
	gdouble            last_mouse_x;
	gdouble            last_mouse_y;
	struct {
		int      page;
		gboolean viewer_properties;
		gboolean viewer_tools;
		gboolean thumbnail_list;
	} before_fullscreen;

	/* history */

	GList             *history;
	GList             *history_current;
	GtkWidget         *back_history_menu;
	GtkWidget         *forward_history_menu;
	GtkWidget         *go_parent_menu;
};


static guint gth_browser_signals[LAST_SIGNAL] = { 0 };


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


static void
_gth_browser_set_action_sensitive (GthBrowser  *browser,
				   const char  *action_name,
				   gboolean     sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (browser->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
_gth_browser_set_action_active (GthBrowser  *browser,
				const char  *action_name,
				gboolean     active)
{
	GtkAction *action;

	action = gtk_action_group_get_action (browser->priv->actions, action_name);
	g_object_set (action, "active", active, NULL);
}


static gboolean
_gth_browser_get_action_active (GthBrowser  *browser,
				const char  *action_name)
{
	GtkAction *action;
	gboolean   active;

	action = gtk_action_group_get_action (browser->priv->actions, action_name);
	g_object_get (action, "active", &active, NULL);

	return active;
}


static void
activate_go_back_menu_item (GtkMenuItem *menuitem,
			    gpointer     data)
{
	GthBrowser *browser = data;

	gth_browser_go_back (browser, GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "steps")));
}


static void
activate_go_forward_menu_item (GtkMenuItem *menuitem,
			       gpointer     data)
{
	GthBrowser *browser = data;

	gth_browser_go_forward (browser, GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "steps")));
}


static void
activate_go_up_menu_item (GtkMenuItem *menuitem,
			  gpointer     data)
{
	GthBrowser *browser = data;

	gth_browser_go_up (browser, GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "steps")));
}


static void
activate_go_to_menu_item (GtkMenuItem *menuitem,
			  gpointer     data)
{
	GthBrowser *browser = data;
	GFile      *location;

	location = g_file_new_for_uri (g_object_get_data (G_OBJECT (menuitem), "uri"));
	gth_browser_go_to (browser, location, NULL);

	g_object_unref (location);
}


void
_gth_browser_add_file_menu_item_full (GthBrowser *browser,
				      GtkWidget  *menu,
				      GFile      *file,
				      GIcon      *icon,
				      const char *display_name,
				      GthAction   action,
				      int         steps,
				      int         position)
{
	GdkPixbuf *pixbuf;
	GtkWidget *menu_item;

	pixbuf = gth_icon_cache_get_pixbuf (browser->priv->menu_icon_cache, icon);

	menu_item = gtk_image_menu_item_new_with_label (display_name);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menu_item), TRUE);
	if (pixbuf != NULL)
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), gtk_image_new_from_pixbuf (pixbuf));
	else
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
	gtk_widget_show (menu_item);
	if (position == -1)
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	else
		gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menu_item, position);

	if (action == GTH_ACTION_GO_TO) {
		char *parse_name;
		char *tooltip;

		parse_name = g_file_get_parse_name (file);
		tooltip = g_strdup_printf (_("Open %s"), parse_name);
		gtk_widget_set_tooltip_text (GTK_WIDGET (menu_item), tooltip);

		g_object_set_data_full (G_OBJECT (menu_item),
					"uri",
					g_file_get_uri (file),
					(GDestroyNotify) g_free);
		g_signal_connect (menu_item,
				  "activate",
				  G_CALLBACK (activate_go_to_menu_item),
			  	  browser);

		g_free (tooltip);
		g_free (parse_name);
	}
	else {
		g_object_set_data (G_OBJECT (menu_item),
				   "steps",
				   GINT_TO_POINTER (steps));
		if (action == GTH_ACTION_GO_BACK)
			g_signal_connect (menu_item,
					  "activate",
					  G_CALLBACK (activate_go_back_menu_item),
			  	  	  browser);
		else if (action == GTH_ACTION_GO_FORWARD)
			g_signal_connect (menu_item,
					  "activate",
					  G_CALLBACK (activate_go_forward_menu_item),
			  	  	  browser);
		else if (action == GTH_ACTION_GO_UP)
			g_signal_connect (menu_item,
					  "activate",
					  G_CALLBACK (activate_go_up_menu_item),
			  	  	  browser);
	}

	if (pixbuf != NULL)
		g_object_unref (pixbuf);
}


void
_gth_browser_add_file_menu_item (GthBrowser *browser,
				 GtkWidget  *menu,
			 	 GFile      *file,
			 	 const char *display_name,
			 	 GthAction   action,
				 int         steps)
{
	GthFileSource *file_source;
	GFileInfo     *info;

	file_source = gth_main_get_file_source (file);
	info = gth_file_source_get_file_info (file_source, file, GFILE_DISPLAY_ATTRIBUTES);
	if (info != NULL) {
		_gth_browser_add_file_menu_item_full (browser,
						      menu,
						      file,
						      g_file_info_get_icon (info),
						      (display_name != NULL) ? display_name : g_file_info_get_display_name (info),
						      action,
						      steps,
						      -1);
		g_object_unref (info);
	}
	else {
		GIcon *icon;
		char  *name;

		icon = _g_file_get_icon (file);
		name = _g_file_get_display_name (file);
		_gth_browser_add_file_menu_item_full (browser,
						      menu,
						      file,
						      icon,
						      (display_name != NULL) ? display_name : name,
						      action,
						      steps,
						      -1);

		g_free (name);
		g_object_unref (icon);
	}

	g_object_unref (file_source);
}


static void
_gth_browser_update_parent_list (GthBrowser *browser)
{
	GtkWidget *menu;
	int        i;
	GFile     *parent;

	menu = browser->priv->go_parent_menu;
	_gtk_container_remove_children (GTK_CONTAINER (menu), NULL, NULL);

	if (browser->priv->location == NULL)
		return;

	/* Update the parent list menu. */

	i = 0;
	parent = g_file_get_parent (browser->priv->location->file);
	while (parent != NULL) {
		GFile *parent_parent;

		_gth_browser_add_file_menu_item (browser,
						 menu,
						 parent,
						 NULL,
						 GTH_ACTION_GO_UP,
						 ++i);

		parent_parent = g_file_get_parent (parent);
		g_object_unref (parent);
		parent = parent_parent;
	}
}


void
gth_browser_update_title (GthBrowser *browser)
{
	GString      *title;
	const char   *name = NULL;
	GthFileStore *file_store;

	title = g_string_new (NULL);

	if (browser->priv->current_file != NULL)
		name = g_file_info_get_display_name (browser->priv->current_file->info);
	else if (browser->priv->location != NULL)
		name = g_file_info_get_display_name (browser->priv->location->info);

	if (name != NULL) {
		g_string_append (title, name);
		if (gth_browser_get_file_modified (browser)) {
			g_string_append (title, " ");
			g_string_append (title, _("[modified]"));
		}
	}

	file_store = gth_browser_get_file_store (browser);
	browser->priv->n_visibles = gth_file_store_n_visibles (file_store);
	browser->priv->current_file_position = -1;

	if (browser->priv->current_file != NULL) {
		int pos;

		pos = gth_file_store_get_pos (file_store, browser->priv->current_file->file);
		if (pos >= 0) {
			browser->priv->current_file_position = pos;
			g_string_append_printf (title, " - %d/%d", browser->priv->current_file_position + 1, browser->priv->n_visibles);
		}
	}

	if (title->len > 0)
		g_string_append (title, " - ");
	g_string_append (title, _("gThumb"));

	gtk_window_set_title (GTK_WINDOW (browser), title->str);

	g_string_free (title, TRUE);
}


void
gth_browser_update_sensitivity (GthBrowser *browser)
{
	GFile    *parent;
	gboolean  parent_available;
	gboolean  viewer_can_save;
	gboolean  modified;
	int       current_file_pos;
	int       n_files;
	int       n_selected;

	if (browser->priv->location != NULL)
		parent = g_file_get_parent (browser->priv->location->file);
	else
		parent = NULL;
	parent_available = (parent != NULL);
	_g_object_unref (parent);

	viewer_can_save = (browser->priv->location != NULL) && (browser->priv->viewer_page != NULL) && gth_viewer_page_can_save (GTH_VIEWER_PAGE (browser->priv->viewer_page));
	modified = gth_browser_get_file_modified (browser);

	if (browser->priv->current_file != NULL)
		current_file_pos = gth_file_store_get_pos (gth_browser_get_file_store (browser), browser->priv->current_file->file);
	else
		current_file_pos = -1;
	n_files = gth_file_store_n_visibles (gth_browser_get_file_store (browser));
	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	_gth_browser_set_action_sensitive (browser, "File_Open", n_selected == 1);
	_gth_browser_set_action_sensitive (browser, "File_Save", viewer_can_save && modified);
	_gth_browser_set_action_sensitive (browser, "File_SaveAs", viewer_can_save);
	_gth_browser_set_action_sensitive (browser, "File_Revert", viewer_can_save && modified);
	_gth_browser_set_action_sensitive (browser, "Go_Up", parent_available);
	_gth_browser_set_action_sensitive (browser, "Toolbar_Go_Up", parent_available);
	_gth_browser_set_action_sensitive (browser, "View_Stop", browser->priv->fullscreen || (browser->priv->activity_ref > 0));
	_gth_browser_set_action_sensitive (browser, "View_Prev", current_file_pos > 0);
	_gth_browser_set_action_sensitive (browser, "View_Next", (current_file_pos != -1) && (current_file_pos < n_files - 1));
	_gth_browser_set_action_sensitive (browser, "View_Thumbnail_List", gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER);
	_gth_browser_set_action_sensitive (browser, "View_Sidebar", gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER);
	_gth_browser_set_action_sensitive (browser, "View_Reload", gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER);

	gth_sidebar_update_sensitivity (GTH_SIDEBAR (browser->priv->file_properties));

	if (browser->priv->viewer_page != NULL)
		gth_viewer_page_update_sensitivity (browser->priv->viewer_page);

	gth_hook_invoke ("gth-browser-update-sensitivity", browser);
}


void
gth_browser_update_extra_widget (GthBrowser *browser)
{
	gedit_message_area_clear_action_area (GEDIT_MESSAGE_AREA (browser->priv->list_extra_widget));
	gth_embedded_dialog_set_from_file (GTH_EMBEDDED_DIALOG (browser->priv->list_extra_widget), browser->priv->location->file);
	gth_hook_invoke ("gth-browser-update-extra-widget", browser);
}


static void
_gth_browser_set_location (GthBrowser  *browser,
			   GthFileData *location)
{
	GtkWidget *location_chooser;

	if (location == NULL)
		return;

	if (browser->priv->location != NULL)
		g_object_unref (browser->priv->location);
	browser->priv->location = gth_file_data_dup (location);

	gth_browser_update_title (browser);
	_gth_browser_update_parent_list (browser);
	gth_browser_update_sensitivity (browser);

	location_chooser = gth_embedded_dialog_get_chooser (GTH_EMBEDDED_DIALOG (browser->priv->list_extra_widget));
	g_signal_handlers_block_by_data (location_chooser, browser);
	gth_browser_update_extra_widget (browser);
	g_signal_handlers_unblock_by_data (location_chooser, browser);
}


static void
_gth_browser_set_location_from_file (GthBrowser *browser,
				     GFile      *file)
{
	GthFileSource *file_source;
	GthFileData   *file_data;
	GFileInfo     *info;

	file_source = gth_main_get_file_source (file);
	info = gth_file_source_get_file_info (file_source, file, GFILE_DISPLAY_ATTRIBUTES);
	file_data = gth_file_data_new (file, info);
	_gth_browser_set_location (browser, file_data);

	g_object_unref (file_data);
	_g_object_unref (info);
	g_object_unref (file_source);
}


static void
_gth_browser_update_go_sensitivity (GthBrowser *browser)
{
	gboolean  sensitive;

	sensitive = (browser->priv->history_current != NULL) && (browser->priv->history_current->next != NULL);
	_gth_browser_set_action_sensitive (browser, "Go_Back", sensitive);
	_gth_browser_set_action_sensitive (browser, "Toolbar_Go_Back", sensitive);

	sensitive = (browser->priv->history_current != NULL) && (browser->priv->history_current->prev != NULL);
	_gth_browser_set_action_sensitive (browser, "Go_Forward", sensitive);
	_gth_browser_set_action_sensitive (browser, "Toolbar_Go_Forward", sensitive);
}


static void
activate_clear_history_menu_item (GtkMenuItem *menuitem,
				  gpointer     data)
{
	gth_browser_clear_history ((GthBrowser *)data);
}


static void
_gth_browser_add_clear_history_menu_item (GthBrowser *browser,
					  GtkWidget  *menu)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_image_menu_item_new_with_mnemonic (_("_Delete History"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (activate_clear_history_menu_item),
		  	  browser);
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
	GtkWidget *menu;
	GList     *scan;
	GtkWidget *separator;

	_gth_browser_update_go_sensitivity (browser);

	/* Update the back history menu. */

	menu = browser->priv->back_history_menu;
	_gtk_container_remove_children (GTK_CONTAINER (menu), NULL, NULL);

	if ((browser->priv->history != NULL)
	    && (browser->priv->history_current->next != NULL))
	{
		int i;

		for (i = 0, scan = browser->priv->history_current->next;
		     scan && (i < MAX_HISTORY_LENGTH);
		     scan = scan->next)
		{
			_gth_browser_add_file_menu_item (browser,
							 menu,
							 scan->data,
							 NULL,
							 GTH_ACTION_GO_BACK,
							 ++i);
		}
		if (i > 0)
			_gth_browser_add_clear_history_menu_item (browser, menu);
	}

	/* Update the forward history menu. */

	menu = browser->priv->forward_history_menu;
	_gtk_container_remove_children (GTK_CONTAINER (menu), NULL, NULL);

	if ((browser->priv->history != NULL)
	    && (browser->priv->history_current->prev != NULL))
	{
		int i;

		for (i = 0, scan = browser->priv->history_current->prev;
		     scan && (i < MAX_HISTORY_LENGTH);
		     scan = scan->prev)
		{
			_gth_browser_add_file_menu_item (browser,
							 menu,
							 scan->data,
							 NULL,
							 GTH_ACTION_GO_FORWARD,
							 ++i);
		}
		if (i > 0)
			_gth_browser_add_clear_history_menu_item (browser, menu);
	}

	/* Update the history list in the go menu */

	separator = gtk_ui_manager_get_widget (browser->priv->ui, "/MenuBar/Go/HistoryList");
	menu = gtk_widget_get_parent (separator);

	_gtk_container_remove_children (GTK_CONTAINER (menu), separator, NULL);

	if (browser->priv->history != NULL) {
		int i;

		for (i = 0, scan = browser->priv->history;
		     scan && (i < MAX_HISTORY_LENGTH);
		     scan = scan->next)
		{
			_gth_browser_add_file_menu_item (browser,
							 menu,
							 scan->data,
							 NULL,
							 GTH_ACTION_GO_TO,
							 ++i);
		}
	}

	separator = gtk_ui_manager_get_widget (browser->priv->ui, "/MenuBar/Go/BeforeHistoryList");
	gtk_widget_show (separator);
}


static void
_gth_browser_history_add (GthBrowser *browser,
			  GFile      *file)
{
	if (file == NULL)
		return;

	if ((browser->priv->history_current == NULL) || ! _g_file_equal_uris (file, browser->priv->history_current->data)) {
		GList *scan;

		/* remove all files after the current position */
		for (scan = browser->priv->history; scan && (scan != browser->priv->history_current); /* void */) {
			GList *next = scan->next;

			browser->priv->history = g_list_remove_link (browser->priv->history, scan);
			_g_object_list_unref (scan);

			scan = next;
		}

		/* remove all the occurrences of 'file' from the history */
		for (scan = browser->priv->history; scan; /* void */) {
			GList *next = scan->next;
			GFile *file_in_history = scan->data;

			if (_g_file_equal_uris (file, file_in_history)) {
				browser->priv->history = g_list_remove_link (browser->priv->history, scan);
				_g_object_list_unref (scan);
			}

			scan = next;
		}

		browser->priv->history = g_list_prepend (browser->priv->history, g_object_ref (file));
		browser->priv->history_current = browser->priv->history;
	}
}


static void
_gth_browser_history_save (GthBrowser *browser)
{
	GBookmarkFile *bookmarks;
	GFile         *file;
	char          *filename;
	GList         *scan;
	int            n;

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
	GBookmarkFile *bookmarks;
	GFile         *file;
	char          *filename;

	_g_object_list_unref (browser->priv->history);
	browser->priv->history = NULL;

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
	GtkWidget *separator1;
	GtkWidget *separator2;
	GtkWidget *menu;
	GList     *entry_points;
	GList     *scan;
	int        position;
	GFile     *root;

	separator1 = gtk_ui_manager_get_widget (browser->priv->ui, "/MenuBar/Go/BeforeEntryPointList");
	separator2 = gtk_ui_manager_get_widget (browser->priv->ui, "/MenuBar/Go/EntryPointList");
	menu = gtk_widget_get_parent (separator1);
	_gtk_container_remove_children (GTK_CONTAINER (menu), separator1, separator2);

	separator1 = separator2;
	separator2 = gtk_ui_manager_get_widget (browser->priv->ui, "/MenuBar/Go/EntryPointListSeparator");
	_gtk_container_remove_children (GTK_CONTAINER (menu), separator1, separator2);

	position = 6;
	entry_points = gth_main_get_all_entry_points ();
	for (scan = entry_points; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		g_file_info_set_attribute_boolean (file_data->info, "gthumb::entry-point", TRUE);
		g_file_info_set_sort_order (file_data->info, position);
		_gth_browser_add_file_menu_item_full (browser,
						      menu,
						      file_data->file,
						      g_file_info_get_icon (file_data->info),
						      g_file_info_get_display_name (file_data->info),
						      GTH_ACTION_GO_TO,
						      0,
						      position++);
	}
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
	GFile         *requested_folder_parent;
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
	load_data->requested_folder_parent = g_file_get_parent (load_data->requested_folder->file);
	if (file_to_select != NULL)
		load_data->file_to_select = g_file_dup (file_to_select);
	else if (browser->priv->current_file != NULL)
		load_data->file_to_select = g_file_dup (browser->priv->current_file->file);
	load_data->selected = g_list_copy_deep (selected, (GCopyFunc) gtk_tree_path_copy, NULL);
	load_data->vscroll = vscroll;
	load_data->action = action;
	load_data->automatic = automatic;
	load_data->cancellable = g_cancellable_new ();

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

	browser->priv->load_data_queue = g_list_prepend (browser->priv->load_data_queue, load_data);

	return load_data;
}


static void
load_data_free (LoadData *data)
{
	data->browser->priv->load_data_queue = g_list_remove (data->browser->priv->load_data_queue, data);

	g_object_unref (data->requested_folder);
	_g_object_unref (data->requested_folder_parent);
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


static char *
file_format (const char *format,
	     GFile      *file)
{
	char *name;
	char *s;

	name = g_file_get_parse_name (file);
	s = g_strdup_printf (format, name);

	g_free (name);

	return s;
}


static void
_gth_browser_show_error (GthBrowser *browser,
			 const char *title,
			 GError     *error)
{
	/* _gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), title, error); */

	gth_info_bar_set_primary_text (GTH_INFO_BAR (browser->priv->infobar), title);
	gth_info_bar_set_secondary_text (GTH_INFO_BAR (browser->priv->infobar), error->message);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (gtk_info_bar_get_action_area (GTK_INFO_BAR (browser->priv->infobar))), GTK_ORIENTATION_HORIZONTAL);
	gth_info_bar_set_icon (GTH_INFO_BAR (browser->priv->infobar), GTK_STOCK_DIALOG_ERROR);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (browser->priv->infobar), GTK_MESSAGE_ERROR);
	_gtk_info_bar_clear_action_area (GTK_INFO_BAR (browser->priv->infobar));
	gtk_info_bar_add_buttons (GTK_INFO_BAR (browser->priv->infobar),
				  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				  NULL);
	gtk_widget_show (browser->priv->infobar);
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

	/* moving the "gth-browser-load-location-after" after the
	 * LOCATION_READY signal emition can brake the extensions */

	if ((load_data->action == GTH_ACTION_GO_TO)
	    || (load_data->action == GTH_ACTION_GO_BACK)
	    || (load_data->action == GTH_ACTION_GO_FORWARD)
	    || (load_data->action == GTH_ACTION_GO_UP)
	    || (load_data->action == GTH_ACTION_VIEW))
	{
		if (error == NULL) {
			_g_object_unref (browser->priv->location_source);
			browser->priv->location_source = g_object_ref (load_data->file_source);
		}
		gth_browser_update_extra_widget (browser);
		gth_hook_invoke ("gth-browser-load-location-after", browser, browser->priv->location, error);
	}

	browser->priv->activity_ref--;
	g_signal_emit (G_OBJECT (browser),
		       gth_browser_signals[LOCATION_READY],
		       0,
		       load_data->requested_folder->file,
		       (error != NULL));

	if (error == NULL)
		return;

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_error_free (error);
		return;
	}

	if (load_data->automatic) {
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
	title = file_format (_("Could not load the position \"%s\""), load_data->requested_folder->file);
	_gth_browser_show_error (browser, title, error);

	g_free (title);
	g_error_free (error);
}


static void
load_data_error (LoadData *load_data,
		 GError   *error)
{
	GthBrowser *browser = load_data->browser;
	GFile      *loaded_folder;

	loaded_folder = (GFile *) load_data->current->data;
	gth_folder_tree_set_children (GTH_FOLDER_TREE (browser->priv->folder_tree), loaded_folder, NULL);

	switch (load_data->action) {
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
	case GTH_ACTION_GO_UP:
	case GTH_ACTION_VIEW:
		gth_file_list_set_files (GTH_FILE_LIST (browser->priv->file_list), NULL);
		gth_file_list_set_files (GTH_FILE_LIST (browser->priv->thumbnail_list), NULL);
		break;

	default:
		break;
	}

	load_data_done (load_data, error);
}


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


static void _gth_browser_load_ready_cb (GthFileSource *file_source, GList *files, GError *error, gpointer user_data);


/* -- _gth_browser_set_sort_order -- */


static gboolean
_gth_browser_reload_required (GthBrowser *browser)
{
	char        *old_attributes;
	const char  *new_attributes;
	gboolean     reload_required;

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
			     gboolean         save)
{
	g_return_if_fail (sort_type != NULL);

	browser->priv->current_sort_type = sort_type;
	browser->priv->current_sort_inverse = inverse;

	gth_file_list_set_sort_func (GTH_FILE_LIST (browser->priv->file_list),
				     sort_type->cmp_func,
				     inverse);
	gth_file_list_set_sort_func (GTH_FILE_LIST (browser->priv->thumbnail_list),
				     sort_type->cmp_func,
				     inverse);
	gth_browser_update_title (browser);

	if (! browser->priv->constructed || (browser->priv->location == NULL))
		return;

	g_file_info_set_attribute_string (browser->priv->location->info, "sort::type", (sort_type != NULL) ? sort_type->name : "general::unsorted");
	g_file_info_set_attribute_boolean (browser->priv->location->info, "sort::inverse", (sort_type != NULL) ? inverse : FALSE);

	if (! save || (browser->priv->location_source == NULL))
		return;

	gth_file_source_write_metadata (browser->priv->location_source,
					browser->priv->location,
					"sort::type,sort::inverse",
					write_sort_order_ready_cb,
					browser);
}


static void
requested_folder_attributes_ready_cb (GObject  *file_source,
				      GError   *error,
				      gpointer  user_data)
{
	LoadData   *load_data = user_data;
	GthBrowser *browser = load_data->browser;

	if (error != NULL) {
		load_data_error (load_data, error);
		load_data_free (load_data);
		return;
	}

	gth_file_data_set_info (browser->priv->location, load_data->requested_folder->info);

	browser->priv->current_sort_type = gth_main_get_sort_type (g_file_info_get_attribute_string (browser->priv->location->info, "sort::type"));
	browser->priv->current_sort_inverse = g_file_info_get_attribute_boolean (browser->priv->location->info, "sort::inverse");
	if (browser->priv->current_sort_type == NULL) {
		browser->priv->current_sort_type = browser->priv->default_sort_type;
		browser->priv->current_sort_inverse = browser->priv->default_sort_inverse;
		g_file_info_set_attribute_string (browser->priv->location->info, "sort::type", browser->priv->current_sort_type->name);
		g_file_info_set_attribute_boolean (browser->priv->location->info, "sort::inverse", browser->priv->current_sort_inverse);
	}
	_gth_browser_set_sort_order (browser, browser->priv->current_sort_type, browser->priv->current_sort_inverse, FALSE);

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

		if ((load_data->requested_folder_parent != NULL) && g_file_equal (folder_to_load, load_data->requested_folder_parent)) {
			path = gth_folder_tree_get_path (folder_tree, folder_to_load);
			if (path == NULL)
				break;

			if (! gth_folder_tree_is_loaded (folder_tree, path)) {
				gtk_tree_path_free (path);
				break;
			}
		}

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

	if ((load_data->action != GTH_ACTION_LIST_CHILDREN) && g_file_equal (folder_to_load, load_data->requested_folder->file))
		gth_file_source_read_metadata (load_data->file_source,
					       load_data->requested_folder,
					       "*",
					       requested_folder_attributes_ready_cb,
     					       load_data);
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

	if ((load_data->action != GTH_ACTION_LIST_CHILDREN)
	    && ! g_file_equal (load_data->requested_folder->file, load_data->browser->priv->location->file))
	{
		load_data_done (load_data, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
		load_data_free (load_data);
		return;
	}

	loaded_folder = (GFile *) load_data->current->data;
	files = _gth_browser_get_visible_files (browser, loaded_files);
	gth_folder_tree_set_children (GTH_FOLDER_TREE (browser->priv->folder_tree), loaded_folder, files);

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (browser->priv->folder_tree), loaded_folder);
	loaded_requested_folder = g_file_equal (loaded_folder, load_data->requested_folder->file);
	if ((path != NULL) && ! loaded_requested_folder)
		gth_folder_tree_expand_row (GTH_FOLDER_TREE (browser->priv->folder_tree), path, FALSE);

	if (! loaded_requested_folder) {
		gtk_tree_path_free (path);
		_g_object_list_unref (files);
		load_data_load_next_folder (load_data);
		return;
	}

	load_data_done (load_data, NULL);

	switch (load_data->action) {
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
	case GTH_ACTION_GO_UP:
	case GTH_ACTION_VIEW:
		if (path != NULL) {
			GList    *entry_points;
			GList    *scan;
			gboolean  is_entry_point = FALSE;

			/* expand the path if it's an entry point */

			entry_points = gth_main_get_all_entry_points ();
			for (scan = entry_points; scan; scan = scan->next) {
				GthFileData *file_data = scan->data;

				if (g_file_equal (file_data->file, load_data->requested_folder->file)) {
					gth_folder_tree_collapse_all (GTH_FOLDER_TREE (browser->priv->folder_tree));
					gtk_tree_view_expand_row (GTK_TREE_VIEW (browser->priv->folder_tree), path, FALSE);
					is_entry_point = TRUE;
					break;
				}
			}

			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (browser->priv->folder_tree),
						      path,
						      NULL,
						      is_entry_point,
						      0.0,
						      0.0);
			gth_folder_tree_select_path (GTH_FOLDER_TREE (browser->priv->folder_tree), path);

			_g_object_list_unref (entry_points);
		}
		break;

	default:
		break;
	}

	changed_current_location = FALSE;
	switch (load_data->action) {
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
	case GTH_ACTION_GO_UP:
	case GTH_ACTION_VIEW:
		changed_current_location = TRUE;
		browser->priv->recalc_location_free_space = TRUE;
		break;
	default:
		break;
	}

	if (changed_current_location) {
		GthTest *filter;

		filter = _gth_browser_get_file_filter (browser);
		gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->file_list), filter);
		gth_file_list_set_files (GTH_FILE_LIST (browser->priv->file_list), files);
		gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->thumbnail_list), filter);
		gth_file_list_set_files (GTH_FILE_LIST (browser->priv->thumbnail_list), files);
		g_object_unref (filter);

		if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER)
			gtk_widget_grab_focus (browser->priv->file_list);

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
	load_data_free (load_data);
	_g_object_list_unref (files);
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
		load_data_free (load_data);
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
		load_data_free (load_data);
	}
	else if ((load_data->action != GTH_ACTION_LIST_CHILDREN)
		 && g_file_equal ((GFile *) load_data->current->data, load_data->requested_folder->file))
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


static GFile *
get_nearest_entry_point (GFile *file)
{
	GList *list;
	GList *scan;
	GList *entries;
	char  *nearest_uri;
	char  *uri;
	int    file_uri_len;
	int    min_diff;
	GFile *nearest;

	entries = NULL;
	list = gth_main_get_all_entry_points ();
	for (scan = list; scan; scan = scan->next) {
		GthFileData *entry_point = scan->data;

		if (g_file_equal (file, entry_point->file) || _g_file_has_prefix (file, entry_point->file))
			entries = g_list_prepend (entries, g_file_get_uri (entry_point->file));
	}

	nearest_uri = NULL;
	uri = g_file_get_uri (file);
	file_uri_len = strlen (uri);
	min_diff = 0;
	for (scan = entries; scan; scan = scan->next) {
		char *entry_uri = scan->data;
		int   entry_len;
		int   diff;

		entry_len = strlen (entry_uri);
		diff = abs (entry_len - file_uri_len);
		if ((scan == entries) || (diff < min_diff)) {
			min_diff = diff;
			nearest_uri = entry_uri;
		}
	}
	g_free (uri);

	nearest = NULL;
	if (nearest_uri != NULL)
		nearest = g_file_new_for_uri (nearest_uri);

	_g_string_list_free (entries);
	_g_object_list_unref (list);

	return nearest;
}


static void
mount_volume_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	if (! g_file_mount_enclosing_volume_finish (G_FILE (source_object), result, &error)) {
		char *title;

		title = file_format (_("Could not load the position \"%s\""), load_data->requested_folder->file);
		_gth_browser_show_error (load_data->browser, title, error);
		g_clear_error (&error);

		g_free (title);
		load_data_free (load_data);
		return;
	}

	/* try to load again */

	gth_monitor_entry_points_changed (gth_main_get_default_monitor());

	_gth_browser_update_entry_point_list (load_data->browser);
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
	LoadData *load_data;
	GFile    *entry_point;

	if (! automatic)
		_gth_browser_hide_infobar (browser);

	switch (action) {
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
	case GTH_ACTION_GO_UP:
	case GTH_ACTION_VIEW:
		if ((browser->priv->location_source != NULL) && (browser->priv->monitor_location != NULL)) {
			gth_file_source_monitor_directory (browser->priv->location_source,
							   browser->priv->monitor_location,
							   FALSE);
			_g_clear_object (&browser->priv->monitor_location);
		}
		break;
	default:
		break;
	}

	entry_point = get_nearest_entry_point (location);
	load_data = load_data_new (browser,
				   location,
				   file_to_select,
				   selected,
				   vscroll,
				   action,
				   automatic,
				   entry_point);

	if (entry_point == NULL) {
		GMountOperation *mount_op;

		/* try to mount the enclosing volume */

		mount_op = gtk_mount_operation_new (GTK_WINDOW (browser));
		g_file_mount_enclosing_volume (location, 0, mount_op, NULL, mount_volume_ready_cb, load_data);

		g_object_unref (mount_op);

		return;
	}

	if ((load_data->action == GTH_ACTION_GO_TO)
	    || (load_data->action == GTH_ACTION_GO_BACK)
	    || (load_data->action == GTH_ACTION_GO_FORWARD)
	    || (load_data->action == GTH_ACTION_GO_UP)
	    || (load_data->action == GTH_ACTION_VIEW))
	{
		gth_hook_invoke ("gth-browser-load-location-before", browser, load_data->requested_folder->file);
	}
	browser->priv->activity_ref++;

	if (entry_point == NULL) {
		GError *error;
		char   *uri;

		uri = g_file_get_uri (location);
		error = g_error_new (GTH_ERROR, 0, _("No suitable module found for %s"), uri);
		load_data_ready (load_data, NULL, error);

		g_free (uri);

		return;
	}

	switch (action) {
	case GTH_ACTION_LIST_CHILDREN:
		gth_folder_tree_loading_children (GTH_FOLDER_TREE (browser->priv->folder_tree), location);
		break;
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_VIEW:
		gth_file_list_clear (GTH_FILE_LIST (browser->priv->file_list), _("Getting folder listing..."));
		break;
	default:
		break;
	}

	if (load_data->file_source == NULL) {
		GError *error;
		char   *uri;

		uri = g_file_get_uri (load_data->requested_folder->file);
		error = g_error_new (GTH_ERROR, 0, _("No suitable module found for %s"), uri);
		load_data_ready (load_data, NULL, error);

		g_free (uri);

		return;
	}

	switch (load_data->action) {
	case GTH_ACTION_GO_TO:
	case GTH_ACTION_VIEW:
		_gth_browser_set_location_from_file (browser, load_data->requested_folder->file);
		_gth_browser_history_add (browser, browser->priv->location->file);
		_gth_browser_history_menu (browser);
		break;
	case GTH_ACTION_GO_BACK:
	case GTH_ACTION_GO_FORWARD:
		_gth_browser_set_location_from_file (browser, load_data->requested_folder->file);
		_gth_browser_history_menu (browser);
		break;
	default:
		break;
	}

	{
		char *uri;

		uri = g_file_get_uri (load_data->requested_folder->file);

		debug (DEBUG_INFO, "LOAD: %s\n", uri);
		performance (DEBUG_INFO, "loading %s", uri);

		g_free (uri);
	}

	gth_browser_update_sensitivity (browser);
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
	if (cancelled)
		g_file_info_set_attribute_boolean (data->browser->priv->current_file->info, "gth::file::is-modified", TRUE);
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

	if (response_id == RESPONSE_SAVE)
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

	title = g_strdup_printf (_("Save changes to file '%s'?"), g_file_info_get_display_name (browser->priv->current_file->info));
	d = _gtk_message_dialog_new (GTK_WINDOW (browser),
				     GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_QUESTION,
				     title,
				     _("If you don't save, changes to the file will be permanently lost."),
				     _("Do _Not Save"), RESPONSE_NO_SAVE,
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     GTK_STOCK_SAVE, RESPONSE_SAVE,
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
_gth_browser_deactivate_viewer_page (GthBrowser *browser)
{
	if (browser->priv->viewer_page != NULL) {
		if (browser->priv->fullscreen)
			gth_viewer_page_show_pointer (GTH_VIEWER_PAGE (browser->priv->viewer_page), TRUE);
		gth_viewer_page_deactivate (browser->priv->viewer_page);
		gtk_ui_manager_ensure_update (browser->priv->ui);
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
		GdkWindowState state;
		gboolean       maximized;
		GtkAllocation  allocation;

		/* Save visualization options only if the window is not maximized. */

		state = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (browser)));
		maximized = (state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
		if (! maximized && gtk_widget_get_visible (GTK_WIDGET (browser))) {
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

		gtk_widget_get_allocation (browser->priv->viewer_sidebar_alignment, &allocation);
		if (allocation.width > MIN_SIDEBAR_SIZE)
			g_settings_set_int (browser->priv->browser_settings, PREF_BROWSER_VIEWER_SIDEBAR_WIDTH, allocation.width);

		switch (gth_file_list_get_mode (GTH_FILE_LIST (browser->priv->thumbnail_list))) {
		case GTH_FILE_LIST_MODE_H_SIDEBAR:
			g_settings_set_int (browser->priv->browser_settings,
					    PREF_BROWSER_THUMBNAIL_LIST_SIZE,
					    _gtk_paned_get_position2 (GTK_PANED (browser->priv->viewer_thumbnails_pane)));
			break;
		case GTH_FILE_LIST_MODE_V_SIDEBAR:
			g_settings_set_int (browser->priv->browser_settings,
					    PREF_BROWSER_THUMBNAIL_LIST_SIZE,
					    gtk_paned_get_position (GTK_PANED (browser->priv->viewer_thumbnails_pane)));
			break;
		default:
			g_warning ("Wrong thumbnail list mode");
			break;
		}

		g_settings_set_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_SIDEBAR, browser->priv->viewer_sidebar);
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

	if (browser->priv->folder_popup != NULL)
		gtk_widget_destroy (browser->priv->folder_popup);
	if (browser->priv->file_list_popup != NULL)
		gtk_widget_destroy (browser->priv->file_list_popup);
	if (browser->priv->file_popup != NULL)
		gtk_widget_destroy (browser->priv->file_popup);

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

	g_signal_handlers_disconnect_by_data (browser->priv->browser_settings, browser);
	g_signal_handlers_disconnect_by_data (browser->priv->messages_settings, browser);
	g_signal_handlers_disconnect_by_data (browser->priv->desktop_interface_settings, browser);

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

	/* remove timeouts */

	if (browser->priv->construct_step2_event != 0) {
		g_source_remove (browser->priv->construct_step2_event);
		browser->priv->construct_step2_event = 0;
	}

	if (browser->priv->selection_changed_event != 0) {
		g_source_remove (browser->priv->selection_changed_event);
		browser->priv->selection_changed_event = 0;
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


static void
_gth_browser_update_viewer_ui (GthBrowser *browser,
			       int         page)
{
	if (page == GTH_BROWSER_PAGE_VIEWER) {
		GError *error = NULL;

		if (browser->priv->viewer_ui_merge_id != 0)
			return;
		browser->priv->viewer_ui_merge_id = gtk_ui_manager_add_ui_from_string (browser->priv->ui, viewer_ui_info, -1, &error);
		if (browser->priv->viewer_ui_merge_id == 0) {
			g_warning ("ui building failed: %s", error->message);
			g_clear_error (&error);
		}
		gtk_ui_manager_ensure_update (gth_browser_get_ui_manager (browser));
	}
	else if (browser->priv->viewer_ui_merge_id != 0) {
		gtk_ui_manager_remove_ui (browser->priv->ui, browser->priv->viewer_ui_merge_id);
		browser->priv->viewer_ui_merge_id = 0;
		gtk_ui_manager_ensure_update (gth_browser_get_ui_manager (browser));
	}

	if (browser->priv->viewer_page != NULL) {
		if (page == GTH_BROWSER_PAGE_VIEWER)
			gth_viewer_page_show (browser->priv->viewer_page);
		else
			gth_viewer_page_hide (browser->priv->viewer_page);
	}
}


static void
_gth_browser_update_browser_ui (GthBrowser *browser,
				int         page)
{
	if (page == GTH_BROWSER_PAGE_BROWSER) {
		GError *error = NULL;

		if (browser->priv->browser_ui_merge_id != 0)
			return;
		browser->priv->browser_ui_merge_id = gtk_ui_manager_add_ui_from_string (browser->priv->ui, browser_ui_info, -1, &error);
		if (browser->priv->browser_ui_merge_id == 0) {
			g_warning ("ui building failed: %s", error->message);
			g_clear_error (&error);
		}
		gtk_ui_manager_ensure_update (gth_browser_get_ui_manager (browser));
	}
	else if (browser->priv->browser_ui_merge_id != 0) {
		gtk_ui_manager_remove_ui (browser->priv->ui, browser->priv->browser_ui_merge_id);
		browser->priv->browser_ui_merge_id = 0;
		gtk_ui_manager_ensure_update (gth_browser_get_ui_manager (browser));
	}
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

	_gth_browser_update_viewer_ui (browser, page);
	_gth_browser_update_browser_ui (browser, page);
	_gth_browser_hide_infobar (browser);

	/* move the sidebar from the browser to the viewer and vice-versa */

	gtk_widget_unrealize (browser->priv->file_properties);
	if (page == GTH_BROWSER_PAGE_BROWSER) {
		GtkWidget *file_properties_parent;

		file_properties_parent = _gth_browser_get_browser_file_properties_container (browser);
		gtk_widget_reparent (browser->priv->file_properties, file_properties_parent);
		/* restore the child properties that gtk_widget_reparent doesn't preserve. */
		gtk_container_child_set (GTK_CONTAINER (file_properties_parent),
					 browser->priv->file_properties,
					 "resize", ! browser->priv->file_properties_on_the_right,
					 "shrink", FALSE,
					 NULL);
	}
	else
		gtk_widget_reparent (browser->priv->file_properties, browser->priv->viewer_sidebar_alignment);

	/* update the sidebar state depending on the current visible page */

	if (page == GTH_BROWSER_PAGE_BROWSER) {
		gth_sidebar_show_properties (GTH_SIDEBAR (browser->priv->file_properties));
		if (_gth_browser_get_action_active (browser, "Browser_Properties"))
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

	if (prev_page == GTH_BROWSER_PAGE_BROWSER) {
		GdkWindow *gdk_win;

		gdk_win = gtk_widget_get_window (GTK_WIDGET (browser));
		if (gdk_win != NULL) {
			GdkWindowState state = gdk_window_get_state (gdk_win);

			if ((state & GDK_WINDOW_STATE_MAXIMIZED) == 0) { /* ! maximized */
				gtk_window_get_size (GTK_WINDOW (browser), &width, &height);
				gth_window_save_page_size (GTH_WINDOW (browser), prev_page, width, height);
			}
		}
	}

	/* restore the browser window size */

	if (page == GTH_BROWSER_PAGE_BROWSER)
		gth_window_apply_saved_size (GTH_WINDOW (window), page);

	/* set the focus */

	if (page == GTH_BROWSER_PAGE_BROWSER)
		gtk_widget_grab_focus (browser->priv->file_list);
	else if (page == GTH_BROWSER_PAGE_VIEWER)
		_gth_browser_make_file_visible (browser, browser->priv->current_file);

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
	g_free (browser->priv->list_attributes);
	_g_object_unref (browser->priv->folder_popup_file_data);
	_g_object_unref (browser->priv->back_history_menu);
	_g_object_unref (browser->priv->forward_history_menu);
	_g_object_unref (browser->priv->go_parent_menu);

	G_OBJECT_CLASS (gth_browser_parent_class)->finalize (object);
}


static void
gth_browser_class_init (GthBrowserClass *klass)
{
	GObjectClass   *gobject_class;
	GthWindowClass *window_class;

	g_type_class_add_private (klass, sizeof (GthBrowserPrivate));

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


static void
menu_item_select_cb (GtkMenuItem *proxy,
		     GthBrowser  *browser)
{
	GtkAction *action;
	char      *message;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message != NULL) {
		gtk_statusbar_push (GTK_STATUSBAR (browser->priv->statusbar),
				    browser->priv->help_message_cid,
				    message);
		g_free (message);
	}
}


static void
menu_item_deselect_cb (GtkMenuItem *proxy,
		       GthBrowser  *browser)
{
	gtk_statusbar_pop (GTK_STATUSBAR (browser->priv->statusbar),
			   browser->priv->help_message_cid);
}


static void
disconnect_proxy_cb (GtkUIManager *manager,
		     GtkAction    *action,
		     GtkWidget    *proxy,
		     GthBrowser   *browser)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), browser);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), browser);
	}
}


static void
connect_proxy_cb (GtkUIManager *manager,
		  GtkAction    *action,
		  GtkWidget    *proxy,
		  GthBrowser   *browser)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), browser);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), browser);
	}
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
		GdkDragAction action =
			_gtk_menu_ask_drag_drop_action (tree_view,
							gdk_drag_context_get_actions (context),
							time);
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
		gth_hook_invoke ("gth-browser-folder-tree-drag-data-received", browser, destination, file_list,
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

	if (gdk_drag_context_get_actions (drag_context) && GDK_ACTION_MOVE) {
		GdkDragAction action =
			gth_file_source_can_cut (file_source, file_data->file) ?
			GDK_ACTION_MOVE : GDK_ACTION_COPY;
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
	gth_browser_go_to (browser, file, NULL);
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
	_gth_browser_load (browser, file, NULL, NULL, 0, GTH_ACTION_LIST_CHILDREN, FALSE);
}


static void
folder_tree_load_cb (GthFolderTree *folder_tree,
		     GFile         *file,
		     GthBrowser    *browser)
{
	_gth_browser_load (browser, file, NULL, NULL, 0, GTH_ACTION_VIEW, FALSE);
}


static void
folder_tree_folder_popup_cb (GthFolderTree *folder_tree,
			     GthFileData   *file_data,
			     guint          time,
			     gpointer       user_data)
{
	GthBrowser    *browser = user_data;
	gboolean       sensitive;
	GthFileSource *file_source;

	sensitive = (file_data != NULL);
	_gth_browser_set_action_sensitive (browser, "Folder_Open", sensitive);
	_gth_browser_set_action_sensitive (browser, "Folder_OpenInNewWindow", sensitive);

	_g_object_unref (browser->priv->folder_popup_file_data);
	browser->priv->folder_popup_file_data = _g_object_ref (file_data);

	if (file_data != NULL)
		file_source = gth_main_get_file_source (file_data->file);
	else
		file_source = NULL;
	gth_hook_invoke ("gth-browser-folder-tree-popup-before", browser, file_source, file_data);
	gtk_ui_manager_ensure_update (browser->priv->ui);

	gtk_menu_popup (GTK_MENU (browser->priv->folder_popup),
			NULL,
			NULL,
			NULL,
			NULL,
			3,
			(guint32) time);

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
	gth_file_source_rename (file_source, file, new_name, file_source_rename_ready_cb, browser);
}


static void
filterbar_changed_cb (GthFilterbar *filterbar,
		      GthBrowser   *browser)
{
	GthTest *filter;

	filter = _gth_browser_get_file_filter (browser);
	gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->file_list), filter);
	gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->thumbnail_list), filter);
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
filterbar_close_button_clicked_cb (GthFilterbar *filterbar,
				   GthBrowser   *browser)
{
	gth_browser_show_filterbar (browser, FALSE);
}


static void
_gth_browser_change_file_list_order (GthBrowser *browser,
				     int        *new_order)
{
	g_file_info_set_attribute_string (browser->priv->location->info, "sort::type", "general::unsorted");
	g_file_info_set_attribute_boolean (browser->priv->location->info, "sort::inverse", FALSE);
	gth_file_store_reorder (gth_browser_get_file_store (browser), new_order);
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
							     FALSE);
			gth_file_list_add_files (GTH_FILE_LIST (browser->priv->file_list), files, monitor_data->position);
			gth_file_list_update_files (GTH_FILE_LIST (browser->priv->file_list), files);
			gth_file_list_add_files (GTH_FILE_LIST (browser->priv->thumbnail_list), files, monitor_data->position);
			gth_file_list_update_files (GTH_FILE_LIST (browser->priv->thumbnail_list), files);
		}
	}
	else if (monitor_data->event == GTH_MONITOR_EVENT_CHANGED) {
		if (monitor_data->update_folder_tree)
			gth_folder_tree_update_children (GTH_FOLDER_TREE (browser->priv->folder_tree), monitor_data->parent, visible_folders);
		if (monitor_data->update_file_list) {
			gth_file_list_update_files (GTH_FILE_LIST (browser->priv->file_list), files);
			gth_file_list_update_files (GTH_FILE_LIST (browser->priv->thumbnail_list), files);
		}
	}

	if (browser->priv->current_file != NULL) {
		GList *link;

		link = gth_file_data_list_find_file (files, browser->priv->current_file->file);
		if (link != NULL) {
			GthFileData *file_data = link->data;
			gth_browser_load_file (browser, file_data, FALSE);
		}
	}

	_gth_browser_update_statusbar_list_info (browser);
	gth_browser_update_title (browser);
	gth_browser_update_sensitivity (browser);

	_g_object_list_unref (visible_folders);
	monitor_event_data_unref (monitor_data);
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


static void _gth_browser_load_file (GthBrowser  *browser,
				    GthFileData *file_data,
				    gboolean     view);


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

	if ((event == GTH_MONITOR_EVENT_DELETED) && (_g_file_list_find_file_or_ancestor (list, browser->priv->location->file) != NULL))
		_gth_browser_load (browser, parent, NULL, NULL, 0, GTH_ACTION_GO_TO, TRUE);

	if ((event == GTH_MONITOR_EVENT_CHANGED) && (_g_file_list_find_file_or_ancestor (list, browser->priv->location->file) != NULL)) {
		_gth_browser_load (browser, browser->priv->location->file, NULL, NULL, 0, GTH_ACTION_GO_TO, TRUE);
		return;
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

	if ((event == GTH_MONITOR_EVENT_REMOVED) && g_file_equal (parent, browser->priv->location->file))
		event = GTH_MONITOR_EVENT_DELETED;

	if (update_folder_tree || update_file_list) {
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

		case GTH_MONITOR_EVENT_DELETED:
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
					g_signal_handlers_block_by_data (gth_browser_get_file_list_view (browser), browser);
				gth_file_list_delete_files (GTH_FILE_LIST (browser->priv->file_list), list);
				gth_file_list_delete_files (GTH_FILE_LIST (browser->priv->thumbnail_list), list);
				if (current_file_deleted)
					g_signal_handlers_unblock_by_data (gth_browser_get_file_list_view (browser), browser);
			}

			if (current_file_deleted) {
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
					_gth_browser_load_file (browser, NULL, FALSE);
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
	gth_file_list_rename_file (GTH_FILE_LIST (browser->priv->thumbnail_list), rename_data->file, file_data);

	if (g_file_equal (rename_data->file, browser->priv->location->file)) {
		GthFileData *new_location;
		GFileInfo   *new_info;

		new_location = gth_file_data_new (rename_data->new_file, browser->priv->location->info);
		new_info = gth_file_source_get_file_info (rename_data->file_source, new_location->file, GFILE_DISPLAY_ATTRIBUTES);
		g_file_info_copy_into (new_info, new_location->info);
		_gth_browser_set_location (browser, new_location);

		g_object_unref (new_info);
		g_object_unref (new_location);
	}
	else if ((browser->priv->current_file != NULL) && g_file_equal (rename_data->file, browser->priv->current_file->file))
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
	const char  *file_date;
	const char  *file_size;
	GthMetadata *metadata;
	GString     *status;

	if (browser->priv->current_file == NULL) {
		gth_statusbar_set_primary_text (GTH_STATUSBAR (browser->priv->statusbar), "");
		gth_statusbar_set_secondary_text (GTH_STATUSBAR (browser->priv->statusbar), "");
		return;
	}

	extra_info = g_file_info_get_attribute_string (browser->priv->current_file->info, "gthumb::statusbar-extra-info");
	image_size = g_file_info_get_attribute_string (browser->priv->current_file->info, "general::dimensions");
	metadata = (GthMetadata *) g_file_info_get_attribute_object (browser->priv->current_file->info, "general::datetime");
	if (metadata != NULL)
		file_date = gth_metadata_get_formatted (metadata);
	else
		file_date = g_file_info_get_attribute_string (browser->priv->current_file->info, "gth::file::display-mtime");
	file_size = g_file_info_get_attribute_string (browser->priv->current_file->info, "gth::file::display-size");

	status = g_string_new ("");

	if (extra_info != NULL)
		g_string_append (status, extra_info);

	if (image_size != NULL) {
		if (status->len > 0)
			g_string_append (status, STATUSBAR_SEPARATOR);
		g_string_append (status, image_size);
	}

	if (gth_browser_get_file_modified (browser)) {
		if (status->len > 0)
			g_string_append (status, STATUSBAR_SEPARATOR);
		g_string_append (status, _("Modified"));
	}
	else {
		if (file_size != NULL) {
			if (status->len > 0)
				g_string_append (status, STATUSBAR_SEPARATOR);
			g_string_append (status, file_size);
		}
		if (file_date != NULL) {
			if (status->len > 0)
				g_string_append (status, STATUSBAR_SEPARATOR);
			g_string_append (status, file_date);
		}
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
		if (file_data->info != browser->priv->location->info)
			g_file_info_copy_into (file_data->info, browser->priv->location->info);
		gth_browser_update_extra_widget (browser);
		return;
	}

	if ((browser->priv->current_file != NULL) && g_file_equal (browser->priv->current_file->file, file_data->file)) {
		if (file_data->info != browser->priv->current_file->info)
			g_file_info_copy_into (file_data->info, browser->priv->current_file->info);

		gth_sidebar_set_file (GTH_SIDEBAR (browser->priv->file_properties), browser->priv->current_file);

		gth_browser_update_statusbar_file_info (browser);
		gth_browser_update_title (browser);
		gth_browser_update_sensitivity (browser);
	}
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
		gth_file_list_update_emblems (GTH_FILE_LIST (browser->priv->thumbnail_list), files);
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
pref_general_filter_changed (GSettings  *settings,
			     const char *key,
			     gpointer    user_data)
{
	GthBrowser *browser = user_data;
	GthTest    *filter;

	filter = _gth_browser_get_file_filter (browser);
	gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->file_list), filter);
	gth_file_list_set_filter (GTH_FILE_LIST (browser->priv->thumbnail_list), filter);

	g_object_unref (filter);
}


static void
gth_file_list_popup_menu (GthBrowser     *browser,
			  GdkEventButton *event)
{
	int button, event_time;

	gth_hook_invoke ("gth-browser-file-list-popup-before", browser);
	gtk_ui_manager_ensure_update (browser->priv->ui);

	if (event != NULL) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (browser->priv->file_list_popup),
			NULL,
			NULL,
			NULL,
			NULL,
			button,
			event_time);
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

	gth_hook_invoke ("gth-browser-selection-changed", browser);

	if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_BROWSER_PAGE_BROWSER)
		return FALSE;

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
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
			gth_browser_load_file (browser, file_data, TRUE);
	}
}


static gboolean
gth_file_list_key_press_cb (GtkWidget   *widget,
			    GdkEventKey *event,
			    gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gboolean    result = FALSE;
	guint       modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();

	if ((event->state & modifiers) == 0) {
		switch (event->keyval) {
		case GDK_KEY_f:
			gth_browser_fullscreen (browser);
			result = TRUE;
			break;

		case GDK_KEY_e:
			if (browser->priv->viewer_page != NULL)
				gth_browser_show_viewer_tools (GTH_BROWSER (browser));
			result = TRUE;
			break;

		case GDK_KEY_i:
			if (_gth_browser_get_action_active (browser, "Browser_Properties"))
				gth_browser_hide_sidebar (browser);
			else
				gth_browser_show_file_properties (browser);
			result = TRUE;
			break;

		default:
			break;
		}
        }

	if (! result)
		result = gth_hook_invoke_get ("gth-browser-file-list-key-press", browser, event) != NULL;

	return result;
}


static void
_gth_browser_add_custom_actions (GthBrowser     *browser,
				 GtkActionGroup *actions)
{
	GtkAction *action;

	/* Go Back */

	browser->priv->back_history_menu = gtk_menu_new ();
	action = g_object_new (GTH_TYPE_MENU_ACTION,
			       "name", "Toolbar_Go_Back",
			       "stock-id", GTK_STOCK_GO_BACK,
			       "button-tooltip", _("Go to the previous visited location"),
			       "arrow-tooltip", _("View the list of visited locations"),
			       "is-important", TRUE,
			       "menu", browser->priv->back_history_menu,
			       NULL);
	g_signal_connect (action,
			  "activate",
			  G_CALLBACK (gth_browser_activate_action_go_back),
			  browser);
	gtk_action_group_add_action (actions, action);
	g_object_unref (action);

	/* Go Forward */

	browser->priv->forward_history_menu = gtk_menu_new ();
	action = g_object_new (GTH_TYPE_MENU_ACTION,
			       "name", "Toolbar_Go_Forward",
			       "stock-id", GTK_STOCK_GO_FORWARD,
			       "button-tooltip", _("Go to the next visited location"),
			       "arrow-tooltip", _("View the list of visited locations"),
			       "is-important", TRUE,
			       "menu", browser->priv->forward_history_menu,
			       NULL);
	g_signal_connect (action,
			  "activate",
			  G_CALLBACK (gth_browser_activate_action_go_forward),
			  browser);
	gtk_action_group_add_action (actions, action);
	g_object_unref (action);

	/* Go Up */

	browser->priv->go_parent_menu = gtk_menu_new ();
	action = g_object_new (GTH_TYPE_MENU_ACTION,
			       "name", "Toolbar_Go_Up",
			       "stock-id", GTK_STOCK_GO_UP,
			       "button-tooltip", _("Go up one level"),
			       "arrow-tooltip", _("View the list of upper locations"),
			       "is-important", FALSE,
			       "menu", browser->priv->go_parent_menu,
			       NULL);
	g_signal_connect (action,
			  "activate",
			  G_CALLBACK (gth_browser_activate_action_go_up),
			  browser);
	gtk_action_group_add_action (actions, action);
	g_object_unref (action);
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

	if (data->file_to_select != NULL)
		gth_browser_go_to (browser, data->location, data->file_to_select);
	else
		gth_browser_load_location (browser, data->location);

	new_window_data_free (data);
}


static void
_gth_browser_update_toolbar_style (GthBrowser *browser)
{
	GthToolbarStyle toolbar_style;
	GtkToolbarStyle prop = GTK_TOOLBAR_BOTH;

	toolbar_style = gth_pref_get_real_toolbar_style ();
	switch (toolbar_style) {
	case GTH_TOOLBAR_STYLE_TEXT_BELOW:
		prop = GTK_TOOLBAR_BOTH;
		break;
	case GTH_TOOLBAR_STYLE_TEXT_BESIDE:
		prop = GTK_TOOLBAR_BOTH_HORIZ;
		break;
	case GTH_TOOLBAR_STYLE_ICONS:
		prop = GTK_TOOLBAR_ICONS;
		break;
	case GTH_TOOLBAR_STYLE_TEXT:
		prop = GTK_TOOLBAR_TEXT;
		break;
	default:
		break;
	}

	gtk_toolbar_set_style (GTK_TOOLBAR (browser->priv->browser_toolbar), prop);
	gtk_toolbar_set_style (GTK_TOOLBAR (browser->priv->viewer_toolbar), prop);
}


static void
pref_ui_toolbar_style_changed (GSettings  *settings,
			       const char *key,
			       gpointer    user_data)
{
	GthBrowser *browser = user_data;
	_gth_browser_update_toolbar_style (browser);
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

	gtk_widget_unrealize (browser->priv->file_properties);
	gtk_widget_reparent (browser->priv->file_properties, new_parent);
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

	if (_gth_browser_get_action_active (browser, "View_Thumbnail_List"))
		gtk_widget_show (browser->priv->thumbnail_list);
	gtk_widget_show (browser->priv->viewer_sidebar_pane);
	gtk_widget_show (browser->priv->viewer_thumbnails_pane);
}


static void
_gth_browser_set_toolbar_visibility (GthBrowser *browser,
				    gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	_gth_browser_set_action_active (browser, "View_Toolbar", visible);
	if (visible) {
		gtk_widget_show (browser->priv->browser_toolbar);
		gtk_widget_show (browser->priv->viewer_toolbar);
	}
	else {
		gtk_widget_hide (browser->priv->browser_toolbar);
		gtk_widget_hide (browser->priv->viewer_toolbar);
	}
}


static void
pref_ui_toolbar_visible_changed (GSettings  *settings,
				 const char *key,
				 gpointer    user_data)
{
	GthBrowser *browser = user_data;
	_gth_browser_set_toolbar_visibility (browser, g_settings_get_boolean (settings, key));
}


static void
_gth_browser_set_statusbar_visibility (GthBrowser *browser,
				       gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	_gth_browser_set_action_active (browser, "View_Statusbar", visible);
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

	_gth_browser_set_action_active (browser, "View_Sidebar", visible);
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
	gth_hook_invoke ("gth-browser-selection-changed", browser);
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

	_gth_browser_set_action_active (browser, "View_Thumbnail_List", visible);
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

	_gth_browser_set_action_active (browser, "View_ShowHiddenFiles", show_hidden_files);
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
gth_browser_init (GthBrowser *browser)
{
	int             window_width;
	int             window_height;
	GError         *error = NULL;
	GtkWidget      *vbox;
	GtkWidget      *scrolled_window;
	GtkWidget      *menubar;
	char           *general_filter;
	char           *sort_type;
	char           *caption;
	int             i;

	g_object_set (browser, "n-pages", GTH_BROWSER_N_PAGES, NULL);

	browser->priv = G_TYPE_INSTANCE_GET_PRIVATE (browser, GTH_TYPE_BROWSER, GthBrowserPrivate);
	browser->priv->viewer_pages = NULL;
	browser->priv->viewer_page = NULL;
	browser->priv->image_preloader = gth_image_preloader_new (GTH_LOAD_POLICY_TWO_STEPS, 10);
	browser->priv->progress_dialog = NULL;
	browser->priv->named_dialogs = g_hash_table_new (g_str_hash, g_str_equal);
	for (i = 0; i < GTH_BROWSER_N_PAGES; i++)
		browser->priv->toolbar_menu_buttons[i] = NULL;
	browser->priv->browser_ui_merge_id = 0;
	browser->priv->viewer_ui_merge_id = 0;
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
	browser->priv->background_tasks = NULL;
	browser->priv->load_data_queue = NULL;
	browser->priv->load_file_data_queue = NULL;
	browser->priv->load_file_timeout = 0;
	browser->priv->list_attributes = NULL;
	browser->priv->constructed = FALSE;
	browser->priv->selection_changed_event = 0;
	browser->priv->folder_popup_file_data = NULL;
	browser->priv->properties_on_screen = FALSE;
	browser->priv->location_free_space = NULL;
	browser->priv->recalc_location_free_space = TRUE;
	browser->priv->fullscreen = FALSE;
	browser->priv->fullscreen_toolbar = NULL;
	browser->priv->fullscreen_controls = NULL;
	browser->priv->hide_mouse_timeout = 0;
	browser->priv->motion_signal = 0;
	browser->priv->last_mouse_x = 0.0;
	browser->priv->last_mouse_y = 0.0;
	browser->priv->history = NULL;
	browser->priv->history_current = NULL;
	browser->priv->back_history_menu = NULL;
	browser->priv->forward_history_menu = NULL;
	browser->priv->go_parent_menu = NULL;
	browser->priv->shrink_wrap_viewer = FALSE;
	browser->priv->browser_settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	browser->priv->messages_settings = g_settings_new (GTHUMB_MESSAGES_SCHEMA);
	browser->priv->desktop_interface_settings = g_settings_new (GNOME_DESKTOP_INTERFACE_SCHEMA);
	browser->priv->file_properties_on_the_right = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT);

	browser_state_init (&browser->priv->state);

	/* find a suitable size for the window */

	window_width = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_WINDOW_WIDTH);
	window_height = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_WINDOW_HEIGHT);

	if ((window_width == 0) || (window_height == 0)) {
		GdkScreen *screen;
		int        max_width;
		int        max_height;
		int        sidebar_width;
		int        thumb_size;
		int        thumb_spacing;
		int        default_columns_of_thumbnails;
		int        n_cols;

		screen = gtk_widget_get_screen (GTK_WIDGET (browser));
		max_width = gdk_screen_get_width (screen) * 5 / 6;
		max_height = gdk_screen_get_height (screen) * 3 / 4;

		sidebar_width = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_BROWSER_SIDEBAR_WIDTH) + 10;
		thumb_size = g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE);
		thumb_spacing = 40;
		default_columns_of_thumbnails = 5;

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
	gtk_window_set_has_resize_grip (GTK_WINDOW (browser), TRUE);

	/* realize the widget before adding the ui to get the icons from the icon theme */

	g_signal_connect (browser, "realize", G_CALLBACK (_gth_browser_realize), NULL);
	g_signal_connect (browser, "unrealize", G_CALLBACK (_gth_browser_unrealize), NULL);
	gtk_widget_realize (GTK_WIDGET (browser));

	/* ui actions */

	browser->priv->actions = gtk_action_group_new ("Actions");
	gtk_action_group_set_translation_domain (browser->priv->actions, NULL);
	gtk_action_group_add_actions (browser->priv->actions,
				      gth_window_action_entries,
				      G_N_ELEMENTS (gth_window_action_entries),
				      browser);
	_gtk_action_group_add_actions_with_flags (browser->priv->actions,
						  gth_browser_action_entries,
						  G_N_ELEMENTS (gth_browser_action_entries),
						  browser);
	gtk_action_group_add_toggle_actions (browser->priv->actions,
					     gth_browser_action_toggle_entries,
					     G_N_ELEMENTS (gth_browser_action_toggle_entries),
					     browser);
	_gth_browser_add_custom_actions (browser, browser->priv->actions);

	browser->priv->ui = gtk_ui_manager_new ();
	g_signal_connect (browser->priv->ui,
			  "connect_proxy",
			  G_CALLBACK (connect_proxy_cb),
			  browser);
	g_signal_connect (browser->priv->ui,
			  "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb),
			  browser);

	gtk_ui_manager_insert_action_group (browser->priv->ui, browser->priv->actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (browser), gtk_ui_manager_get_accel_group (browser->priv->ui));

	if (! gtk_ui_manager_add_ui_from_string (browser->priv->ui, fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	/* -- image page -- */

	/* toolbar */

	browser->priv->viewer_toolbar = gtk_ui_manager_get_widget (browser->priv->ui, "/ViewerToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (browser->priv->viewer_toolbar), TRUE);
	gth_window_attach_toolbar (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER, browser->priv->viewer_toolbar);

	/* content */

	browser->priv->viewer_thumbnails_orientation = g_settings_get_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT);
	if (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		browser->priv->viewer_thumbnails_pane = gth_paned_new (GTK_ORIENTATION_VERTICAL);
	else
		browser->priv->viewer_thumbnails_pane = gth_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show (browser->priv->viewer_thumbnails_pane);
	gth_window_attach_content (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER, browser->priv->viewer_thumbnails_pane);

	browser->priv->viewer_sidebar_pane = gth_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_size_request (browser->priv->viewer_sidebar_pane, -1, MIN_VIEWER_SIZE);
	gtk_widget_show (browser->priv->viewer_sidebar_pane);
	if (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_paned_pack1 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->viewer_sidebar_pane, TRUE, FALSE);
	else
		gtk_paned_pack2 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->viewer_sidebar_pane, TRUE, FALSE);

	browser->priv->viewer_container = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_widget_set_size_request (browser->priv->viewer_container, MIN_VIEWER_SIZE, -1);
	gtk_widget_show (browser->priv->viewer_container);

	gtk_paned_pack1 (GTK_PANED (browser->priv->viewer_sidebar_pane), browser->priv->viewer_container, TRUE, FALSE);
	browser->priv->viewer_sidebar_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gth_paned_set_position2 (GTH_PANED (browser->priv->viewer_sidebar_pane), g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_BROWSER_SIDEBAR_WIDTH));
	gtk_paned_pack2 (GTK_PANED (browser->priv->viewer_sidebar_pane), browser->priv->viewer_sidebar_alignment, FALSE, FALSE);

	browser->priv->thumbnail_list = gth_file_list_new (gth_grid_view_new (), (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL) ? GTH_FILE_LIST_MODE_H_SIDEBAR : GTH_FILE_LIST_MODE_V_SIDEBAR, TRUE);
	gth_file_list_set_caption (GTH_FILE_LIST (browser->priv->thumbnail_list), "none");
	gth_grid_view_set_cell_spacing (GTH_GRID_VIEW (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->thumbnail_list))), 0);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (browser->priv->thumbnail_list), 95);
	if (browser->priv->viewer_thumbnails_orientation == GTK_ORIENTATION_HORIZONTAL) {
		gth_paned_set_position2 (GTH_PANED (browser->priv->viewer_thumbnails_pane), g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_LIST_SIZE));
		gtk_paned_pack2 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->thumbnail_list, FALSE, FALSE);
	}
	else {
		gtk_paned_set_position (GTK_PANED (browser->priv->viewer_thumbnails_pane), g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_LIST_SIZE));
		gtk_paned_pack1 (GTK_PANED (browser->priv->viewer_thumbnails_pane), browser->priv->thumbnail_list, FALSE, FALSE);
	}
	_gth_browser_set_thumbnail_list_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE));

	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (browser->priv->thumbnail_list)),
			  "file-selection-changed",
			  G_CALLBACK (gth_thumbnail_view_selection_changed_cb),
			  browser);

	/* -- browser page -- */

	/* menus */

	menubar = gtk_ui_manager_get_widget (browser->priv->ui, "/MenuBar");
#ifdef USE_MACOSMENU
	{
		GtkWidget *widget;

		ige_mac_menu_install_key_handler ();
		ige_mac_menu_set_menu_bar (GTK_MENU_SHELL (menubar));
		gtk_widget_hide (menubar);
		widget = gtk_ui_manager_get_widget(ui, "/MenuBar/File/Close");
		if (widget != NULL) {
			ige_mac_menu_set_quit_menu_item (GTK_MENU_ITEM (widget));
		}
		widget = gtk_ui_manager_get_widget(ui, "/MenuBar/Help/About");
		if (widget != NULL) {
			ige_mac_menu_add_app_menu_item  (ige_mac_menu_add_app_menu_group (),
			GTK_MENU_ITEM (widget),
			NULL);
		}
		widget = gtk_ui_manager_get_widget(ui, "/MenuBar/Edit/Preferences");
			if (widget != NULL) {
			ige_mac_menu_add_app_menu_item  (ige_mac_menu_add_app_menu_group (),
			GTK_MENU_ITEM (widget),
			NULL);
		}
	}
#endif
	gth_window_attach (GTH_WINDOW (browser), menubar, GTH_WINDOW_MENUBAR);
	browser->priv->folder_popup = gtk_ui_manager_get_widget (browser->priv->ui, "/FolderListPopup");
	g_signal_connect (browser->priv->folder_popup,
			  "hide",
			  G_CALLBACK (folder_popup_hide_cb),
			  browser);

	/* toolbar */

	browser->priv->browser_toolbar = gtk_ui_manager_get_widget (browser->priv->ui, "/ToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (browser->priv->browser_toolbar), TRUE);
	gth_window_attach_toolbar (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER, browser->priv->browser_toolbar);

	/* infobar */

	browser->priv->infobar = gth_info_bar_new (NULL, NULL, NULL);
	gth_window_attach (GTH_WINDOW (browser), browser->priv->infobar, GTH_WINDOW_INFOBAR);

	/* statusbar */

	browser->priv->statusbar = gth_statusbar_new ();
	browser->priv->help_message_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (browser->priv->statusbar), "gth_help_message");
	_gth_browser_set_statusbar_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_STATUSBAR_VISIBLE));
	gth_window_attach (GTH_WINDOW (browser), browser->priv->statusbar, GTH_WINDOW_STATUSBAR);

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
	gtk_widget_set_size_request (vbox, -1, FILE_PROPERTIES_MINIMUM_HEIGHT);
	gtk_widget_show (vbox);
	gtk_paned_pack1 (GTK_PANED (browser->priv->browser_sidebar), vbox, TRUE, FALSE);

	/* the folder list */

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_ETCHED_IN);
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
			  "load",
			  G_CALLBACK (folder_tree_load_cb),
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

	browser->priv->file_properties = gth_sidebar_new ("file-tools");
	gtk_widget_set_size_request (browser->priv->file_properties, -1, FILE_PROPERTIES_MINIMUM_HEIGHT);
	gtk_widget_hide (browser->priv->file_properties);
	gtk_paned_pack2 (GTK_PANED (_gth_browser_get_browser_file_properties_container (browser)),
			 browser->priv->file_properties,
			 ! browser->priv->file_properties_on_the_right,
			 FALSE);

	/* the box that contains the file list and the filter bar.  */

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	gtk_paned_pack2 (GTK_PANED (browser->priv->browser_left_container), vbox, TRUE, TRUE);

	/* the list extra widget container */

	browser->priv->list_extra_widget_container = gtk_alignment_new (0, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (browser->priv->list_extra_widget_container), 0, 0, 0, 0);
	gtk_widget_show (browser->priv->list_extra_widget_container);
	gtk_box_pack_start (GTK_BOX (vbox), browser->priv->list_extra_widget_container, FALSE, FALSE, 0);

	browser->priv->list_extra_widget = gth_embedded_dialog_new ();
	gtk_widget_show (browser->priv->list_extra_widget);
	gtk_container_add (GTK_CONTAINER (browser->priv->list_extra_widget_container), browser->priv->list_extra_widget);

	g_signal_connect (gth_embedded_dialog_get_chooser (GTH_EMBEDDED_DIALOG (browser->priv->list_extra_widget)),
			  "changed",
			  G_CALLBACK (location_chooser_changed_cb),
			  browser);

	/* the file list */

	browser->priv->file_list = gth_file_list_new (gth_grid_view_new (), GTH_FILE_LIST_MODE_NORMAL, TRUE);
	sort_type = g_settings_get_string (browser->priv->browser_settings, PREF_BROWSER_SORT_TYPE);
	gth_browser_set_sort_order (browser,
				    gth_main_get_sort_type (sort_type),
				    g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SORT_INVERSE));
	gth_browser_enable_thumbnails (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SHOW_THUMBNAILS));
	gth_file_list_set_thumb_size (GTH_FILE_LIST (browser->priv->file_list),
				      g_settings_get_int (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE));
	caption = g_settings_get_string (browser->priv->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION);
	gth_file_list_set_caption (GTH_FILE_LIST (browser->priv->file_list), caption);

	g_free (caption);
	g_free (sort_type);

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
	g_signal_connect (browser->priv->file_list,
			  "key-press-event",
			  G_CALLBACK (gth_file_list_key_press_cb),
			  browser);

	browser->priv->file_list_popup = gtk_ui_manager_get_widget (browser->priv->ui, "/FileListPopup");

	/* the filter bar */

	general_filter = g_settings_get_string (browser->priv->browser_settings, PREF_BROWSER_GENERAL_FILTER);
	browser->priv->filterbar = gth_filterbar_new (general_filter);
	gth_filterbar_load_filter (GTH_FILTERBAR (browser->priv->filterbar), "active_filter.xml");
	g_free (general_filter);

	gth_browser_show_filterbar (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_FILTERBAR_VISIBLE));
	gtk_box_pack_end (GTK_BOX (vbox), browser->priv->filterbar, FALSE, FALSE, 0);

	g_signal_connect (browser->priv->filterbar,
			  "changed",
			  G_CALLBACK (filterbar_changed_cb),
			  browser);
	g_signal_connect (browser->priv->filterbar,
			  "personalize",
			  G_CALLBACK (filterbar_personalize_cb),
			  browser);
	g_signal_connect (browser->priv->filterbar,
			  "close_button_clicked",
			  G_CALLBACK (filterbar_close_button_clicked_cb),
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

	/* init browser data */

	browser->priv->file_popup = gtk_ui_manager_get_widget (browser->priv->ui, "/FilePopup");

	_gth_browser_set_sidebar_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SIDEBAR_VISIBLE));
	_gth_browser_set_toolbar_visibility (browser, g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_TOOLBAR_VISIBLE));
	_gth_browser_update_toolbar_style (browser);

	browser->priv->show_hidden_files = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SHOW_HIDDEN_FILES);
	_gth_browser_set_action_active (browser, "View_ShowHiddenFiles", browser->priv->show_hidden_files);

	browser->priv->shrink_wrap_viewer = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_SHRINK_WRAP_VIEWER);
	_gth_browser_set_action_active (browser, "View_ShrinkWrap", browser->priv->shrink_wrap_viewer);

	if (g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_VISIBLE))
		gth_browser_show_file_properties (browser);
	else
		gth_browser_hide_sidebar (browser);

	browser->priv->viewer_sidebar = g_settings_get_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_SIDEBAR);

	browser->priv->fast_file_type = g_settings_get_boolean (browser->priv->browser_settings, PREF_BROWSER_FAST_FILE_TYPE);

	/* load the history only for the first window */
	{
		GList * windows = gtk_application_get_windows (Main_Application);
		if ((windows == NULL) || (windows->next == NULL))
			_gth_browser_history_load (browser);
	}

	gtk_widget_realize (browser->priv->file_list);
	gth_hook_invoke ("gth-browser-construct", browser);

	performance (DEBUG_INFO, "window initialized");

	/* settings notifications */

	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_GENERAL_FILTER,
			  G_CALLBACK (pref_general_filter_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_TOOLBAR_STYLE,
			  G_CALLBACK (pref_ui_toolbar_style_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT,
			  G_CALLBACK (pref_ui_viewer_thumbnails_orient_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_PROPERTIES_ON_THE_RIGHT,
			  G_CALLBACK (pref_browser_properties_on_the_right_changed),
			  browser);
	g_signal_connect (browser->priv->desktop_interface_settings,
			  "changed::" PREF_BROWSER_TOOLBAR_STYLE,
			  G_CALLBACK (pref_ui_toolbar_style_changed),
			  browser);
	g_signal_connect (browser->priv->browser_settings,
			  "changed::" PREF_BROWSER_TOOLBAR_VISIBLE,
			  G_CALLBACK (pref_ui_toolbar_visible_changed),
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
	return browser->priv->location->file;
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

	location = g_file_new_for_uri (gth_pref_get_startup_location ());
	gth_browser_go_to (browser, location, NULL);

	g_object_unref (location);
}


void
gth_browser_clear_history (GthBrowser *browser)
{
	_g_object_list_unref (browser->priv->history);
	browser->priv->history = NULL;
	browser->priv->history_current = NULL;

	_gth_browser_history_add (browser, browser->priv->location->file);
	_gth_browser_history_menu (browser);
}


void
gth_browser_set_dialog (GthBrowser *browser,
			const char *dialog_name,
			GtkWidget  *dialog)
{
	g_hash_table_insert (browser->priv->named_dialogs, (gpointer) dialog_name, dialog);
}


GtkWidget *
gth_browser_get_dialog (GthBrowser *browser,
			const char *dialog_name)
{
	return g_hash_table_lookup (browser->priv->named_dialogs, dialog_name);
}


GtkUIManager *
gth_browser_get_ui_manager (GthBrowser *browser)
{
	return browser->priv->ui;
}


GtkActionGroup *
gth_browser_get_actions (GthBrowser *browser)
{
	return browser->priv->actions;
}


GthIconCache *
gth_browser_get_menu_icon_cache (GthBrowser *browser)
{
	return browser->priv->menu_icon_cache;
}


GtkWidget *
gth_browser_get_browser_toolbar (GthBrowser *browser)
{
	return browser->priv->browser_toolbar;
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
	_gth_browser_set_sort_order (browser, sort_type, sort_inverse, TRUE);
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
	if (browser->priv->fullscreen)
		gth_browser_unfullscreen (browser);
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
	GthBrowser *browser;
	GthTask    *task;
	gulong      completed_event;
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

	browser->priv->background_tasks = g_list_remove (browser->priv->background_tasks, task_data);
	task_data_free (task_data);

	if (error == NULL)
		return;

	if (! g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED) && ! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
		_gth_browser_show_error (browser, _("Could not perform the operation"), error);
}


static TaskData *
task_data_new (GthBrowser *browser,
	       GthTask    *task)
{
	TaskData *task_data;

	task_data = g_new0 (TaskData, 1);
	task_data->browser = g_object_ref (browser);
	task_data->task = g_object_ref (task);
	task_data->completed_event = g_signal_connect (task_data->task,
						       "completed",
						       G_CALLBACK (background_task_completed_cb),
						       task_data);
	return task_data;
}


static void
foreground_task_completed_cb (GthTask    *task,
			      GError     *error,
			      GthBrowser *browser)
{
	browser->priv->activity_ref--;

	g_signal_handler_disconnect (browser->priv->task, browser->priv->task_completed);
	g_signal_handler_disconnect (browser->priv->task, browser->priv->task_progress);

	gth_statusbar_set_progress (GTH_STATUSBAR (browser->priv->statusbar), NULL, FALSE, 0.0);
	gth_browser_update_sensitivity (browser);
	if ((error != NULL) && ! g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED))
		_gth_browser_show_error (browser, _("Could not perform the operation"), error);

	g_object_unref (browser->priv->task);
	browser->priv->task = NULL;
}


static void
foreground_task_progress_cb (GthTask    *task,
			     const char *description,
			     const char *details,
			     gboolean    pulse,
			     double      fraction,
			     GthBrowser *browser)
{
	gth_statusbar_set_progress (GTH_STATUSBAR (browser->priv->statusbar), description, pulse, fraction);
}


void
gth_browser_exec_task (GthBrowser *browser,
		       GthTask    *task,
		       gboolean    foreground)
{
	g_return_if_fail (task != NULL);

	if (! foreground) {
		TaskData *task_data;

		task_data = task_data_new (browser, task);
		browser->priv->background_tasks = g_list_prepend (browser->priv->background_tasks, task_data);

		if (browser->priv->progress_dialog == NULL) {
			browser->priv->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (browser));
			g_object_add_weak_pointer (G_OBJECT (browser->priv->progress_dialog), (gpointer*) &(browser->priv->progress_dialog));
		}
		gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (browser->priv->progress_dialog), task);

		return;
	}

	/* foreground task */

	if (browser->priv->task != NULL)
		gth_task_cancel (task);

	browser->priv->task = g_object_ref (task);
	browser->priv->task_completed = g_signal_connect (task,
							  "completed",
							  G_CALLBACK (foreground_task_completed_cb),
							  browser);
	browser->priv->task_progress = g_signal_connect (task,
							 "progress",
							 G_CALLBACK (foreground_task_progress_cb),
							 browser);
	browser->priv->activity_ref++;
	gth_browser_update_sensitivity (browser);
	gth_task_exec (browser->priv->task, NULL);
}


GtkWidget *
gth_browser_get_list_extra_widget (GthBrowser *browser)
{
	return browser->priv->list_extra_widget;
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
	g_return_val_if_fail (event != NULL, FALSE);

	if (gth_sidebar_tool_is_active (GTH_SIDEBAR (browser->priv->file_properties)))
		return FALSE;

	if (event->state & GDK_SHIFT_MASK)
		return FALSE;

	if (event->state & GDK_CONTROL_MASK)
		return FALSE;

	if ((event->direction != GDK_SCROLL_UP) && (event->direction != GDK_SCROLL_DOWN))
		return FALSE;

	if (event->direction == GDK_SCROLL_UP)
		gth_browser_show_prev_image (browser, FALSE, FALSE);
	else
		gth_browser_show_next_image (browser, FALSE, FALSE);

	return TRUE;
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
	guint modifiers;

	g_return_val_if_fail (event != NULL, FALSE);

	modifiers = gtk_accelerator_get_default_mod_mask ();
	if ((event->state & modifiers) == 0) {
		switch (event->keyval) {
		case GDK_KEY_Page_Up:
		case GDK_KEY_BackSpace:
			gth_browser_show_prev_image (browser, FALSE, FALSE);
			return TRUE;

		case GDK_KEY_Page_Down:
		case GDK_KEY_space:
			gth_browser_show_next_image (browser, FALSE, FALSE);
			return TRUE;

		case GDK_KEY_Home:
			gth_browser_show_first_image (browser, FALSE, FALSE);
			return TRUE;

		case GDK_KEY_End:
			gth_browser_show_last_image (browser, FALSE, FALSE);
			return TRUE;

		case GDK_KEY_e:
			if (browser->priv->viewer_sidebar != GTH_SIDEBAR_STATE_TOOLS)
				gth_browser_show_viewer_tools (browser);
			else
				gth_browser_hide_sidebar (browser);
			return TRUE;

		case GDK_KEY_i:
			gth_browser_toggle_properties_on_screen (browser);
			return TRUE;

		case GDK_KEY_f:
			gth_browser_fullscreen (browser);
			return TRUE;
		}
	}

	if (gtk_widget_get_realized (browser->priv->file_list))
		return gth_hook_invoke_get ("gth-browser-file-list-key-press", browser, event) != NULL;
	else
		return FALSE;
}


void
gth_browser_set_viewer_widget (GthBrowser *browser,
			       GtkWidget  *widget)
{
	_gtk_container_remove_children (GTK_CONTAINER (browser->priv->viewer_container), NULL, NULL);
	if (widget != NULL)
		gtk_container_add (GTK_CONTAINER (browser->priv->viewer_container), widget);
}


GtkWidget *
gth_browser_get_viewer_widget (GthBrowser *browser)
{
	GtkWidget *child = NULL;
	GList     *children;

	children = gtk_container_get_children (GTK_CONTAINER (browser->priv->viewer_container));
	if (children != NULL)
		child = children->data;
	g_list_free (children);

	return child;
}


GtkWidget *
gth_browser_get_viewer_page (GthBrowser *browser)
{
	return (GtkWidget *) browser->priv->viewer_page;
}


GtkWidget *
gth_browser_get_viewer_toolbar (GthBrowser *browser)
{
	return browser->priv->viewer_toolbar;
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


gboolean
gth_browser_show_next_image (GthBrowser *browser,
			     gboolean    skip_broken,
			     gboolean    only_selected)
{
	GthFileView *view;
	int          pos;

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
			gth_browser_load_file (browser, file_data, TRUE);
		}
	}
	else
		gdk_beep ();

	return (pos >= 0);
}


gboolean
gth_browser_show_prev_image (GthBrowser *browser,
			     gboolean    skip_broken,
			     gboolean    only_selected)
{
	GthFileView *view;
	int          pos;

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
			gth_browser_load_file (browser, file_data, TRUE);
		}
	}
	else
		gdk_beep ();

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

	pos = gth_file_list_first_file (GTH_FILE_LIST (browser->priv->file_list), skip_broken, only_selected);
	if (pos < 0)
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));

	if (! gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), pos, &iter))
		return FALSE;

	file_data = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
	gth_browser_load_file (browser, file_data, TRUE);

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

	pos = gth_file_list_last_file (GTH_FILE_LIST (browser->priv->file_list), skip_broken, only_selected);
	if (pos < 0)
		return FALSE;

	view = GTH_FILE_VIEW (gth_browser_get_file_list_view (browser));

	if (! gth_file_store_get_nth_visible (GTH_FILE_STORE (gth_file_view_get_model (view)), pos, &iter))
		return FALSE;

	file_data = gth_file_store_get_file (GTH_FILE_STORE (gth_file_view_get_model (view)), &iter);
	gth_browser_load_file (browser, file_data, TRUE);

	return TRUE;
}


/* -- gth_browser_load_file -- */


typedef struct {
	int           ref;
	GthBrowser   *browser;
	GthFileData  *file_data;
	gboolean      view;
	GCancellable *cancellable;
} LoadFileData;


static LoadFileData *
load_file_data_new (GthBrowser  *browser,
		    GthFileData *file_data,
		    gboolean     view)
{
	LoadFileData *data;

	data = g_new0 (LoadFileData, 1);
	data->ref = 1;
	data->browser = g_object_ref (browser);
	if (file_data != NULL)
		data->file_data = gth_file_data_dup (file_data);
	data->view = view;
	data->cancellable = g_cancellable_new ();

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
		if (browser->priv->fullscreen)
			gth_viewer_page_show_pointer (GTH_VIEWER_PAGE (browser->priv->viewer_page), FALSE);
		gtk_ui_manager_ensure_update (browser->priv->ui);

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

	g_file_info_copy_into (file_data->info, browser->priv->current_file->info);
	g_file_info_set_attribute_boolean (browser->priv->current_file->info, "gth::file::is-modified", FALSE);

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
			_gth_browser_load_file (browser, data->file_data, data->view);
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


static void
gth_viewer_page_file_loaded_cb (GthViewerPage *viewer_page,
				GthFileData   *file_data,
				gboolean       success,
				gpointer       user_data)
{
	GthBrowser   *browser = user_data;
	LoadFileData *data;
	GList        *files;

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

	g_file_info_set_attribute_boolean (browser->priv->current_file->info, "gth::file::is-modified", FALSE);

	if (browser->priv->shrink_wrap_viewer) {
		/* the frame::[width|height] properties have been set by the
		 * viewer, copy them in the current file. */

		g_file_info_set_attribute_int32 (browser->priv->current_file->info, "frame::width",
						 g_file_info_get_attribute_int32 (file_data->info, "frame::width"));
		g_file_info_set_attribute_int32 (browser->priv->current_file->info, "frame::height",
						 g_file_info_get_attribute_int32 (file_data->info, "frame::height"));
		gth_browser_set_shrink_wrap_viewer (browser, TRUE);
	}

	data = load_file_data_new (browser, browser->priv->current_file, FALSE);
	files = g_list_prepend (NULL, browser->priv->current_file->file);
	_g_query_all_metadata_async (files,
				     GTH_LIST_DEFAULT,
				     "*",
				     data->cancellable,
				     file_metadata_ready_cb,
				     data);

	g_list_free (files);
}


static void
_gth_browser_load_file (GthBrowser  *browser,
			GthFileData *file_data,
			gboolean     view)
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

	if (browser->priv->viewer_pages == NULL)
		browser->priv->viewer_pages = g_list_reverse (gth_main_get_registered_objects (GTH_TYPE_VIEWER_PAGE));

	for (scan = browser->priv->viewer_pages; scan; scan = scan->next) {
		GthViewerPage *registered_viewer_page = scan->data;

		if (gth_viewer_page_can_view (registered_viewer_page, browser->priv->current_file)) {
			_gth_browser_set_current_viewer_page (browser, registered_viewer_page);
			break;
		}
	}

	if (view)
		gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);

	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER) {
		int file_pos;

		gth_viewer_page_show (browser->priv->viewer_page);
		if (browser->priv->fullscreen) {
			gth_viewer_page_fullscreen (browser->priv->viewer_page, TRUE);
			gth_viewer_page_show_pointer (browser->priv->viewer_page, FALSE);
		}

		file_pos = gth_file_store_get_pos (GTH_FILE_STORE (gth_browser_get_file_store (browser)), browser->priv->current_file->file);
		if (file_pos >= 0) {
			GtkWidget *view;

			/* the main file list */

			view = gth_browser_get_file_list_view (browser);
			g_signal_handlers_block_by_func (view, gth_file_view_selection_changed_cb, browser);
			gth_file_view_set_cursor (GTH_FILE_VIEW (view), file_pos);
			g_signal_handlers_unblock_by_func (view, gth_file_view_selection_changed_cb, browser);

			/* the thumbnail list in viewer mode */

			view = gth_browser_get_thumbnail_list_view (browser);
			g_signal_handlers_block_by_func (view, gth_thumbnail_view_selection_changed_cb, browser);
			gth_file_selection_unselect_all (GTH_FILE_SELECTION (view));
			gth_file_selection_select (GTH_FILE_SELECTION (view), file_pos);
			gth_file_view_set_cursor (GTH_FILE_VIEW (view), file_pos);
			g_signal_handlers_unblock_by_func (view, gth_thumbnail_view_selection_changed_cb, browser);
		}
	}

	if (browser->priv->viewer_page != NULL)
		gth_viewer_page_view (browser->priv->viewer_page, browser->priv->current_file);
	else
		gth_viewer_page_file_loaded_cb (NULL, browser->priv->current_file, FALSE, browser);
}


static void
load_file__previuos_file_saved_cb (GthBrowser *browser,
				   gboolean    cancelled,
				   gpointer    user_data)
{
	LoadFileData *data = user_data;

	if (! cancelled)
		_gth_browser_load_file (data->browser, data->file_data, data->view);

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

	if (g_settings_get_boolean (browser->priv->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE)
	    && gth_browser_get_file_modified (browser))
	{
		load_file_data_ref (data);
		gth_browser_ask_whether_to_save (browser,
						 load_file__previuos_file_saved_cb,
						 data);
	}
	else
		_gth_browser_load_file (data->browser, data->file_data, data->view);

	load_file_data_unref (data);

	return FALSE;
}


void
gth_browser_load_file (GthBrowser  *browser,
		       GthFileData *file_data,
		       gboolean     view)
{
	LoadFileData *data;

	_gth_browser_hide_infobar (browser);

	data = load_file_data_new (browser, file_data, view);

	if (view) {
		if (browser->priv->load_file_timeout != 0) {
			g_source_remove (browser->priv->load_file_timeout);
			browser->priv->load_file_timeout = 0;
		}
		load_file_delayed_cb (data);
		load_file_data_unref (data);
	}
	else {
		if (browser->priv->load_file_timeout != 0)
			g_source_remove (browser->priv->load_file_timeout);
		browser->priv->load_file_timeout =
				g_timeout_add_full (G_PRIORITY_DEFAULT,
						    LOAD_FILE_DELAY,
						    load_file_delayed_cb,
						    data,
						    (GDestroyNotify) load_file_data_unref);
	}
}


void
gth_browser_show_file_properties (GthBrowser *browser)
{
	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
	case GTH_WINDOW_PAGE_UNDEFINED: /* when called from gth_browser_init */
		g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_VISIBLE, TRUE);
		_gth_browser_set_action_active (browser, "Browser_Properties", TRUE);
		if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_WINDOW_PAGE_UNDEFINED)
			gtk_widget_show (browser->priv->file_properties);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		_gth_browser_set_action_active (browser, "Viewer_Tools", FALSE);
		browser->priv->viewer_sidebar = GTH_SIDEBAR_STATE_PROPERTIES;
		_gth_browser_set_action_active (browser, "Viewer_Properties", TRUE);
		gtk_widget_show (browser->priv->viewer_sidebar_alignment);
		gtk_widget_show (browser->priv->file_properties);
		gth_sidebar_show_properties (GTH_SIDEBAR (browser->priv->file_properties));
		break;
	}
}


void
gth_browser_show_viewer_tools (GthBrowser *browser)
{
	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);

	_gth_browser_set_action_active (browser, "Viewer_Properties", FALSE);
	browser->priv->viewer_sidebar = GTH_SIDEBAR_STATE_TOOLS;
	_gth_browser_set_action_active (browser, "Viewer_Tools", TRUE);
	gtk_widget_show (browser->priv->viewer_sidebar_alignment);
	gtk_widget_show (browser->priv->file_properties);
	gth_sidebar_show_tools (GTH_SIDEBAR (browser->priv->file_properties));
}


void
gth_browser_hide_sidebar (GthBrowser *browser)
{
	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_PROPERTIES_VISIBLE, FALSE);
		_gth_browser_set_action_active (browser, "Browser_Properties", FALSE);
		gtk_widget_hide (browser->priv->file_properties);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		if (browser->priv->viewer_sidebar == GTH_SIDEBAR_STATE_PROPERTIES)
			_gth_browser_set_action_active (browser, "Viewer_Properties", FALSE);
		else if (browser->priv->viewer_sidebar == GTH_SIDEBAR_STATE_TOOLS)
			_gth_browser_set_action_active (browser, "Viewer_Tools", FALSE);
		browser->priv->viewer_sidebar = GTH_SIDEBAR_STATE_HIDDEN;
		gtk_widget_hide (browser->priv->viewer_sidebar_alignment);
		break;
	}
}


void
gth_browser_set_shrink_wrap_viewer (GthBrowser *browser,
				    gboolean    value)
{
	int        width;
	int        height;
	double     ratio;
	int        other_width;
	int        other_height;
	GdkScreen *screen;
	int        max_width;
	int        max_height;

	browser->priv->shrink_wrap_viewer = value;
	g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_SHRINK_WRAP_VIEWER, browser->priv->shrink_wrap_viewer);

	if (browser->priv->viewer_page == NULL)
		return;

	if (! browser->priv->shrink_wrap_viewer) {
		if (gth_window_get_page_size (GTH_WINDOW (browser),
					      GTH_BROWSER_PAGE_BROWSER,
					      &width,
					      &height))
		{
			gth_window_save_page_size (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER, width, height);
			gth_window_apply_saved_size (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
		}
		else
			gth_window_clear_saved_size (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
		gth_viewer_page_shrink_wrap (browser->priv->viewer_page, FALSE, NULL, NULL);

		return;
	}

	if (browser->priv->current_file == NULL)
		return;

	width = g_file_info_get_attribute_int32 (browser->priv->current_file->info, "frame::width");
	height = g_file_info_get_attribute_int32 (browser->priv->current_file->info, "frame::height");
	if ((width <= 0) || (height <= 0))
		return;

	ratio = (double) width / height;

	other_width = 0;
	other_height = 0;
	other_height += _gtk_widget_get_allocated_height (gth_window_get_area (GTH_WINDOW (browser), GTH_WINDOW_MENUBAR));
	other_height += _gtk_widget_get_allocated_height (gth_window_get_area (GTH_WINDOW (browser), GTH_WINDOW_TOOLBAR));
	other_height += _gtk_widget_get_allocated_height (gth_window_get_area (GTH_WINDOW (browser), GTH_WINDOW_STATUSBAR));
	other_height += _gtk_widget_get_allocated_height (gth_browser_get_viewer_toolbar (browser));
	if (g_settings_get_enum (browser->priv->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT) == GTK_ORIENTATION_HORIZONTAL)
		other_height += _gtk_widget_get_allocated_height (gth_browser_get_thumbnail_list (browser));
	else
		other_width += _gtk_widget_get_allocated_width (gth_browser_get_thumbnail_list (browser));
	other_width += _gtk_widget_get_allocated_width (gth_browser_get_viewer_sidebar (browser));
	other_width += 2;
	other_height += 2;

	gth_viewer_page_shrink_wrap (browser->priv->viewer_page, TRUE, &other_width, &other_height);

	screen = gtk_widget_get_screen (GTK_WIDGET (browser));
	max_width = gdk_screen_get_width (screen) - SHIRNK_WRAP_WIDTH_OFFSET;
	max_height = gdk_screen_get_height (screen)- SHIRNK_WRAP_HEIGHT_OFFSET;

	if (width + other_width > max_width) {
		width = max_width - other_width;
		height = width / ratio;
	}

	if (height + other_height > max_height) {
		height = max_height - other_height;
		width = height * ratio;
	}

	gth_window_save_page_size (GTH_WINDOW (browser),
				   GTH_BROWSER_PAGE_VIEWER,
				   width + other_width,
				   height + other_height);
	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER)
		gth_window_apply_saved_size (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
}


gboolean
gth_browser_get_shrink_wrap_viewer (GthBrowser *browser)
{
	return browser->priv->shrink_wrap_viewer;
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

			title =  file_format (_("Could not load the position \"%s\""), data->location_data->file);
			error = g_error_new (GTH_ERROR, 0, _("File type not supported"));
			_gth_browser_show_error (browser, title, error);
			g_clear_error (&error);

			g_free (title);
		}
	}
	else if (browser->priv->history == NULL) {
		GFile *home;

		home = g_file_new_for_uri (get_home_uri ());
		gth_browser_load_location (browser, home);

		g_object_unref (home);
	}
	else {
		char *title;

		title =  file_format (_("Could not load the position \"%s\""), data->location_data->file);
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

		title =  file_format (_("Could not load the position \"%s\""), data->location_data->file);
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
	    && (browser->priv->background_tasks == NULL))
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

	for (scan = browser->priv->background_tasks; scan; scan = scan->next) {
		TaskData *data = scan->data;
		if (gth_task_is_running (data->task))
			gth_task_cancel (data->task);
	}

	if ((browser->priv->task != NULL) && gth_task_is_running (browser->priv->task))
		gth_task_cancel (browser->priv->task);

	cancel_data->check_id = g_timeout_add (CHECK_CANCELLABLE_INTERVAL,
					       check_cancellable_cb,
					       cancel_data);
}


void
gth_browser_enable_thumbnails (GthBrowser *browser,
			       gboolean    show)
{
	gth_file_list_enable_thumbs (GTH_FILE_LIST (browser->priv->file_list), show);
	_gth_browser_set_action_active (browser, "View_Thumbnails", show);
	g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_SHOW_THUMBNAILS, show);
}


void
gth_browser_show_filterbar (GthBrowser *browser,
			    gboolean    show)
{
	if (show)
		gtk_widget_show (browser->priv->filterbar);
	else
		gtk_widget_hide (browser->priv->filterbar);
	_gth_browser_set_action_active (browser, "View_Filterbar", show);
	g_settings_set_boolean (browser->priv->browser_settings, PREF_BROWSER_FILTERBAR_VISIBLE, show);
}


gpointer
gth_browser_get_image_preloader (GthBrowser *browser)
{
	return g_object_ref (browser->priv->image_preloader);
}


void
gth_browser_register_fullscreen_control (GthBrowser *browser,
					 GtkWidget  *widget)
{
	browser->priv->fullscreen_controls = g_list_prepend (browser->priv->fullscreen_controls, widget);
}


void
gth_browser_unregister_fullscreen_control (GthBrowser *browser,
					   GtkWidget  *widget)
{
	browser->priv->fullscreen_controls = g_list_remove (browser->priv->fullscreen_controls, widget);
}


static void
_gth_browser_move_fullscreen_toolbar (GthBrowser *browser)
{
	GdkScreen    *screen;
	int           n_monitor;
	GdkRectangle  work_area;
	int           preferred_height;

	gtk_widget_realize (browser->priv->fullscreen_toolbar);

	screen = gtk_widget_get_screen (GTK_WIDGET (browser));
	n_monitor = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (GTK_WIDGET (browser)));
	gdk_screen_get_monitor_geometry (screen, n_monitor, &work_area);

	gtk_window_set_screen (GTK_WINDOW (browser->priv->fullscreen_toolbar), screen);
	gtk_widget_get_preferred_height (browser->priv->fullscreen_toolbar, &preferred_height, NULL);
	gtk_window_resize (GTK_WINDOW (browser->priv->fullscreen_toolbar),
			   work_area.width,
			   preferred_height);
	gtk_window_move (GTK_WINDOW (browser->priv->fullscreen_toolbar), work_area.x, work_area.y);
}


static void
_gth_browser_create_fullscreen_toolbar (GthBrowser *browser)
{
	if (browser->priv->fullscreen_toolbar != NULL)
		return;

	browser->priv->fullscreen_toolbar = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_container_set_border_width (GTK_CONTAINER (browser->priv->fullscreen_toolbar), 0);
	gtk_container_add (GTK_CONTAINER (browser->priv->fullscreen_toolbar),
			   gtk_ui_manager_get_widget (browser->priv->ui, "/Fullscreen_ToolBar"));
	gth_browser_register_fullscreen_control (browser, browser->priv->fullscreen_toolbar);
}


typedef struct {
	GthBrowser *browser;
	GdkDevice  *device;
} HideMouseData;


static gboolean
hide_mouse_pointer_cb (gpointer data)
{
	HideMouseData *hmdata = data;
	GthBrowser    *browser = hmdata->browser;
	int            px, py;
	GdkScreen     *screen;
	int            n_monitor;
	GdkRectangle   work_area;
	GList         *scan;

	gdk_window_get_device_position (gtk_widget_get_window (GTK_WIDGET (browser)),
					hmdata->device,
					&px,
					&py,
					0);

	screen = gtk_widget_get_screen (GTK_WIDGET (browser));
	n_monitor = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (GTK_WIDGET (browser)));
	gdk_screen_get_monitor_geometry (screen, n_monitor, &work_area);

	px += work_area.x;
	py += work_area.y;

	for (scan = browser->priv->fullscreen_controls; scan; scan = scan->next) {
		GtkWidget *widget = scan->data;
		int        x, y, w, h;

		gdk_window_get_geometry (gtk_widget_get_window (widget), &x, &y, &w, &h);

		if ((px >= x) && (px <= x + w) && (py >= y) && (py <= y + h))
			return FALSE;
	}

	for (scan = browser->priv->fullscreen_controls; scan; scan = scan->next)
		gtk_widget_hide ((GtkWidget *) scan->data);

	if (browser->priv->viewer_page != NULL)
		gth_viewer_page_show_pointer (GTH_VIEWER_PAGE (browser->priv->viewer_page), FALSE);

	browser->priv->hide_mouse_timeout = 0;

	return FALSE;
}


static gboolean
fullscreen_motion_notify_event_cb (GtkWidget      *widget,
				   GdkEventMotion *event,
				   gpointer        data)
{
	GthBrowser    *browser = data;
	HideMouseData *hmdata;

	if (browser->priv->last_mouse_x == 0.0)
		browser->priv->last_mouse_x = event->x;
	if (browser->priv->last_mouse_y == 0.0)
		browser->priv->last_mouse_y = event->y;

	if ((abs (browser->priv->last_mouse_x - event->x) > MOTION_THRESHOLD)
	    || (abs (browser->priv->last_mouse_y - event->y) > MOTION_THRESHOLD))
	{
		if (! gtk_widget_get_visible (browser->priv->fullscreen_toolbar)) {
			GList *scan;
			int    fullscreen_toolbar_height;

			for (scan = browser->priv->fullscreen_controls; scan; scan = scan->next)
				gtk_widget_show ((GtkWidget *) scan->data);

			if (browser->priv->viewer_page != NULL)
				gth_viewer_page_show_pointer (GTH_VIEWER_PAGE (browser->priv->viewer_page), TRUE);

			gtk_window_get_size (GTK_WINDOW (browser->priv->fullscreen_toolbar), NULL, &fullscreen_toolbar_height);
			gtk_alignment_set_padding (GTK_ALIGNMENT (browser->priv->viewer_sidebar_alignment), fullscreen_toolbar_height + 6, 0, 0, 0);
		}
	}

	if (browser->priv->hide_mouse_timeout != 0)
		g_source_remove (browser->priv->hide_mouse_timeout);

	hmdata = g_new0 (HideMouseData, 1);
	hmdata->browser = browser;
	hmdata->device = event->device;
	browser->priv->hide_mouse_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT,
								HIDE_MOUSE_DELAY,
							   	hide_mouse_pointer_cb,
							   	hmdata,
							   	g_free);

	browser->priv->last_mouse_x = event->x;
	browser->priv->last_mouse_y = event->y;

	return FALSE;
}


void
gth_browser_fullscreen (GthBrowser *browser)
{
	browser->priv->fullscreen = ! browser->priv->fullscreen;

	if (! browser->priv->fullscreen) {
		gth_browser_unfullscreen (browser);
		return;
	}

	if (browser->priv->current_file == NULL)
		if (! gth_browser_show_first_image (browser, FALSE, FALSE)) {
			browser->priv->fullscreen = FALSE;
			return;
		}

	_gth_browser_create_fullscreen_toolbar (browser);
	g_list_free (browser->priv->fullscreen_controls);
	browser->priv->fullscreen_controls = g_list_append (NULL, browser->priv->fullscreen_toolbar);

	browser->priv->before_fullscreen.page = gth_window_get_current_page (GTH_WINDOW (browser));
	browser->priv->before_fullscreen.viewer_properties = _gth_browser_get_action_active (browser, "Viewer_Properties");
	browser->priv->before_fullscreen.viewer_tools = _gth_browser_get_action_active (browser, "Viewer_Tools");
	browser->priv->before_fullscreen.thumbnail_list = _gth_browser_get_action_active (browser, "View_Thumbnail_List");

	gth_browser_hide_sidebar (browser);
	_gth_browser_set_thumbnail_list_visibility (browser, FALSE);
	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
	gth_window_show_only_content (GTH_WINDOW (browser), TRUE);
	_gth_browser_move_fullscreen_toolbar (browser);

	browser->priv->properties_on_screen = FALSE;

	gtk_window_fullscreen (GTK_WINDOW (browser));
	if (browser->priv->viewer_page != NULL) {
		gth_viewer_page_show_properties (browser->priv->viewer_page, browser->priv->properties_on_screen);
		gth_viewer_page_show (browser->priv->viewer_page);
		gth_viewer_page_fullscreen (browser->priv->viewer_page, TRUE);
		gth_viewer_page_show_pointer (browser->priv->viewer_page, FALSE);
	}

	gth_browser_update_sensitivity (browser);

	browser->priv->last_mouse_x = 0.0;
	browser->priv->last_mouse_y = 0.0;
	browser->priv->motion_signal = g_signal_connect (browser,
							 "motion_notify_event",
							 G_CALLBACK (fullscreen_motion_notify_event_cb),
							 browser);
}


void
gth_browser_unfullscreen (GthBrowser *browser)
{
	if (browser->priv->motion_signal != 0)
		g_signal_handler_disconnect (browser, browser->priv->motion_signal);
	if (browser->priv->hide_mouse_timeout != 0)
		g_source_remove (browser->priv->hide_mouse_timeout);

	browser->priv->fullscreen = FALSE;

	gtk_widget_hide (browser->priv->fullscreen_toolbar);
	gth_window_show_only_content (GTH_WINDOW (browser), FALSE);
	gth_window_set_current_page (GTH_WINDOW (browser), browser->priv->before_fullscreen.page);
	_gth_browser_set_thumbnail_list_visibility (browser, browser->priv->before_fullscreen.thumbnail_list);

	if (browser->priv->before_fullscreen.viewer_properties)
		gth_browser_show_file_properties (browser);
	else if (browser->priv->before_fullscreen.viewer_tools)
		gth_browser_show_viewer_tools (browser);
	else
		gth_browser_hide_sidebar (browser);

	browser->priv->properties_on_screen = FALSE;
	if (GTH_VIEWER_PAGE_GET_INTERFACE (browser->priv->viewer_page)->show_properties != NULL)
		gth_viewer_page_show_properties (browser->priv->viewer_page, FALSE);

	gtk_window_unfullscreen (GTK_WINDOW (browser));
	if (browser->priv->viewer_page != NULL) {
		gth_viewer_page_fullscreen (browser->priv->viewer_page, FALSE);
		gth_viewer_page_show_pointer (browser->priv->viewer_page, TRUE);
	}
	g_list_free (browser->priv->fullscreen_controls);
	browser->priv->fullscreen_controls = NULL;

	gtk_alignment_set_padding (GTK_ALIGNMENT (browser->priv->viewer_sidebar_alignment), 0, 0, 0, 0);

	gth_browser_update_sensitivity (browser);
}


void
gth_browser_file_menu_popup (GthBrowser     *browser,
			     GdkEventButton *event)
{
	int button;
	int event_time;

	if (event != NULL) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gth_hook_invoke ("gth-browser-file-popup-before", browser);
	gtk_ui_manager_ensure_update (browser->priv->ui);
	gtk_menu_popup (GTK_MENU (browser->priv->file_popup),
			NULL,
			NULL,
			NULL,
			NULL,
			button,
			event_time);
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
			_gth_browser_load_file (browser, NULL, FALSE);
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
