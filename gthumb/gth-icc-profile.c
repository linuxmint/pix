/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

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

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#ifdef HAVE_LCMS2
#include <lcms2.h>
#endif /* HAVE_LCMS2 */
#include "gth-icc-profile.h"
#include "glib-utils.h"


struct _GthICCProfilePrivate {
	GthCMSProfile cms_profile;
	char *id;
};


struct _GthICCTransformPrivate {
	GthCMSTransform cms_transform;
};


G_DEFINE_TYPE_WITH_CODE (GthICCProfile,
			 gth_icc_profile,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthICCProfile))

G_DEFINE_TYPE_WITH_CODE (GthICCTransform,
			 gth_icc_transform,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthICCTransform))


void
gth_cms_profile_free (GthCMSProfile profile)
{
#ifdef HAVE_LCMS2
	if (profile != NULL)
		cmsCloseProfile ((cmsHPROFILE) profile);
#endif
}


void
gth_cms_transform_free (GthCMSTransform transform)
{
#ifdef HAVE_LCMS2
	if (transform != NULL)
		cmsDeleteTransform ((cmsHTRANSFORM) transform);
#endif
}


/* -- GthICCProfile -- */


static void
gth_icc_profile_finalize (GObject *object)
{
	GthICCProfile *icc_profile;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_ICC_PROFILE (object));

	icc_profile = GTH_ICC_PROFILE (object);
	gth_cms_profile_free (icc_profile->priv->cms_profile);
	g_free (icc_profile->priv->id);

	/* Chain up */
	G_OBJECT_CLASS (gth_icc_profile_parent_class)->finalize (object);
}


static void
gth_icc_profile_class_init (GthICCProfileClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_icc_profile_finalize;
}


static void
gth_icc_profile_init (GthICCProfile *self)
{
	self->priv = gth_icc_profile_get_instance_private (self);
	self->priv->cms_profile = NULL;
	self->priv->id = NULL;
}


static char *
_gth_cms_profile_create_id (GthCMSProfile cms_profile)
{
	GString  *str;
	gboolean  id_set = FALSE;

	str = g_string_new ("");

#ifdef HAVE_LCMS2
	{
		/* taken from colord:
		 * Copyright (C) 2013 Richard Hughes <richard@hughsie.com>
		 * Licensed under the GNU Lesser General Public License Version 2.1
		 * */
		cmsUInt8Number  icc_id[16];
		gboolean        md5_precooked = FALSE;
		gchar          *md5 = NULL;
		guint           i;

		cmsGetHeaderProfileID ((cmsHPROFILE) cms_profile, icc_id);
		for (i = 0; i < 16; i++) {
			if (icc_id[i] != 0) {
				md5_precooked = TRUE;
				break;
			}
		}
		if (md5_precooked) {
			/* convert to a hex string */
			md5 = g_new0 (gchar, 32 + 1);
			for (i = 0; i < 16; i++)
				g_snprintf (md5 + i * 2, 3, "%02x", icc_id[i]);
		}

		if (md5 != NULL) {
			g_string_append (str, GTH_ICC_PROFILE_WITH_MD5);
			g_string_append (str, md5);
			id_set = TRUE;
		}
	}
#endif

	if (! id_set)
		g_string_append (str, GTH_ICC_PROFILE_ID_UNKNOWN);

	return g_string_free (str, FALSE);
}


GthICCProfile *
gth_icc_profile_new (const char    *id,
		     GthCMSProfile  profile)
{
	GthICCProfile *icc_profile;

	g_return_val_if_fail (profile != NULL, NULL);

	icc_profile = g_object_new (GTH_TYPE_ICC_PROFILE, NULL);
	if (! gth_icc_profile_id_is_unknown (id))
		icc_profile->priv->id = g_strdup (id);
	else
		icc_profile->priv->id = _gth_cms_profile_create_id (profile);
	icc_profile->priv->cms_profile = profile;

	return icc_profile;
}


GthICCProfile *
gth_icc_profile_new_srgb (void)
{
#ifdef HAVE_LCMS2

	char          *id;
	GthCMSProfile  cmd_profile;
	GthICCProfile *icc_profile;

	id = g_strdup ("standard://srgb");
	cmd_profile = (GthCMSProfile) cmsCreate_sRGBProfile ();
	icc_profile = gth_icc_profile_new (id, cmd_profile);

	g_free (id);

	return icc_profile;

#else

	return NULL;

#endif
}


