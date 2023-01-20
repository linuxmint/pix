/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include <string.h>
#include "color-utils.h"
#include "gtk-utils.h"


#define REQUEST_ENTRY_WIDTH_IN_CHARS 40
#define GTHUMB_RESOURCE_BASE_PATH "/org/gnome/gThumb/resources/"


SizeValue
ImageSizeValues[IMAGE_SIZE_N] = {
	{ 320, 200 },
	{ 320, 320 },
	{ 640, 480 },
	{ 640, 640 },
	{ 800, 600 },
	{ 800, 800 },
	{ 1024, 768 },
	{ 1024, 1024 },
	{ 1280, 960 },
	{ 1280, 1280 }
};


GtkWidget*
_gtk_message_dialog_new (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *icon_name,
			 const char       *message,
			 const char       *secondary_message,
			 const char       *first_button_text,
			 ...)
{
	GtkWidget *dialog;

	dialog = g_object_new (GTK_TYPE_MESSAGE_DIALOG,
			       "title", "",
			       "transient-for", parent,
			       "modal", ((flags & GTK_DIALOG_MODAL) == GTK_DIALOG_MODAL),
			       "destroy-with-parent", ((flags & GTK_DIALOG_DESTROY_WITH_PARENT) == GTK_DIALOG_DESTROY_WITH_PARENT),
			       "text", message,
			       NULL);

	if (secondary_message != NULL)
		g_object_set (dialog, "secondary-text", secondary_message, NULL);

	if (flags & GTK_DIALOG_MODAL)
		_gtk_dialog_add_to_window_group (GTK_DIALOG (dialog));

	if (icon_name != NULL) {
		GtkWidget *image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
		gtk_message_dialog_set_image (GTK_MESSAGE_DIALOG (dialog), image);
	}

	/* add the buttons */

	if (first_button_text != NULL) {
		va_list     args;
		const char *text;
		int         response_id;

		va_start (args, first_button_text);
		text = first_button_text;
		response_id = va_arg (args, gint);
		while (text != NULL) {
			gtk_dialog_add_button (GTK_DIALOG (dialog), text, response_id);

			text = va_arg (args, char*);
			if (text == NULL)
				break;
			response_id = va_arg (args, int);
		}
		va_end (args);
	}

	return dialog;
}


GtkWidget*
_gtk_yesno_dialog_new (GtkWindow        *parent,
		       GtkDialogFlags    flags,
		       const char       *message,
		       const char       *no_button_text,
		       const char       *yes_button_text)
{
	GtkWidget *dialog;

	dialog = _gtk_message_dialog_new (parent,
			                  flags,
					  _GTK_ICON_NAME_DIALOG_QUESTION,
					  message,
					  NULL,
					  no_button_text, GTK_RESPONSE_CANCEL,
					  yes_button_text, GTK_RESPONSE_YES,
					  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

	return dialog;
}


void
_gtk_error_dialog_from_gerror_run (GtkWindow   *parent,
				   const char  *title,
				   GError      *gerror)
{
	GtkWidget *d;

	g_return_if_fail (gerror != NULL);

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     gerror->message,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	gtk_dialog_run (GTK_DIALOG (d));

	gtk_widget_destroy (d);
}


static void
error_dialog_response_cb (GtkDialog *dialog,
			  int        response,
			  gpointer   user_data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
_gtk_error_dialog_from_gerror_show (GtkWindow   *parent,
				    const char  *title,
				    GError      *gerror)
{
	GtkWidget *d;

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     (gerror != NULL) ? gerror->message : NULL,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));
}


void
_gtk_error_dialog_run (GtkWindow        *parent,
		       const gchar      *format,
		       ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      _GTK_ICON_NAME_DIALOG_ERROR,
				      message,
				      NULL,
				      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


void
_gtk_error_dialog_show (GtkWindow  *parent,
			const char *title,
			const char *format,
			...)
{
	va_list    args;
	char      *message;
	GtkWidget *d;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     message,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));

	g_free (message);
}


