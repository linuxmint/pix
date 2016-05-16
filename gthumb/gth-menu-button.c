/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

#include <string.h>
#include <gtk/gtk.h>
#include "gth-menu-button.h"

#define MENU_ID "gth-menu-button-menu-id"

struct _GthMenuButtonPrivate {
	guint      active : 1;
	GtkMenu   *menu;
	GtkWidget *icon_widget;
	GtkWidget *label_widget;
};

enum {
	SHOW_MENU,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_LABEL,
	PROP_USE_UNDERLINE,
	PROP_STOCK_ID,
	PROP_ICON_NAME,
	PROP_MENU
};


static int signals[LAST_SIGNAL];


G_DEFINE_TYPE (GthMenuButton, gth_menu_button, GTK_TYPE_TOGGLE_BUTTON)


static void
gth_menu_button_state_changed (GtkWidget    *widget,
			       GtkStateType  previous_state)
{
	GthMenuButton *self = GTH_MENU_BUTTON (widget);

	if (! gtk_widget_is_sensitive (widget) && (self->priv->menu != NULL))
		gtk_menu_shell_deactivate (GTK_MENU_SHELL (self->priv->menu));
}


static void
gth_menu_button_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthMenuButton *self = GTH_MENU_BUTTON (object);

	switch (prop_id) {
	case PROP_LABEL:
		gth_menu_button_set_label (self, g_value_get_string (value));
		break;
	case PROP_USE_UNDERLINE:
		gth_menu_button_set_use_underline (self, g_value_get_boolean (value));
		break;
	case PROP_STOCK_ID:
		gth_menu_button_set_stock_id (self, g_value_get_string (value));
		break;
	case PROP_ICON_NAME:
		gth_menu_button_set_icon_name (self, g_value_get_string (value));
		break;
	case PROP_MENU:
		gth_menu_button_set_menu (self, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_menu_button_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthMenuButton *self = GTH_MENU_BUTTON (object);

	switch (prop_id) {
	case PROP_LABEL:
		g_value_set_string (value, gth_menu_button_get_label (self));
		break;
	case PROP_USE_UNDERLINE:
		g_value_set_boolean (value, gth_menu_button_get_use_underline (self));
		break;
	case PROP_STOCK_ID:
		g_value_set_string (value, gth_menu_button_get_stock_id (self));
		break;
	case PROP_ICON_NAME:
		g_value_set_string (value, gth_menu_button_get_icon_name (self));
		break;
	case PROP_MENU:
		g_value_set_object (value, self->priv->menu);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


/* Callback for the "deactivate" signal on the pop-up menu.
 * This is used so that we unset the state of the toggle button
 * when the pop-up menu disappears.
 */
static int
menu_deactivate_cb (GtkMenuShell  *menu_shell,
		    GthMenuButton *self)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), FALSE);
	return TRUE;
}


static void
menu_position_func (GtkMenu       *menu,
                    int           *x,
                    int           *y,
                    gboolean      *push_in,
                    GthMenuButton *self)
{
	GtkWidget             *widget = GTK_WIDGET (self);
	GtkRequisition         menu_req;
	GtkTextDirection       direction;
	cairo_rectangle_int_t  monitor;
	int                    monitor_num;
	GdkScreen             *screen;
	GtkAllocation          allocation;

	gtk_widget_get_preferred_size (GTK_WIDGET (self->priv->menu), &menu_req, NULL);

	direction = gtk_widget_get_direction (widget);

	screen = gtk_widget_get_screen (GTK_WIDGET (menu));
	monitor_num = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (widget));
	if (monitor_num < 0)
		monitor_num = 0;
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
	gtk_widget_get_allocation (widget, &allocation);

	gdk_window_get_origin (gtk_widget_get_window (widget), x, y);
	*x += allocation.x;
	*y += allocation.y;

	if (direction == GTK_TEXT_DIR_LTR)
		*x += MAX (allocation.width - menu_req.width, 0);
	else if (menu_req.width > allocation.width)
		*x -= menu_req.width - allocation.width;

	if ((*y + allocation.height + menu_req.height) <= monitor.y + monitor.height)
		*y += allocation.height;
	else if ((*y - menu_req.height) >= monitor.y)
		*y -= menu_req.height;
	else if (monitor.y + monitor.height - (*y + allocation.height) > *y)
		*y += allocation.height;
	else
		*y -= menu_req.height;

	*push_in = FALSE;
}


static void
popup_menu_under_button (GthMenuButton  *self,
                         GdkEventButton *event)
{
	g_signal_emit (self, signals[SHOW_MENU], 0);

	if (self->priv->menu == NULL)
		return;

	if (gtk_menu_get_attach_widget (self->priv->menu) != NULL)
		gtk_menu_detach (self->priv->menu);
	gtk_menu_popup (self->priv->menu, NULL, NULL,
			(GtkMenuPositionFunc) menu_position_func,
			self,
			event ? event->button : 0,
			event ? event->time : gtk_get_current_event_time ());
}


