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
#include "gth-toggle-menu-action.h"
#include "gth-toggle-menu-tool-button.h"

#define MENU_ID "gth-toggle-menu-tool-button-menu-id"

struct _GthToggleMenuToolButtonPrivate {
	guint      active : 1;
	guint      use_underline : 1;
	guint      contents_invalid : 1;
	char      *stock_id;
	char      *icon_name;
	char      *label_text;
	GtkWidget *toggle_button;
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
	PROP_ACTIVE,
	PROP_MENU
};


static GtkActivatableIface *parent_activatable_iface;
static int signals[LAST_SIGNAL];


static void gth_toggle_menu_tool_button_gtk_activable_interface_init (GtkActivatableIface *iface);


G_DEFINE_TYPE_WITH_CODE (GthToggleMenuToolButton,
			 gth_toggle_menu_tool_button,
			 GTK_TYPE_TOOL_ITEM,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_ACTIVATABLE,
					 	gth_toggle_menu_tool_button_gtk_activable_interface_init))


gchar *
_gtk_toolbar_elide_underscores (const gchar *original)
{
  gchar *q, *result;
  const gchar *p, *end;
  gsize len;
  gboolean last_underscore;

  if (!original)
    return NULL;

  len = strlen (original);
  q = result = g_malloc (len + 1);
  last_underscore = FALSE;

  end = original + len;
  for (p = original; p < end; p++)
    {
      if (!last_underscore && *p == '_')
        last_underscore = TRUE;
      else
        {
          last_underscore = FALSE;
          if (original + 2 <= p && p + 1 <= end &&
              p[-2] == '(' && p[-1] == '_' && p[0] != '_' && p[1] == ')')
            {
              q--;
              *q = '\0';
              p++;
            }
          else
            *q++ = *p;
        }
    }

  if (last_underscore)
    *q++ = '_';

  *q = '\0';

  return result;
}


