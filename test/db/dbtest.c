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
#include <sqlite3.h>

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-credentials.h"
#include "gsignond/gsignond-secret-storage.h"
#include "daemon/gsignond-daemon.h"

#include "daemon/db/gsignond-db-metadata-database.h"
#include "daemon/db/gsignond-db-credentials-database.h"
#include "daemon/db/gsignond-db-secret-cache.h"

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

static void
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

static GSignondIdentityInfo *
_get_filled_identity_info (guint32 identity_id)
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

START_TEST (test_identity_info)
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
    GSequence *seq1 = NULL, *seq_realms, *seq21;
    gchar *allowmech;
    GList *list2;

    identity = gsignond_identity_info_new ();
    fail_if (identity == NULL);

    fail_unless (gsignond_identity_info_set_id (identity, id) == TRUE);

    fail_unless (id == gsignond_identity_info_get_id (identity));

    fail_unless (gsignond_identity_info_set_identity_new (identity) == TRUE);

    fail_unless (gsignond_identity_info_get_is_identity_new (
            identity) == TRUE);

    fail_unless (gsignond_identity_info_set_username (
            identity, username) == TRUE);

    fail_unless (g_strcmp0 (username, gsignond_identity_info_get_username (
            identity)) == 0);

    fail_unless (gsignond_identity_info_set_username_secret (
            identity, TRUE) == TRUE);

    fail_unless (gsignond_identity_info_get_is_username_secret (
            identity) == TRUE);

    fail_unless (gsignond_identity_info_set_secret (identity, secret) == TRUE);

    fail_unless (g_strcmp0 (secret, gsignond_identity_info_get_secret (
            identity)) == 0);

    fail_unless (gsignond_identity_info_set_store_secret (
            identity, TRUE) == TRUE);

    fail_unless (gsignond_identity_info_get_store_secret (
            identity) == TRUE);

    fail_unless (gsignond_identity_info_set_caption (
            identity, caption) == TRUE);

    fail_unless (g_strcmp0 (caption, gsignond_identity_info_get_caption (
            identity)) == 0);

    /*realms*/
    seq_realms = _sequence_new("realms1");
    fail_unless (gsignond_identity_info_set_realms (
            identity, seq_realms) == TRUE);

    seq1 = gsignond_identity_info_get_realms (identity);
    fail_if (seq1 == NULL);
    fail_unless (_compare_sequences (seq1, seq_realms) == TRUE);
    g_sequence_free (seq1); seq1 = NULL;
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
    fail_unless (gsignond_identity_info_set_methods (
            identity, methods) == TRUE);

    methods2 = gsignond_identity_info_get_methods (identity);
    fail_if (methods2 == NULL);
    seq21 = g_hash_table_lookup (methods, "method1");
    fail_if (seq21 == NULL);
    fail_unless (_compare_sequences (seq1, seq21) == TRUE);
    g_hash_table_unref (methods2);
    g_hash_table_unref (methods);

    /*acl*/
    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    ctx2 = gsignond_security_context_new_from_values ("sysctx2", "appctx2");
    ctx3 = gsignond_security_context_new_from_values ("sysctx3", "appctx3");
    ctx_list = g_list_append (ctx_list,ctx1);
    ctx_list = g_list_append (ctx_list,ctx2);
    ctx_list = g_list_append (ctx_list,ctx3);
    fail_unless (gsignond_identity_info_set_access_control_list (
            identity, ctx_list) == TRUE);
    list = gsignond_identity_info_get_access_control_list (identity);
    fail_if (list == NULL);
    list2 = g_list_nth (list, 0);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx1) == 0);
    list2 = g_list_nth (list, 1);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx2) == 0);
    list2 = g_list_nth (list, 2);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx3) == 0);
    gsignond_security_context_list_free (list); list = NULL;

    /*owners*/
    fail_unless (gsignond_identity_info_set_owner_list (
            identity, ctx_list) == TRUE);
    list = gsignond_identity_info_get_owner_list (identity);
    fail_if (list == NULL);
    list2 = g_list_nth (list, 0);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx1) == 0);
    list2 = g_list_nth (list, 1);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx2) == 0);
    list2 = g_list_nth (list, 2);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx3) == 0);
    gsignond_security_context_list_free (list); list = NULL;
    gsignond_security_context_list_free (ctx_list);

    fail_unless (gsignond_identity_info_set_validated (
            identity, FALSE) == TRUE);

    fail_unless (gsignond_identity_info_get_validated (identity) == FALSE);

    fail_unless (gsignond_identity_info_set_identity_type (
            identity, type) == TRUE);

    fail_unless (type == gsignond_identity_info_get_identity_type (identity));

    /*copy*/
    identity2 = gsignond_dictionary_copy (identity);
    fail_if (identity2 == NULL);
    fail_unless (gsignond_identity_info_compare (identity, identity2) == TRUE);
    gsignond_identity_info_free (identity2);

    fail_unless (gsignond_identity_info_check_method_mechanism (
            identity, "method1", "mech11", &allowmech) == TRUE);
    fail_unless (g_strcmp0 (allowmech, "mech11") == 0);
    g_free (allowmech);
    gsignond_identity_info_free (identity);
}
END_TEST

