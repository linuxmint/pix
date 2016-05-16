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

typedef enum {
	GTH_CHANGE_CASE_NONE = 0,
	GTH_CHANGE_CASE_LOWER,
	GTH_CHANGE_CASE_UPPER,
} GthChangeCase;

/* schema */

#define GTHUMB_RENAME_SERIES_SCHEMA        GTHUMB_SCHEMA ".rename-series"

/* keys */

#define  PREF_RENAME_SERIES_TEMPLATE       "template"
#define  PREF_RENAME_SERIES_START_AT       "start-at"
#define  PREF_RENAME_SERIES_SORT_BY        "sort-by"
#define  PREF_RENAME_SERIES_REVERSE_ORDER  "reverse-order"
#define  PREF_RENAME_SERIES_CHANGE_CASE    "change-case"

G_END_DECLS

#endif /* PREFERENCES_H */
