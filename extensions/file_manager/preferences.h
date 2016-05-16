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

#ifndef PREFERENCES_H
#define PREFERENCES_H

G_BEGIN_DECLS

/* schema */

#define GTHUMB_FILE_MANAGER_SCHEMA              GTHUMB_SCHEMA ".file-manager"

/* keys */

#define PREF_FILE_MANAGER_COPY_LAST_FOLDER      "last-folder"
#define PREF_FILE_MANAGER_COPY_VIEW_DESTINATION "view-destination"
#define PREF_FILE_MANAGER_COPY_HISTORY          "copy-destination-history"

G_END_DECLS

#endif /* PREFERENCES_H */
