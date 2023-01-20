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
#include <gth-file-tool-color-picker.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "callbacks.h"
#include "gth-file-tool-adjust-colors.h"
#include "gth-file-tool-adjust-contrast.h"
#include "gth-file-tool-color-picker.h"
#include "gth-file-tool-crop.h"
#include "gth-file-tool-curves.h"
#include "gth-file-tool-effects.h"
#include "gth-file-tool-flip.h"
#include "gth-file-tool-grayscale.h"
#include "gth-file-tool-lomo.h"
#include "gth-file-tool-mirror.h"
#include "gth-file-tool-negative.h"
#include "gth-file-tool-redo.h"
#include "gth-file-tool-resize.h"
#include "gth-file-tool-rotate.h"
#include "gth-file-tool-rotate-left.h"
#include "gth-file-tool-rotate-right.h"
#include "gth-file-tool-save.h"
#include "gth-file-tool-save-as.h"
#include "gth-file-tool-sharpen.h"
#include "gth-file-tool-undo.h"


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	int p;

	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_SAVE);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_SAVE_AS);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_UNDO);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_REDO);

	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_ADJUST_CONTRAST);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_ADJUST_COLORS);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_SHARPEN);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_GRAYSCALE);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_CURVES);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_EFFECTS);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_COLOR_PICKER);

	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_ROTATE_LEFT);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_ROTATE_RIGHT);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_MIRROR);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_FLIP);

	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_ROTATE);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_RESIZE);
	gth_main_register_type ("file-tools", GTH_TYPE_FILE_TOOL_CROP);

	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (file_tools__gth_browser_construct_cb), NULL);

	/**
	 * Add a filter to the filter list shown in the Effects tool
	 *
	 * @filter_grid (GthFilterGrid*): the filter grid to add the effect to.
	 **/
	gth_hook_register ("add-special-effect", 1);

	p = 10;
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (desert_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (cherry_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (soil_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (artic_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (fresh_blue_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (mangos_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (warmer_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (cooler_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (vintage_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (lomo_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (vignette_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (blurred_edges_add_to_special_effects), NULL);
	gth_hook_add_callback ("add-special-effect", p++, G_CALLBACK (negative_add_to_special_effects), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
