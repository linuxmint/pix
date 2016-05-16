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
#include "gth-main.h"
#include "gth-multipage.h"


#define GTH_MULTIPAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_MULTIPAGE, GthMultipagePrivate))

enum {
	CHANGED,
	LAST_SIGNAL
};


enum {
	ICON_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};


static guint gth_multipage_signals[LAST_SIGNAL] = { 0 };


struct _GthMultipagePrivate {
	GtkListStore *model;
	GtkWidget    *combobox;
	GtkWidget    *notebook;
	GList        *children;
};


G_DEFINE_TYPE (GthMultipage, gth_multipage, GTK_TYPE_BOX)


static void
gth_multipage_finalize (GObject *object)
{
	GthMultipage *multipage;

	multipage = GTH_MULTIPAGE (object);

	g_list_free (multipage->priv->children);

	G_OBJECT_CLASS (gth_multipage_parent_class)->finalize (object);
}


static void
gth_multipage_class_init (GthMultipageClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthMultipagePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_multipage_finalize;

	/* signals */

	gth_multipage_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMultipageClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
combobox_changed_cb (GtkComboBox *widget,
		     gpointer     user_data)
{
	GthMultipage *multipage = user_data;

	gtk_notebook_set_current_page (GTK_NOTEBOOK (multipage->priv->notebook),
				       gtk_combo_box_get_active (GTK_COMBO_BOX (multipage->priv->combobox)));
	g_signal_emit (G_OBJECT (multipage), gth_multipage_signals[CHANGED], 0);
}


static void
multipage_realize_cb (GtkWidget *widget,
		      gpointer   user_data)
{
	GthMultipage *multipage = user_data;
	GtkWidget    *orientable_parent;

	orientable_parent = gtk_widget_get_parent (widget);
	while ((orientable_parent != NULL ) && ! GTK_IS_ORIENTABLE (orientable_parent)) {
		orientable_parent = gtk_widget_get_parent (orientable_parent);
	}

	if (orientable_parent == NULL)
		return;

	switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (orientable_parent))) {
	case GTK_ORIENTATION_HORIZONTAL:
		gtk_box_set_spacing (GTK_BOX (multipage), 0);
		gtk_widget_set_margin_top (multipage->priv->combobox, 4);
		gtk_widget_set_margin_bottom (multipage->priv->combobox, 4);
		break;

	case GTK_ORIENTATION_VERTICAL:
		gtk_box_set_spacing (GTK_BOX (multipage), 6);
		gtk_widget_set_margin_top (multipage->priv->combobox, 0);
		gtk_widget_set_margin_bottom (multipage->priv->combobox, 0);
		break;
	}
}


static void
gth_multipage_init (GthMultipage *multipage)
{
	GtkCellRenderer *renderer;

	multipage->priv = GTH_MULTIPAGE_GET_PRIVATE (multipage);
	multipage->priv->children = NULL;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (multipage), GTK_ORIENTATION_VERTICAL);

	g_signal_connect (multipage,
			  "realize",
			  G_CALLBACK (multipage_realize_cb),
			  multipage);
	multipage->priv->model = gtk_list_store_new (N_COLUMNS,
						     G_TYPE_STRING,
						     G_TYPE_STRING);
	multipage->priv->combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (multipage->priv->model));
	gtk_widget_show (multipage->priv->combobox);
	gtk_box_pack_start (GTK_BOX (multipage), multipage->priv->combobox, FALSE, FALSE, 0);
	g_object_unref (multipage->priv->model);

	g_signal_connect (multipage->priv->combobox,
			  "changed",
			  G_CALLBACK (combobox_changed_cb),
			  multipage);

	/* icon renderer */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (multipage->priv->combobox),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (multipage->priv->combobox),
					 renderer,
					 "icon-name", ICON_COLUMN,
					 NULL);

	/* name renderer */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (multipage->priv->combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (multipage->priv->combobox),
					 renderer,
					 "text", NAME_COLUMN,
					 NULL);

	/* notebook */

	multipage->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (multipage->priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (multipage->priv->notebook), FALSE);
	gtk_widget_show (multipage->priv->notebook);
	gtk_box_pack_start (GTK_BOX (multipage), multipage->priv->notebook, TRUE, TRUE, 0);
}


GtkWidget *
gth_multipage_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_MULTIPAGE, NULL);
}


void
gth_multipage_add_child (GthMultipage      *multipage,
			 GthMultipageChild *child)
{
	GtkWidget   *box;
	GtkTreeIter  iter;

	multipage->priv->children = g_list_append (multipage->priv->children, child);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (child), TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET (child));
	gtk_widget_show (box);
	gtk_notebook_append_page (GTK_NOTEBOOK (multipage->priv->notebook), box, NULL);

	gtk_list_store_append (GTK_LIST_STORE (multipage->priv->model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (multipage->priv->model), &iter,
			    NAME_COLUMN, gth_multipage_child_get_name (child),
			    ICON_COLUMN, gth_multipage_child_get_icon (child),
			    -1);
}


GList *
gth_multipage_get_children (GthMultipage *multipage)
{
	return multipage->priv->children;
}


void
gth_multipage_set_current (GthMultipage *multipage,
			   int           index_)
{
	gtk_combo_box_set_active (GTK_COMBO_BOX (multipage->priv->combobox), index_);
}


int
gth_multipage_get_current (GthMultipage *multipage)
{
	return gtk_combo_box_get_active (GTK_COMBO_BOX (multipage->priv->combobox));
}


/* -- gth_multipage_child -- */


G_DEFINE_INTERFACE (GthMultipageChild, gth_multipage_child, 0)


static void
gth_multipage_child_default_init (GthMultipageChildInterface *iface)
{
	/* void */
}


const char *
gth_multipage_child_get_name (GthMultipageChild *self)
{
	return GTH_MULTIPAGE_CHILD_GET_INTERFACE (self)->get_name (self);
}


const char *
gth_multipage_child_get_icon (GthMultipageChild *self)
{
	return GTH_MULTIPAGE_CHILD_GET_INTERFACE (self)->get_icon (self);
}