static void
gth_toggle_menu_tool_button_construct_contents (GtkToolItem *tool_item)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (tool_item);
	GtkWidget               *label = NULL;
	GtkWidget               *icon = NULL;
	GtkToolbarStyle          style;
	gboolean                 need_label = FALSE;
	gboolean                 need_icon = FALSE;
	GtkWidget               *arrow;
	GtkWidget               *arrow_align;
	GtkIconSize              icon_size;
	GtkWidget               *main_box;
	GtkWidget               *box;
	guint                    icon_spacing;
	GtkOrientation           text_orientation = GTK_ORIENTATION_HORIZONTAL;
	GtkSizeGroup            *size_group = NULL;

	button->priv->contents_invalid = FALSE;

	gtk_widget_style_get (GTK_WIDGET (tool_item),
			      "icon-spacing", &icon_spacing,
			      NULL);

	if ((button->priv->icon_widget != NULL) && (gtk_widget_get_parent (button->priv->icon_widget) != NULL))
		gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (button->priv->icon_widget)),
				      button->priv->icon_widget);

	if ((button->priv->label_widget != NULL) && (gtk_widget_get_parent (button->priv->label_widget) != NULL))
		gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (button->priv->label_widget)),
				      button->priv->label_widget);

	if (gtk_bin_get_child (GTK_BIN (button->priv->toggle_button)) != NULL)
		/* Note: we are not destroying the label_widget or icon_widget
		 * here because they were removed from their containers above
		 */
		gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (button->priv->toggle_button)));

	style = gtk_tool_item_get_toolbar_style (GTK_TOOL_ITEM (button));

	if (style != GTK_TOOLBAR_TEXT)
		need_icon = TRUE;

	if ((style != GTK_TOOLBAR_ICONS) && (style != GTK_TOOLBAR_BOTH_HORIZ))
		need_label = TRUE;

	if ((style == GTK_TOOLBAR_BOTH_HORIZ) &&
	    (gtk_tool_item_get_is_important (GTK_TOOL_ITEM (button))
	     || (gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button)) == GTK_ORIENTATION_VERTICAL)
	     || (gtk_tool_item_get_text_orientation (GTK_TOOL_ITEM (button)) == GTK_ORIENTATION_VERTICAL)))
	{
		need_label = TRUE;
	}

	if ((style == GTK_TOOLBAR_ICONS)
	    && (button->priv->icon_widget == NULL)
	    && (button->priv->stock_id == NULL)
	    && button->priv->icon_name == NULL)
	{
		need_label = TRUE;
		need_icon = FALSE;
		style = GTK_TOOLBAR_TEXT;
	}

	if ((style == GTK_TOOLBAR_TEXT)
	    && (button->priv->label_widget == NULL)
	    && (button->priv->stock_id == NULL)
	    && (button->priv->label_text == NULL))
	{
		need_label = FALSE;
		need_icon = TRUE;
		style = GTK_TOOLBAR_ICONS;
	}

	if (need_label) {
		if (button->priv->label_widget) {
			label = button->priv->label_widget;
		}
		else {
			GtkStockItem  stock_item;
			gboolean      elide;
			char         *label_text;

			if (button->priv->label_text) {
				label_text = button->priv->label_text;
				elide = button->priv->use_underline;
			}
			else if (button->priv->stock_id && gtk_stock_lookup (button->priv->stock_id, &stock_item)) {
				label_text = stock_item.label;
				elide = TRUE;
			}
			else {
				label_text = "";
				elide = FALSE;
			}

			if (elide)
				label_text = _gtk_toolbar_elide_underscores (label_text);
			else
				label_text = g_strdup (label_text);

			label = gtk_label_new (label_text);

			g_free (label_text);

			gtk_widget_show (label);
		}

		gtk_label_set_ellipsize (GTK_LABEL (label),
					 gtk_tool_item_get_ellipsize_mode (GTK_TOOL_ITEM (button)));
		text_orientation = gtk_tool_item_get_text_orientation (GTK_TOOL_ITEM (button));
		if (text_orientation == GTK_ORIENTATION_HORIZONTAL) {
			gtk_label_set_angle (GTK_LABEL (label), 0);
			gtk_misc_set_alignment (GTK_MISC (label),
						gtk_tool_item_get_text_alignment (GTK_TOOL_ITEM (button)),
						0.5);
		}
		else {
			gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_NONE);
			if (gtk_widget_get_direction (GTK_WIDGET (tool_item)) == GTK_TEXT_DIR_RTL)
				gtk_label_set_angle (GTK_LABEL (label), -90);
			else
				gtk_label_set_angle (GTK_LABEL (label), 90);
			gtk_misc_set_alignment (GTK_MISC (label),
						0.5,
						1 - gtk_tool_item_get_text_alignment (GTK_TOOL_ITEM (button)));
		}
	}

	icon_size = gtk_tool_item_get_icon_size (GTK_TOOL_ITEM (button));
	if (need_icon) {
		if (button->priv->icon_widget) {
			icon = button->priv->icon_widget;

			if (GTK_IS_IMAGE (icon)) {
				g_object_set (button->priv->icon_widget,
					      "icon-size", icon_size,
					      NULL);
			}
		}
		else if (button->priv->stock_id
			 && gtk_icon_factory_lookup_default (button->priv->stock_id))
		{
			icon = gtk_image_new_from_stock (button->priv->stock_id, icon_size);
			gtk_widget_show (icon);
		}
		else if (button->priv->icon_name) {
			icon = gtk_image_new_from_icon_name (button->priv->icon_name, icon_size);
			gtk_widget_show (icon);
		}

		if (icon && text_orientation == GTK_ORIENTATION_HORIZONTAL)
			gtk_misc_set_alignment (GTK_MISC (icon),
						1.0 - gtk_tool_item_get_text_alignment (GTK_TOOL_ITEM (button)),
						0.5);
		else if (icon)
			gtk_misc_set_alignment (GTK_MISC (icon),
						0.5,
						gtk_tool_item_get_text_alignment (GTK_TOOL_ITEM (button)));

		if (icon) {
			size_group = gtk_tool_item_get_text_size_group (GTK_TOOL_ITEM (button));
			if (size_group != NULL)
				gtk_size_group_add_widget (size_group, icon);
		}
	}

	arrow = gtk_arrow_new ((text_orientation == GTK_ORIENTATION_HORIZONTAL) ? GTK_ARROW_DOWN : GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	gtk_widget_show (arrow);

	arrow_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (arrow_align), 0, 0, icon_spacing, 0);
	gtk_widget_show (arrow_align);
	gtk_container_add (GTK_CONTAINER (arrow_align), arrow);

	size_group = gtk_tool_item_get_text_size_group (GTK_TOOL_ITEM (button));
	if (size_group != NULL)
		gtk_size_group_add_widget (size_group, arrow_align);

	main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, icon_spacing);
	gtk_widget_show (main_box);
	gtk_container_add (GTK_CONTAINER (button->priv->toggle_button), main_box);

	if (style == GTK_TOOLBAR_BOTH_HORIZ)
		box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, icon_spacing);
	else
		box = gtk_box_new (GTK_ORIENTATION_VERTICAL, icon_spacing);
	gtk_widget_show (box);

	gtk_box_pack_start (GTK_BOX (main_box), box, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (main_box), arrow_align, FALSE, FALSE, 0);

	switch (style) {
	case GTK_TOOLBAR_ICONS:
		if (icon)
			gtk_box_pack_start (GTK_BOX (box), icon, TRUE, TRUE, 0);
		break;

	case GTK_TOOLBAR_BOTH:
		if (icon)
			gtk_box_pack_start (GTK_BOX (box), icon, TRUE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (box), label, FALSE, TRUE, 0);
		break;

	case GTK_TOOLBAR_BOTH_HORIZ:
		if (text_orientation == GTK_ORIENTATION_HORIZONTAL) {
			if (icon)
				gtk_box_pack_start (GTK_BOX (box), icon, label? FALSE : TRUE, TRUE, 0);
			if (label)
				gtk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);
		}
		else {
			if (icon)
				gtk_box_pack_end (GTK_BOX (box), icon, label ? FALSE : TRUE, TRUE, 0);
			if (label)
				gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
		}
		break;

	case GTK_TOOLBAR_TEXT:
		gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
		break;
	}

	gtk_button_set_relief (GTK_BUTTON (button->priv->toggle_button),
			       gtk_tool_item_get_relief_style (GTK_TOOL_ITEM (button)));

	gtk_tool_item_rebuild_menu (tool_item);

	gtk_widget_queue_resize (GTK_WIDGET (button));
}


