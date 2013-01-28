/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
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

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <glib.h>
#include <gio/gio.h>
#include <sqlite3.h>

#include <gsignond/gsignond-config.h>
#include <gsignond/gsignond-log.h>
#include <gsignond/gsignond-credentials.h>
#include <gsignond/gsignond-secret-storage.h>
#include <daemon/gsignond-daemon.h>

#include <daemon/db/gsignond-db-metadata-database.h>
#include <daemon/db/gsignond-db-credentials-database.h>
#include <daemon/db/gsignond-db-secret-cache.h>

static GSequence*
_sequence_new (guint8 *data)
{
    GSequence *value = NULL;
    value = g_sequence_new (NULL);
    g_sequence_append (value, data);
    return value;
}

typedef struct {
    GHashTable *table;
    int status;
} Data;

void
_compare_key_value(
        gchar *key,
        GBytes *value,
        Data *user_data)
{
    GBytes *value2 = NULL;
    value2 = (GBytes *)g_hash_table_lookup (user_data->table, key);
    if (value2 && g_bytes_compare (value2, value) == 0)
        return;
    user_data->status = 0;
}

static gboolean
_compare_sequences (
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

GSignondIdentityInfo *
get_filled_identity_info (guint32 identity_id)
{
    guint32 id = 0;
    guint32 type = 456;
    const gchar *username = "username1";
    const gchar *secret = "secret1";
    const gchar *caption = "caption1";
    const gchar *method1 = "method1";
    GSignondIdentityInfo *identity = NULL;
    GSignondSecurityContextList *ctx_list;
    GSignondSecurityContext *ctx, *ctx1, *ctx2, *ctx3 ;
    GHashTable *methods = NULL;
    GSequence *seq1 = NULL, *seq_realms;

    identity = gsignond_identity_info_new ();
    gsignond_identity_info_set_identity_new (identity);
    gsignond_identity_info_set_username (identity, username);
    gsignond_identity_info_set_secret (identity, secret);
    gsignond_identity_info_set_store_secret (identity, TRUE);
    gsignond_identity_info_set_caption (identity, caption);

    /*realms*/
    seq_realms = _sequence_new("realms1");
    gsignond_identity_info_set_realms (identity, seq_realms);
    g_sequence_free (seq_realms);

    /*methods*/
    methods = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_sequence_free);
    seq1 = _sequence_new("mech11"); g_sequence_append (seq1, "mech12");
    g_hash_table_insert (methods, "method1", seq1);
    g_hash_table_insert (methods, "method2", _sequence_new("mech21"));
    g_hash_table_insert (methods, "method3", _sequence_new("mech31"));
    gsignond_identity_info_set_methods (identity, methods);
    g_hash_table_unref (methods);

    /*acl*/
    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    ctx2 = gsignond_security_context_new_from_values ("sysctx2", "appctx2");
    ctx3 = gsignond_security_context_new_from_values ("sysctx3", "appctx3");
    ctx_list = g_list_append (ctx_list,ctx1);
    ctx_list = g_list_append (ctx_list,ctx2);
    ctx_list = g_list_append (ctx_list,ctx3);
    gsignond_identity_info_set_access_control_list (identity, ctx_list);

    /*owners*/
    gsignond_identity_info_set_owner_list (identity, ctx_list);
    gsignond_security_context_list_free (ctx_list);

    gsignond_identity_info_set_validated (identity, FALSE);
    gsignond_identity_info_set_identity_type (identity, type);
    return identity;
}

