/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
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

#include <gsignond/gsignond-identity-info.h>
#include "gsignond-identity-info-internal.h"


void
_gsignond_identity_info_free_array (gchar ** array)
{
    gint i;
    i = 0;
    while (array[i]) {
        g_free (array[i]);
    }
    g_free (array);
}

static gboolean
_gsignond_identity_info_seq_cmp (
        GSequence *one,
        GSequence *two)
{
    GSequenceIter *iter1 = NULL, *iter2 = NULL;
    gboolean equal = TRUE;

    if (one == NULL && two == NULL)
        return TRUE;

    if ((one != NULL && two == NULL) ||
        (one == NULL && two != NULL) ||
        (g_sequence_get_length (one) != g_sequence_get_length (two)))
        return FALSE;

    if (one == two)
        return TRUE;

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

static GSequence *
_gsignond_identity_info_array_to_sequence (const gchar *const * array)
{
    GSequence *seq = NULL;
    gint i;

    seq = g_sequence_new ((GDestroyNotify)g_free);
    i = 0;
    while (array[i]) {
        g_sequence_insert_sorted (seq, g_strdup (array[i]),
        		(GCompareDataFunc)_compare_strings, NULL);
        ++i;
    }
    return seq;
}

static GVariant *
_gsignond_identity_info_sequence_to_variant (GSequence *seq)

{
    GSequenceIter * iter = NULL;
    GVariant *var = NULL;
    GVariantBuilder builder;
    const gchar *item = NULL;

    if (!seq) return NULL;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_STRING_ARRAY);
    iter = g_sequence_get_begin_iter (seq);
    while (!g_sequence_iter_is_end (iter)) {
        const gchar * d = g_sequence_get (iter);
        g_variant_builder_add (&builder, "s", d);
        iter = g_sequence_iter_next (iter);
    }
    var = g_variant_builder_end (&builder);
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
    while (g_variant_iter_next (&iter, "s", &item))
    {
        g_sequence_insert_sorted (seq, item,
                        (GCompareDataFunc)_compare_strings, NULL);
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

/**
 * gsignond_identity_info_new:
 *
 * Creates new instance of GSignondIdentityInfo.
 *
 * Returns: (transfer full) #GSignondIdentityInfo object if successful,
 * NULL otherwise.
 */
GSignondIdentityInfo *
gsignond_identity_info_new (void)
{
    return g_hash_table_new_full ((GHashFunc)g_str_hash,
                            (GEqualFunc)g_str_equal,
                            NULL,
                            (GDestroyNotify)g_variant_unref);
}

/**
 * gsignond_identity_info_free:
 * @info: instance of #GSignondIdentityInfo
 *
 * Frees the memory allocated by info structure.
 *
 */
void
gsignond_identity_info_free (GSignondIdentityInfo *info)
{
    g_return_if_fail (info != NULL);
    g_hash_table_unref (info);
}

/**
 * gsignond_identity_info_copy:
 * @info: instance of #GSignondIdentityInfo
 *
 * Creates a copy of the info.
 *
 * Returns: (transfer full) #GSignondIdentityInfo object if successful,
 * NULL otherwise.
 */
GSignondIdentityInfo *
gsignond_identity_info_copy (GSignondIdentityInfo *other)
{
    GSignondIdentityInfo *info = NULL;
    GHashTable *methods = NULL;
    GSignondSecurityContextList *owners = NULL, *acl = NULL;
    GSequence *realms = NULL;

    const gchar *str = NULL;

    g_return_val_if_fail (other != NULL, NULL);

    info = gsignond_identity_info_new ();

    gsignond_identity_info_set_id (info, gsignond_identity_info_get_id (other));

    str = gsignond_identity_info_get_username (other);
    if (str) {
        gsignond_identity_info_set_username (info, str);
    }

    str = gsignond_identity_info_get_secret (other);
    if (str) {
        gsignond_identity_info_set_secret (info, str);
    }

    gsignond_identity_info_set_store_secret (info,
            gsignond_identity_info_get_store_secret (other));

    str = gsignond_identity_info_get_caption(other);
    if (str) {
        gsignond_identity_info_set_caption (info, str);
    }

    realms = gsignond_identity_info_get_realms (other);
    if (realms) {
        gsignond_identity_info_set_realms (info, realms);
        g_sequence_free (realms);
    }

    methods = gsignond_identity_info_get_methods (other);
    if (methods) {
        gsignond_identity_info_set_methods (info, methods);
        g_hash_table_unref (methods);
    }

    acl = gsignond_identity_info_get_access_control_list (other);
    if (acl) {
        gsignond_identity_info_set_access_control_list (info, acl);
        gsignond_security_context_list_free (acl);
    }

    owners = gsignond_identity_info_get_owner_list (other);
    if (owners) {
        gsignond_identity_info_set_owner_list (info, owners);
        gsignond_security_context_list_free (owners);
    }

    gsignond_identity_info_set_validated (info,
            gsignond_identity_info_get_validated (other));

    gsignond_identity_info_set_identity_type (info,
            gsignond_identity_info_get_identity_type (other));

    return info;
}


/**
 * gsignond_identity_info_new_from_variant:
 * @variant: instance of #GVariant
 *
 * Converts the variant to GSignondIdentityInfo.
 *
 * Returns: (transfer full) object if successful, NULL otherwise.
 */
GSignondIdentityInfo *
gsignond_identity_info_new_from_variant (GVariant *variant)
{
    GSignondIdentityInfo *info = NULL;
    GVariantIter iter;
    gchar *key = NULL;
    GVariant *value = NULL;

    g_return_val_if_fail (variant != NULL, NULL);

    info = gsignond_identity_info_new ();
    g_variant_iter_init (&iter, variant);
    while (g_variant_iter_next (&iter, "{sv}", &key, &value))
    {
        g_hash_table_insert (info, key, value);
    }

    return info;
}

/**
 * gsignond_identity_info_to_variant:
 * @info: instance of #GSignondIdentityInfo
 *
 * Converts the GSignondIdentityInfo to variant.
 *
 * Returns: (transfer full) #GVariant object if successful, NULL otherwise.
 */
GVariant *
gsignond_identity_info_to_variant (GSignondIdentityInfo *info)
{
    GVariantBuilder builder;
    GHashTableIter iter;
    GVariant *vinfo = NULL;
    const gchar *key = NULL;
    GVariant *value = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
    g_hash_table_iter_init (&iter, info);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&key,
                                   (gpointer)&value))
    {
        g_variant_builder_add (&builder, "{sv}",
                               key,
                               value);
    }
    vinfo = g_variant_builder_end (&builder);
    return vinfo;
}

