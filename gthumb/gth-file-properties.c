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
#include <glib/gi18n.h>
#include "dlg-favorite-properties.h"
#include "glib-utils.h"
#include "gth-file-properties.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-property-view.h"
#include "gth-string-list.h"
#include "gth-time.h"
#include "gtk-utils.h"


#define FONT_SCALE (0.85)
#define MIN_HEIGHT 100
#define COMMENT_DEFAULT_HEIGHT 100
#define CATEGORY_SIZE 1000
#define MAX_ATTRIBUTE_LENGTH 128

/* Properties */
enum {
	PROP_0,
	PROP_SHOW_DETAILS
};

enum {
	SCALE_SET_COLUMN,
	WEIGHT_COLUMN,
	ID_COLUMN,
	DISPLAY_NAME_COLUMN,
	VALUE_COLUMN,
	TOOLTIP_COLUMN,
	RAW_COLUMN,
	POS_COLUMN,
	WRITEABLE_COLUMN,
	NUM_COLUMNS
};


struct _GthFilePropertiesPrivate {
	GSettings     *settings;
	GtkWidget     *main_container;
	GtkWidget     *tree_view;
	GtkListStore  *tree_model;
	GtkWidget     *popup_menu;
	GtkWidget     *copy_menu_item;
	GtkWidget     *edit_favorites_menu_item;
	GtkWidget     *context_menu_sep;
	gboolean       show_details;
	GthFileData   *last_file_data;
	GHashTable    *favorites;
	gboolean       default_favorites;
	char          *last_selected;
};


static void gth_file_properties_gth_property_view_interface_init (GthPropertyViewInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthFileProperties,
			 gth_file_properties,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthFileProperties)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
						gth_file_properties_gth_property_view_interface_init))


static gboolean
gth_file_properties_real_can_view (GthPropertyView *base,
				   GthFileData     *file_data)
{
	GthFileProperties *self = GTH_FILE_PROPERTIES (base);
	gboolean           data_available;
	GList             *metadata_info;
	GList             *scan;

	if (file_data == NULL)
		return FALSE;

	if (! self->priv->show_details)
		return TRUE;

	data_available = FALSE;
	metadata_info = gth_main_get_all_metadata_info ();
	for (scan = metadata_info; scan; scan = scan->next) {
		GthMetadataInfo *info = scan->data;
		char            *value;

		if ((info->flags & GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW) != GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW)
			continue;

		value = gth_file_data_get_attribute_as_string (file_data, info->id);
		if ((value == NULL) || (*value == '\0')) {
			g_free (value);
			continue;
		}

		if (info->id != NULL) {
			if (g_str_has_prefix (info->id, "Exif")) {
				data_available = TRUE;
				break;
			}
			else if (g_str_has_prefix (info->id, "Iptc")) {
				data_available = TRUE;
				break;
			}
			else if (g_str_has_prefix (info->id, "Xmp")) {
				data_available = TRUE;
				break;
			}
		}
	}

	return data_available;
}


