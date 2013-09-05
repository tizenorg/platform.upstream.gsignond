/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2013 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@linux.intel.com>
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

#include "gsignond-identity-info.h"
#include "gsignond-identity-info-internal.h"

G_DEFINE_BOXED_TYPE(GSignondIdentityInfo,
                    gsignond_identity_info,
                    gsignond_identity_info_ref,
                    gsignond_identity_info_unref)

struct _GSignondIdentityInfo
{
    volatile gint ref_count;
    gchar *username;
    gchar *secret;
    GSignondIdentityInfoPropFlags edit_flags;
    GSignondDictionary *map;
};

static gboolean
_gsignond_identity_info_seq_cmp (
        GSequence *one,
        GSequence *two)
{
    GSequenceIter *iter1 = NULL, *iter2 = NULL;
    gboolean equal = TRUE;

   if (one == two)
        return TRUE;

   if (one == NULL) {
	if (g_sequence_get_length (two) == 0)
	    return TRUE;
        else
	    return FALSE;
   }

   if (two == NULL) {
	if (g_sequence_get_length (one) == 0)
	    return TRUE;
	else
	    return FALSE;
    }

    if (g_sequence_get_length (one) != g_sequence_get_length (two))
        return FALSE;

    iter1 = g_sequence_get_begin_iter (one);
    while (!g_sequence_iter_is_end (iter1)) {
        iter2 = g_sequence_get_iter_at_pos (two,
                    g_sequence_iter_get_position (iter1));
        if (g_strcmp0 (g_sequence_get (iter1), g_sequence_get (iter2)) != 0) {
            equal = FALSE;
            break;
        }
        iter1 = g_sequence_iter_next (iter1);
    }

    return equal;
}

static gint
_compare_strings (
		const gchar* a,
		const gchar* b,
		gpointer data)
{
	(void)data;
	return g_strcmp0 (a,b);
}

static GVariant *
_gsignond_identity_info_sequence_to_variant (GSequence *seq)

{
    GSequenceIter * iter = NULL;
    GVariant *var = NULL;
    GVariantBuilder builder;

    if (!seq) return NULL;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_STRING_ARRAY);
    iter = g_sequence_get_begin_iter (seq);
    while (!g_sequence_iter_is_end (iter)) {
        const gchar * d = g_sequence_get (iter);
        g_variant_builder_add (&builder, "s", d);
        iter = g_sequence_iter_next (iter);
    }
    var = g_variant_builder_end (&builder);
    return var;
}

static GSequence *
_gsignond_identity_info_variant_to_sequence (GVariant *var)

{
    GVariantIter iter;
    GSequence *seq = NULL;
    gchar *item = NULL;

    if (!var) return NULL;

    seq = g_sequence_new ((GDestroyNotify)g_free);
    g_variant_iter_init (&iter, var);
    while (g_variant_iter_next (&iter, "s", &item)) {
        g_sequence_insert_sorted (seq,
                                  item,
                                  (GCompareDataFunc) _compare_strings,
                                  NULL);
    }
    return seq;
}

static gchar **
_gsignond_identity_info_sequence_to_array (GSequence *seq)
{
    gchar **items, **temp;
    GSequenceIter *iter;

    if (!seq) return NULL;

    items = g_malloc0 ((g_sequence_get_length (seq) + 1) * sizeof (gchar *));
    temp = items;
    for (iter = g_sequence_get_begin_iter (seq);
         iter != g_sequence_get_end_iter (seq);
         iter = g_sequence_iter_next (iter)) {
        *temp = g_sequence_get (iter);
        temp++;
    }
    return items;
}

static GSequence *
_gsignond_identity_info_array_to_sequence (gchar **items)

{
    GSequence *seq = NULL;

    if (!items) return NULL;

    seq = g_sequence_new ((GDestroyNotify) g_free);
    while (*items) {
        g_sequence_insert_sorted (seq,
                                  *items,
                                  (GCompareDataFunc) _compare_strings,
                                  NULL);
        items++;
    }
    return seq;
}

static gboolean
_gsignond_identity_info_sec_context_list_cmp (
        GSignondSecurityContextList *one,
        GSignondSecurityContextList *two)
{
    GSignondSecurityContextList *list_elem1 = NULL, *list_elem2 = NULL;
    gboolean equal = TRUE;

    if (one == NULL && two == NULL)
        return TRUE;

    if ((one != NULL && two == NULL) ||
        (one == NULL && two != NULL) ||
        (g_list_length (one) != g_list_length (two)))
        return FALSE;

    if (one == two)
        return TRUE;

    list_elem1 = one;
    for ( ; list_elem1 != NULL; list_elem1 = g_list_next (list_elem1)) {
        list_elem2 = g_list_nth (two, g_list_position (one, list_elem1));
        if (!gsignond_security_context_match (
                (GSignondSecurityContext *)list_elem1->data,
                (GSignondSecurityContext *)list_elem2->data)) {
            equal = FALSE;
            break;
        }
    }

    return equal;
}