void
test_identity_info ()
{
    guint32 id = 125;
    guint32 type = 456;

    const gchar *username = "username1";
    const gchar *secret = "secret1";
    const gchar *caption = "caption1";

    const gchar *method1 = "method1";
    GSignondIdentityInfo *identity = NULL;
    GSignondIdentityInfo *identity2 = NULL;
    GSignondSecurityContextList *ctx_list, *list;
    GSignondSecurityContext *ctx, *ctx1, *ctx2, *ctx3 ;
    GHashTable *methods = NULL, *methods2;
    GSequence *seq1 = NULL, *seq_realms;
    gchar *allowmech;

    identity = gsignond_identity_info_new ();
    if (identity) {
        INFO ("IdentityInfo created");
    } else {
        WARN ("IdentityInfo creation FAILED");
        return;
    }

    if (gsignond_identity_info_set_id (identity, id)) {
        INFO ("IdentityInfo id set");
    } else {
        WARN ("IdentityInfo id set FAILED");
    }

    if (id == gsignond_identity_info_get_id (identity)) {
        INFO ("IdentityInfo id get");
    } else {
        WARN ("IdentityInfo id get FAILED");
    }

    if (gsignond_identity_info_set_identity_new (identity)) {
        INFO ("IdentityInfo id set new");
    } else {
        WARN ("IdentityInfo id set new FAILED");
    }

    if (gsignond_identity_info_get_is_identity_new (identity)) {
        INFO ("IdentityInfo id get new");
    } else {
        WARN ("IdentityInfo id get new FAILED");
    }

    if (gsignond_identity_info_set_username (identity, username)) {
        INFO ("IdentityInfo username set");
    } else {
        WARN ("IdentityInfo username set FAILED");
    }

    if (g_strcmp0 (username, gsignond_identity_info_get_username (
            identity)) == 0) {
        INFO ("IdentityInfo username get");
    } else {
        WARN ("IdentityInfo username get FAILED");
    }

    if (gsignond_identity_info_set_username_secret (identity, TRUE)) {
        INFO ("IdentityInfo username_secret set");
    } else {
        WARN ("IdentityInfo username_secret set FAILED");
    }

    if (TRUE == gsignond_identity_info_get_is_username_secret (identity)) {
        INFO ("IdentityInfo username_secret get");
    } else {
        WARN ("IdentityInfo username_secret get FAILED");
    }

    if (gsignond_identity_info_set_secret (identity, secret)) {
        INFO ("IdentityInfo secret set");
    } else {
        WARN ("IdentityInfo secret set FAILED");
    }

    if (g_strcmp0 (secret, gsignond_identity_info_get_secret (identity)) == 0) {
        INFO ("IdentityInfo secret get");
    } else {
        WARN ("IdentityInfo secret get FAILED");
    }

    if (gsignond_identity_info_set_store_secret (identity, TRUE)) {
        INFO ("IdentityInfo store_secret set");
    } else {
        WARN ("IdentityInfo store_secret set FAILED");
    }

    if (TRUE == gsignond_identity_info_get_store_secret (identity)) {
        INFO ("IdentityInfo store_secret get");
    } else {
        WARN ("IdentityInfo store_secret get FAILED");
    }

    if (gsignond_identity_info_set_caption (identity, caption)) {
        INFO ("IdentityInfo caption set");
    } else {
        WARN ("IdentityInfo caption set FAILED");
    }

    if (g_strcmp0 (caption, gsignond_identity_info_get_caption (
            identity)) == 0) {
        INFO ("IdentityInfo caption get");
    } else {
        WARN ("IdentityInfo caption get FAILED");
    }

    /*realms*/
    seq_realms = _sequence_new("realms1");
    if (gsignond_identity_info_set_realms (identity, seq_realms)) {
        INFO ("IdentityInfo realms set");
    } else {
        WARN ("IdentityInfo realms set FAILED");
    }
    seq1 = gsignond_identity_info_get_realms (identity);
    if (seq1 && _compare_sequences (seq1, seq_realms)) {
        INFO ("IdentityInfo realms get");
        g_sequence_free (seq1); seq1 = NULL;
    } else {
        WARN ("IdentityInfo realms get FAILED");
    }
    g_sequence_free (seq_realms);

    /*methods*/
    methods = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_sequence_free);
    seq1 = _sequence_new("mech11"); g_sequence_append (seq1, "mech12");
    g_hash_table_insert (methods, "method1", seq1);
    g_hash_table_insert (methods, "method2", _sequence_new("mech21"));
    g_hash_table_insert (methods, "method3", _sequence_new("mech31"));
    if (gsignond_identity_info_set_methods (identity, methods)) {
        INFO ("IdentityInfo methods set");
    } else {
        WARN ("IdentityInfo methods set FAILED");
    }
    methods2 = gsignond_identity_info_get_methods (identity);
    if (methods2) {
        GSequence *seq21 = NULL;
        seq21 = g_hash_table_lookup (methods, "method1");
        if (!seq21 || !_compare_sequences (seq1,seq21)) {
            WARN ("IdentityInfo methods get FAILED as invalid seq");
        }
        INFO ("IdentityInfo methods get");
        g_hash_table_unref (methods2);
    } else {
        WARN ("IdentityInfo methods get FAILED");
    }
    g_hash_table_unref (methods);

    /*acl*/
    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    ctx2 = gsignond_security_context_new_from_values ("sysctx2", "appctx2");
    ctx3 = gsignond_security_context_new_from_values ("sysctx3", "appctx3");
    ctx_list = g_list_append (ctx_list,ctx1);
    ctx_list = g_list_append (ctx_list,ctx2);
    ctx_list = g_list_append (ctx_list,ctx3);
    if (gsignond_identity_info_set_access_control_list (identity, ctx_list)) {
        INFO ("IdentityInfo acl set");
    } else {
        WARN ("IdentityInfo acl set FAILED");
    }
    list = gsignond_identity_info_get_access_control_list (identity);
    if (list) {
        GList *list2;
        INFO ("IdentityInfo acl get");
        list2 = g_list_nth (list, 0);
        ctx = (GSignondSecurityContext *) list2->data;
        if (gsignond_security_context_compare (ctx, ctx1) != 0) {
            WARN ("IdentityInfo acl get - ctx1 MISMTACH");
        }
        list2 = g_list_nth (list, 1);
        ctx = (GSignondSecurityContext *) list2->data;
        if (gsignond_security_context_compare (ctx, ctx2) != 0) {
            WARN ("IdentityInfo acl get - ctx2 MISMTACH");
        }
        list2 = g_list_nth (list, 2);
        ctx = (GSignondSecurityContext *) list2->data;
        if (gsignond_security_context_compare (ctx, ctx3) != 0) {
            WARN ("IdentityInfo acl get - ctx3 MISMTACH");
        }
        gsignond_security_context_list_free (list); list = NULL;
    } else {
        WARN ("IdentityInfo acl get FAILED");
    }

    /*owners*/
    if (gsignond_identity_info_set_owner_list (identity, ctx_list)) {
        INFO ("IdentityInfo owner set");
    } else {
        WARN ("IdentityInfo owner set FAILED");
    }
    list = gsignond_identity_info_get_owner_list (identity);
    if (list) {
        GList *list2;
        INFO ("IdentityInfo owner get");
        list2 = g_list_nth (list, 0);
        ctx = (GSignondSecurityContext *) list2->data;
        if (gsignond_security_context_compare (ctx, ctx1) != 0) {
            WARN ("IdentityInfo owner get - ctx1 MISMTACH");
        }
        list2 = g_list_nth (list, 1);
        ctx = (GSignondSecurityContext *) list2->data;
        if (gsignond_security_context_compare (ctx, ctx2) != 0) {
            WARN ("IdentityInfo owner get - ctx2 MISMTACH");
        }
        list2 = g_list_nth (list, 2);
        ctx = (GSignondSecurityContext *) list2->data;
        if (gsignond_security_context_compare (ctx, ctx3) != 0) {
            WARN ("IdentityInfo owner get - ctx3 MISMTACH");
        }
        gsignond_security_context_list_free (list); list = NULL;
    } else {
        WARN ("IdentityInfo owner get FAILED");
    }
    gsignond_security_context_list_free (ctx_list);

    if (gsignond_identity_info_set_validated (identity, FALSE)) {
        INFO ("IdentityInfo validated set");
    } else {
        WARN ("IdentityInfo validated set FAILED");
    }

    if (FALSE == gsignond_identity_info_get_validated (identity)) {
        INFO ("IdentityInfo validated get");
    } else {
        WARN ("IdentityInfo validated get FAILED");
    }

    if (gsignond_identity_info_set_identity_type (identity, type)) {
        INFO ("IdentityInfo type set");
    } else {
        WARN ("IdentityInfo type set FAILED");
    }

    if (type == gsignond_identity_info_get_identity_type (identity)) {
        INFO ("IdentityInfo type get");
    } else {
        WARN ("IdentityInfo type get FAILED");
    }

    /*copy*/
    identity2 = gsignond_identity_info_copy (identity);
    if (gsignond_identity_info_compare (identity, identity2)) {
        INFO ("IdentityInfo copy/compare");
    } else {
        WARN ("IdentityInfo copy/compare FAILED");
    }
    gsignond_identity_info_free (identity2);

    if (gsignond_identity_info_check_method_mechanism (identity, "method1",
            "mech11", &allowmech)) {
        if (g_strcmp0 (allowmech, "mech11") == 0) {
            INFO ("IdentityInfo check");
        } else {
            WARN ("IdentityInfo check FAILED as allowed mech does not match");
        }
    } else {
        WARN ("IdentityInfo check FAILED");
    }
    if (allowmech) g_free (allowmech);
}

