/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#ifndef GTH_MENU_BUTTON_H
#define GTH_MENU_BUTTON_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_MENU_BUTTON            (gth_menu_button_get_type ())
#define GTH_MENU_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MENU_BUTTON, GthMenuButton))
#define GTH_MENU_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MENU_BUTTON, GthMenuButtonClass))
#define GTH_IS_MENU_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MENU_BUTTON))
#define GTH_IS_MENU_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_MENU_BUTTON))
#define GTH_MENU_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_MENU_BUTTON, GthMenuButtonClass))

typedef struct _GthMenuButton        GthMenuButton;
typedef struct _GthMenuButtonClass   GthMenuButtonClass;
typedef struct _GthMenuButtonPrivate GthMenuButtonPrivate;

struct _GthMenuButton
{
	GtkToggleButton parent;
	GthMenuButtonPrivate *priv;
};

struct _GthMenuButtonClass
{
	GtkToggleButtonClass parent_class;

	/*< signals >*/

	void (*show_menu) (GthMenuButton *button);
};

GType                   gth_menu_button_get_type          (void) G_GNUC_CONST;
GtkWidget *             gth_menu_button_new               (void);
GtkWidget *             gth_menu_button_new_from_stock    (const char    *stock_id);
void                    gth_menu_button_set_label         (GthMenuButton *button,
							   const char    *label);
const char *            gth_menu_button_get_label         (GthMenuButton *button);
void                    gth_menu_button_set_use_underline (GthMenuButton *button,
							   gboolean       use_underline);
gboolean                gth_menu_button_get_use_underline (GthMenuButton *button);
void                    gth_menu_button_set_stock_id      (GthMenuButton *button,
							   const char    *stock_id);
const char *            gth_menu_button_get_stock_id      (GthMenuButton *button);
void                    gth_menu_button_set_icon_name     (GthMenuButton *button,
							   const char    *icon_name);
const char *            gth_menu_button_get_icon_name     (GthMenuButton *button);
void                    gth_menu_button_set_menu          (GthMenuButton *button,
							   GtkWidget     *menu);
GtkWidget *             gth_menu_button_get_menu          (GthMenuButton *button);

G_END_DECLS

#endif /* GTH_MENU_BUTTON_H */