void
_gtk_info_dialog_run (GtkWindow        *parent,
		      const gchar      *format,
		      ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      _GTK_ICON_NAME_DIALOG_INFO,
				      message,
				      NULL,
				      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


void
_gtk_dialog_add_to_window_group (GtkDialog *dialog)
{
	GtkWindow *toplevel;

	g_return_if_fail (dialog != NULL);

	toplevel = _gtk_widget_get_toplevel_if_window (GTK_WIDGET (dialog));
	if ((toplevel != NULL) && gtk_window_has_group (toplevel))
		gtk_window_group_add_window (gtk_window_get_group (toplevel), GTK_WINDOW (dialog));
}


void
_gtk_dialog_add_class_to_response (GtkDialog    *dialog,
				   int           respose_id,
				   const char	*class_name)
{
	gtk_style_context_add_class (gtk_widget_get_style_context (gtk_dialog_get_widget_for_response (dialog, respose_id)), class_name);
}


void
_gtk_dialog_add_action_widget (GtkDialog *dialog,
			       GtkWidget *button)
{
	if (gtk_dialog_get_header_bar (dialog)) {
		GtkWidget *headerbar = gtk_dialog_get_header_bar (dialog);

		gtk_container_add (GTK_CONTAINER (headerbar), button);
		gtk_container_child_set (GTK_CONTAINER (headerbar),
					 button,
					 "pack-type", GTK_PACK_END,
					 NULL);
	}
	else
		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_action_area (dialog)), button);
}


GdkPixbuf *
_g_icon_get_pixbuf (GIcon        *icon,
		    int           icon_size,
		    GtkIconTheme *icon_theme)
{
	GdkPixbuf   *pixbuf = NULL;
	GtkIconInfo *icon_info;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
						    icon,
						    icon_size,
						    GTK_ICON_LOOKUP_USE_BUILTIN);

	if (icon_info != NULL) {
		GError *error = NULL;

		pixbuf = gtk_icon_info_load_icon (icon_info, &error);
		if (error != NULL) {
			g_print ("%s\n", error->message);
			g_error_free (error);
		}

		g_object_unref (icon_info);
	}

	return pixbuf;
}


GdkPixbuf *
get_mime_type_pixbuf (const char   *mime_type,
		      int           icon_size,
		      GtkIconTheme *icon_theme)
{
	GdkPixbuf *pixbuf = NULL;
	GIcon     *icon;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	icon = g_content_type_get_icon (mime_type);
	pixbuf = _g_icon_get_pixbuf (icon, icon_size, icon_theme);
	g_object_unref (icon);

	return pixbuf;
}


void
show_help_dialog (GtkWindow  *parent,
		  const char *section)
{
	char   *uri;
	GError *error = NULL;

	uri = g_strconcat ("help:gthumb", section ? "/" : NULL, section, NULL);
	if (! gtk_show_uri_on_window (parent, uri, GDK_CURRENT_TIME, &error)) {
  		GtkWidget *dialog;

		dialog = _gtk_message_dialog_new (parent,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  _GTK_ICON_NAME_DIALOG_ERROR,
						  _("Could not display help"),
						  error->message,
						  _GTK_LABEL_OK, GTK_RESPONSE_OK,
						  NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);

		g_clear_error (&error);
	}
	g_free (uri);
}


void
_gtk_container_remove_children (GtkContainer *container,
				gpointer      start_after_this,
				gpointer      stop_before_this)
{
	GList *children;
	GList *scan;

	children = gtk_container_get_children (container);

	if (start_after_this != NULL) {
		for (scan = children; scan; scan = scan->next)
			if (scan->data == start_after_this) {
				scan = scan->next;
				break;
			}
	}
	else
		scan = children;

	for (/* void */; scan && (scan->data != stop_before_this); scan = scan->next)
		gtk_container_remove (container, scan->data);
	g_list_free (children);
}


