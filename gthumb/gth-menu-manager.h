/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef GTH_MENU_MANAGER_H
#define GTH_MENU_MANAGER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_MENU_MANAGER_NEW_MERGE_ID -1

#define GTH_TYPE_MENU_MANAGER         (gth_menu_manager_get_type ())
#define GTH_MENU_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_MENU_MANAGER, GthMenuManager))
#define GTH_MENU_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_MENU_MANAGER, GthMenuManagerClass))
#define GTH_IS_MENU_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_MENU_MANAGER))
#define GTH_IS_MENU_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_MENU_MANAGER))
#define GTH_MENU_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_MENU_MANAGER, GthMenuManagerClass))

typedef struct {
	const char *label;
	const char *detailed_action;
	const char *accel;
	const char *icon_name;
} GthMenuEntry;

typedef struct _GthMenuManager         GthMenuManager;
typedef struct _GthMenuManagerPrivate  GthMenuManagerPrivate;
typedef struct _GthMenuManagerClass    GthMenuManagerClass;

struct _GthMenuManager
{
	GObject __parent;
	GthMenuManagerPrivate *priv;
};

struct _GthMenuManagerClass
{
	GObjectClass __parent_class;
};

GType			gth_menu_manager_get_type          	(void) G_GNUC_CONST;
GthMenuManager *	gth_menu_manager_new                	(GMenu 			*menu);
GMenu *			gth_menu_manager_get_menu		(GthMenuManager		*menu_manager);
guint			gth_menu_manager_append_entries		(GthMenuManager   	*menu_manager,
								 const GthMenuEntry 	*entries,
								 int                	 n_entries);
void			gth_menu_manager_remove_entries		(GthMenuManager		*menu_manager,
								 guint			 merge_id);
guint			gth_menu_manager_new_merge_id		(GthMenuManager		*menu_manager);
void			gth_menu_manager_append_entry		(GthMenuManager   	*menu_manager,
								 guint			 merge_id,
								 const char		*label,
								 const char		*detailed_action,
								 const char		*accel,
								 const char		*icon_name);

G_END_DECLS

#endif /* GTH_MENU_MANAGER_H */
