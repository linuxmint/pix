/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2011 The Free Software Foundation, Inc.
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
#include <libintl.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-file-data.h"
#include "gth-file-selection.h"
#include "gth-file-store.h"
#include "gth-file-view.h"
#include "gth-icon-cache.h"
#include "gth-grid-view.h"
#include "gth-marshal.h"
#include "gth-enum-types.h"
#include "gtk-utils.h"


#define GTH_GRID_VIEW_ITEM(x)      ((GthGridViewItem *)(x))
#define GTH_GRID_VIEW_LINE(x)      ((GthGridViewLine *)(x))
#define CAPTION_LINE_SPACING       4
#define DEFAULT_CAPTION_SPACING    4
#define DEFAULT_CAPTION_PADDING    2
#define DEFAULT_CELL_SPACING       16
#define DEFAULT_CELL_PADDING       5
#define DEFAULT_THUMBNAIL_BORDER   3
#define SCROLL_DELAY               30
#define LAYOUT_DELAY               20
#define RUBBERBAND_BORDER          2
#define STEP_INCREMENT             0.10
#define PAGE_INCREMENT             0.33


static void gth_grid_view_gth_file_selection_interface_init (GthFileSelectionInterface *iface);
static void gth_grid_view_gth_file_view_interface_init (GthFileViewInterface *iface);
static void gth_grid_view_gtk_scrollable_interface_init (GtkScrollableInterface *iface);


enum {
	SELECT_ALL,
	UNSELECT_ALL,
	MOVE_CURSOR,
	SELECT_CURSOR_ITEM,
	TOGGLE_CURSOR_ITEM,
	ACTIVATE_CURSOR_ITEM,
	LAST_SIGNAL
};


enum {
	PROP_0,
	PROP_CAPTION,
	PROP_CELL_SPACING,
	PROP_HADJUSTMENT,
	PROP_HSCROLL_POLICY,
	PROP_MODEL,
	PROP_THUMBNAIL_SIZE,
	PROP_ACTIVATE_ON_SINGLE_CLICK,
	PROP_VADJUSTMENT,
	PROP_VSCROLL_POLICY
};


typedef enum {
	SYNC_INSERT,
	SYNC_REMOVE
} SyncType;


static guint grid_view_signals[LAST_SIGNAL] = { 0 };


typedef struct {
	/* data */

	guint                  ref;
	GthFileData           *file_data;
	cairo_surface_t       *thumbnail;
	gboolean               is_icon : 1;
	char                  *caption;
	gboolean               is_image : 1;
	gboolean               is_video : 1;
	gboolean               has_alpha : 1;
	ItemStyle	       style;

	/* item state */

	GtkStateFlags          state;
	GtkStateFlags          tmp_state;
	gboolean               update_caption_height : 1;

	/* geometry info */

	cairo_rectangle_int_t  area;          /* union of thumbnail_area and caption_area */
	cairo_rectangle_int_t  thumbnail_area;
	cairo_rectangle_int_t  pixbuf_area;
	cairo_rectangle_int_t  caption_area;
} GthGridViewItem;


typedef struct {
	int    y;
	int    height;
	GList *items;
} GthGridViewLine;


struct _GthGridViewPrivate {
	GtkTreeModel          *model;
	GPtrArray             *itemv;
	int                    n_items;
	GList                 *lines;
	GList                 *selection;
	int                    focused_item;
	int                    first_focused_item;  /* Used to do multiple selection with the keyboard. */
	guint                  make_focused_visible : 1;
	double                 initial_vscroll;
	guint                  needs_relayout : 1;
	guint                  needs_relayout_after_size_allocate : 1;

	gboolean               activate_on_single_click;
	guint                  activate_pending : 1; /* postpone activation on button release */

	guint                  layout_timeout;
	int                    relayout_from_line;
	guint                  update_caption_height : 1;

	int                    width;               /* size of the view */
	int                    height;
	int                    thumbnail_size;
	int                    thumbnail_border;
	int                    cell_size;           /* max size of any cell area */
	int                    cell_spacing;        /* vertical space and mininum horizontal space between adjacent cell areas */
	double                 cell_x_spacing;      /* horizontal space between adjacent cell areas (calculated automatically to fill the horizontal space uniformly). */
	int                    cell_padding;        /* space between the cell area border and its content */
	int                    caption_spacing;     /* space between the thumbnail area and the caption area */
	int                    caption_padding;     /* space between the caption area border and its content */

	guint                  scroll_timeout;      /* timeout ID for autoscrolling */
	double                 autoscroll_y_delta;  /* change the adjustment value by this amount when autoscrolling */

	double                 event_last_x;        /* mouse position for autoscrolling */
	double                 event_last_y;

	/* selection */

	guint                  selecting : 1;       /* whether the user is performing a rubberband selection. */
	guint                  select_pending : 1;  /* whether selection is pending after a button press. */
	int                    select_pending_pos;
	GthGridViewItem       *select_pending_item;
	GtkSelectionMode       selection_mode;
	cairo_rectangle_int_t  selection_area;
	int                    last_selected_pos;
	guint                  multi_selecting_with_keyboard : 1; /* Whether a multi selection with the keyboard has started. */
	guint                  selection_changed : 1;
	int                    sel_start_x;         /* The point where the mouse selection started. */
	int                    sel_start_y;
	guint                  sel_state;           /* Modifier state when the selection began. */

	/* drag and drop */

	guint                  dragging : 1;        /* Whether the user is dragging items. */
	guint                  drag_started : 1;    /* Whether the drag has started. */
	gboolean               drag_source_enabled;
	GdkModifierType        drag_start_button_mask;
	int                    drag_button;
	GtkTargetList         *drag_target_list;
	GdkDragAction          drag_actions;
	int                    drag_start_x;        /* The point where the drag started. */
	int                    drag_start_y;
	int                    drop_item;
	GthDropPosition        drop_pos;

	/*  */

	GtkAdjustment         *hadjustment;
	GtkAdjustment         *vadjustment;
	GdkWindow             *bin_window;

	char                  *caption_attributes;
	char                 **caption_attributes_v;
	PangoLayout           *caption_layout;
	gboolean               no_caption;

	GthIconCache          *icon_cache;
};


G_DEFINE_TYPE_WITH_CODE (GthGridView,
			 gth_grid_view,
			 GTK_TYPE_WIDGET,
			 G_ADD_PRIVATE (GthGridView)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_FILE_SELECTION,
						gth_grid_view_gth_file_selection_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_FILE_VIEW,
						gth_grid_view_gth_file_view_interface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE,
					 	gth_grid_view_gtk_scrollable_interface_init))


/* -- gth_grid_view_item -- */


static void
gth_grid_view_item_set_file_data (GthGridViewItem *item,
	          	  	  GthFileData     *file_data)
{
	_g_object_unref (item->file_data);
	item->file_data = _g_object_ref (file_data);

	item->is_video = (item->file_data != NULL) ? _g_mime_type_is_video (gth_file_data_get_mime_type (item->file_data)) : FALSE;
	item->is_image = (item->file_data != NULL) ? _g_mime_type_is_image (gth_file_data_get_mime_type (item->file_data)) : FALSE;
}


static void
gth_grid_view_item_set_thumbnail (GthGridViewItem *item,
				  cairo_surface_t *thumbnail)
{
	cairo_surface_destroy (item->thumbnail);
	item->thumbnail = cairo_surface_reference (thumbnail);

	if (item->thumbnail != NULL) {
		item->pixbuf_area.width = cairo_image_surface_get_width (item->thumbnail);
		item->pixbuf_area.height = cairo_image_surface_get_height (item->thumbnail);
		item->has_alpha = _cairo_image_surface_get_has_alpha (item->thumbnail);
	}
	else {
		item->pixbuf_area.width = 0;
		item->pixbuf_area.height = 0;
		item->has_alpha = FALSE;
	}

	item->pixbuf_area.x = item->thumbnail_area.x  + ((item->thumbnail_area.width - item->pixbuf_area.width) / 2);
	item->pixbuf_area.y = item->thumbnail_area.y  + ((item->thumbnail_area.height - item->pixbuf_area.height) / 2);
}


#define MAX_TEXT_LENGTH     70
#define ODD_ROW_ATTR_STYLE  " size='small'"
#define EVEN_ROW_ATTR_STYLE " size='small' style='italic'"


static void
gth_grid_view_item_update_caption (GthGridViewItem  *item,
				   char            **attributes_v)
{
	GString  *metadata;
	gboolean  odd;
	int       i;

	item->update_caption_height = TRUE;

	g_free (item->caption);
	item->caption = NULL;

	if ((item->file_data == NULL)
	    || (attributes_v == NULL)
	    || g_str_equal (attributes_v[0], "none"))
	{
		return;
	}

	metadata = g_string_new (NULL);

	odd = TRUE;
	for (i = 0; attributes_v[i] != NULL; i++) {
		char *value;

		value = gth_file_data_get_attribute_as_string (item->file_data, attributes_v[i]);
		if ((value != NULL) && ! g_str_equal (value, "")) {
			char *escaped;
			char *style;

			if (metadata->len > 0)
				g_string_append (metadata, "\n");
			if (g_utf8_strlen (value, -1) > MAX_TEXT_LENGTH) {
				char *tmp;

				tmp = g_strdup (value);
				g_utf8_strncpy (tmp, value, MAX_TEXT_LENGTH);
				g_free (value);
				value = g_strdup_printf ("%s…", tmp);

				g_free (tmp);
			}

			escaped = g_markup_escape_text (value, -1);
			if (strcmp (attributes_v[i], "general::rating") == 0)
				style = "";
			else
				style = (odd ? ODD_ROW_ATTR_STYLE : EVEN_ROW_ATTR_STYLE);
			g_string_append_printf (metadata, "<span%s>%s</span>", style, escaped);

			g_free (escaped);
		}
		odd = ! odd;

		g_free (value);
	}

	item->caption = g_string_free (metadata, FALSE);
}


static GthGridViewItem *
gth_grid_view_item_new (GthGridView      *grid_view,
			GthFileData      *file_data,
			cairo_surface_t  *thumbnail,
			gboolean          is_icon,
			char            **attributes_v)
{
	GthGridViewItem *item;

	item = g_new0 (GthGridViewItem, 1);
	item->ref = 1;
	gth_grid_view_item_set_file_data (item, file_data);
	gth_grid_view_item_set_thumbnail (item, thumbnail);
	item->is_icon = is_icon;
	gth_grid_view_item_update_caption (item, attributes_v);

	return item;
}


static GthGridViewItem *
gth_grid_view_item_ref (GthGridViewItem *item)
{
	if (item != NULL)
		item->ref++;
	return item;
}


static void
gth_grid_view_item_unref (GthGridViewItem *item)
{
	if ((item == NULL) || (--item->ref > 0))
		return;

	g_free (item->caption);
	cairo_surface_destroy (item->thumbnail);
	_g_object_unref (item->file_data);
	g_free (item);
}


/* -- gth_grid_view_line -- */


static void
gth_grid_view_line_free (GthGridViewLine *line)
{
	g_list_foreach (line->items, (GFunc) gth_grid_view_item_unref, NULL);
	g_list_free (line->items);
	g_free (line);
}


/**/


static void
_gth_grid_view_free_lines (GthGridView *self)
{
	g_list_foreach (self->priv->lines, (GFunc) gth_grid_view_line_free, NULL);
	g_list_free (self->priv->lines);
	self->priv->lines = NULL;
	self->priv->height = 0;
}


static void
_gth_grid_view_free_items (GthGridView *self)
{
	g_ptr_array_unref (self->priv->itemv);
	self->priv->itemv = NULL;
	self->priv->n_items = 0;

	g_list_free (self->priv->selection);
	self->priv->selection = NULL;
}


static void
gth_grid_view_finalize (GObject *object)
{
	GthGridView *self;

	self = GTH_GRID_VIEW (object);

	if (self->priv->layout_timeout != 0) {
		g_source_remove (self->priv->layout_timeout);
		self->priv->layout_timeout = 0;
	}

	if (self->priv->scroll_timeout != 0) {
		g_source_remove (self->priv->scroll_timeout);
		self->priv->scroll_timeout = 0;
	}

	_gth_grid_view_free_items (self);
	_gth_grid_view_free_lines (self);
	g_list_free (self->priv->selection);

	if (self->priv->hadjustment != NULL) {
		_g_signal_handlers_disconnect_by_data (self->priv->hadjustment, self);
		g_object_unref (self->priv->hadjustment);
		self->priv->hadjustment = NULL;
	}

	if (self->priv->vadjustment != NULL) {
		_g_signal_handlers_disconnect_by_data (self->priv->vadjustment, self);
		g_object_unref (self->priv->vadjustment);
		self->priv->vadjustment = NULL;
	}

	if (self->priv->drag_target_list != NULL) {
		gtk_target_list_unref (self->priv->drag_target_list);
		self->priv->drag_target_list = NULL;
	}
	g_free (self->priv->caption_attributes);
	g_strfreev (self->priv->caption_attributes_v);
	_g_object_unref (self->priv->model);

	G_OBJECT_CLASS (gth_grid_view_parent_class)->finalize (object);
}


static void
adjustment_value_changed (GtkAdjustment *adj,
			  GthGridView   *self)
{
	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		gdk_window_move (self->priv->bin_window,
				 (int) - gtk_adjustment_get_value (self->priv->hadjustment),
				 (int) - gtk_adjustment_get_value (self->priv->vadjustment));
}


static void
gth_grid_view_map (GtkWidget *widget)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	gtk_widget_set_mapped (widget, TRUE);
	gdk_window_show (self->priv->bin_window);
	gdk_window_show (gtk_widget_get_window (widget));
}


static void
_gth_grid_view_stop_dragging (GthGridView *self)
{
	if (! self->priv->dragging)
		return;

	self->priv->dragging = FALSE;
	self->priv->drag_started = FALSE;
}


static void
gth_grid_view_unmap (GtkWidget *widget)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	_gth_grid_view_stop_dragging (self);
	GTK_WIDGET_CLASS (gth_grid_view_parent_class)->unmap (widget);
}


/* -- _gth_grid_view_make_item_fully_visible -- */


