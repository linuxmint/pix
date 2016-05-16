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
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "gth-image-viewer-page.h"
#include "preferences.h"


static void gth_viewer_page_interface_init (GthViewerPageInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthImageViewerPage,
			 gth_image_viewer_page,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_VIEWER_PAGE,
					        gth_viewer_page_interface_init))


struct _GthImageViewerPagePrivate {
	GthBrowser        *browser;
	GSettings         *settings;
	GtkWidget         *image_navigator;
	GtkWidget         *viewer;
	GthImagePreloader *preloader;
	GtkActionGroup    *actions;
	guint              viewer_merge_id;
	guint              browser_merge_id;
	GthImageHistory   *history;
	GthFileData       *file_data;
	gulong             requested_ready_id;
	gulong             original_size_ready_id;
	guint              hide_mouse_timeout;
	guint              motion_signal;
	gboolean           image_changed;
	GFile             *last_loaded;
	gboolean           can_paste;
};


static const char *image_viewer_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='File_Actions_1'>"
"        <menuitem action='ImageViewer_Edit_Undo'/>"
"        <menuitem action='ImageViewer_Edit_Redo'/>"
"        <separator />"
"        <menuitem action='ImageViewer_Edit_Copy_Image'/>"
"        <menuitem action='ImageViewer_Edit_Paste_Image'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='ViewerCommands'>"
"      <toolitem action='ImageViewer_View_ZoomIn'/>"
"      <toolitem action='ImageViewer_View_ZoomOut'/>"
"      <toolitem action='ImageViewer_View_Zoom100'/>"
"      <toolitem action='ImageViewer_View_ZoomFit'/>"
"      <toolitem action='ImageViewer_View_ZoomFitWidth'/>"
"    </placeholder>"
"  </toolbar>"
"  <toolbar name='Fullscreen_ToolBar'>"
"    <placeholder name='ViewerCommands'>"
"      <toolitem action='ImageViewer_View_ZoomIn'/>"
"      <toolitem action='ImageViewer_View_ZoomOut'/>"
"      <toolitem action='ImageViewer_View_Zoom100'/>"
"      <toolitem action='ImageViewer_View_ZoomFit'/>"
"      <toolitem action='ImageViewer_View_ZoomFitWidth'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static void
image_viewer_activate_action_view_zoom_in (GtkAction          *action,
					   GthImageViewerPage *self)
{
	gth_image_viewer_zoom_in (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
image_viewer_activate_action_view_zoom_out (GtkAction          *action,
					    GthImageViewerPage *self)
{
	gth_image_viewer_zoom_out (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
image_viewer_activate_action_view_zoom_100 (GtkAction          *action,
					    GthImageViewerPage *self)
{
	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (self->priv->viewer), 1.0);
}


static void
image_viewer_activate_action_view_zoom_fit (GtkAction          *action,
					    GthImageViewerPage *self)
{
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE);
}


static void
image_viewer_activate_action_view_zoom_fit_width (GtkAction          *action,
						  GthImageViewerPage *self)
{
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_WIDTH);
}


static void
image_viewer_activate_action_edit_undo (GtkAction          *action,
					GthImageViewerPage *self)
{
	gth_image_viewer_page_undo (self);
}


static void
image_viewer_activate_action_edit_redo (GtkAction          *action,
					GthImageViewerPage *self)
{
	gth_image_viewer_page_redo (self);
}


static void
image_viewer_activate_action_edit_copy_image (GtkAction          *action,
					      GthImageViewerPage *self)
{
	gth_image_viewer_page_copy_image (self);
}


static void
image_viewer_activate_action_edit_paste_image (GtkAction          *action,
					       GthImageViewerPage *self)
{
	gth_image_viewer_page_paste_image (self);
}


static GtkActionEntry image_viewer_action_entries[] = {
	{ "ImageViewer_Edit_Undo", GTK_STOCK_UNDO,
	  NULL, "<control>z",
	  NULL,
	  G_CALLBACK (image_viewer_activate_action_edit_undo) },

	{ "ImageViewer_Edit_Redo", GTK_STOCK_REDO,
	  NULL, "<shift><control>z",
	  NULL,
	  G_CALLBACK (image_viewer_activate_action_edit_redo) },

	{ "ImageViewer_Edit_Copy_Image", GTK_STOCK_COPY,
	  N_("Copy Image"), "<control>c",
	  N_("Copy the image to the clipboard"),
	  G_CALLBACK (image_viewer_activate_action_edit_copy_image) },

	{ "ImageViewer_Edit_Paste_Image", GTK_STOCK_PASTE,
	  N_("Paste Image"), "<control>p",
	  N_("Paste the image from the clipboard"),
	  G_CALLBACK (image_viewer_activate_action_edit_paste_image) },

	{ "ImageViewer_View_ZoomIn", GTK_STOCK_ZOOM_IN,
	  N_("In"), "<control>plus",
	  N_("Zoom in"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_in) },

	{ "ImageViewer_View_ZoomOut", GTK_STOCK_ZOOM_OUT,
	  N_("Out"), "<control>minus",
	  N_("Zoom out"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_out) },

	{ "ImageViewer_View_Zoom100", GTK_STOCK_ZOOM_100,
	  N_("1:1"), "<control>0",
	  N_("Actual size"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_100) },

	{ "ImageViewer_View_ZoomFit", GTK_STOCK_ZOOM_FIT,
	  N_("Fit"), "",
	  N_("Zoom to fit window"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_fit) },

	{ "ImageViewer_View_ZoomFitWidth", GTH_STOCK_ZOOM_FIT_WIDTH,
	  N_("Width"), "",
	  N_("Zoom to fit width"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_fit_width) },
};


static void
gth_image_viewer_page_file_loaded (GthImageViewerPage *self,
				   gboolean            success)
{
	if (_g_file_equal (self->priv->last_loaded, self->priv->file_data->file))
		return;

	_g_object_unref (self->priv->last_loaded);
	self->priv->last_loaded = g_object_ref (self->priv->file_data->file);

	gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self),
				     self->priv->file_data,
				     success);
}