static gboolean
_gsignond_identity_info_methods_cmp (
        GHashTable *one,
        GHashTable *two)
{
    GHashTableIter iter1;
    GSequence *mechs1 = NULL, *mechs2 = NULL;
    gchar *key = NULL;
    gboolean equal = TRUE;

    if (one == NULL && two == NULL)
        return TRUE;

    if ((one != NULL && two == NULL) ||
        (one == NULL && two != NULL) ||
        (g_hash_table_size (one) != g_hash_table_size (two)))
        return FALSE;

    if (one == two)
        return TRUE;

    g_hash_table_iter_init(&iter1, one);
    while (g_hash_table_iter_next (&iter1, (gpointer *)&key,
            (gpointer *)&mechs1)) {
        mechs2 = (GSequence *)g_hash_table_lookup (two, key);
        equal = _gsignond_identity_info_seq_cmp (mechs1, mechs2);
        if (!equal) {
            break;
        }
    }

    return equal;
}

GSignondIdentityInfoPropFlags
gsignond_identity_info_get_edit_flags (
        GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO(info),
                          IDENTITY_INFO_PROP_NONE);

    return info->edit_flags;
}

gboolean
gsignond_identity_info_set_edit_flags (
        GSignondIdentityInfo *info,
        GSignondIdentityInfoPropFlags flag)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO(info), FALSE);

    info->edit_flags |= flag;

    return TRUE;
}

gboolean
gsignond_identity_info_reset_edit_flags (
        GSignondIdentityInfo *info,
        GSignondIdentityInfoPropFlags flags)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO(info), FALSE);

    info->edit_flags = flags;

    return TRUE; 
}

gboolean
gsignond_identity_info_unset_edit_flags (
        GSignondIdentityInfo *info,
        GSignondIdentityInfoPropFlags unset_flags)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO(info), FALSE);

    info->edit_flags &= ~unset_flags;

    return TRUE;
}

GSignondIdentityInfoPropFlags
gsignond_identity_info_selective_copy (GSignondIdentityInfo *dest,
                                       const GSignondIdentityInfo *src,
                                       GSignondIdentityInfoPropFlags flags)
{
    GSignondIdentityInfoPropFlags tmp_flag;
    guint i;
    g_return_val_if_fail (src, IDENTITY_INFO_PROP_NONE);
    g_return_val_if_fail (dest, IDENTITY_INFO_PROP_NONE);
    g_return_val_if_fail (flags != IDENTITY_INFO_PROP_NONE, flags);

    /* This table should match to GSignondIdentityInfoPropFlags order */
    const gchar *keys[] = {
        GSIGNOND_IDENTITY_INFO_ID,
        GSIGNOND_IDENTITY_INFO_TYPE,
        GSIGNOND_IDENTITY_INFO_CAPTION,
        GSIGNOND_IDENTITY_INFO_STORESECRET,
        GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET,
        GSIGNOND_IDENTITY_INFO_OWNER,
        GSIGNOND_IDENTITY_INFO_ACL,
        GSIGNOND_IDENTITY_INFO_AUTHMETHODS,
        GSIGNOND_IDENTITY_INFO_REALMS,
        GSIGNOND_IDENTITY_INFO_REFCOUNT,
        GSIGNOND_IDENTITY_INFO_VALIDATED
    };

    for (i= 0, tmp_flag = IDENTITY_INFO_PROP_ID; 
         tmp_flag < IDENTITY_INFO_PROP_MAX;
         tmp_flag <<= 1, i++) {
        if (flags & tmp_flag && 
            gsignond_dictionary_contains (src->map, keys[i])) {
            gsignond_dictionary_set (dest->map, keys[i],
               g_variant_ref (gsignond_dictionary_get (src->map, keys[i])));
        }
        else {
            flags &= ~tmp_flag;
        }
    }

    if (flags & IDENTITY_INFO_PROP_USERNAME) {
        g_free (dest->username);
        dest->username = g_strdup (src->username);
    }

    if (flags & IDENTITY_INFO_PROP_SECRET) {
        g_free (dest->secret);
        dest->secret = g_strdup (src->secret);
    }

    dest->edit_flags |= flags;

    return flags;
}

