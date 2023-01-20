/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright © 2001-2011 Free Software Foundation, Inc.
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

#ifndef GTH_PREF_H
#define GTH_PREF_H

#include <glib.h>
#include <gtk/gtk.h>
#include "typedefs.h"

G_BEGIN_DECLS

/* schemas */

#define GTHUMB_SCHEMA                         "org.gnome.gthumb"
#define GTHUMB_GENERAL_SCHEMA                 GTHUMB_SCHEMA ".general"
#define GTHUMB_DATA_MIGRATION_SCHEMA          GTHUMB_SCHEMA ".data-migration"
#define GTHUMB_BROWSER_SCHEMA                 GTHUMB_SCHEMA ".browser"
#define GTHUMB_DIALOGS_SCHEMA                 GTHUMB_SCHEMA ".dialogs"
#define GTHUMB_MESSAGES_SCHEMA                GTHUMB_DIALOGS_SCHEMA ".messages"
#define GTHUMB_ADD_TO_CATALOG_SCHEMA          GTHUMB_DIALOGS_SCHEMA ".add-to-catalog"
#define GTHUMB_SAVE_FILE_SCHEMA               GTHUMB_DIALOGS_SCHEMA ".save-file"

/* keys: general */

#define PREF_GENERAL_ACTIVE_EXTENSIONS        "active-extensions"
#define PREF_GENERAL_STORE_METADATA_IN_FILES  "store-metadata-in-files"

/* keys: dada migration */

#define PREF_DATA_MIGRATION_CATALOGS_2_10     "catalogs-2-10"

/* keys: browser */

#define PREF_BROWSER_GO_TO_LAST_LOCATION      "go-to-last-location"
#define PREF_BROWSER_USE_STARTUP_LOCATION     "use-startup-location"
#define PREF_BROWSER_STARTUP_LOCATION         "startup-location"
#define PREF_BROWSER_STARTUP_CURRENT_FILE     "startup-current-file"
#define PREF_BROWSER_GENERAL_FILTER           "general-filter"
#define PREF_BROWSER_SHOW_HIDDEN_FILES        "show-hidden-files"
#define PREF_BROWSER_FAST_FILE_TYPE           "fast-file-type"
#define PREF_BROWSER_SAVE_THUMBNAILS          "save-thumbnails"
#define PREF_BROWSER_THUMBNAIL_SIZE           "thumbnail-size"
#define PREF_BROWSER_THUMBNAIL_LIMIT          "thumbnail-limit"
#define PREF_BROWSER_THUMBNAIL_CAPTION        "thumbnail-caption"
#define PREF_BROWSER_SINGLE_CLICK_ACTIVATION  "single-click-activation"
#define PREF_BROWSER_OPEN_FILES_IN_FULLSCREEN "open-files-in-fullscreen"
#define PREF_BROWSER_SORT_TYPE                "sort-type"
#define PREF_BROWSER_SORT_INVERSE             "sort-inverse"
#define PREF_BROWSER_WINDOW_WIDTH             "window-width"
#define PREF_BROWSER_WINDOW_HEIGHT            "window-height"
#define PREF_BROWSER_WINDOW_MAXIMIZED         "maximized"
#define PREF_BROWSER_STATUSBAR_VISIBLE        "statusbar-visible"
#define PREF_BROWSER_SIDEBAR_VISIBLE          "sidebar-visible"
#define PREF_BROWSER_SIDEBAR_SECTIONS         "sidebar-sections"
#define PREF_BROWSER_PROPERTIES_VISIBLE       "properties-visible"
#define PREF_BROWSER_PROPERTIES_ON_THE_RIGHT  "properties-on-the-right"
#define PREF_BROWSER_THUMBNAIL_LIST_VISIBLE   "thumbnail-list-visible"
#define PREF_BROWSER_BROWSER_SIDEBAR_WIDTH    "browser-sidebar-width"
#define PREF_BROWSER_VIEWER_SIDEBAR           "viewer-sidebar"
#define PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT "viewer-thumbnails-orientation"
#define PREF_BROWSER_REUSE_ACTIVE_WINDOW      "reuse-active-window"
#define PREF_FULLSCREEN_THUMBNAILS_VISIBLE    "fullscreen-thumbnails-visible"
#define PREF_FULLSCREEN_SIDEBAR		      "fullscreen-sidebar"
#define PREF_VIEWER_SCROLL_ACTION             "scroll-action"
#define PREF_BROWSER_FAVORITE_PROPERTIES      "favorite-properties"

/* keys: add to catalog */

#define PREF_ADD_TO_CATALOG_LAST_CATALOG      "last-catalog"
#define PREF_ADD_TO_CATALOG_VIEW              "view"

/* keys: save file */

#define PREF_SAVE_FILE_SHOW_OPTIONS           "show-options"

/* keys: messages */

#define PREF_MSG_CANNOT_MOVE_TO_TRASH         "cannot-move-to-trash"
#define PREF_MSG_SAVE_MODIFIED_IMAGE          "save-modified-image"
#define PREF_MSG_CONFIRM_DELETION             "confirm-deletion"

/* foreign schemas */

#define GNOME_DESKTOP_BACKGROUND_SCHEMA       "org.gnome.desktop.background"
#define PREF_BACKGROUND_PICTURE_URI           "picture-uri"
#define PREF_BACKGROUND_PICTURE_OPTIONS       "picture-options"

#define GNOME_DESKTOP_INTERFACE_SCHEMA        "org.gnome.desktop.interface"

/* utility functions */

void             gth_pref_initialize              (void);
void             gth_pref_release                 (void);
void             gth_pref_set_startup_location    (const char *location);
const char *     gth_pref_get_startup_location    (void);
const char *     gth_pref_get_wallpaper_filename  (void);
const char *     gth_pref_get_wallpaper_options   (void);
void             gth_pref_save_window_geometry    (GtkWindow  *window,
                               	       	       	   const char *schema);
void             gth_pref_restore_window_geometry (GtkWindow  *window,
						   const char *schema);

G_END_DECLS

#endif /* GTH_PREF_H */