static void
gth_toggle_menu_tool_button_state_changed (GtkWidget    *widget,
					   GtkStateType  previous_state)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (widget);

	if (! gtk_widget_is_sensitive (widget) && (button->priv->menu != NULL))
		gtk_menu_shell_deactivate (GTK_MENU_SHELL (button->priv->menu));
}


static void
gth_toggle_menu_tool_button_update_icon_spacing (GthToggleMenuToolButton *button)
{
	GtkWidget *box;
	guint spacing;

	box = gtk_bin_get_child (GTK_BIN (button->priv->toggle_button));
	if (GTK_IS_BOX (box)) {
		gtk_widget_style_get (GTK_WIDGET (button),
				      "icon-spacing", &spacing,
				      NULL);
		gtk_box_set_spacing (GTK_BOX (box), spacing);
	}
}


static void
gth_toggle_menu_tool_button_style_updated (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (gth_toggle_menu_tool_button_parent_class)->style_updated (widget);
	gth_toggle_menu_tool_button_update_icon_spacing (GTH_TOGGLE_MENU_TOOL_BUTTON (widget));
}


static void
gth_toggle_menu_tool_button_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	switch (prop_id) {
	case PROP_LABEL:
		gth_toggle_menu_tool_button_set_label (button, g_value_get_string (value));
		break;
	case PROP_USE_UNDERLINE:
		gth_toggle_menu_tool_button_set_use_underline (button, g_value_get_boolean (value));
		break;
	case PROP_STOCK_ID:
		gth_toggle_menu_tool_button_set_stock_id (button, g_value_get_string (value));
		break;
	case PROP_ICON_NAME:
		gth_toggle_menu_tool_button_set_icon_name (button, g_value_get_string (value));
		break;
	case PROP_ACTIVE:
		gth_toggle_menu_tool_button_set_active (button, g_value_get_boolean (value));
		break;
	case PROP_MENU:
		gth_toggle_menu_tool_button_set_menu (button, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_toggle_menu_tool_button_get_property (GObject    *object,
					  guint       prop_id,
					  GValue     *value,
					  GParamSpec *pspec)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	switch (prop_id) {
	case PROP_LABEL:
		g_value_set_string (value, gth_toggle_menu_tool_button_get_label (button));
		break;
	case PROP_USE_UNDERLINE:
		g_value_set_boolean (value, gth_toggle_menu_tool_button_get_use_underline (button));
		break;
	case PROP_STOCK_ID:
		g_value_set_string (value, button->priv->stock_id);
		break;
	case PROP_ICON_NAME:
		g_value_set_string (value, button->priv->icon_name);
		break;
	case PROP_ACTIVE:
		g_value_set_boolean (value, gth_toggle_menu_tool_button_get_active (button));
		break;
	case PROP_MENU:
		g_value_set_object (value, button->priv->menu);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_toggle_menu_tool_button_property_notify (GObject    *object,
            				     GParamSpec *pspec)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	if (button->priv->contents_invalid || strcmp ("is-important", pspec->name) == 0)
		gth_toggle_menu_tool_button_construct_contents (GTK_TOOL_ITEM (object));

	if (G_OBJECT_CLASS (gth_toggle_menu_tool_button_parent_class)->notify)
		G_OBJECT_CLASS (gth_toggle_menu_tool_button_parent_class)->notify (object, pspec);
}


/* Callback for the "deactivate" signal on the pop-up menu.
 * This is used so that we unset the state of the toggle button
 * when the pop-up menu disappears.
 */
static int
menu_deactivate_cb (GtkMenuShell            *menu_shell,
		    GthToggleMenuToolButton *button)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->priv->toggle_button), FALSE);
	return TRUE;
}


