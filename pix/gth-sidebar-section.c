/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
#include "gth-property-view.h"
#include "gth-sidebar-section.h"
#include "gtk-utils.h"


struct _GthSidebarSectionPrivate {
	GtkWidget       *void_view;
	GtkWidget       *expander;
	GthPropertyView *view;
	GthFileData     *file_data;
	gboolean         dirty;
};


G_DEFINE_TYPE_WITH_CODE (GthSidebarSection,
			 gth_sidebar_section,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthSidebarSection))


static void
gth_sidebar_section_finalize (GObject *object)
{
	GthSidebarSection *self = GTH_SIDEBAR_SECTION (object);

	_g_object_unref (self->priv->file_data);
	G_OBJECT_CLASS (gth_sidebar_section_parent_class)->finalize (object);
}


static void
gth_sidebar_section_class_init (GthSidebarSectionClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;
	object_class->finalize = gth_sidebar_section_finalize;
}


static void
gth_sidebar_section_init (GthSidebarSection *self)
{
	self->priv = gth_sidebar_section_get_instance_private (self);
	self->priv->void_view = NULL;
	self->priv->expander = NULL;
	self->priv->view = NULL;
	self->priv->dirty = FALSE;
}


static void
_gth_sidebar_section_update_view (GthSidebarSection *self)
{
	gth_property_view_set_file (self->priv->view, self->priv->file_data);
	self->priv->dirty = FALSE;
}


static void
expander_expanded_cb (GObject    *object,
		      GParamSpec *param_spec,
		      gpointer    user_data)
{
	GthSidebarSection *self = user_data;

	if (gtk_expander_get_expanded (GTK_EXPANDER (self->priv->expander))) {
		if (self->priv->dirty)
			_gth_sidebar_section_update_view (self);
	}
}


static void
_gth_sidebar_section_add_view (GthSidebarSection *self,
			       GthPropertyView   *view)
{
	GtkWidget *exp;
	GtkWidget *exp_label;
	GtkWidget *exp_content;
	GtkWidget *icon;
	GtkWidget *label;

	self->priv->view = view;
	_gtk_widget_set_margin (GTK_WIDGET (view), 1, 2, 1, 2);
	gtk_widget_show (GTK_WIDGET (view));

	self->priv->expander = exp = gtk_expander_new (NULL);
	exp_label = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	_gtk_widget_set_margin (GTK_WIDGET (exp_label), 3, 2, 3, 2);
	icon = gtk_image_new_from_icon_name (gth_property_view_get_icon (view), GTK_ICON_SIZE_MENU);
	label = gtk_label_new (gth_property_view_get_name (view));
	gtk_box_pack_start (GTK_BOX (exp_label), icon, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (exp_label), label, FALSE, FALSE, 0);

	gtk_widget_show_all (exp_label);
	gtk_expander_set_label_widget (GTK_EXPANDER (exp), exp_label);

	self->priv->void_view = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (self->priv->void_view), GTK_SHADOW_IN);
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->void_view), GTK_STYLE_CLASS_VIEW);
	_gtk_widget_set_margin (self->priv->void_view, 0, 2, 0, 2);
	gtk_widget_hide (self->priv->void_view);

	icon = gtk_image_new_from_icon_name ("action-unavailable-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_set_sensitive (icon, FALSE);
	_gtk_widget_set_margin (icon, 10, 0, 10, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (icon), "void-view");
	gtk_widget_show (icon);
	gtk_container_add (GTK_CONTAINER (self->priv->void_view), icon);

	exp_content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (exp_content), GTK_WIDGET (view), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (exp_content), self->priv->void_view, FALSE, FALSE, 0);
	gtk_widget_show (exp_content);

	gtk_expander_set_expanded (GTK_EXPANDER (exp), TRUE);
	gtk_container_add (GTK_CONTAINER (exp), exp_content);
	gtk_widget_show (exp);
	gtk_box_pack_start (GTK_BOX (self), exp, FALSE, FALSE, 0);

	g_signal_connect (exp,
			  "notify::expanded",
			  G_CALLBACK (expander_expanded_cb),
			  self);
}


GtkWidget *
gth_sidebar_section_new (GthPropertyView *view)
{
	GthSidebarSection *sidebar;

	sidebar = g_object_new (GTH_TYPE_SIDEBAR_SECTION,
				"orientation", GTK_ORIENTATION_VERTICAL,
				"vexpand", FALSE,
				NULL);
	_gth_sidebar_section_add_view (sidebar, view);

	return (GtkWidget *) sidebar;
}


static gboolean
_gth_sidebar_section_visible (GthSidebarSection *self)
{
	return gtk_expander_get_expanded (GTK_EXPANDER (self->priv->expander));
}


gboolean
gth_sidebar_section_set_file (GthSidebarSection  *self,
			      GthFileData        *file_data)
{
	gboolean can_view;

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = NULL;
	if (file_data != NULL)
		self->priv->file_data = _g_object_ref (file_data);

	can_view = (self->priv->file_data != NULL) && gth_property_view_can_view (self->priv->view, self->priv->file_data);
	gtk_widget_set_visible (GTK_WIDGET (self), can_view);

	if (! can_view)
		self->priv->dirty = FALSE;
	else if (! _gth_sidebar_section_visible (self))
		self->priv->dirty = TRUE;
	else
		_gth_sidebar_section_update_view (self);

	return can_view;
}


void
gth_sidebar_section_set_expanded (GthSidebarSection *self,
				  gboolean	     expanded)
{
	gtk_expander_set_expanded (GTK_EXPANDER (self->priv->expander), expanded);
}


gboolean
gth_sidebar_section_get_expanded (GthSidebarSection *self)
{
	return gtk_expander_get_expanded (GTK_EXPANDER (self->priv->expander));
}


const char *
gth_sidebar_section_get_id (GthSidebarSection *self)
{
	return G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self->priv->view));
}