START_TEST (test_secret_cache)
{
    GSignondConfig *config = NULL;
    GSignondSecretStorage *storage =NULL;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    GSignondDbSecretCache *cache = NULL;
    GSignondCredentials *creds = NULL, *creds2;

    cache = gsignond_db_secret_cache_new();
    fail_if (cache == NULL);

    creds = gsignond_credentials_new ();
    gsignond_credentials_set_data (creds, 1, "username2", "password2");

    fail_unless (gsignond_db_secret_cache_update_credentials (
            cache, creds, TRUE) == TRUE);

    creds2 = gsignond_db_secret_cache_get_credentials (cache, 1);
    fail_if (creds2 == NULL);
    fail_unless (gsignond_credentials_equal (creds, creds2) == TRUE);
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
    fail_unless (gsignond_db_secret_cache_update_data (
            cache, 1, 5, data) == TRUE);
    g_hash_table_unref (data);

    data2 = gsignond_db_secret_cache_get_data (cache, 1, 5);
    fail_if (data2 == NULL);

    config = gsignond_config_new ();
    storage = g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
            "config", config, NULL);
    g_object_unref (config);
    gsignond_secret_storage_open_db (storage);
    fail_unless (gsignond_db_secret_cache_write_to_storage (
            cache, storage) == TRUE);
    g_object_unref (storage);
    g_object_unref (cache);
}
END_TEST

START_TEST (test_secret_storage)
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
    g_object_unref(config);
    fail_if (storage == NULL);

    fail_unless (gsignond_secret_storage_open_db (storage) == TRUE);
    /* don't open the db again if its already open */
    fail_unless (gsignond_secret_storage_open_db (storage) == TRUE);

    creds = gsignond_credentials_new ();
    fail_if (creds == NULL);

    fail_unless (gsignond_credentials_set_data (
            creds, id, "user 1", "pass 1") == TRUE);

    fail_unless (gsignond_secret_storage_update_credentials (
            storage, creds) == TRUE);
    g_object_unref (creds); creds = NULL;

    creds = gsignond_secret_storage_load_credentials (storage, id);
    fail_if (creds == NULL);

    fail_unless (gsignond_secret_storage_check_credentials (
            storage, creds) == TRUE);
    if (creds) {
        g_object_unref (creds);
    }

    /* remove the added credentials */
    fail_unless (gsignond_secret_storage_remove_credentials (
            storage, id) == TRUE);

    /* add data to store */
    data = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_bytes_unref);
    fail_if (data == NULL);

    g_hash_table_insert (data,"key1",g_bytes_new("value1", strlen ("value1")));
    g_hash_table_insert (data,"key2",g_bytes_new("value2", strlen ("value2")));
    g_hash_table_insert (data,"key3",g_bytes_new("value3", strlen ("value3")));
    g_hash_table_insert (data,"key4",g_bytes_new("value4", strlen ("value4")));
    g_hash_table_insert (data,"key5",g_bytes_new("value5", strlen ("value5")));

    fail_unless (gsignond_secret_storage_update_data (
            storage, id, method, data) == TRUE);
    data2 = gsignond_secret_storage_load_data (storage, id, method);
    fail_if (data2 == NULL);
    input.table = data;
    input.status = 1;
    g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
    fail_if (input.status != 1);

    g_hash_table_unref(data2);
    g_hash_table_unref(data);

    fail_unless (gsignond_secret_storage_remove_data (
            storage, id, method) == TRUE);
    fail_unless (gsignond_secret_storage_clear_db (storage) == TRUE);
    fail_unless (gsignond_secret_storage_close_db (storage) == TRUE);
    g_object_unref(storage);
}
END_TEST

