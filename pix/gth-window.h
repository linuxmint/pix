/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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

#ifndef GTH_WINDOW_H
#define GTH_WINDOW_H

#include <gtk/gtk.h>
#include "gtk-utils.h"
#include "gth-shortcut.h"

G_BEGIN_DECLS

#define GTH_WINDOW_PAGE_UNDEFINED -1

typedef enum { /*< skip >*/
	GTH_WINDOW_MENUBAR,
	GTH_WINDOW_TOOLBAR,
	GTH_WINDOW_INFOBAR,
	GTH_WINDOW_STATUSBAR,
} GthWindowArea;

typedef struct { /*< skip >*/
	gboolean saved;
	int      width;
	int      height;
} GthWindowSize;

#define GTH_TYPE_WINDOW              (gth_window_get_type ())
#define GTH_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_WINDOW, GthWindow))
#define GTH_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_WINDOW, GthWindowClass))
#define GTH_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_WINDOW))
#define GTH_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_WINDOW))
#define GTH_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_WINDOW, GthWindowClass))

typedef struct _GthWindow        GthWindow;
typedef struct _GthWindowClass   GthWindowClass;
typedef struct _GthWindowPrivate GthWindowPrivate;

struct _GthWindow
{
	GtkApplicationWindow __parent;
	GthWindowPrivate *priv;
};

struct _GthWindowClass
{
	GtkApplicationWindowClass __parent_class;

	/*< virtual functions >*/

	void (*set_current_page)  (GthWindow *window,
				   int        page);
	void (*close)             (GthWindow *window);
};

GType          gth_window_get_type           (void);
void           gth_window_close              (GthWindow     *window);
void           gth_window_attach             (GthWindow     *window,
					      GtkWidget     *child,
					      GthWindowArea  area);
void           gth_window_attach_toolbar     (GthWindow     *window,
					      int            page,
					      GtkWidget     *child);
void           gth_window_attach_content     (GthWindow     *window,
					      int            page,
					      GtkWidget     *child);
void           gth_window_set_current_page   (GthWindow     *window,
					      int            page);
int            gth_window_get_current_page   (GthWindow     *window);
void           gth_window_show_only_content  (GthWindow     *window,
					      gboolean       only_content);
GtkWidget *    gth_window_get_area           (GthWindow     *window,
					      GthWindowArea  area);
void           gth_window_add_overlay        (GthWindow     *window,
					      GtkWidget     *widget);
void           gth_window_set_header_bar     (GthWindow     *window,
					      GtkWidget     *header_bar);
GtkWidget *    gth_window_get_header_bar     (GthWindow     *window);
void           gth_window_save_page_size     (GthWindow     *window,
					      int            page,
					      int            width,
					      int            height);
void           gth_window_apply_saved_size   (GthWindow     *window,
					      int            page);
gboolean       gth_window_get_page_size      (GthWindow     *window,
		      	      	      	      int            page,
		      	      	      	      int           *width,
		      	      	      	      int           *height);
void           gth_window_clear_saved_size   (GthWindow     *window,
      	      	      	      	      	      int            page);
void           gth_window_set_title          (GthWindow     *window,
					      const char    *title,
					      GList	    *emblems);
GtkAccelGroup *gth_window_get_accel_group	(GthWindow		*window);
void           gth_window_add_accelerators	(GthWindow		*window,
						 const GthAccelerator	*accelerators,
						 int			 n_accelerators);
void		gth_window_enable_action	(GthWindow		*window,
						 const char		*action_name,
						 gboolean		 enabled);
gboolean	gth_window_get_action_state	(GthWindow		*window,
						 const char		*action_name);
void		gth_window_change_action_state	(GthWindow		*window,
						 const char		*action_name,
						 gboolean		 value);
void		gth_window_activate_action	(GthWindow		*window,
						 const char		*action_name,
						 GVariant		*parameter);
void		gth_window_add_viewer_shortcuts (GthWindow              *window,
						 const char             *viewer_context,
						 const GthShortcut      *shortcuts,
						 int                     n_shortcuts);
void		gth_window_add_shortcuts	(GthWindow		*window,
						 const GthShortcut	*shortcuts,
						 int			 n_shortcuts);
GPtrArray *	gth_window_get_shortcuts	(GthWindow		*window);
GthShortcut *   gth_window_get_shortcut	(GthWindow		*window,
						 const char             *detailed_action);
GPtrArray *	gth_window_get_shortcuts_by_category
						(GthWindow		*window);
gboolean	gth_window_activate_shortcut	(GthWindow		*window,
						 int			 context,
						 const char             *viewer_context,
						 guint			 keycode,
						 GdkModifierType	 modifiers);
void		gth_window_load_shortcuts	(GthWindow		*window);
void		gth_window_add_removable_shortcut
						(GthWindow		*window,
						 const char		*group_name,
						 GthShortcut		*shortcut);
void		gth_window_remove_shortcuts	(GthWindow		*window,
						 const char		*group_name);
GHashTable *	gth_window_get_accels_for_group (GthWindow		*window,
						 const char		*group_name);
gboolean	gth_window_can_change_shortcut	(GthWindow		*window,
						 const char		*detailed_action,
						 int			 context,
						 const char		*viewer_context,
						 guint			 keycode,
						 GdkModifierType	 modifiers,
						 GtkWindow		*parent);
void		gth_window_copy_shortcuts	(GthWindow		*to_window,
						 GthWindow		*from_window,
						 int			 context);

G_END_DECLS

#endif /* GTH_WINDOW_H */
