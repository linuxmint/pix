/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009-2013 The Free Software Foundation, Inc.
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
#include "gth-enum-types.h"
#include "gth-main.h"
#include "gth-tags-entry.h"
#include "gth-tags-file.h"
#include "gtk-utils.h"


#define EXPANDED_LIST_HEIGHT 160
#define POPUP_WINDOW_HEIGHT 250


enum {
	COMPLETION_NAME_COLUMN,
	COMPLETION_N_COLUMNS
};


enum {
	EXPANDED_LIST_HIGHLIGHTED_COLUMN,
	EXPANDED_LIST_USED_COLUMN,
	EXPANDED_LIST_INCONSISTENT_COLUMN,
	EXPANDED_LIST_SEPARATOR_COLUMN,
	EXPANDED_LIST_NAME_COLUMN,
	EXPANDED_LIST_N_COLUMNS
};


/* Properties */
enum {
	PROP_0,
	PROP_MODE
};

/* Signals */
enum {
	LIST_COLLAPSED,
	LAST_SIGNAL
};


typedef struct {
	char     *name;
	gboolean  highlighted;
	gboolean  used;
	gboolean  suggested;
	gboolean  inconsistent;
} TagData;


typedef struct  {
	char         **last_used;
	GtkWidget     *container;
	GtkWidget     *scrolled_window;
	GtkWidget     *tree_view;
	GtkListStore  *store;
	GtkWidget     *popup_menu;
} ExpandedList;


typedef struct {
	GtkWidget *window;
	GtkWidget *container;
	GdkDevice *grab_device;
} PopupWindow;


struct _GthTagsEntryPrivate {
	GthTagsEntryMode	  mode;
	GtkWidget		 *entry;
	GtkWidget		 *expand_button;
	ExpandedList		  expanded_list;
	PopupWindow               popup;
	gboolean		  expanded;
	char			**tags;
	GtkEntryCompletion	 *completion;
	GtkListStore		 *completion_store;
	char			 *new_tag;
	gboolean		  action_create;
	gulong			  monitor_event;
	GHashTable		 *inconsistent;
};


static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthTagsEntry,
			 gth_tags_entry,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthTagsEntry))


static void
gth_tags_entry_finalize (GObject *obj)
{
	GthTagsEntry *self;

	self = GTH_TAGS_ENTRY (obj);

	g_signal_handler_disconnect (gth_main_get_default_monitor (), self->priv->monitor_event);

	g_free (self->priv->new_tag);
	g_object_unref (self->priv->completion);
	g_strfreev (self->priv->tags);
	g_strfreev (self->priv->expanded_list.last_used);
	gtk_widget_destroy (self->priv->expanded_list.popup_menu);
	gtk_widget_destroy (self->priv->popup.window);
	g_hash_table_unref (self->priv->inconsistent);

	G_OBJECT_CLASS (gth_tags_entry_parent_class)->finalize (obj);
}


static void
gth_tags_entry_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GthTagsEntry *self;

	self = GTH_TAGS_ENTRY (object);

	switch (property_id) {
	case PROP_MODE:
	{
		GthTagsEntryMode mode;

		mode = g_value_get_enum (value);
		if (self->priv->mode != mode) {
			self->priv->mode = mode;

			g_object_ref (self->priv->expanded_list.scrolled_window);
			gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (self->priv->expanded_list.scrolled_window)), self->priv->expanded_list.scrolled_window);

			if (self->priv->mode == GTH_TAGS_ENTRY_MODE_POPUP)
				gtk_box_pack_start (GTK_BOX (self->priv->popup.container), self->priv->expanded_list.scrolled_window, TRUE, TRUE, 0);
			else
				gtk_box_pack_start (GTK_BOX (self->priv->expanded_list.container), self->priv->expanded_list.scrolled_window, TRUE, TRUE, 0);

			g_object_unref (self->priv->expanded_list.scrolled_window);
		}
	}
		break;
	default:
		break;
	}
}