/**
 * gsignond_identity_info_get_id:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the id from the info.
 *
 * Returns: the id; negative id is returned in case of failure.
 */
gint
gsignond_identity_info_get_id (GSignondIdentityInfo *info)
{
    GVariant *var = NULL;
    gint id = -1;

    g_return_val_if_fail (info != NULL, -1);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_ID);
    if (var != NULL) {
        id = g_variant_get_int32 (var);
    }

    return id;
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
        gint id)
{
    GVariant *vid = NULL;

    g_return_val_if_fail (info != NULL, -1);

    vid = g_variant_new_int32 (id);
    g_variant_ref_sink(vid);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_ID,
            vid);

    return TRUE;
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
    g_return_val_if_fail (info != NULL, -1);

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
    g_return_val_if_fail (info != NULL, -1);

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
    GVariant *var = NULL;
    const gchar *name = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_USERNAME);
    if (var != NULL) {
        name = g_variant_get_string (var, NULL);
    }

    return name;
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
    GVariant *vname = NULL;
    g_return_val_if_fail (info != NULL, FALSE);

    if (username) {
        vname = g_variant_new_string (username);
        g_variant_ref_sink(vname);
        g_hash_table_replace (
                info,
                GSIGNOND_IDENTITY_INFO_USERNAME,
                vname);
    }

    return TRUE;
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
    GVariant *var = NULL;
    gboolean is_username_secret = FALSE;

    g_return_val_if_fail (info != NULL, -1);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET);
    if (var != NULL) {
        is_username_secret = g_variant_get_boolean (var);
    }

    return is_username_secret;
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
    GVariant *vusername_secret = NULL;

    g_return_val_if_fail (info != NULL, FALSE);

    vusername_secret = g_variant_new_boolean(username_secret);
    g_variant_ref_sink(vusername_secret);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET,
            vusername_secret);

    return TRUE;
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
    GVariant *var = NULL;
    const gchar *secret = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_SECRET);
    if (var != NULL) {
        secret = g_variant_get_string (var, NULL);
    }

    return secret;
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
    GVariant *vsecret = NULL;
    g_return_val_if_fail (info != NULL, FALSE);

    if (secret) {
        vsecret = g_variant_new_string (secret);
        g_variant_ref_sink(vsecret);
        g_hash_table_replace (
                info,
                GSIGNOND_IDENTITY_INFO_SECRET,
                vsecret);
    }
    return TRUE;
}

