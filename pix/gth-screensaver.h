/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_SCREENSAVER_H
#define GTH_SCREENSAVER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_SCREENSAVER         (gth_screensaver_get_type ())
#define GTH_SCREENSAVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_SCREENSAVER, GthScreensaver))
#define GTH_SCREENSAVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_SCREENSAVER, GthScreensaverClass))
#define GTH_IS_SCREENSAVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_SCREENSAVER))
#define GTH_IS_SCREENSAVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_SCREENSAVER))
#define GTH_SCREENSAVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_SCREENSAVER, GthScreensaverClass))

typedef struct _GthScreensaver         GthScreensaver;
typedef struct _GthScreensaverPrivate  GthScreensaverPrivate;
typedef struct _GthScreensaverClass    GthScreensaverClass;


struct _GthScreensaver {
	GObject __parent;
	GthScreensaverPrivate *priv;
};

struct _GthScreensaverClass {
	GObjectClass __parent_class;
};

GType             gth_screensaver_get_type   (void) G_GNUC_CONST;
GthScreensaver *  gth_screensaver_new        (GApplication   *application);
void              gth_screensaver_inhibit    (GthScreensaver *self,
					      GtkWindow      *window,
					      const char     *reason);
void              gth_screensaver_uninhibit  (GthScreensaver *self);

G_END_DECLS

#endif /* GTH_SCREENSAVER_H */

