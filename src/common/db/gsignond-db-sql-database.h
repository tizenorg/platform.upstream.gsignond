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

#ifndef __GSIGNOND_DB_SQL_DATABASE_H__
#define __GSIGNOND_DB_SQL_DATABASE_H__

#include <glib.h>
#include <glib-object.h>
#include <sqlite3.h>

G_BEGIN_DECLS

/*
 * Type macros.
 */
#define GSIGNOND_DB_TYPE_SQL_DATABASE   \
                                       (gsignond_db_sql_database_get_type ())
#define GSIGNOND_DB_SQL_DATABASE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_DB_TYPE_SQL_DATABASE, \
                                           GSignondDbSqlDatabase))
#define GSIGNOND_DB_IS_SQL_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_DB_TYPE_SQL_DATABASE))
#define GSIGNOND_DB_SQL_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                             GSIGNOND_DB_TYPE_SQL_DATABASE, \
                                             GSignondDbSqlDatabaseClass))
#define GSIGNOND_DB_IS_SQL_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                             GSIGNOND_DB_TYPE_SQL_DATABASE))
#define GSIGNOND_DB_SQL_DATABASE_GET_CLASS(obj) \
                                            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                             GSIGNOND_DB_TYPE_SQL_DATABASE, \
                                             GSignondDbSqlDatabaseClass))

typedef struct _GSignondDbSqlDatabasePrivate GSignondDbSqlDatabasePrivate;

typedef gboolean (*GSignondDbSqlDatabaseQueryCallback) (sqlite3_stmt *statement,
                                                        gpointer userdata);

typedef struct
{
    GObject parent_instance;

    /*< private >*/
    GSignondDbSqlDatabasePrivate *priv;
} GSignondDbSqlDatabase;

typedef struct {
    GObjectClass parent_class;

    /**
     * open:
     *
     * See #gsignond_db_sql_database_open.
     */
    gboolean
    (*open) (
            GSignondDbSqlDatabase *self,
            const gchar *filename,
            int flags);

    /**
     * close:
     *
     * See #gsignond_db_sql_database_close.
     */
    gboolean
    (*close) (GSignondDbSqlDatabase *self);

    /**
     * is_open:
     *
     * See #gsignond_db_sql_database_is_open
     */
    gboolean
    (*is_open) (GSignondDbSqlDatabase *self);

    /**
     * create:
     *
     * See #gsignond_db_sql_database_create.
     */
    gboolean
    (*create) (GSignondDbSqlDatabase *self);

    /**
     * clear:
     *
     * See #gsignond_db_sql_database_clear.
     */
    gboolean
    (*clear) (GSignondDbSqlDatabase *self);

} GSignondDbSqlDatabaseClass;

/* used by GSIGNOND_DB_TYPE_SQL_DATABASE */
GType
gsignond_db_sql_database_get_type (void);

gboolean
gsignond_db_sql_database_open (
        GSignondDbSqlDatabase *self,
        const gchar *filename,
        int flags);

gboolean
gsignond_db_sql_database_close (GSignondDbSqlDatabase *self);

gboolean
gsignond_db_sql_database_is_open (
        GSignondDbSqlDatabase *self);

gboolean
gsignond_db_sql_database_create (
        GSignondDbSqlDatabase *self);

gboolean
gsignond_db_sql_database_clear (
        GSignondDbSqlDatabase *self);

sqlite3_stmt *
gsignond_db_sql_database_prepare_statement (
        GSignondDbSqlDatabase *self,
        const gchar *query);

gboolean
gsignond_db_sql_database_exec (
        GSignondDbSqlDatabase *self,
        const gchar *statements);

gint
gsignond_db_sql_database_query_exec (
        GSignondDbSqlDatabase *self,
        const gchar *query,
        GSignondDbSqlDatabaseQueryCallback callback,
        gpointer userdata);

gchar *
gsignond_db_sql_database_query_exec_string (
        GSignondDbSqlDatabase *self,
        const gchar *query);

GList *
gsignond_db_sql_database_query_exec_string_list (
        GSignondDbSqlDatabase *self,
        const gchar *query);

GHashTable *
gsignond_db_sql_database_query_exec_string_tuple (
        GSignondDbSqlDatabase *self,
        const gchar *query);

GHashTable *
gsignond_db_sql_database_query_exec_int_string_tuple (
        GSignondDbSqlDatabase *self,
        const gchar *query);

gboolean
gsignond_db_sql_database_query_exec_int (
        GSignondDbSqlDatabase *self,
        const gchar *query,
        gint *result);

GArray *
gsignond_db_sql_database_query_exec_int_array (
        GSignondDbSqlDatabase *self,
        const gchar *query);

gint
gsignond_db_sql_database_query_exec_stmt (
        GSignondDbSqlDatabase *self,
        sqlite3_stmt *sql_stmt,
        GSignondDbSqlDatabaseQueryCallback callback,
        gpointer userdata);

gboolean
gsignond_db_sql_database_start_transaction (GSignondDbSqlDatabase *self);

gboolean
gsignond_db_sql_database_commit_transaction (GSignondDbSqlDatabase *self);

gboolean
gsignond_db_sql_database_rollback_transaction (GSignondDbSqlDatabase *self);

gboolean
gsignond_db_sql_database_transaction_exec (
        GSignondDbSqlDatabase *self,
        const gchar *stmts);

gint
gsignond_db_sql_database_get_db_version (
        GSignondDbSqlDatabase *self,
        const gchar *query);

void
gsignond_db_sql_database_set_last_error (
        GSignondDbSqlDatabase *self,
        GError *error);

const GError *
gsignond_db_sql_database_get_last_error (GSignondDbSqlDatabase *self);

void
gsignond_db_sql_database_clear_last_error (GSignondDbSqlDatabase *self);

gint64
gsignond_db_sql_database_get_last_insert_rowid (GSignondDbSqlDatabase *self);

G_END_DECLS

#endif /* __GSIGNOND_DB_SQL_DATABASE_H__ */