static gboolean
viewer_zoom_changed_cb (GtkWidget          *widget,
			GthImageViewerPage *self)
{
	double  zoom;
	char   *text;

	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));

	zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));
	text = g_strdup_printf ("  %d%%  ", (int) (zoom * 100));
	gth_statusbar_set_secondary_text (GTH_STATUSBAR (gth_browser_get_statusbar (self->priv->browser)), text);

	g_free (text);

	return TRUE;
}


static gboolean
viewer_button_press_event_cb (GtkWidget          *widget,
			      GdkEventButton     *event,
			      GthImageViewerPage *self)
{
	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static gboolean
viewer_popup_menu_cb (GtkWidget          *widget,
		      GthImageViewerPage *self)
{
	gth_browser_file_menu_popup (self->priv->browser, NULL);
	return TRUE;
}


static gboolean
viewer_scroll_event_cb (GtkWidget 	   *widget,
		        GdkEventScroll      *event,
		        GthImageViewerPage  *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
viewer_image_map_event_cb (GtkWidget          *widget,
			   GdkEvent           *event,
			   GthImageViewerPage *self)
{
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
	return FALSE;
}


static gboolean
viewer_key_press_cb (GtkWidget          *widget,
		     GdkEventKey        *event,
		     GthImageViewerPage *self)
{
	return gth_browser_viewer_key_press_cb (self->priv->browser, event);
}


static void
_set_action_sensitive (GthImageViewerPage *self,
		       const char         *action_name,
		       gboolean            sensitive)
{
	GtkAction *action;

	if (self->priv->actions == NULL)
		return;
	action = gtk_action_group_get_action (self->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
clipboard_targets_received_cb (GtkClipboard *clipboard,
			       GdkAtom      *atoms,
                               int           n_atoms,
                               gpointer      user_data)
{
	GthImageViewerPage *self = user_data;
	int                 i;

	self->priv->can_paste = FALSE;
	for (i = 0; ! self->priv->can_paste && (i < n_atoms); i++)
		if (atoms[i] == gdk_atom_intern_static_string ("image/png"))
			self->priv->can_paste = TRUE;

	_set_action_sensitive (self, "ImageViewer_Edit_Paste_Image", self->priv->can_paste);

	g_object_unref (self);
}


static void
_gth_image_viewer_page_update_paste_command_sensitivity (GthImageViewerPage *self,
							 GtkClipboard       *clipboard)
{
	self->priv->can_paste = FALSE;
        _set_action_sensitive (self, "ImageViewer_Edit_Paste_Image", FALSE);

	if (clipboard == NULL)
		clipboard = gtk_widget_get_clipboard (GTK_WIDGET (self->priv->viewer), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_targets (clipboard,
				       clipboard_targets_received_cb,
				       g_object_ref (self));
}


static void
clipboard_owner_change_cb (GtkClipboard *clipboard,
                           GdkEvent     *event,
                           gpointer      user_data)
{
	_gth_image_viewer_page_update_paste_command_sensitivity ((GthImageViewerPage *) user_data, clipboard);
}


static void
viewer_realize_cb (GtkWidget *widget,
                   gpointer   user_data)
{
	GthImageViewerPage *self = user_data;
	GtkClipboard       *clipboard;

	clipboard = gtk_widget_get_clipboard (self->priv->viewer, GDK_SELECTION_CLIPBOARD);
	g_signal_connect (clipboard,
	                  "owner_change",
	                  G_CALLBACK (clipboard_owner_change_cb),
	                  self);
}


static void
viewer_unrealize_cb (GtkWidget *widget,
		     gpointer   user_data)
{
	GthImageViewerPage *self = user_data;
	GtkClipboard       *clipboard;

	clipboard = gtk_widget_get_clipboard (self->priv->viewer, GDK_SELECTION_CLIPBOARD);
	g_signal_handlers_disconnect_by_func (clipboard,
	                                      G_CALLBACK (clipboard_owner_change_cb),
	                                      self);
}


static void
image_preloader_requested_ready_cb (GthImagePreloader  *preloader,
				    GthFileData        *requested,
				    GthImage           *image,
				    int                 original_width,
				    int                 original_height,
				    GError             *error,
				    GthImageViewerPage *self)
{
	if (! _g_file_equal (requested->file, self->priv->file_data->file))
		return;

	if ((error != NULL) || (image == NULL)) {
		gth_image_viewer_page_file_loaded (self, FALSE);
		return;
	}

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->viewer),
				    image,
				    original_width,
				    original_height);

	gth_image_history_clear (self->priv->history);
	gth_image_history_add_image (self->priv->history,
				     gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)),
				     FALSE);

	if ((original_width == -1) || (original_height == -1))
		/* In this case the image was loaded at its original size,
		 * so we get the original size from the image surface size. */
		gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer),
						    &original_width,
						    &original_height);
	g_file_info_set_attribute_int32 (self->priv->file_data->info,
					 "frame::width",
					 original_width);
	g_file_info_set_attribute_int32 (self->priv->file_data->info,
					 "frame::height",
					 original_height);

	gth_image_viewer_page_file_loaded (self, TRUE);
}


