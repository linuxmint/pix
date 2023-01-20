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

#include <config.h>
#include "gth-screensaver.h"

#define GNOME_SESSION_MANAGER_INHIBIT_IDLE 8


/* Properties */
enum {
	PROP_0,
	PROP_APPLICATION
};


struct _GthScreensaverPrivate {
	GApplication *application;
	guint         cookie;
};


G_DEFINE_TYPE_WITH_CODE (GthScreensaver,
			 gth_screensaver,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthScreensaver))


static void
gth_screensaver_finalize (GObject *object)
{
	GthScreensaver *self;

	self = GTH_SCREENSAVER (object);

	if (self->priv->application != NULL)
		g_object_unref (self->priv->application);

	G_OBJECT_CLASS (gth_screensaver_parent_class)->finalize (object);
}


static void
gth_screensaver_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthScreensaver *self;

	self = GTH_SCREENSAVER (object);

	switch (property_id) {
	case PROP_APPLICATION:
		if (self->priv->application != NULL)
			g_object_unref (self->priv->application);
		self->priv->application = g_value_dup_object (value);
		break;
	default:
		break;
	}
}


static void
gth_screensaver_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthScreensaver *self;

	self = GTH_SCREENSAVER (object);

	switch (property_id) {
	case PROP_APPLICATION:
		g_value_set_object (value, self->priv->application);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_screensaver_class_init (GthScreensaverClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_screensaver_set_property;
	object_class->get_property = gth_screensaver_get_property;
	object_class->finalize = gth_screensaver_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_APPLICATION,
					 g_param_spec_object ("application",
                                                              "Application",
                                                              "The application",
                                                              G_TYPE_APPLICATION,
                                                              G_PARAM_READWRITE));
}


static void
gth_screensaver_init (GthScreensaver *self)
{
	self->priv = gth_screensaver_get_instance_private (self);
	self->priv->application = NULL;
	self->priv->cookie = 0;
}


GthScreensaver *
gth_screensaver_new (GApplication *application)
{
	if (application == NULL)
		application = g_application_get_default ();

	return (GthScreensaver*) g_object_new (GTH_TYPE_SCREENSAVER,
					       "application", application,
					       NULL);
}


void
gth_screensaver_inhibit (GthScreensaver *self,
			 GtkWindow      *window,
			 const char     *reason)
{
	if (self->priv->cookie != 0)
		return;

	self->priv->cookie = gtk_application_inhibit (GTK_APPLICATION (self->priv->application),
						      window,
						      GTK_APPLICATION_INHIBIT_IDLE,
						      reason);
}


void
gth_screensaver_uninhibit (GthScreensaver *self)
{
	if (self->priv->cookie == 0)
		return;

	gtk_application_uninhibit (GTK_APPLICATION (self->priv->application), self->priv->cookie);
	self->priv->cookie = 0;
}
