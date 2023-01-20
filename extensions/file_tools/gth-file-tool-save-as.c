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

#include <config.h>
#include <gthumb.h>
#include "gth-file-tool-save-as.h"


G_DEFINE_TYPE (GthFileToolSaveAs, gth_file_tool_save_as, GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL)


static void
gth_file_tool_save_as_update_sensitivity (GthFileTool *base)
{
	GtkWidget *window;
	gboolean   can_save;

	window = gth_file_tool_get_window (base);

	can_save = gth_viewer_page_can_save (gth_browser_get_viewer_page (GTH_BROWSER (window)));
	can_save = can_save && (gth_browser_get_current_file (GTH_BROWSER (window)) != NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (base), can_save);
}


static void
gth_file_tool_save_as_activate (GthFileTool *tool)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;

	window = gth_file_tool_get_window (tool);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_viewer_page_save_as (viewer_page, NULL, NULL);
}


static void
gth_file_tool_save_as_class_init (GthFileToolSaveAsClass *klass)
{
	GthFileToolClass *file_tool_class;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->update_sensitivity = gth_file_tool_save_as_update_sensitivity;
	file_tool_class->activate = gth_file_tool_save_as_activate;
}


static void
gth_file_tool_save_as_init (GthFileToolSaveAs *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "document-save-as-symbolic", _("Save As…"), GTH_TOOLBOX_SECTION_FILE);
}
