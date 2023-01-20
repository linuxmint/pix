/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "gth-contact-sheet-theme-dialog.h"

#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


struct _GthContactSheetThemeDialogPrivate {
	GtkBuilder           *builder;
	GtkWidget            *copy_from_button;
	GtkWidget            *copy_from_menu;
	GthContactSheetTheme *theme;
	GList                *all_themes;
};


G_DEFINE_TYPE_WITH_CODE (GthContactSheetThemeDialog,
			 gth_contact_sheet_theme_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthContactSheetThemeDialog))


static void
gth_contact_sheet_theme_dialog_finalize (GObject *object)
{
	GthContactSheetThemeDialog *self;

	self = GTH_CONTACT_SHEET_THEME_DIALOG (object);

	_g_object_unref (self->priv->builder);
	gth_contact_sheet_theme_unref (self->priv->theme);
	gth_contact_sheet_theme_list_free (self->priv->all_themes);

	G_OBJECT_CLASS (gth_contact_sheet_theme_dialog_parent_class)->finalize (object);
}


static void
gth_contact_sheet_theme_dialog_class_init (GthContactSheetThemeDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_contact_sheet_theme_dialog_finalize;
}


static gboolean
preview_area_draw_cb (GtkWidget *widget,
		      cairo_t   *cr,
		      gpointer   user_data)
{
	GthContactSheetThemeDialog *self = user_data;

	gth_contact_sheet_theme_paint_preview (self->priv->theme,
					       cr,
					       gdk_window_get_width (gtk_widget_get_window (widget)),
					       gdk_window_get_height (gtk_widget_get_window (widget)));

	return TRUE;
}


static void
update_theme_from_controls (GthContactSheetThemeDialog *self)
{
	self->priv->theme->display_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))));

	/* background */

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("solid_color_radiobutton")))) {
		self->priv->theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID;
		gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("solid_color_colorpicker")), &self->priv->theme->background_color1);
	}
	else {
		gboolean h_gradient_active = FALSE;
		gboolean v_gradient_active = FALSE;

		h_gradient_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("h_gradient_checkbutton")));
		v_gradient_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("v_gradient_checkbutton")));

		if (h_gradient_active && v_gradient_active) {
			self->priv->theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL;
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &self->priv->theme->background_color1);
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &self->priv->theme->background_color2);
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &self->priv->theme->background_color3);
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &self->priv->theme->background_color4);
		}
		else if (h_gradient_active) {
			self->priv->theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL;
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &self->priv->theme->background_color1);
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &self->priv->theme->background_color2);
		}
		else if (v_gradient_active) {
			self->priv->theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL;
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &self->priv->theme->background_color1);
			gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &self->priv->theme->background_color2);
		}
	}

	/* frame */

	self->priv->theme->frame_style = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("frame_style_combobox")));
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("frame_colorpicker")), &self->priv->theme->frame_color);

	/* header */

	self->priv->theme->header_font_name = g_strdup (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (GET_WIDGET ("header_fontpicker"))));
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("header_colorpicker")), &self->priv->theme->header_color);

	/* footer */

	self->priv->theme->footer_font_name = g_strdup (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (GET_WIDGET ("footer_fontpicker"))));
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("footer_colorpicker")), &self->priv->theme->footer_color);

	/* caption */

	self->priv->theme->caption_font_name = g_strdup (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (GET_WIDGET ("caption_fontpicker"))));
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("caption_colorpicker")), &self->priv->theme->caption_color);
}


static void
update_preview (GthContactSheetThemeDialog *self)
{
	update_theme_from_controls (self);
	gtk_widget_queue_draw (GET_WIDGET ("preview_area"));
}


static void
h_gradient_swap_button_clicked_cb (GtkButton *button,
				   gpointer   user_data)
{
	GthContactSheetThemeDialog *self = user_data;
	GdkRGBA                     color1;
	GdkRGBA                     color2;

	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &color1);
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &color2);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &color1);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &color2);
	update_preview (self);
}


