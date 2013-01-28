/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __GSIGNOND_DICTIONARY_H__
#define __GSIGNOND_DICTIONARY_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_DICTIONARY (G_TYPE_HASH_TABLE)

#define GSIGNOND_DICTIONARY(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_TYPE_DICTIONARY, \
                                           GSignondDictionary))
#define GSIGNOND_IS_DICTIONARY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_TYPE_DICTIONARY))

typedef GHashTable GSignondDictionary;

GType
gsignond_dictionary_get_type (void);

GSignondDictionary *
gsignond_dictionary_new (void);

void
gsignond_dictionary_free (GSignondDictionary *dict);

GSignondDictionary *
gsignond_dictionary_copy (GSignondDictionary *other);

GSignondDictionary *
gsignond_dictionary_new_from_variant (GVariant *variant);

GVariant *
gsignond_dictionary_to_variant (GSignondDictionary *self);

GVariant *
gsignond_dictionary_get (GSignondDictionary *dict, const gchar *key);

gboolean
gsignond_dictionary_set (GSignondDictionary *dict, 
    const gchar *key, GVariant *value);

G_END_DECLS

#endif /* __GSIGNOND_DICTIONARY_H__ */
