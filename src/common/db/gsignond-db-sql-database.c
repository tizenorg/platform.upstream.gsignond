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

#include <sys/stat.h>

#include <glib/gstdio.h>

#include "gsignond/gsignond-log.h"
#include "gsignond-db-error.h"
#include "gsignond-db-sql-database.h"
#include "gsignond-db-sql-database-private.h"

#define GSIGNOND_DB_SQL_DATABASE_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_DB_TYPE_SQL_DATABASE, \
                                           GSignondDbSqlDatabasePrivate))

G_DEFINE_TYPE (GSignondDbSqlDatabase, gsignond_db_sql_database, G_TYPE_OBJECT);


static void
_gsignond_db_sql_database_finalize_db (GSignondDbSqlDatabase *self)
{
    if (self->priv->begin_statement) {
        sqlite3_finalize (self->priv->begin_statement);
        self->priv->begin_statement = NULL;
    }

    if (self->priv->commit_statement) {
        sqlite3_finalize (self->priv->commit_statement);
        self->priv->commit_statement = NULL;
    }

    if (self->priv->rollback_statement) {
        sqlite3_finalize (self->priv->rollback_statement);
        self->priv->rollback_statement = NULL;
    }
}

gboolean
_gsignond_db_sql_database_is_open (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    return self->priv->db != NULL;
}

#ifdef ENABLE_SQL_LOG
void trace_callback (void *s, const char *stmt)
{
    if (stmt) {
        DBG ("SQLITE TRACE: %s", stmt);
    }
}
#endif

gboolean
_gsignond_db_sql_database_open (
        GSignondDbSqlDatabase *self,
        const gchar *filename,
        int flags)
{
    int ret;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    if (_gsignond_db_sql_database_is_open (self)) {
        return TRUE;
    }

    ret = sqlite3_open_v2 (filename, &self->priv->db, flags, NULL);
    if (ret != SQLITE_OK) {
        if (self->priv->db) {
            DBG ("Cannot open %s DB: %s", filename,
                    sqlite3_errmsg (self->priv->db));
        }
        gsignond_db_sql_database_update_error_from_db(self);
        GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->close (self);
        return FALSE;
    }
    if (flags & SQLITE_OPEN_CREATE) {
        if (g_chmod (filename, S_IRUSR | S_IWUSR))
            WARN ("setting file permissions on %s failed", filename);
    }

#ifdef ENABLE_SQL_LOG
    sqlite3_trace (self->priv->db, trace_callback, NULL);
#endif

    if (!GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->create (self)) {
        GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->close (self);
        return FALSE;
    }
    return TRUE;
}

gboolean
_gsignond_db_sql_database_close (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (self->priv->db != NULL, FALSE);

    _gsignond_db_sql_database_finalize_db (self);

    if (sqlite3_close (self->priv->db) != SQLITE_OK) {
        DBG ("Unable to close db: %s", sqlite3_errmsg (self->priv->db));
        gsignond_db_sql_database_update_error_from_db(self);
    }
    self->priv->db = NULL;
    self->priv->db_version = 0;

    return TRUE;
}

static int
_prepare_transaction_statement (
        GSignondDbSqlDatabase *self,
        sqlite3_stmt **sql_stmt,
        const gchar *statement)
{
    int ret = SQLITE_OK;

    if (G_UNLIKELY (!*sql_stmt)) {
        ret = sqlite3_prepare_v2 (self->priv->db, statement, -1,
                                  sql_stmt, NULL);
    } else {
        sqlite3_reset (*sql_stmt);
    }

    return ret;
}

static void
_gsignond_db_sql_database_finalize (GObject *gobject)
{
    GSignondDbSqlDatabase *self = GSIGNOND_DB_SQL_DATABASE (gobject);

    _gsignond_db_sql_database_finalize_db (self);

    if (self->priv->db) {
        sqlite3_close (self->priv->db);
        self->priv->db = NULL;
    }

    if (self->priv->last_error) {
        g_error_free (self->priv->last_error);
        self->priv->last_error = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_db_sql_database_parent_class)->finalize (gobject);
}

static void
gsignond_db_sql_database_class_init (GSignondDbSqlDatabaseClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = _gsignond_db_sql_database_finalize;

    /* pure virtual methods */
    klass->create = NULL;
    klass->clear = NULL;

    /* virtual methods */
    klass->open = _gsignond_db_sql_database_open;
    klass->close = _gsignond_db_sql_database_close;
    klass->is_open = _gsignond_db_sql_database_is_open;

    g_type_class_add_private (klass, sizeof (GSignondDbSqlDatabasePrivate));
}

