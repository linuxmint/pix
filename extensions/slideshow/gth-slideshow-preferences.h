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

#ifndef GTH_SLIDESHOW_PREFERENCES_H
#define GTH_SLIDESHOW_PREFERENCES_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_SLIDESHOW_PREFERENCES              (gth_slideshow_preferences_get_type ())
#define GTH_SLIDESHOW_PREFERENCES(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SLIDESHOW_PREFERENCES, GthSlideshowPreferences))
#define GTH_SLIDESHOW_PREFERENCES_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_SLIDESHOW_PREFERENCES_TYPE, GthSlideshowPreferencesClass))
#define GTH_IS_SLIDESHOW_PREFERENCES(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SLIDESHOW_PREFERENCES))
#define GTH_IS_SLIDESHOW_PREFERENCES_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SLIDESHOW_PREFERENCES))
#define GTH_SLIDESHOW_PREFERENCES_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SLIDESHOW_PREFERENCES, GthSlideshowPreferencesClass))

typedef struct _GthSlideshowPreferences         GthSlideshowPreferences;
typedef struct _GthSlideshowPreferencesClass    GthSlideshowPreferencesClass;
typedef struct _GthSlideshowPreferencesPrivate  GthSlideshowPreferencesPrivate;

struct _GthSlideshowPreferences
{
	GtkBox __parent;
	GthSlideshowPreferencesPrivate *priv;
};

struct _GthSlideshowPreferencesClass
{
	GtkBoxClass __parent_class;
};

GType         gth_slideshow_preferences_get_type           (void);
GtkWidget *   gth_slideshow_preferences_new                (const char               *transition,
						            gboolean                  automatic,
						            int                       delay,
						            gboolean                  wrap_around,
							    gboolean                  random_order);
void          gth_slideshow_preferences_set_audio          (GthSlideshowPreferences  *self,
							    char                    **files);
GtkWidget *   gth_slideshow_preferences_get_widget         (GthSlideshowPreferences  *self,
						            const char               *name);
gboolean      gth_slideshow_preferences_get_personalize    (GthSlideshowPreferences  *self);
char *        gth_slideshow_preferences_get_transition_id  (GthSlideshowPreferences  *self);
gboolean      gth_slideshow_preferences_get_automatic      (GthSlideshowPreferences  *self);
int           gth_slideshow_preferences_get_delay          (GthSlideshowPreferences  *self);
gboolean      gth_slideshow_preferences_get_wrap_around    (GthSlideshowPreferences  *self);
gboolean      gth_slideshow_preferences_get_random_order   (GthSlideshowPreferences  *self);
char **       gth_slideshow_preferences_get_audio_files    (GthSlideshowPreferences  *self);

G_END_DECLS

#endif /* GTH_SLIDESHOW_PREFERENCES_H */
