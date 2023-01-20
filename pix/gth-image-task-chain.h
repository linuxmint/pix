/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_TASK_CHAIN_H
#define GTH_IMAGE_TASK_CHAIN_H

#include <glib.h>
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_TASK_CHAIN            (gth_image_task_chain_get_type ())
#define GTH_IMAGE_TASK_CHAIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_TASK_CHAIN, GthImageTaskChain))
#define GTH_IMAGE_TASK_CHAIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_TASK_CHAIN, GthImageTaskChainClass))
#define GTH_IS_IMAGE_TASK_CHAIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_TASK_CHAIN))
#define GTH_IS_IMAGE_TASK_CHAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_TASK_CHAIN))
#define GTH_IMAGE_TASK_CHAIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_TASK_CHAIN, GthImageTaskChainClass))

typedef struct _GthImageTaskChain        GthImageTaskChain;
typedef struct _GthImageTaskChainClass   GthImageTaskChainClass;
typedef struct _GthImageTaskChainPrivate GthImageTaskChainPrivate;

struct _GthImageTaskChain {
	GthTask __parent;
	GthImageTaskChainPrivate *priv;
};

struct _GthImageTaskChainClass {
	GthTaskClass __parent;
};

GType		gth_image_task_chain_get_type	(void);
GthTask *	gth_image_task_chain_new	(const char	*description,
						 GthTask	*task,
						 ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* GTH_IMAGE_TASK_CHAIN_H */
