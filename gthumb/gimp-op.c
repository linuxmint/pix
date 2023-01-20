/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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

#include <config.h>
#include "gimp-op.h"


guchar add_alpha_table[256][256];
static GOnce  gimp_op_init_once = G_ONCE_INIT;


static gpointer
init_tables (gpointer data)
{
	int v;
	int a;
	int r;

	/* add_alpha_table[v][a] = v * a / 255 */

	for (v = 0; v < 128; v++) {
		for (a = 0; a <= v; a++) {
			r = (v * a + 127) / 255;
			add_alpha_table[v][a] = add_alpha_table[a][v] = r;
			add_alpha_table[255-v][a] = add_alpha_table[a][255-v] = a - r;
			add_alpha_table[v][255-a] = add_alpha_table[255-a][v] = v - r;
			add_alpha_table[255-v][255-a] = add_alpha_table[255-a][255-v] = (255 - a) - (v - r);
		}
	}

	return NULL;
}


void
gimp_op_init (void)
{
	g_once (&gimp_op_init_once, init_tables, NULL);
}