static void
gsignond_db_sql_database_init (GSignondDbSqlDatabase *self)
{
    self->priv = GSIGNOND_DB_SQL_DATABASE_GET_PRIVATE (self);
    self->priv->last_error = NULL;
    self->priv->db = NULL;
    self->priv->db_version = 0;
}

void
gsignond_db_sql_database_update_error_from_db (GSignondDbSqlDatabase *self)
{
    GSignondDbError code;
    GError *error;
    int sql_code;

    g_return_if_fail (self->priv != NULL);

    sql_code = sqlite3_errcode (self->priv->db);

    switch (sql_code)
    {
    case SQLITE_OK:
    case SQLITE_DONE:
        gsignond_db_sql_database_set_last_error (self, NULL);
        return;
    case SQLITE_BUSY:
        code = GSIGNOND_DB_ERROR_LOCKED;
        break;
    default:
        code = GSIGNOND_DB_ERROR_UNKNOWN;
        break;
    }

    error = g_error_new (GSIGNOND_DB_ERROR,
                         code,
                         "Database (SQLite) error %d: %s",
                         sqlite3_errcode (self->priv->db),
                         sqlite3_errmsg (self->priv->db));
    gsignond_db_sql_database_set_last_error (self, error);
}

int
gsignond_db_sql_database_prepare_transaction_statements (
        GSignondDbSqlDatabase *self)
{
    int ret;

    g_return_val_if_fail (self->priv != NULL, FALSE);

    ret = _prepare_transaction_statement(self, &(self->priv->begin_statement),
            "BEGIN EXCLUSIVE;");
    if (ret != SQLITE_OK) return ret;

    ret = _prepare_transaction_statement(self, &(self->priv->commit_statement),
            "COMMIT;");
    if (ret != SQLITE_OK) return ret;

    ret = _prepare_transaction_statement(self,&(self->priv->rollback_statement),
            "ROLLBACK;");

    return ret;
}

/**
 * gsignond_db_sql_database_open:
 *
 * @self: instance of #GSignondDbSqlDatabase
 * @filename: db filename
 * @flags: sqlite3_open_v2 flags for opening db
 *
 * Opens a connection to DB.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_open (
        GSignondDbSqlDatabase *self,
        const gchar *filename,
        int flags)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);

    return GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->open (
            self, filename, flags);
}

/**
 * gsignond_db_sql_database_close:
 *
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Closes the connection to DB if it is opened already.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_close (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);

    return GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->close (self);
}

/**
 * gsignond_db_sql_database_is_open:
 *
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Retrieves the connectivity status to database if it is open or not.
 *
 * Returns: TRUE if there exist a valid connection to database,
 * FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_is_open (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);

    return GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->is_open (self);
}

/**
 * gsignond_db_sql_database_create:
 *
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Creates database structure/tables if does not already exist.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_create (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);

    return GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->create (self);
}

/**
 * gsignond_db_sql_database_clear:
 *
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Clear database data as per needed.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_clear (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);

    return GSIGNOND_DB_SQL_DATABASE_GET_CLASS (self)->clear (self);
}

/**
 * gsignond_db_sql_database_prepare_statement:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be prepared
 *
 * Prepares the statement from the query.
 *
 * Returns: (transfer full) NULL if fails, valid sql statement otherwise.
 */
sqlite3_stmt *
gsignond_db_sql_database_prepare_statement (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    int ret;
    sqlite3_stmt *sql_stmt = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    ret = sqlite3_prepare_v2 (self->priv->db, query, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        DBG ("statement preparation failed for \"%s\": %s",
                query, sqlite3_errmsg (self->priv->db));
        return NULL;
    }

    return sql_stmt;
}

/**
 * gsignond_db_sql_database_exec:
 * @self: instance of #GSignondDbSqlDatabase
 * @stmts: sql statements to be executed on the database
 *
 * Executes SQL statements. transaction begin and commit statements should be
 * explicitly called if needed.
 *
 * Returns: TRUE if the sql statements executes successfully,
 * FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_exec (
        GSignondDbSqlDatabase *self,
        const gchar *statements)
{
    int ret;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (self->priv->db != NULL, FALSE);
    g_return_val_if_fail (statements != NULL, FALSE);

    /* exec statements */
    ret = sqlite3_exec (self->priv->db, statements, NULL, NULL, NULL);
    if (G_UNLIKELY (ret != SQLITE_OK)) {
        gsignond_db_sql_database_update_error_from_db (self);
        return FALSE;
    }

    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 * @callback: callback to be invoked if not NULL for the result of each row
 * @userdata: user_data to be relayed back through the callback
 *
 * Executes an SQL statement, and optionally calls
 * the callback for every row of the result.
 * Returns the number of rows fetched.
 *
 * Returns: 0 if no row is fetched, number of rows fetched otherwise.
 */