static void
gth_file_properties_real_set_file (GthPropertyView *base,
		 		   GthFileData     *file_data)
{
	GthFileProperties *self;
	GtkTreeIter        iter;
	GHashTable        *category_hash;
	GList             *metadata_info;
	GList             *scan;

	self = GTH_FILE_PROPERTIES (base);

	if (file_data != self->priv->last_file_data) {
		_g_object_unref (self->priv->last_file_data);
		self->priv->last_file_data = gth_file_data_dup (file_data);
	}

	/* Save the last selected id. */
	{
		char *selected_id;

		selected_id = NULL;
		if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view)), NULL, &iter))
			gtk_tree_model_get (GTK_TREE_MODEL (self->priv->tree_model), &iter, ID_COLUMN, &selected_id, -1);

		if (selected_id != NULL)
			_g_str_set (&self->priv->last_selected, selected_id);
	}

	gtk_list_store_clear (self->priv->tree_model);

	if (file_data == NULL)
		return;

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->tree_model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, 0);

	category_hash = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	metadata_info = gth_main_get_all_metadata_info ();
	for (scan = metadata_info; scan; scan = scan->next) {
		GthMetadataInfo     *info = scan->data;
		char                *value;
		char                *tooltip;
		GthMetadataCategory *category;

		if ((info->flags & GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW) != GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW)
			continue;

		if (info->id == NULL)
			continue;

		value = gth_file_data_get_attribute_as_string (file_data, info->id);
		if ((value == NULL) || (*value == '\0')) {
			g_free (value);
			continue;
		}

		if (! self->priv->show_details && ! self->priv->default_favorites) {
			if (! g_hash_table_contains (self->priv->favorites, info->id))
				continue;
		}
		else if (g_str_has_prefix (info->id, "Exif")) {
			if (! self->priv->show_details)
				continue;
		}
		else if (g_str_has_prefix (info->id, "Iptc")) {
			if (! self->priv->show_details)
				continue;
		}
		else if (g_str_has_prefix (info->id, "Xmp")) {
			if (! self->priv->show_details)
				continue;
		}
		else
			if (self->priv->show_details)
				continue;

		if (value != NULL) {
			char *utf8_value;
			char *tmp_value;

			utf8_value = _g_utf8_from_any (value);
			if (g_utf8_strlen (utf8_value, -1) > MAX_ATTRIBUTE_LENGTH)
				g_utf8_strncpy (g_utf8_offset_to_pointer (utf8_value, MAX_ATTRIBUTE_LENGTH - 1), "…", 1);
			tmp_value = _g_utf8_replace_pattern (utf8_value, "[\r\n]", " ");
			g_free (value);
			value = tmp_value;

			g_free (utf8_value);
		}
		tooltip = g_markup_printf_escaped ("%s: %s", _(info->display_name), value);

		category = g_hash_table_lookup (category_hash, info->category);
		if (category == NULL) {
			category = gth_main_get_metadata_category (info->category);
			gtk_list_store_append (self->priv->tree_model, &iter);
			gtk_list_store_set (self->priv->tree_model, &iter,
					    SCALE_SET_COLUMN, FALSE,
					    WEIGHT_COLUMN, PANGO_WEIGHT_BOLD,
					    ID_COLUMN, category->id,
					    DISPLAY_NAME_COLUMN, _(category->display_name),
					    POS_COLUMN, category->sort_order * CATEGORY_SIZE,
					    -1);
			g_hash_table_insert (category_hash, g_strdup (info->category), category);
		}

		gtk_list_store_append (self->priv->tree_model, &iter);
		gtk_list_store_set (self->priv->tree_model,
				    &iter,
				    SCALE_SET_COLUMN, TRUE,
				    WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
				    ID_COLUMN, info->id,
				    DISPLAY_NAME_COLUMN, _(info->display_name),
				    VALUE_COLUMN, value,
				    TOOLTIP_COLUMN, tooltip,
				    POS_COLUMN, (category->sort_order * CATEGORY_SIZE) + info->sort_order,
				    -1);

		if ((self->priv->last_selected != NULL) && _g_str_equal (info->id, self->priv->last_selected))
			gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view)), &iter);

		g_free (tooltip);
		g_free (value);
	}
	g_list_free (metadata_info);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->tree_model), POS_COLUMN, GTK_SORT_ASCENDING);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (self->priv->tree_view));

	g_hash_table_destroy (category_hash);
}


static const char *
gth_file_properties_real_get_name (GthPropertyView *self)
{
	return _("Properties");
}


static const char *
gth_file_properties_real_get_icon (GthPropertyView *self)
{
	return "document-properties-symbolic";
}


static void
gth_file_properties_finalize (GObject *base)
{
	GthFileProperties *self;

	self = (GthFileProperties *) base;

	if (self->priv->popup_menu != NULL)
		gtk_widget_destroy (self->priv->popup_menu);
	_g_object_unref (self->priv->last_file_data);
	_g_object_unref (self->priv->settings);
	if (self->priv->favorites != NULL)
		g_hash_table_unref (self->priv->favorites);
	g_free (self->priv->last_selected);

	G_OBJECT_CLASS (gth_file_properties_parent_class)->finalize (base);
}