static void
gth_grid_view_scroll_to (GthFileView *file_view,
			 int          pos,
			 double       yalign)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);
	int          n_line;
	int          y;
	int          i;
	GList       *line;

	g_return_if_fail ((pos >= 0) && (pos < self->priv->n_items));
	g_return_if_fail ((yalign >= 0.0) && (yalign <= 1.0));

	if (self->priv->lines == NULL)
		return;

	n_line = pos / gth_grid_view_get_items_per_line (self);
	y = self->priv->cell_spacing;
	for (i = 0, line = self->priv->lines;
	     (i < n_line) && (line != NULL);
	     i++, line = line->next)
	{
		y += GTH_GRID_VIEW_LINE (line->data)->height + self->priv->cell_spacing;
	}

	if (line != NULL) {
		int    h;
		double value;

		h = gtk_widget_get_allocated_height (GTK_WIDGET (self)) - GTH_GRID_VIEW_LINE (line->data)->height - self->priv->cell_spacing;
		value = CLAMP ((y - (h * yalign) - ((1.0 - yalign) * self->priv->cell_spacing)),
			       0.0,
			       self->priv->height - gtk_widget_get_allocated_height (GTK_WIDGET (self)));
		gtk_adjustment_set_value (self->priv->vadjustment, value);
	}
}



static void
gth_grid_view_set_vscroll (GthFileView *file_view,
			   double       vscroll)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	self->priv->initial_vscroll = vscroll;
	gtk_adjustment_set_value (self->priv->vadjustment, vscroll);
}


static GthVisibility
gth_grid_view_get_visibility (GthFileView *file_view,
			      int          pos)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);
	int          cell_top;
	int          line_n;
	int          i;
	GList       *line;
	int          cell_bottom;
	int          window_top;
	int          window_bottom;

	g_return_val_if_fail ((pos >= 0) && (pos < self->priv->n_items), GTH_VISIBILITY_NONE);

	if (self->priv->lines == NULL)
		return GTH_VISIBILITY_NONE;

	cell_top = self->priv->cell_spacing;
	line_n = pos / gth_grid_view_get_items_per_line (self);
	for (i = 0, line = self->priv->lines;
	     (i < line_n) && (line != NULL);
	     i++, line = line->next)
	{
		cell_top += GTH_GRID_VIEW_LINE (line->data)->height + self->priv->cell_spacing;
	}

	if (line == NULL)
		return GTH_VISIBILITY_NONE;

	cell_bottom = cell_top + GTH_GRID_VIEW_LINE (line->data)->height + self->priv->cell_spacing;
	window_top = gtk_adjustment_get_value (self->priv->vadjustment);
	window_bottom = window_top + gtk_widget_get_allocated_height (GTK_WIDGET (self));

	if (cell_bottom < window_top)
		return GTH_VISIBILITY_NONE;

	if (cell_top > window_bottom)
		return GTH_VISIBILITY_NONE;

	if ((cell_top >= window_top) && (cell_bottom <= window_bottom))
		return GTH_VISIBILITY_FULL;

	if ((cell_top < window_top) && (cell_bottom >= window_top))
		return GTH_VISIBILITY_PARTIAL_TOP;

	if ((cell_top <= window_bottom) && (cell_bottom > window_bottom))
		return GTH_VISIBILITY_PARTIAL_BOTTOM;

	return GTH_VISIBILITY_PARTIAL;
}


static void
_gth_grid_view_make_item_fully_visible (GthGridView *self,
					int          pos)
{
	GthVisibility visibility;

	if (pos < 0)
		return;

	visibility = gth_grid_view_get_visibility (GTH_FILE_VIEW (self), pos);
	if (visibility != GTH_VISIBILITY_FULL) {
		double y_alignment = -1.0;

		switch (visibility) {
		case GTH_VISIBILITY_NONE:
			y_alignment = 0.5;
			break;

		case GTH_VISIBILITY_PARTIAL_TOP:
			y_alignment = 0.0;
			break;

		case GTH_VISIBILITY_PARTIAL_BOTTOM:
			y_alignment = 1.0;
			break;

		case GTH_VISIBILITY_PARTIAL:
		case GTH_VISIBILITY_FULL:
			y_alignment = -1.0;
			break;
		}

		if (y_alignment >= 0.0)
			gth_grid_view_scroll_to (GTH_FILE_VIEW (self),
						 pos,
						 y_alignment);
	}
}


/* -- grid layout -- */


static void
_gth_grid_view_update_item_size (GthGridView     *self,
				 GthGridViewItem *item)
{
	int thumbnail_size;

	thumbnail_size = self->priv->cell_size - (self->priv->cell_padding * 2);

	if (item->is_icon
	    || item->has_alpha
	    || ((item->pixbuf_area.width < self->priv->thumbnail_size) && (item->pixbuf_area.height < self->priv->thumbnail_size))
            || (item->file_data == NULL))
	{
		item->style = ITEM_STYLE_ICON;
	}
	else if (item->is_video)
		item->style = ITEM_STYLE_VIDEO;
	else
		item->style = ITEM_STYLE_IMAGE;

	switch (item->style) {
	case ITEM_STYLE_VIDEO:
		item->thumbnail_area.width = item->pixbuf_area.width;
		item->thumbnail_area.height = thumbnail_size - (self->priv->thumbnail_border * 2);
		break;
	case ITEM_STYLE_IMAGE:
		item->thumbnail_area.width = item->pixbuf_area.width + (self->priv->thumbnail_border * 2);
		item->thumbnail_area.height = item->pixbuf_area.height + (self->priv->thumbnail_border * 2);
		break;
	case ITEM_STYLE_ICON:
		item->thumbnail_area.width = thumbnail_size;
		item->thumbnail_area.height = thumbnail_size;
		break;
	}

	item->caption_area.width = thumbnail_size;

	item->area.width = self->priv->cell_size;
	item->area.height = self->priv->cell_padding + thumbnail_size;

	if ((self->priv->caption_layout != NULL) && (self->priv->update_caption_height || item->update_caption_height)) {
		if ((item->caption != NULL) && (g_strcmp0 (item->caption, "") != 0)) {
			pango_layout_set_markup (self->priv->caption_layout, item->caption, -1);
			pango_layout_get_pixel_size (self->priv->caption_layout, NULL, &item->caption_area.height);
			item->caption_area.height += self->priv->caption_padding * 2;
		}
		else
			item->caption_area.height = 0;

		item->update_caption_height = FALSE;
	}

	if (item->caption_area.height > 0)
		item->area.height += self->priv->caption_spacing + item->caption_area.height;

	item->area.height += self->priv->cell_padding;
}


static void
_gth_grid_view_place_item_at (GthGridView     *self,
			      GthGridViewItem *item,
			      int              x,
			      int              y)
{
	item->area.x = x;
	item->area.y = y;

	switch (item->style) {
	case ITEM_STYLE_VIDEO:
	case ITEM_STYLE_IMAGE:
		item->thumbnail_area.x = item->area.x + ((self->priv->cell_size - item->thumbnail_area.width) / 2);
		if (self->priv->no_caption)
			item->thumbnail_area.y = item->area.y + ((self->priv->cell_size - item->thumbnail_area.height) / 2);
		else
			item->thumbnail_area.y = item->area.y + self->priv->cell_size - self->priv->cell_padding - item->thumbnail_area.height;
		break;
	case ITEM_STYLE_ICON:
		item->thumbnail_area.x = item->area.x + self->priv->cell_padding;
		item->thumbnail_area.y = item->area.y + self->priv->cell_padding;
		break;
	}

	item->pixbuf_area.x = item->thumbnail_area.x + ((item->thumbnail_area.width - item->pixbuf_area.width) / 2);
	item->pixbuf_area.y = item->thumbnail_area.y + ((item->thumbnail_area.height - item->pixbuf_area.height) / 2);

	item->caption_area.x = item->area.x + self->priv->cell_padding;
	item->caption_area.y = item->area.y + self->priv->cell_size - self->priv->cell_padding + self->priv->caption_spacing;
}


static void
_gth_grid_view_layout_line (GthGridView     *self,
			    GthGridViewLine *line)
{
	double x;
	int    direction;
	GList *scan;

	if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
		x = self->priv->width - self->priv->cell_size;
		direction = -1;
	}
	else {
		x = 0;
		direction = 1;
	}

	for (scan = line->items; scan; scan = scan->next) {
		GthGridViewItem *item = scan->data;

		x += direction * self->priv->cell_x_spacing;
		_gth_grid_view_place_item_at (self, item, round (x), line->y);
		x += direction * self->priv->cell_size;
		x += direction * self->priv->cell_x_spacing;
	}
}


static GthGridViewLine *
_gth_grid_view_line_new (GthGridView *self,
		     	 GList       *items,
		     	 int          y,
		     	 int          height)
{
	GthGridViewLine *line;

	line = g_new0 (GthGridViewLine, 1);
	line->items = g_list_reverse (items);
	line->y = y;
	line->height = height;
	_gth_grid_view_layout_line (self, line);

	return line;
}


static void
_gth_grid_view_configure_hadjustment (GthGridView *self)
{
	double page_size;
	double value;

	if (self->priv->hadjustment == NULL)
		return;

	page_size = gtk_widget_get_allocated_width (GTK_WIDGET (self));
	value = gtk_adjustment_get_value (self->priv->hadjustment);
	if (value + page_size > self->priv->width)
		value = MAX (self->priv->width - page_size, 0);

	gtk_adjustment_configure (self->priv->hadjustment,
				  value,
				  0.0,
				  MAX (page_size, self->priv->width),
				  STEP_INCREMENT * page_size,
				  PAGE_INCREMENT * page_size,
				  page_size);
}


static void
_gth_grid_view_configure_vadjustment (GthGridView *self)
{
	double page_size;
	double value;

	if (self->priv->vadjustment == NULL)
		return;

	page_size = gtk_widget_get_allocated_height (GTK_WIDGET (self));
	value = gtk_adjustment_get_value (self->priv->vadjustment);
	if (value + page_size > self->priv->height)
		value = MAX (self->priv->height - page_size, 0);

	gtk_adjustment_configure (self->priv->vadjustment,
				  value,
				  0.0,
				  MAX (page_size, self->priv->height),
				  STEP_INCREMENT * page_size,
				  PAGE_INCREMENT * page_size,
				  page_size);
}


static void
_gth_grid_view_relayout_at (GthGridView *self,
			    int          pos,
			    int          y)
{
	GList *new_lines;
	int    items_per_line;
	GList *items;
	int    max_height;
	int    n;

	new_lines = NULL;
	items_per_line = gth_grid_view_get_items_per_line (self);
	items = NULL;
	max_height = 0;
	for (n = pos; n < self->priv->n_items;  n++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, n);

		if ((n % items_per_line) == 0) {
			if (items != NULL) {
				new_lines = g_list_prepend (new_lines, _gth_grid_view_line_new (self, items, y, max_height));
				items = NULL;
				y += max_height + self->priv->cell_spacing;
			}
			max_height = 0;
		}

		_gth_grid_view_update_item_size (self, item);
		max_height = MAX (item->area.height, max_height);

		items = g_list_prepend (items, gth_grid_view_item_ref (item));
	}

	if (items != NULL) {
		new_lines = g_list_prepend (new_lines, _gth_grid_view_line_new (self, items, y, max_height));
		y += max_height + self->priv->cell_spacing;
	}

	self->priv->lines = g_list_concat (self->priv->lines, g_list_reverse (new_lines));

	if (y != self->priv->height) {
		GtkAllocation allocation;

		self->priv->height = y;

		gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
		gdk_window_resize (self->priv->bin_window,
				   MAX (self->priv->width, allocation.width),
				   MAX (self->priv->height, allocation.height));
	}

	_gth_grid_view_configure_hadjustment (self);
	_gth_grid_view_configure_vadjustment (self);
}


static void
_gth_grid_view_free_lines_from (GthGridView *self,
				int          first_line)
{
	GList *lines;
	GList *scan;

	lines = g_list_nth (self->priv->lines, first_line);
	if (lines == NULL)
		return;

	/* truncate self->priv->lines before the first line to free */
	if (lines->prev != NULL)
		lines->prev->next = NULL;
	else
		self->priv->lines = NULL;

	for (scan = lines; scan; scan = scan->next)
		gth_grid_view_line_free (GTH_GRID_VIEW_LINE (scan->data));
	g_list_free (lines);
}


