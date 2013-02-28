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

#ifndef __GSIGNOND_DB_METADATA_DATABASE_H__
#define __GSIGNOND_DB_METADATA_DATABASE_H__

#include <glib.h>
#include <glib-object.h>
#include <gsignond/gsignond-identity-info.h>
#include <gsignond/gsignond-config.h>
#include <common/db/gsignond-db-sql-database.h>

G_BEGIN_DECLS

/*
 * Type macros.
 */
#define GSIGNOND_DB_TYPE_METADATA_DATABASE   \
                                       (gsignond_db_metadata_database_get_type ())
#define GSIGNOND_DB_METADATA_DATABASE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_DB_TYPE_METADATA_DATABASE, \
                                           GSignondDbMetadataDatabase))
#define GSIGNOND_DB_IS_METADATA_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_DB_TYPE_METADATA_DATABASE))
#define GSIGNOND_DB_METADATA_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                             GSIGNOND_DB_TYPE_METADATA_DATABASE, \
                                             GSignondDbMetadataDatabaseClass))
#define GSIGNOND_DB_IS_METADATA_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                             GSIGNOND_DB_TYPE_METADATA_DATABASE))
#define GSIGNOND_DB_METADATA_DATABASE_GET_CLASS(obj) \
                                            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                             GSIGNOND_DB_TYPE_METADATA_DATABASE, \
                                             GSignondDbMetadataDatabaseClass))

typedef struct _GSignondDbMetadataDatabasePrivate
                    GSignondDbMetadataDatabasePrivate;

typedef struct
{
    GSignondDbSqlDatabase parent_instance;

    /*< private >*/
    GSignondConfig *config;
    GSignondDbMetadataDatabasePrivate *priv;
} GSignondDbMetadataDatabase;

typedef struct {
    GSignondDbSqlDatabaseClass parent_class;

} GSignondDbMetadataDatabaseClass;

/* used by GSIGNOND_DB_TYPE_METADATA_DATABASE */
GType
gsignond_db_metadata_database_get_type (void);

GSignondDbMetadataDatabase*
gsignond_db_metadata_database_new (GSignondConfig *config);

gboolean
gsignond_db_metadata_database_open (GSignondDbMetadataDatabase *self);

gboolean
gsignond_db_metadata_database_insert_method (
        GSignondDbMetadataDatabase *self,
        const gchar *method,
        guint32 *method_id);

guint32
gsignond_db_metadata_database_get_method_id (
        GSignondDbMetadataDatabase *self,
        const gchar *method);

GList *
gsignond_db_metadata_database_get_methods (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        GSignondSecurityContext* sec_ctx);

guint32
gsignond_db_metadata_database_update_identity (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity);

GSignondIdentityInfo *
gsignond_db_metadata_database_get_identity (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id);

GSignondIdentityInfoList *
gsignond_db_metadata_database_get_identities (GSignondDbMetadataDatabase *self);

gboolean
gsignond_db_metadata_database_remove_identity (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id);

gboolean
gsignond_db_metadata_database_insert_reference (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference);

gboolean
gsignond_db_metadata_database_remove_reference (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference);

GList *
gsignond_db_metadata_database_get_references (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext* ref_owner);

GSignondSecurityContextList *
gsignond_db_metadata_database_get_accesscontrol_list(
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id);

GSignondSecurityContext *
gsignond_db_metadata_database_get_owner(
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id);

G_END_DECLS

#endif /* __GSIGNOND_DB_METADATA_DATABASE_H__ */