static void
gth_tags_entry_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GthTagsEntry *self;

	self = GTH_TAGS_ENTRY (object);

	switch (property_id) {
	case PROP_MODE:
		g_value_set_enum (value, self->priv->mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_tags_entry_grab_focus (GtkWidget *widget)
{
	GthTagsEntry *entry = GTH_TAGS_ENTRY (widget);

	gtk_widget_grab_focus (entry->priv->entry);
}


static void
gth_tags_entry_class_init (GthTagsEntryClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass*) (klass);
	object_class->set_property = gth_tags_entry_set_property;
	object_class->get_property = gth_tags_entry_get_property;
	object_class->finalize = gth_tags_entry_finalize;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->grab_focus = gth_tags_entry_grab_focus;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_MODE,
					 g_param_spec_enum ("mode",
                                                            "Mode",
                                                            "The way to show the list",
                                                            GTH_TYPE_TAGS_ENTRY_MODE,
                                                            GTH_TAGS_ENTRY_MODE_INLINE,
                                                            G_PARAM_READWRITE));

	/* signals */

	signals[LIST_COLLAPSED] =
		g_signal_new ("list-collapsed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTagsEntryClass, list_collapsed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
get_tag_limits (GthTagsEntry  *self,
		const char   **tag_start_p,
		const char   **tag_end_p)
{
	const char *text;
	const char *cursor_start;
	const char *tag_start;
	const char *tag_end;

	text = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
	cursor_start = g_utf8_offset_to_pointer (text, gtk_editable_get_position (GTK_EDITABLE (self->priv->entry)));

	if (g_utf8_get_char (cursor_start) == ',') {
		if (cursor_start != text + strlen (text))
			tag_start = g_utf8_next_char (cursor_start);
		else
			tag_start = text + strlen (text);
	}
	else {
		tag_start = g_utf8_strrchr (text, (gssize)(cursor_start - text), ',');
		if (tag_start == NULL)
			tag_start = text;
		else
			tag_start = g_utf8_next_char (tag_start);
	}

	tag_end = g_utf8_strchr (tag_start, -1, ',');
	if (tag_end == NULL)
		tag_end = text + strlen (text);

	while ((tag_start != tag_end) && g_unichar_isspace (g_utf8_get_char (tag_start)))
		tag_start = g_utf8_next_char (tag_start);

	*tag_start_p = tag_start;
	*tag_end_p = tag_end;
}


static gboolean
can_create_tag (GthTagsEntry *self,
		const char   *tag)
{
	int i;

	if (tag == NULL)
		return FALSE;
	if (tag[0] == '\0')
		return FALSE;

	for (i = 0; self->priv->tags[i] != NULL; i++)
		if (strcmp (self->priv->tags[i], tag) == 0)
			return FALSE;

	return TRUE;
}


static int
sort_tag_data (gconstpointer a,
	       gconstpointer b,
	       gpointer      user_data)
{
	TagData *tag_data_a = * (TagData **) a;
	TagData *tag_data_b = * (TagData **) b;

	if (tag_data_a->highlighted && tag_data_b->highlighted)
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
	else if (tag_data_a->highlighted || tag_data_b->highlighted)
		return tag_data_a->used ? -1 : 1;
	else if (tag_data_a->suggested && tag_data_b->suggested)
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
	else if (tag_data_a->suggested || tag_data_b->suggested)
		return tag_data_a->suggested ? -1 : 1;
	else
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
}


static gboolean
_tag_is_valid (const char *tag)
{
	if (tag == NULL)
		return FALSE;
	if (_g_utf8_all_spaces (tag))
		return FALSE;
	return TRUE;
}


static void
update_expanded_list_from_entry (GthTagsEntry *self)
{
	char        **all_tags;
	char        **used_tags;
	TagData     **tag_data;
	int           i, t, tag_data_len;
	GtkTreeIter   iter;
	gboolean      separator_required;

	all_tags = g_strdupv (self->priv->tags);
	used_tags = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (self), FALSE);

	tag_data = g_new0 (TagData *, g_strv_length (all_tags) + 1);
	for (i = 0, t = 0; all_tags[i] != NULL; i++) {
		int j;

		if (! _tag_is_valid (all_tags[i]))
			continue;

		tag_data[t] = g_new0 (TagData, 1);
		tag_data[t]->name = g_strdup (all_tags[i]);
		tag_data[t]->highlighted = FALSE;
		tag_data[t]->used = FALSE;
		tag_data[t]->inconsistent = (g_hash_table_lookup (self->priv->inconsistent, tag_data[t]->name) != NULL);
		tag_data[t]->suggested = tag_data[t]->inconsistent;
		for (j = 0; ! tag_data[t]->used && (used_tags[j] != NULL); j++)
			if (g_utf8_collate (tag_data[t]->name, used_tags[j]) == 0) {
				tag_data[t]->highlighted = TRUE;
				tag_data[t]->used = TRUE;
				tag_data[t]->inconsistent = FALSE;
				tag_data[t]->suggested = FALSE;
			}

		if (! tag_data[t]->used)
			for (j = 0; ! tag_data[t]->suggested && (self->priv->expanded_list.last_used[j] != NULL); j++)
				if (g_utf8_collate (tag_data[t]->name, self->priv->expanded_list.last_used[j]) == 0)
					tag_data[t]->suggested = TRUE;

		t++;
	}
	tag_data_len = t - 1;

	g_qsort_with_data (tag_data,
			   tag_data_len,
			   sizeof (TagData *),
			   sort_tag_data,
			   NULL);

	gtk_list_store_clear (self->priv->expanded_list.store);

	/* highlighted */

	separator_required = FALSE;
	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (! tag_data[i]->highlighted)
			continue;

		separator_required = TRUE;

		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
				    EXPANDED_LIST_HIGHLIGHTED_COLUMN, TRUE,
				    EXPANDED_LIST_USED_COLUMN, TRUE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, tag_data[i]->inconsistent,
				    EXPANDED_LIST_SEPARATOR_COLUMN, FALSE,
				    EXPANDED_LIST_NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	if (separator_required) {
		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
				    EXPANDED_LIST_HIGHLIGHTED_COLUMN, FALSE,
				    EXPANDED_LIST_USED_COLUMN, FALSE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, FALSE,
				    EXPANDED_LIST_SEPARATOR_COLUMN, TRUE,
				    EXPANDED_LIST_NAME_COLUMN, "",
				    -1);
	}

	/* suggested */

	separator_required = FALSE;
	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (! tag_data[i]->suggested)
			continue;

		separator_required = TRUE;

		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
				    EXPANDED_LIST_HIGHLIGHTED_COLUMN, FALSE,
				    EXPANDED_LIST_USED_COLUMN, FALSE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, tag_data[i]->inconsistent,
				    EXPANDED_LIST_SEPARATOR_COLUMN, FALSE,
				    EXPANDED_LIST_NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	if (separator_required) {
		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
				    EXPANDED_LIST_HIGHLIGHTED_COLUMN, FALSE,
				    EXPANDED_LIST_USED_COLUMN, FALSE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, FALSE,
				    EXPANDED_LIST_SEPARATOR_COLUMN, TRUE,
				    EXPANDED_LIST_NAME_COLUMN, "",
				    -1);
	}

	/* others */

	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (tag_data[i]->used || tag_data[i]->suggested)
			continue;

		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
				    EXPANDED_LIST_HIGHLIGHTED_COLUMN, FALSE,
				    EXPANDED_LIST_USED_COLUMN, FALSE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, tag_data[i]->inconsistent,
				    EXPANDED_LIST_SEPARATOR_COLUMN, FALSE,
				    EXPANDED_LIST_NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	g_strfreev (self->priv->expanded_list.last_used);
	self->priv->expanded_list.last_used = used_tags;

	for (i = 0; tag_data[i] != NULL; i++) {
		g_free (tag_data[i]->name);
		g_free (tag_data[i]);
	}
	g_free (tag_data);
	g_strfreev (all_tags);
}


