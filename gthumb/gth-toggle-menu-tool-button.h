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

#ifndef GTH_TOGGLE_MENU_TOOL_BUTTON_H
#define GTH_TOGGLE_MENU_TOOL_BUTTON_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON            (gth_toggle_menu_tool_button_get_type ())
#define GTH_TOGGLE_MENU_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, GthToggleMenuToolButton))
#define GTH_TOGGLE_MENU_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, GthToggleMenuToolButtonClass))
#define GTH_IS_TOGGLE_MENU_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON))
#define GTH_IS_TOGGLE_MENU_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON))
#define GTH_TOGGLE_MENU_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, GthToggleMenuToolButtonClass))

typedef struct _GthToggleMenuToolButton        GthToggleMenuToolButton;
typedef struct _GthToggleMenuToolButtonClass   GthToggleMenuToolButtonClass;
typedef struct _GthToggleMenuToolButtonPrivate GthToggleMenuToolButtonPrivate;

struct _GthToggleMenuToolButton
{
	GtkToolItem parent;
	GthToggleMenuToolButtonPrivate *priv;
};

struct _GthToggleMenuToolButtonClass
{
	GtkToolItemClass parent_class;

	/*< signals >*/

	void (*show_menu) (GthToggleMenuToolButton *button);
};

GType          gth_toggle_menu_tool_button_get_type          (void) G_GNUC_CONST;
GtkToolItem *  gth_toggle_menu_tool_button_new               (void);
GtkToolItem *  gth_toggle_menu_tool_button_new_from_stock    (const char              *stock_id);
void           gth_toggle_menu_tool_button_set_label         (GthToggleMenuToolButton *button,
							      const char              *label);
const char *   gth_toggle_menu_tool_button_get_label         (GthToggleMenuToolButton *button);
void           gth_toggle_menu_tool_button_set_use_underline (GthToggleMenuToolButton *button,
							      gboolean                 use_underline);
gboolean       gth_toggle_menu_tool_button_get_use_underline (GthToggleMenuToolButton *button);
void           gth_toggle_menu_tool_button_set_stock_id      (GthToggleMenuToolButton *button,
							      const char              *stock_id);
const char *   gth_toggle_menu_tool_button_get_stock_id      (GthToggleMenuToolButton *button);
void           gth_toggle_menu_tool_button_set_icon_name     (GthToggleMenuToolButton *button,
							      const char              *icon_name);
const char *   gth_toggle_menu_tool_button_get_icon_name     (GthToggleMenuToolButton *button);
void           gth_toggle_menu_tool_button_set_active        (GthToggleMenuToolButton *button,
							      gboolean                 is_active);
gboolean       gth_toggle_menu_tool_button_get_active        (GthToggleMenuToolButton *button);
void           gth_toggle_menu_tool_button_set_menu          (GthToggleMenuToolButton *button,
							      GtkWidget               *menu);
GtkWidget *    gth_toggle_menu_tool_button_get_menu          (GthToggleMenuToolButton *button);

G_END_DECLS

#endif /* GTH_TOGGLE_MENU_TOOL_BUTTON_H */
