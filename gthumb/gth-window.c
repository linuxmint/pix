/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-window.h"
#include "gth-window-title.h"
#include "gtk-utils.h"
#include "main.h"


enum  {
	PROP_0,
	PROP_N_PAGES,
	PROP_USE_HEADER_BAR
};


struct _GthWindowPrivate {
	int              n_pages;
	gboolean         use_header_bar;
	int              current_page;
	GtkWidget       *overlay;
	GtkWidget       *grid;
	GtkWidget       *stack;
	GtkWidget       *headerbar_container;
	GtkWidget       *headerbar;
	GtkWidget       *title;
	GtkWidget       *menubar;
	GtkWidget       *toolbar;
	GtkWidget       *infobar;
	GtkWidget       *statusbar;
	GtkWidget      **toolbars;
	GtkWidget      **contents;
	GtkWidget      **pages;
	GthWindowSize   *window_size;
	GtkWindowGroup  *window_group;
	GtkAccelGroup   *accel_group;
	GHashTable      *shortcuts;
	GPtrArray       *shortcuts_v;
	GHashTable      *shortcut_groups;
};


G_DEFINE_TYPE_WITH_CODE (GthWindow,
			 gth_window,
			 GTK_TYPE_APPLICATION_WINDOW,
			 G_ADD_PRIVATE (GthWindow))


static gboolean
overlay_get_child_position_cb (GtkOverlay   *overlay,
			       GtkWidget    *widget,
			       GdkRectangle *allocation,
			       gpointer      user_data)
{
	GtkAllocation main_alloc;

	gtk_widget_get_allocation (gtk_bin_get_child (GTK_BIN (overlay)), &main_alloc);

	allocation->x = 0;
	allocation->y = 0;
	allocation->width = main_alloc.width;
	gtk_widget_get_preferred_height (widget, NULL, &allocation->height);

	return TRUE;
}


static void
gth_window_set_n_pages (GthWindow *self,
			int        n_pages)
{
	int i;

	if (self->priv->n_pages != 0) {
		g_critical ("The number of pages of a GthWindow can be set only once.");
		return;
	}

	self->priv->n_pages = n_pages;

	self->priv->overlay = gtk_overlay_new ();
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->overlay), "window-overlay");
	gtk_widget_show (self->priv->overlay);
	gtk_container_add (GTK_CONTAINER (self), self->priv->overlay);

	g_signal_connect (self->priv->overlay,
			  "get-child-position",
			  G_CALLBACK (overlay_get_child_position_cb),
			  self);

	self->priv->grid = gtk_grid_new ();
	gtk_widget_show (self->priv->grid);
	gtk_container_add (GTK_CONTAINER (self->priv->overlay), self->priv->grid);

	self->priv->stack = gtk_stack_new ();
	gtk_stack_set_transition_type (GTK_STACK (self->priv->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
	gtk_widget_show (self->priv->stack);
	gtk_grid_attach (GTK_GRID (self->priv->grid),
			 self->priv->stack,
			 0, 2,
			 1, 1);

	self->priv->toolbars = g_new0 (GtkWidget *, n_pages);
	self->priv->contents = g_new0 (GtkWidget *, n_pages);
	self->priv->pages = g_new0 (GtkWidget *, n_pages);

	for (i = 0; i < n_pages; i++) {
		GtkWidget *page;

		self->priv->pages[i] = page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (page);
		gtk_container_add (GTK_CONTAINER (self->priv->stack), page);

		self->priv->toolbars[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (self->priv->toolbars[i]);
		gtk_box_pack_start (GTK_BOX (page), self->priv->toolbars[i], FALSE, FALSE, 0);

		self->priv->contents[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_hide (self->priv->contents[i]);
		gtk_box_pack_start (GTK_BOX (page), self->priv->contents[i], TRUE, TRUE, 0);
	}

	self->priv->window_size = g_new0 (GthWindowSize, n_pages);
	for (i = 0; i < n_pages; i++)
		self->priv->window_size[i].saved = FALSE;
}


static void
_gth_window_add_header_bar (GthWindow *self)
{
	self->priv->headerbar = gtk_header_bar_new ();
	gtk_widget_show (self->priv->headerbar);
	gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self->priv->headerbar), TRUE);

	g_object_add_weak_pointer (G_OBJECT (self->priv->headerbar), (gpointer *) &self->priv->headerbar);

	{
		gboolean  shell_shows_app_menu;
		char     *decoration_layout;

		g_object_get (gtk_settings_get_default (),
			      "gtk-shell-shows-app-menu", &shell_shows_app_menu,
			      "gtk-decoration-layout", &decoration_layout,
			      NULL);
		if (! shell_shows_app_menu && ((decoration_layout == NULL) || (strstr (decoration_layout, "menu") == NULL))) {
			gboolean  left_part_is_empty;
			char     *new_layout;

			/* add 'menu' to the left */

			left_part_is_empty = (decoration_layout == NULL) || (decoration_layout[0] == '\0') || (decoration_layout[0] == ':');
			new_layout = g_strconcat ("menu", (left_part_is_empty ? "" : ","), decoration_layout, NULL);
			gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (self->priv->headerbar), new_layout);

			g_free (new_layout);
		}

		g_free (decoration_layout);
	}

	self->priv->title = gth_window_title_new ();
	gtk_widget_show (self->priv->title);
	gtk_header_bar_set_custom_title (GTK_HEADER_BAR (self->priv->headerbar), self->priv->title);

	self->priv->headerbar_container = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (self->priv->headerbar_container);
	gtk_box_pack_start (GTK_BOX (self->priv->headerbar_container), self->priv->headerbar, TRUE, TRUE, 0);

	gtk_window_set_titlebar (GTK_WINDOW (self), self->priv->headerbar_container);
}