static void
image_preloader_original_size_ready_cb (GthImagePreloader  *preloader,
				        GthFileData        *requested,
				        GthImage           *image,
				        int                 original_width,
				        int                 original_height,
				        GError             *error,
				        GthImageViewerPage *self)
{
	if (! _g_file_equal (requested->file, self->priv->file_data->file))
		return;

	if (error != NULL)
		return;

	gth_image_viewer_set_better_quality (GTH_IMAGE_VIEWER (self->priv->viewer),
					     image,
					     original_width,
					     original_height);
	gth_image_history_clear (self->priv->history);
	gth_image_history_add_image (self->priv->history,
				     gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)),
				     FALSE);
}


static void
pref_zoom_quality_changed (GSettings *settings,
			   char      *key,
			   gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer),
					   g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_zoom_change_changed (GSettings *settings,
		   	  char      *key,
		   	  gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer),
					  g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_transp_type_changed (GSettings *settings,
			  char      *key,
			  gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_set_transp_type (GTH_IMAGE_VIEWER (self->priv->viewer),
					  g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_TRANSP_TYPE));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_check_type_changed (GSettings *settings,
		   	 char      *key,
		   	 gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_set_check_type (GTH_IMAGE_VIEWER (self->priv->viewer),
					 g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_CHECK_TYPE));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_check_size_changed (GSettings *settings,
		   	 char      *key,
		   	 gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_set_check_size (GTH_IMAGE_VIEWER (self->priv->viewer),
					 g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_CHECK_SIZE));
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
pref_reset_scrollbars_changed (GSettings *settings,
		   	       char      *key,
		   	       gpointer   user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer),
					       g_settings_get_boolean (self->priv->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS));
}


