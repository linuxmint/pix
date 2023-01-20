/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright © 2011 Free Software Foundation, Inc.
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

#include <gthumb.h>

G_BEGIN_DECLS

/* schema */

#define GTHUMB_CHANGE_DATE_SCHEMA                        GTHUMB_SCHEMA ".change-date"

/* keys */

#define PREF_CHANGE_DATE_SET_LAST_MODIFIED_DATE          "set-last-modified-date"
#define PREF_CHANGE_DATE_SET_COMMENT_DATE		 "set-comment-date"
#define PREF_CHANGE_DATE_TO_FOLLOWING_DATE               "to-following-date"
#define PREF_CHANGE_DATE_DATE                            "date"
#define PREF_CHANGE_DATE_TO_FILE_MODIFIED_DATE           "to-file-modified-date"
#define PREF_CHANGE_DATE_TO_FILE_CREATION_DATE           "to-file-creation-date"
#define PREF_CHANGE_DATE_TO_PHOTO_ORIGINAL_DATE          "to-photo-original-date"
#define PREF_CHANGE_DATE_ADJUST_TIME                     "adjust-time"
#define PREF_CHANGE_DATE_TIME_ADJUSTMENT                 "time-adjustment"

G_END_DECLS

#endif /* PREFERENCES_H */