static void
menu_position_func (GtkMenu                 *menu,
                    int                     *x,
                    int                     *y,
                    gboolean                *push_in,
                    GthToggleMenuToolButton *button)
{
	GtkWidget             *widget = GTK_WIDGET (button);
	GtkRequisition         req;
	GtkRequisition         menu_req;
	GtkOrientation         orientation;
	GtkTextDirection       direction;
	cairo_rectangle_int_t  monitor;
	int                    monitor_num;
	GdkScreen             *screen;
	GtkAllocation          allocation;

	gtk_widget_get_preferred_size (GTK_WIDGET (button->priv->menu), &menu_req, NULL);

	orientation = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button));
	direction = gtk_widget_get_direction (widget);

	screen = gtk_widget_get_screen (GTK_WIDGET (menu));
	monitor_num = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (widget));
	if (monitor_num < 0)
		monitor_num = 0;
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
	gtk_widget_get_allocation (widget, &allocation);

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
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
	}
	else {
		gdk_window_get_origin (gtk_button_get_event_window (GTK_BUTTON (widget)), x, y);
		gtk_widget_get_preferred_size (widget, &req, NULL);

		if (direction == GTK_TEXT_DIR_LTR)
			*x += allocation.width;
		else
			*x -= menu_req.width;

		if ((*y + menu_req.height > monitor.y + monitor.height) &&
		    (*y + allocation.height - monitor.y > monitor.y + monitor.height - *y))
		{
			*y += allocation.height - menu_req.height;
		}
	}

	*push_in = FALSE;
}


