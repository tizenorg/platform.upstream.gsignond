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

GSignondDaemon *_daemon = NULL;

static void _signal_handler (int sig)
{
    (void) sig;
}

static void _install_sighandlers()
{
    struct sigaction act;

    act.sa_handler = _signal_handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_RESTART;

    sigaction (SIGHUP, &act, 0);
    sigaction (SIGTERM, &act, 0);
    sigaction (SIGINT, &act, 0);
}

static void
_key_free (GString *data)
{
    g_string_free (data, TRUE);
}

static void
_value_free (GByteArray *data)
{
    g_byte_array_free (data, TRUE);
}

static GByteArray*
_byte_array_new (const guint8 *data)
{
    GByteArray *value = NULL;
    value = g_byte_array_new ();
    value = g_byte_array_append (value,
                data, strlen((const char*)data));
    return value;
}

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
        GString *key,
        GByteArray *value,
        Data *user_data)
{
    GByteArray *value_found = NULL;
    value_found = (GByteArray *)g_hash_table_lookup (user_data->table, key);
    if (value_found &&
        g_strcmp0(value_found->data, value->data) == 0)
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
test_metadata_database ()
{
    guint32 methodid = 0;
    const gchar *method1 = "method1";
    GSignondIdentityInfo *identity = NULL;

    GSignondDbMetadataDatabase* metadata_db = NULL;
    metadata_db = gsignond_db_metadata_database_new ();
    if (metadata_db) {
        INFO ("MetadataDB created");
    } else {
        WARN ("MetadataDB creation FAILED");
    }
    if (gsignond_db_sql_database_open (
            GSIGNOND_DB_SQL_DATABASE (metadata_db),
            "/home/imran/.cache/gsignond-metadata.db",
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) {
        INFO ("MetadataDB opened ");
    } else {
        WARN ("MetadataDB cannot be opened");
    }

    if (gsignond_db_sql_database_open (
            GSIGNOND_DB_SQL_DATABASE (metadata_db),
            "/home/imran/.cache/gsignond-metadata.db",
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) {
        INFO ("MetadataDB already open");
    } else {
        WARN ("MetadataDB opened AGAIN");
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

    identity = gsignond_identity_info_new ();
    gsignond_identity_info_set_id (identity, 1);

    //gsignond_db_metadata_database_update_identity

    gsignond_identity_info_free (identity);

    if (gsignond_db_sql_database_close (
            GSIGNOND_DB_SQL_DATABASE (metadata_db))) {
        INFO ("MetadataDB closed");
    } else {
        WARN ("MetadataDB CANNOT be closed");
    }
    g_object_unref(metadata_db);
}

int main (int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *opt_context = NULL;
    gint ret = 0;
    guint sigint_id =  0;
    GOptionEntry opt_entries[] = {
        {NULL }
    };
    GMainLoop *loop = 0;
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

    g_type_init ();

    opt_context = g_option_context_new ("SSO daemon");
    g_option_context_add_main_entries (opt_context, opt_entries, NULL);
    if (!g_option_context_parse (opt_context, &argc, &argv, &error)) {
        ERR ("Error parsing options: %s", error->message);
        g_error_free (error);
        return -1;
    }

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
    data = g_hash_table_new_full ((GHashFunc)g_string_hash,
            (GEqualFunc)g_string_equal,
            (GDestroyNotify)_key_free,
            (GDestroyNotify)_value_free);
    g_hash_table_insert (data, g_string_new("key1"), _byte_array_new("value1"));
    g_hash_table_insert (data, g_string_new("key2"), _byte_array_new("value2"));
    g_hash_table_insert (data, g_string_new("key3"), _byte_array_new("value3"));
    g_hash_table_insert (data, g_string_new("key4"), _byte_array_new("value4"));
    g_hash_table_insert (data, g_string_new("key5"), _byte_array_new("value5"));
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
    test_identity_info ();
    test_metadata_database ();

    return 0;
}