static void
text_changed_cb (GtkEntry     *entry,
		 GParamSpec   *pspec,
		 GthTagsEntry *self)
{
	const char *tag_start;
	const char *tag_end;

	g_return_if_fail (GTH_IS_TAGS_ENTRY (self));

	g_free (self->priv->new_tag);
	self->priv->new_tag = NULL;

	if (self->priv->action_create) {
		gtk_entry_completion_delete_action (self->priv->completion, 0);
		self->priv->action_create = FALSE;
	}

	get_tag_limits (self, &tag_start, &tag_end);
	if (tag_start == tag_end)
		return;

	self->priv->new_tag = g_strndup (tag_start, tag_end - tag_start);
	self->priv->new_tag = g_strstrip (self->priv->new_tag);

	if (can_create_tag (self, self->priv->new_tag)) {
		char *action_text;

		action_text = g_strdup_printf (_("Create tag “%s”"), self->priv->new_tag);
		gtk_entry_completion_insert_action_text (self->priv->completion, 0, action_text);
		self->priv->action_create = TRUE;

		g_free (action_text);
	}

	update_expanded_list_from_entry (self);
}


static gboolean
match_func (GtkEntryCompletion *completion,
            const char         *key,
            GtkTreeIter        *iter,
            gpointer            user_data)
{
	GthTagsEntry *self = user_data;
	char         *name;
	gboolean      result;

	if (self->priv->new_tag == NULL)
		return TRUE;

	if (self->priv->new_tag[0] == '\0')
		return TRUE;

	gtk_tree_model_get (GTK_TREE_MODEL (self->priv->completion_store), iter,
			    COMPLETION_NAME_COLUMN, &name,
			    -1);

	if (name != NULL) {
		char *k1;
		char *k2;

		k1 = g_utf8_casefold (self->priv->new_tag, -1);
		k2 = g_utf8_casefold (name, strlen (self->priv->new_tag));

		result = g_utf8_collate (k1, k2) == 0;


		g_free (k2);
		g_free (k1);
	}
	else
		result = FALSE;

	g_free (name);

	return result;
}


