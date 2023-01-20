/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
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
#include "file-tools-enum-types.h"
#include "gth-curve-preset.h"


/* Signals */
enum {
	CHANGED,
	PRESET_CHANGED,
	LAST_SIGNAL
};


typedef struct {
	GthPoints	 points[GTH_HISTOGRAM_N_CHANNELS];
	int		 id;
	char		*name;
} Preset;


struct _GthCurvePresetPrivate {
	GFile *file;
	GList *set;
	int    next_id;
};


static guint gth_curve_preset_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthCurvePreset,
			 gth_curve_preset,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthCurvePreset))


static char * Channel_Names[GTH_HISTOGRAM_N_CHANNELS] = { "value", "red", "green", "blue", "alpha" };


static int
get_channel_number (const char *name)
{
	int i;

	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS; i++)
		if (g_strcmp0 (Channel_Names[i], name) == 0)
			return i;

	return -1;
}


static Preset *
preset_new (int id)
{
	Preset *preset;
	int     c;

	preset = g_new (Preset, 1);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		gth_points_init (&preset->points[c], 0);
	preset->id = id;
	preset->name = NULL;

	return preset;
}


static DomElement *
preset_create_element (Preset      *preset,
		       DomDocument *doc)
{
	DomElement *element;
	int         c;

	element = dom_document_create_element (doc, "preset", "name", preset->name, NULL);

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		DomElement *channel;
		int         i;

		channel = dom_document_create_element (doc, "channel", "type", Channel_Names[c], NULL);
		for (i = 0; i < preset->points[c].n; i++) {
			GthPoint *p = preset->points[c].p + i;
			char     *x;
			char     *y;

			x = g_strdup_printf ("%d", (int) p->x);
			y = g_strdup_printf ("%d", (int) p->y);
			dom_element_append_child (channel, dom_document_create_element (doc, "point", "x", x, "y", y, NULL));

			g_free (x);
			g_free (y);
		}
		dom_element_append_child (element, channel);
	}

	return element;
}


static void
preset_load_from_element (Preset     *preset,
			  DomElement *element)
{
	int         c;
	DomElement *node;

	g_return_if_fail (element != NULL);
	g_return_if_fail (g_strcmp0 (element->tag_name, "preset") == 0);

	g_free (preset->name);
	preset->name = g_strdup (dom_element_get_attribute (element, "name"));

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		gth_points_dispose (&preset->points[c]);

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "channel") == 0) {
			int n_channel;

			n_channel = get_channel_number (dom_element_get_attribute (node, "type"));
			if (n_channel > -1) {
				DomElement *point;

				for (point = node->first_child; point; point = point->next_sibling) {
					if (g_strcmp0 (point->tag_name, "point") == 0) {
						const char *sx, *sy;
						int         x, y;

						sx = dom_element_get_attribute (point, "x");
						sy = dom_element_get_attribute (point, "y");

						if (sscanf (sx, "%d", &x) != 1)
							continue;
						if (sscanf (sy, "%d", &y) != 1)
							continue;

						gth_points_add_point (&preset->points[n_channel], x, y);
					}
				}
			}
		}
	}
}


static void
preset_free (Preset *preset)
{
	int c;

	g_return_if_fail (preset != NULL);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		gth_points_dispose (&preset->points[c]);
	g_free (preset->name);
	g_free (preset);
}


static void
gth_curve_preset_finalize (GObject *obj)
{
	GthCurvePreset *self;

	self = GTH_CURVE_PRESET (obj);
	g_list_free_full (self->priv->set, (GDestroyNotify) preset_free);
	_g_object_unref (self->priv->file);

	G_OBJECT_CLASS (gth_curve_preset_parent_class)->finalize (obj);
}


static void
gth_curve_preset_class_init (GthCurvePresetClass *klass)
{
	GObjectClass   *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_curve_preset_finalize;

	/* signals */

	gth_curve_preset_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthCurvePresetClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
	gth_curve_preset_signals[PRESET_CHANGED] =
                g_signal_new ("preset-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthCurvePresetClass, preset_changed),
                              NULL, NULL,
                              gth_marshal_VOID__ENUM_INT,
                              G_TYPE_NONE,
                              2,
			      GTH_TYPE_PRESET_ACTION,
			      G_TYPE_INT);
}


static void
gth_curve_preset_changed (GthCurvePreset *self)
{
	g_signal_emit (self, gth_curve_preset_signals[CHANGED], 0);
}


static void
gth_curve_preset_init (GthCurvePreset *self)
{
	self->priv = gth_curve_preset_get_instance_private (self);
	self->priv->set = NULL;
	self->priv->file = NULL;
	self->priv->next_id = 0;
}


GthCurvePreset *
gth_curve_preset_new_from_file (GFile *file)
{
	GthCurvePreset *self;
	DomDocument    *doc;
	void           *buffer;
	gsize           size;

	self = (GthCurvePreset *) g_object_new (GTH_TYPE_CURVE_PRESET, NULL);
	self->priv->file = g_file_dup (file);

	doc = dom_document_new ();
	if (_g_file_load_in_buffer (self->priv->file, &buffer, &size, NULL, NULL)) {
		if (dom_document_load (doc, buffer, size, NULL)) {
			DomElement *presets = DOM_ELEMENT (doc)->first_child;

			if ((presets != NULL) && (g_strcmp0 (presets->tag_name, "presets") == 0)) {
				DomElement *node;

				for (node = presets->first_child; node; node = node->next_sibling) {
					if (g_strcmp0 (node->tag_name, "preset") == 0) {
						Preset *preset;

						preset = preset_new (self->priv->next_id++);
						preset_load_from_element (preset, node);
						self->priv->set = g_list_append (self->priv->set, preset);
					}
				}
			}
		}
		g_free (buffer);
	}

	g_object_unref (doc);

	return self;
}