/**
 * gsignond_identity_info_new:
 *
 * Creates new instance of GSignondIdentityInfo.
 *
 * Returns: (transfer full): #GSignondIdentityInfo object if successful,
 * NULL otherwise.
 */
GSignondIdentityInfo *
gsignond_identity_info_new (void)
{
    return gsignond_identity_info_new_from_variant (NULL);
}

/**
 * gsignond_identity_info_new_from_variant:
 *
 * Creates new instance of GSignondIdentityInfo.
 *
 * Returns: (transfer full) #GSignondIdentityInfo object if successful,
 * NULL otherwise.
 */
GSignondIdentityInfo *
gsignond_identity_info_new_from_variant (GVariant *variant_map)
{
    gboolean uname_is_secret = FALSE;
    GSignondIdentityInfo *info = g_slice_new0 (GSignondIdentityInfo);
    if (!info) return NULL;

    info->ref_count = 1;
    info->edit_flags = IDENTITY_INFO_PROP_NONE;
    info->username = info->secret = NULL;

    if (!variant_map) {
        info->map = gsignond_dictionary_new ();

        gsignond_dictionary_set (info->map, GSIGNOND_IDENTITY_INFO_ID,
            g_variant_new_uint32 (GSIGNOND_IDENTITY_INFO_NEW_IDENTITY));

        return info;
    }

    info->map = gsignond_dictionary_new_from_variant (variant_map);
    if (!info->map) return info;

    /* update edit flags */
    if (gsignond_dictionary_contains (info->map, GSIGNOND_IDENTITY_INFO_ID))
        info->edit_flags |= IDENTITY_INFO_PROP_ID;
    if (gsignond_dictionary_contains (info->map, GSIGNOND_IDENTITY_INFO_TYPE))
        info->edit_flags |= IDENTITY_INFO_PROP_TYPE;
    if (gsignond_dictionary_contains (info->map, 
                GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET)) {
        info->edit_flags |= IDENTITY_INFO_PROP_USERNAME_IS_SECRET;
        gsignond_dictionary_get_boolean (info->map,
                GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET, &uname_is_secret);
    }
    if (gsignond_dictionary_contains (info->map,
                GSIGNOND_IDENTITY_INFO_USERNAME)) {
        info->edit_flags |= IDENTITY_INFO_PROP_USERNAME;
        info->username = g_strdup (gsignond_dictionary_get_string (info->map,
                                            GSIGNOND_IDENTITY_INFO_USERNAME));
        gsignond_dictionary_remove (info->map, GSIGNOND_IDENTITY_INFO_USERNAME);
    }
    if (gsignond_dictionary_contains (info->map, 
                GSIGNOND_IDENTITY_INFO_SECRET)) {
        info->edit_flags |= IDENTITY_INFO_PROP_SECRET;
        info->secret = g_strdup (gsignond_dictionary_get_string (info->map,
                                    GSIGNOND_IDENTITY_INFO_SECRET));
        gsignond_dictionary_remove (info->map, GSIGNOND_IDENTITY_INFO_SECRET);
    }
    if (gsignond_dictionary_contains (info->map,
                GSIGNOND_IDENTITY_INFO_STORESECRET))
        info->edit_flags |= IDENTITY_INFO_PROP_STORE_SECRET;
    if (gsignond_dictionary_contains (info->map,
                GSIGNOND_IDENTITY_INFO_CAPTION))
        info->edit_flags |= IDENTITY_INFO_PROP_CAPTION;
    if (gsignond_dictionary_contains (info->map,
                GSIGNOND_IDENTITY_INFO_AUTHMETHODS))
        info->edit_flags |= IDENTITY_INFO_PROP_METHODS;
    if (gsignond_dictionary_contains (info->map, GSIGNOND_IDENTITY_INFO_REALMS))
        info->edit_flags |= IDENTITY_INFO_PROP_REALMS;
    if (gsignond_dictionary_contains (info->map, GSIGNOND_IDENTITY_INFO_ACL))
        info->edit_flags |= IDENTITY_INFO_PROP_ACL;
    if (gsignond_dictionary_contains (info->map, GSIGNOND_IDENTITY_INFO_OWNER))
        info->edit_flags |= IDENTITY_INFO_PROP_OWNER;
    if (gsignond_dictionary_contains (info->map,
                GSIGNOND_IDENTITY_INFO_REFCOUNT))
        info->edit_flags |= IDENTITY_INFO_PROP_REF_COUNT;
    if (gsignond_dictionary_contains (info->map,
                GSIGNOND_IDENTITY_INFO_VALIDATED))
        info->edit_flags |= IDENTITY_INFO_PROP_VALIDATED;

    return info;
}

