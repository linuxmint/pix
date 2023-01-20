/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
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

#ifndef GTH_HOOK_H
#define GTH_HOOK_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

void      gth_hooks_initialize      (void);
void      gth_hook_register         (const char *name,
				     int         n_args);
gboolean  gth_hook_present          (const char *name);
void      gth_hook_add_callback     (const char *name,
				     int         sort_order,
				     GCallback   callback,
				     gpointer    data);
void      gth_hook_remove_callback  (const char *name,
				     GCallback   callback);
void      gth_hook_invoke           (const char *name,
				     gpointer    first_data,
				     ...);
void *    gth_hook_invoke_get       (const char *name,
				     gpointer    first_data,
				     ...);

G_END_DECLS

#endif /* GTH_HOOK_H */
