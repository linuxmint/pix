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

#ifndef IMPORTER_PREFERENCES_H
#define IMPORTER_PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

/* schemas */

#define GTHUMB_IMPORTER_SCHEMA                 GTHUMB_SCHEMA ".importer"

/* keys: viewer */

#define PREF_IMPORTER_DESTINATION              "destination"
#define PREF_IMPORTER_SUBFOLDER_TEMPLATE       "subfolder-template"
#define PREF_IMPORTER_WARN_DELETE_UNSUPPORTED  "warn-delete-unsupported"

G_END_DECLS

#endif /* IMPORTER_PREFERENCES_H */