static void
v_gradient_swap_button_clicked_cb (GtkButton *button,
				   gpointer   user_data)
{
	GthContactSheetThemeDialog *self = user_data;
	GdkRGBA                     color1;
	GdkRGBA                     color2;

	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &color1);
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &color2);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &color1);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &color2);
	update_preview (self);
}


static void
gth_contact_sheet_theme_dialog_init (GthContactSheetThemeDialog *self)
{
	GtkWidget *content;

	self->priv = gth_contact_sheet_theme_dialog_get_instance_private (self);
	self->priv->builder = _gtk_builder_new_from_file ("contact-sheet-theme-properties.ui", "contact_sheet");
	self->priv->theme = NULL;
	self->priv->all_themes = NULL;

	gtk_window_set_title (GTK_WINDOW (self), _("Theme Properties"));
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	content = _gtk_builder_get_widget (self->priv->builder, "theme_properties");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	/* "Copy from" button */

	self->priv->copy_from_button = gtk_menu_button_new ();
	gtk_container_add (GTK_CONTAINER (self->priv->copy_from_button), gtk_label_new_with_mnemonic (_("Copy _From")));
	gtk_widget_show_all (self->priv->copy_from_button);

	self->priv->copy_from_menu = gtk_menu_new ();
	gtk_menu_button_set_popup (GTK_MENU_BUTTON (self->priv->copy_from_button), self->priv->copy_from_menu);

	gtk_dialog_add_action_widget (GTK_DIALOG (self), self->priv->copy_from_button, 100);

	/* other buttons */

	gtk_dialog_add_button (GTK_DIALOG (self),
			       _GTK_LABEL_CANCEL,
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self),
			       _GTK_LABEL_SAVE,
			       GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);

	g_signal_connect (GET_WIDGET ("preview_area"),
			  "draw",
			  G_CALLBACK (preview_area_draw_cb),
			  self);
	g_signal_connect_swapped (GET_WIDGET ("solid_color_radiobutton"),
			  	  "toggled",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("gradient_radiobutton"),
			  	  "toggled",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("h_gradient_checkbutton"),
			  	  "toggled",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("v_gradient_checkbutton"),
			  	  "toggled",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("solid_color_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("h_gradient_1_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("h_gradient_2_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("v_gradient_1_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("v_gradient_2_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("frame_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("header_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("footer_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("caption_colorpicker"),
			  	  "color-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("frame_style_combobox"),
			  	  "changed",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("header_fontpicker"),
			  	  "font-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("footer_fontpicker"),
			  	  "font-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect_swapped (GET_WIDGET ("caption_fontpicker"),
			  	  "font-set",
			  	  G_CALLBACK (update_preview),
			  	  self);
	g_signal_connect (GET_WIDGET ("h_gradient_swap_button"),
			  "clicked",
			  G_CALLBACK (h_gradient_swap_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("v_gradient_swap_button"),
			  "clicked",
			  G_CALLBACK (v_gradient_swap_button_clicked_cb),
			  self);
}


static GthContactSheetTheme *
_gth_contact_sheet_theme_new_default (void)
{
	GthContactSheetTheme *theme;

	theme = gth_contact_sheet_theme_new ();
	theme->display_name = g_strdup (_("New theme"));

	theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID;
	gdk_rgba_parse (&theme->background_color1, "#fff");
	gdk_rgba_parse (&theme->background_color2, "#fff");
	gdk_rgba_parse (&theme->background_color3, "#fff");
	gdk_rgba_parse (&theme->background_color4, "#fff");

	theme->frame_style = GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE_WITH_SHADOW;
	gdk_rgba_parse (&theme->frame_color, "#000");

	theme->header_font_name = g_strdup ("Sans 22");
	gdk_rgba_parse (&theme->header_color, "#000");

	theme->footer_font_name = g_strdup ("Sans Bold 12");
	gdk_rgba_parse (&theme->footer_color, "#000");

	theme->caption_font_name = g_strdup ("Sans 8");
	gdk_rgba_parse (&theme->caption_color, "#000");

	return theme;
}


static void
update_controls_from_theme (GthContactSheetThemeDialog *self,
			    GthContactSheetTheme       *theme)
{
	GthContactSheetTheme *default_theme = NULL;

	if (theme == NULL)
		theme = default_theme = _gth_contact_sheet_theme_new_default ();
	self->priv->theme = gth_contact_sheet_theme_dup (theme);

	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), theme->display_name);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("solid_color_radiobutton")), theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("gradient_radiobutton")), theme->background_type != GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("h_gradient_checkbutton")), (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL) || (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("v_gradient_checkbutton")), (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL) || (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL));

	if (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID) {
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color1);
	}
	else if (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL) {
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color2);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color3);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color4);
	}
	else {
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color2);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color2);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("frame_style_combobox")), theme->frame_style);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("frame_colorpicker")), &theme->frame_color);

	gtk_font_chooser_set_font (GTK_FONT_CHOOSER (GET_WIDGET ("header_fontpicker")), theme->header_font_name);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("header_colorpicker")), &theme->header_color);

	gtk_font_chooser_set_font (GTK_FONT_CHOOSER (GET_WIDGET ("footer_fontpicker")), theme->footer_font_name);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("footer_colorpicker")), &theme->footer_color);

	gtk_font_chooser_set_font (GTK_FONT_CHOOSER (GET_WIDGET ("caption_fontpicker")), theme->caption_font_name);
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("caption_colorpicker")), &theme->caption_color);

	update_preview (self);

	gth_contact_sheet_theme_unref (default_theme);
}