static void
paint_comment_over_image_func (GthImageViewer *image_viewer,
			       cairo_t        *cr,
			       gpointer        user_data)
{
	GthImageViewerPage *self = user_data;
	GthFileData        *file_data = self->priv->file_data;
	GString            *file_info;
	char               *comment;
	const char         *file_date;
	const char         *file_size;
	int                 current_position;
	int                 n_visibles;
	int                 width;
	int                 height;
	GthMetadata        *metadata;
	PangoLayout        *layout;
	PangoAttrList      *attr_list = NULL;
	GError             *error = NULL;
	char               *text;
	static GdkPixbuf   *icon = NULL;
	int                 icon_width;
	int                 icon_height;
	int                 image_width;
	int                 image_height;
	const int           x_padding = 10;
	const int           y_padding = 10;
	int                 max_text_width;
	PangoRectangle      bounds;
	int                 text_x;
	int                 text_y;
	int                 icon_x;
	int                 icon_y;

	file_info = g_string_new ("");

	comment = gth_file_data_get_attribute_as_string (file_data, "general::description");
	if (comment != NULL) {
		g_string_append_printf (file_info, "<b>%s</b>\n\n", comment);
		g_free (comment);
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL)
		file_date = gth_metadata_get_formatted (metadata);
	else
		file_date = g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime");
	file_size = g_file_info_get_attribute_string (file_data->info, "gth::file::display-size");

	gth_browser_get_file_list_info (self->priv->browser, &current_position, &n_visibles);
	gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer), &width, &height);

	g_string_append_printf (file_info,
			        "<small><i>%s - %dx%d (%d%%) - %s</i>\n<tt>%d/%d - %s</tt></small>",
			        file_date,
			        width,
			        height,
			        (int) (gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer)) * 100),
			        file_size,
			        current_position + 1,
			        n_visibles,
				g_file_info_get_attribute_string (file_data->info, "standard::display-name"));

	layout = gtk_widget_create_pango_layout (GTK_WIDGET (self->priv->viewer), NULL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

	if (! pango_parse_markup (file_info->str,
			          -1,
				  0,
				  &attr_list,
				  &text,
				  NULL,
				  &error))
	{
		g_warning ("Failed to set text from markup due to error parsing markup: %s\nThis is the text that caused the error: %s",  error->message, file_info->str);
		g_error_free (error);
		g_object_unref (layout);
		g_string_free (file_info, TRUE);
		return;
	}

	pango_layout_set_attributes (layout, attr_list);
        pango_layout_set_text (layout, text, strlen (text));

        if (icon == NULL) {
        	GIcon *gicon;

        	gicon = g_themed_icon_new (GTK_STOCK_PROPERTIES);
        	icon = _g_icon_get_pixbuf (gicon, 24, _gtk_widget_get_icon_theme (GTK_WIDGET (image_viewer)));

        	g_object_unref (gicon);
        }
	icon_width = gdk_pixbuf_get_width (icon);
	icon_height = gdk_pixbuf_get_height (icon);

	image_width = gdk_window_get_width (gtk_widget_get_window (self->priv->viewer));
	image_height = gdk_window_get_height (gtk_widget_get_window (self->priv->viewer));
	max_text_width = ((image_width * 3 / 4) - icon_width - (x_padding * 3) - (x_padding * 2));

	pango_layout_set_width (layout, max_text_width * PANGO_SCALE);
	pango_layout_get_pixel_extents (layout, NULL, &bounds);

	bounds.width += (2 * x_padding) + (icon_width + x_padding);
	bounds.height = MIN (image_height - icon_height - (y_padding * 2), bounds.height + (2 * y_padding));
	bounds.x = MAX ((image_width - bounds.width) / 2, 0);
	bounds.y = MAX (image_height - bounds.height - (y_padding * 3), 0);

	text_x = bounds.x + x_padding + icon_width + x_padding;
	text_y = bounds.y + y_padding;
	icon_x = bounds.x + x_padding;
	icon_y = bounds.y + (bounds.height - icon_height) / 2;

	cairo_save (cr);

	/* background */

	_cairo_draw_rounded_box (cr, bounds.x, bounds.y, bounds.width, bounds.height, 8.0);
	cairo_set_source_rgba (cr, 0.94, 0.94, 0.94, 0.81);
	cairo_fill (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke (cr);

	/* icon */

	gdk_cairo_set_source_pixbuf (cr, icon, icon_x, icon_y);
	cairo_rectangle (cr, icon_x, icon_y, icon_width, icon_height);
	cairo_fill (cr);

	/* text */

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	pango_cairo_update_layout (cr, layout);
	cairo_move_to (cr, text_x, text_y);
	pango_cairo_show_layout (cr, layout);

	cairo_restore (cr);

        g_free (text);
        pango_attr_list_unref (attr_list);
	g_object_unref (layout);
	g_string_free (file_info, TRUE);
}


static void
gth_image_viewer_page_real_activate (GthViewerPage *base,
				     GthBrowser    *browser)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;

	self->priv->browser = browser;

	self->priv->actions = gtk_action_group_new ("Image Viewer Actions");
	gtk_action_group_set_translation_domain (self->priv->actions, NULL);
	gtk_action_group_add_actions (self->priv->actions,
				      image_viewer_action_entries,
				      G_N_ELEMENTS (image_viewer_action_entries),
				      self);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), self->priv->actions, 0);

	self->priv->preloader = gth_browser_get_image_preloader (browser);
	self->priv->requested_ready_id = g_signal_connect (G_OBJECT (self->priv->preloader),
							   "requested_ready",
							   G_CALLBACK (image_preloader_requested_ready_cb),
							   self);
	self->priv->original_size_ready_id = g_signal_connect (G_OBJECT (self->priv->preloader),
							       "original_size_ready",
							       G_CALLBACK (image_preloader_original_size_ready_cb),
							       self);

	self->priv->viewer = gth_image_viewer_new ();
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer),
					   g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer),
					  g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE));
	gth_image_viewer_set_transp_type (GTH_IMAGE_VIEWER (self->priv->viewer),
					  g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_TRANSP_TYPE));
	gth_image_viewer_set_check_type (GTH_IMAGE_VIEWER (self->priv->viewer),
					 g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_CHECK_TYPE));
	gth_image_viewer_set_check_size (GTH_IMAGE_VIEWER (self->priv->viewer),
					 g_settings_get_enum (self->priv->settings, PREF_IMAGE_VIEWER_CHECK_SIZE));
	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer),
					       g_settings_get_boolean (self->priv->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS));

	gtk_widget_show (self->priv->viewer);

	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "zoom_changed",
			  G_CALLBACK (viewer_zoom_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "popup-menu",
			  G_CALLBACK (viewer_popup_menu_cb),
			  self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"button_press_event",
				G_CALLBACK (viewer_button_press_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"scroll_event",
				G_CALLBACK (viewer_scroll_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"map_event",
				G_CALLBACK (viewer_image_map_event_cb),
				self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "realize",
			  G_CALLBACK (viewer_realize_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "unrealize",
			  G_CALLBACK (viewer_unrealize_cb),
			  self);

	self->priv->image_navigator = gth_image_navigator_new (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_widget_show (self->priv->image_navigator);

	gth_browser_set_viewer_widget (browser, self->priv->image_navigator);
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	/* settings notifications */

	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_ZOOM_QUALITY,
			  G_CALLBACK (pref_zoom_quality_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_ZOOM_CHANGE,
			  G_CALLBACK (pref_zoom_change_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_TRANSP_TYPE,
			  G_CALLBACK (pref_transp_type_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_CHECK_TYPE,
			  G_CALLBACK (pref_check_type_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_CHECK_SIZE,
			  G_CALLBACK (pref_check_size_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_IMAGE_VIEWER_RESET_SCROLLBARS,
			  G_CALLBACK (pref_reset_scrollbars_changed),
			  self);
}


static void
gth_image_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;

	if (self->priv->browser_merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (self->priv->browser), self->priv->browser_merge_id);
		self->priv->browser_merge_id = 0;
	}

	gtk_ui_manager_remove_action_group (gth_browser_get_ui_manager (self->priv->browser), self->priv->actions);
	g_object_unref (self->priv->actions);
	self->priv->actions = NULL;

	g_signal_handler_disconnect (self->priv->preloader, self->priv->requested_ready_id);
	g_signal_handler_disconnect (self->priv->preloader, self->priv->original_size_ready_id);
	self->priv->requested_ready_id = 0;
	self->priv->original_size_ready_id = 0;

	g_object_unref (self->priv->preloader);
	self->priv->preloader = NULL;

	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static void
gth_image_viewer_page_real_show (GthViewerPage *base)
{
	GthImageViewerPage *self;
	GError             *error = NULL;

	self = (GthImageViewerPage*) base;

	if (self->priv->viewer_merge_id != 0)
		return;

	self->priv->viewer_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (self->priv->browser), image_viewer_ui_info, -1, &error);
	if (self->priv->viewer_merge_id == 0) {
		g_warning ("ui building failed: %s", error->message);
		g_error_free (error);
	}

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
}


static void
gth_image_viewer_page_real_hide (GthViewerPage *base)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;

	if (self->priv->viewer_merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (self->priv->browser), self->priv->viewer_merge_id);
		self->priv->viewer_merge_id = 0;
	}
}


static gboolean
gth_image_viewer_page_real_can_view (GthViewerPage *base,
				     GthFileData   *file_data)
{
	g_return_val_if_fail (file_data != NULL, FALSE);

	return _g_mime_type_is_image (gth_file_data_get_mime_type (file_data));
}


#define N_PRELOADERS 4


static void
gth_image_viewer_page_real_view (GthViewerPage *base,
				 GthFileData   *file_data)
{
	GthImageViewerPage *self;
	GthFileStore       *file_store;
	GtkTreeIter         iter;
	int                 i;
	GthFileData        *next_file_data[N_PRELOADERS];
	GthFileData        *prev_file_data[N_PRELOADERS];
	int                 window_width;
	int                 window_height;

	self = (GthImageViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	_g_clear_object (&self->priv->last_loaded);

	if ((self->priv->file_data != NULL)
	    && g_file_equal (file_data->file, self->priv->file_data->file)
	    && (gth_file_data_get_mtime (file_data) == gth_file_data_get_mtime (self->priv->file_data))
	    && ! self->priv->image_changed)
	{
		gth_image_viewer_page_file_loaded (self, TRUE);
		return;
	}

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);
	self->priv->image_changed = FALSE;

	for (i = 0; i < N_PRELOADERS; i++) {
		next_file_data[i] = NULL;
		prev_file_data[i] = NULL;
	}

	file_store = gth_browser_get_file_store (self->priv->browser);
	if (gth_file_store_find_visible (file_store, self->priv->file_data->file, &iter)) {
		GtkTreeIter next_iter;

		next_iter = iter;
		for (i = 0; i < N_PRELOADERS; i++) {
			if (! gth_file_store_get_next_visible (file_store, &next_iter))
				break;
			next_file_data[i] = gth_file_store_get_file (file_store, &next_iter);
		}

		next_iter = iter;
		for (i = 0; i < N_PRELOADERS; i++) {
			if (! gth_file_store_get_prev_visible (file_store, &next_iter))
				break;
			prev_file_data[i] = gth_file_store_get_file (file_store, &next_iter);
		}
	}

	gtk_window_get_size (GTK_WINDOW (self->priv->browser),
			     &window_width,
			     &window_height);

	gth_image_preloader_load (self->priv->preloader,
				  self->priv->file_data,
				  (gth_image_prelaoder_get_load_policy (self->priv->preloader) == GTH_LOAD_POLICY_TWO_STEPS) ? MAX (window_width, window_height) : -1,
				  next_file_data[0],
				  next_file_data[1],
				  next_file_data[2],
				  next_file_data[3],
				  prev_file_data[0],
				  prev_file_data[1],
				  prev_file_data[2],
				  prev_file_data[3],
				  NULL);
}


static void
gth_image_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_IMAGE_VIEWER_PAGE (base)->priv->viewer;
	if (gtk_widget_get_realized (widget) && gtk_widget_get_mapped (widget))
		gtk_widget_grab_focus (widget);
}


static void
gth_image_viewer_page_real_fullscreen (GthViewerPage *base,
				       gboolean       active)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);
	gth_image_navigator_set_automatic_scrollbars (GTH_IMAGE_NAVIGATOR (self->priv->image_navigator), ! active);
}


