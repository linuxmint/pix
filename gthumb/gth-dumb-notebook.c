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
#include "gth-dumb-notebook.h"


struct _GthDumbNotebookPrivate {
	GList     *children;
	int        n_children;
	GtkWidget *current;
	int        current_pos;
};


G_DEFINE_TYPE (GthDumbNotebook, gth_dumb_notebook, GTK_TYPE_CONTAINER)


static void
gth_dumb_notebook_finalize (GObject *object)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (object);
	
	if (dumb_notebook->priv != NULL) {
		g_free (dumb_notebook->priv);
		dumb_notebook->priv = NULL;
	}
	
	G_OBJECT_CLASS (gth_dumb_notebook_parent_class)->finalize (object);
}


static GtkSizeRequestMode
gth_dumb_notebook_get_request_mode (GtkWidget *widget)
{
	return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}


static void
gth_dumb_notebook_get_preferred_height (GtkWidget *widget,
                			int       *minimum_height,
                			int       *natural_height)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (widget);
	int              border_width;

	*minimum_height = 0;
	*natural_height = 0;

	if (dumb_notebook->priv->current != NULL) {
		int child_minimum_height;
		int child_natural_height;
		
		gtk_widget_get_preferred_height (dumb_notebook->priv->current,
						 &child_minimum_height,
						 &child_natural_height);
		*minimum_height = MAX (*minimum_height, child_minimum_height);
		*natural_height = MAX (*natural_height, child_natural_height);
	}
	
	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	*minimum_height += border_width * 2;
	*natural_height += border_width * 2;
}


static void
gth_dumb_notebook_get_preferred_width_for_height (GtkWidget *widget,
                				  int        height,
                				  int       *minimum_width,
                				  int       *natural_width)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (widget);
	int              border_width;

	*minimum_width = 0;
	*natural_width = 0;

	if (dumb_notebook->priv->current != NULL) {
		int child_minimum_width;
		int child_natural_width;

		gtk_widget_get_preferred_width_for_height (dumb_notebook->priv->current,
							   height,
							   &child_minimum_width,
							   &child_natural_width);
		*minimum_width = MAX (*minimum_width, child_minimum_width);
		*natural_width = MAX (*natural_width, child_natural_width);
	}

	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	*minimum_width += border_width * 2;
	*natural_width += border_width * 2;
}


static void
gth_dumb_notebook_get_preferred_width (GtkWidget *widget,
                		       int       *minimum_width,
                		       int       *natural_width)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (widget);
	int              border_width;

	*minimum_width = 0;
	*natural_width = 0;

	if (dumb_notebook->priv->current != NULL) {
		int child_minimum_width;
		int child_natural_width;

		gtk_widget_get_preferred_width (dumb_notebook->priv->current,
						&child_minimum_width,
						&child_natural_width);
		*minimum_width = MAX (*minimum_width, child_minimum_width);
		*natural_width = MAX (*natural_width, child_natural_width);
	}

	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	*minimum_width += border_width * 2;
	*natural_width += border_width * 2;
}


static void
gth_dumb_notebook_get_preferred_height_for_width (GtkWidget *widget,
                				  int        width,
                				  int       *minimum_height,
                				  int       *natural_height)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (widget);
	int              border_width;

	*minimum_height = 0;
	*natural_height = 0;

	if (dumb_notebook->priv->current != NULL) {
		int child_minimum_height;
		int child_natural_height;

		gtk_widget_get_preferred_height_for_width (dumb_notebook->priv->current,
							   width,
							   &child_minimum_height,
							   &child_natural_height);
		*minimum_height = MAX (*minimum_height, child_minimum_height);
		*natural_height = MAX (*natural_height, child_natural_height);
	}

	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	*minimum_height += border_width * 2;
	*natural_height += border_width * 2;
}


static void
gth_dumb_notebook_size_allocate (GtkWidget     *widget,
				 GtkAllocation *allocation)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (widget);
	
	if (dumb_notebook->priv->current != NULL) {
		int           border_width;
		GtkAllocation child_allocation;

		border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
		child_allocation.x = allocation->x + border_width;
		child_allocation.y = allocation->y + border_width;
		child_allocation.width = MAX (1, allocation->width - border_width * 2);
		child_allocation.height = MAX (1, allocation->height - border_width * 2);

		gtk_widget_size_allocate (dumb_notebook->priv->current, &child_allocation);
	}

	gtk_widget_set_allocation (widget, allocation);
}


static gboolean
gth_dumb_notebook_draw (GtkWidget *widget,
			cairo_t   *cr)
{
	GthDumbNotebook *dumb_notebook = GTH_DUMB_NOTEBOOK (widget);
	
	if ((dumb_notebook->priv->current != NULL) && gtk_widget_get_child_visible (dumb_notebook->priv->current))
		gtk_container_propagate_draw (GTK_CONTAINER (widget),
					      dumb_notebook->priv->current,
					      cr);

	return FALSE;
}