void
test_credentials_database ()
{
    GSignondConfig *config = NULL;
    guint32 methodid = 0;
    guint32 identity_id = 5;
    const gchar *method1 = "method1";
    GSignondIdentityInfo *identity = NULL, *identity2= NULL;
    GSignondIdentityInfoList *identities = NULL;
    GSignondSecurityContext *ctx1 = NULL;
    GList *methods = NULL, *reflist = NULL;
    GSignondSecurityContextList *acl, *owners;
    GSignondSecurityContext *owner;
    GSignondDbCredentialsDatabase *credentials_db = NULL;
    GSignondSecretStorage *storage =NULL;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    Data input;

    config = gsignond_config_new ();

    storage = g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
            "config", config, NULL);

    credentials_db = gsignond_db_credentials_database_new (config,
            storage);
    g_object_unref (storage);

    if (credentials_db) {
        INFO ("CredentialsDB created");
    } else {
        WARN ("CredentialsDB creation FAILED");
    }

    if (gsignond_db_credentials_database_open_secret_storage (credentials_db)) {
        INFO ("CredentialsDB open_secret_storage");
    } else {
        WARN ("CredentialsDB open_secret_storage FAILED");
    }

    if (gsignond_db_credentials_database_clear (credentials_db)) {
        INFO ("CredentialsDB clear");
    } else {
        WARN ("CredentialsDB clear FAILED");
    }

    identity = get_filled_identity_info (identity_id);

    identity_id = gsignond_db_credentials_database_insert_identity (
            credentials_db, identity, TRUE);
    if (identity_id != 0) {
        INFO ("CredentialsDB insert_identity");
    } else {
        WARN ("CredentialsDB insert_identity FAILED");
    }
    gsignond_identity_info_set_id (identity, identity_id);

    identity2 = gsignond_db_credentials_database_load_identity (credentials_db,
            identity_id, TRUE);
    if (identity2) {
        INFO ("CredentialsDB load_identity");
        gsignond_identity_info_free (identity2);
    } else {
        WARN ("CredentialsDB load_identity FAILED");
    }

    if (gsignond_db_credentials_database_check_secret (credentials_db,
            identity_id, "username1", "secret1")) {
        INFO ("CredentialsDB check_secret");
    } else {
        WARN ("CredentialsDB check_secret FAILED");
    }

    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    methods = gsignond_db_credentials_database_get_methods (credentials_db,
                identity_id, ctx1);
    if (methods) {
        INFO ("CredentialsDB get_methods");
        g_list_free_full (methods, g_free);
    } else {
        WARN ("CredentialsDB get_methods FAILED");
    }

    /* add data to store */
    data = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_bytes_unref);
    g_hash_table_insert (data,"key1",g_bytes_new("value1", strlen ("value1")));
    g_hash_table_insert (data,"key2",g_bytes_new("value2", strlen ("value2")));
    g_hash_table_insert (data,"key3",g_bytes_new("value3", strlen ("value3")));
    g_hash_table_insert (data,"key4",g_bytes_new("value4", strlen ("value4")));
    g_hash_table_insert (data,"key5",g_bytes_new("value5", strlen ("value5")));
    if (gsignond_db_credentials_database_update_data (credentials_db,
            identity_id, "method1", data)) {
        INFO ("CredentialsDB update_data");
    } else {
        WARN ("CredentialsDB update_data FAILED");
    }

    data2 = gsignond_db_credentials_database_load_data (credentials_db,
            identity_id, "method1");
    if (data2) {
        INFO ("CredentialsDB load_data");
        input.table = data;
        input.status = 1;
        g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
        if (input.status != 1) {
            WARN ("CredentialsDB load_data - data DOES NOT MATCH");
        }
        g_hash_table_unref(data2);

    } else {
        WARN ("CredentialsDB load_data FAILED");
    }
    g_hash_table_unref(data);

    if (gsignond_db_credentials_database_remove_data (credentials_db,
            identity_id, "method1")) {
        INFO ("CredentialsDB remove_data");
    } else {
        WARN ("CredentialsDB remove_data");
    }


    if (gsignond_db_credentials_database_insert_reference (credentials_db,
            identity_id, ctx1, "reference1" )) {
        INFO ("CredentialsDB insert_reference");
    } else {
        WARN ("CredentialsDB insert_reference FAILED");
    }

    reflist = gsignond_db_credentials_database_get_references (credentials_db,
                identity_id, ctx1);
    if (reflist) {
        if (g_list_length (reflist) == 1) {
            INFO ("CredentialsDB get_references");
        } else {
            WARN ("CredentialsDB get_references FAILED - count does not match");
        }
        g_list_free_full (reflist, g_free);
    } else {
        WARN ("CredentialsDB get_references FAILED");
    }

    if (gsignond_db_credentials_database_remove_reference (credentials_db,
            identity_id, ctx1, "reference1" )) {
        INFO ("CredentialsDB remove_reference");
    } else {
        WARN ("CredentialsDB remove_reference FAILED");
    }
    gsignond_security_context_free (ctx1);

    acl = gsignond_db_credentials_database_get_accesscontrol_list (
            credentials_db, identity_id);
    if (acl) {
        INFO ("CredentialsDB get_acl");
        gsignond_security_context_list_free (acl);
    } else {
        WARN ("CredentialsDB get_acl FAILED");
    }

    owners = gsignond_db_credentials_database_get_owner_list (credentials_db,
            identity_id);
    if (owners) {
        INFO ("CredentialsDB get_owners");
        gsignond_security_context_list_free (owners);
    } else {
        WARN ("CredentialsDB get_owners FAILED");
    }

    owner = gsignond_db_credentials_database_get_owner (credentials_db,
            identity_id);
    if (owner) {
        INFO ("CredentialsDB get_owner");
        gsignond_security_context_free (owner);
    } else {
        WARN ("CredentialsDB get_owner FAILED");
    }

    identities = gsignond_db_credentials_database_load_identities (
            credentials_db);
    if (identities) {
        if (g_list_length (identities) == 1) {
            INFO ("CredentialsDB load_identities");
        } else {
            WARN ("CredentialsDB load_identities FAILED - not same size");
        }
        gsignond_identity_info_list_free (identities);
    } else {
        WARN ("CredentialsDB get_identities FAILED");
    }

    if (gsignond_db_credentials_database_remove_identity (credentials_db,
            identity_id)) {
        INFO ("CredentialsDB remove_identity");
    } else {
        WARN ("CredentialsDB remove_identity FAILED");
    }
    gsignond_identity_info_free (identity);

    g_object_unref(credentials_db);
}

