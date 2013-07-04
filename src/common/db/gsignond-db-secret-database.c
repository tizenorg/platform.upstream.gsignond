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
#include <sqlite3.h>
#include <string.h>

#include "gsignond/gsignond-log.h"
#include "gsignond-db-error.h"
#include "gsignond-db-defines.h"
#include "gsignond-db-secret-database.h"
#include "gsignond-db-sql-database-private.h"

#define RETURN_IF_NOT_OPEN(obj, retval) \
    if (gsignond_db_sql_database_is_open ( \
            GSIGNOND_DB_SQL_DATABASE (obj)) == FALSE) { \
        GError* last_error = gsignond_db_create_error( \
                            GSIGNOND_DB_ERROR_NOT_OPEN,\
                            "DB Not Open"); \
        DBG("SecretDB is not available"); \
        gsignond_db_sql_database_set_last_error( \
                GSIGNOND_DB_SQL_DATABASE (obj), last_error); \
        return retval; \
    }

#define GSIGNOND_DB_SECRET_DATABASE_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_DB_TYPE_SECRET_DATABASE, \
                                           GSignondDbSecretDatabasePrivate))

G_DEFINE_TYPE (GSignondDbSecretDatabase, gsignond_db_secret_database,
        GSIGNOND_DB_TYPE_SQL_DATABASE);

struct _GSignondDbSecretDatabasePrivate
{
};

static gboolean
gsignond_db_secret_database_create (GSignondDbSqlDatabase *obj);

static gboolean
gsignond_db_secret_database_clear (GSignondDbSqlDatabase *obj);

static gboolean
_gsignond_db_read_username_password (
        sqlite3_stmt *stmt,
        GSignondCredentials *creds)
{
    gsignond_credentials_set_username (creds,
            (const gchar *)sqlite3_column_text (stmt, 0));

    gsignond_credentials_set_password (creds,
            (const gchar *)sqlite3_column_text (stmt, 1));

    return TRUE;
}

static gboolean
_gsignond_db_read_key_value (
        sqlite3_stmt *stmt,
        GSignondDictionary* data)
{
    const gchar *key = NULL;
    gpointer v_data = 0;
    GVariant *value = NULL;
    const GVariantType *type;
    gsize type_len ;
    gsize size = (gsize) sqlite3_column_bytes(stmt, 1);

    type = (const GVariantType *)sqlite3_column_blob(stmt, 1) ;
    type_len = g_variant_type_get_string_length (type) + 1;

    size -= type_len;
    v_data = g_new0(gconstpointer, size);
    mempcpy(v_data, sqlite3_column_blob(stmt, 1) + type_len, size);

    key = (const gchar *)sqlite3_column_text (stmt, 0);
    value = g_variant_new_from_data (type, 
                (gconstpointer)v_data, size,
                 TRUE, (GDestroyNotify)g_free, v_data);

    gsignond_dictionary_set (data, key, value);
    return TRUE;
}


static void
_gsignond_db_secret_database_finalize (GObject *gobject)
{
    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_db_secret_database_parent_class)->finalize (
            gobject);
}

static void
gsignond_db_secret_database_class_init (GSignondDbSecretDatabaseClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = _gsignond_db_secret_database_finalize;

    GSignondDbSqlDatabaseClass *sql_class =
            GSIGNOND_DB_SQL_DATABASE_CLASS (klass);

    sql_class->create = gsignond_db_secret_database_create;
    sql_class->clear = gsignond_db_secret_database_clear;

}

static void
gsignond_db_secret_database_init (GSignondDbSecretDatabase *self)
{
    /*self->priv = GSIGNOND_DB_SECRET_DATABASE_GET_PRIVATE (self);*/
}

/**
 * gsignond_db_secret_database_new:
 *
 * Creates new #GSignondDbSecretDatabase object
 * Returns : (transfer full): the #GSignondDbSecretDatabase object
 *
 */
GSignondDbSecretDatabase *
gsignond_db_secret_database_new ()
{
    return GSIGNOND_DB_SECRET_DATABASE (
            g_object_new (GSIGNOND_DB_TYPE_SECRET_DATABASE,
                         NULL));
}