int
gth_curve_preset_get_size (GthCurvePreset  *self)
{
	return g_list_length (self->priv->set);
}


gboolean
gth_curve_preset_get_nth (GthCurvePreset  *self,
			  int		   n,
			  int		  *id,
			  const char	 **name,
			  GthPoints	 **points)
{
	Preset *preset;

	preset = g_list_nth_data (self->priv->set, n);
	if (preset == NULL)
		return FALSE;

	if (id) *id = preset->id;
	if (name) *name = preset->name;
	if (points) *points = preset->points;

	return TRUE;
}


gboolean
gth_curve_preset_get_by_id (GthCurvePreset	 *self,
			    int			  id,
			    const char		**name,
			    GthPoints		**points)
{
	GList *scan;

	for (scan = self->priv->set; scan; scan = scan->next) {
		Preset *preset = scan->data;

		if (preset->id == id) {
			if (name) *name = preset->name;
			if (points) *points = preset->points;
			return TRUE;
		}
	}

	return FALSE;
}


int
gth_curve_preset_get_pos (GthCurvePreset  *self,
			  int		  id)
{
	GList *scan;
	int    pos;

	for (pos = 0, scan = self->priv->set; scan; scan = scan->next, pos++) {
		Preset *preset = scan->data;

		if (preset->id == id)
			return pos;
	}

	return -1;
}


int
gth_curve_preset_add (GthCurvePreset	 *self,
		      const char	 *name,
		      GthPoints		*points)
{
	Preset *preset;
	int     c;

	preset = preset_new (self->priv->next_id++);
	preset->name = g_strdup (name);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		gth_points_copy (points + c, &preset->points[c]);
	self->priv->set = g_list_append (self->priv->set, preset);
	gth_curve_preset_changed (self);
	g_signal_emit (self, gth_curve_preset_signals[PRESET_CHANGED], 0, GTH_PRESET_ACTION_ADDED, preset->id);

	return preset->id;
}


static int
compare_preset_by_id (Preset   *a,
		      gpointer  id_p)
{
	int id = GPOINTER_TO_INT (id_p);
	return (a->id == id) ? 0 : (a->id > id) ? 1 : -1;
}


void
gth_curve_preset_remove (GthCurvePreset  *self,
			 int              id)
{
	GList  *link;

	link = g_list_find_custom (self->priv->set, GINT_TO_POINTER (id), (GCompareFunc) compare_preset_by_id);
	if (link == NULL)
		return;

	self->priv->set = g_list_remove_link (self->priv->set, link);
	gth_curve_preset_changed (self);
	g_signal_emit (self, gth_curve_preset_signals[PRESET_CHANGED], 0, GTH_PRESET_ACTION_REMOVED, id);

	g_list_free_full (link, (GDestroyNotify) preset_free);
}


void
gth_curve_preset_rename (GthCurvePreset  *self,
			 int              id,
			 const char      *new_name)
{
	GList  *link;
	Preset *preset;

	link = g_list_find_custom (self->priv->set, GINT_TO_POINTER (id), (GCompareFunc) compare_preset_by_id);
	g_return_if_fail (link != NULL);

	preset = link->data;
	g_free (preset->name);
	preset->name = g_strdup (new_name);

	gth_curve_preset_changed (self);
	g_signal_emit (self, gth_curve_preset_signals[PRESET_CHANGED], 0, GTH_PRESET_ACTION_RENAMED, id);
}


void
gth_curve_preset_change_order (GthCurvePreset  *self,
			       GList	       *id_list)
{
	GList *set, *scan;

	set = NULL;
	for (scan = id_list; scan; scan = scan->next) {
		int    id = GPOINTER_TO_INT (scan->data);
		GList *link;

		link = g_list_find_custom (self->priv->set, GINT_TO_POINTER (id), (GCompareFunc) compare_preset_by_id);
		g_return_if_fail (link != NULL);

		set = g_list_prepend (set, link->data);
	}
	set = g_list_reverse (set);

	g_list_free (self->priv->set);
	self->priv->set = set;

	gth_curve_preset_changed (self);
	g_signal_emit (self, gth_curve_preset_signals[PRESET_CHANGED], 0, GTH_PRESET_ACTION_CHANGED_ORDER, -1);
}


GList *
gth_curve_preset_get_order (GthCurvePreset  *self)
{
	GList *id_list, *scan;

	id_list = NULL;
	for (scan = self->priv->set; scan; scan = scan->next) {
		Preset *preset = scan->data;
		id_list = g_list_prepend (id_list, GINT_TO_POINTER (preset->id));
	}

	return g_list_reverse (id_list);
}


gboolean
gth_curve_preset_save (GthCurvePreset	 *self,
		       GError		**error)
{
	DomDocument *doc;
	DomElement  *curves;
	GList       *scan;
	char        *buffer;
	gsize        size;
	gboolean     result;

	g_return_val_if_fail (self->priv->file != NULL, FALSE);

	doc = dom_document_new ();
	curves = dom_document_create_element (doc, "presets", NULL);
	for (scan = self->priv->set; scan; scan = scan->next) {
		Preset *preset = scan->data;
		dom_element_append_child (curves, preset_create_element (preset, doc));
	}
	dom_element_append_child (DOM_ELEMENT (doc), curves);

	buffer = dom_document_dump (doc, &size);
	result = _g_file_write (self->priv->file, FALSE, G_FILE_CREATE_NONE, buffer, size, NULL, error);

	g_free (buffer);
	g_object_unref (doc);

	return result;
}
