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

#ifndef GTH_ACCEL_BUTTON_H
#define GTH_ACCEL_BUTTON_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_ACCEL_BUTTON         (gth_accel_button_get_type ())
#define GTH_ACCEL_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_ACCEL_BUTTON, GthAccelButton))
#define GTH_ACCEL_BUTTON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_ACCEL_BUTTON, GthAccelButtonClass))
#define GTH_IS_ACCEL_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_ACCEL_BUTTON))
#define GTH_IS_ACCEL_BUTTON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_ACCEL_BUTTON))
#define GTH_ACCEL_BUTTON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_ACCEL_BUTTON, GthAccelButtonClass))

typedef struct _GthAccelButton         GthAccelButton;
typedef struct _GthAccelButtonPrivate  GthAccelButtonPrivate;
typedef struct _GthAccelButtonClass    GthAccelButtonClass;

struct _GthAccelButton {
	GtkButton __parent;
	GthAccelButtonPrivate *priv;
};

struct _GthAccelButtonClass {
	GtkButtonClass __parent_class;

	/*< signals >*/

	gboolean (*change_value)	(GthAccelButton  *accel_button,
					 guint            keycode,
					 GdkModifierType  modifiers);
	void	 (*changed)		(GthAccelButton  *accel_button);
};

GType         gth_accel_button_get_type            (void) G_GNUC_CONST;
GtkWidget *   gth_accel_button_new                 (void);
gboolean      gth_accel_button_set_accelerator     (GthAccelButton  *accel_button,
						    guint            accelerator_key,
						    GdkModifierType  accelerator_mods);
gboolean      gth_accel_button_get_accelerator     (GthAccelButton  *accel_button,
						    guint           *accelerator_key,
						    GdkModifierType *accelerator_mods);
gboolean      gth_accel_button_get_valid           (GthAccelButton  *accel_button);

G_END_DECLS

#endif /* GTH_ACCEL_BUTTON_H */
