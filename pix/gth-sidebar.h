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

#ifndef GTH_SIDEBAR_H
#define GTH_SIDEBAR_H

#include <gtk/gtk.h>
#include "gth-file-data.h"

G_BEGIN_DECLS

#define GTH_TYPE_SIDEBAR              (gth_sidebar_get_type ())
#define GTH_SIDEBAR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SIDEBAR, GthSidebar))
#define GTH_SIDEBAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SIDEBAR, GthSidebarClass))
#define GTH_IS_SIDEBAR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SIDEBAR))
#define GTH_IS_SIDEBAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SIDEBAR))
#define GTH_SIDEBAR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SIDEBAR, GthSidebarClass))

typedef enum {
	GTH_SIDEBAR_STATE_HIDDEN,
	GTH_SIDEBAR_STATE_PROPERTIES,
	GTH_SIDEBAR_STATE_TOOLS
} GthSidebarState;

typedef struct _GthSidebar        GthSidebar;
typedef struct _GthSidebarClass   GthSidebarClass;
typedef struct _GthSidebarPrivate GthSidebarPrivate;

struct _GthSidebar {
	GtkStack __parent;
	GthSidebarPrivate *priv;
};

struct _GthSidebarClass {
	GtkStackClass __parent_class;
};

GType          gth_sidebar_get_type            (void);
GtkWidget *    gth_sidebar_new                 (char           **sections_status);
GtkWidget *    gth_sidebar_get_toolbox         (GthSidebar      *sidebar);
void           gth_sidebar_set_file            (GthSidebar      *sidebar,
						GthFileData     *file_data);
void           gth_sidebar_show_properties     (GthSidebar      *sidebar);
void           gth_sidebar_show_tools          (GthSidebar      *sidebar);
gboolean       gth_sidebar_tool_is_active      (GthSidebar      *sidebar);
void           gth_sidebar_deactivate_tool     (GthSidebar      *sidebar);
void           gth_sidebar_update_sensitivity  (GthSidebar      *sidebar);
char **        gth_sidebar_get_sections_status (GthSidebar      *sidebar);

G_END_DECLS

#endif /* GTH_SIDEBAR_H */