/**
 * gsignond_identity_info_copy:
 * @info: instance of #GSignondIdentityInfo
 *
 * Creates a copy of info structure.
 *
 * Returns: copy of the info.
 */
GSignondIdentityInfo *
gsignond_identity_info_copy (GSignondIdentityInfo *info)
{
    GSignondIdentityInfo *new_info = NULL;
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    new_info = g_slice_new0 (GSignondIdentityInfo);
    if (!new_info) return NULL;

    new_info->ref_count = 1;
    new_info->edit_flags = info->edit_flags;
    new_info->username = g_strdup (info->username);
    new_info->secret = g_strdup (info->secret);
    new_info->map = gsignond_dictionary_copy (info->map);

    return new_info;
}

/**
 * gsignond_identity_info_ref:
 * @info: instance of #GSignondIdentityInfo
 *
 * Increment reference count of the info structure.
 * 
 * Returns: (transfer none) returns the same  copy of the info.
 */
GSignondIdentityInfo *
gsignond_identity_info_ref (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info != NULL, info);

    g_atomic_int_inc (&info->ref_count);

    return info;
}

/**
 * gsignond_identity_info_unref:
 * @info: instance of #GSignondIdentityInfo
 *
 * Decrement reference count of the info structure.
 */
void
gsignond_identity_info_unref (GSignondIdentityInfo *info)
{
    g_return_if_fail (info != NULL);

    if (g_atomic_int_dec_and_test (&info->ref_count)) {
        gsignond_dictionary_unref (info->map);
        g_slice_free (GSignondIdentityInfo, info);
    }
}

/**
 * gsignond_identity_info_get_id:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the id from the info.
 *
 * Returns: the id; negative id is returned in case of failure.
 */
guint32
gsignond_identity_info_get_id (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info),
            GSIGNOND_IDENTITY_INFO_NEW_IDENTITY);

    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_ID);

    return var ? g_variant_get_uint32 (var)
               : GSIGNOND_IDENTITY_INFO_NEW_IDENTITY;
}

/**
 * gsignond_identity_info_set_id:
 * @info: instance of #GSignondIdentityInfo
 *
 * @id: id to be set
 *
 * Sets the id of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_id (
        GSignondIdentityInfo *info,
        guint32 id)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    if (gsignond_identity_info_get_id (info) == id)
        return TRUE;

    return gsignond_dictionary_set (
            info->map,
            GSIGNOND_IDENTITY_INFO_ID,
            g_variant_new_uint32 (id)) &&
           gsignond_identity_info_set_edit_flags (info,
            IDENTITY_INFO_PROP_ID);
}

/**
 * gsignond_identity_info_get_is_identity_new:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the info whether the identity is new or not.
 *
 * Returns: TRUE if new, FALSE otherwise.
 */
gboolean
gsignond_identity_info_get_is_identity_new (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    return GSIGNOND_IDENTITY_INFO_NEW_IDENTITY ==
            gsignond_identity_info_get_id (info);
}

/**
 * gsignond_identity_info_set_identity_new:
 * @info: instance of #GSignondIdentityInfo
 *
 * Sets the id of the identity info to be new.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_identity_new (
        GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);
 
    return gsignond_identity_info_set_id (
            info,
            GSIGNOND_IDENTITY_INFO_NEW_IDENTITY);
}

/**
 * gsignond_identity_info_get_username:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the username from the info.
 *
 * Returns: the username if successful, NULL otherwise.
 */
const gchar *
gsignond_identity_info_get_username (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    return (const gchar *)info->username;
}

/**
 * gsignond_identity_info_set_username:
 * @info: instance of #GSignondIdentityInfo
 *
 * @username: username to be set
 *
 * Sets the username of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_username (
        GSignondIdentityInfo *info,
        const gchar *username)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    const gchar *current_name = gsignond_identity_info_get_username(info);
    if (g_strcmp0 (username, current_name) == 0) return TRUE;

    g_free (info->username);
    info->username = g_strdup (username);
 
    return gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_USERNAME);
}

/**
 * gsignond_identity_info_get_is_username_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the is_username_secret flag from the info.
 *
 * Returns: the is_username_secret flag.
 */
gboolean
gsignond_identity_info_get_is_username_secret (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    GVariant *var = gsignond_dictionary_get (info->map,
            GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET);
    return var ? g_variant_get_boolean (var) : FALSE;
}