const char *
gth_icc_profile_get_id (GthICCProfile *self)
{
	g_return_val_if_fail (GTH_IS_ICC_PROFILE (self), NULL);
	return self->priv->id;
}


char *
gth_icc_profile_get_description	(GthICCProfile *self)
{
#ifdef HAVE_LCMS2

	GString         *color_profile;
	cmsHPROFILE      hProfile;
	const int        buffer_size = 128;
	wchar_t          buffer[buffer_size];
	cmsUInt32Number  size;
	char            *result;

	color_profile = g_string_new ("");
	hProfile = (cmsHPROFILE) gth_icc_profile_get_profile (self);
	size = cmsGetProfileInfo (hProfile, cmsInfoDescription, "en", "US", (wchar_t *) buffer, buffer_size);
	if (size > 0) {
		for (int i = 0; (i < size) && (buffer[i] != 0); i++)
			g_string_append_c (color_profile, buffer[i]);
	}

	result = NULL;
	if (color_profile->len > 0)
		result = _g_utf8_try_from_any (color_profile->str);

	g_string_free (color_profile, TRUE);

	return result;

#else

	return NULL;

#endif
}


gboolean
gth_icc_profile_id_is_unknown (const char *id)
{
	return (id == NULL) || (g_strcmp0 (id, GTH_ICC_PROFILE_ID_UNKNOWN) == 0);
}


GthCMSProfile
gth_icc_profile_get_profile (GthICCProfile *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->cms_profile;
}


gboolean
gth_icc_profile_equal (GthICCProfile *a,
		       GthICCProfile *b)
{
	g_return_val_if_fail ((a == NULL) || (b == NULL), FALSE);

	if (gth_icc_profile_id_is_unknown (a->priv->id) || gth_icc_profile_id_is_unknown (b->priv->id))
		return FALSE;
	else
		return g_strcmp0 (a->priv->id, b->priv->id) == 0;
}


/* -- GthICCTransform -- */


static void
gth_icc_transform_finalize (GObject *object)
{
	GthICCTransform *icc_transform;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_ICC_TRANSFORM (object));

	icc_transform = GTH_ICC_TRANSFORM (object);
	gth_cms_transform_free (icc_transform->priv->cms_transform);

	/* Chain up */
	G_OBJECT_CLASS (gth_icc_transform_parent_class)->finalize (object);
}


static void
gth_icc_transform_class_init (GthICCTransformClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_icc_transform_finalize;
}


static void
gth_icc_transform_init (GthICCTransform *self)
{
	self->priv = gth_icc_transform_get_instance_private (self);
	self->priv->cms_transform = NULL;
}


GthICCTransform *
gth_icc_transform_new (GthCMSTransform transform)
{
	GthICCTransform *icc_transform;

	icc_transform = g_object_new (GTH_TYPE_ICC_TRANSFORM, NULL);
	icc_transform->priv->cms_transform = transform;

	return icc_transform;
}


#if HAVE_LCMS2
#if G_BYTE_ORDER == G_LITTLE_ENDIAN /* BGRA */
#define _LCMS2_CAIRO_FORMAT TYPE_BGRA_8
#elif G_BYTE_ORDER == G_BIG_ENDIAN /* ARGB */
#define _LCMS2_CAIRO_FORMAT TYPE_ARGB_8
#else
#define _LCMS2_CAIRO_FORMAT TYPE_ABGR_8
#endif
#endif


GthICCTransform *
gth_icc_transform_new_from_profiles (GthICCProfile *from_profile,
			  	     GthICCProfile *to_profile)
{
#if HAVE_LCMS2
	GthICCTransform *transform = NULL;
	GthCMSTransform  cms_transform;

	cms_transform = (GthCMSTransform) cmsCreateTransform ((cmsHPROFILE) gth_icc_profile_get_profile (from_profile),
							      _LCMS2_CAIRO_FORMAT,
							      (cmsHPROFILE) gth_icc_profile_get_profile (to_profile),
							      _LCMS2_CAIRO_FORMAT,
							      INTENT_PERCEPTUAL,
							      0);
	if (cms_transform != NULL)
		transform = gth_icc_transform_new (cms_transform);

	return transform;
#else
	return NULL;
#endif
}


GthCMSTransform
gth_icc_transform_get_transform (GthICCTransform *transform)
{
	g_return_val_if_fail (transform != NULL, NULL);
	return transform->priv->cms_transform;
}