int
_gtk_container_get_pos (GtkContainer *container,
			GtkWidget    *child)
{
	GList    *children;
	gboolean  found = FALSE;
	int       idx;
	GList    *scan;

	children = gtk_container_get_children (container);
	for (idx = 0, scan = children; ! found && scan; idx++, scan = scan->next) {
		if (child == scan->data) {
			found = TRUE;
			break;
		}
	}
	g_list_free (children);

	return ! found ? -1 : idx;
}


guint
_gtk_container_get_n_children (GtkContainer *container)
{
	GList *children;
	guint  len;

	children = gtk_container_get_children (container);
	len = g_list_length (children);
	g_list_free (children);

	return len;
}


GtkBuilder *
_gtk_builder_new_from_file (const char *ui_file,
			    const char *extension)
{
	char       *filename;
	GtkBuilder *builder;
	GError     *error = NULL;

#ifdef RUN_IN_PLACE
	if (extension == NULL)
		filename = g_build_filename (GTHUMB_UI_DIR, ui_file, NULL);
	else
		filename = g_build_filename (GTHUMB_EXTENSIONS_UI_DIR, extension, "data", "ui", ui_file, NULL);
#else
	filename = g_build_filename (GTHUMB_UI_DIR, ui_file, NULL);
#endif

	builder = gtk_builder_new ();
	if (! gtk_builder_add_from_file (builder, filename, &error)) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
	}
	g_free (filename);

	return builder;
}


GtkBuilder *
_gtk_builder_new_from_resource (const char *resource_path)
{
	GtkBuilder *builder;
	char       *full_path;
	GError     *error = NULL;

	builder = gtk_builder_new ();
	full_path = g_strconcat (GTHUMB_RESOURCE_BASE_PATH, resource_path, NULL);
        if (! gtk_builder_add_from_resource (builder, full_path, &error)) {
                g_warning ("%s\n", error->message);
                g_clear_error (&error);
        }
	g_free (full_path);

        return builder;
}


GtkWidget *
_gtk_builder_get_widget (GtkBuilder *builder,
			 const char *name)
{
	return (GtkWidget *) gtk_builder_get_object (builder, name);
}


GtkWidget *
_gtk_combo_box_new_with_texts (const char *first_text,
			       ...)
{
	GtkWidget  *combo_box;
	va_list     args;
	const char *text;

	combo_box = gtk_combo_box_text_new ();

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_box), text);
		text = va_arg (args, const char *);
	}

	va_end (args);

	return combo_box;
}


void
_gtk_combo_box_append_texts (GtkComboBoxText *combo_box,
			     const char      *first_text,
			     ...)
{
	va_list     args;
	const char *text;

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_text_append_text (combo_box, text);
		text = va_arg (args, const char *);
	}

	va_end (args);
}


GtkWidget *
_gtk_image_new_from_xpm_data (char * xpm_data[])
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;

	pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) xpm_data);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_widget_show (image);

	g_object_unref (G_OBJECT (pixbuf));

	return image;
}


gboolean
_gtk_widget_get_screen_size (GtkWidget *widget,
			     int       *width,
			     int       *height)
{
	GdkRectangle geometry;
	gboolean     success;

	if (_gtk_widget_get_monitor_geometry (widget, &geometry)) {
		if (width != NULL) *width = geometry.width;
		if (height != NULL) *height = geometry.height;
		success = TRUE;
	}
	else {
		if (width != NULL) *width = 0;
		if (height != NULL) *height = 0;
		success = FALSE;
	}

	return success;
}


int
_gtk_widget_get_allocated_width (GtkWidget *widget)
{
	int width = 0;

	if ((widget != NULL) && gtk_widget_get_mapped (widget)) {
		width = gtk_widget_get_allocated_width (widget);
		width += gtk_widget_get_margin_start (widget);
		width += gtk_widget_get_margin_end (widget);
	}

	return width;
}


