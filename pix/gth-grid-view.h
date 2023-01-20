/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2011 The Free Software Foundation, Inc.
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

#ifndef GTH_GRID_VIEW_H
#define GTH_GRID_VIEW_H

#include <gtk/gtk.h>
#include "typedefs.h"
#include "gth-file-selection.h"

G_BEGIN_DECLS

typedef enum {
	GTH_CURSOR_MOVE_UP,
	GTH_CURSOR_MOVE_DOWN,
	GTH_CURSOR_MOVE_RIGHT,
	GTH_CURSOR_MOVE_LEFT,
	GTH_CURSOR_MOVE_PAGE_UP,
	GTH_CURSOR_MOVE_PAGE_DOWN,
	GTH_CURSOR_MOVE_BEGIN,
	GTH_CURSOR_MOVE_END,
} GthCursorMovement;

typedef enum {
        GTH_DROP_POSITION_NONE,
        GTH_DROP_POSITION_INTO,
        GTH_DROP_POSITION_LEFT,
        GTH_DROP_POSITION_RIGHT
} GthDropPosition;

typedef enum {
	GTH_SELECTION_KEEP,           /* Do not change the selection. */
	GTH_SELECTION_SET_CURSOR,     /* Select the cursor image. */
	GTH_SELECTION_SET_RANGE       /* Select the images contained
				       * in the rectangle that has as
				       * opposite corners the last
				       * focused image and the
				       * currently focused image. */
} GthSelectionChange;

#define GTH_TYPE_GRID_VIEW            (gth_grid_view_get_type ())
#define GTH_GRID_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_GRID_VIEW, GthGridView))
#define GTH_GRID_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_GRID_VIEW, GthGridViewClass))
#define GTH_IS_GRID_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_GRID_VIEW))
#define GTH_IS_GRID_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_GRID_VIEW))
#define GTH_GRID_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_GRID_VIEW, GthGridViewClass))

typedef struct _GthGridView GthGridView;
typedef struct _GthGridViewClass GthGridViewClass;
typedef struct _GthGridViewPrivate GthGridViewPrivate;

struct _GthGridView {
	GtkWidget __parent;
	GthGridViewPrivate *priv;
};

struct _GthGridViewClass {
	GtkWidgetClass __parent_class;

        /*< key binding signals >*/

	void     (* select_all)               (GthFileSelection   *grid_view);
	void     (* unselect_all)             (GthFileSelection   *grid_view);
	gboolean (* select_cursor_item)       (GthGridView        *grid_view);
	gboolean (* toggle_cursor_item)       (GthGridView        *grid_view);
        gboolean (* move_cursor)              (GthGridView        *grid_view,
					       GthCursorMovement   dir,
					       GthSelectionChange  sel_change);
	gboolean (* activate_cursor_item)     (GthGridView        *grid_view);
};

GType          gth_grid_view_get_type              (void);
GtkWidget *    gth_grid_view_new                   (void);
void           gth_grid_view_set_cell_spacing      (GthGridView   *grid_view,
						    int            cell_spacing);
int            gth_grid_view_get_items_per_line    (GthGridView   *self);

G_END_DECLS

#endif /* GTH_GRID_VIEW_H */