static void
gth_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GthWindow *self;

	self = GTH_WINDOW (object);

	switch (property_id) {
	case PROP_N_PAGES:
		gth_window_set_n_pages (self, g_value_get_int (value));
		break;
	case PROP_USE_HEADER_BAR:
		self->priv->use_header_bar = g_value_get_boolean (value);
		if (self->priv->use_header_bar)
			_gth_window_add_header_bar (self);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_window_get_property (GObject    *object,
		         guint       property_id,
		         GValue     *value,
		         GParamSpec *pspec)
{
	GthWindow *self;

        self = GTH_WINDOW (object);

	switch (property_id) {
	case PROP_N_PAGES:
		g_value_set_int (value, self->priv->n_pages);
		break;
	case PROP_USE_HEADER_BAR:
		g_value_set_boolean (value, self->priv->use_header_bar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_window_finalize (GObject *object)
{
	GthWindow *window = GTH_WINDOW (object);

	g_free (window->priv->toolbars);
	g_free (window->priv->contents);
	g_free (window->priv->pages);
	g_free (window->priv->window_size);
	g_object_unref (window->priv->window_group);
	g_object_unref (window->priv->accel_group);
	g_hash_table_unref (window->priv->shortcuts);
	g_ptr_array_unref (window->priv->shortcuts_v);
	g_hash_table_unref (window->priv->shortcut_groups);

	G_OBJECT_CLASS (gth_window_parent_class)->finalize (object);
}


static gboolean
gth_window_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
	gth_window_close ((GthWindow*) widget);
	return TRUE;
}


static void
gth_window_real_close (GthWindow *window)
{
	/* virtual */
}


static void
gth_window_real_set_current_page (GthWindow *window,
				  int        page)
{
	int i;

	if (window->priv->current_page == page)
		return;

	window->priv->current_page = page;
	gtk_stack_set_visible_child (GTK_STACK (window->priv->stack), window->priv->pages[page]);

	for (i = 0; i < window->priv->n_pages; i++)
		if (i == page)
			gtk_widget_show (window->priv->contents[i]);
		else
			gtk_widget_hide (window->priv->contents[i]);
}


static void
_gth_window_add_css_provider (GtkWidget  *widget,
			      const char *path)
{
	GBytes         *bytes;
	gconstpointer   css_data;
	gsize           css_data_size;
	GtkCssProvider *css_provider;
	GError         *error = NULL;


	bytes = g_resources_lookup_data (path, 0, &error);
	if (bytes == NULL) {
		g_warning ("%s", error->message);
		g_error_free (error);
		return;
	}

	css_data = g_bytes_get_data (bytes, &css_data_size);
	css_provider = gtk_css_provider_new ();
	if (! gtk_css_provider_load_from_data (css_provider, css_data, css_data_size, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);
	}
	gtk_style_context_add_provider_for_screen (gtk_widget_get_screen (widget),
						   GTK_STYLE_PROVIDER (css_provider),
						   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref (css_provider);
	g_bytes_unref (bytes);
}


static void
gth_window_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (gth_window_parent_class)->realize (widget);

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget)), GTHUMB_ICON_DIR);

	_gth_window_add_css_provider (widget, "/org/gnome/gThumb/resources/gthumb.css");
	if ((gtk_major_version >= 3) && (gtk_minor_version >= 20))
		_gth_window_add_css_provider (widget, "/org/gnome/gThumb/resources/gthumb-gtk320.css");
	else if ((gtk_major_version >= 3) && (gtk_minor_version >= 14))
		_gth_window_add_css_provider (widget, "/org/gnome/gThumb/resources/gthumb-gtk314.css");
	else if ((gtk_major_version >= 3) && (gtk_minor_version >= 10))
		_gth_window_add_css_provider (widget, "/org/gnome/gThumb/resources/gthumb-gtk312.css");
}