static void
update_favorites (GthFileProperties *self)
{
	char *favorites;

	if (self->priv->show_details)
		return;

	favorites = g_settings_get_string (self->priv->settings, PREF_BROWSER_FAVORITE_PROPERTIES);

	if (self->priv->favorites != NULL)
		g_hash_table_unref (self->priv->favorites);
	self->priv->favorites = _g_str_split_as_hash_table (favorites);
	self->priv->default_favorites = _g_str_equal (favorites, "default");

	g_free (favorites);
}


static void
update_context_menu_separator_visibility (GthFileProperties *self)
{
	int n_visible;

	n_visible = 0;
	if (gtk_widget_get_visible (self->priv->copy_menu_item))
		n_visible++;
	if (gtk_widget_get_visible (self->priv->edit_favorites_menu_item))
		n_visible++;
	gtk_widget_set_visible (self->priv->context_menu_sep, n_visible > 1);
}


static void
gth_file_properties_set_property (GObject      *object,
				  guint         property_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GthFileProperties *self;

	self = GTH_FILE_PROPERTIES (object);

	switch (property_id) {
	case PROP_SHOW_DETAILS:
		self->priv->show_details = g_value_get_boolean (value);
		update_favorites (self);
		if (self->priv->edit_favorites_menu_item != NULL) {
			gtk_widget_set_visible (self->priv->edit_favorites_menu_item, ! self->priv->show_details);
			update_context_menu_separator_visibility (self);
		}
		break;
	default:
		break;
	}
}


static void
gth_file_properties_get_property (GObject    *object,
				  guint       property_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	GthFileProperties *self;

	self = GTH_FILE_PROPERTIES (object);

	switch (property_id) {
	case PROP_SHOW_DETAILS:
		g_value_set_boolean (value, self->priv->show_details);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_file_properties_class_init (GthFilePropertiesClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = gth_file_properties_set_property;
	object_class->get_property = gth_file_properties_get_property;
	object_class->finalize = gth_file_properties_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SHOW_DETAILS,
					 g_param_spec_boolean ("show-details",
							       "Show details",
							       "Whether to show all the file properties",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
copy_menu_item_activate_cb (GtkMenuItem *menuitem,
			    gpointer     user_data)
{
	GthFileProperties *self = user_data;
	GtkTreeModel      *model;
	GtkTreeIter        iter;
	char              *value;

	if (! gtk_tree_selection_get_selected (GTK_TREE_SELECTION (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view))), &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, VALUE_COLUMN, &value, -1);
	if (value != NULL)
		gtk_clipboard_set_text (gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (menuitem)), GDK_SELECTION_CLIPBOARD),
					value,
					-1);

	g_free (value);
}


static void
edit_favorites_menu_item_activate_cb (GtkMenuItem *menuitem,
				      gpointer     user_data)
{
	GthFileProperties *self = user_data;
	dlg_favorite_properties (GTH_BROWSER (_gtk_widget_get_toplevel_if_window (GTK_WIDGET (self))));
}


static gboolean
tree_view_button_press_event_cb (GtkWidget      *widget,
				 GdkEventButton *event,
				 gpointer        user_data)
{
	GthFileProperties *self = user_data;

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		GtkTreePath *path;
		gboolean     path_selected;

		path_selected = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (self->priv->tree_view),
			event->x,
			event->y,
			&path,
			NULL,
			NULL,
			NULL);

		gtk_widget_set_visible (self->priv->copy_menu_item, path_selected);
		update_context_menu_separator_visibility (self);

		if (path != NULL) {
			gtk_tree_selection_select_path (GTK_TREE_SELECTION (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view))), path);
			gtk_tree_path_free (path);
		}

		gtk_menu_popup_at_pointer (GTK_MENU (self->priv->popup_menu), (GdkEvent *) event);

		return TRUE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)) {
		GtkTreePath *path;

		gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (self->priv->tree_view),
					       event->x,
					       event->y,
					       &path,
					       NULL,
					       NULL,
					       NULL);

		if (path != NULL) {
			GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view));

			if ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
				if (gtk_tree_selection_path_is_selected (selection, path)) {
					gtk_tree_selection_unselect_path (selection, path);
					_g_str_set (&self->priv->last_selected, NULL);
				}
				else
					gtk_tree_selection_select_path (selection, path);
			}
			else
				gtk_tree_selection_select_path (selection, path);
			gtk_tree_path_free (path);
		}

		return TRUE;
	}

	return FALSE;
}