START_TEST (test_metadata_database)
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
    g_object_unref(config);
    fail_if (metadata_db == NULL);
    fail_unless (gsignond_db_metadata_database_open (metadata_db) == TRUE);

    fail_unless (gsignond_db_metadata_database_open (metadata_db) == TRUE);

    fail_unless (gsignond_db_sql_database_clear (
            GSIGNOND_DB_SQL_DATABASE (metadata_db)) == TRUE);

    methodid = gsignond_db_metadata_database_get_method_id (
            metadata_db, method1);
    if (methodid <= 0) {
        fail_unless (gsignond_db_metadata_database_insert_method (
                        metadata_db, method1, &methodid) == TRUE);
    }

    fail_unless (methodid == gsignond_db_metadata_database_get_method_id (
            metadata_db, method1) == TRUE);

    identity = _get_filled_identity_info (identity_id);
    identity_id = gsignond_db_metadata_database_update_identity (
            metadata_db, identity);
    fail_if (identity_id == 0);
    gsignond_identity_info_set_id (identity, identity_id);

    identity2 = gsignond_db_metadata_database_get_identity (
            metadata_db, identity_id);
    fail_if (identity2 == NULL);
    gsignond_identity_info_free (identity2);

    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    methods = gsignond_db_metadata_database_get_methods (metadata_db,
                identity_id, ctx1);
    fail_if (methods == NULL);
    g_list_free_full (methods, g_free);

    fail_unless (gsignond_db_metadata_database_insert_reference (
            metadata_db, identity_id, ctx1, "reference1") == TRUE);

    reflist = gsignond_db_metadata_database_get_references (
            metadata_db, identity_id, ctx1);
    fail_if (reflist == NULL);
    fail_unless (g_list_length (reflist) == 1);
    g_list_free_full (reflist, g_free);

    fail_unless (gsignond_db_metadata_database_remove_reference (
            metadata_db, identity_id, ctx1, "reference1" ) == TRUE);
    gsignond_security_context_free (ctx1);

    acl = gsignond_db_metadata_database_get_accesscontrol_list (metadata_db,
            identity_id);
    fail_if (acl == NULL);
    gsignond_security_context_list_free (acl);

    owners = gsignond_db_metadata_database_get_owner_list (metadata_db,
            identity_id);
    fail_if (owners == NULL);
    gsignond_security_context_list_free (owners);

    identities = gsignond_db_metadata_database_get_identities (metadata_db);
    fail_if (identities == NULL);
    fail_unless (g_list_length (identities) == 1);
    gsignond_identity_info_list_free (identities);

    fail_unless (gsignond_db_metadata_database_remove_identity (
            metadata_db, identity_id) == TRUE);
    gsignond_identity_info_free (identity);

    fail_unless (gsignond_db_sql_database_close (
            GSIGNOND_DB_SQL_DATABASE (metadata_db)) == TRUE);
    g_object_unref(metadata_db);
}
END_TEST