static gboolean
toggle_button_toggled_cb (GtkToggleButton *togglebutton,
			  gpointer         user_data)
{
	GthMenuButton *self = user_data;
	gboolean       toggle_active = gtk_toggle_button_get_active (togglebutton);

	if (self->priv->menu == NULL)
		return FALSE;

	if (self->priv->active != toggle_active) {
		self->priv->active = toggle_active;
		g_object_notify (G_OBJECT (self), "active");

		if (self->priv->active && ! gtk_widget_get_visible (GTK_WIDGET (self->priv->menu))) {
			/* we get here only when the menu is activated by a key
			 * press, so that we can select the first menu item */
			popup_menu_under_button (self, NULL);
			gtk_menu_shell_select_first (GTK_MENU_SHELL (self->priv->menu), FALSE);
		}
	}

	return FALSE;
}


static gboolean
toggle_button_press_event_cb (GtkWidget      *widget,
                              GdkEventButton *event,
                              gpointer        user_data)
{
	GthMenuButton *self = user_data;

	if ((event->button == 1) && (self->priv->menu != NULL))  {
		popup_menu_under_button (self, event);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		return TRUE;
	}
	else
		return FALSE;
}


static void
gth_menu_button_class_init (GthMenuButtonClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (klass, sizeof (GthMenuButtonPrivate));

	object_class = (GObjectClass *) klass;
	object_class->set_property = gth_menu_button_set_property;
	object_class->get_property = gth_menu_button_get_property;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->state_changed = gth_menu_button_state_changed;

	/* signals */

	/**
	 * GthMenuButton::show-menu:
	 * @button: the object on which the signal is emitted
	 *
	 * The ::show-menu signal is emitted before the menu is shown.
	 *
	 * It can be used to populate the menu on demand, using
	 * gth_menu_button_get_menu().
	 *
	 * Note that even if you populate the menu dynamically in this way,
	 * you must set an empty menu on the #GthMenuButton beforehand,
	 * since the arrow is made insensitive if the menu is not set.
	 */
	signals[SHOW_MENU] =
		g_signal_new ("show-menu",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthMenuButtonClass, show_menu),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_LABEL,
					 g_param_spec_string ("label",
							      "Label",
							      "Text to show in the item.",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_USE_UNDERLINE,
					 g_param_spec_boolean ("use-underline",
							       "Use underline",
							       "If set, an underline in the label property indicates that the next character should be used for the mnemonic accelerator key in the overflow menu",
							       FALSE,
							       G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_STOCK_ID,
					 g_param_spec_string ("stock-id",
							      "Stock Id",
							      "The stock icon displayed on the item",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_ICON_NAME,
					 g_param_spec_string ("icon-name",
							      "Icon name",
							      "The name of the themed icon displayed on the item",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_MENU,
					 g_param_spec_object ("menu",
							      "Menu",
							      "The dropdown menu",
							      GTK_TYPE_MENU,
							      G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
gth_menu_button_init (GthMenuButton *self)
{
	GtkSettings *settings;
	gboolean     show_image;
	guint        image_spacing;
	GtkWidget   *arrow;
	GtkWidget   *arrow_align;
	GtkWidget   *main_box;
	GtkWidget   *box;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_MENU_BUTTON, GthMenuButtonPrivate);
	self->priv->menu = NULL;
	self->priv->active = FALSE;

	gtk_widget_style_get (GTK_WIDGET (self),
			      "image-spacing", &image_spacing,
			      NULL);

	/* icon and label */

	self->priv->icon_widget = gtk_image_new ();

	settings = gtk_widget_get_settings (GTK_WIDGET (self));
	g_object_get (settings, "gtk-button-images", &show_image, NULL);
	if (show_image)
		gtk_widget_show (self->priv->icon_widget);
	else
		gtk_widget_hide (self->priv->icon_widget);

	self->priv->label_widget = gtk_label_new (NULL);
	gtk_widget_show (self->priv->label_widget);

	/* arrow */

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_show (arrow);

	arrow_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (arrow_align), 0, 0, image_spacing, 0);
	gtk_widget_show (arrow_align);
	gtk_container_add (GTK_CONTAINER (arrow_align), arrow);

	/* box */

	main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, image_spacing);
	gtk_widget_show (main_box);
	gtk_container_add (GTK_CONTAINER (self), main_box);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, image_spacing);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (box), self->priv->icon_widget, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (box), self->priv->label_widget, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (main_box), box, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (main_box), arrow_align, FALSE, FALSE, 0);

	/* signals */

	g_signal_connect (self,
			  "toggled",
			  G_CALLBACK (toggle_button_toggled_cb),
			  self);
	g_signal_connect (self,
			  "button-press-event",
		          G_CALLBACK (toggle_button_press_event_cb),
		          self);
}


GtkWidget *
gth_menu_button_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_MENU_BUTTON, NULL);
}