static void
_gth_dumb_notebook_grab_focus_child (GthDumbNotebook *notebook)
{
	GtkWidget *child;

	child = notebook->priv->current;
	if (child == NULL)
		return;

	if (GTK_IS_SCROLLED_WINDOW (notebook->priv->current)) {
		GList *list = gtk_container_get_children (GTK_CONTAINER (notebook->priv->current));
		if (list != NULL)
			child = list->data;
	}

	gtk_widget_grab_focus (child);
}


static void
gth_dumb_notebook_grab_focus (GtkWidget *widget)
{
	_gth_dumb_notebook_grab_focus_child (GTH_DUMB_NOTEBOOK (widget));
}


static void
gth_dumb_notebook_add (GtkContainer *container,
		       GtkWidget    *child)
{
	GthDumbNotebook *notebook;
	
	notebook = GTH_DUMB_NOTEBOOK (container);
	
	gtk_widget_freeze_child_notify (child);
	
	notebook->priv->children = g_list_append (notebook->priv->children, child);
	notebook->priv->n_children++;
	if (notebook->priv->current_pos == notebook->priv->n_children - 1) {
		gtk_widget_set_child_visible (child, TRUE);
		notebook->priv->current = child;
	}
	else
		gtk_widget_set_child_visible (child, FALSE);
	gtk_widget_set_parent (child, GTK_WIDGET (notebook));

	gtk_widget_thaw_child_notify (child);
}


static void
gth_dumb_notebook_remove (GtkContainer *container,
			  GtkWidget    *child)
{
	GthDumbNotebook *notebook;
	GList           *link;

	notebook = GTH_DUMB_NOTEBOOK (container);

	link = g_list_find (notebook->priv->children, child);
	if (link == NULL)
		return;

	gtk_widget_unparent (child);

	notebook->priv->children = g_list_remove_link (notebook->priv->children, link);
	notebook->priv->n_children--;
	g_list_free (link);
}


static void
gth_dumb_notebook_forall (GtkContainer *container,
			  gboolean      include_internals,
			  GtkCallback   callback,
			  gpointer      callback_data)
{
	GthDumbNotebook *notebook;
	GList           *scan;
	
	notebook = GTH_DUMB_NOTEBOOK (container);
	
	for (scan = notebook->priv->children; scan; /* void */) {
		GtkWidget *child = scan->data;

		scan = scan->next;
		(* callback) (child, callback_data);
	}
}


static GType
gth_dumb_notebook_child_type (GtkContainer *container)
{
	return GTK_TYPE_WIDGET;
}


static void 
gth_dumb_notebook_class_init (GthDumbNotebookClass *klass) 
{
	GObjectClass      *gobject_class;
	GtkWidgetClass    *widget_class;
	GtkContainerClass *container_class;
	
	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gth_dumb_notebook_finalize;
	
	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->get_request_mode = gth_dumb_notebook_get_request_mode;
	widget_class->get_preferred_height = gth_dumb_notebook_get_preferred_height;
	widget_class->get_preferred_width_for_height = gth_dumb_notebook_get_preferred_width_for_height;
	widget_class->get_preferred_width = gth_dumb_notebook_get_preferred_width;
	widget_class->get_preferred_height_for_width = gth_dumb_notebook_get_preferred_height_for_width;
	widget_class->size_allocate = gth_dumb_notebook_size_allocate;
	widget_class->draw = gth_dumb_notebook_draw;
	widget_class->grab_focus = gth_dumb_notebook_grab_focus;

	container_class = GTK_CONTAINER_CLASS (klass);
	container_class->add = gth_dumb_notebook_add;
	container_class->remove = gth_dumb_notebook_remove;
	container_class->forall = gth_dumb_notebook_forall;
	container_class->child_type = gth_dumb_notebook_child_type;
}


static void
gth_dumb_notebook_init (GthDumbNotebook *notebook) 
{
	gtk_widget_set_has_window (GTK_WIDGET (notebook), FALSE);
	gtk_widget_set_can_focus (GTK_WIDGET (notebook), TRUE);
	
	notebook->priv = g_new0 (GthDumbNotebookPrivate, 1);
	notebook->priv->n_children = 0;	
	notebook->priv->current_pos = 0;
}


GtkWidget *
gth_dumb_notebook_new (void) 
{
	return g_object_new (GTH_TYPE_DUMB_NOTEBOOK, NULL);
}


void
gth_dumb_notebook_show_child (GthDumbNotebook *notebook,
			      int              pos)
{
	GList    *link;
	gboolean  current_is_focus;
	
	link = g_list_nth (notebook->priv->children, pos);
	if (link == NULL)
		return;

	current_is_focus = (notebook->priv->current != NULL) && gtk_widget_has_focus (notebook->priv->current);

	if (notebook->priv->current != link->data)
		gtk_widget_set_child_visible (notebook->priv->current, FALSE);

	notebook->priv->current_pos = pos;
	notebook->priv->current = link->data;
	gtk_widget_set_child_visible (notebook->priv->current, TRUE);
	if (current_is_focus)
		_gth_dumb_notebook_grab_focus_child (notebook);

	gtk_widget_queue_resize (GTK_WIDGET (notebook));
}