/**
 * gsignond_identity_info_get_store_secret:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the store_secret flag from the info.
 *
 * Returns: the store_secret flage.
 */
gboolean
gsignond_identity_info_get_store_secret (GSignondIdentityInfo *info)
{
    GVariant *var = NULL;
    gboolean store_secret = FALSE;

    g_return_val_if_fail (info != NULL, FALSE);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_STORESECRET);
    if (var != NULL) {
        store_secret = g_variant_get_boolean (var);
    }

    return store_secret;
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
    GVariant *vstore_secret = NULL;

    g_return_val_if_fail (info != NULL, FALSE);

    vstore_secret = g_variant_new_boolean(store_secret);
    g_variant_ref_sink(vstore_secret);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_STORESECRET,
            vstore_secret);

    return TRUE;
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
    GVariant *var = NULL;
    const gchar *caption = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_CAPTION);
    if (var != NULL) {
        caption = g_variant_get_string (var, NULL);
    }

    return caption;
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
    GVariant *vcaption = NULL;
    g_return_val_if_fail (info != NULL, FALSE);

    if (caption) {
        vcaption = g_variant_new_string (caption);
        g_variant_ref_sink(vcaption);
        g_hash_table_replace (
                info,
                GSIGNOND_IDENTITY_INFO_CAPTION,
                vcaption);
    }

    return TRUE;
}

/**
 * gsignond_identity_info_get_realms:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the realms from the info.
 *
 * Returns: (transfer full) the realms if successful, NULL Otherwise.
 * when done realms should be freed using g_sequence_free.
 */
GSequence *
gsignond_identity_info_get_realms (GSignondIdentityInfo *info)
{
    GVariant *var = NULL;
    const gchar *const *realms = NULL;
    GSequence *out = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_REALMS);
    if (var != NULL) {
        out = _gsignond_identity_info_variant_to_sequence (var);
    }

    return out;
}

/**
 * gsignond_identity_info_set_realms:
 * @info: instance of #GSignondIdentityInfo
 *
 * @realms: (transfer none) realms to be set
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
    GVariant *vrealms = NULL;
    GSequenceIter *iter = NULL;
    GVariantBuilder builder;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (realms != NULL, FALSE);

    vrealms = _gsignond_identity_info_sequence_to_variant (realms);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_REALMS,
            vrealms);

    return TRUE;
}

/**
 * gsignond_identity_info_get_methods:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the methods from the info whereas #GHashTable consists of
 * <gchar*,GSequence*> and #GSequence is a sequence of gchar *.
 *
 * Returns: (transfer full) the methods if successful, NULL otherwise.
 * when done, methods should be freed using g_hash_table_unref.
 */
GHashTable *
gsignond_identity_info_get_methods (GSignondIdentityInfo *info)
{
    GVariant *var = NULL;
    GHashTable *methods = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_AUTHMETHODS);
    if (var != NULL) {
        GVariantIter iter;
        gchar *vmethod;
        GVariant *vmechanisms = NULL;
        GSequence *seq = NULL;

        methods = g_hash_table_new_full ((GHashFunc)g_str_hash,
                                (GEqualFunc)g_str_equal,
                                (GDestroyNotify)g_free,
                                (GDestroyNotify)g_sequence_free);

        g_variant_iter_init (&iter, var);
        while (g_variant_iter_next (&iter, "{sv}", &vmethod, &vmechanisms))
        {
            seq = _gsignond_identity_info_variant_to_sequence (vmechanisms);
            /*vmethod ownership is transferred to methods*/
            g_hash_table_insert (methods, vmethod, seq);
            g_variant_unref (vmechanisms);
        }
    }

    return methods;
}