int
_gtk_widget_get_allocated_height (GtkWidget *widget)
{
	int height = 0;

	if ((widget != NULL) && gtk_widget_get_mapped (widget)) {
		height = gtk_widget_get_allocated_height (widget);
		height += gtk_widget_get_margin_top (widget);
		height += gtk_widget_get_margin_bottom (widget);
	}

	return height;
}


int
_gtk_widget_lookup_for_size (GtkWidget   *widget,
			     GtkIconSize  icon_size)
{
	int w, h;

	gtk_icon_size_lookup (icon_size, &w, &h);

	return MAX (w, h);
}


void
_gtk_widget_set_margin (GtkWidget        *widget,
			int               top,
			int               right,
			int               bottom,
			int               left)
{
	gtk_widget_set_margin_top (widget, top);
	gtk_widget_set_margin_end (widget, right);
	gtk_widget_set_margin_bottom (widget, bottom);
	gtk_widget_set_margin_start (widget, left);
}


void
_gtk_tree_path_list_free (GList *list)
{
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}


GtkTreePath *
_gtk_tree_path_get_previous_or_parent (GtkTreePath *path)
{
	int         *indices;
	int          depth;
	int          last;
	gboolean     return_parent;
	int         *new_indices;
	int          new_depth;
	int          i;
	GtkTreePath *new_path;

	indices = gtk_tree_path_get_indices_with_depth (path, &depth);
	if (indices == NULL)
		return NULL;

	last = depth - 1;
	return_parent = indices[last] == 0;
	new_depth = return_parent ? depth - 1 : depth;
	if (new_depth == 0)
		return NULL;

	new_indices = g_new (int, new_depth);
	for (i = 0; i < new_depth; i++)
		new_indices[i] = indices[i];
	if (! return_parent) /* return previous element */
		new_indices[last] = indices[last] - 1;

	new_path = gtk_tree_path_new_from_indicesv (new_indices, new_depth);

	g_free (new_indices);

	return new_path;
}


int
_gtk_paned_get_position2 (GtkPaned *paned)
{
	int           pos;
	GtkAllocation allocation;
	int           size;

	if (! gtk_widget_get_visible (GTK_WIDGET (paned)))
		return 0;

	pos = gtk_paned_get_position (paned);
	if (pos == 0)
		return 0;

	gtk_widget_get_allocation (GTK_WIDGET (paned), &allocation);
	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
		size = allocation.width;
	else
		size = allocation.height;

	if (size == 0)
		return 0;

	return size - pos;
}


void
_g_launch_command (GtkWidget           *parent,
		   const char          *command,
		   const char          *name,
		   GAppInfoCreateFlags  flags,
		   GList               *files)
{
	GError              *error = NULL;
	GAppInfo            *app_info;
	GdkAppLaunchContext *launch_context;

	app_info = g_app_info_create_from_commandline (command, name, flags, &error);
	if (app_info == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (parent), _("Could not launch the application"), error);
		g_clear_error (&error);
		return;
	}

	launch_context = gdk_display_get_app_launch_context (gtk_widget_get_display (parent));
	if (! g_app_info_launch (app_info, files, G_APP_LAUNCH_CONTEXT (launch_context), &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (parent), _("Could not launch the application"), error);
		g_clear_error (&error);
		return;
	}

	g_object_unref (app_info);
}


void
_gtk_window_resize_to_fit_screen_height (GtkWidget *window,
					 int        default_width)
{
	int screen_height;

	gtk_widget_realize (window);
	_gtk_widget_get_screen_size (window, NULL, &screen_height);
	if (screen_height < 768)
		/* maximize on netbooks */
		gtk_window_maximize (GTK_WINDOW (window));
	else
		/* This should fit on a XGA/WXGA (height 768) screen
		 * with top and bottom panels */
		gtk_window_set_default_size (GTK_WINDOW (window), default_width, 670);
}