static void
popup_menu_under_button (GthToggleMenuToolButton *button,
                         GdkEventButton          *event)
{
	g_signal_emit (button, signals[SHOW_MENU], 0);

	if (button->priv->menu == NULL)
		return;

	if (gtk_menu_get_attach_widget (button->priv->menu) != NULL)
		gtk_menu_detach (button->priv->menu);
	gtk_menu_popup (button->priv->menu, NULL, NULL,
			(GtkMenuPositionFunc) menu_position_func,
			button,
			event ? event->button : 0,
			event ? event->time : gtk_get_current_event_time ());
}


static gboolean
real_button_toggled_cb (GtkToggleButton         *togglebutton,
                        GthToggleMenuToolButton *button)
{
	gboolean toggle_active = gtk_toggle_button_get_active (togglebutton);

	if (button->priv->menu == NULL)
		return FALSE;

	if (button->priv->active != toggle_active) {
		button->priv->active = toggle_active;
		g_object_notify (G_OBJECT (button), "active");

		if (button->priv->active && ! gtk_widget_get_visible (GTK_WIDGET (button->priv->menu))) {
			/* we get here only when the menu is activated by a key
			 * press, so that we can select the first menu item */
			popup_menu_under_button (button, NULL);
			gtk_menu_shell_select_first (GTK_MENU_SHELL (button->priv->menu), FALSE);
		}
	}

	return FALSE;
}


static gboolean
real_button_button_press_event_cb (GtkWidget               *widget,
                                   GdkEventButton          *event,
                                   GthToggleMenuToolButton *button)
{
	if ((event->button == 1) && (button->priv->menu != NULL))  {
		popup_menu_under_button (button, event);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		return TRUE;
	}
	else
		return FALSE;
}


static void
gth_toggle_menu_tool_button_finalize (GObject *object)
{
	GthToggleMenuToolButton *button;

	button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	if (button->priv->toggle_button != NULL) {
		g_signal_handlers_disconnect_by_func (button->priv->toggle_button,
						      real_button_toggled_cb,
						      button);
		g_signal_handlers_disconnect_by_func (button->priv->toggle_button,
						      real_button_button_press_event_cb,
						      button);
		button->priv->toggle_button = NULL;
	}

	G_OBJECT_CLASS (gth_toggle_menu_tool_button_parent_class)->finalize (object);
}


static gboolean
gth_toggle_menu_tool_button_create_menu_proxy (GtkToolItem *item)
{
	GthToggleMenuToolButton *button;
	GtkStockItem             stock_item;
	gboolean                 use_mnemonic = TRUE;
	const char              *label_text;
	const char              *stock_id;
	const char              *label;
	GtkWidget               *menu_item;
	GtkWidget               *menu_image;

	button = GTH_TOGGLE_MENU_TOOL_BUTTON (item);

	label_text = gth_toggle_menu_tool_button_get_label (button);
	stock_id = gth_toggle_menu_tool_button_get_stock_id (button);

	if (label_text) {
		label = label_text;
		use_mnemonic = gth_toggle_menu_tool_button_get_use_underline (button);
	}
	else if ((stock_id != NULL) && gtk_stock_lookup (stock_id, &stock_item)) {
		label = stock_item.label;
	}
	else {
		label = "";
	}

	if (use_mnemonic)
		menu_item = gtk_image_menu_item_new_with_mnemonic (label);
	else
		menu_item = gtk_image_menu_item_new_with_label (label);

	menu_image = NULL;
	if (button->priv->stock_id)
		menu_image = gtk_image_new_from_stock (button->priv->stock_id, GTK_ICON_SIZE_MENU);
	else if (button->priv->icon_name)
		menu_image = gtk_image_new_from_icon_name (button->priv->icon_name, GTK_ICON_SIZE_MENU);

	if (menu_image != NULL)
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), menu_image);

	g_object_ref (button->priv->menu);
	if (gtk_menu_get_attach_widget (button->priv->menu) != NULL)
		gtk_menu_detach (button->priv->menu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), GTK_WIDGET (button->priv->menu));

	gtk_tool_item_set_proxy_menu_item (item, MENU_ID, menu_item);

	return TRUE;
}


