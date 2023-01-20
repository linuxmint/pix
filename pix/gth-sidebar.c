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
#include "gth-property-view.h"
#include "gth-sidebar.h"
#include "gth-sidebar-section.h"
#include "gth-toolbox.h"
#include "gtk-utils.h"


#define GTH_SIDEBAR_PAGE_PROPERTIES "GthSidebar.Properties"
#define GTH_SIDEBAR_PAGE_TOOLS "GthSidebar.Tools"
#define GTH_SIDEBAR_PAGE_EMPTY "GthSidebar.Empty"
#define MIN_SIDEBAR_WIDTH 365


struct _GthSidebarPrivate {
	GtkWidget   *properties;
	GtkWidget   *toolbox;
	GthFileData *file_data;
	const char  *selected_page;
	GList       *sections;
	int          n_visibles;
};


G_DEFINE_TYPE_WITH_CODE (GthSidebar,
			 gth_sidebar,
			 GTK_TYPE_STACK,
			 G_ADD_PRIVATE (GthSidebar))


static void
gth_sidebar_finalize (GObject *object)
{
	GthSidebar *sidebar = GTH_SIDEBAR (object);

	_g_object_unref (sidebar->priv->file_data);

	G_OBJECT_CLASS (gth_sidebar_parent_class)->finalize (object);
}


static void
gth_sidebar_class_init (GthSidebarClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;
	object_class->finalize = gth_sidebar_finalize;
}


static void
gth_sidebar_init (GthSidebar *sidebar)
{
	sidebar->priv = gth_sidebar_get_instance_private (sidebar);
	sidebar->priv->file_data = NULL;
	sidebar->priv->sections = NULL;
	sidebar->priv->selected_page = GTH_SIDEBAR_PAGE_EMPTY;
	sidebar->priv->n_visibles = 0;
}


#define STATUS_DELIMITER ":"
#define STATUS_EXPANDED  "expanded"
#define STATUS_NOT_EXPANDED  "collapsed"


typedef struct {
	char     *id;
	gboolean  expanded;
} SectionStatus;


static SectionStatus *
section_status_new (const char *str)
{
	SectionStatus  *status;
	char          **strv;

	status = g_new (SectionStatus, 1);
	status->id = NULL;
	status->expanded = TRUE;

	strv = g_strsplit (str, STATUS_DELIMITER, 2);
	if (strv[0] != NULL)
		status->id = g_strdup (strv[0]);
	if (strv[1] != NULL)
		status->expanded = (strcmp (strv[1], STATUS_EXPANDED) == 0);
	g_strfreev (strv);

	return status;
}


static void
section_status_free (SectionStatus *status)
{
	g_free (status->id);
	g_free (status);
}


static GHashTable *
create_section_status_hash (char **sections_status)
{
	GHashTable *status_hash;
	int         i;

	status_hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) section_status_free);
	if (sections_status == NULL)
		return status_hash;

	for (i = 0; sections_status[i] != NULL; i++) {
		SectionStatus *status = section_status_new (sections_status[i]);
		if (status != NULL)
			g_hash_table_insert (status_hash, status->id, status);
	}

	return status_hash;
}


static void
_gth_sidebar_add_sections (GthSidebar  *sidebar,
			   char       **sections_status)
{
	GArray     *children;
	GHashTable *status_hash;
	int         i;

	children = gth_main_get_type_set ("file-properties");
	if (children == NULL)
		return;

	status_hash = create_section_status_hash (sections_status);
	for (i = 0; i < children->len; i++) {
		GType          child_type;
		GtkWidget     *child;
		GtkWidget     *section;
		SectionStatus *status;

		child_type = g_array_index (children, GType, i);
		child = g_object_new (child_type, NULL);
		g_return_if_fail (GTH_IS_PROPERTY_VIEW (child));

		section = gth_sidebar_section_new (GTH_PROPERTY_VIEW (child));
		status = g_hash_table_lookup (status_hash, gth_sidebar_section_get_id (GTH_SIDEBAR_SECTION (section)));
		if (status != NULL) {
			gth_sidebar_section_set_expanded (GTH_SIDEBAR_SECTION (section), status->expanded);
		}

		sidebar->priv->sections = g_list_prepend (sidebar->priv->sections, section);
		gtk_box_pack_start (GTK_BOX (sidebar->priv->properties),
				    section,
				    FALSE,
				    FALSE,
				    0);
	}
	g_hash_table_unref (status_hash);

	sidebar->priv->sections = g_list_reverse (sidebar->priv->sections);
}