static void
_gth_grid_view_relayout_from_line (GthGridView *self,
				   int          line)
{
	int    y;
	GList *scan;

	if (! gtk_widget_get_realized (GTK_WIDGET (self))) {
		self->priv->needs_relayout = TRUE;
		return;
	}

	if (self->priv->update_caption_height)
		pango_layout_set_width (self->priv->caption_layout,
				        (self->priv->cell_size - (self->priv->cell_padding * 2)) * PANGO_SCALE);

	_gth_grid_view_free_lines_from (self, line);
	y = self->priv->cell_spacing;
	for (scan = self->priv->lines; scan; scan = scan->next)
		y += GTH_GRID_VIEW_LINE (scan->data)->height + self->priv->cell_spacing;

	_gth_grid_view_relayout_at (self,
				    line * gth_grid_view_get_items_per_line (self),
				    y);

	self->priv->update_caption_height = FALSE;
	self->priv->relayout_from_line = -1;
	self->priv->needs_relayout = FALSE;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


static gboolean
_gth_grid_view_relayout_cb (gpointer data)
{
	GthGridView *self = data;

	if (self->priv->layout_timeout != 0) {
		g_source_remove (self->priv->layout_timeout);
		self->priv->layout_timeout = 0;
	}

	_gth_grid_view_relayout_from_line (self, self->priv->relayout_from_line);

	if (self->priv->make_focused_visible) {
		self->priv->make_focused_visible = FALSE;
		_gth_grid_view_make_item_fully_visible (self, self->priv->focused_item);
	}

	if (self->priv->initial_vscroll > 0) {
		gtk_adjustment_set_value (self->priv->vadjustment, self->priv->initial_vscroll);
		self->priv->initial_vscroll = 0;
	}

	return FALSE;
}


static void
_gth_grid_view_queue_relayout_from_line (GthGridView *self,
					 int          line)
{
	if (self->priv->relayout_from_line != -1)
		self->priv->relayout_from_line = MIN (line, self->priv->relayout_from_line);
	else
		self->priv->relayout_from_line = line;

	if (! gtk_widget_get_realized (GTK_WIDGET (self))) {
		self->priv->needs_relayout = TRUE;
		return;
	}

	if (self->priv->layout_timeout == 0)
		self->priv->layout_timeout = g_timeout_add (LAYOUT_DELAY,
							    _gth_grid_view_relayout_cb,
							    self);
}


static void
_gth_grid_view_queue_relayout (GthGridView *self)
{
	_gth_grid_view_queue_relayout_from_line (self, 0);
}


static void
_gth_grid_view_queue_relayout_from_position (GthGridView *self,
					     int          pos)
{
	_gth_grid_view_queue_relayout_from_line (self, pos / gth_grid_view_get_items_per_line (self));
}


static void
gth_grid_view_realize (GtkWidget *widget)
{
	GthGridView     *self;
	GtkAllocation    allocation;
	GdkWindowAttr    attributes;
	int              attributes_mask;
	GdkWindow       *window;

	self = GTH_GRID_VIEW (widget);

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	/* view window */

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = allocation.x;
	attributes.y           = allocation.y;
	attributes.width       = allocation.width;
	attributes.height      = allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.event_mask  = GDK_VISIBILITY_NOTIFY_MASK;
	attributes_mask        = (GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL);
	window = gdk_window_new (gtk_widget_get_parent_window (widget),
				 &attributes,
				 attributes_mask);
	gtk_widget_register_window (widget, window);
	gtk_widget_set_window (widget, window);

	/* bin window */

	attributes.x = 0;
	attributes.y = 0;
	attributes.width = MAX (self->priv->width, allocation.width);
	attributes.height = MAX (self->priv->height, allocation.height);
	attributes.event_mask = (GDK_EXPOSURE_MASK
				 | GDK_SCROLL_MASK
				 | GDK_SMOOTH_SCROLL_MASK
				 | GDK_POINTER_MOTION_MASK
				 | GDK_ENTER_NOTIFY_MASK
				 | GDK_LEAVE_NOTIFY_MASK
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK
				 | gtk_widget_get_events (widget));

	self->priv->bin_window = gdk_window_new (gtk_widget_get_window (widget),
						 &attributes,
						 attributes_mask);
	gtk_widget_register_window (widget, self->priv->bin_window);

	/* style */

	self->priv->caption_layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_wrap (self->priv->caption_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment (self->priv->caption_layout, PANGO_ALIGN_LEFT);
	pango_layout_set_spacing (self->priv->caption_layout, CAPTION_LINE_SPACING);

	/**/

	gdk_window_show (self->priv->bin_window);
	self->priv->needs_relayout = TRUE;

	/* this is used to make make_focused_visible work correctly */
	if (self->priv->needs_relayout_after_size_allocate) {
		self->priv->needs_relayout_after_size_allocate = FALSE;
		_gth_grid_view_queue_relayout (self);
	}
}


static void
gth_grid_view_unrealize (GtkWidget *widget)
{
	GthGridView *self;

	self = GTH_GRID_VIEW (widget);

	gtk_widget_unregister_window (widget, self->priv->bin_window);
	gdk_window_destroy (self->priv->bin_window);
	self->priv->bin_window = NULL;

	g_object_unref (self->priv->caption_layout);
	self->priv->caption_layout = NULL;

	gth_icon_cache_free (self->priv->icon_cache);
	self->priv->icon_cache = NULL;

	GTK_WIDGET_CLASS (gth_grid_view_parent_class)->unrealize (widget);
}


static void
gth_grid_view_state_flags_changed (GtkWidget     *widget,
                                   GtkStateFlags  previous_state)
{
	gtk_widget_queue_draw (widget);
}


static void
gth_grid_view_style_updated (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (gth_grid_view_parent_class)->style_updated (widget);

	gtk_widget_queue_resize (widget);
}


static void
gth_grid_view_get_preferred_width (GtkWidget *widget,
                                   int       *minimum,
                                   int       *natural)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	if (minimum != NULL)
		*minimum = self->priv->cell_size;
	if (natural != NULL)
		*natural = *minimum;
}


static void
gth_grid_view_get_preferred_height (GtkWidget *widget,
                                    int       *minimum,
                                    int       *natural)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	if (minimum != NULL)
		*minimum = self->priv->cell_size;
	if (natural != NULL)
		*natural = *minimum;
}


static void
_gth_grid_view_relayout_lines (GthGridView *self)
{
	GList *scan;

	for (scan = self->priv->lines; scan; scan = scan->next) {
		GthGridViewLine *line = scan->data;
		_gth_grid_view_layout_line (self, line);
	}

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


static double
round_to_0_2 (double n)
{
	double i = floor (n);
	double f = n - i;
	double r = 0;

	if (f < 0.1)
		r = 0;
	else if (f < 0.3)
		r = 0.2;
	else if (f < 0.5)
		r = 0.4;
	else if (f < 0.7)
		r = 0.6;
	else if (f < 0.9)
		r = 0.8;
	else
		r = 1.0;
	return i + r;
}


static void
gth_grid_view_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
	GthGridView *self;
	int          old_cells_per_line;
	int          new_cells_per_line;
	double       new_cell_x_spacing;
	gboolean     cell_x_spacing_changed;

	self = GTH_GRID_VIEW (widget);

	old_cells_per_line = gth_grid_view_get_items_per_line (self);
	self->priv->width = allocation->width;

	new_cells_per_line = gth_grid_view_get_items_per_line (self);
	new_cell_x_spacing = round_to_0_2 ((((double) self->priv->width / new_cells_per_line) - self->priv->cell_size) / 2.0);
	cell_x_spacing_changed = (new_cell_x_spacing != self->priv->cell_x_spacing);
	self->priv->cell_x_spacing = new_cell_x_spacing;

	gtk_widget_set_allocation (widget, allocation);

	if (gtk_widget_get_realized (widget)) {
		gdk_window_move_resize (gtk_widget_get_window (widget),
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);
		gdk_window_resize (self->priv->bin_window,
				   MAX (self->priv->width, allocation->width),
				   MAX (self->priv->height, allocation->height));

		if (self->priv->needs_relayout || (old_cells_per_line != new_cells_per_line)) {
			self->priv->make_focused_visible = TRUE;
			_gth_grid_view_queue_relayout (self);
		}
		else if (cell_x_spacing_changed)
			_gth_grid_view_relayout_lines (self);
	}
	else
		self->priv->needs_relayout_after_size_allocate = TRUE;

	_gth_grid_view_configure_hadjustment (self);
	_gth_grid_view_configure_vadjustment (self);
}


static int
get_first_visible_at_offset (GthGridView *self,
			     double       ofs)
{
	int    n_line;
	GList *scan;
	int    pos;

	if ((self->priv->n_items == 0) || (self->priv->lines == NULL))
		return -1;

	n_line = 0;
	for (scan = self->priv->lines; scan && (ofs > 0.0); scan = scan->next) {
		ofs -= GTH_GRID_VIEW_LINE (scan->data)->height + self->priv->cell_spacing;
		n_line++;
	}
	pos = gth_grid_view_get_items_per_line (self) * (n_line - 1);
	return CLAMP (pos, 0, self->priv->n_items - 1);
}


static int
gth_grid_view_get_first_visible (GthFileView *file_view)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	return get_first_visible_at_offset (self, gtk_adjustment_get_value (self->priv->vadjustment));
}


static int
get_last_visible_at_offset (GthGridView *self,
			    double       ofs)
{
	int    n_line;
	GList *scan;
	int    pos;

	if ((self->priv->n_items == 0) || (self->priv->lines == NULL))
		return -1;

	n_line = 0;
	for (scan = self->priv->lines; scan && (ofs > 0.0); scan = scan->next) {
		ofs -= GTH_GRID_VIEW_LINE (scan->data)->height + self->priv->cell_spacing;
		n_line++;
	}
	pos = gth_grid_view_get_items_per_line (self) * n_line - 1;

	return CLAMP (pos, 0, self->priv->n_items - 1);
}


static int
gth_grid_view_get_last_visible (GthFileView *file_view)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	return get_last_visible_at_offset (self,
					   (gtk_adjustment_get_value (self->priv->vadjustment)
					    + gtk_adjustment_get_page_size (self->priv->vadjustment)));
}


/* -- gth_grid_view_draw -- */


static void
_gth_grid_view_item_draw_thumbnail (GthGridViewItem *item,
				    cairo_t         *cr,
				    GtkWidget       *widget,
				    GtkStateFlags    item_state,
				    GthGridView     *grid_view)
{
	cairo_surface_t       *image;
	GtkStyleContext       *style_context;
	cairo_rectangle_int_t  frame_rect;

	image = item->thumbnail;
	if (image == NULL)
		return;

	cairo_surface_reference (image);

	cairo_save (cr);
	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_save (style_context);

#if GTK_CHECK_VERSION(3, 20, 0)
	gtk_style_context_set_state (style_context, item_state);
#else
	gtk_style_context_remove_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_CELL);
#endif

	frame_rect = item->pixbuf_area;

	if ((item->style == ITEM_STYLE_ICON)
            || ! (item->is_image || (item_state & GTK_STATE_FLAG_SELECTED) || (item_state == GTK_STATE_FLAG_NORMAL)))
	{
		/* use a gray rounded box for icons or when the original size
		 * is smaller than the thumbnail size... */

#if GTK_CHECK_VERSION(3, 20, 0)
		gtk_style_context_save (style_context);
		gtk_style_context_add_class (style_context, "icon");
		gtk_render_background (style_context,
				       cr,
				       item->thumbnail_area.x,
				       item->thumbnail_area.y,
				       item->thumbnail_area.width,
				       item->thumbnail_area.height);
		gtk_style_context_restore (style_context);
#else
		GdkRGBA background_color;

		gtk_style_context_get_background_color (style_context, item_state, &background_color);
		gdk_cairo_set_source_rgba (cr, &background_color);

		_cairo_draw_rounded_box (cr,
					 item->thumbnail_area.x,
					 item->thumbnail_area.y,
					 item->thumbnail_area.width,
					 item->thumbnail_area.height,
					 4);
		cairo_fill (cr);
#endif
	}

	if (item->style == ITEM_STYLE_IMAGE) {

		/* ...draw a frame with a drop-shadow effect */

		_cairo_draw_thumbnail_frame (cr,
					     item->thumbnail_area.x,
					     item->thumbnail_area.y,
					     item->thumbnail_area.width,
					     item->thumbnail_area.height);
	}

	if (item->style == ITEM_STYLE_VIDEO) {
		frame_rect = item->thumbnail_area;

		_cairo_draw_film_background (cr,
					     item->thumbnail_area.x,
					     item->thumbnail_area.y,
					     item->thumbnail_area.width,
					     item->thumbnail_area.height);
	}

	/* thumbnail */

	cairo_set_source_surface (cr, image, item->pixbuf_area.x, item->pixbuf_area.y);
	cairo_rectangle (cr, item->pixbuf_area.x, item->pixbuf_area.y, item->pixbuf_area.width, item->pixbuf_area.height);
	cairo_fill (cr);

	if (item->style == ITEM_STYLE_VIDEO) {
		_cairo_draw_film_foreground (cr,
					     item->thumbnail_area.x,
					     item->thumbnail_area.y,
					     item->thumbnail_area.width,
					     item->thumbnail_area.height,
					     grid_view->priv->thumbnail_size);
	}

	if ((item_state & GTK_STATE_FLAG_SELECTED) || (item_state & GTK_STATE_FLAG_FOCUSED)) {
#if GTK_CHECK_VERSION(3, 20, 0)
		gtk_style_context_save (style_context);
		gtk_style_context_add_class (style_context, "icon-effect");
		gtk_render_background (style_context,
				       cr,
				       frame_rect.x,
				       frame_rect.y,
				       frame_rect.width,
				       frame_rect.height);
		gtk_style_context_restore (style_context);
#else
		GdkRGBA color;
		gtk_style_context_get_background_color (style_context, item_state, &color);
		cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.5);
		cairo_rectangle (cr,
				 frame_rect.x,
				 frame_rect.y,
				 frame_rect.width,
				 frame_rect.height);
		cairo_fill_preserve (cr);

		cairo_set_line_width (cr, 2);
		cairo_set_source_rgb (cr, color.red, color.green, color.blue);
		cairo_stroke (cr);
#endif
	}

	gtk_style_context_restore (style_context);
	cairo_restore (cr);

	cairo_surface_destroy (image);
}


static void
_gth_grid_view_item_draw_caption (GthGridViewItem *item,
				  cairo_t         *cr,
				  GtkWidget       *widget,
				  GtkStateFlags    item_state,
				  PangoLayout     *pango_layout,
				  GthGridView     *grid_view)
{
	GtkStyleContext *style_context;
	GdkRGBA          color;

	if (item->caption_area.height == 0)
		return;

	cairo_save (cr);
	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_save (style_context);

	gtk_style_context_get_color (style_context, item_state, &color);
	gdk_cairo_set_source_rgba (cr, &color);
	cairo_move_to (cr, item->caption_area.x, item->caption_area.y + grid_view->priv->caption_padding);
	pango_layout_set_markup (pango_layout, item->caption, -1);
	pango_cairo_show_layout (cr, pango_layout);

	if (item_state & GTK_STATE_FLAG_FOCUSED)
		gtk_render_focus (style_context,
				  cr,
				  item->caption_area.x,
				  item->caption_area.y,
				  item->caption_area.width,
				  item->caption_area.height);

	gtk_style_context_restore (style_context);
	cairo_restore (cr);
}


#define EMBLEM_SIZE 16


static void
_gth_grid_view_item_draw_emblems (GthGridViewItem *item,
				  cairo_t         *cr,
				  GtkWidget       *widget,
				  GtkStateFlags    item_state,
				  GthGridView     *grid_view)
{
	GthStringList *emblems;
	GList         *scan;
	int            emblem_offset;

	cairo_save (cr);

	emblem_offset = 0;
	emblems = (GthStringList *) g_file_info_get_attribute_object (item->file_data->info, GTH_FILE_ATTRIBUTE_EMBLEMS);
	for (scan = gth_string_list_get_list (emblems); scan; scan = scan->next) {
		char            *emblem = scan->data;
		GIcon           *icon;
		cairo_surface_t *image;

		if (grid_view->priv->icon_cache == NULL)
			grid_view->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (grid_view))), EMBLEM_SIZE);

		icon = g_themed_icon_new (emblem);
		image = gth_icon_cache_get_surface (grid_view->priv->icon_cache, icon);
		if (image != NULL) {
			cairo_rectangle (cr,
					 item->thumbnail_area.x + emblem_offset + 1,
					 item->thumbnail_area.y + 1,
					 cairo_image_surface_get_width (image),
					 cairo_image_surface_get_height (image));
			cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
			cairo_fill_preserve (cr);
			cairo_set_source_surface (cr, image, item->thumbnail_area.x + emblem_offset + 1, item->thumbnail_area.y + 1);
			cairo_fill (cr);

			cairo_surface_destroy (image);

			emblem_offset += EMBLEM_SIZE;
		}

		g_object_unref (icon);
	}

	cairo_restore (cr);
}


