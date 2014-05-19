/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2013 Intel Corporation.
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

GSignondDictionary *
gsignond_dictionary_new (void);

GSignondDictionary *
gsignond_dictionary_ref (GSignondDictionary *dict);

void
gsignond_dictionary_unref (GSignondDictionary *dict);

GSignondDictionary *
gsignond_dictionary_copy (GSignondDictionary *other);

GSignondDictionary *
gsignond_dictionary_new_from_variant (GVariant *variant);

GVariant *
gsignond_dictionary_to_variant (GSignondDictionary *dict);

GVariantBuilder *
gsignond_dictionary_to_variant_builder (GSignondDictionary *dict);

GVariant *
gsignond_dictionary_get (GSignondDictionary *dict, const gchar *key);

gboolean
gsignond_dictionary_set (GSignondDictionary *dict, 
    const gchar *key, GVariant *value);

gboolean
gsignond_dictionary_get_boolean (GSignondDictionary *dict, const gchar *key,
                                 gboolean *value);

gboolean
gsignond_dictionary_set_boolean (GSignondDictionary *dict, const gchar *key,
                                 gboolean value);

gboolean
gsignond_dictionary_get_int32 (GSignondDictionary *dict, const gchar *key,
                               gint *value);

gboolean
gsignond_dictionary_set_int32 (GSignondDictionary *dict, const gchar *key,
                               gint value);

gboolean
gsignond_dictionary_get_uint32 (GSignondDictionary *dict, const gchar *key,
                                guint *value);

gboolean
gsignond_dictionary_set_uint32 (GSignondDictionary *dict, const gchar *key,
                                guint32 value);

gboolean
gsignond_dictionary_get_int64 (GSignondDictionary *dict, const gchar *key,
                               gint64 *value);

gboolean
gsignond_dictionary_set_int64 (GSignondDictionary *dict, const gchar *key,
                               gint64 value);

gboolean
gsignond_dictionary_get_uint64 (GSignondDictionary *dict, const gchar *key,
                                guint64 *value);

gboolean
gsignond_dictionary_set_uint64 (GSignondDictionary *dict, const gchar *key,
                                guint64 value);

const gchar *
gsignond_dictionary_get_string (GSignondDictionary *dict, const gchar *key);

gboolean
gsignond_dictionary_set_string (GSignondDictionary *dict, const gchar *key,
                                const gchar *value);

gboolean
gsignond_dictionary_remove (GSignondDictionary *dict, const gchar *key);

gboolean
gsignond_dictionary_contains (GSignondDictionary *dict, const gchar *key);

G_END_DECLS

#endif /* __GSIGNOND_DICTIONARY_H__ */
