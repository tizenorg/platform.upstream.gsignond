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

#ifndef __GSIGNOND_DB_CREDENTIALS_DATABASE_H__
#define __GSIGNOND_DB_CREDENTIALS_DATABASE_H__

#include <glib.h>
#include <glib-object.h>
#include <gsignond/gsignond-identity-info.h>
#include <gsignond/gsignond-config.h>
#include <gsignond/gsignond-secret-storage.h>

#include "gsignond-db-metadata-database.h"

G_BEGIN_DECLS

/*
 * Type macros.
 */
#define GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE   \
                                       (gsignond_db_credentials_database_get_type ())
#define GSIGNOND_DB_CREDENTIALS_DATABASE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE, \
                                           GSignondDbCredentialsDatabase))
#define GSIGNOND_DB_IS_CREDENTIALS_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE))
#define GSIGNOND_DB_CREDENTIALS_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                             GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE, \
                                             GSignondDbCredentialsDatabaseClass))
#define GSIGNOND_DB_IS_CREDENTIALS_DATABASE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                             GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE))
#define GSIGNOND_DB_CREDENTIALS_DATABASE_GET_CLASS(obj) \
                                            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                             GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE, \
                                             GSignondDbCredentialsDatabaseClass))

typedef struct _GSignondDbCredentialsDatabasePrivate
                    GSignondDbCredentialsDatabasePrivate;

typedef struct
{
    GObject parent_instance;

    /*< private >*/
    GSignondConfig *config;
    GSignondSecretStorage *secret_storage;
    GSignondDbCredentialsDatabasePrivate *priv;
} GSignondDbCredentialsDatabase;

typedef struct {
    GObjectClass parent_class;

} GSignondDbCredentialsDatabaseClass;

/* used by GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE */
GType
gsignond_db_credentials_database_get_type (void);

GSignondDbCredentialsDatabase*
gsignond_db_credentials_database_new (
        GSignondConfig *config,
        GSignondSecretStorage *storage);

gboolean
gsignond_db_credentials_database_open_secret_storage (
        GSignondDbCredentialsDatabase *self);

gboolean
gsignond_db_credentials_database_close_secret_storage (
        GSignondDbCredentialsDatabase *self);

gboolean
gsignond_db_credentials_database_is_open_secret_storage (
        GSignondDbCredentialsDatabase *self);

gboolean
gsignond_db_credentials_database_clear (
        GSignondDbCredentialsDatabase *self);

GSignondIdentityInfo *
gsignond_db_credentials_database_load_identity (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        gboolean query_secret);

GSignondIdentityInfoList *
gsignond_db_credentials_database_load_identities (
        GSignondDbCredentialsDatabase *self);

gboolean
gsignond_db_credentials_database_insert_identity (
        GSignondDbCredentialsDatabase *self,
        GSignondIdentityInfo* identity,
        gboolean store_secret);

gboolean
gsignond_db_credentials_database_update_identity (
        GSignondDbCredentialsDatabase *self,
        GSignondIdentityInfo* identity,
        gboolean store_secret);

gboolean
gsignond_db_credentials_database_remove_identity (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id);

gboolean
gsignond_db_credentials_database_check_secret (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *username,
        const gchar *secret);

GHashTable*
gsignond_db_credentials_database_load_data (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *method);

gboolean
gsignond_db_credentials_database_update_data (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *method,
        GHashTable *data);

gboolean
gsignond_db_credentials_database_remove_data (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *method);

GList *
gsignond_db_credentials_database_get_methods (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        GSignondSecurityContext* sec_ctx);

gboolean
gsignond_db_credentials_database_insert_reference (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference);
gboolean
gsignond_db_credentials_database_remove_reference (
        GSignondDbCredentialsDatabase *self,
        const guint32 id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference);

GList *
gsignond_db_credentials_database_get_references (
        GSignondDbCredentialsDatabase *self,
        const guint32 id,
        const GSignondSecurityContext *ref_owner);

GSignondSecurityContextList *
gsignond_db_credentials_database_get_accesscontrol_list(
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id);

GSignondSecurityContextList *
gsignond_db_credentials_database_get_owner_list(
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id);

GSignondSecurityContext *
gsignond_db_credentials_database_get_identity_owner (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id);

G_END_DECLS

#endif /* __GSIGNOND_DB_CREDENTIALS_DATABASE_H__ */

