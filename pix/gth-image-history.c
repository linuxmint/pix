/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2009 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-image-history.h"


#define MAX_UNDO_HISTORY_LEN 5


/* GthImageData */


GthImageData *
gth_image_data_new (GthImage *image,
		    int       requested_size,
		    gboolean  unsaved)
{
	GthImageData *idata;

	g_return_val_if_fail (image != NULL, NULL);

	idata = g_new0 (GthImageData, 1);

	idata->ref = 1;
	idata->image = _g_object_ref (image);
	idata->requested_size = requested_size;
	idata->unsaved = unsaved;

	return idata;
}


GthImageData *
gth_image_data_ref (GthImageData *idata)
{
	g_return_val_if_fail (idata != NULL, NULL);
	idata->ref++;
	return idata;
}


void
gth_image_data_unref (GthImageData *idata)
{
	g_return_if_fail (idata != NULL);

	idata->ref--;
	if (idata->ref == 0) {
		_g_object_unref (idata->image);
		g_free (idata);
	}
}


void
gth_image_data_list_free (GList *list)
{
	if (list == NULL)
		return;
	g_list_foreach (list, (GFunc) gth_image_data_unref, NULL);
	g_list_free (list);
}


/* GthImageHistory */


enum {
	CHANGED,
	LAST_SIGNAL
};


static guint gth_image_history_signals[LAST_SIGNAL] = { 0 };


struct _GthImageHistoryPrivate {
	GList *undo_history;  /* GthImageData items */
	GList *redo_history;  /* GthImageData items */
};


G_DEFINE_TYPE_WITH_CODE (GthImageHistory,
			 gth_image_history,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImageHistory))


static void
gth_image_history_finalize (GObject *object)
{
	GthImageHistory *history;

	g_return_if_fail (GTH_IS_IMAGE_HISTORY (object));
	history = GTH_IMAGE_HISTORY (object);

	gth_image_history_clear (history);

	G_OBJECT_CLASS (gth_image_history_parent_class)->finalize (object);
}


static void
gth_image_history_class_init (GthImageHistoryClass *class)
{
	GObjectClass *object_class;

	gth_image_history_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageHistoryClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_history_finalize;
}


static void
gth_image_history_init (GthImageHistory *history)
{
	history->priv = gth_image_history_get_instance_private (history);
	history->priv->undo_history = NULL;
	history->priv->redo_history = NULL;
}


GthImageHistory *
gth_image_history_new (void)
{
	return (GthImageHistory *) g_object_new (GTH_TYPE_IMAGE_HISTORY, NULL);
}


static GthImageData*
remove_first_image (GList **list)
{
	GList        *head;
	GthImageData *idata;

	if (*list == NULL)
		return NULL;

	head = *list;
	*list = g_list_remove_link (*list, head);
	idata = head->data;
	g_list_free (head);

	return idata;
}


static GList*
add_image_to_list (GList           *list,
		   GthImage	   *image,
		   int              requested_size,
		   gboolean         unsaved)
{
	if (g_list_length (list) > MAX_UNDO_HISTORY_LEN) {
		GList *last;

		last = g_list_nth (list, MAX_UNDO_HISTORY_LEN - 1);
		if (last->prev != NULL) {
			last->prev->next = NULL;
			gth_image_data_list_free (last);
		}
	}

	if (image == NULL)
		return list;

	return g_list_prepend (list, gth_image_data_new (image, requested_size, unsaved));
}


static void
add_image_to_undo_history (GthImageHistory *history,
			   GthImage        *image,
			   int              requested_size,
		   	   gboolean         unsaved)
{
	history->priv->undo_history = add_image_to_list (history->priv->undo_history,
							 image,
							 requested_size,
							 unsaved);
}


static void
add_image_to_redo_history (GthImageHistory *history,
			   GthImage        *image,
			   int              requested_size,
	   		   gboolean         unsaved)
{
	history->priv->redo_history = add_image_to_list (history->priv->redo_history,
							 image,
							 requested_size,
							 unsaved);
}


void
gth_image_history_add_image (GthImageHistory *history,
			     GthImage        *image,
			     int              requested_size,
			     gboolean         unsaved)
{
	add_image_to_undo_history (history, image, requested_size, unsaved);
	gth_image_data_list_free (history->priv->redo_history);
	history->priv->redo_history = NULL;

	g_signal_emit (G_OBJECT (history),
		       gth_image_history_signals[CHANGED],
		       0);
}


void
gth_image_history_add_surface (GthImageHistory *history,
			       cairo_surface_t *surface,
			       int              requested_size,
			       gboolean         unsaved)
{
	GthImage *image;

	image = gth_image_new_for_surface (surface);
	gth_image_history_add_image (history, image, requested_size, unsaved);

	g_object_unref (image);
}


GthImageData *
gth_image_history_undo (GthImageHistory *history)
{
	GthImageData *idata;

	if ((history->priv->undo_history == NULL) || (history->priv->undo_history->next == NULL))
		return NULL;

	idata = remove_first_image (&(history->priv->undo_history));
	add_image_to_redo_history (history, idata->image, idata->requested_size, idata->unsaved);
	gth_image_data_unref (idata);

	g_signal_emit (G_OBJECT (history),
		       gth_image_history_signals[CHANGED],
		       0);

	return (GthImageData *) history->priv->undo_history->data;
}


GthImageData *
gth_image_history_redo (GthImageHistory *history)
{
	GthImageData *idata;

	if (history->priv->redo_history == NULL)
		return NULL;

	idata = remove_first_image (&(history->priv->redo_history));
	add_image_to_undo_history (history, idata->image, idata->requested_size, idata->unsaved);
	gth_image_data_unref (idata);

	g_signal_emit (G_OBJECT (history),
		       gth_image_history_signals[CHANGED],
		       0);

	return (GthImageData *) history->priv->undo_history->data;
}


void
gth_image_history_clear (GthImageHistory *history)
{
	gth_image_data_list_free (history->priv->undo_history);
	history->priv->undo_history = NULL;

	gth_image_data_list_free (history->priv->redo_history);
	history->priv->redo_history = NULL;
}


gboolean
gth_image_history_can_undo (GthImageHistory *history)
{
	return (history->priv->undo_history != NULL) && (history->priv->undo_history->next != NULL);
}


gboolean
gth_image_history_can_redo (GthImageHistory *history)
{
	return history->priv->redo_history != NULL;
}


GthImageData *
gth_image_history_revert (GthImageHistory *history)
{
	GthImageData *last_saved = NULL;
	GList        *scan;

	for (scan = history->priv->undo_history; scan; scan = scan->next) {
		GthImageData *idata = scan->data;

		if (idata->unsaved)
			continue;

		last_saved = gth_image_data_ref (idata);
	}

	gth_image_history_clear (history);

	return last_saved;
}


GthImageData *
gth_image_history_get_last (GthImageHistory *history)
{
	if (history->priv->undo_history == NULL)
		return NULL;
	else
		return (GthImageData *) history->priv->undo_history->data;
}
