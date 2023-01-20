/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 Free Software Foundation, Inc.
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

#ifndef GTH_COLOR_MANAGER_H
#define GTH_COLOR_MANAGER_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-icc-profile.h"

G_BEGIN_DECLS

#define GTH_TYPE_COLOR_MANAGER         (gth_color_manager_get_type ())
#define GTH_COLOR_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_COLOR_MANAGER, GthColorManager))
#define GTH_COLOR_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_COLOR_MANAGER, GthColorManagerClass))
#define GTH_IS_COLOR_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_COLOR_MANAGER))
#define GTH_IS_COLOR_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_COLOR_MANAGER))
#define GTH_COLOR_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_COLOR_MANAGER, GthColorManagerClass))

typedef struct _GthColorManager         GthColorManager;
typedef struct _GthColorManagerPrivate  GthColorManagerPrivate;
typedef struct _GthColorManagerClass    GthColorManagerClass;

struct _GthColorManager {
	GObject __parent;
	GthColorManagerPrivate *priv;
};

struct _GthColorManagerClass {
	GObjectClass __parent_class;

	/*< signals >*/

	void	(*changed)	(GthColorManager *color_manager);
};

GType			gth_color_manager_get_type		(void) G_GNUC_CONST;
GthColorManager *	gth_color_manager_new			(void);
void			gth_color_manager_get_profile_async	(GthColorManager	 *color_manager,
								 char 			 *monitor_name,
		     	     	     	     	     	         GCancellable		 *cancellable,
								 GAsyncReadyCallback	  callback,
								 gpointer		  user_data);
GthICCProfile *		gth_color_manager_get_profile_finish	(GthColorManager	 *color_manager,
								 GAsyncResult		 *result,
								 GError			**error);
GthICCTransform *       gth_color_manager_get_transform         (GthColorManager	 *color_manager,
								 GthICCProfile           *from_profile,
								 GthICCProfile           *to_profile);

G_END_DECLS

#endif /* GTH_COLOR_MANAGER_H */
