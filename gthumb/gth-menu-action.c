/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gth-menu-action.h"


G_DEFINE_TYPE (GthMenuAction, gth_menu_action, GTK_TYPE_ACTION)


/* Properties */
enum {
        PROP_0,
        PROP_BUTTON_TOOLTIP,
        PROP_ARROW_TOOLTIP,
        PROP_MENU
};


struct _GthMenuActionPrivate {
	char      *button_tooltip;
	char      *arrow_tooltip;
	GtkWidget *menu;
};


static void
gth_menu_action_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthMenuAction *self = GTH_MENU_ACTION (object);

	switch (property_id) {
	case PROP_BUTTON_TOOLTIP:
		g_free (self->priv->button_tooltip);
		self->priv->button_tooltip = g_strdup (g_value_get_string (value));
		break;

	case PROP_ARROW_TOOLTIP:
		g_free (self->priv->arrow_tooltip);
		self->priv->arrow_tooltip = g_strdup (g_value_get_string (value));
		break;

	case PROP_MENU:
		if (self->priv->menu != NULL)
			g_object_unref (self->priv->menu);
		self->priv->menu = g_value_dup_object (value);
		break;

	default:
		break;
	}
}


static void
gth_menu_action_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthMenuAction *self = GTH_MENU_ACTION (object);

	switch (property_id) {
	case PROP_BUTTON_TOOLTIP:
		g_value_set_string (value, self->priv->button_tooltip);
		break;

	case PROP_ARROW_TOOLTIP:
		g_value_set_string (value, self->priv->arrow_tooltip);
		break;

	case PROP_MENU:
		g_value_set_object (value, self->priv->menu);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_menu_action_init (GthMenuAction *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_MENU_ACTION, GthMenuActionPrivate);
	self->priv->menu = NULL;
	self->priv->arrow_tooltip = NULL;
	self->priv->button_tooltip = NULL;
}


static void
gth_menu_action_finalize (GObject *base)
{
	GthMenuAction *self = GTH_MENU_ACTION (base);

	g_free (self->priv->arrow_tooltip);
	g_free (self->priv->button_tooltip);

	G_OBJECT_CLASS (gth_menu_action_parent_class)->finalize (base);
}


static GtkWidget *
gth_menu_action_create_tool_item (GtkAction *base)
{
	GthMenuAction *self = GTH_MENU_ACTION (base);
	GtkWidget     *tool_item;

	tool_item = g_object_new (GTK_TYPE_MENU_TOOL_BUTTON, NULL);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), FALSE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (tool_item), self->priv->button_tooltip);
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (tool_item), self->priv->arrow_tooltip);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (tool_item), self->priv->menu);

	return tool_item;
}


static void
gth_menu_action_class_init (GthMenuActionClass *klass)
{
	GObjectClass   *object_class;
	GtkActionClass *action_class;

	g_type_class_add_private (klass, sizeof (GthMenuActionPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_menu_action_set_property;
	object_class->get_property = gth_menu_action_get_property;
	object_class->finalize = gth_menu_action_finalize;

	action_class = (GtkActionClass *) klass;
	action_class->toolbar_item_type = GTK_TYPE_MENU_TOOL_BUTTON;
	action_class->create_tool_item = gth_menu_action_create_tool_item;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_BUTTON_TOOLTIP,
					 g_param_spec_string ("button-tooltip",
                                                              "Button Tooltip",
                                                              "The tooltip for the button",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_ARROW_TOOLTIP,
					 g_param_spec_string ("arrow-tooltip",
                                                              "Arrow Tooltip",
                                                              "The tooltip for the arrow",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MENU,
					 g_param_spec_object ("menu",
                                                              "Menu",
                                                              "The menu to show",
                                                              GTK_TYPE_MENU,
                                                              G_PARAM_READWRITE));
}