/**
 * gsignond_identity_info_set_username_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * @store_secret: store_secret to be set
 *
 * Sets the store_secret of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_username_secret (
        GSignondIdentityInfo *info,
        gboolean username_secret)
{
    gboolean res = FALSE;
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    if (gsignond_identity_info_get_is_username_secret(info) == username_secret)
        return TRUE;

    res = gsignond_dictionary_set (info->map,
            GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET,
            g_variant_new_boolean(username_secret)) &&
           gsignond_identity_info_set_edit_flags (info, 
            IDENTITY_INFO_PROP_USERNAME_IS_SECRET);

    return res;
}

/**
 * gsignond_identity_info_get_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the secret from the info.
 *
 * Returns: the secret if successful, NULL otherwise.
 */
const gchar *
gsignond_identity_info_get_secret (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    return info->secret;
}

/**
 * gsignond_identity_info_set_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * @secret: secret to be set
 *
 * Sets the secret of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_secret (
        GSignondIdentityInfo *info,
        const gchar *secret)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    const gchar *current_secret = 
                gsignond_identity_info_get_secret (info);

    if (g_strcmp0 (current_secret, secret) == 0) return TRUE;

    if (info->secret) g_free (info->secret);

    info->secret = g_strdup (secret);

    return gsignond_identity_info_set_edit_flags (
                info, IDENTITY_INFO_PROP_SECRET);
}

/**
 * gsignond_identity_info_get_store_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the store_secret flag from the info.
 *
 * Returns: the store_secret flag.
 */
gboolean
gsignond_identity_info_get_store_secret (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    GVariant *var = gsignond_dictionary_get (info->map,
            GSIGNOND_IDENTITY_INFO_STORESECRET);
    return var ? g_variant_get_boolean (var) : FALSE;
}

/**
 * gsignond_identity_info_set_store_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * @store_secret: store_secret to be set
 *
 * Sets the store_secret of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_store_secret (
        GSignondIdentityInfo *info,
        gboolean store_secret)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    if (gsignond_identity_info_get_store_secret (info) == store_secret)
        return TRUE;

    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_STORESECRET,
                g_variant_new_boolean(store_secret)) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_STORE_SECRET);
}

/**
 * gsignond_identity_info_get_caption:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the caption from the info.
 *
 * Returns: the caption if successful, NULL otherwise.
 */
const gchar *
gsignond_identity_info_get_caption (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_CAPTION);
    return var ? g_variant_get_string (var, NULL) : NULL;
}

/**
 * gsignond_identity_info_set_caption:
 * @info: instance of #GSignondIdentityInfo
 *
 * @caption: caption to be set
 *
 * Sets the caption of the info.
 *
 * Returns: TRUE in case of success, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_caption (
        GSignondIdentityInfo *info,
        const gchar *caption)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);
    const gchar *current_caption = 
        gsignond_identity_info_get_caption (info);

    if (g_strcmp0 (current_caption, caption) == 0)
        return TRUE;

    if (!caption) {
        return gsignond_dictionary_remove (info->map,
                GSIGNOND_IDENTITY_INFO_CAPTION) &&
               gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_CAPTION);
    }
    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_CAPTION,
                g_variant_new_string (caption)) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_CAPTION);
}

/**
 * gsignond_identity_info_get_realms:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the realms from the info.
 *
 * Returns: (transfer full): the realms if successful, NULL Otherwise.
 * when done realms should be freed using g_sequence_free.
 */
GSequence *
gsignond_identity_info_get_realms (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_REALMS);
    return var ? _gsignond_identity_info_variant_to_sequence (var) : NULL;
}

/**
 * gsignond_identity_info_set_realms:
 * @info: instance of #GSignondIdentityInfo
 *
 * @realms: (transfer none): realms to be set
 *
 * Sets the realms of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_realms (
        GSignondIdentityInfo *info,
        GSequence *realms)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    g_return_val_if_fail (realms != NULL, FALSE);
    GVariant *current_realms = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_REALMS);
    GVariant *var_realms = _gsignond_identity_info_sequence_to_variant (realms);

    if (current_realms != NULL &&
        g_variant_equal (current_realms, var_realms) == TRUE) {
        g_variant_unref (var_realms);
        return TRUE;
    }

    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_REALMS,
                var_realms) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_REALMS);
}

/**
 * gsignond_identity_info_get_methods:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the methods from the info whereas #GHashTable consists of
 * (gchar*,GSequence*) and #GSequence is a sequence of gchar *.
 *
 * Returns: (transfer full): the methods if successful, NULL otherwise.
 * when done, methods should be freed using g_hash_table_unref.
 */
