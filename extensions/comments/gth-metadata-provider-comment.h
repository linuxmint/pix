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

#ifndef GTH_METADATA_PROVIDER_COMMENT_H
#define GTH_METADATA_PROVIDER_COMMENT_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

#define GTH_TYPE_METADATA_PROVIDER_COMMENT         (gth_metadata_provider_comment_get_type ())
#define GTH_METADATA_PROVIDER_COMMENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_METADATA_PROVIDER_COMMENT, GthMetadataProviderComment))
#define GTH_METADATA_PROVIDER_COMMENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_METADATA_PROVIDER_COMMENT, GthMetadataProviderCommentClass))
#define GTH_IS_METADATA_PROVIDER_COMMENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_METADATA_PROVIDER_COMMENT))
#define GTH_IS_METADATA_PROVIDER_COMMENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_METADATA_PROVIDER_COMMENT))
#define GTH_METADATA_PROVIDER_COMMENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_METADATA_PROVIDER_COMMENT, GthMetadataProviderCommentClass))

typedef struct _GthMetadataProviderComment         GthMetadataProviderComment;
typedef struct _GthMetadataProviderCommentClass    GthMetadataProviderCommentClass;
typedef struct _GthMetadataProviderCommentPrivate  GthMetadataProviderCommentPrivate;

struct _GthMetadataProviderComment
{
	GthMetadataProvider __parent;
	GthMetadataProviderCommentPrivate *priv;
};

struct _GthMetadataProviderCommentClass
{
	GthMetadataProviderClass __parent_class;
};

GType gth_metadata_provider_comment_get_type (void) G_GNUC_CONST;

#endif /* GTH_METADATA_PROVIDER_COMMENT_H */
