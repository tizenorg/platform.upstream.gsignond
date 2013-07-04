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
#include <gsignond/gsignond-log.h>

/**
 * SECTION:gsignond-dictionary
 * @short_description: a dictionary container holding string keys and variant values
 * @title: GSignondDictionary
 * @include: gsignond/gsignond-dictionary.h
 *
 * A #GSignondDictionary is a dictionary data structure that maps string keys to #GVariant values.
 * It's used in multiple places in gsignond and its public API to pass key-value
 * data sets.
 * 
 * |[    GSignondDictionary* dict = gsignond_dictionary_new();
 *     gsignond_dictionary_set_string(dict, "name", "John Smith");
 *     gsignond_dictionary_set_uint32(dict, "age", 32);
 *
 *     guint32 age;
 *     gboolean success = gsignond_dictionary_get_uint32(dict, "age", &age);
 *     const gchar* name = gsignond_dictionary_get_string(dict, "name");
 *     gsignond_dictionary_unref(dict);
 * ]| 
 */


/**
 * gsignond_dictionary_new_from_variant:
 * @variant: instance of #GVariant
 *
 * Converts the #GVariant to #GSignondDictionary. This is useful for example if 
 * the dictionary needs to be deserialized, or if it's contained in another 
 * #GSignondDictionary and has been retrieved using gsignond_dictionary_get().
 *
 * Returns: (transfer full): #GSignondDictionary if successful, NULL otherwise.
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
 * Converts the #GSignondDictionary to a #GVariant. The result can be serialized
 * or put into another #GSignondDictionary using gsignond_dictionary_set().
 *
 * Returns: (transfer full): #GVariant object if successful, NULL otherwise.
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
 * Creates a new instance of #GSignondDictionary.
 *
 * Returns: (transfer full): #GSignondDictionary object if successful,
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
 * gsignond_dictionary_ref:
 * @dict: instance of #GSignondDictionary
 *
 * Increments the reference count of the dictionary structure.
 * 
 * Returns: the pointer to the passed in #GSignondDictionary
 */
GSignondDictionary*
gsignond_dictionary_ref (GSignondDictionary *dict)
{
    g_return_val_if_fail (dict != NULL, NULL);

    return (GSignondDictionary*)g_hash_table_ref (dict);
}

/**
 * gsignond_dictionary_unref:
 * @dict: instance of #GSignondDictionary
 *
 * Decrements the reference count of the dictionary structure. If the reference
 * count reaches zero, the structure is deallocated and shouldn't be used.
 *
 */
void
gsignond_dictionary_unref (GSignondDictionary *dict)
{
    if (!dict)
        return;

    g_hash_table_unref (dict);
}

