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

#ifndef PIXBUF_CACHE_H
#define PIXBUF_CACHE_H


typedef enum {
	PIXBUF_CACHE_CHANNEL_VALUE = 0,
	PIXBUF_CACHE_CHANNEL_RED,
	PIXBUF_CACHE_CHANNEL_GREEN,
	PIXBUF_CACHE_CHANNEL_BLUE,
	PIXBUF_CACHE_CHANNEL_ALPHA,
	PIXBUF_CACHE_CHANNEL_SIZE
} PixbufCacheChannel;


typedef struct _PixbufCache PixbufCache;


PixbufCache * pixbuf_cache_new   (void);
void          pixbuf_cache_free  (PixbufCache        *cache);
gboolean      pixbuf_cache_get   (PixbufCache        *cache,
				  PixbufCacheChannel  channel,
				  int                *value);
void          pixbuf_cache_set   (PixbufCache        *cache,
				  PixbufCacheChannel  channel,
				  guchar              value,
				  guchar              result);

#endif /* PIXBUF_CACHE_H */
