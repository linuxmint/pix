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

#ifndef GTH_VIEWER_PAGE_H
#define GTH_VIEWER_PAGE_H

#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "typedefs.h"

G_BEGIN_DECLS

typedef struct _GthBrowser GthBrowser;

#define GTH_TYPE_VIEWER_PAGE (gth_viewer_page_get_type ())
#define GTH_VIEWER_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_VIEWER_PAGE, GthViewerPage))
#define GTH_IS_VIEWER_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_VIEWER_PAGE))
#define GTH_VIEWER_PAGE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_VIEWER_PAGE, GthViewerPageInterface))

typedef struct _GthViewerPage GthViewerPage;
typedef struct _GthViewerPageInterface GthViewerPageInterface;

typedef void (*FileSavedFunc)  (GthViewerPage *viewer_page,
				GthFileData   *file_data,
				GError        *error,
				gpointer       user_data);

struct _GthViewerPageInterface {
	GTypeInterface parent_iface;

	/*< virtual functions >*/

	void      (*activate)            (GthViewerPage *self,
				 	  GthBrowser    *browser);
	void      (*deactivate)          (GthViewerPage *self);
	void      (*show)                (GthViewerPage *self);
	void      (*hide)                (GthViewerPage *self);
	gboolean  (*can_view)            (GthViewerPage *self,
				 	  GthFileData   *file_data);
	void      (*view)                (GthViewerPage *self,
				 	  GthFileData   *file_data);
	void      (*focus)               (GthViewerPage *self);
	void      (*fullscreen)          (GthViewerPage *self,
					  gboolean       active);
	void      (*show_pointer)        (GthViewerPage *self,
					  gboolean       show);
	void      (*update_sensitivity)  (GthViewerPage *self);
	gboolean  (*can_save)            (GthViewerPage *self);
	void      (*save)                (GthViewerPage *self,
					  GFile         *file,
					  FileSavedFunc  func,
					  gpointer       data);
	void      (*save_as)             (GthViewerPage *self,
					  FileSavedFunc  func,
					  gpointer       data);
	void      (*revert)              (GthViewerPage *self);
	void      (*update_info)         (GthViewerPage *self,
					  GthFileData   *file_data);
	void      (*show_properties)     (GthViewerPage *self,
					  gboolean       show);
	gboolean  (*zoom_from_scroll)    (GthViewerPage *self,
					  GdkEventScroll *event);

	const char * (*shortcut_context) (GthViewerPage *self);

	/*< signals >*/

	void      (*file_loaded)         (GthViewerPage *self,
					  GthFileData   *file_data,
					  gboolean       success);
};

GType        gth_viewer_page_get_type            (void);
void         gth_viewer_page_activate            (GthViewerPage  *self,
					  	  GthBrowser     *browser);
void         gth_viewer_page_deactivate          (GthViewerPage  *self);
void         gth_viewer_page_show                (GthViewerPage  *self);
void         gth_viewer_page_hide                (GthViewerPage  *self);
gboolean     gth_viewer_page_can_view            (GthViewerPage  *self,
						  GthFileData    *file_data);
void         gth_viewer_page_view                (GthViewerPage  *self,
						  GthFileData    *file_data);
void         gth_viewer_page_focus               (GthViewerPage  *self);
void         gth_viewer_page_fullscreen          (GthViewerPage  *self,
						  gboolean       active);
void         gth_viewer_page_show_pointer        (GthViewerPage  *self,
						  gboolean        show);
void         gth_viewer_page_update_sensitivity  (GthViewerPage  *self);
gboolean     gth_viewer_page_can_save            (GthViewerPage  *self);
void         gth_viewer_page_save                (GthViewerPage  *self,
						  GFile          *file,
						  FileSavedFunc   func,
						  gpointer        data);
void         gth_viewer_page_save_as             (GthViewerPage  *self,
						  FileSavedFunc   func,
						  gpointer        data);
void         gth_viewer_page_revert              (GthViewerPage  *self);
void         gth_viewer_page_update_info         (GthViewerPage  *self,
		  	  	  	  	  GthFileData    *file_data);
void         gth_viewer_page_show_properties     (GthViewerPage  *self,
						  gboolean        show);
gboolean     gth_viewer_page_zoom_from_scroll    (GthViewerPage  *self,
						  GdkEventScroll *event);
void         gth_viewer_page_file_loaded         (GthViewerPage  *self,
						  GthFileData    *file_data,
						  GFileInfo      *updated_metadata,
						  gboolean        success);
const char * gth_viewer_page_get_shortcut_context
						 (GthViewerPage  *self);

G_END_DECLS

#endif /* GTH_VIEWER_PAGE_H */