static void
gth_window_class_init (GthWindowClass *klass)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->set_property = gth_window_set_property;
	gobject_class->get_property = gth_window_get_property;
	gobject_class->finalize = gth_window_finalize;

	widget_class = (GtkWidgetClass*) klass;
	widget_class->delete_event = gth_window_delete_event;
	widget_class->realize = gth_window_realize;

	klass->close = gth_window_real_close;
	klass->set_current_page = gth_window_real_set_current_page;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 PROP_N_PAGES,
					 g_param_spec_int ("n-pages",
							   "n-pages",
							   "n-pages",
							   0,
							   G_MAXINT,
							   1,
							   G_PARAM_READWRITE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 PROP_USE_HEADER_BAR,
					 g_param_spec_boolean ("use-header-bar",
							       "use-header-bar",
							       "use-header-bar",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
gth_window_init (GthWindow *window)
{
	window->priv = gth_window_get_instance_private (window);
	window->priv->grid = NULL;
	window->priv->contents = NULL;
	window->priv->pages = NULL;
	window->priv->n_pages = 0;
	window->priv->current_page = GTH_WINDOW_PAGE_UNDEFINED;
	window->priv->menubar = NULL;
	window->priv->toolbar = NULL;
	window->priv->infobar = NULL;
	window->priv->statusbar = NULL;
	window->priv->headerbar = NULL;
	window->priv->use_header_bar = FALSE;

	window->priv->window_group = gtk_window_group_new ();
	gtk_window_group_add_window (window->priv->window_group, GTK_WINDOW (window));

	window->priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (window), window->priv->accel_group);

	window->priv->shortcuts = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	window->priv->shortcuts_v = g_ptr_array_new_with_free_func ((GDestroyNotify) gth_shortcut_free);

	window->priv->shortcut_groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_ptr_array_unref);

	gtk_window_set_application (GTK_WINDOW (window), Main_Application);
}


void
gth_window_close (GthWindow *window)
{
	GTH_WINDOW_GET_CLASS (window)->close (window);
}


void
gth_window_attach (GthWindow     *window,
		   GtkWidget     *child,
		   GthWindowArea  area)
{
	int position;

	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	switch (area) {
	case GTH_WINDOW_MENUBAR:
		window->priv->menubar = child;
		position = 0;
		break;
	case GTH_WINDOW_TOOLBAR:
		window->priv->toolbar = child;
		position = 1;
		break;
	case GTH_WINDOW_INFOBAR:
		window->priv->infobar = child;
		position = 4;
		break;
	case GTH_WINDOW_STATUSBAR:
		window->priv->statusbar = child;
		position = 3;
		break;
	default:
		return;
	}

	gtk_widget_set_vexpand (child, FALSE);
	gtk_grid_attach (GTK_GRID (window->priv->grid),
			  child,
			  0, position,
			  1, 1);
}


void
gth_window_attach_toolbar (GthWindow *window,
			   int        page,
			   GtkWidget *child)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	_gtk_container_remove_children (GTK_CONTAINER (window->priv->toolbars[page]), NULL, NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (child), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
	gtk_widget_set_hexpand (child, TRUE);
	gtk_container_add (GTK_CONTAINER (window->priv->toolbars[page]), child);
}


