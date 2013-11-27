/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef _GSIGNOND_UTILS_H_
#define _GSIGNOND_UTILS_H_

#include <glib.h>

G_BEGIN_DECLS

gboolean
gsignond_wipe_file (const gchar *filename);

gboolean
gsignond_wipe_directory (const gchar *dirname);

gchar *
gsignond_generate_nonce ();

GVariant *
gsignond_sequence_to_variant (GSequence *seq);

GSequence *
gsignond_variant_to_sequence (GVariant *var);

gchar **
gsignond_sequence_to_array (GSequence *seq);

GSequence *
gsignond_array_to_sequence (gchar **items);

GSequence *
gsignond_copy_array_to_sequence (const gchar **items);

G_END_DECLS

#endif  /* _SGINOND_UTILS_H_ */