GHashTable *
gsignond_identity_info_get_methods (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    GHashTable *methods = NULL;
    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_AUTHMETHODS);
    if (var != NULL) {
        GVariantIter iter;
        gchar *vmethod;
        gchar **vmechanisms = NULL;
        GSequence *seq = NULL;

        methods = g_hash_table_new_full ((GHashFunc) g_str_hash,
                                         (GEqualFunc) g_str_equal,
                                         (GDestroyNotify) g_free,
                                         (GDestroyNotify) g_sequence_free);

        g_variant_iter_init (&iter, var);
        while (g_variant_iter_next (&iter, "{s^as}", &vmethod, &vmechanisms))
        {
            /* ownership of all content is transferred */
            seq = _gsignond_identity_info_array_to_sequence (vmechanisms);
            g_hash_table_insert (methods, vmethod, seq);
            g_free (vmechanisms);
        }
    }
    return methods;
}

/**
 * gsignond_identity_info_set_methods:
 * @info: instance of #GSignondIdentityInfo
 *
 * @methods: (transfer none): methods to be set whereas #GHashTable consists of
 * (gchar*,#GSequence*) and #GSequence is a sequence of gchar *.
 *
 * Sets the methods of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_methods (
        GSignondIdentityInfo *info,
        GHashTable *methods)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    gchar **items = NULL;
    GVariantBuilder builder;
    GVariant *current_mehtods, *var_methods;

    GHashTableIter iter;
    const gchar *method;
    GSequence *mechanisms = NULL;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (methods != NULL, FALSE);

    g_variant_builder_init (&builder, (const GVariantType *)"a{sas}");
    g_hash_table_iter_init (&iter, methods);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&method,
                                   (gpointer)&mechanisms))
    {
        items = _gsignond_identity_info_sequence_to_array (mechanisms);
        g_variant_builder_add (&builder, "{s^as}", method, items);
        g_free (items);
    }

    var_methods = g_variant_builder_end (&builder);
    current_mehtods = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_AUTHMETHODS);

    if (current_mehtods != NULL &&
        g_variant_equal (current_mehtods, var_methods) == TRUE) {
        g_variant_unref (var_methods);
        return TRUE;
    }

    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_AUTHMETHODS,
                var_methods) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_METHODS);
}

/**
 * gsignond_identity_info_get_mechanisms:
 * @info: instance of #GSignondIdentityInfo
 *
 * @method: the method for which mechanisms are sought
 *
 * Retrieves the mechanisms from the info.
 *
 * Returns: (transfer full): the mechanisms if successful, NULL otherwise.
 * when done, mechanisms should be freed using g_sequence_free; #GSequence is a
 * sequence of gchar *.
 */
GSequence *
gsignond_identity_info_get_mechanisms (
        GSignondIdentityInfo *info,
        const gchar *method)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    GVariant *var = NULL;
    GSequence *mechanisms = NULL;

    g_return_val_if_fail (method != NULL, NULL);

    var = gsignond_dictionary_get (info->map,
                GSIGNOND_IDENTITY_INFO_AUTHMETHODS);
    if (var != NULL) {
        GVariantIter iter;
        gchar *vmethod;
        gchar **vmechanisms;

        g_variant_iter_init (&iter, var);
        while (g_variant_iter_next (&iter, "{s^as}", &vmethod, &vmechanisms))
        {
            /* ownership of content is transferred */
            if (vmethod != NULL && g_strcmp0 (vmethod, method) == 0) {
                mechanisms = _gsignond_identity_info_array_to_sequence (
                                 vmechanisms);
                g_free (vmethod);
                g_free (vmechanisms);
                break;
            }
            g_free (vmethod); vmethod = NULL;
            g_strfreev (vmechanisms); vmechanisms = NULL;
        }
    }
    return mechanisms;
}

/**
 * gsignond_identity_info_remove_method:
 * @info: instance of #GSignondIdentityInfo
 *
 * Removes the method from the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_remove_method (
        GSignondIdentityInfo *info,
        const gchar *method)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    GHashTable *methods = NULL;
    gboolean ret = FALSE;

    g_return_val_if_fail (method != NULL, FALSE);

    methods = gsignond_identity_info_get_methods (info);
    if (methods && g_hash_table_remove (methods, method)) {
        ret = gsignond_identity_info_set_methods (info, methods);
    }
    if (methods)
        g_hash_table_unref (methods);
    return ret;
}

/**
 * gsignond_identity_info_get_access_control_list:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the access control list from the info.
 *
 * Returns: (transfer full): the list if successful, NULL otherwise.
 * when done, list should be freed using gsignond_security_context_list_free.
 */
