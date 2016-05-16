/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include "gth-main.h"
#include "gth-tags-entry.h"
#include "gth-tags-file.h"


#define EXPANDED_LIST_HEIGHT 160


enum {
	COMPLETION_NAME_COLUMN,
	COMPLETION_N_COLUMNS
};


enum {
	EXPANDED_LIST_USED_COLUMN,
	EXPANDED_LIST_INCONSISTENT_COLUMN,
	EXPANDED_LIST_SEPARATOR_COLUMN,
	EXPANDED_LIST_NAME_COLUMN,
	EXPANDED_LIST_N_COLUMNS
};


/* Signals */
enum {
	LIST_COLLAPSED,
	LAST_SIGNAL
};


typedef struct {
	char     *name;
	gboolean  used;
	gboolean  suggested;
	gboolean  inconsistent;
} TagData;


typedef struct  {
	char         **last_used;
	GtkWidget     *container;
	GtkWidget     *tree_view;
	GtkListStore  *store;
	GtkWidget     *popup_menu;
} ExpandedList;


struct _GthTagsEntryPrivate {
	GtkWidget           *entry;
	GtkWidget           *expand_button;
	ExpandedList         expanded_list;
	gboolean             expanded;
	char               **tags;
	GtkEntryCompletion  *completion;
	GtkListStore        *completion_store;
	char                *new_tag;
	gboolean             action_create;
	gulong               monitor_event;
	GHashTable          *inconsistent;
};


static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GthTagsEntry, gth_tags_entry, GTK_TYPE_BOX)


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
	g_hash_table_unref (self->priv->inconsistent);

	G_OBJECT_CLASS (gth_tags_entry_parent_class)->finalize (obj);
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

	g_type_class_add_private (klass, sizeof (GthTagsEntryPrivate));

	object_class = (GObjectClass*) (klass);
	object_class->finalize = gth_tags_entry_finalize;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->grab_focus = gth_tags_entry_grab_focus;

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

	if (tag_data_a->used && tag_data_b->used)
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
	else if (tag_data_a->used || tag_data_b->used)
		return tag_data_a->used ? -1 : 1;
	else if (tag_data_a->suggested && tag_data_b->suggested)
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
	else if (tag_data_a->suggested || tag_data_b->suggested)
		return tag_data_a->suggested ? -1 : 1;
	else
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
}


static void
update_expanded_list_from_entry (GthTagsEntry *self)
{
	char        **all_tags;
	char        **used_tags;
	TagData     **tag_data;
	int           i;
	GtkTreeIter   iter;
	gboolean      separator_required;

	all_tags = g_strdupv (self->priv->tags);
	used_tags = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (self), FALSE);

	tag_data = g_new (TagData *, g_strv_length (all_tags) + 1);
	for (i = 0; all_tags[i] != NULL; i++) {
		int j;

		tag_data[i] = g_new0 (TagData, 1);
		tag_data[i]->name = g_strdup (all_tags[i]);
		tag_data[i]->used = FALSE;
		tag_data[i]->inconsistent = (g_hash_table_lookup (self->priv->inconsistent, tag_data[i]->name) != NULL);
		tag_data[i]->suggested = tag_data[i]->inconsistent;
		for (j = 0; ! tag_data[i]->used && (used_tags[j] != NULL); j++)
			if (g_utf8_collate (tag_data[i]->name, used_tags[j]) == 0) {
				tag_data[i]->used = TRUE;
				tag_data[i]->inconsistent = FALSE;
				tag_data[i]->suggested = FALSE;
			}

		if (! tag_data[i]->used)
			for (j = 0; ! tag_data[i]->suggested && (self->priv->expanded_list.last_used[j] != NULL); j++)
				if (g_utf8_collate (tag_data[i]->name, self->priv->expanded_list.last_used[j]) == 0)
					tag_data[i]->suggested = TRUE;
	}
	tag_data[i] = NULL;

	g_qsort_with_data (tag_data,
			   g_strv_length (all_tags),
			   sizeof (TagData *),
			   sort_tag_data,
			   NULL);

	gtk_list_store_clear (self->priv->expanded_list.store);

	/* used */

	separator_required = FALSE;
	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (! tag_data[i]->used)
			continue;

		separator_required = TRUE;

		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
				    EXPANDED_LIST_USED_COLUMN, TRUE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, tag_data[i]->inconsistent,
				    EXPANDED_LIST_SEPARATOR_COLUMN, FALSE,
				    EXPANDED_LIST_NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	if (separator_required) {
		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
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
				    EXPANDED_LIST_USED_COLUMN, FALSE,
				    EXPANDED_LIST_INCONSISTENT_COLUMN, tag_data[i]->inconsistent,
				    EXPANDED_LIST_SEPARATOR_COLUMN, FALSE,
				    EXPANDED_LIST_NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	if (separator_required) {
		gtk_list_store_append (self->priv->expanded_list.store, &iter);
		gtk_list_store_set (self->priv->expanded_list.store, &iter,
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

		action_text = g_strdup_printf (_("Create tag «%s»"), self->priv->new_tag);
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
	gth_tags_entry_set_tags (GTH_TAGS_ENTRY (self), tags);

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
expand_button_toggled_cb (GtkToggleButton *button,
			  gpointer         user_data)
{
	GthTagsEntry *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gtk_widget_show (self->priv->expanded_list.container);
	else
		gtk_widget_hide (self->priv->expanded_list.container);
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

	if (event->button == 3) {
		gtk_menu_popup (GTK_MENU (self->priv->expanded_list.popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				event->button,
				event->time);
		return FALSE;
	}

	return FALSE;
}


static void
gth_tags_entry_init (GthTagsEntry *self)
{
	GtkWidget         *hbox;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;
	GtkWidget         *menu_item;

	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TAGS_ENTRY, GthTagsEntryPrivate);
	self->priv->expanded_list.last_used = g_new0 (char *, 1);
	self->priv->expanded = FALSE;
	self->priv->inconsistent = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	gtk_box_set_spacing (GTK_BOX (self), 3);

	/* entry / expander button box */

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
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

	/* expand button */

	self->priv->expand_button = gtk_toggle_button_new ();
	gtk_widget_set_tooltip_text (self->priv->expand_button, _("Show all the tags"));
	gtk_container_add (GTK_CONTAINER (self->priv->expand_button), gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
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
	g_object_set (renderer, "icon-name", "tag", NULL);

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", EXPANDED_LIST_NAME_COLUMN,
                                             NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->expanded_list.tree_view), column);
	gtk_widget_show (self->priv->expanded_list.tree_view);

	/* expanded list, popup menu */

	self->priv->expanded_list.popup_menu = gtk_menu_new ();
	menu_item = gtk_menu_item_new_with_mnemonic (_("_Delete"));
	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (delete_tag_activate_cb),
			  self);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->expanded_list.popup_menu), menu_item);

	/**/

	self->priv->expanded_list.container = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
							    "hadjustment", NULL,
							    "vadjustment", NULL,
							    "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
							    "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
							    "shadow_type", GTK_SHADOW_IN,
							    NULL);
	gtk_widget_set_size_request (self->priv->expanded_list.container, -1, EXPANDED_LIST_HEIGHT);
	gtk_container_add (GTK_CONTAINER (self->priv->expanded_list.container), self->priv->expanded_list.tree_view);
	g_signal_connect (self->priv->expanded_list.container, "unmap", G_CALLBACK (tag_list_unmap_cb), self);
	gtk_box_pack_start (GTK_BOX (self), self->priv->expanded_list.container, TRUE, TRUE, 0);

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
}


