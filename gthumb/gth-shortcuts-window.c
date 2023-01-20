/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-shortcuts-window.h"
#include "typedefs.h"


typedef struct {
	int   id;
	char *name;
	char *title;
} ContextInfo;


static ContextInfo contexts[] = {
	{ GTH_SHORTCUT_CONTEXT_BROWSER, "browser", N_("Browser") },
	{ GTH_SHORTCUT_CONTEXT_VIEWER, "viewer", N_("Viewer") },
	{ GTH_SHORTCUT_CONTEXT_SLIDESHOW, "slideshow", N_("Presentation") }
};


void
gth_shortcuts_window_new (GthWindow *app_window)
{
	GtkWidget *window;
	GPtrArray *shortcuts_v;
	int        ctx;

	window = g_object_new (GTK_TYPE_SHORTCUTS_WINDOW,
			       "transient-for", GTK_WINDOW (app_window),
			       NULL);

	shortcuts_v = gth_window_get_shortcuts_by_category (app_window);
	for (ctx = 0; ctx < G_N_ELEMENTS (contexts); ctx++) {
		ContextInfo *context = &contexts[ctx];
		GtkWidget   *section;
		GtkWidget   *group;
		const char  *last_category;
		int          i;

		section = g_object_new (GTK_TYPE_SHORTCUTS_SECTION,
					"section-name", context->name,
					"title", _(context->title),
					NULL);
		gtk_widget_show (section);
		gtk_container_add (GTK_CONTAINER (window), section);

		group = NULL;
		last_category = NULL;
		for (i = 0; i < shortcuts_v->len; i++) {
			GthShortcut *shortcut_info = g_ptr_array_index (shortcuts_v, i);
			GtkWidget   *shortcut;

			if (g_strcmp0 (shortcut_info->category, GTH_SHORTCUT_CATEGORY_HIDDEN) == 0)
				continue;

			if ((shortcut_info->context & context->id) == 0)
				continue;

			if ((shortcut_info->keyval == 0) && ((shortcut_info->context & GTH_SHORTCUT_CONTEXT_DOC) == 0))
				continue;

			if (g_strcmp0 (shortcut_info->category, last_category) != 0) {
				GthShortcutCategory *category_info;
				char                *title;

				last_category = shortcut_info->category;

				category_info = gth_main_get_shortcut_category (shortcut_info->category);
				if ((category_info != NULL) && (category_info->display_name != NULL))
					title = _(category_info->display_name);
				else
					title = _("Other");

				group = g_object_new (GTK_TYPE_SHORTCUTS_GROUP,
						      "title", title,
						      NULL);
				gtk_widget_show (group);
				gtk_container_add (GTK_CONTAINER (section), group);
			}

			shortcut = g_object_new (GTK_TYPE_SHORTCUTS_SHORTCUT,
						 "title", _(shortcut_info->description),
						 "accelerator", shortcut_info->accelerator,
						 NULL);
			gtk_widget_show (shortcut);
			gtk_container_add (GTK_CONTAINER (group), shortcut);
		}
	}

	switch (gth_window_get_current_page (app_window)) {
	case GTH_BROWSER_PAGE_BROWSER:
		g_object_set (window, "section-name", "browser", NULL);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		g_object_set (window, "section-name", "viewer", NULL);
		break;
	}

	gtk_widget_show (window);
}