static void
_gth_grid_view_draw_item (GthGridView     *self,
			  GthGridViewItem *item,
			  cairo_t         *cr)
{
	GtkStateFlags item_state;

	item_state = item->state;
	if (! gtk_widget_has_focus (GTK_WIDGET (self)) && (item_state & GTK_STATE_FLAG_FOCUSED))
		item_state ^= GTK_STATE_FLAG_FOCUSED;
	if (! gtk_widget_has_focus (GTK_WIDGET (self)) && (item_state & GTK_STATE_FLAG_ACTIVE))
		item_state ^= GTK_STATE_FLAG_ACTIVE;

	if (item_state ^ GTK_STATE_FLAG_NORMAL) {
		GtkStyleContext *style_context;

		cairo_save (cr);
		style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
		gtk_style_context_save (style_context);

#if GTK_CHECK_VERSION(3, 20, 0)
		gtk_style_context_set_state (style_context, item_state);

		if (item->style != ITEM_STYLE_ICON) {
			cairo_region_t		 *area;
			cairo_rectangle_int_t	  extents;

			area = cairo_region_create_rectangle (&item->thumbnail_area);
			cairo_region_union_rectangle (area, &item->caption_area);
			cairo_region_get_extents (area, &extents);

			gtk_render_background (style_context,
					       cr,
					       extents.x - self->priv->cell_padding,
					       extents.y - self->priv->cell_padding,
					       extents.width + (self->priv->cell_padding * 2),
					       extents.height + (self->priv->cell_padding * 2));

			cairo_region_destroy (area);
		}
		else
			gtk_render_background (style_context,
					       cr,
					       item->area.x,
					       item->area.y,
					       item->area.width,
					       item->area.height);
#else
		GdkRGBA color;

		gtk_style_context_remove_class (style_context, GTK_STYLE_CLASS_VIEW);
		gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_CELL);
		gtk_style_context_set_state (style_context, item_state);
		gtk_style_context_get_background_color (style_context, item_state, &color);
		_gdk_rgba_lighter (&color, &color);
		cairo_set_source_rgba (cr, color.red, color.green, color.blue, color.alpha);

		if (item->style != ITEM_STYLE_ICON) {
			cairo_region_t		 *area;
			cairo_rectangle_int_t	  extents;

			area = cairo_region_create_rectangle (&item->thumbnail_area);
			cairo_region_union_rectangle (area, &item->caption_area);
			cairo_region_get_extents (area, &extents);

			_cairo_draw_rounded_box (cr,
						 extents.x - self->priv->cell_padding,
						 extents.y - self->priv->cell_padding,
						 extents.width + (self->priv->cell_padding * 2),
						 extents.height + (self->priv->cell_padding * 2),
						 4);

			cairo_region_destroy (area);
		}
		else
			_cairo_draw_rounded_box (cr,
						 item->area.x,
						 item->area.y,
						 item->area.width,
						 item->area.height,
						 4);

		cairo_fill (cr);
#endif

		gtk_style_context_restore (style_context);
		cairo_restore (cr);
	}

	_gth_grid_view_item_draw_thumbnail (item, cr, GTK_WIDGET (self), item_state, self);
	_gth_grid_view_item_draw_caption (item, cr, GTK_WIDGET (self), item_state, self->priv->caption_layout, self);
	_gth_grid_view_item_draw_emblems (item, cr, GTK_WIDGET (self), item_state, self);
}


static void
_gth_grid_view_draw_rubberband (GthGridView *self,
		  	  	cairo_t     *cr)
{
	GtkStyleContext *style_context;

	if ((self->priv->selection_area.width <= 1.0) || (self->priv->selection_area.height <= 1.0))
		return;

	cairo_save (cr);

	style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
	gtk_style_context_save (style_context);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_RUBBERBAND);

	gdk_cairo_rectangle (cr, &self->priv->selection_area);
	cairo_clip (cr);
	gtk_render_background (style_context,
			       cr,
			       self->priv->selection_area.x,
			       self->priv->selection_area.y,
			       self->priv->selection_area.width,
			       self->priv->selection_area.height);
	gtk_render_frame (style_context,
			  cr,
			  self->priv->selection_area.x,
			  self->priv->selection_area.y,
			  self->priv->selection_area.width,
			  self->priv->selection_area.height);

	gtk_style_context_restore (style_context);
	cairo_restore (cr);
}


static void
_gth_grid_view_draw_drop_target (GthGridView *self,
				 cairo_t     *cr)
{
	GtkStyleContext *style_context;
	GthGridViewItem *item;
	int              x;

	if ((self->priv->drop_item < 0) || (self->priv->drop_item >= self->priv->n_items))
		return;

	style_context = gtk_widget_get_style_context (GTK_WIDGET (self));

	item = g_ptr_array_index (self->priv->itemv, self->priv->drop_item);

	x = 0;
	if (self->priv->drop_pos == GTH_DROP_POSITION_LEFT)
		x = item->area.x - (self->priv->cell_x_spacing / 2);
	else if (self->priv->drop_pos == GTH_DROP_POSITION_RIGHT)
		x = item->area.x + self->priv->cell_size + (self->priv->cell_x_spacing / 2);

	gtk_render_focus (style_context,
			   cr,
			   x - 1,
			   item->area.y + self->priv->cell_padding,
			   2,
			   item->area.height - (self->priv->cell_padding * 2));
}


static void
_gth_grid_draw_background (GthGridView *self,
		 	   cairo_t     *cr)
{
	GtkAllocation allocation;

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	gtk_render_background (gtk_widget_get_style_context (GTK_WIDGET (self)),
			       cr,
			       0,
			       gtk_adjustment_get_value (self->priv->vadjustment),
			       allocation.width,
			       allocation.height);
}


static gboolean
gth_grid_view_draw (GtkWidget *widget,
		    cairo_t   *cr)
{
	GthGridView *self = (GthGridView*) widget;
	int          first_visible;

	if (! gtk_cairo_should_draw_window (cr, self->priv->bin_window))
		return FALSE;

	cairo_save (cr);
	gtk_cairo_transform_to_window (cr, widget, self->priv->bin_window);
	cairo_set_line_width (cr, 1.0);

	_gth_grid_draw_background (self, cr);

	first_visible = gth_grid_view_get_first_visible (GTH_FILE_VIEW (self));
	if (first_visible >= 0) {
		int last_visible;
		int i;

		last_visible = gth_grid_view_get_last_visible (GTH_FILE_VIEW (self));

		for (i = first_visible; i <= last_visible; i++) {
			GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);
			_gth_grid_view_draw_item (self, item, cr);
		}

		if (self->priv->selecting || self->priv->multi_selecting_with_keyboard)
			_gth_grid_view_draw_rubberband (self, cr);

		if (self->priv->drop_pos != GTH_DROP_POSITION_NONE)
			_gth_grid_view_draw_drop_target (self, cr);
	}

	cairo_restore (cr);

	return TRUE;
}


static gboolean
gth_grid_view_key_press (GtkWidget   *widget,
			  GdkEventKey *event)
{
	GthGridView *self = GTH_GRID_VIEW (widget);
	gboolean     handled;

	if (! self->priv->multi_selecting_with_keyboard
	    && (event->state & GDK_SHIFT_MASK)
	    && ((event->keyval == GDK_KEY_Left)
		|| (event->keyval == GDK_KEY_Right)
		|| (event->keyval == GDK_KEY_Up)
		|| (event->keyval == GDK_KEY_Down)
		|| (event->keyval == GDK_KEY_Page_Up)
		|| (event->keyval == GDK_KEY_Page_Down)
		|| (event->keyval == GDK_KEY_Home)
		|| (event->keyval == GDK_KEY_End)))
	{
		self->priv->multi_selecting_with_keyboard = TRUE;
		self->priv->first_focused_item = self->priv->focused_item;

		self->priv->selection_area.x = 0;
		self->priv->selection_area.y = 0;
		self->priv->selection_area.width = 0;
		self->priv->selection_area.height = 0;
	}

	handled = gtk_bindings_activate (G_OBJECT (widget),
					 event->keyval,
					 event->state);

	if (handled)
		return TRUE;

	if ((GTK_WIDGET_CLASS (gth_grid_view_parent_class)->key_press_event != NULL)
	    && GTK_WIDGET_CLASS (gth_grid_view_parent_class)->key_press_event (widget, event))
	{
		return TRUE;
	}

	return FALSE;
}


static gboolean
gth_grid_view_key_release (GtkWidget   *widget,
			   GdkEventKey *event)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	if (self->priv->multi_selecting_with_keyboard
	    && (event->state & GDK_SHIFT_MASK)
	    && ((event->keyval == GDK_KEY_Shift_L)
		|| (event->keyval == GDK_KEY_Shift_R)))
	{
		self->priv->multi_selecting_with_keyboard = FALSE;
	}

	gtk_widget_queue_draw (widget);

	if ((GTK_WIDGET_CLASS (gth_grid_view_parent_class)->key_press_event != NULL)
	    && GTK_WIDGET_CLASS (gth_grid_view_parent_class)->key_press_event (widget, event))
	{
		return TRUE;
	}

	return FALSE;
}


/* -- GthFileSelection interface -- */


static void
gth_grid_view_set_selection_mode (GthFileSelection *selection,
				  GtkSelectionMode  mode)
{
	GthGridView *self = GTH_GRID_VIEW (selection);

	self->priv->selection_mode = mode;

	/* the cell padding is used to show the selection, set it to 0 if
	 * selection is not allowed. */
	self->priv->cell_padding = (mode == GTK_SELECTION_NONE) ? 0 : DEFAULT_CELL_PADDING;
}


static GList *
gth_grid_view_get_selected (GthFileSelection *selection)
{
	GthGridView *self = GTH_GRID_VIEW (selection);
	GList       *selected;
	GList       *scan;

	selected = NULL;
	for (scan = self->priv->selection; scan; scan = scan->next) {
		int pos;

		pos = GPOINTER_TO_INT (scan->data);
		selected = g_list_prepend (selected, gtk_tree_path_new_from_indices (pos, -1));
	}

	return g_list_reverse (selected);
}


static void
_gth_grid_view_queue_draw_item (GthGridView     *self,
				GthGridViewItem *item)
{
	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		gdk_window_invalidate_rect (self->priv->bin_window, &item->area, FALSE);
}


static void
_gth_grid_view_select_item (GthGridView *self,
			    int          pos)
{
	GthGridViewItem *item;

	g_return_if_fail ((pos >= 0) && (pos < self->priv->n_items));

	if (self->priv->selection_mode == GTK_SELECTION_NONE)
		return;

	item = g_ptr_array_index (self->priv->itemv, pos);
	if (item->state & GTK_STATE_FLAG_SELECTED)
		return;

	item->state |= GTK_STATE_FLAG_SELECTED;
	self->priv->selection = g_list_prepend (self->priv->selection, GINT_TO_POINTER (pos));
	self->priv->selection_changed = TRUE;

	_gth_grid_view_queue_draw_item (self, item);
}


static void
_gth_grid_view_unselect_item (GthGridView *self,
		     	      int          pos)
{
	GthGridViewItem *item;

	g_return_if_fail ((pos >= 0) && (pos < self->priv->n_items));

	if (self->priv->selection_mode == GTK_SELECTION_NONE)
		return;

	item = g_ptr_array_index (self->priv->itemv, pos);
	if (! (item->state & GTK_STATE_FLAG_SELECTED))
		return;

	item->state ^= GTK_STATE_FLAG_SELECTED;
	self->priv->selection = g_list_remove (self->priv->selection, GINT_TO_POINTER (pos));
	self->priv->selection_changed = TRUE;

	_gth_grid_view_queue_draw_item (self, item);
}


static void
_gth_grid_view_set_item_selected (GthGridView *self,
				  gboolean     selected,
				  int          pos)
{
	if (selected)
		_gth_grid_view_select_item (self, pos);
	else
		_gth_grid_view_unselect_item (self, pos);
}


static void
_gth_grid_view_emit_selection_changed (GthGridView *self)
{
	if (self->priv->selection_changed && (self->priv->selection_mode != GTK_SELECTION_NONE)) {
		gth_file_selection_changed (GTH_FILE_SELECTION (self));
		self->priv->selection_changed = FALSE;
	}
}


static void
_gth_grid_view_set_item_selected_and_emit_signal (GthGridView *self,
						  gboolean     select,
						  int          pos)
{
	_gth_grid_view_set_item_selected (self, select, pos);
	_gth_grid_view_emit_selection_changed (self);
}


static int
_gth_grid_view_unselect_all (GthGridView *self,
			     gpointer     keep_selected)
{
	int idx;
	int i;

	idx = 0;
	for (i = 0; i < self->priv->n_items; i++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);

		if (item == keep_selected)
			idx = i;
		else if (item->state & GTK_STATE_FLAG_SELECTED)
			_gth_grid_view_set_item_selected (self, FALSE, i);
	}

	return idx;
}


static void
gth_grid_view_select (GthFileSelection *selection,
		      int               pos)
{
	GthGridView *self = GTH_GRID_VIEW (selection);
	int          i;

	switch (self->priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
		for (i = 0; i < self->priv->n_items; i++) {
			GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);

			if ((i != pos) && (item->state & GTK_STATE_FLAG_SELECTED))
				_gth_grid_view_set_item_selected (self, FALSE, i);
		}
		_gth_grid_view_set_item_selected (self, TRUE, pos);
		_gth_grid_view_emit_selection_changed (self);
		break;

	case GTK_SELECTION_MULTIPLE:
		self->priv->select_pending = FALSE;

		_gth_grid_view_set_item_selected_and_emit_signal (self, TRUE, pos);
		self->priv->last_selected_pos = pos;
		break;

	default:
		break;
	}
}


static void
gth_grid_view_unselect (GthFileSelection *selection,
		        int               pos)
{
	_gth_grid_view_set_item_selected_and_emit_signal (GTH_GRID_VIEW (selection), FALSE, pos);
}


static void
_gth_grid_view_select_all (GthGridView *self)
{
	int i;

	for (i = 0; i < self->priv->n_items; i++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);

		if (! (item->state & GTK_STATE_FLAG_SELECTED))
			_gth_grid_view_set_item_selected (self, TRUE, i);
	}
}


static void
gth_grid_view_select_all (GthFileSelection *selection)
{
	GthGridView *self = GTH_GRID_VIEW (selection);

	_gth_grid_view_select_all (self);
	_gth_grid_view_emit_selection_changed (self);
}


static void
gth_grid_view_unselect_all (GthFileSelection *selection)
{
	GthGridView *self = GTH_GRID_VIEW (selection);

	_gth_grid_view_unselect_all (self, NULL);
	_gth_grid_view_emit_selection_changed (self);
}


static gboolean
gth_grid_view_is_selected (GthFileSelection *selection,
			   int               pos)
{
	GthGridView *self = GTH_GRID_VIEW (selection);
	GList       *scan;

	for (scan = self->priv->selection; scan; scan = scan->next)
		if (GPOINTER_TO_INT (scan->data) == pos)
			return TRUE;

	return FALSE;
}