static gboolean
completion_match_selected_cb (GtkEntryCompletion *widget,
                              GtkTreeModel       *model,
                              GtkTreeIter        *iter,
                              gpointer            user_data)
{
	GthTagsEntry *self = user_data;
	const char   *tag_start;
	const char   *tag_end;
	const char   *text;
	char         *head;
	const char   *tail;
	char         *tag_name = NULL;

	get_tag_limits (self, &tag_start, &tag_end);
	text = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
	head = g_strndup (text, tag_start - text);
	head = g_strstrip (head);
	if (tag_end[0] != '\0')
		tail = g_utf8_next_char (tag_end);
	else
		tail = tag_end;
	gtk_tree_model_get (model, iter,
			    COMPLETION_NAME_COLUMN, &tag_name,
			    -1);

	if (tag_name != NULL) {
		char *new_text;

		new_text = g_strconcat (head,
					" ",
					tag_name,
					", " /*(tail != tag_end ? ", " : "")*/,
					tail,
					NULL);
		gtk_entry_set_text (GTK_ENTRY (self->priv->entry), new_text);
		gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), -1);

		g_free (new_text);
		g_free (tag_name);
	}

	g_free (head);

	return TRUE;
}


static void
completion_action_activated_cb (GtkEntryCompletion *widget,
                                int                 index,
                                gpointer            user_data)
{
	GthTagsEntry *self = user_data;
	const char   *tag_start;
	const char   *tag_end;
	const char   *text;
	char         *head;
	const char   *tail;
	char         *tag_name = NULL;
	char         *new_text;
	GthTagsFile  *tags_file;

	if (index != 0)
		return;

	get_tag_limits (self, &tag_start, &tag_end);
	text = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
	head = g_strndup (text, tag_start - text);
	head = g_strstrip (head);
	if (tag_end[0] != '\0')
		tail = g_utf8_next_char (tag_end);
	else
		tail = tag_end;
	tag_name = g_strndup (tag_start, tag_end - tag_start);
	tag_name = g_strstrip (tag_name);
	new_text = g_strconcat (head,
				" ",
				tag_name,
				", ",
				tail,
				NULL);

	gtk_entry_set_text (GTK_ENTRY (self->priv->entry), new_text);
	gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), -1);

	tags_file = gth_main_get_default_tag_file ();
	gth_tags_file_add (tags_file, tag_name);
	gth_main_tags_changed ();

	g_free (new_text);
	g_free (tag_name);
	g_free (head);
}


static void
update_completion_list (GthTagsEntry *self)
{
	GthTagsFile *tags;
	int          i;

	tags = gth_main_get_default_tag_file ();

	g_strfreev (self->priv->tags);
	self->priv->tags = g_strdupv (gth_tags_file_get_tags (tags));

	gtk_list_store_clear (self->priv->completion_store);
	for (i = 0; self->priv->tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (self->priv->completion_store, &iter);
		gtk_list_store_set (self->priv->completion_store, &iter,
				    COMPLETION_NAME_COLUMN, self->priv->tags[i],
				    -1);
	}
}


static void
tags_changed_cb (GthMonitor   *monitor,
		 GthTagsEntry *self)
{
	update_completion_list (self);
	update_expanded_list_from_entry (self);
}


static void
update_entry_from_expanded_list (GthTagsEntry *self)
{
	GtkTreeIter   iter;
	GList        *name_list;
	char        **tags;
	GList        *scan;
	int           i;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->expanded_list.store), &iter))
		return;

	name_list = NULL;
	do {
		char     *name;
		gboolean  used;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->expanded_list.store), &iter,
				    EXPANDED_LIST_NAME_COLUMN, &name,
				    EXPANDED_LIST_USED_COLUMN, &used,
				    -1);

		if (used)
			name_list = g_list_prepend (name_list, name);
		else
			g_free (name);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->expanded_list.store), &iter));

	name_list = g_list_reverse (name_list);
	tags = g_new (char *, g_list_length (name_list) + 1);
	for (i = 0, scan = name_list; scan; scan = scan->next)
		tags[i++] = scan->data;
	tags[i] = NULL;
	g_signal_handlers_block_by_func (G_OBJECT (self->priv->entry), text_changed_cb, self);
	g_signal_handler_block (gth_main_get_default_monitor (), self->priv->monitor_event);
	gth_tags_entry_set_tags (GTH_TAGS_ENTRY (self), tags);
	g_signal_handler_unblock (gth_main_get_default_monitor (), self->priv->monitor_event);
	g_signal_handlers_unblock_by_func (G_OBJECT (self->priv->entry), text_changed_cb, self);

	g_free (tags);
	_g_string_list_free (name_list);
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	GthTagsEntry *self = user_data;
	GtkTreePath  *tpath;
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->priv->expanded_list.tree_view));
	if (gtk_tree_model_get_iter (tree_model, &iter, tpath)) {
		char     *tag;
		gboolean  used;

		gtk_tree_model_get (tree_model, &iter,
				    EXPANDED_LIST_NAME_COLUMN, &tag,
				    EXPANDED_LIST_USED_COLUMN, &used,
				    -1);

		g_hash_table_remove (self->priv->inconsistent, tag);
		gtk_list_store_set (GTK_LIST_STORE (tree_model), &iter,
				    EXPANDED_LIST_USED_COLUMN, ! used,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, FALSE,
				    -1);

		g_free (tag);
	}

	update_entry_from_expanded_list (self);

	gtk_tree_path_free (tpath);
}