/**
 * gsignond_dictionary_get:
 * @dict: instance of #GSignondDictionary
 * @key: the key to look up in the dictionary
 *
 * Retrieves a #GVariant value from the dictionary. This can be used to retrieve
 * a value of an arbitrary type, and then convert it manually to a specific type
 * using #GVariant methods. For most commonly used types, also getters that
 * return the specific type directly are provided (gsignond_dictionary_get_string()
 * and similar).
 *
 * Returns: (transfer none): the value; NULL is returned in case of failure (for 
 * example if the entry corresponding to the supplied key doesn't exist).
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
 * @key: (transfer none): key to be set
 * @value: (transfer full): value to be set
 *
 * Adds or replaces key-value pair in the dictionary. This allows to set a value
 * of an arbitrary type: it first needs to be converted to a #GVariant. For most
 * commonly used types also type-specific setters are provided.
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
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to look up
 * @value: points to the location where the value should be set
 *
 * Retrieves a gboolean value.
 *
 * Returns: TRUE if the value was retrieved successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_get_boolean (GSignondDictionary *dict, const gchar *key,
                                 gboolean *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    if (variant == NULL)
        return FALSE;

    if (value)
        *value = g_variant_get_boolean (variant);
    return TRUE;
}

/**
 * gsignond_dictionary_set_boolean:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to set
 * @value: value to set
 *
 * Sets or replaces a gboolean value in the dictionary.
 * 
 * Returns: TRUE if the value was set or replaced successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_set_boolean (GSignondDictionary *dict, const gchar *key,
                                 gboolean value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_boolean (value));
}

/**
 * gsignond_dictionary_get_int32:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to look up
 * @value: points to the location where the value should be set
 *
 * Retrieves a int32 value.
 * 
 * Returns: TRUE if the value was retrieved successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_get_int32 (GSignondDictionary *dict, const gchar *key,
                               gint32 *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    if (variant == NULL)
        return FALSE;

    if (value)
        *value = g_variant_get_int32 (variant);
    return TRUE;
}

/**
 * gsignond_dictionary_set_int32:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to set
 * @value: value to set
 *
 * Sets or replaces a int32 value in the dictionary.
 * 
 * Returns: TRUE if the value was set or replaced successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_set_int32 (GSignondDictionary *dict, const gchar *key,
                               gint32 value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_int32 (value));
}

/**
 * gsignond_dictionary_get_uint32:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to look up
 * @value: points to the location where the value should be set
 *
 * Retrieves a uint32 value.
 * 
 * Returns: TRUE if the value was retrieved successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_get_uint32 (GSignondDictionary *dict, const gchar *key,
                                guint32 *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    if (variant == NULL)
        return FALSE;

    if (value)
        *value = g_variant_get_uint32 (variant);
    return TRUE;
}

/**
 * gsignond_dictionary_set_uint32:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to set
 * @value: value to set
 *
 * Sets or replaces a uint32 value in the dictionary.
 * 
 * Returns: TRUE if the value was set or replaced successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_set_uint32 (GSignondDictionary *dict, const gchar *key,
                                guint32 value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_uint32 (value));
}

/**
 * gsignond_dictionary_get_int64:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to look up
 * @value: points to the location where the value should be set
 *
 * Retrieves a int64 value.
 * 
 * Returns: TRUE if the value was retrieved successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_get_int64 (GSignondDictionary *dict, const gchar *key,
                               gint64 *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    if (variant == NULL)
        return FALSE;

    if (value)
        *value = g_variant_get_int64 (variant);
    return TRUE;
}

/**
 * gsignond_dictionary_set_int64:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to set
 * @value: value to set
 *
 * Sets or replaces a int64 value in the dictionary.
 * 
 * Returns: TRUE if the value was set or replaced successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_set_int64 (GSignondDictionary *dict, const gchar *key,
                               gint64 value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_int64 (value));
}

/**
 * gsignond_dictionary_get_uint64:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to look up
 * @value: points to the location where the value should be set
 *
 * Retrieves a uint64 value.
 * 
 * Returns: TRUE if the value was retrieved successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_get_uint64 (GSignondDictionary *dict, const gchar *key,
                                guint64 *value)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    if (variant == NULL)
        return FALSE;

    if (value)
        *value = g_variant_get_uint64 (variant);
    return TRUE;
}

/**
 * gsignond_dictionary_set_uint64:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to set
 * @value: value to set
 *
 * Sets or replaces a uint64 value in the dictionary.
 * 
 * Returns: TRUE if the value was set or replaced successfully, FALSE otherwise.
 */
gboolean
gsignond_dictionary_set_uint64 (GSignondDictionary *dict, const gchar *key,
                                guint64 value)
{
    return gsignond_dictionary_set (dict, key, g_variant_new_uint64 (value));
}


/**
 * gsignond_dictionary_get_string:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to look up
 *
 * Retrieves a string value.
 * 
 * Returns: (transfer none): the value if it was retrieved successfully, NULL otherwise.
 */
const gchar *
gsignond_dictionary_get_string (GSignondDictionary *dict, const gchar *key)
{
    GVariant *variant = gsignond_dictionary_get (dict, key);

    if (variant == NULL)
        return NULL;

    return g_variant_get_string (variant, NULL);
}

/**
 * gsignond_dictionary_set_string:
 * @dict: instance of #GSignondDictionary
 * @key: (transfer none): key to set
 * @value: (transfer none): value to set
 *
 * Sets or replaces a string value in the dictionary.
 * 
 * Returns: TRUE if the value was set or replaced successfully, FALSE otherwise.
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
 * @key: (transfer none): key which needs to be removed from the dictionary
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
 * Returns: (transfer full): #GSignondDictionary object if the copy was successful,
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
