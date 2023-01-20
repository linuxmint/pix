/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GTH_TEST_CHAIN_H
#define GTH_TEST_CHAIN_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-test.h"

G_BEGIN_DECLS

#define GTH_TYPE_TEST_CHAIN         (gth_test_chain_get_type ())
#define GTH_TEST_CHAIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TEST_CHAIN, GthTestChain))
#define GTH_TEST_CHAIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TEST_CHAIN, GthTestChainClass))
#define GTH_IS_TEST_CHAIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TEST_CHAIN))
#define GTH_IS_TEST_CHAIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TEST_CHAIN))
#define GTH_TEST_CHAIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TEST_CHAIN, GthTestChainClass))

typedef struct _GthTestChain         GthTestChain;
typedef struct _GthTestChainPrivate  GthTestChainPrivate;
typedef struct _GthTestChainClass    GthTestChainClass;

typedef enum  {
	GTH_MATCH_TYPE_NONE = 0,
	GTH_MATCH_TYPE_ALL,
	GTH_MATCH_TYPE_ANY
} GthMatchType;

struct _GthTestChain
{
	GthTest __parent;
	GthTestChainPrivate *priv;
};

struct _GthTestChainClass
{
	GthTestClass __parent_class;
};

GType          gth_test_chain_get_type        (void) G_GNUC_CONST;
GthTest *      gth_test_chain_new             (GthMatchType   match_type,
					       ...) G_GNUC_NULL_TERMINATED;
void           gth_test_chain_set_match_type  (GthTestChain  *chain,
					       GthMatchType   match_type);
GthMatchType   gth_test_chain_get_match_type  (GthTestChain  *chain);
void           gth_test_chain_clear_tests     (GthTestChain  *chain);
void           gth_test_chain_add_test        (GthTestChain  *chain,
				               GthTest       *test);
GList *        gth_test_chain_get_tests       (GthTestChain  *chain);
gboolean       gth_test_chain_has_type_test   (GthTestChain  *chain);

G_END_DECLS

#endif /* GTH_TEST_CHAIN_H */
