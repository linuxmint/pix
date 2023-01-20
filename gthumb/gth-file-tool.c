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
#include "gth-file-tool.h"


enum {
	SHOW_OPTIONS,
	HIDE_OPTIONS,
	LAST_SIGNAL
};


static guint gth_file_tool_signals[LAST_SIGNAL] = { 0 };


struct _GthFileToolPrivate {
	GtkWidget		*window;
	const char		*icon_name;
	const char		*options_title;
	GthToolboxSection	 section;
	gboolean		 cancelled;
	gboolean                 zoomable;
	gboolean                 changes_image;
};


G_DEFINE_TYPE_WITH_CODE (GthFileTool,
			 gth_file_tool,
			 GTK_TYPE_BUTTON,
			 G_ADD_PRIVATE (GthFileTool))


static void
gth_file_tool_base_update_sensitivity (GthFileTool *self)
{
	/* void */
}


static void
gth_file_tool_base_activate (GthFileTool *self)
{
	/* void*/
}


static void
gth_file_tool_base_cancel (GthFileTool *self)
{
	/* void*/
}


static GtkWidget *
gth_file_tool_base_get_options (GthFileTool *self)
{
	return NULL;
}


static void
gth_file_tool_base_destroy_options (GthFileTool *self)
{
	/* void */
}


static void
gth_file_tool_base_apply_options (GthFileTool *self)
{
	/* void */
}


static void
gth_file_tool_base_populate_headerbar (GthFileTool *self,
				       GthBrowser  *browser)
{
	/* void */
}


static void
gth_file_tool_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL (object));

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_parent_class)->finalize (object);
}


static void
gth_file_tool_class_init (GthFileToolClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_finalize;

	klass->update_sensitivity = gth_file_tool_base_update_sensitivity;
	klass->activate = gth_file_tool_base_activate;
	klass->cancel = gth_file_tool_base_cancel;
	klass->get_options = gth_file_tool_base_get_options;
	klass->destroy_options = gth_file_tool_base_destroy_options;
	klass->apply_options = gth_file_tool_base_apply_options;
	klass->populate_headerbar = gth_file_tool_base_populate_headerbar;

	gth_file_tool_signals[SHOW_OPTIONS] =
	                g_signal_new ("show-options",
	                              G_TYPE_FROM_CLASS (klass),
	                              G_SIGNAL_RUN_LAST,
	                              G_STRUCT_OFFSET (GthFileToolClass, show_options),
	                              NULL, NULL,
	                              g_cclosure_marshal_VOID__VOID,
	                              G_TYPE_NONE,
	                              0);
	gth_file_tool_signals[HIDE_OPTIONS] =
			g_signal_new ("hide-options",
		                      G_TYPE_FROM_CLASS (klass),
		                      G_SIGNAL_RUN_LAST,
		                      G_STRUCT_OFFSET (GthFileToolClass, hide_options),
		                      NULL, NULL,
		                      g_cclosure_marshal_VOID__VOID,
		                      G_TYPE_NONE,
		                      0);
}


static void
gth_file_tool_init (GthFileTool *self)
{
	self->priv = gth_file_tool_get_instance_private (self);
	self->priv->icon_name = NULL;
	self->priv->options_title = NULL;
	self->priv->cancelled = FALSE;
	self->priv->zoomable = FALSE;
	self->priv->changes_image = TRUE;
	self->priv->section = GTH_TOOLBOX_SECTION_COLORS;

	/*gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);*/
}


void
gth_file_tool_construct (GthFileTool		*self,
			 const char		*icon_name,
			 const char		*options_title,
			 GthToolboxSection       section)
{
	GtkWidget *hbox;
	GtkWidget *icon;

	self->priv->icon_name = icon_name;
	self->priv->options_title = options_title;
	self->priv->section = section;

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

	icon = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	gtk_widget_show (icon);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	gtk_widget_set_tooltip_text (GTK_WIDGET (self), options_title);

	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (self), hbox);
}


GtkWidget *
gth_file_tool_get_window (GthFileTool *self)
{
	if (self->priv->window == NULL)
		self->priv->window = GTK_WIDGET (_gtk_widget_get_toplevel_if_window (GTK_WIDGET (self)));
	return self->priv->window;
}


const char *
gth_file_tool_get_icon_name (GthFileTool *self)
{
	return self->priv->icon_name;
}


void
gth_file_tool_activate (GthFileTool *self)
{
	self->priv->cancelled = FALSE;
	GTH_FILE_TOOL_GET_CLASS (self)->activate (self);
}


void
gth_file_tool_cancel (GthFileTool *self)
{
	if (self->priv->cancelled)
		return;

	self->priv->cancelled = TRUE;
	GTH_FILE_TOOL_GET_CLASS (self)->cancel (self);
}


gboolean
gth_file_tool_is_cancelled (GthFileTool *self)
{
	return self->priv->cancelled;
}


void
gth_file_tool_set_zoomable (GthFileTool *self,
			    gboolean     zoomable)
{
	self->priv->zoomable = zoomable;
}


gboolean
gth_file_tool_get_zoomable (GthFileTool *self)
{
	return self->priv->zoomable;
}


void
gth_file_tool_set_changes_image (GthFileTool *self,
				 gboolean     value)
{
	self->priv->changes_image = value;
}


gboolean
gth_file_tool_get_changes_image (GthFileTool *self)
{
	return self->priv->changes_image;
}


void
gth_file_tool_update_sensitivity (GthFileTool *self)
{
	GTH_FILE_TOOL_GET_CLASS (self)->update_sensitivity (self);
}


void
gth_file_tool_show_options (GthFileTool *self)
{
	g_signal_emit (self, gth_file_tool_signals[SHOW_OPTIONS], 0, NULL);
}


void
gth_file_tool_hide_options (GthFileTool *self)
{
	g_signal_emit (self, gth_file_tool_signals[HIDE_OPTIONS], 0, NULL);
}


GtkWidget *
gth_file_tool_get_options (GthFileTool *self)
{
	return GTH_FILE_TOOL_GET_CLASS (self)->get_options (self);
}


GthToolboxSection
gth_file_tool_get_section (GthFileTool *self)
{
	return self->priv->section;
}


const char *
gth_file_tool_get_options_title (GthFileTool *self)
{
	return self->priv->options_title;
}


void
gth_file_tool_destroy_options (GthFileTool *self)
{
	GTH_FILE_TOOL_GET_CLASS (self)->destroy_options (self);
}


void
gth_file_tool_apply_options (GthFileTool *self)
{
	GTH_FILE_TOOL_GET_CLASS (self)->apply_options (self);
}


void
gth_file_tool_populate_headerbar (GthFileTool *self,
				  GthBrowser  *browser)
{
	GTH_FILE_TOOL_GET_CLASS (self)->populate_headerbar (self, browser);
}
