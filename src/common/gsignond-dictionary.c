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

#include <gsignond/gsignond-dictionary.h>

/**
 * gsignond_dictionary_new_from_variant:
 * @variant: instance of #GVariant
 *
 * Converts the variant to GSignondDictionary.
 *
 * Returns: (transfer full) object if successful, NULL otherwise.
 */
GSignondDictionary *
gsignond_dictionary_new_from_variant (GVariant *variant)
{
    GSignondDictionary *dict = NULL;
    GVariantIter iter;
    gchar *key = NULL;
    GVariant *value = NULL;

    g_return_val_if_fail (variant != NULL, NULL);

    dict = gsignond_dictionary_new ();
    g_variant_iter_init (&iter, variant);
    while (g_variant_iter_next (&iter, "{sv}", &key, &value))
    {
        g_hash_table_insert (dict, key, value);
    }

    return dict;
}

/**
 * gsignond_dictionary_to_variant:
 * @dict: instance of #GSignondDictionary
 *
 * Converts the GSignondDictionary to variant.
 *
 * Returns: (transfer full) #GVariant object if successful, NULL otherwise.
 */
GVariant *
gsignond_dictionary_to_variant (GSignondDictionary *dict)
{
    GVariantBuilder builder;
    GHashTableIter iter;
    GVariant *vdict = NULL;
    const gchar *key = NULL;
    GVariant *value = NULL;

    g_return_val_if_fail (dict != NULL, NULL);

    g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
    g_hash_table_iter_init (&iter, dict);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&key,
                                   (gpointer)&value))
    {
        g_variant_builder_add (&builder, "{sv}",
                               key,
                               value);
    }
    vdict = g_variant_builder_end (&builder);
    return vdict;
}

/**
 * gsignond_dictionary_new:
 *
 * Creates new instance of GSignondDictionary.
 *
 * Returns: (transfer full) #GSignondDictionary object if successful,
 * NULL otherwise.
 */
GSignondDictionary *
gsignond_dictionary_new (void)
{
    return g_hash_table_new_full ((GHashFunc)g_str_hash,
                            (GEqualFunc)g_str_equal,
                            (GDestroyNotify)g_free,
                            (GDestroyNotify)g_variant_unref);
}

/**
 * gsignond_dictionary_free:
 * @dict: instance of #GSignondDictionary
 *
 * Frees the memory allocated by dict structure.
 *
 */
void
gsignond_dictionary_free (GSignondDictionary *dict)
{
    g_return_if_fail (dict != NULL);
    g_hash_table_unref (dict);
}

/**
 * gsignond_dictionary_get:
 * @dict: instance of #GSignondDictionary
 *
 * Retrieves a value from the dictionary.
 *
 * Returns: (transfer none) the value; NULL is returned in case of failure.
 */
GVariant *
gsignond_dictionary_get (GSignondDictionary *dict, const gchar *key)
{
    g_return_val_if_fail (dict != NULL, NULL);
    g_return_val_if_fail (key != NULL, NULL);

    return g_hash_table_lookup (dict, key);
}

/**
 * gsignond_dictionary_set:
 * @dict: instance of #GSignondDictionary
 *
 * @key: key to be set
 * @value: value to be set
 *
 * Adds or replaces key-value pair in the dictionary.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_dictionary_set (GSignondDictionary *dict, 
    const gchar *key, GVariant *value)
{
    g_return_val_if_fail (dict != NULL, FALSE);
    g_return_val_if_fail (key != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    g_variant_ref_sink(value);
    
    g_hash_table_replace (
            dict,
            g_strdup(key),
            value);

    return TRUE;
}

/**
 * gsignond_dictionary_get_boolean:
 *
 * Overload, see #gsignond_dictionary_get for details.
 */
gboolean
gsignond_dictionary_get_boolean (GSignondDictionary *dict, const gchar *key,
                                 gboolean *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    g_return_val_if_fail (variant != NULL, FALSE);
    *value = g_variant_get_boolean (variant);
    return TRUE;
}

/**
 * gsignon_dictionary_set_boolean:
 *
 * Overload, see #gsignond_dictionary_set for details.
 */
gboolean
gsignond_dictionary_set_boolean (GSignondDictionary *dict, const gchar *key,
                                 gboolean value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_boolean (value));
}

/**
 * gsignond_dictionary_get_int32:
 *
 * Overload, see #gsignond_dictionary_get for details.
 */
gboolean
gsignond_dictionary_get_int32 (GSignondDictionary *dict, const gchar *key,
                               gint32 *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    g_return_val_if_fail (variant != NULL, FALSE);
    *value = g_variant_get_int32 (variant);
    return TRUE;
}

/**
 * gsignon_dictionary_set_int32:
 *
 * Overload, see #gsignond_dictionary_set for details.
 */
gboolean
gsignond_dictionary_set_int32 (GSignondDictionary *dict, const gchar *key,
                               gint32 value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_int32 (value));
}

/**
 * gsignond_dictionary_get_guint32:
 *
 * Overload, see #gsignond_dictionary_get for details.
 */
gboolean
gsignond_dictionary_get_uint32 (GSignondDictionary *dict, const gchar *key,
                                guint32 *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    g_return_val_if_fail (variant != NULL, FALSE);
    *value = g_variant_get_boolean (variant);
    return TRUE;
}

/**
 * gsignon_dictionary_set_guint32:
 *
 * Overload, see #gsignond_dictionary_set for details.
 */
gboolean
gsignond_dictionary_set_uint32 (GSignondDictionary *dict, const gchar *key,
                                guint32 value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_uint32 (value));
}

/**
 * gsignond_dictionary_get_string:
 *
 * Overload, see #gsignond_dictionary_get for details.
 */
const gchar *
gsignond_dictionary_get_string (GSignondDictionary *dict, const gchar *key)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    g_return_val_if_fail (variant != NULL, NULL);
    return g_variant_get_string (variant, NULL);
}

/**
 * gsignon_dictionary_set_string:
 *
 * Overload, see #gsignond_dictionary_set for details.
 */
gboolean
gsignond_dictionary_set_string (GSignondDictionary *dict, const gchar *key,
                                const gchar *value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_string (value));
}

/**
 * gsignond_dictionary_remove:
 * @dict: instance of #GSignondDictionary
 *
 * @key: key which needs to be removed from the dictionary
 * @value: value to be set
 *
 * Removes key-value pair in the dictionary as per key.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_dictionary_remove (GSignondDictionary *dict, const gchar *key)
{
    g_return_val_if_fail (dict != NULL, FALSE);
    g_return_val_if_fail (key != NULL, FALSE);

    return g_hash_table_remove (
            dict,
            key);
}

/**
 * gsignond_dictionary_copy:
 * @other: instance of #GSignondDictionary
 *
 * Creates a copy of the dictionary.
 *
 * Returns: (transfer full) #GSignondDictionary object if successful,
 * NULL otherwise.
 */
GSignondDictionary *
gsignond_dictionary_copy (GSignondDictionary *other)
{
    GSignondDictionary *dict = NULL;
    GHashTableIter iter;
    gchar *key = NULL;
    GVariant *value = NULL;

    g_return_val_if_fail (other != NULL, NULL);

    dict = gsignond_dictionary_new ();
    
    g_hash_table_iter_init (&iter, other);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&key,
                                   (gpointer)&value))
    {
        gsignond_dictionary_set (dict, key, value);
    }
    

    return dict;
}
