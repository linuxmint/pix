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

#include "gth-file-viewer-page.h"


struct _GthFileViewerPagePrivate {
	GthBrowser     *browser;
	GtkWidget      *viewer;
	GtkWidget      *icon;
	GtkWidget      *label;
	GthFileData    *file_data;
	GthThumbLoader *thumb_loader;
};


static void gth_viewer_page_interface_init (GthViewerPageInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthFileViewerPage,
			 gth_file_viewer_page,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthFileViewerPage)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_VIEWER_PAGE,
						gth_viewer_page_interface_init))


static gboolean
viewer_scroll_event_cb (GtkWidget 	     *widget,
			GdkEventScroll       *event,
			GthFileViewerPage   *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
viewer_button_press_cb (GtkWidget         *widget,
		        GdkEventButton    *event,
		        GthFileViewerPage *self)
{
	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static gboolean
viewer_popup_menu_cb (GtkWidget         *widget,
		      GthFileViewerPage *self)
{
	gth_browser_file_menu_popup (self->priv->browser, NULL);
	return TRUE;
}


static void
gth_file_viewer_page_real_activate (GthViewerPage *base,
				    GthBrowser    *browser)
{
	GthFileViewerPage *self;
	GtkWidget         *vbox1;
	GtkWidget         *vbox2;

	self = (GthFileViewerPage*) base;

	self->priv->browser = browser;
	self->priv->thumb_loader = gth_thumb_loader_new (128);

	self->priv->viewer = gtk_event_box_new ();
	gtk_widget_add_events (self->priv->viewer, GDK_SCROLL_MASK);
	gtk_widget_show (self->priv->viewer);

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous (GTK_BOX (vbox1), TRUE);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (self->priv->viewer), vbox1);

	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

	self->priv->icon = gtk_image_new ();
	gtk_widget_show (self->priv->icon);
	gtk_box_pack_start (GTK_BOX (vbox2), self->priv->icon, FALSE, FALSE, 0);

	self->priv->label = gtk_label_new ("…");
	gtk_label_set_selectable (GTK_LABEL (self->priv->label), TRUE);
	gtk_widget_show (self->priv->label);
	gtk_box_pack_start (GTK_BOX (vbox2), self->priv->label, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "scroll-event",
			  G_CALLBACK (viewer_scroll_event_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "button_press_event",
			  G_CALLBACK (viewer_button_press_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "popup-menu",
			  G_CALLBACK (viewer_popup_menu_cb),
			  self);

	gth_browser_set_viewer_widget (browser, self->priv->viewer);
}


static void
gth_file_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthFileViewerPage *self;

	self = (GthFileViewerPage*) base;
	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static void
gth_file_viewer_page_real_show (GthViewerPage *base)
{
	/* void */
}


static void
gth_file_viewer_page_real_hide (GthViewerPage *base)
{
	/* void */
}


static gboolean
gth_file_viewer_page_real_can_view (GthViewerPage *base,
				    GthFileData   *file_data)
{
	g_return_val_if_fail (file_data != NULL, FALSE);

	return TRUE;
}


typedef struct {
	GthFileViewerPage *self;
	GthFileData       *file_data;
} ViewData;


static void
view_data_free (ViewData *view_data)
{
	g_object_unref (view_data->file_data);
	g_object_unref (view_data->self);
	g_free (view_data);
}


static void
thumb_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	ViewData          *view_data = user_data;
	GthFileViewerPage *self = view_data->self;
	gboolean           success;
	cairo_surface_t   *image = NULL;

	success = gth_thumb_loader_load_finish (GTH_THUMB_LOADER (source_object),
						result,
						&image,
						NULL);
	if (g_file_equal (self->priv->file_data->file, view_data->file_data->file)) {
		if (success) {
			GdkPixbuf *pixbuf;

			pixbuf = _gdk_pixbuf_new_from_cairo_surface (image);
			gtk_image_set_from_pixbuf (GTK_IMAGE (self->priv->icon), pixbuf);

			_g_object_unref (pixbuf);
		}

		gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, NULL, TRUE);
	}

	cairo_surface_destroy (image);
	view_data_free (view_data);
}


static void
gth_file_viewer_page_real_view (GthViewerPage *base,
				GthFileData   *file_data)
{
	GthFileViewerPage *self;
	GIcon             *icon;
	ViewData          *view_data;

	self = (GthFileViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	gtk_label_set_text (GTK_LABEL (self->priv->label), g_file_info_get_display_name (file_data->info));
	icon = g_file_info_get_symbolic_icon (file_data->info);
	if (icon != NULL)
		gtk_image_set_from_gicon (GTK_IMAGE (self->priv->icon), icon, GTK_ICON_SIZE_DIALOG);

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = g_object_ref (file_data);

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	view_data = g_new0 (ViewData, 1);
	view_data->self = g_object_ref (self);
	view_data->file_data = g_object_ref (file_data);
	gth_thumb_loader_load (self->priv->thumb_loader,
			       view_data->file_data,
			       NULL,
			       thumb_loader_ready_cb,
			       view_data);
}


static void
gth_file_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_FILE_VIEWER_PAGE (base)->priv->label;
	if (gtk_widget_get_realized (widget) && gtk_widget_get_mapped (widget))
		gtk_widget_grab_focus (widget);
}


static void
gth_file_viewer_page_real_fullscreen (GthViewerPage *base,
				      gboolean       active)
{
	/* void */
}


static void
gth_file_viewer_page_real_show_pointer (GthViewerPage *base,
				        gboolean       show)
{
	/* void */
}


static void
gth_file_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
}


static gboolean
gth_file_viewer_page_real_can_save (GthViewerPage *base)
{
	return FALSE;
}


static void
gth_file_viewer_page_real_update_info (GthViewerPage *base,
				       GthFileData   *file_data)
{
	GthFileViewerPage *self = GTH_FILE_VIEWER_PAGE (base);

	if (! _g_file_equal (self->priv->file_data->file, file_data->file))
		return;
	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);
}


static void
gth_file_viewer_page_finalize (GObject *obj)
{
	GthFileViewerPage *self;

	self = GTH_FILE_VIEWER_PAGE (obj);

	_g_object_unref (self->priv->file_data);
	_g_object_unref (self->priv->thumb_loader);

	G_OBJECT_CLASS (gth_file_viewer_page_parent_class)->finalize (obj);
}


static void
gth_file_viewer_page_class_init (GthFileViewerPageClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_file_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageInterface *iface)
{
	iface->activate = gth_file_viewer_page_real_activate;
	iface->deactivate = gth_file_viewer_page_real_deactivate;
	iface->show = gth_file_viewer_page_real_show;
	iface->hide = gth_file_viewer_page_real_hide;
	iface->can_view = gth_file_viewer_page_real_can_view;
	iface->view = gth_file_viewer_page_real_view;
	iface->focus = gth_file_viewer_page_real_focus;
	iface->fullscreen = gth_file_viewer_page_real_fullscreen;
	iface->show_pointer = gth_file_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_file_viewer_page_real_update_sensitivity;
	iface->can_save = gth_file_viewer_page_real_can_save;
	iface->update_info = gth_file_viewer_page_real_update_info;
}


static void
gth_file_viewer_page_init (GthFileViewerPage *self)
{
	self->priv = gth_file_viewer_page_get_instance_private (self);
	self->priv->thumb_loader = NULL;
	self->priv->file_data = NULL;
}