static void
gth_toggle_menu_tool_button_toolbar_reconfigured (GtkToolItem *tool_item)
{
	gth_toggle_menu_tool_button_construct_contents (tool_item);
}


static void
gth_toggle_menu_tool_button_class_init (GthToggleMenuToolButtonClass *klass)
{
	GObjectClass     *object_class;
	GtkWidgetClass   *widget_class;
	GtkToolItemClass *tool_item_class;

	g_type_class_add_private (klass, sizeof (GthToggleMenuToolButtonPrivate));

	object_class = (GObjectClass *) klass;
	object_class->finalize = gth_toggle_menu_tool_button_finalize;
	object_class->set_property = gth_toggle_menu_tool_button_set_property;
	object_class->get_property = gth_toggle_menu_tool_button_get_property;
	object_class->notify = gth_toggle_menu_tool_button_property_notify;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->state_changed = gth_toggle_menu_tool_button_state_changed;
	widget_class->style_updated = gth_toggle_menu_tool_button_style_updated;

	tool_item_class = (GtkToolItemClass *) klass;
	tool_item_class->create_menu_proxy = gth_toggle_menu_tool_button_create_menu_proxy;
	tool_item_class->toolbar_reconfigured = gth_toggle_menu_tool_button_toolbar_reconfigured;

	/* signals */

	/**
	 * GthToggleMenuToolButton::show-menu:
	 * @button: the object on which the signal is emitted
	 *
	 * The ::show-menu signal is emitted before the menu is shown.
	 *
	 * It can be used to populate the menu on demand, using
	 * gth_toggle_menu_tool_button_get_menu().
	 *
	 * Note that even if you populate the menu dynamically in this way,
	 * you must set an empty menu on the #GthToggleMenuToolButton beforehand,
	 * since the arrow is made insensitive if the menu is not set.
	 */
	signals[SHOW_MENU] =
		g_signal_new ("show-menu",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthToggleMenuToolButtonClass, show_menu),
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
					 PROP_ACTIVE,
					 g_param_spec_boolean ("active",
							       "Active",
							       "If the toggle button should be pressed in or not",
							       FALSE,
							       G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_MENU,
					 g_param_spec_object ("menu",
							      "Menu",
							      "The dropdown menu",
							      GTK_TYPE_MENU,
							      G_PARAM_READABLE | G_PARAM_WRITABLE));

	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_int ("icon-spacing",
								   "Icon spacing",
								   "Spacing in pixels between the icon and label",
								   0,
								   G_MAXINT,
								   3,
								   G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
gth_toggle_menu_tool_button_init (GthToggleMenuToolButton *button)
{
	button->priv = G_TYPE_INSTANCE_GET_PRIVATE (button, GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, GthToggleMenuToolButtonPrivate);
	button->priv->menu = NULL;
	button->priv->contents_invalid = TRUE;

	button->priv->toggle_button = gtk_toggle_button_new ();
	gtk_button_set_focus_on_click (GTK_BUTTON (button->priv->toggle_button), FALSE);
	gtk_container_add (GTK_CONTAINER (button), button->priv->toggle_button);
	gtk_widget_show (button->priv->toggle_button);

	g_signal_connect (button->priv->toggle_button,
			  "toggled",
			  G_CALLBACK (real_button_toggled_cb),
			  button);
	g_signal_connect (button->priv->toggle_button,
			  "button-press-event",
		          G_CALLBACK (real_button_button_press_event_cb),
		          button);

	button->priv->active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button->priv->toggle_button));
}


