/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-search-source-selector.h"


enum {
	ADD_SOURCE,
	REMOVE_SOURCE,
	LAST_SIGNAL
};


struct _GthSearchSourceSelectorPrivate {
	GtkWidget *location_chooser;
	GtkWidget *recursive_checkbutton;
	GtkWidget *add_button;
	GtkWidget *remove_button;
};


static guint gth_search_source_selector_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthSearchSourceSelector,
			 gth_search_source_selector,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthSearchSourceSelector))


static void
gth_search_source_selector_class_init (GthSearchSourceSelectorClass *klass)
{
	/* signals */

	gth_search_source_selector_signals[ADD_SOURCE] =
		g_signal_new ("add-source",
			      G_TYPE_FROM_CLASS (klass),
 			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthSearchSourceSelectorClass, add_source),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_search_source_selector_signals[REMOVE_SOURCE] =
		g_signal_new ("remove-source",
			      G_TYPE_FROM_CLASS (klass),
 			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthSearchSourceSelectorClass, remove_source),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_search_source_selector_init (GthSearchSourceSelector *self)
{
	self->priv = gth_search_source_selector_get_instance_private (self);
	self->priv->location_chooser = NULL;
	self->priv->recursive_checkbutton = NULL;
	self->priv->add_button = NULL;
	self->priv->remove_button = NULL;
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
}


static void
add_button_clicked_cb (GtkButton               *button,
		       GthSearchSourceSelector *self)
{
	g_signal_emit (self, gth_search_source_selector_signals[ADD_SOURCE], 0);
}


static void
remove_button_clicked_cb (GtkButton               *button,
			  GthSearchSourceSelector *self)
{
	g_signal_emit (self, gth_search_source_selector_signals[REMOVE_SOURCE], 0);
}


static void
gth_search_source_selector_construct (GthSearchSourceSelector *self)
{
	GtkWidget *vbox;
	GtkWidget *hbox;

	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 0);

	self->priv->location_chooser = g_object_new (GTH_TYPE_LOCATION_CHOOSER,
						     "show-entry-points", TRUE,
						     "show-other", TRUE,
						     "relief", GTK_RELIEF_NORMAL,
						     NULL);
	gtk_widget_show (self->priv->location_chooser);

	self->priv->recursive_checkbutton = gtk_check_button_new_with_mnemonic(_("_Include sub-folders"));
	gtk_widget_show (self->priv->recursive_checkbutton);

	/* add/remove buttons */

	self->priv->add_button = gtk_button_new_from_icon_name ("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_relief (GTK_BUTTON (self->priv->add_button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (self->priv->add_button, _("Add another location"));
	gtk_widget_show_all (self->priv->add_button);

	g_signal_connect (G_OBJECT (self->priv->add_button),
			  "clicked",
			  G_CALLBACK (add_button_clicked_cb),
			  self);

	self->priv->remove_button = gtk_button_new_from_icon_name ("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_relief (GTK_BUTTON (self->priv->remove_button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (self->priv->remove_button, _("Remove"));
	gtk_widget_show_all (self->priv->remove_button);

	g_signal_connect (G_OBJECT (self->priv->remove_button),
			  "clicked",
			  G_CALLBACK (remove_button_clicked_cb),
			  self);

	/**/

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (hbox), self->priv->location_chooser, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), self->priv->recursive_checkbutton, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), vbox, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (self), self->priv->add_button, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (self), self->priv->remove_button, FALSE, FALSE, 0);
}


GtkWidget *
gth_search_source_selector_new (void)
{
	GthSearchSourceSelector *self;

	self = g_object_new (GTH_TYPE_SEARCH_SOURCE_SELECTOR, NULL);
	gth_search_source_selector_construct (self);
	gth_search_source_selector_set_source (self, NULL);

	return (GtkWidget *) self;
}


GtkWidget *
gth_search_source_selector_new_with_source (GthSearchSource *source)
{
	GthSearchSourceSelector *self;

	self = g_object_new (GTH_TYPE_SEARCH_SOURCE_SELECTOR, NULL);
	gth_search_source_selector_construct (self);
	gth_search_source_selector_set_source (self, source);

	return (GtkWidget *) self;
}


void
gth_search_source_selector_set_source (GthSearchSourceSelector *self,
				       GthSearchSource         *source)
{
	GFile    *folder;
	gboolean  recursive;

	if (source != NULL) {
		folder = _g_object_ref (gth_search_source_get_folder (source));
		recursive = gth_search_source_is_recursive (source);
	}
	else {
		folder = NULL;
		recursive = TRUE;
	}

	if (folder == NULL)
		folder = g_file_new_for_uri (_g_uri_get_home ());

	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (self->priv->location_chooser), folder);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->recursive_checkbutton), recursive);
}


GthSearchSource *
gth_search_source_selector_get_source (GthSearchSourceSelector *self)
{
	GthSearchSource *source;

	source = gth_search_source_new ();
	gth_search_source_set_folder (source, gth_location_chooser_get_current (GTH_LOCATION_CHOOSER (self->priv->location_chooser)));
	gth_search_source_set_recursive (source, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->recursive_checkbutton)));

	return source;
}


void
gth_search_source_selector_can_remove (GthSearchSourceSelector *self,
				       gboolean                 value)
{
	gtk_widget_set_sensitive (self->priv->remove_button, value);
}


void
gth_search_source_selector_focus (GthSearchSourceSelector *self)
{
	gtk_widget_grab_focus (self->priv->location_chooser);
}
