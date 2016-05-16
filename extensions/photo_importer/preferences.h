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

#ifndef PHOTO_IMPORTER_PREFERENCES_H
#define PHOTO_IMPORTER_PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

/* schemas */

#define GTHUMB_PHOTO_IMPORTER_SCHEMA             GTHUMB_SCHEMA ".photo-importer"

/* keys: viewer */

#define PREF_PHOTO_IMPORTER_DELETE_FROM_DEVICE   "delete-from-device"
#define PREF_PHOTO_IMPORTER_ADJUST_ORIENTATION   "adjust-orientation"

G_END_DECLS

#endif /* PHOTO_IMPORTER_PREFERENCES_H */