static void
tag_list_unmap_cb (GtkWidget    *widget,
		   GthTagsEntry *self)
{
        g_signal_emit (self, signals[LIST_COLLAPSED], 0);
}


static gboolean
row_separator_func (GtkTreeModel *model,
                    GtkTreeIter  *iter,
                    gpointer      data)
{
	gboolean separator;

	gtk_tree_model_get (model, iter, EXPANDED_LIST_SEPARATOR_COLUMN, &separator, -1);

	return separator;
}


static void
_gth_tags_entry_delete_selected_tag (GthTagsEntry *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	char         *tag;
	GthTagsFile  *tags_file;

	if (! gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->expanded_list.tree_view)),
					       &model,
					       &iter))
	{
		return;
	}

	gtk_tree_model_get (model, &iter, EXPANDED_LIST_NAME_COLUMN, &tag, -1);

	tags_file = gth_main_get_default_tag_file ();
	if (gth_tags_file_remove (tags_file, tag))
		gth_main_tags_changed ();

	g_free (tag);
}


static gboolean
expanded_list_key_press_event_cb (GtkWidget   *widget,
				  GdkEventKey *event,
				  gpointer     user_data)
{
	GthTagsEntry *self = user_data;
	guint         modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();
	if ((event->state & modifiers) == 0) {
		switch (event->keyval) {
		case GDK_KEY_Delete:
			_gth_tags_entry_delete_selected_tag (self);
			return TRUE;
		}
	}

	return FALSE;
}


static void
delete_tag_activate_cb (GtkMenuItem *menuitem,
			gpointer     user_data)
{
	_gth_tags_entry_delete_selected_tag (GTH_TAGS_ENTRY (user_data));
}


static gboolean
expanded_list_button_press_event_cb (GtkStatusIcon  *status_icon,
		 	 	     GdkEventButton *event,
				     gpointer        user_data)
{
	GthTagsEntry *self = user_data;

	if (event->button == 3)
		gtk_menu_popup_at_pointer (GTK_MENU (self->priv->expanded_list.popup_menu), (GdkEvent *) event);

	return FALSE;
}


/* -- popup window -- */


static void
_gth_tags_entry_ungrab_devices (GthTagsEntry *self,
				guint32       time)
{
	if (self->priv->popup.grab_device != NULL) {
		gtk_device_grab_remove (self->priv->popup.window, self->priv->popup.grab_device);
		self->priv->popup.grab_device = NULL;
	}
}


static gboolean
_gth_tags_entry_grab_broken_event (GtkWidget          *widget,
				   GdkEventGrabBroken *event,
				   gpointer            user_data)
{
	GthTagsEntry *self = user_data;

	if (event->grab_window == NULL)
		_gth_tags_entry_ungrab_devices (self, GDK_CURRENT_TIME);

	return TRUE;
}


static void
popup_window_hide (GthTagsEntry *self)
{
	_gth_tags_entry_ungrab_devices (self, GDK_CURRENT_TIME);

	self->priv->expanded = FALSE;
	gtk_widget_hide (self->priv->popup.window);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->expand_button), FALSE);
}


static gboolean
popup_window_button_press_event_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	GthTagsEntry		 *self = user_data;
	cairo_rectangle_int_t	  popup_area;

	gdk_window_get_geometry (gtk_widget_get_window (self->priv->popup.window),
				 &popup_area.x,
				 &popup_area.y,
				 &popup_area.width,
				 &popup_area.height);

	/*g_print ("(%.0f, %.0f) <==> (%d, %d)[%d, %d]\n", event->x_root, event->y_root,  popup_area.x,  popup_area.y, popup_area.width, popup_area.height);*/

	if ((event->x_root < popup_area.x)
	    || (event->x_root > popup_area.x + popup_area.width)
	    || (event->y_root < popup_area.y)
	    || (event->y_root > popup_area.y + popup_area.height))
	{
		popup_window_hide (self);
	}

	return FALSE;
}


static gboolean
popup_window_key_press_event_cb (GtkWidget   *widget,
				 GdkEventKey *event,
				 gpointer     user_data)
{
	GthTagsEntry *self = user_data;

	switch (event->keyval) {
	case GDK_KEY_Escape:
		popup_window_hide (self);
		break;

	default:
		break;
	}

	return FALSE;
}