void
gth_window_attach_content (GthWindow *window,
			   int        page,
			   GtkWidget *child)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	_gtk_container_remove_children (GTK_CONTAINER (window->priv->contents[page]), NULL, NULL);
	gtk_widget_set_hexpand (child, TRUE);
	gtk_widget_set_vexpand (child, TRUE);
	gtk_container_add (GTK_CONTAINER (window->priv->contents[page]), child);
}


void
gth_window_set_current_page (GthWindow *window,
			     int        page)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	GTH_WINDOW_GET_CLASS (window)->set_current_page (window, page);
}


int
gth_window_get_current_page (GthWindow *window)
{
	return window->priv->current_page;
}


static void
hide_widget (GtkWidget *widget)
{
	if (widget != NULL)
		gtk_widget_hide (widget);
}


static void
show_widget (GtkWidget *widget)
{
	if (widget != NULL)
		gtk_widget_show (widget);
}


void
gth_window_show_only_content (GthWindow *window,
			      gboolean   only_content)
{
	int i;

	if (only_content) {
		for (i = 0; i < window->priv->n_pages; i++)
			hide_widget (window->priv->toolbars[i]);
		hide_widget (window->priv->menubar);
		hide_widget (window->priv->toolbar);
		hide_widget (window->priv->statusbar);
	}
	else {
		for (i = 0; i < window->priv->n_pages; i++)
			show_widget (window->priv->toolbars[i]);
		show_widget (window->priv->menubar);
		show_widget (window->priv->toolbar);
		show_widget (window->priv->statusbar);
	}
}


GtkWidget *
gth_window_get_area (GthWindow     *window,
		     GthWindowArea  area)
{
	switch (area) {
	case GTH_WINDOW_MENUBAR:
		return window->priv->menubar;
		break;
	case GTH_WINDOW_TOOLBAR:
		return window->priv->toolbar;
		break;
	case GTH_WINDOW_INFOBAR:
		return window->priv->infobar;
		break;
	case GTH_WINDOW_STATUSBAR:
		return window->priv->statusbar;
		break;
	default:
		break;
	}

	return NULL;
}


void
gth_window_add_overlay (GthWindow *window,
			GtkWidget *widget)
{
	gtk_overlay_add_overlay (GTK_OVERLAY (window->priv->overlay), widget);
}


void
gth_window_set_header_bar (GthWindow *window,
			   GtkWidget *header_bar)
{
	if (window->priv->headerbar != header_bar) {
		if (window->priv->headerbar != NULL)
			gtk_widget_destroy (window->priv->headerbar);
		window->priv->headerbar = header_bar;
	}
	gtk_widget_show (window->priv->headerbar);
	gtk_box_pack_start (GTK_BOX (window->priv->headerbar_container), window->priv->headerbar, TRUE, TRUE, 0);
}


GtkWidget *
gth_window_get_header_bar (GthWindow *window)
{
	return window->priv->headerbar;
}


void
gth_window_save_page_size (GthWindow *window,
			   int        page,
			   int        width,
			   int        height)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	window->priv->window_size[page].width = width;
	window->priv->window_size[page].height = height;
	window->priv->window_size[page].saved = TRUE;
}


void
gth_window_apply_saved_size (GthWindow *window,
			     int        page)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	if (! window->priv->window_size[page].saved)
		return;

	gtk_window_resize (GTK_WINDOW (window),
			   window->priv->window_size[page].width,
			   window->priv->window_size[page].height);
}


void
gth_window_clear_saved_size (GthWindow *window,
			     int        page)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	window->priv->window_size[page].saved = FALSE;
}


gboolean
gth_window_get_page_size (GthWindow *window,
			  int        page,
			  int       *width,
			  int       *height)
{
	g_return_val_if_fail (window != NULL, FALSE);
	g_return_val_if_fail (GTH_IS_WINDOW (window), FALSE);
	g_return_val_if_fail (page >= 0 && page < window->priv->n_pages, FALSE);

	if (! window->priv->window_size[page].saved)
		return FALSE;

	if (width != NULL)
		*width = window->priv->window_size[page].width;
	if (height != NULL)
		*height = window->priv->window_size[page].height;

	return TRUE;
}