static GtkTreePath *
gth_grid_view_get_first_selected (GthFileSelection *selection)
{
	GthGridView *self = GTH_GRID_VIEW (selection);
	GList       *scan;
	int          pos;

	scan = self->priv->selection;
	if (scan == NULL)
		return NULL;

	pos = GPOINTER_TO_INT (scan->data);
	for (scan = scan->next; scan; scan = scan->next)
		pos = MIN (pos, GPOINTER_TO_INT (scan->data));

	return gtk_tree_path_new_from_indices (pos, -1);
}


static GtkTreePath *
gth_grid_view_get_last_selected (GthFileSelection *selection)
{
	GthGridView *self = GTH_GRID_VIEW (selection);
	GList       *scan;
	int          pos;

	scan = self->priv->selection;
	if (scan == NULL)
		return NULL;

	pos = GPOINTER_TO_INT (scan->data);
	for (scan = scan->next; scan; scan = scan->next)
		pos = MAX (pos, GPOINTER_TO_INT (scan->data));

	return gtk_tree_path_new_from_indices (pos, -1);
}


static guint
gth_grid_view_get_n_selected (GthFileSelection *selection)
{
	return g_list_length (GTH_GRID_VIEW (selection)->priv->selection);
}


/* -- GthFileView interface -- */


static void
model_row_changed_cb (GtkTreeModel *tree_model,
		      GtkTreePath  *path,
		      GtkTreeIter  *iter,
		      gpointer      user_data)
{
	GthGridView     *self = user_data;
	int              pos;
	GthFileData     *file_data;
	cairo_surface_t *thumbnail;
	gboolean         is_icon;
	GthGridViewItem *item;

	gtk_tree_model_get (tree_model,
			    iter,
			    GTH_FILE_STORE_FILE_DATA_COLUMN, &file_data,
			    GTH_FILE_STORE_THUMBNAIL_COLUMN, &thumbnail,
			    GTH_FILE_STORE_IS_ICON_COLUMN, &is_icon,
			    -1);

	pos = gtk_tree_path_get_indices (path)[0];
	item = g_ptr_array_index (self->priv->itemv, pos);
	gth_grid_view_item_set_file_data (item, file_data);
	gth_grid_view_item_set_thumbnail (item, thumbnail);
	item->is_icon = is_icon;
	gth_grid_view_item_update_caption (item, self->priv->caption_attributes_v);

	_gth_grid_view_queue_relayout_from_position (self, pos);

	g_object_unref (file_data);
	cairo_surface_destroy (thumbnail);
}


static void
model_row_deleted_cb (GtkTreeModel *tree_model,
		      GtkTreePath  *path,
		      gpointer      user_data)
{
	GthGridView *self = user_data;
	int          pos;
	GList       *scan;
	GList       *selected_link;

	pos = gtk_tree_path_get_indices (path)[0];
	g_ptr_array_remove_index (self->priv->itemv, pos);
	self->priv->n_items = (int) self->priv->itemv->len;

	/* update the selection */

	selected_link = NULL;
	for (scan = self->priv->selection; scan; scan = scan->next) {
		int selected_pos = GPOINTER_TO_INT (scan->data);
		if (selected_pos > pos)
			scan->data = GINT_TO_POINTER (selected_pos - 1);
		else if (selected_pos == pos)
			selected_link = scan;
	}
	if (selected_link != NULL) {
		self->priv->selection = g_list_remove_link (self->priv->selection, selected_link);
		g_list_free (selected_link);
	}

	/* update the cursor position */

	if (pos < self->priv->focused_item)
		self->priv->focused_item--;
	else if (pos == self->priv->focused_item)
		self->priv->focused_item = -1;

	/* update the last selected position */

	if (pos < self->priv->last_selected_pos)
		self->priv->last_selected_pos--;
	else if (pos == self->priv->last_selected_pos)
		self->priv->last_selected_pos = -1;

	/* relayout from the minimum changed position */

	_gth_grid_view_queue_relayout_from_position (self, pos);
}


static void
model_row_inserted_cb (GtkTreeModel *tree_model,
		       GtkTreePath  *path,
		       GtkTreeIter  *iter,
		       gpointer      user_data)
{
	GthGridView     *self = user_data;
	GthFileData     *file_data;
	cairo_surface_t *thumbnail;
	gboolean         is_icon;
	GthGridViewItem *item;
	int              pos;
	GList           *scan;

	gtk_tree_model_get (tree_model,
			    iter,
			    GTH_FILE_STORE_FILE_DATA_COLUMN, &file_data,
			    GTH_FILE_STORE_THUMBNAIL_COLUMN, &thumbnail,
			    GTH_FILE_STORE_IS_ICON_COLUMN, &is_icon,
			    -1);
	item = gth_grid_view_item_new (self,
				       file_data,
				       thumbnail,
				       is_icon,
				       self->priv->caption_attributes_v);
	pos = gtk_tree_path_get_indices (path)[0];
	g_ptr_array_insert (self->priv->itemv, pos, item);
	self->priv->n_items = (int) self->priv->itemv->len;

	/* update the selection */

	for (scan = self->priv->selection; scan; scan = scan->next) {
		int selected_pos = GPOINTER_TO_INT (scan->data);
		if (selected_pos >= pos)
			scan->data = GINT_TO_POINTER (selected_pos + 1);
	}

	/* update the cursor position */

	if (pos <= self->priv->focused_item)
		self->priv->focused_item++;

	/* update the last selected position */

	if (pos <= self->priv->last_selected_pos)
		self->priv->last_selected_pos++;

	/* relayout from the minimum changed position */

	_gth_grid_view_queue_relayout_from_position (self, pos);

	g_object_unref (file_data);
	cairo_surface_destroy (thumbnail);
}


static void
model_rows_reordered_cb (GtkTreeModel *tree_model,
			 GtkTreePath  *path,
			 GtkTreeIter  *iter,
			 gpointer      new_order,
			 gpointer      user_data)
{
	GthGridView *self = user_data;
	GPtrArray   *itemv;
	int          i;
	int          min_changed_pos;
	gboolean     focused_updated = FALSE;
	GList       *scan;

	/* change the order of the items list */

	min_changed_pos = -1;
	itemv = g_ptr_array_new_full (self->priv->n_items, (GDestroyNotify) gth_grid_view_item_unref);
	for (i = 0; i < self->priv->n_items; i++) {
		int              old_pos;
		GthGridViewItem *item;

		old_pos = ((int *) new_order)[i];
		if ((min_changed_pos == -1) && (old_pos != i))
			min_changed_pos = i;

		/* update the cursor position */

		if (! focused_updated && (old_pos == self->priv->focused_item)) {
			self->priv->focused_item = i;
			focused_updated = TRUE;
		}

		item = g_ptr_array_index (self->priv->itemv, old_pos);
		g_ptr_array_add (itemv, gth_grid_view_item_ref (item));
	}

	g_ptr_array_unref (self->priv->itemv);
	self->priv->itemv = itemv;
	self->priv->n_items = (int) self->priv->itemv->len;

	/* update the selection */

	for (scan = self->priv->selection; scan; scan = scan->next) {
		int selected_pos = GPOINTER_TO_INT (scan->data);

		for (i = 0; i < self->priv->n_items; i++) {
			int old_pos = ((int *) new_order)[i];

			if (selected_pos == old_pos) {
				scan->data = GINT_TO_POINTER (i);
				break;
			}
		}
	}

	/* relayout from the minimum changed position */

	if (min_changed_pos >= 0)
		_gth_grid_view_queue_relayout_from_position (self, min_changed_pos);
}


static void
model_thumbnail_changed_cb (GtkTreeModel *tree_model,
		      	    GtkTreePath  *path,
		      	    GtkTreeIter  *iter,
		      	    gpointer      user_data)
{
	GthGridView     *self = user_data;
	int              pos;
	cairo_surface_t *thumbnail;
	gboolean         is_icon;
	GthGridViewItem *item;

	gtk_tree_model_get (tree_model,
			    iter,
			    GTH_FILE_STORE_THUMBNAIL_COLUMN, &thumbnail,
			    GTH_FILE_STORE_IS_ICON_COLUMN, &is_icon,
			    -1);

	pos = gtk_tree_path_get_indices (path)[0];
	item = g_ptr_array_index (self->priv->itemv, pos);
	gth_grid_view_item_set_thumbnail (item, thumbnail);
	item->is_icon = is_icon;

	_gth_grid_view_update_item_size (self, item);
	_gth_grid_view_place_item_at (self, item, item->area.x, item->area.y);
	_gth_grid_view_queue_draw_item (self, item);

	cairo_surface_destroy (thumbnail);
}


static void
gth_grid_view_set_model (GthFileView  *file_view,
			 GtkTreeModel *model)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	if (model != NULL)
		g_object_ref (model);
	if (self->priv->model != NULL) {
		_g_signal_handlers_disconnect_by_data (self->priv->model, self);
		g_object_unref (self->priv->model);
	}
	self->priv->model = model;
	g_object_notify (G_OBJECT (self), "model");

	if (self->priv->model == NULL)
		return;

	g_signal_connect (self->priv->model,
			  "row-changed",
			  G_CALLBACK (model_row_changed_cb),
			  self);
	g_signal_connect (self->priv->model,
			  "row-deleted",
			  G_CALLBACK (model_row_deleted_cb),
			  self);
	g_signal_connect (self->priv->model,
			  "row-inserted",
			  G_CALLBACK (model_row_inserted_cb),
			  self);
	g_signal_connect (self->priv->model,
			  "rows-reordered",
			  G_CALLBACK (model_rows_reordered_cb),
			  self);
	g_signal_connect (self->priv->model,
			  "thumbnail-changed",
			  G_CALLBACK (model_thumbnail_changed_cb),
			  self);
}


static int
_gth_grid_view_get_at_position (GthGridView      *self,
			        int               x,
			        int               y,
			        GthGridViewItem **item_ref)
{
	int n;

	for (n = 0; n < self->priv->n_items; n++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, n);

		if (_cairo_rectangle_contains_point (&item->thumbnail_area, x, y)
		    || _cairo_rectangle_contains_point (&item->caption_area, x, y))
		{
			if (item_ref != NULL)
				*item_ref = item;
			return n;
		}
	}

	if (item_ref != NULL)
		*item_ref = NULL;

	return -1;
}


static int
gth_grid_view_get_at_position (GthFileView     *file_view,
			       int               x,
			       int               y)
{
	return _gth_grid_view_get_at_position (GTH_GRID_VIEW (file_view), x, y, NULL);
}


static void
gth_grid_view_cursor_changed (GthFileView *file_view,
			      int          pos)
{
	GthGridView     *self = GTH_GRID_VIEW (file_view);
	GthGridViewItem *new_item;

	if ((pos != self->priv->focused_item) && (self->priv->focused_item >= 0)) {
		GthGridViewItem *old_item = NULL;

		old_item = g_ptr_array_index (self->priv->itemv, self->priv->focused_item);
		if (old_item != NULL) {
			old_item->state ^= GTK_STATE_FLAG_FOCUSED | GTK_STATE_FLAG_ACTIVE;
			_gth_grid_view_queue_draw_item (self, old_item);
		}
	}

	self->priv->focused_item = pos;
	new_item = g_ptr_array_index (self->priv->itemv, pos);
	new_item->state |= GTK_STATE_FLAG_FOCUSED | GTK_STATE_FLAG_ACTIVE;
	_gth_grid_view_queue_draw_item (self, new_item);

	self->priv->make_focused_visible = TRUE;
	_gth_grid_view_make_item_fully_visible (self, self->priv->focused_item);
}


static int
gth_grid_view_get_cursor (GthFileView *file_view)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	if (! gtk_widget_has_focus (GTK_WIDGET (self)))
		return -1;
	else
		return self->priv->focused_item;
}


static void
gth_grid_view_enable_drag_source (GthFileView          *file_view,
				  GdkModifierType       start_button_mask,
				  const GtkTargetEntry *targets,
				  int                   n_targets,
				  GdkDragAction         actions)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	if (self->priv->drag_target_list != NULL)
		gtk_target_list_unref (self->priv->drag_target_list);

	self->priv->drag_source_enabled = TRUE;
	self->priv->drag_start_button_mask = start_button_mask;
	self->priv->drag_target_list = gtk_target_list_new (targets, n_targets);
	self->priv->drag_actions = actions;
}


static void
gth_grid_view_unset_drag_source (GthFileView *file_view)
{
	GTH_GRID_VIEW (file_view)->priv->drag_source_enabled = FALSE;
}


static void
gth_grid_view_enable_drag_dest (GthFileView          *file_view,
				const GtkTargetEntry *targets,
				int                   n_targets,
				GdkDragAction         actions)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	gtk_drag_dest_set (GTK_WIDGET (self),
			   0,
			   targets,
			   n_targets,
			   actions);
}


static void
gth_grid_view_unset_drag_dest (GthFileView *file_view)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	gtk_drag_dest_unset (GTK_WIDGET (self));
}


static int
_gth_grid_view_get_drop_target_at (GthGridView *self,
				   int          x,
				   int          y)
{
	int    row;
	int    height;
	GList *scan;
	int    items_per_line;
	int    col;

	x += gtk_adjustment_get_value (self->priv->hadjustment);
	y += gtk_adjustment_get_value (self->priv->vadjustment);

	row = -1;
	height = self->priv->cell_spacing;
	for (scan = self->priv->lines; scan && (height < y); scan = scan->next) {
		height += GTH_GRID_VIEW_LINE (scan->data)->height + self->priv->cell_spacing;
		row++;
	}
	if (height < y)
		row++;
	row = MAX (row, 0);

	items_per_line = gth_grid_view_get_items_per_line (self);
	col = (x - (self->priv->cell_x_spacing / 2)) / (self->priv->cell_size + self->priv->cell_x_spacing) + 1;
	col = MIN (col, items_per_line);

	return (items_per_line * row) + col - 1;
}


