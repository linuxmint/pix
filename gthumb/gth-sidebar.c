/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include "gth-multipage.h"
#include "gth-sidebar.h"
#include "gth-toolbox.h"
#include "gtk-utils.h"


#define GTH_SIDEBAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_SIDEBAR, GthSidebarPrivate))


enum {
	GTH_SIDEBAR_PAGE_PROPERTIES,
	GTH_SIDEBAR_PAGE_TOOLS
};


struct _GthSidebarPrivate {
	GtkWidget   *properties;
	GtkWidget   *toolbox;
	gboolean    *dirty;
	GthFileData *file_data;
};


G_DEFINE_TYPE (GthSidebar, gth_sidebar, GTK_TYPE_NOTEBOOK)


static void
gth_sidebar_finalize (GObject *object)
{
	GthSidebar *sidebar = GTH_SIDEBAR (object);

	g_free (sidebar->priv->dirty);
	_g_object_unref (sidebar->priv->file_data);

	G_OBJECT_CLASS (gth_sidebar_parent_class)->finalize (object);
}


static gboolean
_gth_sidebar_properties_visible (GthSidebar *sidebar)
{
	return (gtk_widget_get_mapped (GTK_WIDGET (sidebar->priv->properties))
		&& (gtk_notebook_get_current_page (GTK_NOTEBOOK (sidebar)) == GTH_SIDEBAR_PAGE_PROPERTIES));
}


static void
_gth_sidebar_update_current_child (GthSidebar *sidebar)
{
	int current;

	if (! _gth_sidebar_properties_visible (sidebar))
		return;

	current = gth_multipage_get_current (GTH_MULTIPAGE (sidebar->priv->properties));
	if (sidebar->priv->dirty == NULL)
		return;

	if (sidebar->priv->dirty[current]) {
		GList     *children;
		GtkWidget *current_child;

		children = gth_multipage_get_children (GTH_MULTIPAGE (sidebar->priv->properties));
		current_child = g_list_nth_data (children, current);

		sidebar->priv->dirty[current] = FALSE;
		gth_property_view_set_file (GTH_PROPERTY_VIEW (current_child), sidebar->priv->file_data);
	}
}


static void
gth_sidebar_class_init (GthSidebarClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthSidebarPrivate));

	object_class = (GObjectClass *) klass;
	object_class->finalize = gth_sidebar_finalize;
}


static void
gth_sidebar_init (GthSidebar *sidebar)
{
	sidebar->priv = GTH_SIDEBAR_GET_PRIVATE (sidebar);
	sidebar->priv->dirty = NULL;
	sidebar->priv->file_data = NULL;

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sidebar), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (sidebar), FALSE);
}


static void
_gth_sidebar_add_property_views (GthSidebar *sidebar)
{
	GArray *children;
	int     i;

	children = gth_main_get_type_set ("file-properties");
	if (children == NULL)
		return;

	for (i = 0; i < children->len; i++) {
		GType      child_type;
		GtkWidget *child;

		child_type = g_array_index (children, GType, i);
		child = g_object_new (child_type, NULL);
		gth_multipage_add_child (GTH_MULTIPAGE (sidebar->priv->properties), GTH_MULTIPAGE_CHILD (child));
	}
	gth_multipage_set_current (GTH_MULTIPAGE (sidebar->priv->properties), 0);
}


static void
_gth_sidebar_construct (GthSidebar *sidebar,
		        const char *name)
{
	sidebar->priv->properties = gth_multipage_new ();
	gtk_widget_show (sidebar->priv->properties);
	gtk_notebook_append_page (GTK_NOTEBOOK (sidebar), sidebar->priv->properties, NULL);

	g_signal_connect_swapped (sidebar->priv->properties,
			  	  "map",
			  	  G_CALLBACK (_gth_sidebar_update_current_child),
			  	  sidebar);
	g_signal_connect_swapped (sidebar->priv->properties,
			  	  "changed",
			  	  G_CALLBACK (_gth_sidebar_update_current_child),
			  	  sidebar);

	sidebar->priv->toolbox = gth_toolbox_new (name);
	gtk_widget_show (sidebar->priv->toolbox);
	gtk_notebook_append_page (GTK_NOTEBOOK (sidebar), sidebar->priv->toolbox, NULL);
}


GtkWidget *
gth_sidebar_new (const char *name)
{
	GthSidebar *sidebar;

	sidebar = g_object_new (GTH_TYPE_SIDEBAR, NULL);
	_gth_sidebar_construct (sidebar, name);
	_gth_sidebar_add_property_views (sidebar);

	return (GtkWidget *) sidebar;
}


GtkWidget *
gth_sidebar_get_toolbox (GthSidebar *sidebar)
{
	return sidebar->priv->toolbox;
}


void
gth_sidebar_set_file (GthSidebar  *sidebar,
		      GthFileData *file_data)
{
	GList *children;
	int    current;
	GList *scan;
	int    i;

	if ((file_data == NULL) || ! g_file_info_get_attribute_boolean (file_data->info, "gth::file::is-modified"))
		gth_toolbox_deactivate_tool (GTH_TOOLBOX (sidebar->priv->toolbox));

	children = gth_multipage_get_children (GTH_MULTIPAGE (sidebar->priv->properties));
	current = gth_multipage_get_current (GTH_MULTIPAGE (sidebar->priv->properties));

	_g_object_unref (sidebar->priv->file_data);
	sidebar->priv->file_data = gth_file_data_dup (file_data);

	g_free (sidebar->priv->dirty);
	sidebar->priv->dirty = g_new0 (gboolean, g_list_length (children));

	for (scan = children, i = 0; scan; scan = scan->next, i++) {
		GtkWidget *child = scan->data;

		if (! GTH_IS_PROPERTY_VIEW (child)) {
			sidebar->priv->dirty[i] = FALSE;
			continue;
		}

		if (! _gth_sidebar_properties_visible (sidebar) || (i != current)) {
			sidebar->priv->dirty[i] = TRUE;
			continue;
		}

		sidebar->priv->dirty[i] = FALSE;
		gth_property_view_set_file (GTH_PROPERTY_VIEW (child), sidebar->priv->file_data);
	}
}


void
gth_sidebar_show_properties (GthSidebar *sidebar)
{
	gtk_notebook_set_current_page (GTK_NOTEBOOK (sidebar), GTH_SIDEBAR_PAGE_PROPERTIES);
}


void
gth_sidebar_show_tools (GthSidebar *sidebar)
{
	gtk_notebook_set_current_page (GTK_NOTEBOOK (sidebar), GTH_SIDEBAR_PAGE_TOOLS);
}


gboolean
gth_sidebar_tool_is_active (GthSidebar *sidebar)
{
	return gth_toolbox_tool_is_active (GTH_TOOLBOX (sidebar->priv->toolbox));
}


void
gth_sidebar_deactivate_tool (GthSidebar *sidebar)
{
	gth_toolbox_deactivate_tool (GTH_TOOLBOX (sidebar->priv->toolbox));
}


void
gth_sidebar_update_sensitivity (GthSidebar *sidebar)
{
	gth_toolbox_update_sensitivity (GTH_TOOLBOX (sidebar->priv->toolbox));
}


/* -- gth_property_view -- */


G_DEFINE_INTERFACE (GthPropertyView, gth_property_view, 0)


static void
gth_property_view_default_init (GthPropertyViewInterface *iface)
{
	/* void */
}


void
gth_property_view_set_file (GthPropertyView *self,
			    GthFileData     *file_data)
{
	GTH_PROPERTY_VIEW_GET_INTERFACE (self)->set_file (self, file_data);
}
