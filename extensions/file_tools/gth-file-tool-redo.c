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
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-redo.h"


G_DEFINE_TYPE (GthFileToolRedo, gth_file_tool_redo, GTH_TYPE_FILE_TOOL)


static void
gth_file_tool_redo_update_sensitivity (GthFileTool *base)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		gtk_widget_set_sensitive (GTK_WIDGET (base), FALSE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (base), gth_image_history_can_redo (gth_image_viewer_page_get_history (GTH_IMAGE_VIEWER_PAGE (viewer_page))));
}


static void
gth_file_tool_redo_activate (GthFileTool *base)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	gth_image_viewer_page_redo (GTH_IMAGE_VIEWER_PAGE (viewer_page));
}


static void
gth_file_tool_redo_class_init (GthFileToolRedoClass *klass)
{
	GthFileToolClass *file_tool_class;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->update_sensitivity = gth_file_tool_redo_update_sensitivity;
	file_tool_class->activate = gth_file_tool_redo_activate;
}


static void
gth_file_tool_redo_init (GthFileToolRedo *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self),
				 "edit-redo-symbolic",
				 _("Redo"),
				 GTH_TOOLBOX_SECTION_FILE);
}
