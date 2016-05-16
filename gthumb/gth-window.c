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
#include "gth-window.h"
#include "gtk-utils.h"
#include "main.h"


G_DEFINE_TYPE (GthWindow, gth_window, GTK_TYPE_APPLICATION_WINDOW)


enum  {
	PROP_0,
	PROP_N_PAGES
};


struct _GthWindowPrivate {
	int              n_pages;
	int              current_page;
	GtkWidget       *grid;
	GtkWidget       *notebook;
	GtkWidget       *menubar;
	GtkWidget       *toolbar;
	GtkWidget       *infobar;
	GtkWidget       *statusbar;
	GtkWidget      **toolbars;
	GtkWidget      **contents;
	GthWindowSize   *window_size;
	GtkWindowGroup  *window_group;
};


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

	self->priv->grid = gtk_grid_new ();
	gtk_widget_show (self->priv->grid);
	gtk_container_add (GTK_CONTAINER (self), self->priv->grid);

	self->priv->notebook = gtk_notebook_new ();
	gtk_style_context_remove_class (gtk_widget_get_style_context (self->priv->notebook), GTK_STYLE_CLASS_NOTEBOOK);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (self->priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (self->priv->notebook), FALSE);
	gtk_widget_show (self->priv->notebook);
	gtk_grid_attach (GTK_GRID (self->priv->grid),
			 self->priv->notebook,
			 0, 2,
			 1, 1);

	self->priv->toolbars = g_new0 (GtkWidget *, n_pages);
	self->priv->contents = g_new0 (GtkWidget *, n_pages);

	for (i = 0; i < n_pages; i++) {
		GtkWidget *page;

		page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (page);
		gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
					  page,
					  NULL);

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
	g_free (window->priv->window_size);
	g_object_unref (window->priv->window_group);

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
	gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), page);

	for (i = 0; i < window->priv->n_pages; i++)
		if (i == page)
			gtk_widget_show (window->priv->contents[i]);
		else
			gtk_widget_hide (window->priv->contents[i]);
}


static void
gth_window_realize (GtkWidget *widget)
{
	GdkScreen      *screen;
	GBytes         *bytes;
	gconstpointer   css_data;
	gsize           css_data_size;
	GtkCssProvider *css_provider;
	GError         *error = NULL;

	GTK_WIDGET_CLASS (gth_window_parent_class)->realize (widget);

	screen = gtk_widget_get_screen (widget);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_for_screen (screen), GTHUMB_ICON_DIR);

	bytes = g_resources_lookup_data ("/org/gnome/gThumb/resources/gthumb.css", 0, &error);
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
	gtk_style_context_add_provider_for_screen (screen,
						   GTK_STYLE_PROVIDER (css_provider),
						   GTK_STYLE_PROVIDER_PRIORITY_THEME);

	g_object_unref (css_provider);
}


static void
gth_window_class_init (GthWindowClass *klass)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (klass, sizeof (GthWindowPrivate));

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
}


static void
gth_window_init (GthWindow *window)
{
	window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, GTH_TYPE_WINDOW, GthWindowPrivate);
	window->priv->grid = NULL;
	window->priv->contents = NULL;
	window->priv->n_pages = 0;
	window->priv->current_page = GTH_WINDOW_PAGE_UNDEFINED;
	window->priv->menubar = NULL;
	window->priv->toolbar = NULL;
	window->priv->infobar = NULL;
	window->priv->statusbar = NULL;
	window->priv->window_group = gtk_window_group_new ();
	gtk_window_group_add_window (window->priv->window_group, GTK_WINDOW (window));

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
		position = 3;
		break;
	case GTH_WINDOW_STATUSBAR:
		window->priv->statusbar = child;
		position = 4;
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