GSignondSecurityContextList *
gsignond_identity_info_get_access_control_list (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_ACL);
    return var ? gsignond_security_context_list_from_variant (var) : NULL;
}

/**
 * gsignond_identity_info_set_access_control_list:
 * @info: instance of #GSignondIdentityInfo
 *
 * @acl: (transfer none): access control list to be set
 *
 * Sets the access control list of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_access_control_list (
        GSignondIdentityInfo *info,
        const GSignondSecurityContextList *acl)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    GVariant *current_acl = gsignond_dictionary_get (info->map,
                              GSIGNOND_IDENTITY_INFO_ACL);
    GVariant *var_acl = NULL;

    if (!current_acl && !acl) return TRUE;

    var_acl = gsignond_security_context_list_to_variant (acl);
    if (current_acl != NULL &&
        g_variant_equal (current_acl, var_acl) == TRUE) {
        g_variant_unref (var_acl);
        return TRUE;
    }

    g_return_val_if_fail (acl != NULL, FALSE);
    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_ACL, var_acl) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_ACL);
}

/**
 * gsignond_identity_info_get_owner:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the id from the info.
 *
 * Returns: (transfer full): the owner if successful, NULL otherwise.
 * when done, owner list should be freed using
 * gsignond_security_context_free.
 */
GSignondSecurityContext *
gsignond_identity_info_get_owner (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    GVariant *var = gsignond_dictionary_get (info->map,
                         GSIGNOND_IDENTITY_INFO_OWNER);
    return var ? gsignond_security_context_from_variant (var) : NULL;
}

/**
 * gsignond_identity_info_set_owner:
 * @info: instance of #GSignondIdentityInfo
 *
 * @owner: (transfer none): owner to be set
 *
 * Sets the owner of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_owner (
        GSignondIdentityInfo *info,
        const GSignondSecurityContext *owner)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    g_return_val_if_fail (owner != NULL, FALSE);
    GSignondSecurityContext *current_owner = 
        gsignond_identity_info_get_owner (info);

    if (current_owner != NULL &&
        gsignond_security_context_compare (current_owner, owner) == 0)
        return TRUE;

    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_OWNER,
                gsignond_security_context_to_variant (owner)) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_OWNER);
}

/**
 * gsignond_identity_info_get_validated:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the validated flag from the info.
 *
 * Returns: the validated flag.
 */
gboolean
gsignond_identity_info_get_validated (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_VALIDATED);
    return var ? g_variant_get_boolean (var) : FALSE;
}

/**
 * gsignond_identity_info_set_validated:
 * @info: instance of #GSignondIdentityInfo
 *
 * @validated: validated flag to be set
 *
 * Sets the validated flag of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_validated (
        GSignondIdentityInfo *info,
        gboolean validated)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    if (gsignond_identity_info_get_validated (info) == validated)
        return TRUE;

    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_VALIDATED,
                g_variant_new_boolean (validated)) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_VALIDATED);
}

/**
 * gsignond_identity_info_get_identity_type:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the type from the info.
 *
 * Returns: the type; negative type is returned in case of failure.
 */
guint32
gsignond_identity_info_get_identity_type (GSignondIdentityInfo *info)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), 0);

    GVariant *var = gsignond_dictionary_get (info->map,
                        GSIGNOND_IDENTITY_INFO_TYPE);
    return var ? g_variant_get_int32 (var) : 0;
}

/**
 * gsignond_identity_info_set_identity_type:
 * @info: instance of #GSignondIdentityInfo
 *
 * @type: type to be set
 *
 * Sets the type of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_identity_type (
        GSignondIdentityInfo *info,
        guint32 type)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);

    if (gsignond_identity_info_get_identity_type (info) == type)
        return TRUE;

    return gsignond_dictionary_set (info->map,
                GSIGNOND_IDENTITY_INFO_TYPE,
                g_variant_new_int32 (type)) &&
           gsignond_identity_info_set_edit_flags (info,
                IDENTITY_INFO_PROP_TYPE);
}

/**
 * gsignond_identity_info_compare:
 * @info: instance1 of #GSignondIdentityInfo
 *
 * @other: instance2 of #GSignondIdentityInfo
 *
 * Compares two instances of #GSignondIdentityInfo for equality.
 *
 * Returns: TRUE if the two instances are equal, FALSE otherwise.
 */
