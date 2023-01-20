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


#include <config.h>
#include <glib/gi18n.h>
#include "gth-hook.h"
#include "gth-main.h"


void
gth_main_register_default_hooks (void)
{
	gth_hooks_initialize ();

	/**
	 * Called at start up time to do basic initialization.
	 *
	 * no arguments.
	 **/
	gth_hook_register ("initialize", 0);

	/**
	 * Called after the window has been initialized.  Can be used by
	 * an extension to create and attach specific data to the window.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-construct", 1);

	/**
	 * Called in an idle callback after the window has been initialized.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-construct-idle-callback", 1);

	/**
	 * Called when the window is realized.
	 *
	 * @browser (GthBrowser*): the window.
	 **/
	gth_hook_register ("gth-browser-realize", 1);

	/**
	 * Called when the window is unrealized.
	 *
	 * @browser (GthBrowser*): the window.
	 **/
	gth_hook_register ("gth-browser-unrealize", 1);

	/**
	 * Called before closing a window.
	 *
	 * @browser (GthBrowser*): the window to be closed.
	 **/
	gth_hook_register ("gth-browser-close", 1);

	/**
	 * Called before closing the last window and after the
	 * 'gth-browser-close' hook functions.
	 *
	 * @browser (GthBrowser*): the window to be closed.
	 **/
	gth_hook_register ("gth-browser-close-last-window", 1);

	/**
	 *  Called when a sensitivity update is requested.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-update-sensitivity", 1);

	/**
	 *  Called when the file list selection changes
	 *
	 *  @browser (GthBrowser*): the relative window.
	 *  @n_selected (int): number of selected files.
	 */
	gth_hook_register ("gth-browser-selection-changed", 2);

	/**
	 *  Called when the current page changes.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-set-current-page", 1);

	/**
	 *  Called when after the activation of a viewer page
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-activate-viewer-page", 1);

	/**
	 *  Called before the deactivation of a viewer page.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-deactivate-viewer-page", 1);

	/**
	 * Called before loading a folder.
	 *
	 * @browser (GthBrowser*): the window
	 * @folder (GFile*): the folder to load
	 **/
	gth_hook_register ("gth-browser-load-location-before", 2);

	/**
	 * Called after the folder has been loaded.
	 *
	 * @browser (GthBrowser*): the window
	 * @folder (GthFileData*): the loaded folder data
	 **/
	gth_hook_register ("gth-browser-load-location-after", 2);

	/**
	 * Called before displaying the file list popup menu.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-file-list-popup-before", 1);

	/**
	 * Called before displaying the file popup menu.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-file-popup-before", 1);

	/**
	 * Called before displaying the folder tree popup menu.
	 *
	 * @browser (GthBrowser*): the relative window.
	 * @file_source (GthFileSource*): the file_source relative to the file.
	 * @file (GFile*): the relative folder or NULL if the button was
	 * pressed in an empty area.
	 **/
	gth_hook_register ("gth-browser-folder-tree-popup-before", 3);

	/**
	 * Called after a drag-data-received event on the folder tree
	 *
	 * @browser (GthBrowser *): the relative window.
	 * @destination (GthFileData *): the drop destination.
	 * @file_list (GList *): the GFile list of the dropped files
	 * @action (GdkDragAction): the drag action
	 * pressed in an empty area.
	 **/
	gth_hook_register ("gth-browser-folder-tree-drag-data-received", 4);

	/**
	 * Called to view file
	 *
	 * @browser (GthBrowser*): the relative window.
	 * @file (GthFileData*): the file
	 **/
	gth_hook_register ("gth-browser-view-file", 2);

	/**
	 * Called after a key press in the file list
	 *
	 * @browser (GthBrowser*): the relative window.
	 * @event (GdkEventKey *); the key event.
	 */
	gth_hook_register ("gth-browser-file-list-key-press", 2);

	/**
	 * Called when the list extra widget needs to be updated.
	 *
	 * @browser (GthBrowser*): the relative window.
	 */
	gth_hook_register ("gth-browser-update-extra-widget", 1);

	/**
	 * Called when a file-renamed signal is received by a browser window.
	 *
	 * @browser (GthBrowser*): the window that received the signal.
	 * @file (GFile *): the file that was renamed.
	 * @new_file (GFile *): the new file.
	 */
	gth_hook_register ("gth-browser-file-renamed", 3);

	/**
	 * Called in gth_image_save_to_file
	 *
	 * @data (GthImageSaveData*):
	 **/
	gth_hook_register ("save-image", 1);

	/**
	 * Called when copying files in _g_copy_files_async with the
	 * GTH_FILE_COPY_ALL_METADATA flag activated and when deleting file
	 * with _g_file_list_delete.  Used to add sidecar files that contain
	 * file metadata.
	 *
	 * @file (GFile *): the original file.
	 * @sidecar_sources (GList **): the sidecars list.
	 */
	gth_hook_register ("add-sidecars", 2);

	/**
	 * Called after a file metadata has been read.  Used to syncronize
	 * embedded metadata with the .comment file.
	 *
	 * @file_list (GList *): list of GthFileData
	 * @attributes (const char *): the attributes read for the file
	 */
	gth_hook_register ("read-metadata-ready", 2);

	/**
	 * Called to generate a thumbnail pixbuf for a file
	 *
	 * @uri (char *): the file uri
	 * @mime_type (char *): the file mime type
	 * @size (int): the requested thumbnail size
	 * @return (GdkPixbuf *): the thumbnail pixbuf
	 */
	gth_hook_register ("generate-thumbnail", 3);

	/**
	 * Called when creating the preferences dialog to add other tabs to
	 * the dialog.
	 *
	 * @dialog (GtkWidget *): the preferences dialog.
	 * @browser (GthBrowser*): the relative window.
	 * @builder (GtkBuilder*): the dialog ui builder.
	 **/
	gth_hook_register ("dlg-preferences-construct", 3);

	/**
	 * Called when to apply the changes from the preferences dialog.
	 *
	 * @dialog (GtkWidget *): the preferences dialog.
	 * @browser (GthBrowser*): the relative window.
	 * @builder (GtkBuilder*): the dialog ui builder.
	 **/
	gth_hook_register ("dlg-preferences-apply", 3);

	/**
	 * Called at start up time if the --import-photos argument is
	 * specified.
	 *
	 * @browser (GthBrowser*): the main window.
	 * @file (GFile *): import from this location
	 **/
	gth_hook_register ("import-photos", 2);

	/**
	 * Called at start up time if the --slideshow argument is
	 * specified.
	 *
	 * @browser (GthBrowser*): the main window.
	 **/
	gth_hook_register ("slideshow", 1);

	/**
	 * Called at start up time with the list of the command line
	 * files.
	 *
	 * @file_list (GList *): list of GFile with the files specified
	 * on the command line.
	 * @return (GFile *): the location where to open the window at or NULL
	 * for nothing.
	 **/
	gth_hook_register ("command-line-files", 1);
}