static void
gth_image_viewer_page_real_show_pointer (GthViewerPage *base,
				         gboolean       show)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage *) base;

	if (show)
		gth_image_viewer_show_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
	else
		gth_image_viewer_hide_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
gth_image_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
	GthImageViewerPage *self;
	gboolean            zoom_enabled;
	double              zoom;
	GthFit              fit_mode;

	self = (GthImageViewerPage*) base;

	_set_action_sensitive (self, "ImageViewer_Edit_Undo", gth_image_history_can_undo (self->priv->history));
	_set_action_sensitive (self, "ImageViewer_Edit_Redo", gth_image_history_can_redo (self->priv->history));

	zoom_enabled = gth_image_viewer_get_zoom_enabled (GTH_IMAGE_VIEWER (self->priv->viewer));
	zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));

	_set_action_sensitive (self, "ImageViewer_View_Zoom100", zoom_enabled && ! FLOAT_EQUAL (zoom, 1.0));
	_set_action_sensitive (self, "ImageViewer_View_ZoomOut", zoom_enabled && (zoom > 0.05));
	_set_action_sensitive (self, "ImageViewer_View_ZoomIn", zoom_enabled && (zoom < 100.0));

	fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer));
	_set_action_sensitive (self, "ImageViewer_View_ZoomFit", zoom_enabled && (fit_mode != GTH_FIT_SIZE));
	_set_action_sensitive (self, "ImageViewer_View_ZoomFitWidth", zoom_enabled && (fit_mode != GTH_FIT_WIDTH));

	_gth_image_viewer_page_update_paste_command_sensitivity (self, NULL);
}