START_TEST (test_credentials_database)
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
    g_object_unref(config);
    credentials_db = gsignond_db_credentials_database_new (
            config, storage);
    g_object_unref (storage);

    fail_if (credentials_db == NULL);

    fail_unless (gsignond_db_credentials_database_open_secret_storage (
            credentials_db) == TRUE);

    fail_unless (gsignond_db_credentials_database_clear (
            credentials_db) == TRUE);

    identity = _get_filled_identity_info (identity_id);

    identity_id = gsignond_db_credentials_database_insert_identity (
            credentials_db, identity, TRUE);
    fail_unless (identity_id != 0);
    gsignond_identity_info_set_id (identity, identity_id);
    identity2 = gsignond_db_credentials_database_load_identity (
            credentials_db, identity_id, TRUE);
    fail_if (identity2 == NULL);
    gsignond_identity_info_free (identity2);

    fail_unless (gsignond_db_credentials_database_check_secret (
            credentials_db, identity_id, "username1", "secret1") == TRUE);

    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    methods = gsignond_db_credentials_database_get_methods (credentials_db,
                identity_id, ctx1);
    fail_if (methods == NULL);
    g_list_free_full (methods, g_free);

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
    fail_unless (gsignond_db_credentials_database_update_data (
            credentials_db, identity_id, "method1", data) == TRUE);

    data2 = gsignond_db_credentials_database_load_data (credentials_db,
            identity_id, "method1");
    fail_if (data2 == NULL);
    input.table = data;
    input.status = 1;
    g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
    fail_if (input.status != 1);
    g_hash_table_unref(data2);
    g_hash_table_unref(data);

    fail_unless (gsignond_db_credentials_database_remove_data (
            credentials_db, identity_id, "method1") == TRUE);

    fail_unless (gsignond_db_credentials_database_insert_reference (
            credentials_db, identity_id, ctx1, "reference1") == TRUE);

    reflist = gsignond_db_credentials_database_get_references (credentials_db,
                identity_id, ctx1);
    fail_if (reflist == NULL);
    fail_unless (g_list_length (reflist) == 1);
    g_list_free_full (reflist, g_free);

    fail_unless (gsignond_db_credentials_database_remove_reference (
            credentials_db, identity_id, ctx1, "reference1") == TRUE);
    gsignond_security_context_free (ctx1);

    acl = gsignond_db_credentials_database_get_accesscontrol_list (
            credentials_db, identity_id);
    fail_if (acl == NULL);
    gsignond_security_context_list_free (acl);

    owners = gsignond_db_credentials_database_get_owner_list (
            credentials_db, identity_id);
    fail_if (owners == NULL);
    gsignond_security_context_list_free (owners);

    owner = gsignond_db_credentials_database_get_owner (
            credentials_db, identity_id);
    fail_if (owner == NULL);
    gsignond_security_context_free (owner);

    identities = gsignond_db_credentials_database_load_identities (
            credentials_db);
    fail_if (identities == NULL);
    fail_unless (g_list_length (identities) == 1);
    gsignond_identity_info_list_free (identities);

    fail_unless (gsignond_db_credentials_database_remove_identity (
            credentials_db, identity_id) == TRUE);
    gsignond_identity_info_free (identity);

    g_object_unref(credentials_db);
}
END_TEST

Suite* db_suite (void)
{
    Suite *s = suite_create ("Database");

    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_identity_info);
    tcase_add_test (tc_core, test_secret_cache);
    tcase_add_test (tc_core, test_secret_storage);
    tcase_add_test (tc_core, test_metadata_database);
    tcase_add_test (tc_core, test_credentials_database);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;

    g_type_init();

    Suite *s = db_suite ();
    SRunner *sr = srunner_create (s);
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

