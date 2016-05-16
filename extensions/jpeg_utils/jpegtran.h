/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2002, 2009 The Free Software Foundation, Inc.
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

#ifndef JPEGTRAN_H
#define JPEGTRAN_H

#include <config.h>
#include <glib.h>
#include <gthumb.h>
#include "transupp.h"


typedef enum {
	JPEG_ERROR_FAILED,
	JPEG_ERROR_MCU
} JpegErrorCode;


#define JPEG_ERROR jpeg_error_quark ()
GQuark jpeg_error_quark (void);


typedef enum {
	JPEG_MCU_ACTION_TRIM,
	JPEG_MCU_ACTION_DONT_TRIM,
	JPEG_MCU_ACTION_ABORT
} JpegMcuAction;


typedef struct {
	void          *in_buffer;
	gsize          in_buffer_size;
	void         **out_buffer;
	gsize         *out_buffer_size;
	GthTransform   transformation;
} JpegTranInfo;


gboolean   jpegtran  (void           *in_buffer,
		      gsize           in_buffer_size,
		      void          **out_buffer,
		      gsize          *out_buffer_size,
		      GthTransform    transformation,
		      JpegMcuAction   mcu_action,
		      GError        **error);

#endif /* JPEGTRAN_H */