static gboolean
gsignond_db_secret_database_create (GSignondDbSqlDatabase *obj)
{
    const gchar *queries = NULL;
    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (obj), FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SECRET_DATABASE (obj), FALSE);

    if (gsignond_db_sql_database_get_db_version(obj,
            "PRAGMA user_version;") > 0) {
        DBG ("DB is already created");
        return TRUE;
    }

    queries = ""
            "CREATE TABLE IF NOT EXISTS CREDENTIALS"
            "(id INTEGER NOT NULL UNIQUE,"
            "username TEXT,"
            "password TEXT,"
            "PRIMARY KEY (id));"

            "CREATE TABLE IF NOT EXISTS STORE"
            "(identity_id INTEGER,"
            "method_id INTEGER,"
            "key TEXT,"
            "value BLOB,"
            "PRIMARY KEY (identity_id, method_id, key));"

            "CREATE TRIGGER IF NOT EXISTS tg_delete_credentials "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM STORE WHERE STORE.identity_id = OLD.id; "
            "END; "

            "PRAGMA user_version = 1;";

    return gsignond_db_sql_database_transaction_exec (obj, queries);
}

static gboolean
gsignond_db_secret_database_clear (GSignondDbSqlDatabase *obj)
{
    const gchar *queries = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (obj), FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SECRET_DATABASE (obj), FALSE);

    queries = ""
            "DELETE FROM CREDENTIALS;"
            "DELETE FROM STORE;";

    return gsignond_db_sql_database_transaction_exec (obj, queries);
}

GSignondCredentials*
gsignond_db_secret_database_load_credentials (
        GSignondDbSecretDatabase *self,
        const guint32 id)
{
    gchar *query = NULL;
    gint rows = 0;
    GSignondCredentials *creds = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (self), NULL);
    RETURN_IF_NOT_OPEN (self, NULL);

    creds = gsignond_credentials_new ();
    query = sqlite3_mprintf ("SELECT username, password FROM credentials "
                             "WHERE id = %u limit 1",
                             id);
    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_username_password,
            creds);
    sqlite3_free (query);

    if (G_UNLIKELY (rows <= 0)) {
        DBG ("Load credentials from DB failed");
        g_object_unref (creds);
        creds = NULL;
    } else {
        gsignond_credentials_set_id (creds, id);
    }
    return creds;
}

gboolean
gsignond_db_secret_database_update_credentials (
        GSignondDbSecretDatabase *self,
        GSignondCredentials *creds)
{
    gchar *query = NULL;
    gboolean ret = FALSE;
    guint32 id = 0;
    const gchar *username = NULL;
    const gchar *password = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);

    id = gsignond_credentials_get_id (creds);
    username = gsignond_credentials_get_username (creds);
    password = gsignond_credentials_get_password (creds);
    query = sqlite3_mprintf ("INSERT OR REPLACE INTO CREDENTIALS "
                             "(id, username, password) "
                             "VALUES (%u, %Q, %Q);",
                             id, username ? username : "",
                             password ? password : "");
    ret = gsignond_db_sql_database_transaction_exec (
            GSIGNOND_DB_SQL_DATABASE (self), query);
    sqlite3_free (query);

    return ret;
}

gboolean
gsignond_db_secret_database_remove_credentials (
        GSignondDbSecretDatabase *self,
        const guint32 id)
{
    gchar *query = NULL;
    gboolean ret = FALSE;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);

    query = sqlite3_mprintf ("DELETE FROM CREDENTIALS WHERE id = %u;"
                             "DELETE FROM STORE WHERE identity_id = %u;",
                             id, id);
    ret = gsignond_db_sql_database_transaction_exec (
            GSIGNOND_DB_SQL_DATABASE (self), query);
    sqlite3_free (query);
    return ret;
}

GSignondDictionary *
gsignond_db_secret_database_load_data (
        GSignondDbSecretDatabase *self,
        const guint32 id,
        const guint32 method)
{
    gchar *query = NULL;
    gint rows = 0;
    GSignondDictionary *data = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (self), NULL);
    RETURN_IF_NOT_OPEN (self, NULL);

    data = gsignond_dictionary_new ();

    query = sqlite3_mprintf (
            "SELECT key, value "
            "FROM STORE WHERE identity_id = %u AND method_id = %u",
            id, method);

    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)_gsignond_db_read_key_value,
            data);

    sqlite3_free (query);

    if (G_UNLIKELY (rows <= 0)) {
        DBG ("Load data from DB failed");
        gsignond_dictionary_unref (data);
        data = NULL;
    }

    return data;
}

