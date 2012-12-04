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

#ifndef __GSIGNOND_DB_SQL_DATABASE_PRIVATE_H__
#define __GSIGNOND_DB_SQL_DATABASE_PRIVATE_H__

#include <glib.h>
#include <sqlite3.h>

#include "gsignond-db-sql-database.h"

G_BEGIN_DECLS

struct _GSignondDbSqlDatabasePrivate
{
    sqlite3 *db;
    gint db_version;
    sqlite3_stmt *begin_statement;
    sqlite3_stmt *commit_statement;
    sqlite3_stmt *rollback_statement;
    GError *last_error;
};

void
gsignond_db_sql_database_update_error_from_db (GSignondDbSqlDatabase *self);

int
gsignond_db_sql_database_prepare_transaction_statements (
            GSignondDbSqlDatabase *self);

G_END_DECLS

#endif /* __GSIGNOND_DB_SQL_DATABASE_PRIVATE_H__ */