static void
copy_from_menu_item_activate_cb (GtkMenuItem *menu_item,
         			 gpointer     user_data)
{
	GthContactSheetThemeDialog *self = user_data;
	char                       *display_name;
	GFile                      *file;
	GthContactSheetTheme       *theme;

	if ((self->priv->theme != NULL) && (self->priv->theme->file != NULL))
		file = g_file_dup (self->priv->theme->file);
	else
		file = NULL;
	display_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))));

	theme = g_object_get_data (G_OBJECT (menu_item), "theme");
	if (theme != NULL)
		update_controls_from_theme (self, theme);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), display_name);

	_g_object_unref (self->priv->theme->file);
	self->priv->theme->file = _g_object_ref (file);

	g_free (display_name);
	_g_object_unref (file);
}


static void
gth_contact_sheet_theme_dialog_construct (GthContactSheetThemeDialog *self,
					  GthContactSheetTheme       *theme)
{
	GList *scan;

	for (scan = self->priv->all_themes; scan; scan = scan->next) {
		GthContactSheetTheme *other_theme = scan->data;
		GtkWidget            *menu_item;

		if ((theme != NULL) && g_file_equal (theme->file, other_theme->file))
			continue;

		menu_item = gtk_menu_item_new_with_label (other_theme->display_name);
		g_object_set_data (G_OBJECT (menu_item), "theme", other_theme);
		gtk_widget_show (menu_item);
		g_signal_connect (menu_item,
				  "activate",
				  G_CALLBACK (copy_from_menu_item_activate_cb),
				  self);

		gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->copy_from_menu), menu_item);
	}

	update_controls_from_theme (self, theme);
	gtk_widget_queue_draw (GET_WIDGET ("preview_area"));
}


GtkWidget *
gth_contact_sheet_theme_dialog_new (GthContactSheetTheme *theme,
				    GList                *all_themes)
{
	GthContactSheetThemeDialog *self;

	self = g_object_new (GTH_TYPE_CONTACT_SHEET_THEME_DIALOG, NULL);
	self->priv->all_themes = gth_contact_sheet_theme_list_copy (all_themes);
	gth_contact_sheet_theme_dialog_construct (self, theme);

	return (GtkWidget *) self;
}


GthContactSheetTheme *
gth_contact_sheet_theme_dialog_get_theme (GthContactSheetThemeDialog *self)
{
	update_theme_from_controls (self);
	return gth_contact_sheet_theme_dup (self->priv->theme);
}