gint
gsignond_db_sql_database_query_exec (
        GSignondDbSqlDatabase *self,
        const gchar *query,
        GSignondDbSqlDatabaseQueryCallback callback,
        gpointer userdata)
{
    sqlite3_stmt *sql_stmt;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    sql_stmt = gsignond_db_sql_database_prepare_statement(self, query);
    if (sql_stmt) {
        rows = gsignond_db_sql_database_query_exec_stmt(self, sql_stmt,
                callback, userdata);
    }

    return rows;
}

static gboolean
_gsignond_db_read_string (
        sqlite3_stmt *stmt,
        gchar **string)
{
    *string = g_strdup ((const gchar *)sqlite3_column_text (stmt, 0));
    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec_string:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 *
 * Executes an SQL statement, and returns the fetched integer from the result.
 *
 * Returns: (transfer full) string if rows fetched are greater than 0,
 * NULL otherwise.
 */
gchar *
gsignond_db_sql_database_query_exec_string (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    gchar *str = NULL;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_string,
            &str);

    if (G_UNLIKELY (rows <= 0)) {
        g_free (str);
        str = NULL;
    }
    return str;
}

static gboolean
_gsignond_db_read_strings (
        sqlite3_stmt *stmt,
        GList** strings)
{
    *strings = g_list_append (*strings,
            g_strdup ((const gchar *)sqlite3_column_text (stmt, 0)));
    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec_string_list:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 *
 * Executes an SQL statement, and returns the fetched strings from the results
 * in the list.
 *
 * Returns: (transfer full) list if rows fetched are greater than 0,
 * NULL otherwise. When done with list, it must be freed using
 * g_list_free_full (list, g_free)
 */
GList *
gsignond_db_sql_database_query_exec_string_list (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    GList *list = NULL;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_strings,
            &list);

    if (G_UNLIKELY (rows <= 0)) {
        g_list_free_full (list, g_free);
        list = NULL;
    }
    return list;
}

static gboolean
_gsignond_db_read_string_tuple (
        sqlite3_stmt *stmt,
        GHashTable *tuples)
{
    g_hash_table_insert(tuples,
            g_strdup ((const gchar *)sqlite3_column_text (stmt, 0)),
            g_strdup ((const gchar *)sqlite3_column_text (stmt, 1)));
    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec_string_tuple:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 *
 * Executes an SQL statement, and returns the fetched string tuples from
 * the results into the hash table.
 *
 * Returns: (transfer full) string tuples if rows fetched are greater than 0,
 * NULL otherwise. When done with tuples, it must be freed using
 * g_hash_table_unref (tuples)
 */
GHashTable *
gsignond_db_sql_database_query_exec_string_tuple (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    GHashTable *tuples = NULL;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    tuples = g_hash_table_new_full ((GHashFunc)g_str_hash,
                                    (GEqualFunc)g_str_equal,
                                    (GDestroyNotify)g_free,
                                    (GDestroyNotify)g_free);

    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_string_tuple,
            tuples);

    if (G_UNLIKELY (rows <= 0)) {
        g_hash_table_destroy (tuples);
        tuples = NULL;
    }
    return tuples;
}

static gboolean
_gsignond_db_read_int_string_tuple (
        sqlite3_stmt *stmt,
        GHashTable *tuples)
{
    gint id;
    const gchar *method = NULL;

    id = sqlite3_column_int (stmt, 0);
    method = (const gchar *)sqlite3_column_text (stmt, 1);
    g_hash_table_insert(tuples, GINT_TO_POINTER(id), g_strdup (method));
    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec_int_string_tuple:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 *
 * Executes an SQL statement, and returns the fetched int-string tuples from
 * the results into the hash table.
 *
 * Returns: (transfer full) string tuples if rows fetched are greater than 0,
 * NULL otherwise.
 */
GHashTable *
gsignond_db_sql_database_query_exec_int_string_tuple (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    GHashTable *tuples = NULL;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    tuples = g_hash_table_new_full ((GHashFunc)g_direct_hash,
                                    (GEqualFunc)g_direct_equal,
                                    (GDestroyNotify)NULL,
                                    (GDestroyNotify)g_free);

    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_int_string_tuple,
            tuples);

    if (G_UNLIKELY (rows <= 0)) {
        g_hash_table_destroy (tuples);
        tuples = NULL;
    }
    return tuples;
}