typedef struct {
	GthImageViewerPage *self;
	GthFileData        *file_to_save;
	GthFileData        *original_file;
	FileSavedFunc       func;
	gpointer            user_data;
} SaveData;


static void
save_data_free (SaveData *data)
{
	g_object_unref (data->file_to_save);
	g_object_unref (data->original_file);
	g_free (data);
}


static void
save_image_task_completed_cb (GthTask *task,
			      GError      *error,
			      gpointer     user_data)
{
	SaveData           *data = user_data;
	GthImageViewerPage *self = data->self;
	gboolean            error_occurred;

	error_occurred = error != NULL;

	if (error_occurred) {
		gth_file_data_set_file (data->file_to_save, data->original_file->file);
		g_file_info_set_attribute_boolean (data->file_to_save->info, "gth::file::is-modified", FALSE);
	}

	if (data->func != NULL)
		(data->func) ((GthViewerPage *) self, data->file_to_save, error, data->user_data);
	else if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not save the file"), error);

	if (! error_occurred) {
		GFile *folder;
		GList *file_list;

		folder = g_file_get_parent (data->file_to_save->file);
		file_list = g_list_prepend (NULL, g_object_ref (data->file_to_save->file));
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    folder,
					    file_list,
					    GTH_MONITOR_EVENT_CHANGED);

		_g_object_list_unref (file_list);
		g_object_unref (folder);
	}

	save_data_free (data);
}


static void
_gth_image_viewer_page_real_save (GthViewerPage *base,
				  GFile         *file,
				  const char    *mime_type,
				  FileSavedFunc  func,
				  gpointer       user_data)
{
	GthImageViewerPage *self;
	SaveData           *data;
	GthFileData        *current_file;
	GthImage           *image;
	GthTask            *task;

	self = (GthImageViewerPage *) base;

	data = g_new0 (SaveData, 1);
	data->self = self;
	data->func = func;
	data->user_data = user_data;

	if (mime_type == NULL)
		mime_type = gth_file_data_get_mime_type (self->priv->file_data);

	current_file = gth_browser_get_current_file (self->priv->browser);
	if (current_file == NULL)
		return;

	data->file_to_save = g_object_ref (current_file);
	data->original_file = gth_file_data_dup (current_file);
	if (file != NULL)
		gth_file_data_set_file (data->file_to_save, file);

	/* save the value of 'gth::file::is-modified' into 'gth::file::image-changed'
	 * to allow the exiv2 metadata writer to not change some fields if the
	 * content wasn't modified. */
	g_file_info_set_attribute_boolean (data->file_to_save->info,
					   "gth::file::image-changed",
					   g_file_info_get_attribute_boolean (data->file_to_save->info, "gth::file::is-modified"));

	/* the 'gth::file::is-modified' attribute must be set to false before
	 * saving the file to avoid a scenario where the user is asked whether
	 * he wants to save the file after saving it.
	 * This is because when a file is modified in the current folder the
	 * folder_changed_cb function in gth-browser.c is called automatically
	 * and if the current file has been modified it is reloaded
	 * (see file_attributes_ready_cb in gth-browser.c) and if it has been
	 * modified ('gth::file::is-modified' is TRUE) the user is asked if he
	 * wants to save (see load_file_delayed_cb in gth-browser.c). */
	g_file_info_set_attribute_boolean (data->file_to_save->info, "gth::file::is-modified", FALSE);

	image = gth_image_new_for_surface (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)));
	task = gth_save_image_task_new (image, mime_type, data->file_to_save, GTH_OVERWRITE_RESPONSE_YES);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (save_image_task_completed_cb),
			  data);
	gth_browser_exec_task (GTH_BROWSER (self->priv->browser), task, FALSE);

	_g_object_unref (task);
	_g_object_unref (image);
}


static gboolean
gth_image_viewer_page_real_can_save (GthViewerPage *base)
{
	GArray *savers;

	savers = gth_main_get_type_set ("image-saver");
	return (savers != NULL) && (savers->len > 0);
}


