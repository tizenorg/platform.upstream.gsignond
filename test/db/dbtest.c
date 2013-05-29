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
#include <glib/gstdio.h>

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-credentials.h"
#include "gsignond/gsignond-secret-storage.h"
#include "common/db/gsignond-db-error.h"
#include "common/db/gsignond-db-secret-database.h"
#include "common/db/gsignond-db-sql-database.h"
#include "daemon/gsignond-daemon.h"
#include "daemon/db/gsignond-db-metadata-database.h"
#include "daemon/db/gsignond-db-credentials-database.h"

static GSequence*
_sequence_new (gchar *data)
{
    GSequence *value = NULL;
    value = g_sequence_new (NULL);
    g_sequence_append (value, (guint8 *)data);
    return value;
}

typedef struct {
    GHashTable *table;
    int status;
} Data;

static void
_compare_key_value(
        gchar *key,
        GVariant *value,
        Data *user_data)
{
    GVariant *value2 = (GVariant *)g_hash_table_lookup (user_data->table, key);

    if (value2 && g_variant_get_size (value2) == g_variant_get_size (value2)
               && memcmp (g_variant_get_data (value2), 
                          g_variant_get_data(value),
                          g_variant_get_size(value2)) == 0) {
        return;
    }
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
_get_filled_identity_info_2 (
        GSignondIdentityInfo **identity_inp,
        gboolean add_creds,
        gboolean add_methods,
        gboolean add_realms,
        gboolean add_acl,
        gboolean add_owner)
{
    guint32 type = 456;
    const gchar *username = "username1";
    const gchar *secret = "secret1";
    const gchar *caption = "caption1";
    GSignondIdentityInfo *identity = NULL;
    GSignondSecurityContextList *ctx_list = NULL;
    GSignondSecurityContext *ctx1, *ctx2, *ctx3 ;
    GHashTable *methods = NULL;
    GSequence *seq1 = NULL, *seq_realms;
    identity = *identity_inp;

    if (identity == NULL)
        identity = gsignond_identity_info_new ();
    gsignond_identity_info_set_identity_new (identity);
    gsignond_identity_info_set_secret (identity, secret);
    gsignond_identity_info_set_store_secret (identity, TRUE);
    if (add_creds) {
        gsignond_identity_info_set_username (identity, username);
        gsignond_identity_info_set_username_secret (identity, TRUE);
        gsignond_identity_info_set_caption (identity, caption);
    }

    /*realms*/
    if (add_realms) {
        seq_realms = _sequence_new("realms1");
        gsignond_identity_info_set_realms (identity, seq_realms);
        g_sequence_free (seq_realms);
    }

    /*methods*/
    if (add_methods) {
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
    }

    /*acl*/
    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    ctx2 = gsignond_security_context_new_from_values ("sysctx2", "appctx2");
    ctx3 = gsignond_security_context_new_from_values ("sysctx3", "appctx3");
    ctx_list = g_list_append (ctx_list,ctx1);
    ctx_list = g_list_append (ctx_list,ctx2);
    ctx_list = g_list_append (ctx_list,ctx3);
    if (add_acl) {
        gsignond_identity_info_set_access_control_list (identity, ctx_list);
    }

    /*owners*/
    if (add_owner) {
        gsignond_identity_info_set_owner (identity, ctx1);
    }
    gsignond_security_context_list_free (ctx_list);

    gsignond_identity_info_set_validated (identity, FALSE);
    gsignond_identity_info_set_identity_type (identity, type);
    return identity;
}

static GSignondIdentityInfo *
_get_filled_identity_info (void)
{
    GSignondIdentityInfo *identity = NULL;
    return _get_filled_identity_info_2 (&identity,
            TRUE, TRUE, TRUE, TRUE, TRUE);
}

START_TEST (test_identity_info)
{
    guint32 id = 125;
    guint32 type = 456;
    const gchar *username = "username1";
    const gchar *secret = "secret1";
    const gchar *caption = "caption1";
    GSignondIdentityInfo *identity = NULL;
    GSignondIdentityInfo *identity2 = NULL;
    GSignondSecurityContextList *ctx_list = NULL, *list = NULL;
    GSignondSecurityContext *ctx, *ctx1, *ctx2, *ctx3 ;
    GHashTable *methods = NULL, *methods2;
    GSequence *seq1 = NULL, *seq_realms, *seq21, *mechs;
    GList *list2;

    identity = gsignond_identity_info_new ();
    fail_if (identity == NULL);

    fail_unless (gsignond_identity_info_get_id (identity) == 0);
    fail_unless (gsignond_identity_info_get_is_identity_new (identity)== TRUE);
    fail_unless (gsignond_identity_info_get_username (identity) == NULL);
    fail_unless (gsignond_identity_info_get_is_username_secret (
            identity) == FALSE);
    fail_unless (gsignond_identity_info_get_secret (identity) == NULL);
    fail_unless (gsignond_identity_info_get_store_secret (identity) == FALSE);
    fail_unless (gsignond_identity_info_get_caption (identity) == NULL);
    fail_unless (gsignond_identity_info_get_realms (identity) == NULL);
    fail_unless (gsignond_identity_info_get_methods (identity) == NULL);
    fail_unless (gsignond_identity_info_get_mechanisms (
            identity, "testmech") == NULL);
    fail_unless (gsignond_identity_info_get_access_control_list (
            identity) == NULL);
    fail_unless (gsignond_identity_info_get_owner (identity) == NULL);
    fail_unless (gsignond_identity_info_get_validated (identity) == FALSE);
    fail_unless (gsignond_identity_info_get_identity_type (identity) == -1);

    fail_unless (gsignond_identity_info_set_id (identity, id) == TRUE);

    fail_unless (id == gsignond_identity_info_get_id (identity));

    fail_unless (gsignond_identity_info_set_identity_new (identity) == TRUE);

    fail_unless (gsignond_identity_info_get_is_identity_new (
            identity) == TRUE);

    fail_unless (gsignond_identity_info_set_username (
            identity, NULL) == FALSE);

    fail_unless (gsignond_identity_info_get_username (identity) == NULL);

    fail_unless (gsignond_identity_info_set_username (
            identity, username) == TRUE);

    fail_unless (g_strcmp0 (username, gsignond_identity_info_get_username (
            identity)) == 0);

    fail_unless (gsignond_identity_info_set_username_secret (
            identity, TRUE) == TRUE);

    fail_unless (gsignond_identity_info_get_is_username_secret (
            identity) == TRUE);

    fail_unless (gsignond_identity_info_set_secret (identity, NULL) == FALSE);

    fail_unless (gsignond_identity_info_get_secret (identity) == NULL);

    fail_unless (gsignond_identity_info_set_secret (identity, secret) == TRUE);

    fail_unless (g_strcmp0 (secret, gsignond_identity_info_get_secret (
            identity)) == 0);

    fail_unless (gsignond_identity_info_set_store_secret (
            identity, TRUE) == TRUE);

    fail_unless (gsignond_identity_info_get_store_secret (
            identity) == TRUE);

    fail_unless (gsignond_identity_info_set_caption (identity, NULL) == FALSE);

    fail_unless (gsignond_identity_info_get_caption (identity) == NULL);

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
    fail_unless (gsignond_identity_info_set_methods (
            identity, methods) == TRUE);
    g_hash_table_insert (methods, "method1", seq1);
    g_hash_table_insert (methods, "method2", _sequence_new("mech21"));
    g_hash_table_insert (methods, "method3", _sequence_new("mech31"));
    g_hash_table_insert (methods, "method4", _sequence_new("mech41"));
    fail_unless (gsignond_identity_info_set_methods (
            identity, methods) == TRUE);

    methods2 = gsignond_identity_info_get_methods (identity);
    fail_if (methods2 == NULL);
    seq21 = g_hash_table_lookup (methods, "method1");
    fail_if (seq21 == NULL);
    fail_unless (_compare_sequences (seq1, seq21) == TRUE);
    g_hash_table_unref (methods2);
    g_hash_table_unref (methods);

    fail_unless (gsignond_identity_info_get_mechanisms (
            identity, "method20") == NULL);

    mechs = gsignond_identity_info_get_mechanisms (
                identity, "method1");
    fail_if (mechs == NULL);
    g_sequence_free (mechs);

    fail_unless (gsignond_identity_info_remove_method (
            identity, "method20") == FALSE);
    fail_unless (gsignond_identity_info_remove_method (
            identity, "method4") == TRUE);

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
    fail_unless (gsignond_identity_info_set_owner (
            identity, ctx1) == TRUE);
    ctx = gsignond_identity_info_get_owner (identity);
    fail_if (ctx == NULL);
    fail_unless (gsignond_security_context_compare (ctx, ctx1) == 0);
    gsignond_security_context_free (ctx); ctx = NULL;

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
    gsignond_identity_info_unref (identity2);
    fail_unless (gsignond_identity_info_compare (identity, identity) == TRUE);

    gsignond_security_context_list_free (ctx_list); ctx_list = NULL;

    gsignond_identity_info_unref (identity);
}
END_TEST

static gboolean
_gsignond_query_read_int (
        sqlite3_stmt *stmt,
        gint *status)
{
    *status = sqlite3_column_int (stmt, 0);
    return TRUE;
}
static gboolean
_gsignond_query_read_string (
        sqlite3_stmt *stmt,
        gint *status)
{
    const gchar* str = NULL;
    *status = 0;
    str = (const gchar *)sqlite3_column_text (stmt, 0);
    if (str && strlen(str) > 0 &&
        g_strcmp0 (str, "username1") == 0) {
        *status = 1;
    }
    return TRUE;
}

START_TEST (test_sql_database)
{
    GSignondDbSecretDatabase *database = NULL;
    GSignondConfig *config = NULL;
    gchar *filename = NULL;
    const gchar *dir = NULL;
    GSignondCredentials *creds = NULL;
    guint32 id = 1, method = 2;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    Data input;
    sqlite3_stmt *stmt = NULL;
    gint status=0;
    GList *list = NULL;
    GHashTable* hashtable = NULL;
    GArray *array = NULL;
    GSignondDbSqlDatabase *sqldb = NULL;
    GError *error = NULL;

    /* Secret Storage */
    database = gsignond_db_secret_database_new ();
    fail_if (database == NULL);
    sqldb = GSIGNOND_DB_SQL_DATABASE (database);

    fail_unless (gsignond_db_sql_database_clear (sqldb) == FALSE);
    fail_unless (gsignond_db_sql_database_is_open (sqldb) == FALSE);
    fail_unless (gsignond_db_secret_database_load_credentials (
            database, 1) == NULL);
    fail_unless (gsignond_db_secret_database_update_credentials (
            database, NULL) == FALSE);
    fail_unless (gsignond_db_secret_database_remove_credentials (
            database, 1) == FALSE);
    fail_unless (gsignond_db_secret_database_load_data (
            database, 1, 2) == NULL);
    fail_unless (gsignond_db_secret_database_update_data (
            database, 1, 2, NULL) == FALSE);
    fail_unless (gsignond_db_secret_database_remove_data (
            database, 1, 2) == FALSE);

    config = gsignond_config_new ();
    dir = gsignond_config_get_string (config,
            GSIGNOND_CONFIG_GENERAL_STORAGE_PATH);
    if (!dir) {
        dir = g_get_user_data_dir ();
    }
    g_mkdir_with_parents (dir, S_IRWXU);
    filename = g_build_filename (dir, "sql_db_test.db", NULL);
    fail_unless (gsignond_db_sql_database_open (sqldb, filename,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) == TRUE);
    /* don't open the db again if its already open */
    fail_unless (gsignond_db_sql_database_open (sqldb, filename,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) == TRUE);
    g_free (filename);
    g_object_unref(config);

    creds = gsignond_credentials_new ();
    fail_if (creds == NULL);

    fail_unless (gsignond_credentials_set_data (
            creds, id, "user 1", "pass 1") == TRUE);

    fail_unless (gsignond_db_secret_database_update_credentials (
            database, creds) == TRUE);
    g_object_unref (creds); creds = NULL;

    creds = gsignond_db_secret_database_load_credentials (database, id);
    fail_if (creds == NULL);
    g_object_unref (creds);

    /* remove the added credentials */
    fail_unless (gsignond_db_secret_database_remove_credentials (
            database, id) == TRUE);

    /* add data to store */
    data = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_variant_unref);
    fail_if (data == NULL);

    g_hash_table_insert (data,"key1",g_variant_new_string ("string_value"));
    g_hash_table_insert (data,"key2",g_variant_new_double (12223.4223));
    g_hash_table_insert (data,"key3",g_variant_new_uint16(20));
    g_hash_table_insert (data,"key4",g_variant_new("^ay", "byte_value"));
    fail_unless (gsignond_db_secret_database_update_data (
            database, id, method, data) == TRUE);
    data2 = gsignond_db_secret_database_load_data (database, id, method);
    fail_if (data2 == NULL);
    input.table = data;
    input.status = 1;
    g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
    fail_if (input.status != 1);

    gsignond_dictionary_unref (data2);
    g_hash_table_unref(data);


    /*sql database tests*/
    fail_unless (gsignond_db_sql_database_clear (sqldb) == TRUE);
    stmt = gsignond_db_sql_database_prepare_statement (
            sqldb, "INSERT INTO CREDENTIALS "
            "(id, username, password) VALUES (1, \"username1\",\"password\");");
    fail_if (stmt == NULL);
    fail_unless (sqlite3_finalize (stmt) == SQLITE_OK); stmt = NULL;

    fail_unless (gsignond_db_sql_database_exec (
            sqldb, "INSERT INTO CREDENTIALS (id, username, password) "
                    "VALUES (1, \"username1\",\"password\");") == TRUE);

    fail_unless (gsignond_db_sql_database_exec (
            sqldb, "INSERT INTO CREDENTIALS (id, username, password) "
                    "VALUES (2, \"username2\",\"password2\");") == TRUE);

    fail_unless (gsignond_db_sql_database_exec (
            sqldb, "SELECT id from CREDENTIALS limit 1;") == TRUE);

    fail_unless (gsignond_db_sql_database_query_exec (
            sqldb, "SELECT id from CREDENTIALS limit 1;",
            (GSignondDbSqlDatabaseQueryCallback)_gsignond_query_read_int,
            &status) == 1);
    fail_unless (status == 1);

    fail_unless (gsignond_db_sql_database_query_exec (
            sqldb, "SELECT username from CREDENTIALS where id=1;",
            (GSignondDbSqlDatabaseQueryCallback)_gsignond_query_read_string,
            &status) == 1);
    fail_unless (status == 1);

    list = gsignond_db_sql_database_query_exec_string_list (
            sqldb, "SELECT username from CREDENTIALS;");
    fail_if (list == NULL);
    fail_unless (g_list_length (list) == 2);
    g_list_free_full (list, g_free);

    hashtable = gsignond_db_sql_database_query_exec_string_tuple (
            sqldb, "SELECT username, password from CREDENTIALS;");
    fail_if (hashtable == NULL);
    fail_unless (g_hash_table_size (hashtable) == 2);
    g_hash_table_unref (hashtable);

    hashtable = gsignond_db_sql_database_query_exec_int_string_tuple (
            sqldb, "SELECT id, username from CREDENTIALS "
                        "where password=\"password2\";");
    fail_if (hashtable == NULL);
    fail_unless (g_hash_table_size (hashtable) == 1);
    g_hash_table_unref (hashtable);

    fail_unless (gsignond_db_sql_database_query_exec_int (
            sqldb,"SELECT id from CREDENTIALS where username=\"username2\";",
            &status) == TRUE);
    fail_unless (status == 2);

    array = gsignond_db_sql_database_query_exec_int_array (
            sqldb,"SELECT id from CREDENTIALS;");
    fail_if (array == NULL);
    fail_unless (array->len == 2);
    g_array_free (array, TRUE);

    stmt = gsignond_db_sql_database_prepare_statement (
            sqldb, "SELECT id from CREDENTIALS where username=\"username1\";");
    fail_if (stmt == NULL);
    fail_unless (gsignond_db_sql_database_query_exec_stmt (
            sqldb, stmt, NULL, NULL) == 1);
    stmt = NULL;

    fail_unless (gsignond_db_sql_database_start_transaction (sqldb) == TRUE);
    fail_unless (gsignond_db_sql_database_commit_transaction (sqldb) == TRUE);
    fail_unless (gsignond_db_sql_database_start_transaction (sqldb) == TRUE);
    fail_unless (gsignond_db_sql_database_rollback_transaction (sqldb) == TRUE);
    fail_unless (gsignond_db_sql_database_start_transaction (sqldb) == TRUE);
    fail_unless (gsignond_db_sql_database_start_transaction (sqldb) == FALSE);
    fail_unless (gsignond_db_sql_database_rollback_transaction (sqldb) == TRUE);

    fail_unless (gsignond_db_sql_database_transaction_exec (
            sqldb, "SELECT id from CREDENTIALS "
                   "where username=\"username1\";") == TRUE);

    fail_unless (gsignond_db_sql_database_get_db_version (
            sqldb, "PRAGMA user_version;") == 1);

    error = gsignond_db_create_error(GSIGNOND_DB_ERROR_UNKNOWN,"Unknown error");
    gsignond_db_sql_database_clear_last_error (sqldb);
    fail_unless (gsignond_db_sql_database_get_last_error (sqldb) == NULL);
    gsignond_db_sql_database_set_last_error (sqldb, error);
    fail_unless (gsignond_db_sql_database_get_last_error (sqldb) != NULL);
    gsignond_db_sql_database_clear_last_error (sqldb);
    fail_unless (gsignond_db_sql_database_get_last_error (sqldb) == NULL);

    fail_unless (gsignond_db_sql_database_exec (
            sqldb, "INSERT INTO CREDENTIALS (id, username, password) "
                    "VALUES (4, \"username4\",\"password3\");") == TRUE);
    fail_unless (gsignond_db_sql_database_get_last_insert_rowid (
            sqldb) == 4);

    fail_unless (gsignond_db_secret_database_remove_data (
            database, id, method) == TRUE);
    fail_unless (gsignond_db_sql_database_clear (sqldb) == TRUE);
    fail_unless (gsignond_db_sql_database_close (sqldb) == TRUE);
    g_object_unref(database);
}
END_TEST