static gboolean
_gsignond_db_read_int (
        sqlite3_stmt *stmt,
        gint *data)
{
    *data = sqlite3_column_int (stmt, 0);
    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec_int:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 *
 * Executes an SQL statement, and returns the fetched integer from the result.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_query_exec_int (
        GSignondDbSqlDatabase *self,
        const gchar *query,
        gint *result)
{
    gint data;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_int,
            &data);
    if (G_UNLIKELY (rows <= 0)) {
        return FALSE;
    }
    *result = data;
    return TRUE;
}

static gboolean
_gsignond_db_read_array (
        sqlite3_stmt *stmt,
        GArray *array)
{
    int item = 0;
    item = sqlite3_column_int (stmt, 0);
    g_array_append_val (array, item);
    return TRUE;
}

/**
 * gsignond_db_sql_database_query_exec_int_array:
 * @self: instance of #GSignondDbSqlDatabase
 * @query: query to be executed on the database
 *
 * Executes an SQL statement, and returns the fetched integers from the results
 * in the array.
 *
 * Returns: (transfer full) list if rows fetched are greater than 0, NULL otherwise.
 */
GArray *
gsignond_db_sql_database_query_exec_int_array (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    GArray *array = NULL;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    array = g_array_new (FALSE, FALSE, sizeof(gint));
    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query,
            (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_read_array,
            array);

    if (G_UNLIKELY (rows <= 0)) {
        g_array_free (array, TRUE);
        array = NULL;
    }
    return array;
}

/**
 * gsignond_db_sql_database_query_exec_stmt:
 * @self: instance of #GSignondDbSqlDatabase
 * @sql_stmt: (transfer full) sql statement executed on the database
 * @callback: callback to be invoked if not NULL for the result of each row
 * @userdata: user_data to be relayed back through the callback
 *
 * Executes an SQL statement, and optionally calls
 * the callback for every row of the result.
 * Returns the number of rows fetched.
 *
 * Returns: 0 if no row is fetched, number of rows fetched otherwise.
 */
gint
gsignond_db_sql_database_query_exec_stmt (
        GSignondDbSqlDatabase *self,
        sqlite3_stmt *sql_stmt,
        GSignondDbSqlDatabaseQueryCallback callback,
        gpointer userdata)
{
    int ret;
    gint rows = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    do {
        ret = sqlite3_step (sql_stmt);
        if (ret == SQLITE_ROW) {
            if (callback && !callback (sql_stmt, userdata))  {
                /* stop if callback return FALSE */
                break;
            }
            rows++;
        } else if (ret != SQLITE_DONE) {
            gsignond_db_sql_database_update_error_from_db (self);
            DBG ("error executing query : %s", sqlite3_errmsg (self->priv->db));
            break;
        }

    } while (ret != SQLITE_DONE);

    sqlite3_finalize (sql_stmt);

    return rows;
}

/**
 * gsignond_db_sql_database_start_transaction:
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Starts a transaction.
 *
 * Returns: TRUE if the transaction starts successfully,
 * FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_start_transaction (GSignondDbSqlDatabase *self)
{
    int ret;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (self->priv->db != NULL, FALSE);

    /* prepare transaction begin, commit and rollback statements */
    ret = gsignond_db_sql_database_prepare_transaction_statements (self);
    if (G_UNLIKELY (ret != SQLITE_OK)) {
        DBG ("Prepare statement failed");
        gsignond_db_sql_database_update_error_from_db (self);
        return FALSE;
    }

    /* begin statement */
    ret = sqlite3_step (self->priv->begin_statement);
    if (G_UNLIKELY (ret != SQLITE_DONE)) {
        DBG ("Begin statement failed");
        gsignond_db_sql_database_update_error_from_db (self);
        return FALSE;
    }
    return TRUE;
}