static void
gth_grid_view_set_drag_dest_pos (GthFileView    *file_view,
				 GdkDragContext *context,
				 int             x,
				 int             y,
				 guint           time,
				 int            *pos)
{
	GthGridView     *self = GTH_GRID_VIEW (file_view);
	GthDropPosition  drop_pos;
	int              drop_image;

	g_return_if_fail (GTH_IS_GRID_VIEW (self));

	drop_pos = self->priv->drop_pos;
	drop_image = self->priv->drop_item;

	if ((x < 0) && (y < 0) && (drop_pos != GTH_DROP_POSITION_NONE)) {
		if (drop_pos == GTH_DROP_POSITION_RIGHT)
			drop_image++;
		drop_pos = GTH_DROP_POSITION_NONE;
	}
	else {
		drop_image = _gth_grid_view_get_drop_target_at (self, x, y);

		if (drop_image < 0) {
			drop_image = 0;
			drop_pos = GTH_DROP_POSITION_LEFT;
		}
		else if (drop_image >= self->priv->n_items) {
			drop_image = self->priv->n_items - 1;
			drop_pos = GTH_DROP_POSITION_RIGHT;
		}
		else {
			GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, drop_image);
			if (x - item->area.x > self->priv->cell_size / 2)
				drop_pos = GTH_DROP_POSITION_RIGHT;
			else
				drop_pos = GTH_DROP_POSITION_LEFT;
		}
	}

	if (pos != NULL) {
		*pos = drop_image;
		if (gtk_widget_get_direction(GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
			if (drop_pos == GTH_DROP_POSITION_LEFT)
				*pos = *pos + 1;
		}
		else {
			if (drop_pos == GTH_DROP_POSITION_RIGHT)
				*pos = *pos + 1;
		}
	}

	if ((drop_pos != self->priv->drop_pos) || (drop_image != self->priv->drop_item)) {
		self->priv->drop_pos = drop_pos;
		self->priv->drop_item = drop_image;
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}


static void
gth_grid_view_get_drag_dest_pos (GthFileView *file_view,
				 int         *pos)
{
	GthGridView *self = GTH_GRID_VIEW (file_view);

	g_return_if_fail (pos != NULL);
	*pos = self->priv->drop_item;
}


/* -- GtkScrollable interface -- */


static gboolean
gth_grid_view_get_border (GtkScrollable *scrollable,
			  GtkBorder     *border)
{
	return FALSE;
}


/* GtkWidget methods */


static void
_gth_grid_view_select_range (GthGridView     *self,
			     GthGridViewItem *item,
			     int              pos)
{
	int a, b;

	if (self->priv->last_selected_pos == -1)
		self->priv->last_selected_pos = pos;

	if (pos < self->priv->last_selected_pos) {
		a = pos;
		b = self->priv->last_selected_pos;
	}
	else {
		a = self->priv->last_selected_pos;
		b = pos;
	}

	for (/* void */; a <= b; a++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, a);

		if (! (item->state & GTK_STATE_FLAG_SELECTED))
			_gth_grid_view_set_item_selected (self, TRUE, a);
	}

	_gth_grid_view_set_item_selected (self, TRUE, pos);
	_gth_grid_view_emit_selection_changed (self);

	gth_file_view_set_cursor (GTH_FILE_VIEW (self), pos);
}


static void
_gth_grid_view_select_single (GthGridView     *self,
	        	      GthGridViewItem *item,
	        	      int              pos,
	        	      GdkEventButton  *event)
{
	if (item->state & GTK_STATE_FLAG_SELECTED) {
		/* postpone selection to handle dragging. */
		self->priv->select_pending = TRUE;
		self->priv->select_pending_pos = pos;
		self->priv->select_pending_item = item;
	}
	else {
		_gth_grid_view_unselect_all (self, NULL);
		_gth_grid_view_set_item_selected_and_emit_signal (self, TRUE, pos);
		self->priv->last_selected_pos = pos;

		if (self->priv->activate_on_single_click && ((event->state & GDK_MOD1_MASK) == 0))
			self->priv->activate_pending = TRUE;
	}

	gth_file_view_set_cursor (GTH_FILE_VIEW (self), pos);
}


static void
_gth_grid_view_select_multiple (GthGridView     *self,
			        GthGridViewItem *item,
			        int              pos,
			        GdkEventButton  *event)
{
	gboolean range;
	gboolean additive;

	range    = (event->state & GDK_SHIFT_MASK) != 0;
	additive = (event->state & GDK_CONTROL_MASK) != 0;

	if (! additive && ! range) {
		_gth_grid_view_select_single (self, item, pos, event);
		return;
	}

	if (range) {
		_gth_grid_view_unselect_all (self, item);
		_gth_grid_view_select_range (self, item, pos);
	}
	else if (additive) {
		_gth_grid_view_set_item_selected_and_emit_signal (self, ! (item->state & GTK_STATE_FLAG_SELECTED), pos);
		self->priv->last_selected_pos = pos;
	}

	gth_file_view_set_cursor (GTH_FILE_VIEW (self), pos);
}


static void
gth_grid_view_store_items_state (GthGridView *self)
{
	int i;

	for (i = 0; i < self->priv->n_items; i++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);
		item->tmp_state = item->state;
	}
}


static int
gth_grid_view_button_press (GtkWidget      *widget,
			    GdkEventButton *event)
{
	GthGridView *    self = GTH_GRID_VIEW (widget);
	int              retval = FALSE;
	int              pos;
	GthGridViewItem *item;

	if (event->window != self->priv->bin_window)
		return FALSE;

	if (! gtk_widget_has_focus (widget))
		gtk_widget_grab_focus (widget);

	pos = _gth_grid_view_get_at_position (self, event->x, event->y, &item);

	if ((pos != -1) && (event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		/* Double click activates the item */

		if (! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && ! (event->state & GDK_MOD1_MASK))
		{
			gth_file_view_activated (GTH_FILE_VIEW (self), pos);
		}
		retval = TRUE;
	}
	else if ((pos != -1) && (event->button == 2) && (event->type == GDK_BUTTON_PRESS)) {
		/* This can be the start of a dragging action. */

		if (! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && ! (event->state & GDK_MOD1_MASK)
		    && self->priv->drag_source_enabled)
		{
			self->priv->dragging = TRUE;
			self->priv->drag_button = 2;
			self->priv->drag_start_x = event->x;
			self->priv->drag_start_y = event->y;
		}
		retval = TRUE;
	}
	else if ((pos != -1) && (event->button == 1) && (event->type == GDK_BUTTON_PRESS)) {
		/* This can be the start of a dragging action. */

		if (! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && ! (event->state & GDK_MOD1_MASK)
		    && self->priv->drag_source_enabled)
		{
			self->priv->dragging = TRUE;
			self->priv->drag_button = 1;
			self->priv->drag_start_x = event->x;
			self->priv->drag_start_y = event->y;
		}

		if (self->priv->selection_mode != GTK_SELECTION_NONE) {
			if (self->priv->selection_mode == GTK_SELECTION_MULTIPLE)
				_gth_grid_view_select_multiple (self, item, pos, event);
			else
				_gth_grid_view_select_single (self, item, pos, event);
		}
		else
			gth_file_view_set_cursor (GTH_FILE_VIEW (self), pos);

		retval = TRUE;
	}
	else if ((pos == -1) && (event->button == 1) && (event->type == GDK_BUTTON_PRESS) && (self->priv->selection_mode == GTK_SELECTION_MULTIPLE)) {
		/* This can be the start of a selection */

		if ((event->state & GDK_CONTROL_MASK) == 0)
			_gth_grid_view_unselect_all (self, NULL);

		if (! self->priv->selecting) {
			gth_grid_view_store_items_state (self);

			self->priv->sel_start_x = event->x;
			self->priv->sel_start_y = event->y;
			self->priv->selection_area.x = event->x;
			self->priv->selection_area.y = event->y;
			self->priv->selection_area.width = 0;
			self->priv->selection_area.height = 0;
			self->priv->sel_state = event->state;
			self->priv->selecting = TRUE;
		}
		retval = TRUE;
	}

	return retval;
}


static gboolean
gth_grid_view_item_is_inside_area (GthGridViewItem *item,
				   int              x1,
				   int              y1,
				   int              x2,
				   int              y2)
{
	GdkRectangle area;
	GdkRectangle item_area;
	double       x_ofs;
	double       y_ofs;

	if ((x1 == x2) && (y1 == y2))
		return FALSE;

	area.x = x1;
	area.y = y1;
	area.width = x2 - x1;
	area.height = y2 - y1;

	item_area = item->thumbnail_area;
	x_ofs = item_area.width / 6;
	y_ofs = item_area.height / 6;
	item_area.x      += x_ofs;
	item_area.y      += y_ofs;
	item_area.width  -= x_ofs * 2;
	item_area.height -= y_ofs * 2;

	return gdk_rectangle_intersect (&item_area, &area, NULL);
}


static void
_gth_grid_view_update_mouse_selection (GthGridView *self,
				       int           x,
				       int           y)
{
	cairo_rectangle_int_t  old_selection_area;
	cairo_region_t        *invalid_region;
	int                    x1, y1, x2, y2;
	GtkAllocation          allocation;
	cairo_region_t        *common_region;
	gboolean               additive;
	gboolean               invert;
	int                    min_y, max_y;
	int                    i, begin_idx, end_idx;

	old_selection_area = self->priv->selection_area;

	/* calculate the new selection area */

	if (self->priv->sel_start_x < x) {
		x1 = self->priv->sel_start_x;
		x2 = x;
	}
	else {
		x1 = x;
		x2 = self->priv->sel_start_x;
	}

	if (self->priv->sel_start_y < y) {
		y1 = self->priv->sel_start_y;
		y2 = y;
	}
	else {
		y1 = y;
		y2 = self->priv->sel_start_y;
	}

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	allocation.width = MAX (allocation.width, self->priv->width);
	allocation.height = MAX (allocation.height, self->priv->height);
	x1 = CLAMP (x1, 0, allocation.width);
	y1 = CLAMP (y1, 0, allocation.height);
	x2 = CLAMP (x2, 0, allocation.width);
	y2 = CLAMP (y2, 0, allocation.height);

	self->priv->selection_area.x = x1;
	self->priv->selection_area.y = y1;
	self->priv->selection_area.width = x2 - x1;
	self->priv->selection_area.height = y2 - y1;

	/* repaint the changed area */

	invalid_region = cairo_region_create_rectangle (&old_selection_area);
	cairo_region_union_rectangle (invalid_region, &self->priv->selection_area);

	common_region = cairo_region_create_rectangle (&old_selection_area);
	cairo_region_intersect_rectangle (common_region, &self->priv->selection_area);

	if (! cairo_region_is_empty (common_region)) {
		cairo_rectangle_int_t common_region_extents;

		/* invalidate the border as well */
		cairo_region_get_extents (common_region, &common_region_extents);
		common_region_extents.x += RUBBERBAND_BORDER;
		common_region_extents.y += RUBBERBAND_BORDER;
		common_region_extents.width -= RUBBERBAND_BORDER * 2;
		common_region_extents.height -= RUBBERBAND_BORDER * 2;

		cairo_region_subtract_rectangle (invalid_region, &common_region_extents);
	}

	gdk_window_invalidate_region (self->priv->bin_window, invalid_region, FALSE);

	cairo_region_destroy (common_region);
	cairo_region_destroy (invalid_region);

	/* select or unselect images as appropriate */

	additive = self->priv->sel_state & GDK_SHIFT_MASK;
	invert   = self->priv->sel_state & GDK_CONTROL_MASK;

	/* Consider only images in the min_y --> max_y offset. */

	min_y = self->priv->selection_area.y;
	max_y = self->priv->selection_area.y + self->priv->selection_area.height;

	begin_idx = get_first_visible_at_offset (self, min_y);
	end_idx = get_last_visible_at_offset (self, max_y);
	if ((begin_idx == -1) || (end_idx == -1))
		return;

	gdk_window_freeze_updates (self->priv->bin_window);

	x1 = self->priv->selection_area.x;
	y1 = self->priv->selection_area.y;
	x2 = x1 + self->priv->selection_area.width;
	y2 = y1 + self->priv->selection_area.height;

	for (i = begin_idx; i <= end_idx; i++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);
		gboolean         selection_changed;

		selection_changed = (item->state & GTK_STATE_FLAG_SELECTED) != (item->tmp_state & GTK_STATE_FLAG_SELECTED);
		if (gth_grid_view_item_is_inside_area (item, x1, y1, x2, y2)) {
			if (invert) {
				if (! selection_changed)
					_gth_grid_view_set_item_selected (self, ! (item->state & GTK_STATE_FLAG_SELECTED), i);
			}
			else if (additive) {
				if (! (item->state & GTK_STATE_FLAG_SELECTED))
					_gth_grid_view_set_item_selected (self, TRUE, i);
			}
			else {
				if (! (item->state & GTK_STATE_FLAG_SELECTED))
					_gth_grid_view_set_item_selected (self, TRUE, i);
			}
		}
		else if (selection_changed)
			_gth_grid_view_set_item_selected (self, (item->tmp_state & GTK_STATE_FLAG_SELECTED), i);
	}

	gdk_window_thaw_updates (self->priv->bin_window);

	_gth_grid_view_emit_selection_changed (self);
}


static void
_gth_grid_view_stop_selecting (GthGridView *self)
{
	if (! self->priv->selecting)
		return;

	self->priv->selecting = FALSE;
	self->priv->sel_start_x = 0;
	self->priv->sel_start_y = 0;

	if (self->priv->scroll_timeout != 0) {
		g_source_remove (self->priv->scroll_timeout);
		self->priv->scroll_timeout = 0;
	}

	gdk_window_invalidate_rect (self->priv->bin_window,
				    &self->priv->selection_area,
				    FALSE);
}


static gboolean
gth_grid_view_button_release (GtkWidget      *widget,
			      GdkEventButton *event)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	if (self->priv->dragging) {
		self->priv->select_pending = self->priv->select_pending && ! self->priv->drag_started;
		self->priv->activate_pending = self->priv->activate_pending && ! self->priv->drag_started;
		_gth_grid_view_stop_dragging (self);
	}

	if (self->priv->selecting) {
		_gth_grid_view_update_mouse_selection (self, event->x, event->y);
		_gth_grid_view_stop_selecting (self);
	}

	if (self->priv->select_pending) {
		self->priv->select_pending = FALSE;
		_gth_grid_view_unselect_all (self, NULL);
		_gth_grid_view_set_item_selected_and_emit_signal (self, TRUE, self->priv->select_pending_pos);
		self->priv->last_selected_pos = self->priv->select_pending_pos;
		if (self->priv->activate_on_single_click && ((event->state & GDK_MOD1_MASK) == 0))
			self->priv->activate_pending = TRUE;
	}

	if (self->priv->activate_pending) {
		self->priv->activate_pending = FALSE;
		gth_file_view_activated (GTH_FILE_VIEW (self), self->priv->last_selected_pos);
	}

	return FALSE;
}