static void
popup_window_show (GthTagsEntry *self)
{
	GtkRequisition         popup_req;
	int                    x;
	int                    y;
	GtkAllocation          allocation;
	int                    selector_height;
	GdkRectangle           monitor;
	GdkGrabStatus          grab_status;

	gdk_window_get_position (gtk_widget_get_window (GTK_WIDGET (self)), &x, &y);
	gtk_widget_get_allocation (self->priv->entry, &allocation);
	x += allocation.x;
	y += allocation.y;
	selector_height = allocation.height;

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	popup_req.width = allocation.width;
	popup_req.height = POPUP_WINDOW_HEIGHT;

	_gtk_widget_get_monitor_geometry (GTK_WIDGET (self), &monitor);
	if (x < monitor.x)
		x = monitor.x;
	else if (x + popup_req.width > monitor.x + monitor.width)
		x = monitor.x + monitor.width - popup_req.width;
	if (y + selector_height + popup_req.height > monitor.y + monitor.height)
		y = y - popup_req.height;
	else
		y = y + selector_height;

	gtk_window_resize (GTK_WINDOW (self->priv->popup.window), popup_req.width, popup_req.height);
	gtk_window_move (GTK_WINDOW (self->priv->popup.window), x, y);
	gtk_widget_show (self->priv->popup.window);

	self->priv->popup.grab_device = gtk_get_current_event_device ();
	grab_status = gdk_seat_grab (gdk_device_get_seat (self->priv->popup.grab_device),
				     gtk_widget_get_window (self->priv->popup.window),
				     GDK_SEAT_CAPABILITY_KEYBOARD | GDK_SEAT_CAPABILITY_ALL_POINTING,
				     TRUE,
				     NULL,
				     NULL,
				     NULL,
				     NULL);
	if (grab_status == GDK_GRAB_SUCCESS) {
		gtk_widget_grab_focus (self->priv->expanded_list.tree_view);
		gtk_device_grab_add (self->priv->popup.window, self->priv->popup.grab_device, TRUE);
	}
}


static void
expand_button_toggled_cb (GtkToggleButton *button,
			  gpointer         user_data)
{
	GthTagsEntry *self = user_data;

	if (gtk_toggle_button_get_active (button)) {
		switch (self->priv->mode) {
		case GTH_TAGS_ENTRY_MODE_INLINE:
			gtk_widget_show (self->priv->expanded_list.container);
			break;
		case GTH_TAGS_ENTRY_MODE_POPUP:
			popup_window_show (self);
			break;
		}
	}
	else
		gtk_widget_hide (self->priv->expanded_list.container);
}


static gboolean
entry_focus_in_event_cb (GtkWidget *widget,
	                 GdkEvent  *event,
	                 gpointer   user_data)
{
	GthTagsEntry *self = user_data;
	gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), -1);
	return GDK_EVENT_PROPAGATE;
}


