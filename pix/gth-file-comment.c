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

#include <config.h>
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-file-comment.h"
#include "gth-main.h"
#include "gth-property-view.h"


#define GTH_STYLE_CLASS_COMMENT "comment"


struct _GthFileCommentPrivate {
	GtkWidget   *comment_view;
	GtkWidget   *comment_win;
	GthFileData *last_file_data;
};


static void gth_file_comment_gth_property_view_interface_init (GthPropertyViewInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthFileComment,
			 gth_file_comment,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthFileComment)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
						gth_file_comment_gth_property_view_interface_init))


static gboolean
gth_file_comment_real_can_view (GthPropertyView *base,
				GthFileData     *file_data)
{
	GthMetadata *value;
	gboolean     value_available = FALSE;

	if (file_data == NULL)
		return FALSE;

	value_available = FALSE;
	value = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (value != NULL) {
		const char *formatted = gth_metadata_get_formatted (value);
		if ((formatted != NULL) && (*formatted != '\0'))
			value_available = TRUE;
	}

	return value_available;
}


static char *
get_comment (GthFileData *file_data)
{
	GString     *string;
	GthMetadata *value;
	gboolean     not_void = FALSE;

	string = g_string_new (NULL);

	value = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (value != NULL) {
		const char *formatted;

		formatted = gth_metadata_get_formatted (value);
		if ((formatted != NULL) && (*formatted != '\0')) {
			g_string_append (string, formatted);
			not_void = TRUE;
		}
	}

	return g_string_free (string, ! not_void);
}


static void
gth_file_comment_real_set_file (GthPropertyView *base,
		 		   GthFileData     *file_data)
{
	GthFileComment *self;
	GtkTextBuffer  *text_buffer;
	char           *comment;

	self = GTH_FILE_COMMENT (base);

	if (file_data != self->priv->last_file_data) {
		_g_object_unref (self->priv->last_file_data);
		self->priv->last_file_data = gth_file_data_dup (file_data);
	}

	if (file_data == NULL)
		return;

	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->comment_view));
	comment = get_comment (file_data);
	if (comment != NULL) {
		GtkTextIter    iter;
		GtkAdjustment *vadj;

		gtk_text_buffer_set_text (text_buffer, comment, strlen (comment));
		gtk_text_buffer_get_iter_at_line (text_buffer, &iter, 0);
		gtk_text_buffer_place_cursor (text_buffer, &iter);

		vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->priv->comment_win));
		gtk_adjustment_set_value (vadj, 0.0);

		g_free (comment);
	}
}


static const char *
gth_file_comment_real_get_name (GthPropertyView *self)
{
	return _("Comment");
}


static const char *
gth_file_comment_real_get_icon (GthPropertyView *self)
{
	return "comment-symbolic";
}


static void
gth_file_comment_finalize (GObject *base)
{
	GthFileComment *self;

	self = (GthFileComment *) base;
	_g_object_unref (self->priv->last_file_data);

	G_OBJECT_CLASS (gth_file_comment_parent_class)->finalize (base);
}



static void
gth_file_comment_class_init (GthFileCommentClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_file_comment_finalize;
}


static void
gth_file_comment_init (GthFileComment *self)
{
	self->priv = gth_file_comment_get_instance_private (self);
	self->priv->last_file_data = NULL;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

	self->priv->comment_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->comment_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->priv->comment_win), GTK_SHADOW_ETCHED_IN);
	//gtk_widget_set_size_request (self->priv->comment_win, -1, COMMENT_DEFAULT_HEIGHT);
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->comment_win), GTH_STYLE_CLASS_COMMENT);
	gtk_box_pack_start (GTK_BOX (self), self->priv->comment_win, TRUE, TRUE, 0);
	gtk_widget_show (self->priv->comment_win);

	self->priv->comment_view = gtk_text_view_new ();
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->comment_view), GTH_STYLE_CLASS_COMMENT);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (self->priv->comment_view), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self->priv->comment_view), GTK_WRAP_WORD);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (self->priv->comment_view), TRUE);

	gtk_widget_show (self->priv->comment_view);
	gtk_container_add (GTK_CONTAINER (self->priv->comment_win), self->priv->comment_view);
}


static void
gth_file_comment_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->get_name = gth_file_comment_real_get_name;
	iface->get_icon = gth_file_comment_real_get_icon;
	iface->can_view = gth_file_comment_real_can_view;
	iface->set_file = gth_file_comment_real_set_file;
}