static void
gth_toggle_menu_tool_button_update (GtkActivatable *activatable,
				    GtkAction      *action,
				    const gchar    *property_name)
{
	GthToggleMenuToolButton *button;

	parent_activatable_iface->update (activatable, action, property_name);

	if (! GTK_IS_TOGGLE_ACTION (action))
		return;

	button = GTH_TOGGLE_MENU_TOOL_BUTTON (activatable);

	if (strcmp (property_name, "active") == 0) {
		gtk_action_block_activate (action);
		gth_toggle_menu_tool_button_set_active (button, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
		gtk_action_unblock_activate (action);
	}
	else if ((strcmp (property_name, "menu") == 0) && GTH_IS_TOGGLE_MENU_ACTION (action)) {
		gth_toggle_menu_tool_button_set_menu (button, gth_toggle_menu_action_get_menu (GTH_TOGGLE_MENU_ACTION (action)));
	}
	else if (gtk_activatable_get_use_action_appearance (activatable)) {
		if (strcmp (property_name, "label") == 0) {
			gth_toggle_menu_tool_button_set_label (button, gtk_action_get_label (action));
		}
		else if (strcmp (property_name, "stock-id") == 0) {
			gth_toggle_menu_tool_button_set_stock_id (button, gtk_action_get_stock_id (action));
		}
		else if (strcmp (property_name, "icon-name") == 0) {
			gth_toggle_menu_tool_button_set_icon_name (button, gtk_action_get_icon_name (action));
		}
	}
}


static void
gth_toggle_menu_tool_button_sync_action_properties (GtkActivatable *activatable,
						    GtkAction      *action)
{
	GthToggleMenuToolButton *button;

	parent_activatable_iface->sync_action_properties (activatable, action);

	if (! GTK_IS_TOGGLE_ACTION (action))
		return;

	button = GTH_TOGGLE_MENU_TOOL_BUTTON (activatable);

	gtk_action_block_activate (action);
	gth_toggle_menu_tool_button_set_active (button, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
	gtk_action_unblock_activate (action);

	if (GTH_IS_TOGGLE_MENU_ACTION (action))
		gth_toggle_menu_tool_button_set_menu (button, gth_toggle_menu_action_get_menu (GTH_TOGGLE_MENU_ACTION (action)));

	if (gtk_activatable_get_use_action_appearance (activatable)) {
		gth_toggle_menu_tool_button_set_label (button, gtk_action_get_label (action));
		gth_toggle_menu_tool_button_set_stock_id (button, gtk_action_get_stock_id (action));
		gth_toggle_menu_tool_button_set_icon_name (button, gtk_action_get_icon_name (action));
	}
}


static void
gth_toggle_menu_tool_button_gtk_activable_interface_init (GtkActivatableIface *iface)
{
	parent_activatable_iface = g_type_interface_peek_parent (iface);
	iface->update = gth_toggle_menu_tool_button_update;
	iface->sync_action_properties = gth_toggle_menu_tool_button_sync_action_properties;
}


GtkToolItem *
gth_toggle_menu_tool_button_new (void)
{
	return (GtkToolItem *) g_object_new (GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, NULL);
}


GtkToolItem *
gth_toggle_menu_tool_button_new_from_stock (const gchar *stock_id)
{
	g_return_val_if_fail (stock_id != NULL, NULL);

	return (GtkToolItem *) g_object_new (GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON,
					     "stock-id", stock_id,
					     NULL);
}


void
gth_toggle_menu_tool_button_set_label (GthToggleMenuToolButton *button,
				       const char              *label)
{
	char *old_label;

	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));

	old_label = button->priv->label_text;
	button->priv->label_text = g_strdup (label);

	if (label != NULL) {
		char      *elided_label;
		AtkObject *accessible;

		elided_label = _gtk_toolbar_elide_underscores (label);
		accessible = gtk_widget_get_accessible (GTK_WIDGET (button->priv->toggle_button));
		atk_object_set_name (accessible, elided_label);

		g_free (elided_label);
	}

	g_free (old_label);

	g_object_notify (G_OBJECT (button), "label");
}