static void
gth_image_viewer_page_real_save (GthViewerPage *base,
				 GFile         *file,
				 FileSavedFunc  func,
				 gpointer       user_data)
{
	_gth_image_viewer_page_real_save (base, file, NULL, func, user_data);
}


/* -- gth_image_viewer_page_real_save_as -- */


typedef struct {
	GthImageViewerPage *self;
	FileSavedFunc       func;
	gpointer            user_data;
	GthFileData        *file_data;
	GtkWidget          *file_sel;
} SaveAsData;


static void
save_as_destroy_cb (GtkWidget  *w,
		    SaveAsData *data)
{
	g_object_unref (data->file_data);
	g_free (data);
}


static void
save_as_response_cb (GtkDialog  *file_sel,
		     int         response,
		     SaveAsData *data)
{
	GFile      *file;
	const char *mime_type;

	if (response != GTK_RESPONSE_OK) {
		if (data->func != NULL) {
			(*data->func) ((GthViewerPage *) data->self,
				       data->file_data,
				       g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""),
				       data->user_data);
		}
		gtk_widget_destroy (GTK_WIDGET (file_sel));
		return;
	}

	if (! gth_file_chooser_dialog_get_file (GTH_FILE_CHOOSER_DIALOG (file_sel), &file, &mime_type))
		return;

	gtk_widget_hide (GTK_WIDGET (data->file_sel));

	gth_file_data_set_file (data->file_data, file);
	_gth_image_viewer_page_real_save ((GthViewerPage *) data->self,
					  file,
					  mime_type,
					  data->func,
					  data->user_data);

	gtk_widget_destroy (GTK_WIDGET (data->file_sel));

	g_object_unref (file);
}


static void
gth_image_viewer_page_real_save_as (GthViewerPage *base,
				    FileSavedFunc  func,
				    gpointer       user_data)
{
	GthImageViewerPage *self;
	GtkWidget          *file_sel;
	char               *uri;
	SaveAsData         *data;

	self = GTH_IMAGE_VIEWER_PAGE (base);
	file_sel = gth_file_chooser_dialog_new (_("Save Image"),
						GTK_WINDOW (self->priv->browser),
						"image-saver");

	uri = g_file_get_uri (self->priv->file_data->file);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (file_sel), uri);

	data = g_new0 (SaveAsData, 1);
	data->self = self;
	data->func = func;
	data->file_data = gth_file_data_dup (self->priv->file_data);
	data->user_data = user_data;
	data->file_sel = file_sel;

	g_signal_connect (GTK_DIALOG (file_sel),
			  "response",
			  G_CALLBACK (save_as_response_cb),
			  data);
	g_signal_connect (G_OBJECT (file_sel),
			  "destroy",
			  G_CALLBACK (save_as_destroy_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);

	g_free (uri);
}


static void
_gth_image_viewer_page_set_image (GthImageViewerPage *self,
				  cairo_surface_t    *image,
				  gboolean            modified)
{
	GthFileData *file_data;
	int          width;
	int          height;
	char        *size;

	if (image == NULL)
		return;

	gth_image_viewer_set_surface (GTH_IMAGE_VIEWER (self->priv->viewer), image, -1, -1);

	file_data = gth_browser_get_current_file (GTH_BROWSER (self->priv->browser));

	g_file_info_set_attribute_boolean (file_data->info, "gth::file::is-modified", modified);

	width = cairo_image_surface_get_width (image);
	height = cairo_image_surface_get_height (image);
	g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
	g_file_info_set_attribute_int32 (file_data->info, "image::height", height);

	size = g_strdup_printf (_("%d × %d"), width, height);
	g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);

	gth_monitor_metadata_changed (gth_main_get_default_monitor (), file_data);

	g_free (size);
}


static void
gth_image_viewer_page_real_revert (GthViewerPage *base)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);
	GthImageData       *idata;

	idata = gth_image_history_revert (self->priv->history);
	if (idata != NULL) {
		_gth_image_viewer_page_set_image (self, idata->image, idata->unsaved);
		gth_image_data_unref (idata);
	}
}


static void
gth_image_viewer_page_real_update_info (GthViewerPage *base,
					GthFileData   *file_data)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);

	if (! _g_file_equal (self->priv->file_data->file, file_data->file))
		return;
	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);

	if (self->priv->viewer == NULL)
		return;

	gtk_widget_queue_draw (self->priv->viewer);
}


static void
gth_image_viewer_page_real_show_properties (GthViewerPage *base,
					    gboolean       show)
{
	GthImageViewerPage *self;

	self = GTH_IMAGE_VIEWER_PAGE (base);

	if (show)
		gth_image_viewer_add_painter (GTH_IMAGE_VIEWER (self->priv->viewer), paint_comment_over_image_func, self);
	else
		gth_image_viewer_remove_painter (GTH_IMAGE_VIEWER (self->priv->viewer), paint_comment_over_image_func, self);
	gtk_widget_queue_draw (self->priv->viewer);
}


static void
gth_image_viewer_page_real_shrink_wrap (GthViewerPage *base,
					gboolean       activate,
					int           *other_width,
					int           *other_height)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE_IF_LARGER);
}