void
_gtk_info_bar_clear_action_area (GtkInfoBar *info_bar)
{
	_gtk_container_remove_children (GTK_CONTAINER (gtk_info_bar_get_action_area (info_bar)), NULL, NULL);
}


/* -- _gtk_menu_ask_drag_drop_action -- */


typedef struct {
	GMainLoop     *loop;
	GdkDragAction  action_name;
} DropActionData;


static void
ask_drag_drop_action_menu_deactivate_cb (GtkMenuShell *menushell,
					 gpointer      user_data)
{
	DropActionData *drop_data = user_data;

	if (g_main_loop_is_running (drop_data->loop))
		g_main_loop_quit (drop_data->loop);
}


static void
ask_drag_drop_action_item_activate_cb (GtkMenuItem *menuitem,
                		       gpointer     user_data)
{
	DropActionData *drop_data = user_data;

	drop_data->action_name = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "drop-action"));
	if (g_main_loop_is_running (drop_data->loop))
		g_main_loop_quit (drop_data->loop);
}


static void
_gtk_menu_ask_drag_drop_action_append_item (GtkWidget      *menu,
					    const char     *label,
					    GdkDragAction   actions,
					    GdkDragAction   action,
					    DropActionData *drop_data)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_mnemonic (label);
	g_object_set_data (G_OBJECT (item), "drop-action", GINT_TO_POINTER (action));
	gtk_widget_set_sensitive (item, ((actions & action) != 0));
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item,
			  "activate",
			  G_CALLBACK (ask_drag_drop_action_item_activate_cb),
			  drop_data);
}


GdkDragAction
_gtk_menu_ask_drag_drop_action (GtkWidget     *widget,
				GdkDragAction  actions)
{
	DropActionData  drop_data;
	GtkWidget      *menu;
	GtkWidget      *item;

	drop_data.action_name = 0;
	drop_data.loop = g_main_loop_new (NULL, FALSE);

	menu = gtk_menu_new ();
	gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

	_gtk_menu_ask_drag_drop_action_append_item (menu,
						    _("_Copy Here"),
						    actions,
						    GDK_ACTION_COPY,
						    &drop_data);
	_gtk_menu_ask_drag_drop_action_append_item (menu,
						    _("_Move Here"),
						    actions,
						    GDK_ACTION_MOVE,
						    &drop_data);
	_gtk_menu_ask_drag_drop_action_append_item (menu,
						    _("_Link Here"),
						    actions,
						    GDK_ACTION_LINK,
						    &drop_data);

	item = gtk_separator_menu_item_new ();
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	item = gtk_menu_item_new_with_label (_("Cancel"));
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (menu,
			  "deactivate",
			  G_CALLBACK (ask_drag_drop_action_menu_deactivate_cb),
			  &drop_data);

	gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
	gtk_grab_add (menu);
	g_main_loop_run (drop_data.loop);

	gtk_grab_remove (menu);
	gtk_widget_destroy (menu);
	g_main_loop_unref (drop_data.loop);

	return drop_data.action_name;
}


static gboolean
_gdk_rgba_shade (GdkRGBA *color,
		 GdkRGBA *result,
		 gdouble  factor)
{
	guchar hue, sat, lum;
	guchar red, green, blue;
	double tmp;

	gimp_rgb_to_hsl (color->red * 255,
			 color->green * 255,
			 color->blue * 255,
			 &hue,
			 &sat,
			 &lum);

	tmp = factor * (double) sat;
	sat = (guchar) CLAMP (tmp, 0, 255);

	tmp = factor * (double) lum;
	lum = (guchar) CLAMP (tmp, 0, 255);

	gimp_hsl_to_rgb (hue, sat, lum, &red, &green, &blue);

	result->red = (double) red / 255.0;
	result->green = (double) green / 255.0;
	result->blue = (double) blue / 255.0;
	result->alpha = color->alpha;

	return TRUE;
}