const char *
gth_toggle_menu_tool_button_get_label (GthToggleMenuToolButton *button)
{
	g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), NULL);

	return button->priv->label_text;
}


void
gth_toggle_menu_tool_button_set_use_underline (GthToggleMenuToolButton *button,
					       gboolean                 use_underline)
{
	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));

	use_underline = use_underline != FALSE;

	if (use_underline != button->priv->use_underline) {
		button->priv->use_underline = use_underline;
		g_object_notify (G_OBJECT (button), "use-underline");
	}
}


gboolean
gth_toggle_menu_tool_button_get_use_underline (GthToggleMenuToolButton *button)
{
	g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), FALSE);

	return button->priv->use_underline;
}


void
gth_toggle_menu_tool_button_set_stock_id (GthToggleMenuToolButton *button,
					  const char              *stock_id)
{
	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));

	g_free (button->priv->stock_id);
	button->priv->stock_id = g_strdup (stock_id);

	g_object_notify (G_OBJECT (button), "stock-id");
}


const char *
gth_toggle_menu_tool_button_get_stock_id (GthToggleMenuToolButton *button)
{
	g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), NULL);

	return button->priv->stock_id;
}


void
gth_toggle_menu_tool_button_set_icon_name (GthToggleMenuToolButton *button,
					   const char              *icon_name)
{
	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));

	g_free (button->priv->icon_name);
	button->priv->icon_name = g_strdup (icon_name);

	g_object_notify (G_OBJECT (button), "icon-name");
}


const char*
gth_toggle_menu_tool_button_get_icon_name (GthToggleMenuToolButton *button)
{
	g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), NULL);

	return button->priv->icon_name;
}


void
gth_toggle_menu_tool_button_set_active (GthToggleMenuToolButton *button,
					gboolean                 is_active)
{
	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));

	is_active = is_active != FALSE;

	if (button->priv->active != is_active)
		gtk_button_clicked (GTK_BUTTON (button->priv->toggle_button));
}


gboolean
gth_toggle_menu_tool_button_get_active (GthToggleMenuToolButton *button)
{
	  g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), FALSE);

	  return button->priv->active;
}


void
gth_toggle_menu_tool_button_set_menu (GthToggleMenuToolButton *button,
				      GtkWidget               *menu)
{
	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));
	g_return_if_fail (GTK_IS_MENU (menu) || menu == NULL);

	if (button->priv->menu != GTK_MENU (menu)) {
		if ((button->priv->menu != NULL) && gtk_widget_get_visible (GTK_WIDGET (button->priv->menu)))
			gtk_menu_shell_deactivate (GTK_MENU_SHELL (button->priv->menu));

		button->priv->menu = GTK_MENU (menu);

		if (button->priv->menu != NULL) {
			g_object_add_weak_pointer (G_OBJECT (button->priv->menu), (gpointer *) &button->priv->menu);

			gtk_widget_set_sensitive (button->priv->toggle_button, TRUE);
			g_signal_connect (button->priv->menu,
					  "deactivate",
					  G_CALLBACK (menu_deactivate_cb),
					  button);
		}
		else
			gtk_widget_set_sensitive (button->priv->toggle_button, FALSE);
	}

	g_object_notify (G_OBJECT (button), "menu");
}


GtkWidget *
gth_toggle_menu_tool_button_get_menu (GthToggleMenuToolButton *button)
{
	g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), NULL);

	return GTK_WIDGET (button->priv->menu);
}
