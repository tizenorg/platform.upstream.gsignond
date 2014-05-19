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

#ifndef __GSIGNOND_DB_SECRET_DATABASE_H__
#define __GSIGNOND_DB_SECRET_DATABASE_H__

#include <glib.h>
#include <glib-object.h>
#include <gsignond/gsignond-credentials.h>
#include <gsignond/gsignond-dictionary.h>

#include "gsignond-db-sql-database.h"

G_BEGIN_DECLS

/*
 * Type macros.
 */
#define GSIGNOND_DB_TYPE_SECRET_DATABASE   \
                                       (gsignond_db_secret_database_get_type ())
#define GSIGNOND_DB_SECRET_DATABASE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_DB_TYPE_SECRET_DATABASE, \
                                           GSignondDbSecretDatabase))
#define GSIGNOND_DB_IS_SECRET_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_DB_TYPE_SECRET_DATABASE))
#define GSIGNOND_DB_SECRET_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                             GSIGNOND_DB_TYPE_SECRET_DATABASE, \
                                             GSignondDbSecretDatabaseClass))
#define GSIGNOND_DB_IS_SECRET_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                             GSIGNOND_DB_TYPE_SECRET_DATABASE))
#define GSIGNOND_DB_SECRET_DATABASE_GET_CLASS(obj) \
                                            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                             GSIGNOND_DB_TYPE_SECRET_DATABASE, \
                                             GSignondDbSecretDatabaseClass))

typedef struct _GSignondDbSecretDatabasePrivate GSignondDbSecretDatabasePrivate;

typedef struct
{
    GSignondDbSqlDatabase parent_instance;

    /*< private >*/
    GSignondDbSecretDatabasePrivate *priv;
} GSignondDbSecretDatabase;

typedef struct {
    GSignondDbSqlDatabaseClass parent_class;

} GSignondDbSecretDatabaseClass;

/* used by GSIGNOND_DB_TYPE_SECRET_DATABASE */
GType
gsignond_db_secret_database_get_type (void);

GSignondDbSecretDatabase *
gsignond_db_secret_database_new (void);

GSignondCredentials *
gsignond_db_secret_database_load_credentials (
        GSignondDbSecretDatabase *self,
        const guint32 id);

gboolean
gsignond_db_secret_database_update_credentials (
        GSignondDbSecretDatabase *self,
        GSignondCredentials *creds);

gboolean
gsignond_db_secret_database_remove_credentials (
        GSignondDbSecretDatabase *self,
        const guint32 id);

GSignondDictionary *
gsignond_db_secret_database_load_data (
        GSignondDbSecretDatabase *self,
        const guint32 id,
        const guint32 method);

gboolean
gsignond_db_secret_database_update_data (
        GSignondDbSecretDatabase *self,
        const guint32 id,
        const guint32 method,
        GSignondDictionary *data);

gboolean
gsignond_db_secret_database_remove_data (
        GSignondDbSecretDatabase *self,
        const guint32 id,
        const guint32 method);

G_END_DECLS

#endif /* __GSIGNOND_DB_SECRET_DATABASE_H__ */
