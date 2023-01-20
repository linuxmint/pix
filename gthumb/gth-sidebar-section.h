/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

#ifndef GTH_SIDEBAR_SECTION_H
#define GTH_SIDEBAR_SECTION_H

#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "gth-sidebar.h"

G_BEGIN_DECLS

#define GTH_TYPE_SIDEBAR_SECTION              (gth_sidebar_section_get_type ())
#define GTH_SIDEBAR_SECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SIDEBAR_SECTION, GthSidebarSection))
#define GTH_SIDEBAR_SECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SIDEBAR_SECTION, GthSidebarSectionClass))
#define GTH_IS_SIDEBAR_SECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SIDEBAR_SECTION))
#define GTH_IS_SIDEBAR_SECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SIDEBAR_SECTION))
#define GTH_SIDEBAR_SECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SIDEBAR_SECTION, GthSidebarSectionClass))

typedef struct _GthSidebarSection        GthSidebarSection;
typedef struct _GthSidebarSectionClass   GthSidebarSectionClass;
typedef struct _GthSidebarSectionPrivate GthSidebarSectionPrivate;

struct _GthSidebarSection {
	GtkBox __parent;
	GthSidebarSectionPrivate *priv;
};

struct _GthSidebarSectionClass {
	GtkBoxClass __parent_class;
};

GType		gth_sidebar_section_get_type	(void);
GtkWidget *	gth_sidebar_section_new		(GthPropertyView	*view);
gboolean	gth_sidebar_section_set_file	(GthSidebarSection	*section,
						 GthFileData		*file_data);
void		gth_sidebar_section_set_expanded(GthSidebarSection	*section,
						 gboolean		 expanded);
gboolean	gth_sidebar_section_get_expanded(GthSidebarSection	*section);
const char *	gth_sidebar_section_get_id	(GthSidebarSection	*section);

G_END_DECLS

#endif /* GTH_SIDEBAR_SECTION_H */