static void
gth_image_viewer_page_finalize (GObject *obj)
{
	GthImageViewerPage *self;

	self = GTH_IMAGE_VIEWER_PAGE (obj);

	g_object_unref (self->priv->settings);
	g_object_unref (self->priv->history);
	_g_object_unref (self->priv->file_data);
	_g_object_unref (self->priv->last_loaded);

	G_OBJECT_CLASS (gth_image_viewer_page_parent_class)->finalize (obj);
}


static void
gth_image_viewer_page_class_init (GthImageViewerPageClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthImageViewerPagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_image_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageInterface *iface)
{
	iface->activate = gth_image_viewer_page_real_activate;
	iface->deactivate = gth_image_viewer_page_real_deactivate;
	iface->show = gth_image_viewer_page_real_show;
	iface->hide = gth_image_viewer_page_real_hide;
	iface->can_view = gth_image_viewer_page_real_can_view;
	iface->view = gth_image_viewer_page_real_view;
	iface->focus = gth_image_viewer_page_real_focus;
	iface->fullscreen = gth_image_viewer_page_real_fullscreen;
	iface->show_pointer = gth_image_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_image_viewer_page_real_update_sensitivity;
	iface->can_save = gth_image_viewer_page_real_can_save;
	iface->save = gth_image_viewer_page_real_save;
	iface->save_as = gth_image_viewer_page_real_save_as;
	iface->revert = gth_image_viewer_page_real_revert;
	iface->update_info = gth_image_viewer_page_real_update_info;
	iface->show_properties = gth_image_viewer_page_real_show_properties;
	iface->shrink_wrap = gth_image_viewer_page_real_shrink_wrap;
}


static void
gth_image_viewer_page_init (GthImageViewerPage *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_VIEWER_PAGE, GthImageViewerPagePrivate);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	self->priv->history = gth_image_history_new ();
	self->priv->last_loaded = NULL;
	self->priv->image_changed = FALSE;
	self->priv->can_paste = FALSE;
	self->priv->viewer_merge_id = 0;
	self->priv->browser_merge_id = 0;
}


GtkWidget *
gth_image_viewer_page_get_image_viewer (GthImageViewerPage *self)
{
	return self->priv->viewer;
}


GdkPixbuf *
gth_image_viewer_page_get_pixbuf (GthImageViewerPage *self)
{
	return gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer));
}


void
gth_image_viewer_page_set_pixbuf (GthImageViewerPage *self,
				  GdkPixbuf          *pixbuf,
				  gboolean            add_to_history)
{
	cairo_surface_t *image;

	image = _cairo_image_surface_create_from_pixbuf (pixbuf);
	gth_image_viewer_page_set_image (self, image, add_to_history);

	cairo_surface_destroy (image);
}


cairo_surface_t *
gth_image_viewer_page_get_image (GthImageViewerPage *self)
{
	return gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
}


void
gth_image_viewer_page_set_image (GthImageViewerPage *self,
			 	 cairo_surface_t    *image,
			 	 gboolean            add_to_history)
{
	if (gth_image_viewer_page_get_image (self) == image)
		return;

	if (add_to_history)
		gth_image_history_add_image (self->priv->history, image, TRUE);

	_gth_image_viewer_page_set_image (self, image, TRUE);
	self->priv->image_changed = TRUE;

	if (add_to_history)
		gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
}


void
gth_image_viewer_page_undo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_undo (self->priv->history);
	if (idata != NULL)
		_gth_image_viewer_page_set_image (self, idata->image, idata->unsaved);
}


void
gth_image_viewer_page_redo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_redo (self->priv->history);
	if (idata != NULL)
		_gth_image_viewer_page_set_image (self, idata->image, idata->unsaved);
}


GthImageHistory *
gth_image_viewer_page_get_history (GthImageViewerPage *self)
{
	return self->priv->history;
}


void
gth_image_viewer_page_reset (GthImageViewerPage *self)
{
	GthImageData *last_image;

	last_image = gth_image_history_get_last (self->priv->history);
	if (last_image == NULL)
		return;

	_gth_image_viewer_page_set_image (self, last_image->image, last_image->unsaved);
}


void
gth_image_viewer_page_copy_image (GthImageViewerPage *self)
{
	cairo_surface_t *image;
	GtkClipboard    *clipboard;
	GdkPixbuf       *pixbuf;

	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	if (image == NULL)
		return;

	clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (self->priv->viewer), GDK_SELECTION_CLIPBOARD);
	pixbuf = _gdk_pixbuf_new_from_cairo_surface (image);
	gtk_clipboard_set_image (clipboard, pixbuf);

	g_object_unref (pixbuf);
}


static void
clipboard_image_received_cb (GtkClipboard *clipboard,
			     GdkPixbuf    *pixbuf,
			     gpointer      user_data)
{
	GthImageViewerPage *self = user_data;

	if (pixbuf != NULL)
		gth_image_viewer_page_set_pixbuf (self, pixbuf, TRUE);
	g_object_unref (self);
}


void
gth_image_viewer_page_paste_image (GthImageViewerPage *self)
{
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (self->priv->viewer), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_image (clipboard,
				     clipboard_image_received_cb,
				     g_object_ref (self));
}