GtkWidget *
gth_tags_entry_new (void)
{
	GthTagsEntry *self;

	self = g_object_new (GTH_TYPE_TAGS_ENTRY, NULL);
	update_completion_list (self);
	update_expanded_list_from_entry (self);

	return (GtkWidget *) self;
}


void
gth_tags_entry_set_expanded (GthTagsEntry *self,
			     gboolean      expanded)
{
	g_return_if_fail (GTH_IS_TAGS_ENTRY (self));

	self->priv->expanded = expanded;
	gtk_widget_set_size_request (self->priv->expanded_list.container, -1, self->priv->expanded ? -1 : EXPANDED_LIST_HEIGHT);
	gtk_widget_set_visible (self->priv->expand_button, ! expanded);
	gtk_widget_set_visible (self->priv->expanded_list.container, expanded || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->expand_button)));
}


gboolean
gth_tags_entry_get_expanded (GthTagsEntry *self)
{
	g_return_val_if_fail (GTH_IS_TAGS_ENTRY (self), FALSE);
	return self->priv->expanded;
}


void
gth_tags_entry_set_tags (GthTagsEntry  *self,
			 char         **tags)
{
	GthTagsFile *tags_file;
	int          i;
	gboolean     global_tags_changed = FALSE;
	char        *s;

	if ((tags == NULL) || (tags[0] == NULL)) {
		gtk_entry_set_text (GTK_ENTRY (self->priv->entry), "");
		return;
	}

	tags_file = gth_main_get_default_tag_file ();
	for (i = 0; tags[i] != NULL; i++)
		if (gth_tags_file_add (tags_file, tags[i]))
			global_tags_changed = TRUE;
	if (global_tags_changed)
		gth_main_tags_changed ();

	s = g_strjoinv (", ", tags);
	gtk_entry_set_text (GTK_ENTRY (self->priv->entry), s);
	g_free (s);
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
		all_tags[i] = g_strstrip (all_tags[i]);
		if (all_tags[i][0] != '\0') {
			tags[j] = g_strdup (g_strstrip (all_tags[i]));
			if (update_globals)
				gth_tags_file_add (tags_file, tags[j]);
			j++;
		}
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
	for (scan = inconsistent; scan; scan = scan->next)
		g_hash_table_insert (self->priv->inconsistent, g_strdup (scan->data), GINT_TO_POINTER (1));

	str = g_string_new ("");
	for (scan = checked; scan; scan = scan->next) {
		if (scan != checked)
			g_string_append (str, ", ");
		g_string_append (str, (char *) scan->data);
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
			*checked = g_list_prepend (*checked, g_strdup (tags_v[i]));
		*checked = g_list_reverse (*checked);
	}

	if (inconsistent != NULL)
		*inconsistent = g_hash_table_get_keys (self->priv->inconsistent);
}
