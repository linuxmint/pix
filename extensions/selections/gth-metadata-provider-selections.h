/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Pix
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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

#ifndef GTH_METADATA_PROVIDER_SELECTIONS_H
#define GTH_METADATA_PROVIDER_SELECTIONS_H

#include <glib.h>
#include <glib-object.h>
#include <pix.h>

#define GTH_TYPE_METADATA_PROVIDER_SELECTIONS         (gth_metadata_provider_selections_get_type ())
#define GTH_METADATA_PROVIDER_SELECTIONS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_METADATA_PROVIDER_SELECTIONS, GthMetadataProviderSelections))
#define GTH_METADATA_PROVIDER_SELECTIONS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_METADATA_PROVIDER_SELECTIONS, GthMetadataProviderSelectionsClass))
#define GTH_IS_METADATA_PROVIDER_SELECTIONS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_METADATA_PROVIDER_SELECTIONS))
#define GTH_IS_METADATA_PROVIDER_SELECTIONS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_METADATA_PROVIDER_SELECTIONS))
#define GTH_METADATA_PROVIDER_SELECTIONS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_METADATA_PROVIDER_SELECTIONS, GthMetadataProviderSelectionsClass))

typedef struct _GthMetadataProviderSelections         GthMetadataProviderSelections;
typedef struct _GthMetadataProviderSelectionsClass    GthMetadataProviderSelectionsClass;

struct _GthMetadataProviderSelections
{
	GthMetadataProvider __parent;
};

struct _GthMetadataProviderSelectionsClass
{
	GthMetadataProviderClass __parent_class;
};

GType gth_metadata_provider_selections_get_type (void) G_GNUC_CONST;

#endif /* GTH_METADATA_PROVIDER_SELECTIONS_H */