void
gth_window_set_title (GthWindow  *window,
		      const char *title,
		      GList	 *emblems)
{
	if (window->priv->use_header_bar) {
		gth_window_title_set_title (GTH_WINDOW_TITLE (window->priv->title), title);
		gth_window_title_set_emblems (GTH_WINDOW_TITLE (window->priv->title), emblems);
	}
	else
		gtk_window_set_title (GTK_WINDOW (window), title);
}


GtkAccelGroup *
gth_window_get_accel_group (GthWindow *window)
{
	if (window->priv->accel_group == NULL) {
		window->priv->accel_group = gtk_accel_group_new ();
		gtk_window_add_accel_group (GTK_WINDOW (window), window->priv->accel_group);
	}

	return window->priv->accel_group;
}


static void
_gth_window_add_shortcut (GthWindow   *window,
			  GthShortcut *shortcut)
{
	g_ptr_array_add (window->priv->shortcuts_v, shortcut);
	g_hash_table_insert (window->priv->shortcuts,
			     g_strdup (shortcut->detailed_action),
			     shortcut);
}


static void
_gth_window_remove_shortcut (GthWindow   *window,
			     GthShortcut *shortcut)
{
	g_hash_table_remove (window->priv->shortcuts, shortcut->detailed_action);
	g_ptr_array_remove (window->priv->shortcuts_v, shortcut);
}


void
gth_window_add_accelerators (GthWindow			*window,
			     const GthAccelerator	*accelerators,
			     int		 	 n_accelerators)
{
	GtkAccelGroup *accel_group;
	int            i;

	accel_group = gth_window_get_accel_group (window);
	for (i = 0; i < n_accelerators; i++) {
		const GthAccelerator *acc = accelerators + i;
		GthShortcut          *shortcut;

		_gtk_window_add_accelerator_for_action (GTK_WINDOW (window),
							accel_group,
							acc->action_name,
							acc->accelerator,
							NULL);

		shortcut = gth_shortcut_new (acc->action_name, NULL);
		shortcut->context = GTH_SHORTCUT_CONTEXT_INTERNAL | GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER;
		shortcut->category = GTH_SHORTCUT_CATEGORY_HIDDEN;
		gth_shortcut_set_accelerator (shortcut, acc->accelerator);
		_gth_window_add_shortcut (window, shortcut);
	}
}


void
gth_window_enable_action (GthWindow  *window,
			  const char *action_name,
			  gboolean    enabled)
{
	GAction *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_object_set (action, "enabled", enabled, NULL);
}


gboolean
gth_window_get_action_state (GthWindow  *window,
			     const char *action_name)
{
	GAction  *action;
	GVariant *state;
	gboolean  value;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_return_val_if_fail (action != NULL, FALSE);
	state = g_action_get_state (action);
	value = g_variant_get_boolean (state);

	g_variant_unref (state);

	return value;
}


void
gth_window_change_action_state (GthWindow  *window,
			        const char *action_name,
			        gboolean    value)
{
	GAction  *action;
	GVariant *old_state;
	GVariant *new_state;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_return_if_fail (action != NULL);

	old_state = g_action_get_state (action);
	new_state = g_variant_ref_sink (g_variant_new_boolean (value));
	if ((old_state == NULL) || ! g_variant_equal (old_state, new_state))
		g_action_change_state (action, new_state);

	if (old_state != NULL)
		g_variant_unref (old_state);
	g_variant_unref (new_state);
}


void
gth_window_activate_action (GthWindow	*window,
			    const char	*action_name,
			    GVariant    *parameter)
{
	GAction *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_return_if_fail (action != NULL);

	g_action_activate (action, parameter);
}


void
gth_window_add_viewer_shortcuts (GthWindow         *window,
				 const char        *viewer_context,
				 const GthShortcut *shortcuts,
				 int                n_shortcuts)
{
	int i;

	for (i = 0; i < n_shortcuts; i++) {
		const GthShortcut *shortcut = shortcuts + i;
		GthShortcut       *new_shortcut;

		new_shortcut = gth_shortcut_dup (shortcut);
		gth_shortcut_set_accelerator (new_shortcut, shortcut->default_accelerator);
		gth_shortcut_set_viewer_context (new_shortcut, viewer_context);

		_gth_window_add_shortcut (window, new_shortcut);
	}
}


