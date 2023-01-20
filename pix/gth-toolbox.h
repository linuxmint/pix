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

#ifndef GTH_TOOLBOX_H
#define GTH_TOOLBOX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	GTH_TOOLBOX_SECTION_FILE,
	GTH_TOOLBOX_SECTION_COLORS,
	GTH_TOOLBOX_SECTION_ROTATION,
	GTH_TOOLBOX_SECTION_FORMAT,
	GTH_TOOLBOX_N_SECTIONS
} GthToolboxSection;

#define GTH_TYPE_TOOLBOX              (gth_toolbox_get_type ())
#define GTH_TOOLBOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TOOLBOX, GthToolbox))
#define GTH_TOOLBOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TOOLBOX, GthToolboxClass))
#define GTH_IS_TOOLBOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TOOLBOX))
#define GTH_IS_TOOLBOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TOOLBOX))
#define GTH_TOOLBOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TOOLBOX, GthToolboxClass))

typedef struct _GthToolbox        GthToolbox;
typedef struct _GthToolboxClass   GthToolboxClass;
typedef struct _GthToolboxPrivate GthToolboxPrivate;

struct _GthToolbox
{
	GtkStack __parent;
	GthToolboxPrivate *priv;
};

struct _GthToolboxClass
{
	GtkStackClass __parent_class;

	void  (*options_visibility)  (GthToolbox *toolbox,
				      gboolean    visible);
};

GType          gth_toolbox_get_type              (void);
GtkWidget *    gth_toolbox_new                   (const char  *name);
void           gth_toolbox_update_sensitivity    (GthToolbox  *toolbox);
void           gth_toolbox_deactivate_tool       (GthToolbox  *toolbox);
gboolean       gth_toolbox_tool_is_active        (GthToolbox  *toolbox);
GtkWidget *    gth_toolbox_get_tool              (GthToolbox  *toolbox,
						  GType        tool_type);
GtkWidget *    gth_toolbox_get_active_tool       (GthToolbox  *toolbox);

G_END_DECLS

#endif /* GTH_TOOLBOX_H */
