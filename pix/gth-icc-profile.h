/* -*- Mode: C; tab-width: 8;	 indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 The Free Software Foundation, Inc.
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

#ifndef GTH_ICC_PROFILE_H
#define GTH_ICC_PROFILE_H

#include <glib.h>

G_BEGIN_DECLS

typedef gpointer GthCMSProfile;
typedef gpointer GthCMSTransform;

#define GTH_ICC_PROFILE_ID_UNKNOWN "unknown://"
#define GTH_ICC_PROFILE_WITH_MD5 "md5://"
#define GTH_ICC_PROFILE_FROM_PROPERTY "property://"

#define GTH_TYPE_ICC_PROFILE            (gth_icc_profile_get_type ())
#define GTH_ICC_PROFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ICC_PROFILE, GthICCProfile))
#define GTH_ICC_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ICC_PROFILE, GthICCProfileClass))
#define GTH_IS_ICC_PROFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ICC_PROFILE))
#define GTH_IS_ICC_PROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ICC_PROFILE))
#define GTH_ICC_PROFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_ICC_PROFILE, GthICCProfileClass))

#define GTH_TYPE_ICC_TRANSFORM            (gth_icc_transform_get_type ())
#define GTH_ICC_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ICC_TRANSFORM, GthICCTransform))
#define GTH_ICC_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ICC_TRANSFORM, GthICCTransformClass))
#define GTH_IS_ICC_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ICC_TRANSFORM))
#define GTH_IS_ICC_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ICC_TRANSFORM))
#define GTH_ICC_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_ICC_TRANSFORM, GthICCTransformClass))

typedef struct _GthICCProfile         GthICCProfile;
typedef struct _GthICCProfilePrivate  GthICCProfilePrivate;
typedef struct _GthICCProfileClass    GthICCProfileClass;

typedef struct _GthICCTransform         GthICCTransform;
typedef struct _GthICCTransformPrivate  GthICCTransformPrivate;
typedef struct _GthICCTransformClass    GthICCTransformClass;

struct _GthICCProfile {
	GObject __parent;
	GthICCProfilePrivate *priv;
};

struct _GthICCProfileClass {
	GObjectClass __parent_class;
};

struct _GthICCTransform {
	GObject __parent;
	GthICCTransformPrivate *priv;
};

struct _GthICCTransformClass {
	GObjectClass __parent_class;
};

void			gth_cms_profile_free		(GthCMSProfile	  profile);
void			gth_cms_transform_free		(GthCMSTransform  transform);

GType			gth_icc_profile_get_type	(void);
GthICCProfile *		gth_icc_profile_new		(const char	 *id,
						 	 GthCMSProfile	  profile);
GthICCProfile *		gth_icc_profile_new_srgb	(void);
const char *		gth_icc_profile_get_id		(GthICCProfile	 *icc_profile);
char *                  gth_icc_profile_get_description	(GthICCProfile	 *icc_profile);
gboolean                gth_icc_profile_id_is_unknown   (const char      *id);
GthCMSProfile		gth_icc_profile_get_profile	(GthICCProfile	 *icc_profile);
gboolean		gth_icc_profile_equal		(GthICCProfile	 *a,
		    	    	    	    	 	 GthICCProfile	 *b);

GType			gth_icc_transform_get_type	(void);
GthICCTransform * 	gth_icc_transform_new		(GthCMSTransform  transform);
GthICCTransform *	gth_icc_transform_new_from_profiles
							(GthICCProfile   *from_profile,
							 GthICCProfile   *to_profile);
GthCMSTransform         gth_icc_transform_get_transform (GthICCTransform *transform);

G_END_DECLS

#endif /* GTH_ICC_PROFILE_H */