gboolean
_gdk_rgba_darker (GdkRGBA *color,
		  GdkRGBA *result)
{
	return _gdk_rgba_shade (color, result, 0.8);
}


gboolean
_gdk_rgba_lighter (GdkRGBA *color,
		   GdkRGBA *result)
{
	return _gdk_rgba_shade (color, result, 1.2);
}


GtkIconTheme *
_gtk_widget_get_icon_theme (GtkWidget *widget)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	if (screen == NULL)
		return NULL;

	return gtk_icon_theme_get_for_screen (screen);
}


void
_gtk_combo_box_add_image_sizes (GtkComboBox *combo_box,
				int          active_width,
				int          active_height)
{
	GtkListStore *list_store;
	int           active_index;
	int           i;

	list_store = GTK_LIST_STORE (gtk_combo_box_get_model (combo_box));
	active_index = 0;
	for (i = 0; i < G_N_ELEMENTS (ImageSizeValues); i++) {
		GtkTreeIter  iter;
		char        *name;

		gtk_list_store_append (list_store, &iter);

		if ((ImageSizeValues[i].width == active_width) && (ImageSizeValues[i].height == active_height))
			active_index = i;

		/* Translators: this is an image size, such as 1024 × 768 */
		name = g_strdup_printf (_("%d × %d"), ImageSizeValues[i].width, ImageSizeValues[i].height);
		gtk_list_store_set (list_store, &iter,
				    0, name,
				    -1);

		g_free (name);
	}
	gtk_combo_box_set_active (combo_box, active_index);
}


gboolean
_gtk_file_chooser_set_file_parent (GtkFileChooser   *chooser,
				   GFile            *file,
				   GError          **error)
{
	GFile *parent;
	gboolean result;

	parent = g_file_get_parent (file);
	result = gtk_file_chooser_set_file (chooser, parent, error);

	if (parent != NULL)
		g_object_unref (parent);

	return result;
}


static void
_gtk_menu_button_set_style_for_header_bar (GtkWidget *button)
{
	GtkStyleContext *context;

	gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
	context = gtk_widget_get_style_context (button);
	gtk_style_context_add_class (context, "image-button");
	gtk_style_context_remove_class (context, "text-button");
}


GtkWidget *
_gtk_menu_button_new_for_header_bar (const char *icon_name)
{
	GtkWidget *button;

	button = gtk_menu_button_new ();
	gtk_menu_button_set_use_popover (GTK_MENU_BUTTON (button), FALSE);
	if (icon_name != NULL) {
		GtkWidget *image;

		image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
		gtk_widget_show (image);
		gtk_container_add (GTK_CONTAINER (button), image);
	}
	_gtk_menu_button_set_style_for_header_bar (button);

	return button;
}


GtkWidget *
_gtk_image_button_new_for_header_bar (const char *icon_name)
{
	GtkWidget *button;

	button = gtk_button_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	_gtk_menu_button_set_style_for_header_bar (button);

	return button;
}


GtkWidget *
_gtk_toggle_image_button_new_for_header_bar (const char *icon_name)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_toggle_button_new ();
	image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (button), image);
	_gtk_menu_button_set_style_for_header_bar (button);

	return button;
}


/* -- _gtk_window_add_accelerator_for_action -- */


typedef struct {
	GtkWindow *window;
	char      *action_name;
	GVariant  *target;
} AccelData;


static void
accel_data_free (gpointer  user_data,
                 GClosure *closure)
{
	AccelData *accel_data = user_data;

	g_return_if_fail (accel_data != NULL);

	if (accel_data->target != NULL)
		g_variant_unref (accel_data->target);
	g_free (accel_data->action_name);
	g_free (accel_data);
}


static void
window_accelerator_activated_cb (GtkAccelGroup	*accel_group,
				 GObject		*object,
				 guint		 key,
				 GdkModifierType	 mod,
				 gpointer		 user_data)
{
	AccelData *accel_data = user_data;
	GAction   *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (accel_data->window), accel_data->action_name);
	if (action != NULL)
		g_action_activate (action, accel_data->target);
}