static gboolean
autoscroll_cb (gpointer user_data)
{
	GthGridView *self = user_data;
	double       max_value;
	double       value;

	max_value = gtk_adjustment_get_upper (self->priv->vadjustment) - gtk_adjustment_get_page_size (self->priv->vadjustment);
	value = gtk_adjustment_get_value (self->priv->vadjustment) + self->priv->autoscroll_y_delta;
	if (value > max_value)
		value = max_value;

	gtk_adjustment_set_value (self->priv->vadjustment, value);
	self->priv->event_last_y = self->priv->event_last_y + self->priv->autoscroll_y_delta;
	_gth_grid_view_update_mouse_selection (self, self->priv->event_last_x, self->priv->event_last_y);

	return TRUE;
}


static gboolean
gth_grid_view_motion_notify (GtkWidget      *widget,
			     GdkEventMotion *event)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	if (self->priv->dragging) {
		if (! self->priv->drag_started
		    && (self->priv->selection != NULL)
		    && gtk_drag_check_threshold (widget,
						 self->priv->drag_start_x,
						 self->priv->drag_start_y,
						 event->x,
						 event->y))
		{
			GthGridViewItem *item;
			int              pos;
			GdkDragContext  *context;
			gboolean         multi_dnd;

			/**/

			pos = _gth_grid_view_get_at_position (self,
							      self->priv->drag_start_x,
							      self->priv->drag_start_y,
							      &item);
			if (pos != -1)
				gth_file_view_set_cursor (GTH_FILE_VIEW (self), pos);

			/**/

			self->priv->drag_started = TRUE;
			self->priv->select_pending = FALSE;
			self->priv->activate_pending = FALSE;
			self->priv->selecting = FALSE;

			context = gtk_drag_begin_with_coordinates (widget,
								   self->priv->drag_target_list,
								   self->priv->drag_actions,
								   self->priv->drag_button,
								   (GdkEvent *) event,
								   -1,
								   -1);

			multi_dnd = self->priv->selection->next != NULL;

			if ((pos != -1) && (item != NULL) && (item->thumbnail != NULL)) {
				cairo_surface_t *icon = _cairo_create_dnd_icon (item->thumbnail,
										self->priv->thumbnail_size,
										item->style,
										multi_dnd);
				cairo_surface_set_device_offset (icon,
								 item->thumbnail_area.x - event->x,
								 item->thumbnail_area.y - event->y);
				gtk_drag_set_icon_surface (context, icon);
				cairo_surface_destroy (icon);
			}
			else
				gtk_drag_set_icon_name (context,
							multi_dnd ? "emblem-documents-symbolic" : "folder-documents-symbolic",
							-4,
							-4);
		}

		return TRUE;
	}
	else if (self->priv->selecting) {
		double y_delta;

		_gth_grid_view_update_mouse_selection (self, event->x, event->y);

		/* If we are out of bounds, schedule a timeout that will do
		 * the scrolling */

		y_delta = event->y - gtk_adjustment_get_value (self->priv->vadjustment);
		if ((y_delta < 0) || (y_delta > gtk_widget_get_allocated_height (widget))) {
			self->priv->event_last_x = event->x;
			self->priv->event_last_y = event->y;

			/* Make the stepping relative to the mouse
			 * distance from the canvas.
			 * Also notice the timeout below is small to give a
			 * more smooth movement.
			 */
			if (y_delta < 0)
				self->priv->autoscroll_y_delta = y_delta;
			else
				self->priv->autoscroll_y_delta = y_delta - gtk_widget_get_allocated_height (widget);
			self->priv->autoscroll_y_delta /= 2;

			if (self->priv->scroll_timeout == 0)
				self->priv->scroll_timeout = gdk_threads_add_timeout (SCROLL_DELAY, autoscroll_cb, self);
		}
		else if (self->priv->scroll_timeout != 0) {
			g_source_remove (self->priv->scroll_timeout);
			self->priv->scroll_timeout = 0;
		}

		return TRUE;
	}

	return FALSE;
}


static void
gth_grid_view_drag_end (GtkWidget      *widget,
	                GdkDragContext *context)
{
	GthGridView *self = GTH_GRID_VIEW (widget);
	_gth_grid_view_stop_dragging (self);
}


static void
gth_grid_view_drag_leave (GtkWidget      *widget,
	                  GdkDragContext *context,
			  guint           time)
{
	GthGridView *self = GTH_GRID_VIEW (widget);

	if ((self->priv->drop_pos != GTH_DROP_POSITION_NONE) || (self->priv->drop_item != -1)) {
		self->priv->drop_item = -1;
		self->priv->drop_pos = GTH_DROP_POSITION_NONE;
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}


static void
select_range_with_keyboard (GthGridView *self,
			    int          next_focused_item)
{
	int begin_idx;
	int end_idx;
	int i;

	if (self->priv->focused_item == -1)
		return;

	begin_idx = MIN (MIN (self->priv->first_focused_item, self->priv->focused_item), next_focused_item);
	end_idx = MAX (MAX (self->priv->first_focused_item, self->priv->focused_item), next_focused_item);

	gdk_window_freeze_updates (self->priv->bin_window);

	for (i = begin_idx; i <= end_idx; i++) {
		if (next_focused_item > self->priv->first_focused_item) {
			if ((i >= self->priv->first_focused_item) && (i <= next_focused_item))
				_gth_grid_view_select_item (self, i);
			else
				_gth_grid_view_unselect_item (self, i);
		}
		else {
			if ((i >= next_focused_item) && (i <= self->priv->first_focused_item))
				_gth_grid_view_select_item (self, i);
			else
				_gth_grid_view_unselect_item (self, i);
		}
	}

	gdk_window_thaw_updates (self->priv->bin_window);

	_gth_grid_view_emit_selection_changed (self);
}


static GList *
_gth_grid_view_get_line_at_position (GthGridView *self,
				     int          pos)
{
	GthGridViewItem *item;
	GList           *scan;

	item = g_ptr_array_index (self->priv->itemv, pos);
	for (scan = self->priv->lines; scan; scan = scan->next) {
		GthGridViewLine *line = scan->data;

		if (g_list_find (line->items, item) != NULL)
			return scan;
	}

	return NULL;
}


static int
_gth_grid_view_get_item_at_page_distance (GthGridView *self,
					  int          focused_item,
					  gboolean     downward)
{
	int    old_focused_item;
	int    direction;
	int    h;
	GList *line;
	int    items_per_line;

	old_focused_item = focused_item;
	direction = downward ? 1 : -1;
	h = gtk_widget_get_allocated_height (GTK_WIDGET (self));
	line = _gth_grid_view_get_line_at_position (self, focused_item);
	items_per_line = gth_grid_view_get_items_per_line (self);

	while ((h > 0) && (line != NULL)) {
		h -= GTH_GRID_VIEW_LINE (line->data)->height + self->priv->cell_spacing;
		if (h > 0) {
			focused_item = focused_item + direction * items_per_line;
			if ((focused_item >= self->priv->n_items - 1) || (focused_item <= 0))
				return focused_item;
		}

		if (downward)
			line = line->next;
		else
			line = line->prev;
	}

	if (old_focused_item == focused_item)
		focused_item = focused_item + (direction * items_per_line);

	return focused_item;
}


static gboolean
gth_grid_view_move_cursor (GthGridView        *self,
			   GthCursorMovement   dir,
			   GthSelectionChange  sel_change)
{
	int items_per_line;
	int next_focused_item;

	if (self->priv->n_items == 0)
		return FALSE;

	if (! gtk_widget_has_focus (GTK_WIDGET (self)))
		return FALSE;

	items_per_line = gth_grid_view_get_items_per_line (self);
	next_focused_item = self->priv->focused_item;

	if (self->priv->focused_item == -1) {
		self->priv->first_focused_item = 0;
		next_focused_item = 0;
	}
	else {
		switch (dir) {
		case GTH_CURSOR_MOVE_RIGHT:
			next_focused_item++;
			break;

		case GTH_CURSOR_MOVE_LEFT:
			next_focused_item--;
			break;

		case GTH_CURSOR_MOVE_DOWN:
			next_focused_item += items_per_line;
			break;

		case GTH_CURSOR_MOVE_UP:
			next_focused_item -= items_per_line;
			break;

		case GTH_CURSOR_MOVE_PAGE_UP:
			next_focused_item = _gth_grid_view_get_item_at_page_distance (self,
								     	     	      next_focused_item,
								     	     	      FALSE);
			break;

		case GTH_CURSOR_MOVE_PAGE_DOWN:
			next_focused_item = _gth_grid_view_get_item_at_page_distance (self,
								     	     	      next_focused_item,
								     	     	      TRUE);
			break;

		case GTH_CURSOR_MOVE_BEGIN:
			next_focused_item = 0;
			break;

		case GTH_CURSOR_MOVE_END:
			next_focused_item = self->priv->n_items - 1;
			break;

		default:
			break;
		}

		if (((next_focused_item < 0) && (self->priv->focused_item == 0))
		    || ((next_focused_item > self->priv->n_items - 1) && (self->priv->focused_item == self->priv->n_items - 1)))
		{
			gdk_window_beep (gtk_widget_get_window (GTK_WIDGET (self)));
		}
		next_focused_item = CLAMP (next_focused_item, 0, self->priv->n_items - 1);
	}

	if (sel_change == GTH_SELECTION_SET_CURSOR) {
		_gth_grid_view_unselect_all (self, NULL);
		_gth_grid_view_set_item_selected (self, TRUE, next_focused_item);
		_gth_grid_view_emit_selection_changed (self);
	}
	else if (sel_change == GTH_SELECTION_SET_RANGE)
		select_range_with_keyboard (self, next_focused_item);

	gth_file_view_set_cursor (GTH_FILE_VIEW (self), next_focused_item);

	return TRUE;
}


static gboolean
gth_grid_view_select_cursor_item (GthGridView *self)
{
	GthGridViewItem *item;

	if (self->priv->focused_item == -1)
		return FALSE;

	item = g_ptr_array_index (self->priv->itemv, self->priv->focused_item);
	g_return_val_if_fail (item != NULL, FALSE);

	_gth_grid_view_unselect_all (self, item);
	_gth_grid_view_select_item (self, self->priv->focused_item);
	self->priv->last_selected_pos = self->priv->select_pending_pos;
	_gth_grid_view_emit_selection_changed (self);

	return TRUE;
}


static gboolean
gth_grid_view_toggle_cursor_item (GthGridView *self)
{
	GthGridViewItem *item;

	if (self->priv->focused_item == -1)
		return FALSE;

	item = g_ptr_array_index (self->priv->itemv, self->priv->focused_item);
	if (item->state & GTK_STATE_FLAG_SELECTED)
		_gth_grid_view_unselect_item (self, self->priv->focused_item);
	else
		_gth_grid_view_select_item (self, self->priv->focused_item);
	_gth_grid_view_emit_selection_changed (self);

	return TRUE;
}


static gboolean
gth_grid_view_activate_cursor_item (GthGridView *self)
{
	if (self->priv->focused_item >= 0) {
		gth_file_view_activated (GTH_FILE_VIEW (self), self->priv->focused_item);
		return TRUE;
	}

	return FALSE;
}


static void
_gtk_binding_entry_add_move_cursor_signals (GtkBindingSet     *binding_set,
					    guint              keyval,
					    GthCursorMovement  dir)
{
	gtk_binding_entry_add_signal (binding_set, keyval, 0,
				      "move-cursor", 2,
				      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTH_SELECTION_SET_CURSOR);

	gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
				      "move-cursor", 2,
				      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTH_SELECTION_KEEP);

	gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK,
				      "move-cursor", 2,
				      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTH_SELECTION_SET_RANGE);
}


static void
_gth_grid_view_set_hadjustment (GthGridView   *self,
				GtkAdjustment *adjustment)
{
	if (adjustment != NULL) {
		g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
		g_object_ref_sink (adjustment);
	}
	else
		adjustment = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	if (self->priv->hadjustment != NULL) {
		_g_signal_handlers_disconnect_by_data (self->priv->hadjustment, self);
		g_object_unref (self->priv->hadjustment);
	}

	self->priv->hadjustment = adjustment;
	_gth_grid_view_configure_hadjustment (self);

	g_signal_connect (self->priv->hadjustment,
			  "value-changed",
			  G_CALLBACK (adjustment_value_changed),
			  self);
}


static void
_gth_grid_view_set_vadjustment (GthGridView   *self,
				GtkAdjustment *adjustment)
{
	if (adjustment != NULL) {
		g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
		g_object_ref_sink (adjustment);
	}
	else
		adjustment = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	if (self->priv->vadjustment != NULL) {
		_g_signal_handlers_disconnect_by_data (self->priv->vadjustment, self);
		g_object_unref (self->priv->vadjustment);
	}

	self->priv->vadjustment = adjustment;
	_gth_grid_view_configure_vadjustment (self);

	g_signal_connect (self->priv->vadjustment,
			  "value-changed",
			  G_CALLBACK (adjustment_value_changed),
			  self);
}


static void
_gth_grid_view_set_thumbnail_size (GthGridView *self,
				   int          size)
{
	self->priv->thumbnail_size = size;
	self->priv->cell_size = self->priv->thumbnail_size + (self->priv->thumbnail_border * 2) + (self->priv->cell_padding * 2);
	self->priv->update_caption_height = TRUE;
	g_object_notify (G_OBJECT (self), "thumbnail-size");

	gtk_widget_queue_resize (GTK_WIDGET (self));
}


static void
_gth_grid_view_set_caption (GthGridView *self,
			    const char  *attributes)
{
	int i;

	g_free (self->priv->caption_attributes);
	self->priv->caption_attributes = g_strdup (attributes);

	if (self->priv->caption_attributes_v != NULL) {
		g_strfreev (self->priv->caption_attributes_v);
		self->priv->caption_attributes_v = NULL;
	}
	if (self->priv->caption_attributes != NULL)
		self->priv->caption_attributes_v = g_strsplit (self->priv->caption_attributes, ",", -1);

	self->priv->no_caption = (self->priv->caption_attributes_v == NULL) || (self->priv->caption_attributes_v[0] == NULL) || (g_strcmp0 (self->priv->caption_attributes_v[0], "none") == 0);

	for (i = 0; i < self->priv->n_items; i++) {
		GthGridViewItem *item = g_ptr_array_index (self->priv->itemv, i);
		gth_grid_view_item_update_caption (item, self->priv->caption_attributes_v);
	}
	self->priv->update_caption_height = TRUE;

	g_object_notify (G_OBJECT (self), "caption");

	_gth_grid_view_queue_relayout (self);
}


static void
_gth_grid_view_set_activate_on_single_click (GthGridView *self,
					     gboolean     value)
{
	self->priv->activate_on_single_click = value;
	g_object_notify (G_OBJECT (self), "activate-on-single-click");
}