START_TEST (test_secret_storage)
{
    GSignondSecretStorage *storage = NULL;
    GSignondConfig *config = NULL;
    GSignondCredentials *creds = NULL;
    guint32 id = 1, method = 2;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    Data input;
    const gchar *dir = NULL;

    config = gsignond_config_new ();
    /* Secret Storage */
    storage = g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
            "config", config, NULL);
    g_object_unref(config);
    fail_if (storage == NULL);

    dir = gsignond_config_get_string (config,
            GSIGNOND_CONFIG_GENERAL_SECURE_DIR);
    if (!dir) {
        dir = g_get_user_data_dir ();
    }
    g_mkdir_with_parents (dir, S_IRWXU);

    fail_unless (gsignond_secret_storage_get_last_error (storage) == NULL);
    fail_unless (gsignond_secret_storage_clear_db (storage) == FALSE);
    fail_unless (gsignond_secret_storage_is_open_db (storage) == FALSE);
    fail_unless (gsignond_secret_storage_load_credentials (storage, 1) == NULL);
    fail_unless (gsignond_secret_storage_update_credentials (
            storage, NULL) == FALSE);
    fail_unless (gsignond_secret_storage_remove_credentials (
            storage, 1) == FALSE);
    fail_unless (gsignond_secret_storage_load_data (
            storage, 1, 2) == NULL);
    fail_unless (gsignond_secret_storage_update_data (
            storage, 1, 2, NULL) == FALSE);
    fail_unless (gsignond_secret_storage_remove_data (
            storage, 1, 2) == FALSE);

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

    gsignond_credentials_set_id (creds, 3);
    fail_unless (gsignond_secret_storage_check_credentials (
            storage, creds) == FALSE);
    g_object_unref (creds);

    /* remove the added credentials */
    fail_unless (gsignond_secret_storage_remove_credentials (
            storage, id) == TRUE);

    /* add data to store */
    data = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_variant_unref);
    fail_if (data == NULL);

    g_hash_table_insert (data,"key1",g_variant_new_string ("string_value"));
    g_hash_table_insert (data,"key2",g_variant_new_double (12223.4223));
    g_hash_table_insert (data,"key3",g_variant_new_uint16(20));
    g_hash_table_insert (data,"key4",g_variant_new("^ay", "byte_value"));

    fail_unless (gsignond_secret_storage_update_data (
            storage, id, method, data) == TRUE);
    data2 = gsignond_secret_storage_load_data (storage, id, method);
    fail_if (data2 == NULL);
    input.table = data;
    input.status = 1;
    g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
    fail_if (input.status != 1);

    gsignond_dictionary_unref(data2);
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
    GSignondSecurityContextList *acl;
    GSignondSecurityContext *owner = NULL;

    config = gsignond_config_new ();
    GSignondDbMetadataDatabase* metadata_db = NULL;
    metadata_db = gsignond_db_metadata_database_new (config);
    g_object_unref(config);
    fail_if (metadata_db == NULL);

    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    identity = _get_filled_identity_info_2 (&identity,
            FALSE, FALSE, FALSE, FALSE, FALSE);
    fail_unless (gsignond_db_metadata_database_insert_method (
            metadata_db, method1, &methodid) == FALSE);
    fail_unless (gsignond_db_metadata_database_get_method_id (
            metadata_db, method1) == 0);
    fail_unless (gsignond_db_metadata_database_get_methods (
            metadata_db, identity_id, ctx1) == NULL);
    fail_unless (gsignond_db_metadata_database_get_methods (
            metadata_db, identity_id, ctx1) == NULL);
    fail_unless (gsignond_db_metadata_database_update_identity (
            metadata_db, identity) == FALSE);
    fail_unless (gsignond_db_metadata_database_get_identity (
            metadata_db, identity_id) == NULL);
    fail_unless (gsignond_db_metadata_database_get_identities (
            metadata_db, NULL) == NULL);
    fail_unless (gsignond_db_metadata_database_remove_identity (
            metadata_db, identity_id) == FALSE);
    fail_unless (gsignond_db_metadata_database_remove_reference (
            metadata_db, identity_id, ctx1, "reference1") == FALSE);
    fail_unless (gsignond_db_metadata_database_get_references (
            metadata_db, identity_id, ctx1) == NULL);
    fail_unless (gsignond_db_metadata_database_get_accesscontrol_list (
            metadata_db, identity_id) == NULL);
    fail_unless (gsignond_db_metadata_database_get_owner (
            metadata_db, identity_id) == NULL);

    fail_unless (gsignond_db_metadata_database_open (metadata_db) == TRUE);

    fail_unless (gsignond_db_metadata_database_open (metadata_db) == TRUE);

    fail_unless (gsignond_db_sql_database_clear (
            GSIGNOND_DB_SQL_DATABASE (metadata_db)) == TRUE);

    fail_unless (gsignond_db_metadata_database_get_accesscontrol_list (
            metadata_db, identity_id) == NULL);

    fail_unless (gsignond_db_metadata_database_get_owner (
            metadata_db, identity_id) == NULL);

    fail_unless (gsignond_db_metadata_database_get_method_id (
            metadata_db, method1) == 0);
    fail_unless (gsignond_db_metadata_database_insert_method (
                        metadata_db, method1, &methodid) == TRUE);

    fail_unless (methodid == gsignond_db_metadata_database_get_method_id (
            metadata_db, method1));

    /*update_identity*/
    identity = _get_filled_identity_info_2 (&identity,
            TRUE, FALSE, FALSE, FALSE, FALSE);
    fail_unless (gsignond_db_metadata_database_update_identity (
            metadata_db, identity) == 0);

    identity = _get_filled_identity_info_2 (&identity,
            FALSE, TRUE, FALSE, FALSE, FALSE);
    fail_unless (gsignond_db_metadata_database_update_identity (
            metadata_db, identity) == 0);

    identity = _get_filled_identity_info_2 (&identity,
            FALSE, FALSE, TRUE, FALSE, FALSE);
    fail_unless (gsignond_db_metadata_database_update_identity (
            metadata_db, identity) == 0);

    identity = _get_filled_identity_info_2 (&identity,
            FALSE, FALSE, FALSE, TRUE, FALSE);
    fail_unless (gsignond_db_metadata_database_update_identity (
            metadata_db, identity) == 0);

    identity = _get_filled_identity_info_2 (&identity,
           FALSE, FALSE, FALSE, FALSE, TRUE);
    identity_id = gsignond_db_metadata_database_update_identity (
            metadata_db, identity);
    fail_unless (identity_id != 0);
    gsignond_identity_info_set_id (identity, identity_id);

    identity2 = gsignond_db_metadata_database_get_identity (
            metadata_db, identity_id);
    fail_if (identity2 == NULL);
    gsignond_identity_info_unref (identity2);

    /*get_identity/identities*/
    fail_unless (gsignond_db_metadata_database_get_identity (
            metadata_db, 2222) == NULL);

    identities = gsignond_db_metadata_database_get_identities (metadata_db, NULL);
    fail_unless (identities != NULL);
    fail_unless (g_list_length (identities) == 1);
    gsignond_identity_info_list_free (identities);

    /*methods*/
    methods = gsignond_db_metadata_database_get_methods (metadata_db,
                identity_id, ctx1);
    fail_if (methods == NULL);
    g_list_free_full (methods, g_free);

    /*references*/
    fail_unless (gsignond_db_metadata_database_get_references (
                metadata_db, identity_id, ctx1) == NULL);
    fail_unless (gsignond_db_metadata_database_remove_reference (
            metadata_db, identity_id, ctx1, "reference1" ) == FALSE);

    fail_unless (gsignond_db_metadata_database_insert_reference (
            metadata_db, identity_id, ctx1, "reference1") == TRUE);

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

    /*acl*/
    acl = gsignond_db_metadata_database_get_accesscontrol_list (metadata_db,
            identity_id);
    fail_if (acl == NULL);
    gsignond_security_context_list_free (acl);

    /*owner*/
    owner = gsignond_db_metadata_database_get_owner (metadata_db,
            identity_id);
    fail_if (owner == NULL);

    fail_unless (gsignond_db_metadata_database_remove_identity (
            metadata_db, identity_id) == TRUE);
    fail_unless (gsignond_db_metadata_database_get_identities (
            metadata_db, NULL) == NULL);

    fail_unless (gsignond_db_metadata_database_get_methods (
            metadata_db, identity_id, owner) == NULL);

    gsignond_security_context_free (owner);

    gsignond_identity_info_unref (identity);

    fail_unless (gsignond_db_sql_database_close (
            GSIGNOND_DB_SQL_DATABASE (metadata_db)) == TRUE);
    g_object_unref(metadata_db);
}
END_TEST