gboolean
gsignond_identity_info_compare (
        GSignondIdentityInfo *info,
        GSignondIdentityInfo *other)
{
    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), FALSE);
    g_return_val_if_fail (other && GSIGNOND_IS_IDENTITY_INFO (other), FALSE);

    GSequence *info_realms = NULL, *other_realms = NULL;
    GHashTable *info_methods = NULL, *other_methods = NULL;
    GSignondSecurityContextList *info_acl = NULL, *other_acl = NULL;
    GSignondSecurityContext *info_owner = NULL, *other_owner = NULL;
    gboolean equal = FALSE;

    if (info == other)
        return TRUE;

    if (info == NULL || other == NULL)
        return FALSE;

    if (gsignond_identity_info_get_id (info) !=
        gsignond_identity_info_get_id (other)) {
        return FALSE;
    }

    if (g_strcmp0 (gsignond_identity_info_get_username (info),
        gsignond_identity_info_get_username (other)) != 0) {
        return FALSE;
    }

    if (g_strcmp0 (gsignond_identity_info_get_secret (info),
        gsignond_identity_info_get_secret (other)) != 0) {
        return FALSE;
    }

    if (gsignond_identity_info_get_store_secret (info) !=
        gsignond_identity_info_get_store_secret (other)) {
        return FALSE;
    }

    if (g_strcmp0 (gsignond_identity_info_get_caption (info),
        gsignond_identity_info_get_caption (other)) != 0) {
        return FALSE;
    }

    info_realms = gsignond_identity_info_get_realms (info);
    other_realms = gsignond_identity_info_get_realms (other);
    equal = _gsignond_identity_info_seq_cmp (info_realms, other_realms);
    if (info_realms) g_sequence_free (info_realms);
    if (other_realms) g_sequence_free (other_realms);
    if (!equal) {
        return FALSE;
    }

    info_methods = gsignond_identity_info_get_methods (info);
    other_methods = gsignond_identity_info_get_methods (other);
    equal = _gsignond_identity_info_methods_cmp (info_methods, other_methods);
    if (info_methods) g_hash_table_unref (info_methods);
    if (other_methods) g_hash_table_unref (other_methods);
    if (!equal) {
        return FALSE;
    }

    info_acl = gsignond_identity_info_get_access_control_list (info);
    if (info_acl)
        info_acl = g_list_sort (
                        info_acl,
                        (GCompareFunc)gsignond_security_context_compare);
    other_acl = gsignond_identity_info_get_access_control_list (other);
    if (other_acl)
        other_acl = g_list_sort (
                        other_acl,
                        (GCompareFunc)gsignond_security_context_compare);
    equal = _gsignond_identity_info_sec_context_list_cmp (info_acl, other_acl);
    if (info_acl) gsignond_security_context_list_free (info_acl);
    if (other_acl) gsignond_security_context_list_free (other_acl);
    if (!equal) {
        return FALSE;
    }

    info_owner = gsignond_identity_info_get_owner (info);
    other_owner = gsignond_identity_info_get_owner (other);
    equal = gsignond_security_context_match (info_owner, other_owner);
    if (info_owner) gsignond_security_context_free (info_owner);
    if (other_owner) gsignond_security_context_free (other_owner);
    if (!equal) {
        return FALSE;
    }

    if (gsignond_identity_info_get_validated (info) !=
        gsignond_identity_info_get_validated (other)) {
        return FALSE;
    }

    if (gsignond_identity_info_get_identity_type (info) !=
        gsignond_identity_info_get_identity_type (other)) {
        return FALSE;
    }

    return TRUE;
}

/**
 * gsignond_identity_info_to_variant:
 * @info: instance of #GSignondIdentityInfo
 *
 * Converts the #GSignondIndentityInfo to a #GVariant.
 *
 * Returns: (transfer full): #GVariant object if successful, NULL otherwise.
 */
GVariant *
gsignond_identity_info_to_variant (GSignondIdentityInfo *info)
{
    gboolean username_is_secret ;

    g_return_val_if_fail (info && GSIGNOND_IS_IDENTITY_INFO (info), NULL);

    username_is_secret = gsignond_identity_info_get_is_username_secret (info);

    if (username_is_secret)
        return gsignond_dictionary_to_variant (info->map);
    else {
        GVariant *vdict = NULL;
        GVariantBuilder *builder = 
                gsignond_dictionary_to_variant_builder (info->map);
        if (!builder) return NULL;

        g_variant_builder_add (builder, "{sv}", GSIGNOND_IDENTITY_INFO_USERNAME,
                g_variant_new_string (info->username ? info->username : ""));

        vdict = g_variant_builder_end (builder);

        g_variant_builder_unref (builder);

        return vdict;
    }
}

void
gsignond_identity_info_list_free (GSignondIdentityInfoList *list)
{
    g_return_if_fail (list != NULL);
    g_list_free_full (list, (GDestroyNotify)gsignond_identity_info_unref);
}
