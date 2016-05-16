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
#include "gth-marshal.h"
#include "gth-viewer-page.h"


enum {
    FILE_LOADED,
    LAST_SIGNAL
};

static guint gth_viewer_page_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_INTERFACE (GthViewerPage, gth_viewer_page, 0)


static void
gth_viewer_page_default_init (GthViewerPageInterface *iface)
{
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	gth_viewer_page_signals[FILE_LOADED] =
	            g_signal_new ("file-loaded",
	        		  GTH_TYPE_VIEWER_PAGE,
	                          G_SIGNAL_RUN_LAST,
	                          G_STRUCT_OFFSET (GthViewerPageInterface, file_loaded),
	                          NULL, NULL,
	                          gth_marshal_VOID__OBJECT_BOOLEAN,
	                          G_TYPE_NONE,
	                          2,
	                          G_TYPE_OBJECT,
	                          G_TYPE_BOOLEAN);

	initialized = TRUE;
}


void
gth_viewer_page_activate (GthViewerPage *self,
			  GthBrowser    *browser)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->activate (self, browser);
}


void
gth_viewer_page_deactivate (GthViewerPage *self)
{
	gth_viewer_page_hide (self);
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->deactivate (self);
}


void
gth_viewer_page_show (GthViewerPage  *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->show (self);
}


void
gth_viewer_page_hide (GthViewerPage  *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->hide (self);
}


gboolean
gth_viewer_page_can_view (GthViewerPage *self,
			  GthFileData   *file_data)
{
	return GTH_VIEWER_PAGE_GET_INTERFACE (self)->can_view (self, file_data);
}


void
gth_viewer_page_view (GthViewerPage *self,
		      GthFileData   *file_data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->view (self, file_data);
}


void
gth_viewer_page_focus (GthViewerPage  *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->focus (self);
}


void
gth_viewer_page_fullscreen (GthViewerPage *self,
			    gboolean       active)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->fullscreen (self, active);
}


void
gth_viewer_page_show_pointer (GthViewerPage *self,
			     gboolean        show)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->show_pointer (self, show);
}


void
gth_viewer_page_update_sensitivity (GthViewerPage *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->update_sensitivity (self);
}


gboolean
gth_viewer_page_can_save (GthViewerPage *self)
{
	if (self == NULL)
		return FALSE;
	if (GTH_VIEWER_PAGE_GET_INTERFACE (self)->can_save != NULL)
		return GTH_VIEWER_PAGE_GET_INTERFACE (self)->can_save (self);
	else
		return FALSE;
}


void
gth_viewer_page_save (GthViewerPage *self,
		      GFile         *file,
		      FileSavedFunc  func,
		      gpointer       data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->save (self, file, func, data);
}


void
gth_viewer_page_save_as (GthViewerPage  *self,
			 FileSavedFunc   func,
			 gpointer        data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->save_as (self, func, data);
}


void
gth_viewer_page_revert (GthViewerPage *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->revert (self);
}


void
gth_viewer_page_update_info (GthViewerPage *self,
		  	     GthFileData   *file_data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->update_info (self, file_data);
}


void
gth_viewer_page_show_properties (GthViewerPage *self,
				 gboolean       show)
{
	if (GTH_VIEWER_PAGE_GET_INTERFACE (self)->show_properties != NULL)
		GTH_VIEWER_PAGE_GET_INTERFACE (self)->show_properties (self, show);
}


void
gth_viewer_page_shrink_wrap (GthViewerPage *self,
			     gboolean       value,
			     int           *other_width,
			     int           *other_height)
{
	if (GTH_VIEWER_PAGE_GET_INTERFACE (self)->shrink_wrap != NULL)
		GTH_VIEWER_PAGE_GET_INTERFACE (self)->shrink_wrap (self, value, other_width, other_height);
}


void
gth_viewer_page_file_loaded (GthViewerPage *self,
			     GthFileData   *file_data,
			     gboolean       success)
{
	g_signal_emit (self, gth_viewer_page_signals[FILE_LOADED], 0, file_data, success);
}