GtkWidget *
gth_menu_button_new_from_stock (const char *stock_id)
{
	g_return_val_if_fail (stock_id != NULL, NULL);

	return (GtkWidget *) g_object_new (GTH_TYPE_MENU_BUTTON,
					   "stock-id", stock_id,
					   NULL);
}


void
gth_menu_button_set_label (GthMenuButton *self,
			   const char    *label)
{
	g_return_if_fail (GTH_IS_MENU_BUTTON (self));

	gtk_label_set_label (GTK_LABEL (self->priv->label_widget), label);
	g_object_notify (G_OBJECT (self), "label");
}


const char *
gth_menu_button_get_label (GthMenuButton *self)
{
	g_return_val_if_fail (GTH_IS_MENU_BUTTON (self), NULL);

	return gtk_label_get_label (GTK_LABEL (self->priv->label_widget));
}


void
gth_menu_button_set_use_underline (GthMenuButton *self,
				   gboolean       use_underline)
{
	g_return_if_fail (GTH_IS_MENU_BUTTON (self));

	gtk_label_set_use_underline (GTK_LABEL (self->priv->label_widget), use_underline);
	g_object_notify (G_OBJECT (self), "use-underline");
}


gboolean
gth_menu_button_get_use_underline (GthMenuButton *self)
{
	g_return_val_if_fail (GTH_IS_MENU_BUTTON (self), FALSE);

	return gtk_label_get_use_underline (GTK_LABEL (self->priv->label_widget));
}


void
gth_menu_button_set_stock_id (GthMenuButton *self,
			      const char    *stock_id)
{
	GtkStockItem  stock_item;
	char         *label_text;

	g_return_if_fail (GTH_IS_MENU_BUTTON (self));

	gtk_image_set_from_stock (GTK_IMAGE (self->priv->icon_widget), stock_id, GTK_ICON_SIZE_BUTTON);

	if ((stock_id != NULL) && gtk_stock_lookup (stock_id, &stock_item))
		label_text = stock_item.label;
	else
		label_text = "";
	gtk_label_set_text (GTK_LABEL (self->priv->label_widget), label_text);
	gth_menu_button_set_use_underline (self, TRUE);

	g_object_notify (G_OBJECT (self), "stock-id");
}


const char *
gth_menu_button_get_stock_id (GthMenuButton *self)
{
	char *stock_id;

	g_return_val_if_fail (GTH_IS_MENU_BUTTON (self), NULL);

	gtk_image_get_stock (GTK_IMAGE (self->priv->icon_widget),
			     &stock_id,
			     NULL);

	return stock_id;
}


void
gth_menu_button_set_icon_name (GthMenuButton *self,
			      const char     *icon_name)
{
	g_return_if_fail (GTH_IS_MENU_BUTTON (self));

	gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->icon_widget), icon_name, GTK_ICON_SIZE_BUTTON);
	g_object_notify (G_OBJECT (self), "icon-name");
}


const char *
gth_menu_button_get_icon_name (GthMenuButton *self)
{
	const char *icon_name;

	g_return_val_if_fail (GTH_IS_MENU_BUTTON (self), NULL);

	gtk_image_get_icon_name (GTK_IMAGE (self->priv->icon_widget),
				 &icon_name,
				 NULL);

	return icon_name;
}


void
gth_menu_button_set_menu (GthMenuButton *self,
		          GtkWidget     *menu)
{
	g_return_if_fail (GTH_IS_MENU_BUTTON (self));
	g_return_if_fail (GTK_IS_MENU (menu) || menu == NULL);

	if (self->priv->menu != GTK_MENU (menu)) {
		if ((self->priv->menu != NULL) && gtk_widget_get_visible (GTK_WIDGET (self->priv->menu)))
			gtk_menu_shell_deactivate (GTK_MENU_SHELL (self->priv->menu));

		self->priv->menu = GTK_MENU (menu);

		if (self->priv->menu != NULL) {
			g_object_add_weak_pointer (G_OBJECT (self->priv->menu), (gpointer *) &self->priv->menu);

			gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
			g_signal_connect (self->priv->menu,
					  "deactivate",
					  G_CALLBACK (menu_deactivate_cb),
					  self);
		}
		else
			gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
	}

	g_object_notify (G_OBJECT (self), "menu");
}


GtkWidget *
gth_menu_button_get_menu (GthMenuButton *self)
{
	g_return_val_if_fail (GTH_IS_MENU_BUTTON (self), NULL);

	return GTK_WIDGET (self->priv->menu);
}