static void
gth_tags_entry_init (GthTagsEntry *self)
{
	GtkWidget         *hbox;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;
	GtkWidget         *menu_item;

	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)), GTK_STYLE_CLASS_COMBOBOX_ENTRY);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 3);

	self->priv = gth_tags_entry_get_instance_private (self);
	self->priv->mode = GTH_TAGS_ENTRY_MODE_INLINE;
	self->priv->expanded_list.last_used = g_new0 (char *, 1);
	self->priv->expanded = FALSE;
	self->priv->inconsistent = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	self->priv->popup.grab_device = NULL;

	/* entry / expander button box */

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (hbox), GTK_STYLE_CLASS_LINKED);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (self), hbox, FALSE, FALSE, 0);

	/* entry completion */

	self->priv->completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_popup_completion (self->priv->completion, TRUE);
	self->priv->completion_store = gtk_list_store_new (COMPLETION_N_COLUMNS, G_TYPE_STRING);
	gtk_entry_completion_set_model (self->priv->completion, GTK_TREE_MODEL (self->priv->completion_store));
	g_object_unref (self->priv->completion_store);
	gtk_entry_completion_set_text_column (self->priv->completion, COMPLETION_NAME_COLUMN);
	gtk_entry_completion_set_match_func (self->priv->completion, match_func, self, NULL);

	g_signal_connect (self->priv->completion,
			  "match-selected",
			  G_CALLBACK (completion_match_selected_cb),
			  self);
	g_signal_connect (self->priv->completion,
			  "action-activated",
			  G_CALLBACK (completion_action_activated_cb),
			  self);

	/* entry */

	self->priv->entry = gtk_entry_new ();
	gtk_entry_set_completion (GTK_ENTRY (self->priv->entry), self->priv->completion);
	gtk_widget_show (self->priv->entry);
	gtk_box_pack_start (GTK_BOX (hbox), self->priv->entry, TRUE, TRUE, 0);
	g_signal_connect (self->priv->entry,
			  "focus-in-event",
			  G_CALLBACK (entry_focus_in_event_cb),
			  self);

	/* expand button */

	self->priv->expand_button = gtk_toggle_button_new ();
	gtk_widget_set_tooltip_text (self->priv->expand_button, _("Show all the tags"));
	gtk_container_add (GTK_CONTAINER (self->priv->expand_button), gtk_image_new_from_icon_name ("go-down-symbolic", GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all (self->priv->expand_button);
	gtk_box_pack_start (GTK_BOX (hbox), self->priv->expand_button, FALSE, FALSE, 0);

	g_signal_connect (self->priv->expand_button,
			  "toggled",
			  G_CALLBACK (expand_button_toggled_cb),
			  self);

	/* expanded list, the treeview */

	self->priv->expanded_list.store = gtk_list_store_new (EXPANDED_LIST_N_COLUMNS,
							      G_TYPE_BOOLEAN,
							      G_TYPE_BOOLEAN,
							      G_TYPE_BOOLEAN,
							      G_TYPE_BOOLEAN,
							      G_TYPE_STRING);
	self->priv->expanded_list.tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (self->priv->expanded_list.store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->expanded_list.tree_view), FALSE);
	gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (self->priv->expanded_list.tree_view),
					      row_separator_func,
					      self,
					      NULL);
	g_object_unref (self->priv->expanded_list.store);

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  self);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "active", EXPANDED_LIST_USED_COLUMN,
					     "inconsistent", EXPANDED_LIST_INCONSISTENT_COLUMN,
					     NULL);

	/* the name column. */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	g_object_set (renderer, "icon-name", "tag-symbolic", NULL);

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", EXPANDED_LIST_NAME_COLUMN,
                                             NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->expanded_list.tree_view), column);
	gtk_widget_show (self->priv->expanded_list.tree_view);

	/* expanded list, context menu */

	self->priv->expanded_list.popup_menu = gtk_menu_new ();
	menu_item = gtk_menu_item_new_with_mnemonic (_("_Delete"));
	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (delete_tag_activate_cb),
			  self);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->expanded_list.popup_menu), menu_item);

	/* popup window */

	self->priv->popup.window = gtk_window_new (GTK_WINDOW_POPUP);
	g_signal_connect (self->priv->popup.window,
			  "button-press-event",
			  G_CALLBACK (popup_window_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->popup.window,
			  "key-press-event",
			  G_CALLBACK (popup_window_key_press_event_cb),
			  self);

	self->priv->popup.container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (self->priv->popup.container);
	gtk_container_add (GTK_CONTAINER (self->priv->popup.window), self->priv->popup.container);

	/**/

	self->priv->expanded_list.container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (self), self->priv->expanded_list.container, TRUE, TRUE, 0);

	self->priv->expanded_list.scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
							    "hadjustment", NULL,
							    "vadjustment", NULL,
							    "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
							    "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
							    "shadow_type", GTK_SHADOW_IN,
							    NULL);
	gtk_widget_set_size_request (self->priv->expanded_list.scrolled_window, -1, EXPANDED_LIST_HEIGHT);
	gtk_box_pack_start (GTK_BOX (self->priv->expanded_list.container),
			    self->priv->expanded_list.scrolled_window,
			    TRUE,
			    TRUE,
			    0);
	gtk_container_add (GTK_CONTAINER (self->priv->expanded_list.scrolled_window),
			   self->priv->expanded_list.tree_view);
	gtk_widget_show_all (self->priv->expanded_list.scrolled_window);

	g_signal_connect (self->priv->expanded_list.scrolled_window,
			  "unmap",
			  G_CALLBACK (tag_list_unmap_cb),
			  self);

	self->priv->monitor_event = g_signal_connect (gth_main_get_default_monitor (),
						      "tags-changed",
						      G_CALLBACK (tags_changed_cb),
						      self);
	g_signal_connect (self->priv->entry,
			  "notify::text",
			  G_CALLBACK (text_changed_cb),
			  self);
	g_signal_connect (self->priv->expanded_list.tree_view,
			  "key-press-event",
			  G_CALLBACK (expanded_list_key_press_event_cb),
			  self);
	g_signal_connect (self->priv->expanded_list.tree_view,
			  "button-press-event",
			  G_CALLBACK (expanded_list_button_press_event_cb),
			  self);
	g_signal_connect (self,
			  "grab-broken-event",
			  G_CALLBACK (_gth_tags_entry_grab_broken_event),
			  self);

	/**/

	update_completion_list (self);
	update_expanded_list_from_entry (self);
}


GtkWidget *
gth_tags_entry_new (GthTagsEntryMode mode)
{
	return g_object_new (GTH_TYPE_TAGS_ENTRY, "mode", mode, NULL);
}


void
gth_tags_entry_set_list_visible (GthTagsEntry *self,
				 gboolean      visible)
{
	g_return_if_fail (GTH_IS_TAGS_ENTRY (self));

	self->priv->expanded = visible;

	switch (self->priv->mode) {
	case GTH_TAGS_ENTRY_MODE_INLINE:
		gtk_widget_set_size_request (self->priv->expanded_list.container, -1, self->priv->expanded ? -1 : EXPANDED_LIST_HEIGHT);
		gtk_widget_set_visible (self->priv->expand_button, ! self->priv->expanded);
		gtk_widget_set_visible (self->priv->expanded_list.container, self->priv->expanded || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->expand_button)));
		break;

	case GTH_TAGS_ENTRY_MODE_POPUP:
		popup_window_show (self);
		break;
	}
}