void
_gtk_window_add_accelerator_for_action (GtkWindow	*window,
					GtkAccelGroup	*accel_group,
					const char	*action_name,
					const char	*accel,
					GVariant	*target)
{
	AccelData	*accel_data;
	guint		 key;
	GdkModifierType  mods;
	GClosure	*closure;

	if ((action_name == NULL) || (accel == NULL))
		return;

	if (g_str_has_prefix (action_name, "app."))
		return;

	accel_data = g_new0 (AccelData, 1);
	accel_data->window = window;
	/* remove the win. prefix from the action name */
	if (g_str_has_prefix (action_name, "win."))
		accel_data->action_name = g_strdup (action_name + strlen ("win."));
	else
		accel_data->action_name = g_strdup (action_name);
	if (target != NULL)
		accel_data->target = g_variant_ref (target);

	gtk_accelerator_parse (accel, &key, &mods);
	closure = g_cclosure_new (G_CALLBACK (window_accelerator_activated_cb),
				  accel_data,
				  accel_data_free);
	gtk_accel_group_connect (accel_group,
				 key,
				 mods,
				 0,
				 closure);
}


/* -- _gtk_window_add_accelerators_from_menu --  */


static void
add_accelerators_from_menu_item (GtkWindow      *window,
				 GtkAccelGroup  *accel_group,
				 GMenuModel     *model,
				 int             item)
{
	GMenuAttributeIter	*iter;
	const char		*key;
	GVariant		*value;
	const char		*accel = NULL;
	const char		*action = NULL;
	GVariant		*target = NULL;

	iter = g_menu_model_iterate_item_attributes (model, item);
	while (g_menu_attribute_iter_get_next (iter, &key, &value)) {
		if (g_str_equal (key, "action") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
			action = g_variant_get_string (value, NULL);
		else if (g_str_equal (key, "accel") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
			accel = g_variant_get_string (value, NULL);
		else if (g_str_equal (key, "target"))
			target = g_variant_ref (value);
		g_variant_unref (value);
	}
	g_object_unref (iter);

	_gtk_window_add_accelerator_for_action (window,
						accel_group,
						action,
						accel,
						target);

	if (target != NULL)
		g_variant_unref (target);
}


static void
add_accelerators_from_menu (GtkWindow      *window,
			    GtkAccelGroup  *accel_group,
			    GMenuModel     *model)
{
	int		 i;
	GMenuLinkIter	*iter;
	const char	*key;
	GMenuModel	*m;

	for (i = 0; i < g_menu_model_get_n_items (model); i++) {
		add_accelerators_from_menu_item (window, accel_group, model, i);

		iter = g_menu_model_iterate_item_links (model, i);
		while (g_menu_link_iter_get_next (iter, &key, &m)) {
			add_accelerators_from_menu (window, accel_group, m);
			g_object_unref (m);
		}
		g_object_unref (iter);
	}
}


void
_gtk_window_add_accelerators_from_menu (GtkWindow  *window,
					GMenuModel *menu)
{
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();
	add_accelerators_from_menu (window, accel_group, menu);
	gtk_window_add_accel_group (window, accel_group);
}


gboolean
_gtk_window_get_is_maximized (GtkWindow *window)
{
	GdkWindow *gdk_win;

	gdk_win = gtk_widget_get_window (GTK_WIDGET (window));
	if (gdk_win == NULL)
		return FALSE;

	return (gdk_window_get_state (gdk_win) & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}


gboolean
_gtk_window_get_monitor_info (GtkWindow	    *window,
			      GdkRectangle  *geometry,
			      int           *number,
			      char         **name)
{
#if GTK_CHECK_VERSION(3, 22, 0)

	GdkWindow  *win;
	GdkMonitor *monitor;

	win = gtk_widget_get_window (GTK_WIDGET (window));
	if (win == NULL)
		return FALSE;

	monitor = gdk_display_get_monitor_at_window (gdk_window_get_display (win), win);
	if (monitor == NULL)
		return FALSE;

	if (geometry != NULL)
		gdk_monitor_get_geometry (monitor, geometry);

	if ((number != NULL) || (name != NULL)) {
		GdkDisplay *display;
		int         monitor_num;
		const char *monitor_name = NULL;
		int         i;

		display = gdk_monitor_get_display (monitor);
		monitor_num = 0;
		for (i = 0; /* void */; i++) {
			GdkMonitor *m = gdk_display_get_monitor (display, i);
			if (m == monitor) {
				monitor_num = i;
				monitor_name = gdk_monitor_get_model (monitor);
				break;
			}
			if (m == NULL)
				break;
		}

		if (number != NULL) *number = monitor_num;
		if (name != NULL) *name = g_strdup (monitor_name);
	}

#else

	GdkWindow *win;
	GdkScreen *screen;
	int        monitor_num;

	win = gtk_widget_get_window (GTK_WIDGET (window));
	if (win == NULL)
		return FALSE;

	screen = gdk_window_get_screen (win);
	if (screen == NULL)
		return FALSE;

	monitor_num = gdk_screen_get_monitor_at_window (screen, win);
	if (number != NULL)
		*number = monitor_num;
	if (geometry != NULL)
		gdk_screen_get_monitor_geometry (screen, monitor_num, geometry);
	if (name != NULL)
		*name = gdk_screen_get_monitor_plug_name (screen, monitor_num);

#endif

	return TRUE;
}


gboolean
_gtk_widget_get_monitor_geometry (GtkWidget    *widget,
				  GdkRectangle *geometry)
{
	gboolean   result = FALSE;
	GtkWindow *window;

	window = _gtk_widget_get_toplevel_if_window (widget);
	if ((window != NULL) && (_gtk_window_get_monitor_info (window, geometry, NULL, NULL)))
		result = TRUE;

	return result;
}


GdkDevice *
_gtk_widget_get_client_pointer (GtkWidget *widget)
{
	GdkDisplay *display;
	GdkSeat    *seat;;

	display = gtk_widget_get_display (widget);
	if (display == NULL)
		return NULL;

	seat = gdk_display_get_default_seat (display);
	if (seat == NULL)
		return NULL;

	return gdk_seat_get_pointer (seat);
}


void
_gtk_list_box_add_separator (GtkListBox *list_box) {
	GtkWidget *row;
	GtkWidget *sep;

	row = gtk_list_box_row_new ();
#if GTK_CHECK_VERSION(3, 14, 0)
	gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), FALSE);
	gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW (row), FALSE);
#endif
	gtk_widget_show (row);
	gtk_container_add (GTK_CONTAINER (list_box), row);

	sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show (sep);
	gtk_container_add (GTK_CONTAINER (row), sep);
}


gboolean
_gtk_settings_get_dialogs_use_header (void)
{
	gboolean use_header;

	g_object_get (gtk_settings_get_default (),
		      "gtk-dialogs-use-header", &use_header,
		      NULL);

	return use_header;
}


GdkCursor *
_gdk_cursor_new_for_widget (GtkWidget     *widget,
			    GdkCursorType  cursor_type)
{
	return gdk_cursor_new_for_display (gtk_widget_get_display (widget), cursor_type);
}


void
_gtk_widget_reparent (GtkWidget *widget,
		      GtkWidget *new_parent)
{
	g_object_ref (widget);
	gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (widget)), widget);
	gtk_container_add (GTK_CONTAINER (new_parent), widget);
	g_object_unref (widget);
}


GtkWindow *
_gtk_widget_get_toplevel_if_window (GtkWidget *widget)
{
	GtkWidget *window;

	window = gtk_widget_get_toplevel (widget);
	if (! GTK_IS_WINDOW (window))
		return NULL;

	return GTK_WINDOW (window);
}