void
gth_window_add_shortcuts (GthWindow         *window,
			  const GthShortcut *shortcuts,
			  int                n_shortcuts)
{
	gth_window_add_viewer_shortcuts (window, NULL, shortcuts, n_shortcuts);
}


GPtrArray *
gth_window_get_shortcuts (GthWindow *window)
{
	g_return_val_if_fail (GTH_IS_WINDOW (window), NULL);

	return window->priv->shortcuts_v;
}


GthShortcut *
gth_window_get_shortcut (GthWindow  *window,
			 const char *detailed_action)
{
	g_return_val_if_fail (GTH_IS_WINDOW (window), NULL);

	return g_hash_table_lookup (window->priv->shortcuts, detailed_action);
}


static int
sort_shortcuts_by_category (gconstpointer a,
			    gconstpointer b)
{
	const GthShortcut   *sa = * (GthShortcut **) a;
	const GthShortcut   *sb = * (GthShortcut **) b;
	int                  result;
	GthShortcutCategory *cat_a;
	GthShortcutCategory *cat_b;

	result = 0;
	cat_a = gth_main_get_shortcut_category (sa->category);
	cat_b = gth_main_get_shortcut_category (sb->category);
	if ((cat_a != NULL) && (cat_b != NULL)) {
		if (cat_a->sort_order < cat_b->sort_order)
			result = -1;
		else if (cat_a->sort_order > cat_b->sort_order)
			result = 1;
	}
	if (result == 0)
		result = g_strcmp0 (sa->description, sb->description);

	return result;
}


GPtrArray *
gth_window_get_shortcuts_by_category (GthWindow *window)
{
	g_return_val_if_fail (GTH_IS_WINDOW (window), NULL);

	g_ptr_array_sort (window->priv->shortcuts_v, sort_shortcuts_by_category);
	return window->priv->shortcuts_v;
}


gboolean
gth_window_activate_shortcut (GthWindow       *window,
			      int              context,
			      const char      *viewer_context,
			      guint            keycode,
			      GdkModifierType  modifiers)
{
	gboolean     activated = FALSE;
	GthShortcut *shortcut;

	modifiers = modifiers & gtk_accelerator_get_default_mod_mask ();
	shortcut = gth_shortcut_array_find (window->priv->shortcuts_v, context, viewer_context, keycode, modifiers);
	if (shortcut != NULL) {
		GAction *action;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_INTERNAL) != 0)
			return FALSE;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_DOC) != 0)
			return FALSE;

		action = g_action_map_lookup_action (G_ACTION_MAP (window), shortcut->action_name);
		if (action != NULL) {
			g_action_activate (action, shortcut->action_parameter);
			activated = TRUE;
		}
	}

	return activated;
}


void
gth_window_load_shortcuts (GthWindow *window)
{
	g_return_if_fail (GTH_IS_WINDOW (window));

	gth_shortcuts_load_from_file (window->priv->shortcuts_v,
				      window->priv->shortcuts,
				      NULL);
}


void
gth_window_add_removable_shortcut (GthWindow   *window,
				   const char  *group_name,
				   GthShortcut *shortcut)
{
	GPtrArray   *shortcuts_v;
	GthShortcut *old_shortcut;
	GthShortcut *new_shortcut;

	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (group_name != NULL);
	g_return_if_fail (shortcut != NULL);
	g_return_if_fail (shortcut->detailed_action != NULL);

	/* create the group if it doesn't exist. */

	shortcuts_v = g_hash_table_lookup (window->priv->shortcut_groups, group_name);
	if (shortcuts_v == NULL) {
		shortcuts_v = g_ptr_array_new ();
		g_hash_table_insert (window->priv->shortcut_groups,
				     g_strdup (group_name),
				     shortcuts_v);
	}

	/* remove the old shortcut */

	old_shortcut = g_hash_table_lookup (window->priv->shortcuts, shortcut->detailed_action);
	if (old_shortcut != NULL) {
		g_ptr_array_remove (shortcuts_v, old_shortcut);
		_gth_window_remove_shortcut (window, old_shortcut);
	}

	/* add the new shortcut */

	new_shortcut = gth_shortcut_dup (shortcut);
	_gth_window_add_shortcut (window, new_shortcut);
	g_ptr_array_add (shortcuts_v, new_shortcut);
}


