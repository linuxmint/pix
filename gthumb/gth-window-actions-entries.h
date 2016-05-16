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

#ifndef GTH_WINDOW_ACTION_ENTRIES_H
#define GTH_WINDOW_ACTION_ENTRIES_H

#include <config.h>
#include <glib/gi18n.h>
#include "gth-window-actions-callbacks.h"

static GtkActionEntry gth_window_action_entries[] = {
	{ "File_CloseWindow", GTK_STOCK_CLOSE,
	  NULL, NULL,
	  N_("Close this window"),
	  G_CALLBACK (gth_window_activate_action_file_close_window) },

	{ "File_Quit", GTK_STOCK_QUIT,
	  N_("Close _All Windows"), NULL,
	  NULL,
	  G_CALLBACK (gth_window_activate_action_file_quit_application) }
};

#endif /* GTH_WINDOW_ACTION_ENTRIES_H */