/**
 * gsignond_identity_info_set_methods:
 * @info: instance of #GSignondIdentityInfo
 *
 * @methods: (transfer none) methods to be set whereas #GHashTable consists of
 * <gchar*,#GSequence*> and #GSequence is a sequence of gchar *.
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
    GVariant *var = NULL;
    GVariant *vmethod_map = NULL;
    GVariantBuilder builder;

    GHashTableIter iter;
    const gchar *method;
    GSequence *mechanisms = NULL;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (methods != NULL, FALSE);

    g_variant_builder_init (&builder, (const GVariantType *)"a{sv}");
    g_hash_table_iter_init (&iter, methods);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&method,
                                   (gpointer)&mechanisms))
    {
        var = _gsignond_identity_info_sequence_to_variant (mechanisms);
        g_variant_builder_add (&builder, "{sv}", method, var);
    }
    vmethod_map = g_variant_builder_end (&builder);

    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_AUTHMETHODS,
            vmethod_map);

    return TRUE;
}

/**
 * gsignond_identity_info_get_mechanisms:
 * @info: instance of #GSignondIdentityInfo
 *
 * @method: the method for which mechanisms are sought
 *
 * Retrieves the mechanisms from the info.
 *
 * Returns: (transfer full) the mechanisms if successful, NULL otherwise.
 * when done, mechanisms should be freed using g_sequence_free; #GSequence is a
 * sequence of gchar *.
 */
GSequence *
gsignond_identity_info_get_mechanisms (
        GSignondIdentityInfo *info,
        const gchar *method)
{
    GVariant *var = NULL;
    GSequence *mechanisms = NULL;

    g_return_val_if_fail (info != NULL, NULL);
    g_return_val_if_fail (method != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_AUTHMETHODS);
    if (var != NULL) {
        GVariantIter iter;
        gchar *vmethod;
        GVariant *vmechanisms;

        g_variant_iter_init (&iter, var);
        while (g_variant_iter_next (&iter, "{sv}", &vmethod, &vmechanisms))
        {
            if (vmethod != NULL && g_strcmp0 (vmethod, method) == 0) {
                mechanisms = _gsignond_identity_info_variant_to_sequence (
                                 vmechanisms);
                g_free (vmethod);
                g_variant_unref (vmechanisms);
                break;
            }
            g_free (vmethod);
            g_variant_unref (vmechanisms);
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
    GVariant *var = NULL;
    GHashTable *methods = NULL;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (method != NULL, FALSE);

    methods = gsignond_identity_info_get_methods (info);
    if (!g_hash_table_remove (methods, method)) {
        return FALSE;
    }
    return gsignond_identity_info_set_methods (info, methods);
}

/**
 * gsignond_identity_info_get_access_control_list:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the access control list from the info.
 *
 * Returns: (transfer full) the list if successful, NULL otherwise.
 * when done, list should be freed using gsignond_security_context_list_free.
 */
GSignondSecurityContextList *
gsignond_identity_info_get_access_control_list (GSignondIdentityInfo *info)
{
    GVariant *var = NULL;
    GSignondSecurityContextList *acl = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_ACL);
    if (var != NULL) {
        acl = gsignond_security_context_list_from_variant (var);
    }

    return acl;
}

/**
 * gsignond_identity_info_set_access_control_list:
 * @info: instance of #GSignondIdentityInfo
 *
 * @acl: (transfer none) access control list to be set
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
    GVariant *vacl = NULL;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (acl != NULL, FALSE);

    vacl = gsignond_security_context_list_to_variant (acl);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_ACL,
            vacl);

    return TRUE;
}

/**
 * gsignond_identity_info_get_owner_list:
 * @info: instance of #GSignondIdentityInfo
 *
 * Retrieves the id from the info.
 *
 * Returns: (transfer full) the owner list if successful, NULL otherwise.
 * when done, owner list should be freed using
 * gsignond_security_context_list_free.
 */
GSignondSecurityContextList *
gsignond_identity_info_get_owner_list (GSignondIdentityInfo *info)
{
    GVariant *var = NULL;
    GSignondSecurityContextList *owners = NULL;

    g_return_val_if_fail (info != NULL, NULL);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_OWNER);
    if (var != NULL) {
        owners = gsignond_security_context_list_from_variant (var);
    }

    return owners;
}