void
gth_window_remove_shortcuts (GthWindow  *window,
			     const char *group_name)
{
	GPtrArray *shortcuts_v;
	int        i;

	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (group_name != NULL);

	shortcuts_v = g_hash_table_lookup (window->priv->shortcut_groups, group_name);
	if (shortcuts_v == NULL)
		return;

	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);
		_gth_window_remove_shortcut (window, shortcut);
	}

	g_hash_table_remove (window->priv->shortcut_groups, group_name);
}


GHashTable *
gth_window_get_accels_for_group (GthWindow  *window,
				 const char *group_name)
{
	GHashTable *map;
	GPtrArray  *shortcuts_v;
	int         i;

	g_return_val_if_fail (GTH_IS_WINDOW (window), NULL);
	g_return_val_if_fail (group_name != NULL, NULL);

	map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	shortcuts_v = g_hash_table_lookup (window->priv->shortcut_groups, group_name);
	if (shortcuts_v == NULL)
		return map;

	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);
		g_hash_table_insert (map, g_strdup (shortcut->detailed_action), g_strdup (shortcut->accelerator));
	}

	return map;
}


gboolean
gth_window_can_change_shortcut (GthWindow         *window,
				const char        *detailed_action,
				int                context,
				const char        *viewer_context,
				guint              keycode,
				GdkModifierType    modifiers,
				GtkWindow         *parent)
{
	GthShortcut *shortcut;

	if (window == NULL)
		return TRUE;

	shortcut = gth_shortcut_array_find (gth_window_get_shortcuts (window ),
					    context,
					    viewer_context,
					    keycode,
					    modifiers);

	if (shortcut == NULL)
		return TRUE;

	if (g_strcmp0 (shortcut->detailed_action, detailed_action) == 0)
		return FALSE;

	if (gth_shortcut_customizable (shortcut)) {
		char      *label;
		char      *msg;
		GtkWidget *dialog;
		gboolean   reassign;

		label = gtk_accelerator_get_label (keycode, modifiers);
		msg = g_strdup_printf (_("The key combination «%s» is already assigned to the action «%s».  Do you want to reassign it to this action instead?"),
				       label,
				       shortcut->description);

		dialog = _gtk_yesno_dialog_new (parent,
						GTK_DIALOG_MODAL,
						msg,
						_GTK_LABEL_CANCEL,
						_("Reassign"));

		reassign = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
		gtk_widget_destroy (GTK_WIDGET (dialog));

		g_free (msg);
		g_free (label);

		if (! reassign)
			return FALSE;
	}
	else {
		char      *label;
		char      *msg;
		GtkWidget *dialog;

		label = gtk_accelerator_get_label (keycode, modifiers);
		if (shortcut->description != NULL)
			msg = g_strdup_printf (_("The key combination «%s» is already assigned to the action «%s» and cannot be changed."),
					       label,
					       shortcut->description);
		else
			msg = g_strdup_printf (_("The key combination «%s» is already assigned and cannot be changed."),
					       label);

		dialog = _gtk_message_dialog_new (parent,
						  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						  _GTK_ICON_NAME_DIALOG_ERROR,
						  NULL,
						  msg,
						  _GTK_LABEL_OK, GTK_RESPONSE_OK,
						  NULL);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (GTK_WIDGET (dialog));

		g_free (msg);
		g_free (label);

		return FALSE;
	}

	return TRUE;
}


void
gth_window_copy_shortcuts (GthWindow *to_window,
			   GthWindow *from_window,
			   int        context)
{
	int i;

	for (i = 0; i < from_window->priv->shortcuts_v->len; i++) {
		const GthShortcut *shortcut = g_ptr_array_index (from_window->priv->shortcuts_v, i);
		GthShortcut       *new_shortcut;

		if ((shortcut->context & context) == 0)
			continue;

		new_shortcut = gth_shortcut_dup (shortcut);
		_gth_window_add_shortcut (to_window, new_shortcut);
	}
}