gboolean
gth_tags_entry_get_list_visible (GthTagsEntry *self)
{
	g_return_val_if_fail (GTH_IS_TAGS_ENTRY (self), FALSE);
	return self->priv->expanded;
}


void
gth_tags_entry_set_tags (GthTagsEntry  *self,
			 char         **tags)
{
	GthTagsFile *tags_file;
	GString     *valid_tags;
	int          i;
	gboolean     global_tags_changed = FALSE;

	if ((tags == NULL) || (tags[0] == NULL)) {
		gtk_entry_set_text (GTK_ENTRY (self->priv->entry), "");
		return;
	}

	valid_tags = g_string_new ("");
	tags_file = gth_main_get_default_tag_file ();
	for (i = 0; tags[i] != NULL; i++) {
		if (! _tag_is_valid (tags[i]))
			continue;
		g_string_append (valid_tags, tags[i]);
		g_string_append (valid_tags, ", ");
		if (gth_tags_file_add (tags_file, tags[i]))
			global_tags_changed = TRUE;
	}
	if (global_tags_changed)
		gth_main_tags_changed ();

	gtk_entry_set_text (GTK_ENTRY (self->priv->entry), valid_tags->str);

	g_string_free (valid_tags, TRUE);
}


void
gth_tags_entry_set_tags_from_text (GthTagsEntry *self,
				   const char   *text)
{
	char **tags;
	int    i;

	if ((text == NULL) || (strcmp (text, "") == 0)) {
		gth_tags_entry_set_tags (self, NULL);
		return;
	}

	tags = g_strsplit (text, ",", -1);
	for (i = 0; tags[i] != NULL; i++)
		tags[i] = g_strstrip (tags[i]);
	gth_tags_entry_set_tags (self, tags);

	g_strfreev (tags);
}


char **
gth_tags_entry_get_tags (GthTagsEntry *self,
			 gboolean      update_globals)
{
	GthTagsFile  *tags_file;
	char        **all_tags;
	char        **tags;
	int           i;
	int           j;

	tags_file = gth_main_get_default_tag_file ();

	all_tags = g_strsplit (gtk_entry_get_text (GTK_ENTRY (self->priv->entry)), ",", -1);
	tags = g_new0 (char *, g_strv_length (all_tags) + 1);
	for (i = 0, j = 0; all_tags[i] != NULL; i++) {
		if (! _tag_is_valid (all_tags[i]))
			continue;
		tags[j] = g_strdup (g_strstrip (all_tags[i]));
		if (update_globals)
			gth_tags_file_add (tags_file, tags[j]);
		j++;
	}
	g_strfreev (all_tags);

	if (update_globals) {
		for (i = 0; self->priv->tags[i] != NULL; i++)
			gth_tags_file_add (tags_file, self->priv->tags[i]);
		gth_main_tags_changed ();
	}

	return tags;
}


void
gth_tags_entry_set_tag_list (GthTagsEntry *self,
			     GList        *checked,
			     GList        *inconsistent)
{
	GString *str;
	GList   *scan;

	g_hash_table_remove_all (self->priv->inconsistent);
	for (scan = inconsistent; scan; scan = scan->next) {
		char *tag = scan->data;
		if (! _tag_is_valid (tag))
			continue;
		g_hash_table_insert (self->priv->inconsistent, g_strdup (tag), GINT_TO_POINTER (1));
	}

	str = g_string_new ("");
	for (scan = checked; scan; scan = scan->next) {
		char *tag = scan->data;
		if (! _tag_is_valid (tag))
			continue;
		g_string_append (str, tag);
		g_string_append (str, ", ");
	}
	gth_tags_entry_set_tags_from_text (self, str->str);

	if (checked == NULL)
		update_expanded_list_from_entry (self);

	g_string_free (str, TRUE);
}


void
gth_tags_entry_get_tag_list (GthTagsEntry  *self,
		             gboolean       update_globals,
			     GList        **checked,
			     GList        **inconsistent)
{
	if (checked != NULL) {
		char **tags_v;
		int    i;

		tags_v = gth_tags_entry_get_tags (self, update_globals);
		*checked = NULL;
		for (i = 0; tags_v[i] != NULL; i++)
			if (_tag_is_valid(tags_v[i]))
				*checked = g_list_prepend (*checked, g_strdup (tags_v[i]));
		*checked = g_list_reverse (*checked);
	}

	if (inconsistent != NULL)
		*inconsistent = g_hash_table_get_keys (self->priv->inconsistent);
}