void
test_secret_cache ()
{
    GSignondConfig *config = NULL;
    GSignondSecretStorage *storage =NULL;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    GSignondDbSecretCache *cache = NULL;
    GSignondCredentials *creds = NULL, *creds2;

    cache = gsignond_db_secret_cache_new();
    if (cache) {
        INFO ("SecretCache created");
    } else {
        WARN ("SecretCache creation FAILED");
    }

    creds = gsignond_credentials_new ();
    gsignond_credentials_set_data (creds, 1, "username2", "password2");
    if (gsignond_db_secret_cache_update_credentials (cache, creds, TRUE)) {
        INFO ("SecretCache update_credentials");
    } else {
        WARN ("SecretCache update_credentials FAILED");
    }

    creds2 = gsignond_db_secret_cache_get_credentials (cache, 1);
    if (creds2) {
        if (gsignond_credentials_equal (creds, creds2)) {
            INFO ("SecretCache get_credentials");
        } else {
            WARN ("SecretCache get_credentials FAILED - mismatch");
        }
    } else {
        WARN ("SecretCache get_credentials FAILED");
    }
    g_object_unref (creds);

    data = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_bytes_unref);
    g_hash_table_insert (data,"key1",g_bytes_new("value1", strlen ("value1")));
    g_hash_table_insert (data,"key2",g_bytes_new("value2", strlen ("value2")));
    g_hash_table_insert (data,"key3",g_bytes_new("value3", strlen ("value3")));
    g_hash_table_insert (data,"key4",g_bytes_new("value4", strlen ("value4")));
    g_hash_table_insert (data,"key5",g_bytes_new("value5", strlen ("value5")));
    if (gsignond_db_secret_cache_update_data (cache, 1, 5, data)) {
        INFO ("SecretCache update_data");
    } else {
        WARN ("SecretCache update_data FAILED");
    }
    g_hash_table_unref (data);

    data2 = gsignond_db_secret_cache_get_data (cache, 1, 5);
    if (data2) {
        INFO ("SecretCache get_data");
    } else {
        WARN ("SecretCache get_data FAILED");
    }

    config = gsignond_config_new ();
    storage = g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
            "config", config, NULL);
    gsignond_secret_storage_open_db (storage);
    if (gsignond_db_secret_cache_write_to_storage (cache, storage)) {
        INFO ("SecretCache write_to_storage");
    } else {
        WARN ("SecretCache write_to_storage FAILED");
    }
    g_object_unref (storage);
    g_object_unref (cache);
}

