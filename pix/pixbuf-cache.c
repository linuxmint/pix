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
#include <glib.h>
#include "pixbuf-cache.h"


typedef struct {
	guchar result[256];
	guchar mask[32];
} ChannelCache;


struct _PixbufCache {
	ChannelCache channel[PIXBUF_CACHE_CHANNEL_SIZE];
};


PixbufCache *
pixbuf_cache_new (void)
{
	return g_new0 (PixbufCache, 1);
}


void
pixbuf_cache_free (PixbufCache *cache)
{
	g_free (cache);
}


gboolean
pixbuf_cache_get (PixbufCache        *cache,
		  PixbufCacheChannel  channel,
		  int                *value)
{
	if (((cache->channel[channel].mask[*value / 8] >> (*value % 8)) & 1) != 1)
		return FALSE;
	*value = cache->channel[channel].result[*value];

	return TRUE;
}


void
pixbuf_cache_set (PixbufCache        *cache,
		  PixbufCacheChannel  channel,
		  guchar              value,
		  guchar              result)
{
	cache->channel[channel].result[value] = result;
	cache->channel[channel].mask[value / 8] |= 1 << (value % 8);
}
