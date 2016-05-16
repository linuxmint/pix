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

#ifndef GTH_MENU_ACTION_H
#define GTH_MENU_ACTION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_MENU_ACTION            (gth_menu_action_get_type ())
#define GTH_MENU_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MENU_ACTION, GthMenuAction))
#define GTH_MENU_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MENU_ACTION, GthMenuActionClass))
#define GTH_IS_MENU_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MENU_ACTION))
#define GTH_IS_MENU_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GTH_TYPE_MENU_ACTION))
#define GTH_MENU_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_MENU_ACTION, GthMenuActionClass))

typedef struct _GthMenuAction        GthMenuAction;
typedef struct _GthMenuActionClass   GthMenuActionClass;
typedef struct _GthMenuActionPrivate GthMenuActionPrivate;

struct _GthMenuAction {
	GtkAction parent;
	GthMenuActionPrivate *priv;
};

struct _GthMenuActionClass {
	GtkActionClass parent_class;
};

GType gth_menu_action_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* GTH_MENU_ACTION_H */