START_TEST (test_credentials_database)
{
    GSignondConfig *config = NULL;
    guint32 identity_id = 5;
    GSignondIdentityInfo *identity = NULL, *identity2= NULL;
    GSignondIdentityInfoList *identities = NULL;
    GSignondSecurityContext *ctx1 = NULL;
    GList *methods = NULL, *reflist = NULL;
    GSignondSecurityContextList *acl = NULL ;
    GSignondSecurityContext *owner = NULL;
    GSignondDbCredentialsDatabase *credentials_db = NULL;
    GSignondSecretStorage *storage =NULL;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    Data input;
    GSignondDictionary *cap_filter = NULL;
    GSignondDictionary *type_filter = NULL;
    GSignondDictionary *cap_type_filter = NULL;
    GSignondDictionary *no_cap_filter = NULL;

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

    identity = _get_filled_identity_info ();

    /*identity load/update*/
    identity_id = gsignond_db_credentials_database_update_identity (
            credentials_db, identity);
    fail_unless (identity_id != 0);
    gsignond_identity_info_set_id (identity, identity_id);

    fail_unless (gsignond_db_credentials_database_load_identity (
                credentials_db, 555, FALSE) == NULL);
    identity2 = gsignond_db_credentials_database_load_identity (
            credentials_db, identity_id, FALSE);
    fail_if (identity2 == NULL);
    gsignond_identity_info_unref (identity2);

    identity2 = gsignond_db_credentials_database_load_identity (
            credentials_db, identity_id, TRUE);
    fail_if (identity2 == NULL);

    fail_unless (g_strcmp0 (gsignond_identity_info_get_username (
            identity2), "username1") == 0);
    fail_unless (g_strcmp0 (gsignond_identity_info_get_secret (
            identity2), "secret1") == 0);
    gsignond_identity_info_unref (identity2);

    fail_unless (gsignond_db_credentials_database_check_secret (
            credentials_db, identity_id, "username2", "secret1") == FALSE);

    fail_unless (gsignond_db_credentials_database_check_secret (
            credentials_db, identity_id, "username1", "secret2") == FALSE);

    fail_unless (gsignond_db_credentials_database_check_secret (
            credentials_db, 0, "username1", "secret2") == FALSE);

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
            (GDestroyNotify)g_variant_unref);
    g_hash_table_insert (data,"key1",g_variant_new_string ("string_value"));
    g_hash_table_insert (data,"key2",g_variant_new_double (12223.4223));
    g_hash_table_insert (data,"key3",g_variant_new_uint16(20));
    g_hash_table_insert (data,"key4",g_variant_new("^ay", "byte_value"));

    fail_unless (gsignond_db_credentials_database_update_data (
            credentials_db, 0, "method1", data) == FALSE);

    fail_unless (gsignond_db_credentials_database_update_data (
            credentials_db, identity_id, "method1", data) == TRUE);

    fail_unless (gsignond_db_credentials_database_update_data (
            credentials_db, identity_id, "method1", data) == TRUE);

    fail_unless (gsignond_db_credentials_database_load_data (
            credentials_db, 0, "method1") == NULL);
    fail_unless (gsignond_db_credentials_database_load_data (
            credentials_db, identity_id, "method2") == NULL);

    data2 = gsignond_db_credentials_database_load_data (credentials_db,
            identity_id, "method1");
    fail_if (data2 == NULL);
    input.table = data;
    input.status = 1;
    g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
    fail_if (input.status != 1);
    gsignond_dictionary_unref(data2);
    g_hash_table_unref(data);

    fail_unless (gsignond_db_credentials_database_remove_data (
            credentials_db, 0, "method1") == FALSE);

    fail_unless (gsignond_db_credentials_database_remove_data (
            credentials_db, identity_id, "method1") == TRUE);

    /*references*/
    fail_unless (gsignond_db_credentials_database_insert_reference (
            credentials_db, identity_id, ctx1, "reference1") == TRUE);

    reflist = gsignond_db_credentials_database_get_references (credentials_db,
                identity_id, ctx1);
    fail_if (reflist == NULL);
    fail_unless (g_list_length (reflist) == 1);
    g_list_free_full (reflist, g_free);

    fail_unless (gsignond_db_credentials_database_remove_reference (
            credentials_db, identity_id, ctx1, "reference2") == FALSE);

    fail_unless (gsignond_db_credentials_database_remove_reference (
            credentials_db, identity_id, ctx1, "reference1") == TRUE);
    gsignond_security_context_free (ctx1);

    acl = gsignond_db_credentials_database_get_accesscontrol_list (
            credentials_db, identity_id);
    fail_if (acl == NULL);
    gsignond_security_context_list_free (acl);

    owner = gsignond_db_credentials_database_get_owner (
            credentials_db, identity_id);
    fail_if (owner == NULL);
    gsignond_security_context_free (owner);

    owner = gsignond_db_credentials_database_get_identity_owner (
            credentials_db, identity_id);
    fail_if (owner == NULL);
    gsignond_security_context_free (owner);

    /* load_identities : matched with caption and security context */
    cap_filter = gsignond_dictionary_new ();
    GSignondSecurityContext *ctx =
    		gsignond_security_context_new_from_values("sysctx1", "appctx1");
    gsignond_dictionary_set_string (cap_filter, "Caption", "cap");
    gsignond_dictionary_set(cap_filter, "Owner",
    		 gsignond_security_context_to_variant(ctx));
    gsignond_security_context_free (ctx);
    identities = gsignond_db_credentials_database_load_identities (
            credentials_db, cap_filter);
    gsignond_dictionary_unref (cap_filter);

    fail_if (identities == NULL);
    fail_unless (g_list_length (identities) == 1);
    gsignond_identity_info_list_free (identities);

    /* load_identities: matched with type */
    type_filter = gsignond_dictionary_new();
    gsignond_dictionary_set_int32 (type_filter, "Type", 456);
    identities = gsignond_db_credentials_database_load_identities (
            credentials_db, type_filter);
    gsignond_dictionary_unref (type_filter);

    fail_if (identities == NULL);
    fail_unless (g_list_length (identities) == 1);
    gsignond_identity_info_list_free (identities);

    /* load_identities: matched with type and caption */
    cap_type_filter = gsignond_dictionary_new();
    gsignond_dictionary_set_int32 (cap_type_filter, "Type", 456);
    gsignond_dictionary_set_string (cap_type_filter, "Caption", "CAP");
    identities = gsignond_db_credentials_database_load_identities (
            credentials_db, cap_type_filter);
    gsignond_dictionary_unref (cap_type_filter);

    fail_if (identities == NULL);
    fail_unless (g_list_length (identities) == 1);
    gsignond_identity_info_list_free (identities);

    /* Negative load_identities query */
    no_cap_filter = gsignond_dictionary_new();
    gsignond_dictionary_set_string (no_cap_filter, "Caption", "non_existing");

    identities = gsignond_db_credentials_database_load_identities (
            credentials_db, no_cap_filter);
    gsignond_dictionary_unref (no_cap_filter);
    fail_unless (identities == NULL);

    fail_unless (gsignond_db_credentials_database_remove_identity (
            credentials_db, identity_id) == TRUE);
    gsignond_identity_info_unref (identity);

    g_object_unref(credentials_db);
}
END_TEST

Suite* db_suite (void)
{
    Suite *s = suite_create ("Database");

    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_identity_info);

    tcase_add_test (tc_core, test_sql_database);
    tcase_add_test (tc_core, test_secret_storage);
    tcase_add_test (tc_core, test_metadata_database);
    tcase_add_test (tc_core, test_credentials_database);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;

#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    Suite *s = db_suite ();
    SRunner *sr = srunner_create (s);
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