void
test_metadata_database ()
{
    GSignondConfig *config = NULL;
    guint32 methodid = 0;
    guint32 identity_id = 5;
    const gchar *method1 = "method1";
    GSignondIdentityInfo *identity = NULL, *identity2= NULL;
    GSignondIdentityInfoList *identities = NULL;
    GSignondSecurityContext *ctx1 = NULL;
    GList *methods = NULL, *reflist = NULL;
    GSignondSecurityContextList *acl, *owners;

    config = gsignond_config_new ();
    GSignondDbMetadataDatabase* metadata_db = NULL;
    metadata_db = gsignond_db_metadata_database_new (config);
    if (metadata_db) {
        INFO ("MetadataDB created");
    } else {
        WARN ("MetadataDB creation FAILED");
    }
    if (gsignond_db_metadata_database_open (metadata_db)) {
        INFO ("MetadataDB opened ");
    } else {
        WARN ("MetadataDB cannot be opened");
    }

    if (gsignond_db_metadata_database_open (metadata_db)) {
        INFO ("MetadataDB already open");
    } else {
        WARN ("MetadataDB opened AGAIN");
    }

    if (gsignond_db_sql_database_clear (
            GSIGNOND_DB_SQL_DATABASE (metadata_db))) {
        INFO ("MetadataDB clear");
    } else {
        WARN ("MetadataDB clear FAILED");
    }

    methodid = gsignond_db_metadata_database_get_method_id (metadata_db,
            method1);
    if (methodid > 0 ) {
        INFO ("MetadataDB method already exists");
    } else if (gsignond_db_metadata_database_insert_method (
            metadata_db,
            method1, &methodid)) {
        INFO ("MetadataDB inserted method with id %u", methodid);
    } else {
        WARN ("MetadataDB method CANNOT be inserted");
    }
    if (methodid == gsignond_db_metadata_database_get_method_id (metadata_db,
            method1)) {
        INFO ("MetadataDB get method");
    } else {
        WARN ("MetadataDB method ids NOT same");
    }

    identity = get_filled_identity_info (identity_id);

    identity_id = gsignond_db_metadata_database_update_identity (metadata_db,
                identity);
    if (identity_id != 0) {
        INFO ("MetadataDB update_identity");
    } else {
        WARN ("MetadataDB update_identity FAILED");
    }
    gsignond_identity_info_set_id (identity, identity_id);

    identity2 = gsignond_db_metadata_database_get_identity (metadata_db,
            identity_id);
    if (identity2) {
        INFO ("MetadataDB get identity");
        gsignond_identity_info_free (identity2);
    } else {
        WARN ("MetadataDB get identity FAILED");
    }

    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    methods = gsignond_db_metadata_database_get_methods (metadata_db,
                identity_id, ctx1);
    if (methods) {
        INFO ("MetadataDB get_methods");
        g_list_free_full (methods, g_free);
    } else {
        WARN ("MetadataDB get_methods FAILED");
    }

    if (gsignond_db_metadata_database_insert_reference (metadata_db,
            identity_id, ctx1, "reference1" )) {
        INFO ("MetadataDB insert_reference");
    } else {
        WARN ("MetadataDB insert_reference FAILED");
    }

    reflist = gsignond_db_metadata_database_get_references (metadata_db,
                identity_id, ctx1);
    if (reflist) {
        if (g_list_length (reflist) == 1) {
            INFO ("MetadataDB get_references");
        } else {
            WARN ("MetadataDB get_references FAILED - count does not match");
        }
        g_list_free_full (reflist, g_free);
    } else {
        WARN ("MetadataDB get_references FAILED");
    }

    if (gsignond_db_metadata_database_remove_reference (metadata_db,
            identity_id, ctx1, "reference1" )) {
        INFO ("MetadataDB remove_reference");
    } else {
        WARN ("MetadataDB remove_reference FAILED");
    }
    gsignond_security_context_free (ctx1);

    acl = gsignond_db_metadata_database_get_accesscontrol_list (metadata_db,
            identity_id);
    if (acl) {
        INFO ("MetadataDB get_acl");
        gsignond_security_context_list_free (acl);
    } else {
        WARN ("MetadataDB get_acl FAILED");
    }

    owners = gsignond_db_metadata_database_get_owner_list (metadata_db,
            identity_id);
    if (owners) {
        INFO ("MetadataDB get_owners");
        gsignond_security_context_list_free (owners);
    } else {
        WARN ("MetadataDB get_owners FAILED");
    }

    identities = gsignond_db_metadata_database_get_identities (metadata_db);
    if (identities) {
        if (g_list_length (identities) == 1) {
            INFO ("MetadataDB get_identities");
        } else {
            WARN ("MetadataDB get_identities FAILED - not same size");
        }
        gsignond_identity_info_list_free (identities);
    } else {
        WARN ("MetadataDB get_identities FAILED");
    }
    if (gsignond_db_metadata_database_remove_identity (metadata_db,
            identity_id)) {
        INFO ("MetadataDB remove_identity");
    } else {
        WARN ("MetadataDB remove_identity FAILED");
    }
    gsignond_identity_info_free (identity);

    if (gsignond_db_sql_database_close (
            GSIGNOND_DB_SQL_DATABASE (metadata_db))) {
        INFO ("MetadataDB closed");
    } else {
        WARN ("MetadataDB CANNOT be closed");
    }
    g_object_unref(metadata_db);
}