/**
 * gsignond_identity_info_set_owner_list:
 * @info: instance of #GSignondIdentityInfo
 *
 * @owners: (transfer none) owner list to be set
 *
 * Sets the owner list of the info.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_set_owner_list (
        GSignondIdentityInfo *info,
        const GSignondSecurityContextList *owners)
{
    GVariant *vowners = NULL;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (owners != NULL, FALSE);

    vowners = gsignond_security_context_list_to_variant (owners);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_OWNER,
            vowners);

    return TRUE;
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
    GVariant *var = NULL;
    gboolean validated = FALSE;

    g_return_val_if_fail (info != NULL, FALSE);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_VALIDATED);
    if (var != NULL) {
        validated = g_variant_get_boolean (var);
    }

    return validated;
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
    GVariant *val = NULL;

    g_return_val_if_fail (info != NULL, FALSE);

    val = g_variant_new_boolean (validated);
    g_variant_ref_sink(val);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_VALIDATED,
            val);

    return TRUE;
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
    GVariant *var = NULL;
    gint type = -1;

    g_return_val_if_fail (info != NULL, 0);

    var = g_hash_table_lookup (info, GSIGNOND_IDENTITY_INFO_TYPE);
    if (var != NULL) {
        type = g_variant_get_int32 (var);
    }

    return type;
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
    GVariant *vtype = NULL;

    g_return_val_if_fail (info != NULL, FALSE);

    vtype = g_variant_new_int32 (type);
    g_variant_ref_sink(vtype);
    g_hash_table_replace (
            info,
            GSIGNOND_IDENTITY_INFO_TYPE,
            vtype);

    return TRUE;
}

/**
 * gsignond_identity_info_check_method_mechanism:
 * @info: instance of #GSignondIdentityInfo
 *
 * @method: the method to be checked for.
 *
 * @mechanism: the mechanism(s) to be checked for.
 *
 * @allowed_mechanisms: (out) string of the allowed mechanisms
 *
 * Checks which mechanisms are allowed for the given method and returns the
 * allowed mechanism(s) in the string.
 *
 * Returns: TRUE if check is successful, FALSE otherwise.
 */
gboolean
gsignond_identity_info_check_method_mechanism (
        GSignondIdentityInfo *info,
        const gchar *method,
        const gchar *mechanism,
        gchar **allowed_mechanisms)
{
    GSequence *mechanisms = NULL;
    GSequenceIter *iter = NULL;
    gchar ** split_mechs = NULL;
    GString* allowed_mechs = NULL;
    gint i, j=0;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (method != NULL && mechanism != NULL, FALSE);

    mechanisms = gsignond_identity_info_get_mechanisms (info, method);

    if (mechanisms == NULL) {
        return TRUE;
    }

    if (g_sequence_lookup (mechanisms, (gpointer)mechanism,
            (GCompareDataFunc) g_strcmp0, NULL) != NULL) {
        *allowed_mechanisms = g_strdup (mechanism);
        g_sequence_free (mechanisms);
        return TRUE;
    }

    split_mechs = g_strsplit (mechanism, (const gchar *)' ', 0);
    if (g_strv_length (split_mechs) <= 1 ) {
        g_sequence_free (mechanisms);
        return FALSE;
    }

    allowed_mechs = g_string_new (NULL);
    i = 0;
    while (split_mechs[i]) {
        if (g_sequence_lookup (mechanisms, (gpointer)split_mechs[i],
                (GCompareDataFunc)g_strcmp0, NULL) != NULL) {
            if (j > 0) {
                g_string_append (allowed_mechs, (const gchar *)' ');
            }
            g_string_append (allowed_mechs, split_mechs[i]);
            ++j;
        }
        ++i;
    }
    g_strfreev (split_mechs);
    g_sequence_free (mechanisms);

    *allowed_mechanisms = g_string_free (allowed_mechs, FALSE);
    return TRUE;
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
    GSequence *info_realms = NULL, *other_realms = NULL;
    GHashTable *info_methods = NULL, *other_methods = NULL;
    GSignondSecurityContextList *info_acl = NULL, *other_acl = NULL;
    GSignondSecurityContextList *info_owners = NULL, *other_owners = NULL;
    gboolean equal = FALSE;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (other != NULL, FALSE);

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
    other_acl = gsignond_identity_info_get_owner_list (other);
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

    info_owners = gsignond_identity_info_get_owner_list (info);
    if (info_owners)
        info_owners = g_list_sort (
                        info_owners,
                        (GCompareFunc)gsignond_security_context_compare);
    other_owners = gsignond_identity_info_get_owner_list (other);
    if (other_owners)
        other_owners = g_list_sort (
                        other_owners,
                        (GCompareFunc)gsignond_security_context_compare);
    equal = _gsignond_identity_info_sec_context_list_cmp (info_owners,
            other_owners);
    if (info_owners) gsignond_security_context_list_free (info_owners);
    if (other_owners) gsignond_security_context_list_free (other_owners);
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

void
gsignond_identity_info_list_free (GSignondIdentityInfoList *list)
{
    g_return_if_fail (list != NULL);

    g_list_free_full (list, (GDestroyNotify)gsignond_identity_info_free);
}