static gboolean
tree_view_popup_menu_cb (GtkWidget *widget,
			 gpointer   user_data)
{
	GthFileProperties *self = user_data;

	gtk_widget_set_visible (self->priv->copy_menu_item, TRUE);
	update_context_menu_separator_visibility (self);

	gtk_menu_popup_at_pointer (GTK_MENU (self->priv->popup_menu), NULL);

	return TRUE;
}


static void
pref_favorite_properties_changed (GSettings  *settings,
				  const char *key,
				  gpointer    user_data)
{
	GthFileProperties *self = user_data;

	update_favorites (self);
	gth_property_view_set_file (GTH_PROPERTY_VIEW (self), self->priv->last_file_data);
}


static void
gth_file_properties_init (GthFileProperties *self)
{
	GtkWidget         *scrolled_win;
	GtkWidget         *menu_item;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	self->priv = gth_file_properties_get_instance_private (self);
	self->priv->show_details = FALSE;
	self->priv->last_file_data = NULL;
	self->priv->settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	self->priv->favorites = NULL;
	self->priv->last_selected = NULL;

	update_favorites (self);

	g_signal_connect (self->priv->settings,
			  "changed::" PREF_BROWSER_FAVORITE_PROPERTIES,
			  G_CALLBACK (pref_favorite_properties_changed),
			  self);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 6);

	self->priv->main_container = scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);
	gtk_widget_show (scrolled_win);
	gtk_widget_set_size_request (scrolled_win, -1, MIN_HEIGHT);
	gtk_box_pack_start (GTK_BOX (self), scrolled_win, TRUE, TRUE, 0);

	self->priv->tree_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->tree_view), FALSE);
	gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (self->priv->tree_view), TOOLTIP_COLUMN);
	self->priv->tree_model = gtk_list_store_new (NUM_COLUMNS,
						     G_TYPE_BOOLEAN,
						     PANGO_TYPE_WEIGHT,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_INT,
						     G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->tree_view),
				 GTK_TREE_MODEL (self->priv->tree_model));
	g_object_unref (self->priv->tree_model);
	gtk_widget_show (self->priv->tree_view);
	gtk_container_add (GTK_CONTAINER (scrolled_win), self->priv->tree_view);

	/* popup menu */

	self->priv->popup_menu = gtk_menu_new ();

	self->priv->copy_menu_item = menu_item = gtk_menu_item_new_with_label (_GTK_LABEL_COPY);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->popup_menu), menu_item);
	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (copy_menu_item_activate_cb),
			  self);

	self->priv->context_menu_sep = menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->popup_menu), menu_item);

	self->priv->edit_favorites_menu_item = menu_item = gtk_menu_item_new_with_label (_("Preferences"));
	gtk_widget_set_visible (menu_item, ! self->priv->show_details);
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->popup_menu), menu_item);
	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (edit_favorites_menu_item_activate_cb),
			  self);

	update_context_menu_separator_visibility (self);

	g_signal_connect (self->priv->tree_view,
			  "button-press-event",
			  G_CALLBACK (tree_view_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->tree_view,
			  "popup-menu",
			  G_CALLBACK (tree_view_popup_menu_cb),
			  self);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", DISPLAY_NAME_COLUMN,
							   "scale-set", SCALE_SET_COLUMN,
							   "weight", WEIGHT_COLUMN,
							   NULL);
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "scale", FONT_SCALE,
		      NULL);

	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", VALUE_COLUMN,
							   NULL);
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "scale", FONT_SCALE,
		      NULL);

	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->tree_model), POS_COLUMN, GTK_SORT_ASCENDING);
}


static void
gth_file_properties_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->get_name = gth_file_properties_real_get_name;
	iface->get_icon = gth_file_properties_real_get_icon;
	iface->can_view = gth_file_properties_real_can_view;
	iface->set_file = gth_file_properties_real_set_file;
}
