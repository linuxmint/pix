/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Pix
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <pix.h>
#include "callbacks.h"
#include "preferences.h"


G_MODULE_EXPORT void
pix_extension_activate (void)
{
	gth_hook_add_callback ("gth-browser-construct", 9, G_CALLBACK (ip__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-selection-changed", 10, G_CALLBACK (ip__gth_browser_selection_changed_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 40, G_CALLBACK (ip__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-apply", 10, G_CALLBACK (ip__dlg_preferences_apply_cb), NULL);
}


G_MODULE_EXPORT void
pix_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
pix_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
pix_extension_configure (GtkWindow *parent)
{
}