static void
gth_grid_view_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GthGridView *self;

	self = GTH_GRID_VIEW (object);

	switch (prop_id) {
	case PROP_CAPTION:
		_gth_grid_view_set_caption (self, g_value_get_string (value));
		break;
	case PROP_HADJUSTMENT:
		_gth_grid_view_set_hadjustment (self, g_value_get_object (value));
		break;
	case PROP_HSCROLL_POLICY:
		/* FIXME */
		break;
	case PROP_MODEL:
		gth_grid_view_set_model (GTH_FILE_VIEW (self), g_value_get_object (value));
		break;
	case PROP_THUMBNAIL_SIZE:
		_gth_grid_view_set_thumbnail_size (self, g_value_get_int (value));
		break;
	case PROP_ACTIVATE_ON_SINGLE_CLICK:
		_gth_grid_view_set_activate_on_single_click (self, g_value_get_boolean (value));
		break;
	case PROP_VADJUSTMENT:
		_gth_grid_view_set_vadjustment (self, g_value_get_object (value));
		break;
	case PROP_VSCROLL_POLICY:
		/* FIXME */
		break;
	default:
		break;
	}
}


static void
gth_grid_view_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GthGridView *self;

	self = GTH_GRID_VIEW (object);

	switch (prop_id) {
	case PROP_CAPTION:
		g_value_set_string (value, self->priv->caption_attributes);
		break;
	case PROP_CELL_SPACING:
		g_value_set_int (value, self->priv->cell_spacing);
		break;
	case PROP_HADJUSTMENT:
		g_value_set_object (value, self->priv->hadjustment);
		break;
	case PROP_HSCROLL_POLICY:
		g_value_set_enum (value, GTK_SCROLL_NATURAL);
		break;
	case PROP_MODEL:
		g_value_set_object (value, self->priv->model);
		break;
	case PROP_THUMBNAIL_SIZE:
		g_value_set_int (value, self->priv->thumbnail_size);
		break;
	case PROP_ACTIVATE_ON_SINGLE_CLICK:
		g_value_set_boolean (value, self->priv->activate_on_single_click);
		break;
	case PROP_VADJUSTMENT:
		g_value_set_object (value, self->priv->vadjustment);
		break;
	case PROP_VSCROLL_POLICY:
		g_value_set_enum (value, GTK_SCROLL_NATURAL);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_grid_view_class_init (GthGridViewClass *grid_view_class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	/* Methods */

	gobject_class = (GObjectClass*) grid_view_class;
	gobject_class->finalize = gth_grid_view_finalize;
	gobject_class->set_property = gth_grid_view_set_property;
	gobject_class->get_property = gth_grid_view_get_property;

	widget_class = (GtkWidgetClass*) grid_view_class;
	widget_class->map = gth_grid_view_map;
	widget_class->unmap = gth_grid_view_unmap;
	widget_class->realize = gth_grid_view_realize;
	widget_class->unrealize = gth_grid_view_unrealize;
	widget_class->state_flags_changed = gth_grid_view_state_flags_changed;
	widget_class->style_updated = gth_grid_view_style_updated;
	widget_class->get_preferred_width = gth_grid_view_get_preferred_width;
	widget_class->get_preferred_height = gth_grid_view_get_preferred_height;
	widget_class->size_allocate = gth_grid_view_size_allocate;
	widget_class->draw = gth_grid_view_draw;
	widget_class->key_press_event = gth_grid_view_key_press;
	widget_class->key_release_event = gth_grid_view_key_release;
	widget_class->button_press_event = gth_grid_view_button_press;
	widget_class->button_release_event = gth_grid_view_button_release;
	widget_class->motion_notify_event = gth_grid_view_motion_notify;
	widget_class->drag_end = gth_grid_view_drag_end;
	widget_class->drag_leave = gth_grid_view_drag_leave;

	grid_view_class->select_all = gth_grid_view_select_all;
	grid_view_class->unselect_all = gth_grid_view_unselect_all;
	grid_view_class->toggle_cursor_item = gth_grid_view_toggle_cursor_item;
	grid_view_class->select_cursor_item = gth_grid_view_select_cursor_item;
	grid_view_class->move_cursor = gth_grid_view_move_cursor;
	grid_view_class->activate_cursor_item = gth_grid_view_activate_cursor_item;

	/* Signals */

	grid_view_signals[SELECT_ALL] =
		g_signal_new ("select-all",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthGridViewClass, select_all),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	grid_view_signals[UNSELECT_ALL] =
		g_signal_new ("unselect-all",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthGridViewClass, unselect_all),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	grid_view_signals[ACTIVATE_CURSOR_ITEM] =
		g_signal_new ("activate-cursor-item",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthGridViewClass, activate_cursor_item),
			      NULL, NULL,
			      gth_marshal_BOOLEAN__VOID,
			      G_TYPE_BOOLEAN, 0);
	grid_view_signals[MOVE_CURSOR] =
		g_signal_new ("move-cursor",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthGridViewClass, move_cursor),
			      NULL, NULL,
			      gth_marshal_BOOLEAN__ENUM_ENUM,
			      G_TYPE_BOOLEAN, 2,
			      GTH_TYPE_CURSOR_MOVEMENT,
			      GTH_TYPE_SELECTION_CHANGE);
	grid_view_signals[SELECT_CURSOR_ITEM] =
		g_signal_new ("select-cursor-item",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthGridViewClass, select_cursor_item),
			      NULL, NULL,
			      gth_marshal_BOOLEAN__VOID,
			      G_TYPE_BOOLEAN, 0);
	grid_view_signals[TOGGLE_CURSOR_ITEM] =
		g_signal_new ("toggle-cursor-item",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthGridViewClass, toggle_cursor_item),
			      NULL, NULL,
			      gth_marshal_BOOLEAN__VOID,
			      G_TYPE_BOOLEAN, 0);

	/* Properties */

	g_object_class_install_property (gobject_class,
					 PROP_CELL_SPACING,
					 g_param_spec_int ("cell-spacing",
							   "Cell Spacing",
							   "Spacing between cells both horizontally and vertically",
							   0,
							   G_MAXINT32,
							   DEFAULT_CELL_SPACING,
							   G_PARAM_READWRITE));

	/* GtkScrollable properties */

	g_object_class_override_property (gobject_class, PROP_HADJUSTMENT, "hadjustment");
	g_object_class_override_property (gobject_class, PROP_VADJUSTMENT, "vadjustment");
	g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
	g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

	/* GthFileView properties */

	g_object_class_override_property (gobject_class, PROP_CAPTION, "caption");
	g_object_class_override_property (gobject_class, PROP_MODEL, "model");
	g_object_class_override_property (gobject_class, PROP_THUMBNAIL_SIZE, "thumbnail-size");
	g_object_class_override_property (gobject_class, PROP_ACTIVATE_ON_SINGLE_CLICK, "activate-on-single-click");

	/* Key bindings */

	binding_set = gtk_binding_set_by_class (grid_view_class);

	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Right, GTH_CURSOR_MOVE_RIGHT);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Left, GTH_CURSOR_MOVE_LEFT);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Down, GTH_CURSOR_MOVE_DOWN);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Up, GTH_CURSOR_MOVE_UP);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Page_Up, GTH_CURSOR_MOVE_PAGE_UP);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Page_Down, GTH_CURSOR_MOVE_PAGE_DOWN);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_Home, GTH_CURSOR_MOVE_BEGIN);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_End, GTH_CURSOR_MOVE_END);

	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Right, GTH_CURSOR_MOVE_RIGHT);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Left, GTH_CURSOR_MOVE_LEFT);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Down, GTH_CURSOR_MOVE_DOWN);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Up, GTH_CURSOR_MOVE_UP);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Page_Up, GTH_CURSOR_MOVE_PAGE_UP);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Page_Down, GTH_CURSOR_MOVE_PAGE_DOWN);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_Home, GTH_CURSOR_MOVE_BEGIN);
	_gtk_binding_entry_add_move_cursor_signals (binding_set, GDK_KEY_KP_End, GTH_CURSOR_MOVE_END);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_space, 0,
				      "select-cursor-item", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, 0,
				      "select-cursor-item", 0);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_space, GDK_CONTROL_MASK,
				      "toggle-cursor-item", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, GDK_CONTROL_MASK,
				      "toggle-cursor-item", 0);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
				      "select-all", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_slash, GDK_CONTROL_MASK,
				      "select-all", 0);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_A, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
				      "unselect-all", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_backslash, GDK_CONTROL_MASK,
				      "unselect-all", 0);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Return, 0,
				      "activate-cursor-item", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_ISO_Enter, 0,
				      "activate-cursor-item", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Enter, 0,
				      "activate-cursor-item", 0);

#if GTK_CHECK_VERSION(3,20,0)
	gtk_widget_class_set_css_name (widget_class, "iconview");
#endif
}


static void
gth_grid_view_gth_file_selection_interface_init (GthFileSelectionInterface *iface)
{
	iface->set_selection_mode = gth_grid_view_set_selection_mode;
	iface->get_selected = gth_grid_view_get_selected;
	iface->select = gth_grid_view_select;
	iface->unselect = gth_grid_view_unselect;
	iface->select_all = gth_grid_view_select_all;
	iface->unselect_all = gth_grid_view_unselect_all;
	iface->is_selected = gth_grid_view_is_selected;
	iface->get_first_selected = gth_grid_view_get_first_selected;
	iface->get_last_selected = gth_grid_view_get_last_selected;
	iface->get_n_selected = gth_grid_view_get_n_selected;
}


static void
gth_grid_view_gth_file_view_interface_init (GthFileViewInterface *iface)
{
	iface->scroll_to = gth_grid_view_scroll_to;
	iface->set_vscroll = gth_grid_view_set_vscroll;
	iface->get_visibility = gth_grid_view_get_visibility;
	iface->get_at_position = gth_grid_view_get_at_position;
	iface->get_first_visible = gth_grid_view_get_first_visible;
	iface->get_last_visible = gth_grid_view_get_last_visible;
	iface->cursor_changed = gth_grid_view_cursor_changed;
	iface->get_cursor = gth_grid_view_get_cursor;
	iface->enable_drag_source = gth_grid_view_enable_drag_source;
	iface->unset_drag_source = gth_grid_view_unset_drag_source;
	iface->enable_drag_dest = gth_grid_view_enable_drag_dest;
	iface->unset_drag_dest = gth_grid_view_unset_drag_dest;
	iface->set_drag_dest_pos = gth_grid_view_set_drag_dest_pos;
	iface->get_drag_dest_pos = gth_grid_view_get_drag_dest_pos;
}


static void
gth_grid_view_gtk_scrollable_interface_init (GtkScrollableInterface *iface)
{
	iface->get_border = gth_grid_view_get_border;
}


static void
gth_grid_view_init (GthGridView *self)
{
	GtkStyleContext *style_context;

	style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_FRAME);

	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

	self->priv = gth_grid_view_get_instance_private (self);

	/* self->priv->model = NULL; */
	self->priv->itemv = g_ptr_array_new_with_free_func ((GDestroyNotify) gth_grid_view_item_unref);
	self->priv->n_items = 0;
	self->priv->lines = NULL;
	self->priv->selection = NULL;
	self->priv->focused_item = -1;
	self->priv->first_focused_item = -1;
	self->priv->make_focused_visible = FALSE;
	self->priv->initial_vscroll = 0.0;
	self->priv->needs_relayout = FALSE;
	self->priv->needs_relayout_after_size_allocate = FALSE;
	self->priv->layout_timeout = 0;
	self->priv->relayout_from_line = -1;
	self->priv->update_caption_height = TRUE;
	self->priv->width = 0;
	self->priv->height = 0;
	/* self->priv->thumbnail_size = 0; */
	self->priv->thumbnail_border = DEFAULT_THUMBNAIL_BORDER;

	/* self->priv->cell_size = 0; */
	self->priv->cell_spacing = DEFAULT_CELL_SPACING;
	self->priv->cell_x_spacing = -1;
	/* self->priv->cell_padding = DEFAULT_CELL_PADDING; */
	self->priv->caption_spacing = DEFAULT_CAPTION_SPACING;
	self->priv->caption_padding = DEFAULT_CAPTION_PADDING;

	self->priv->scroll_timeout = 0;
	self->priv->autoscroll_y_delta = 0;
	self->priv->event_last_x = 0;
	self->priv->event_last_y = 0;

	self->priv->selecting = FALSE;
	self->priv->select_pending = FALSE;
	self->priv->activate_pending = FALSE;
	self->priv->activate_on_single_click = TRUE;
	self->priv->select_pending_pos = -1;
	self->priv->select_pending_item = NULL;
	gth_grid_view_set_selection_mode (GTH_FILE_SELECTION (self), GTK_SELECTION_MULTIPLE);
	/* self->priv->selection_area = 0; */
	self->priv->last_selected_pos = -1;
	self->priv->multi_selecting_with_keyboard = FALSE;
	self->priv->selection_changed = FALSE;
	self->priv->sel_start_x = 0;
	self->priv->sel_start_y = 0;
	self->priv->sel_state = 0;

	self->priv->dragging = FALSE;
	self->priv->drag_started = FALSE;
	self->priv->drag_source_enabled = FALSE;
	self->priv->drag_start_button_mask = 0;
	self->priv->drag_button = 0;
	self->priv->drag_target_list = NULL;
	self->priv->drag_actions = 0;
	self->priv->drag_start_x = 0;
	self->priv->drag_start_y = 0;
	self->priv->drop_item = -1;
	self->priv->drop_pos = GTH_DROP_POSITION_NONE;

	self->priv->bin_window = NULL;

	self->priv->caption_attributes = NULL;
	self->priv->caption_attributes_v = NULL;
	self->priv->no_caption = TRUE;
	self->priv->caption_layout = NULL;
	self->priv->icon_cache = NULL;

	_gth_grid_view_set_hadjustment (self, gtk_adjustment_new (0.0, 1.0, 0.0, 0.1, 1.0, 1.0));
	_gth_grid_view_set_vadjustment (self, gtk_adjustment_new (0.0, 1.0, 0.0, 0.1, 1.0, 1.0));
}


GtkWidget *
gth_grid_view_new (void)
{
	return g_object_new (GTH_TYPE_GRID_VIEW, NULL);
}


void
gth_grid_view_set_cell_spacing (GthGridView *self,
				int          cell_spacing)
{
	g_return_if_fail (GTH_IS_GRID_VIEW (self));

	self->priv->cell_spacing = cell_spacing;
	g_object_notify (G_OBJECT (self), "cell-spacing");

	_gth_grid_view_queue_relayout (self);
}


int
gth_grid_view_get_items_per_line (GthGridView *self)
{
	g_return_val_if_fail (GTH_IS_GRID_VIEW (self), 0);

	return MAX (self->priv->width / (self->priv->cell_size + self->priv->cell_spacing), 1);
}