static void
_gth_sidebar_construct (GthSidebar *sidebar)
{
	GtkWidget *properties_win;
	GtkWidget *empty_view;
	GtkWidget *icon;

	properties_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (properties_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (properties_win), GTK_SHADOW_NONE);
	gtk_widget_set_vexpand (properties_win, TRUE);
	gtk_widget_show (properties_win);
	gtk_stack_add_named (GTK_STACK (sidebar), properties_win, GTH_SIDEBAR_PAGE_PROPERTIES);

	sidebar->priv->properties = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_size_request (GTK_WIDGET (sidebar->priv->properties), MIN_SIDEBAR_WIDTH, -1);
	gtk_widget_show (sidebar->priv->properties);
	gtk_container_add (GTK_CONTAINER (properties_win), sidebar->priv->properties);

	sidebar->priv->toolbox = gth_toolbox_new ("file-tools");
	gtk_style_context_add_class (gtk_widget_get_style_context (sidebar->priv->toolbox), GTK_STYLE_CLASS_SIDEBAR);
	gtk_widget_set_vexpand (sidebar->priv->toolbox, TRUE);
	gtk_widget_set_size_request (GTK_WIDGET (sidebar->priv->toolbox), MIN_SIDEBAR_WIDTH, -1);
	gtk_widget_show (sidebar->priv->toolbox);
	gtk_stack_add_named (GTK_STACK (sidebar), sidebar->priv->toolbox, GTH_SIDEBAR_PAGE_TOOLS);

	empty_view = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (empty_view);
	gtk_stack_add_named (GTK_STACK (sidebar), empty_view, GTH_SIDEBAR_PAGE_EMPTY);

	icon = gtk_image_new_from_icon_name("action-unavailable-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_style_context_add_class (gtk_widget_get_style_context (icon), "void-view");
	gtk_widget_show (icon);
	gtk_box_pack_start (GTK_BOX (empty_view), icon, TRUE, FALSE, 0);
}


GtkWidget *
gth_sidebar_new (char **sections_status)
{
	GthSidebar *sidebar;

	sidebar = g_object_new (GTH_TYPE_SIDEBAR, NULL);
	_gth_sidebar_construct (sidebar);
	_gth_sidebar_add_sections (sidebar, sections_status);

	return (GtkWidget *) sidebar;
}


GtkWidget *
gth_sidebar_get_toolbox (GthSidebar *sidebar)
{
	return sidebar->priv->toolbox;
}


static void
_gth_sidebar_update_view (GthSidebar  *sidebar)
{
	gtk_stack_set_visible_child_name (GTK_STACK (sidebar), (sidebar->priv->n_visibles == 0) ? GTH_SIDEBAR_PAGE_EMPTY : sidebar->priv->selected_page);
}


void
gth_sidebar_set_file (GthSidebar  *sidebar,
		      GthFileData *file_data)
{
	GList *scan;

	if ((file_data == NULL) || ! g_file_info_get_attribute_boolean (file_data->info, "gth::file::is-modified"))
		gth_toolbox_deactivate_tool (GTH_TOOLBOX (sidebar->priv->toolbox));

	_g_object_unref (sidebar->priv->file_data);
	sidebar->priv->file_data = gth_file_data_dup (file_data);

	sidebar->priv->n_visibles = 0;
	for (scan = sidebar->priv->sections; scan; scan = scan->next) {
		GtkWidget *child = scan->data;
		if (gth_sidebar_section_set_file (GTH_SIDEBAR_SECTION (child), sidebar->priv->file_data))
			sidebar->priv->n_visibles++;
	}

	_gth_sidebar_update_view (sidebar);
}


static void
_gth_sidebar_set_visible_page (GthSidebar *sidebar,
			       const char *page)
{
	sidebar->priv->selected_page = page;
	gtk_stack_set_visible_child_name (GTK_STACK (sidebar), sidebar->priv->selected_page);

	_gth_sidebar_update_view (sidebar);
}


void
gth_sidebar_show_properties (GthSidebar *sidebar)
{
	_gth_sidebar_set_visible_page (sidebar, GTH_SIDEBAR_PAGE_PROPERTIES);
}


void
gth_sidebar_show_tools (GthSidebar *sidebar)
{
	_gth_sidebar_set_visible_page (sidebar, GTH_SIDEBAR_PAGE_TOOLS);
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


char **
gth_sidebar_get_sections_status (GthSidebar *sidebar)
{
	GList  *status_list;
	GList  *scan;
	char  **result;

	status_list = NULL;
	for (scan = sidebar->priv->sections; scan; scan = scan->next) {
		GtkWidget *section = scan->data;
		GString   *status;

		status = g_string_new ("");
		g_string_append (status, gth_sidebar_section_get_id (GTH_SIDEBAR_SECTION (section)));
		g_string_append (status, STATUS_DELIMITER);
		g_string_append (status, gth_sidebar_section_get_expanded (GTH_SIDEBAR_SECTION (section)) ? STATUS_EXPANDED : STATUS_NOT_EXPANDED);
		status_list = g_list_prepend (status_list, status->str);

		g_string_free (status, FALSE);
	}
	status_list = g_list_reverse (status_list);

	result = _g_string_list_to_strv (status_list);
	_g_string_list_free (status_list);

	return result;
}