void
test_secret_storage ()
{
    GSignondSecretStorage *storage = NULL;
    GSignondConfig *config = NULL;
    GString *un = NULL;
    GString *pass = NULL;
    GSignondCredentials *creds = NULL;
    GSignondCredentials *creds1 = NULL;
    guint32 id = 1, method = 2;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    GHashTableIter iter;
    GString *key =NULL;
    GByteArray *value =NULL;
    Data input;

    config = gsignond_config_new ();
    /* Secret Storage */
    storage = g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
            "config", config, NULL);
    if (gsignond_secret_storage_open_db (storage)) {
        INFO ("Database open");
    } else {
        WARN ("Database cannot be opened");
    }

    /* test is_open and close functionality */
    if (gsignond_secret_storage_open_db (storage)) {
        INFO ("Database opened AGAIN");
    } else {
        WARN ("Database cannot be opened AGAIN");
    }

    creds = gsignond_credentials_new ();
    gsignond_credentials_set_data(creds, id, "user 1", "pass 1");
    /* add credentials */
    if (gsignond_secret_storage_update_credentials (storage, creds)) {
        INFO ("Database credentials UPDATED");
    } else {
        WARN ("Database credentials CANNOT be updated");
    }
    g_object_unref (creds); creds = NULL;

    /* read the added credentials */
    creds = gsignond_secret_storage_load_credentials (storage, id);
    if (creds) {
        INFO ("Database credentials LOADED");
    } else {
        WARN ("Database credentials CANNOT be loaded");
    }
    /* check the credentials*/
    if (gsignond_secret_storage_check_credentials (storage, creds)) {
        INFO ("Database credentials CHECKED");
    } else {
        WARN ("Database credentials CANNOT be checked");
    }
    if (creds) {
        g_object_unref (creds);
    }

    /* remove the added credentials */
    if (gsignond_secret_storage_remove_credentials (storage, id)) {
        INFO ("Database credentials REMOVED");
    } else {
        WARN ("Database credentials CANNOT be removed");
    }

    /* add data to store */
    data = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_bytes_unref);
    g_hash_table_insert (data,"key1",g_bytes_new("value1", strlen ("value1")));
    g_hash_table_insert (data,"key2",g_bytes_new("value2", strlen ("value2")));
    g_hash_table_insert (data,"key3",g_bytes_new("value3", strlen ("value3")));
    g_hash_table_insert (data,"key4",g_bytes_new("value4", strlen ("value4")));
    g_hash_table_insert (data,"key5",g_bytes_new("value5", strlen ("value5")));
    if (gsignond_secret_storage_update_data (storage, id, method, data)) {
        INFO ("Database data ADDED");
    } else {
        WARN ("Database data CANNOT be ADDED");
    }

    data2 = gsignond_secret_storage_load_data (storage, id, method);
    if (data2) {
        INFO ("Database data LOADED");
        input.table = data;
        input.status = 1;
        g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
        if (input.status != 1) {
            WARN ("Database data DOES NOT MATCH");
        }
        g_hash_table_unref(data2);

    } else {
        WARN ("Database data CANNOT be LOADED");
    }
    g_hash_table_unref(data);

    if (gsignond_secret_storage_remove_data (storage, id, method)) {
        INFO ("Database data REMOVED");
    } else {
        WARN ("Database data CANNOT be REMOVED");
    }

    if (gsignond_secret_storage_clear_db (storage)) {
        INFO ("Database cleared");
    } else {
        WARN ("Database cannot be cleared");
    }

    if (gsignond_secret_storage_close_db (storage)) {
        INFO ("Database closed");
    } else {
        WARN ("Database cannot be closed");
    }
    g_object_unref(storage);
    //g_object_unref(config);
}

Suite* db_suite (void)
{
    Suite *s = suite_create ("Database");

    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    //tcase_add_test (tc_core, test_secret_storage);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;

    g_type_init();

    Suite *s = db_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