gboolean
gsignond_db_secret_database_update_data (
        GSignondDbSecretDatabase *self,
        const guint32 id,
        const guint32 method,
        GSignondDictionary *data)
{
    gchar *query = NULL;
    gint ret = 0;
    GHashTableIter iter;
    gchar *key = NULL;
    GVariant *value = NULL;
    guint32 data_counter = 0;
    GSignondDbSqlDatabase *parent = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    g_return_val_if_fail (data != NULL, FALSE);

    parent = GSIGNOND_DB_SQL_DATABASE (self);
    if (!gsignond_db_sql_database_start_transaction (parent)) {
        DBG ("Start DB transaction Failed");
        return FALSE;
    }

    /* First, remove existing data */
    query = sqlite3_mprintf (
                "DELETE FROM STORE WHERE identity_id = %u ",
                "AND method_id = %u;",
                id, method);
    ret = sqlite3_exec (parent->priv->db, query, NULL, NULL, NULL);
    sqlite3_free (query);
    if (G_UNLIKELY (ret != SQLITE_OK)) {
        DBG ("Delete old data from DB Failed");
        gsignond_db_sql_database_update_error_from_db (parent);
        gsignond_db_sql_database_rollback_transaction (parent);
        return FALSE;
    }

    /* Check if the size requirement is met before running any queries */
    g_hash_table_iter_init (&iter, data);
    while (g_hash_table_iter_next (&iter,(gpointer *) &key,
            (gpointer *) &value)) {
        data_counter = data_counter + strlen (key) +
                       g_variant_type_get_string_length (g_variant_get_type (value)) + 1 +
                       g_variant_get_size(value);
        if (data_counter >= GSIGNOND_DB_MAX_DATA_STORAGE) {
            gsignond_db_sql_database_rollback_transaction (parent);
            DBG ("size limit is exceeded");
            return FALSE;
        }
    }

    /* Insert data to db */
    const char* statement = "INSERT OR REPLACE INTO STORE "
                            "(identity_id, method_id, key, value) "
                            "VALUES(?, ?, ?, ?)";
    g_hash_table_iter_init (&iter, data);
    while (g_hash_table_iter_next (&iter, (gpointer *)&key,
            (gpointer *) &value )) {
        gsize val_size;
        const gchar *val_type;
        gsize val_type_length;
        gpointer value_data;
        sqlite3_stmt *sql_stmt;

        ret = sqlite3_prepare_v2 (parent->priv->db, statement, -1,
                &sql_stmt, NULL);
        if (G_UNLIKELY (ret != SQLITE_OK)) {
            DBG ("Data Insertion to DB Failed");
            gsignond_db_sql_database_update_error_from_db (parent);
            gsignond_db_sql_database_rollback_transaction (parent);
            return FALSE;
        }
        val_type = g_variant_get_type_string(value);
        val_type_length = g_variant_type_get_string_length (
                            (const GVariantType  *)val_type) + 1;
        val_size = g_variant_get_size (value);

        value_data = g_new0(gconstpointer, val_size + val_type_length);
        sprintf ((gchar*)value_data, "%s", val_type);
        memcpy(value_data + val_type_length, g_variant_get_data (value), val_size);

        sqlite3_bind_int(sql_stmt, 1, (int)id);
        sqlite3_bind_int(sql_stmt, 2, (int)method);
        sqlite3_bind_text(sql_stmt, 3, key, -1, SQLITE_STATIC);
        sqlite3_bind_blob(sql_stmt, 4, value_data, (int)val_size + val_type_length, g_free);

        ret = sqlite3_step (sql_stmt);
        if (G_UNLIKELY (ret != SQLITE_DONE)) {
            DBG ("Data Insertion to DB Failed");
            gsignond_db_sql_database_update_error_from_db (parent);
            gsignond_db_sql_database_rollback_transaction (parent);
            return FALSE;
        }

        ret = sqlite3_finalize (sql_stmt);
        if (G_UNLIKELY (ret != SQLITE_OK)) {
            DBG ("Data Insertion to DB Failed");
            gsignond_db_sql_database_update_error_from_db (parent);
            gsignond_db_sql_database_rollback_transaction (parent);
            return FALSE;
        }
    }

    return gsignond_db_sql_database_commit_transaction (parent);
}

gboolean
gsignond_db_secret_database_remove_data (
        GSignondDbSecretDatabase *self,
        const guint32 id,
        const guint32 method)
{
    gchar *statement = NULL;
    gboolean ret = FALSE;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);

    if (method == 0) {
        DBG ("Delete data from DB based on identity id only as method id is 0");
        statement = sqlite3_mprintf (
                "DELETE FROM STORE WHERE identity_id = %u;",
                id);
    } else {
        statement = sqlite3_mprintf (
                "DELETE FROM STORE WHERE identity_id = %u ",
                "AND method_id = %u;",
                id, method);
    }
    ret = gsignond_db_sql_database_transaction_exec (
            GSIGNOND_DB_SQL_DATABASE (self), statement);
    sqlite3_free (statement);

    return ret;
}