/**
 * gsignond_db_sql_database_commit_transaction:
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Runs commit statement.
 *
 * Returns: TRUE if the transaction is committed successfully,
 * FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_commit_transaction (GSignondDbSqlDatabase *self)
{
    int ret;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (self->priv->db != NULL, FALSE);

    ret = sqlite3_step (self->priv->commit_statement);
    if (G_UNLIKELY (ret != SQLITE_DONE)) {
        DBG ("Commit statement failed");
        gsignond_db_sql_database_update_error_from_db (self);
        sqlite3_reset (self->priv->commit_statement);
        return FALSE;
    }
    sqlite3_reset (self->priv->commit_statement);

    return TRUE;
}

/**
 * gsignond_db_sql_database_rollback_transaction:
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Runs rollback statement.
 *
 * Returns: TRUE if the transaction rolls back successfully,
 * FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_rollback_transaction (GSignondDbSqlDatabase *self)
{
    int ret;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (self->priv->db != NULL, FALSE);

    ret = sqlite3_step (self->priv->rollback_statement);
    if (G_UNLIKELY (ret != SQLITE_DONE)) {
        DBG ("Rollback statement failed");
        gsignond_db_sql_database_update_error_from_db (self);
        sqlite3_reset (self->priv->rollback_statement);
        return FALSE;
    }
    sqlite3_reset (self->priv->rollback_statement);
    return TRUE;
}

/**
 * gsignond_db_sql_database_transaction_exec:
 * @self: instance of #GSignondDbSqlDatabase
 * @stmts: sql statements to be executed on the database
 *
 * Executes SQL statements starting with begin statement, and ending with
 * commit statement. In case of any failures, statements are rolledback.
 *
 * Returns: TRUE if the sql statements executes successfully,
 * FALSE otherwise.
 */
gboolean
gsignond_db_sql_database_transaction_exec (
        GSignondDbSqlDatabase *self,
        const gchar *statements)
{
    int ret;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), FALSE);
    g_return_val_if_fail (self->priv->db != NULL, FALSE);

    if (!gsignond_db_sql_database_start_transaction (self)) {
        return FALSE;
    }

    /* exec statements */
    ret = sqlite3_exec (self->priv->db, statements, NULL, NULL, NULL);
    if (G_UNLIKELY (ret != SQLITE_OK)) {
        gsignond_db_sql_database_update_error_from_db (self);
        gsignond_db_sql_database_rollback_transaction (self);
        return FALSE;
    }

    return gsignond_db_sql_database_commit_transaction (self);
}

/**
 * gsignond_db_sql_database_get_db_version:
 * @self: instance of #GSignondDbDefaultStorage
 * @query: query to be executed on db to get version
 * e.g. PRAGMA db_version;
 *
 * reads the database version from db
 *
 */
gint
gsignond_db_sql_database_get_db_version (
        GSignondDbSqlDatabase *self,
        const gchar *query)
{
    int ret;
    sqlite3_stmt *sql_stmt;
    gint db_version = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), 0);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    if (self->priv->db_version > 0) {
        return self->priv->db_version;
    }

    ret = sqlite3_prepare_v2 (self->priv->db, query, -1, &sql_stmt, NULL);

    if (ret == SQLITE_OK) {
        ret = sqlite3_step(sql_stmt);
        if (ret == SQLITE_ROW || ret == SQLITE_DONE) {
            db_version = sqlite3_column_int(sql_stmt, 0);
            DBG ("database version %d", db_version);
            self->priv->db_version = db_version;
        }
        sqlite3_finalize(sql_stmt);
    }

    return db_version;
}

/**
 * gsignond_db_sql_database_set_last_error:
 * @self: instance of #GSignondDbDefaultStorage
 * @error: (transfer full) last occurred #GError
 *
 * sets the last occurred error
 *
 */
void
gsignond_db_sql_database_set_last_error (
        GSignondDbSqlDatabase *self,
        GError* error)
{
    g_return_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self));
    gsignond_db_sql_database_clear_last_error(self);
    self->priv->last_error = error;
}

/**
 * gsignond_db_sql_database_get_last_error:
 * @self: instance of #GSignondDbDefaultStorage
 *
 * retrieves the last occurred error
 *
 * Returns: last occurred #GError
 *
 */
const GError*
gsignond_db_sql_database_get_last_error (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), NULL);
    return self->priv->last_error;
}

/**
 * gsignond_db_sql_database_clear_last_error:
 * @self: instance of #GSignondDbDefaultStorage
 *
 * clears the last occurred error
 *
 */
void
gsignond_db_sql_database_clear_last_error (GSignondDbSqlDatabase *self)
{
    g_return_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self));
    if (self->priv->last_error != NULL) {
        g_error_free(self->priv->last_error);
        self->priv->last_error = NULL;
    }
}

/**
 * gsignond_db_sql_database_get_last_insert_rowid:
 * @self: instance of #GSignondDbDefaultStorage
 *
 * the last inserted row id
 *
 * Returns: last inserted rowid
 */
gint64
gsignond_db_sql_database_get_last_insert_rowid (GSignondDbSqlDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SQL_DATABASE (self), -1);
    g_return_val_if_fail (self->priv->db != NULL, 0);

    return sqlite3_last_insert_rowid (self->priv->db);
}


