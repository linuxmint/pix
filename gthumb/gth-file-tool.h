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

#ifndef GTH_FILE_TOOL_H
#define GTH_FILE_TOOL_H

#include <gtk/gtk.h>
#include "gth-toolbox.h"
#include "gth-browser.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILE_TOOL (gth_file_tool_get_type ())
#define GTH_FILE_TOOL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_TOOL, GthFileTool))
#define GTH_FILE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_TOOL, GthFileToolClass))
#define GTH_IS_FILE_TOOL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_TOOL))
#define GTH_IS_FILE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_TOOL))
#define GTH_FILE_TOOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILE_TOOL, GthFileToolClass))

typedef struct _GthFileTool GthFileTool;
typedef struct _GthFileToolClass GthFileToolClass;
typedef struct _GthFileToolPrivate GthFileToolPrivate;

struct _GthFileTool {
	GtkButton parent_instance;
	GthFileToolPrivate *priv;
};

struct _GthFileToolClass {
	GtkButtonClass parent_class;

	/*< virtual functions >*/

	void         (*update_sensitivity) (GthFileTool *self);
	void         (*activate)           (GthFileTool *self);
	void         (*cancel)             (GthFileTool *self);
	GtkWidget *  (*get_options)        (GthFileTool *self);
	void         (*destroy_options)    (GthFileTool *self);
	void         (*apply_options)      (GthFileTool *self);
	void         (*populate_headerbar) (GthFileTool *self,
					    GthBrowser  *browser);

	/*< signals >*/

	void         (*show_options)       (GthFileTool *self);
	void         (*hide_options)       (GthFileTool *self);
};

GType			gth_file_tool_get_type			(void);
void			gth_file_tool_construct			(GthFileTool		*self,
								 const char		*icon_name,
								 const char		*options_title,
								 GthToolboxSection	 section);
GtkWidget *		gth_file_tool_get_window		(GthFileTool		*self);
const char *		gth_file_tool_get_icon_name		(GthFileTool		*self);
void			gth_file_tool_activate			(GthFileTool		*self);
void			gth_file_tool_cancel			(GthFileTool		*self);
gboolean		gth_file_tool_is_cancelled		(GthFileTool		*self);
void			gth_file_tool_set_zoomable		(GthFileTool		*self,
								 gboolean		 zoomable);
gboolean		gth_file_tool_get_zoomable		(GthFileTool		*self);
void			gth_file_tool_set_changes_image		(GthFileTool		*self,
								 gboolean		 value);
gboolean		gth_file_tool_get_changes_image		(GthFileTool		*self);
void			gth_file_tool_update_sensitivity	(GthFileTool		*self);
GtkWidget *		gth_file_tool_get_options		(GthFileTool		*self);
const char *		gth_file_tool_get_options_title		(GthFileTool 		*self);
GthToolboxSection	gth_file_tool_get_section		(GthFileTool		*self);
void			gth_file_tool_destroy_options		(GthFileTool		*self);
void			gth_file_tool_show_options		(GthFileTool		*self);
void			gth_file_tool_hide_options		(GthFileTool		*self);
void			gth_file_tool_apply_options		(GthFileTool		*self);
void			gth_file_tool_populate_headerbar	(GthFileTool		*self,
								 GthBrowser		*browser);

G_END_DECLS

#endif /* GTH_FILE_TOOL_H */
