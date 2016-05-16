/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#ifndef PICASA_WEB_TYPES_H
#define PICASA_WEB_TYPES_H

typedef enum  {
	PICASA_WEB_ACCESS_ALL,
	PICASA_WEB_ACCESS_PRIVATE,
	PICASA_WEB_ACCESS_PUBLIC,
	PICASA_WEB_ACCESS_VISIBLE
} PicasaWebAccess;

typedef enum {
	PICASA_WEB_THUMB_SIZE_SMALL = 72,
	PICASA_WEB_THUMB_SIZE_MEDIUM = 144,
	PICASA_WEB_THUMB_SIZE_LARGE = 288
} PicasaWebThumbSize;

#endif /* PICASA_WEB_TYPES_H */
