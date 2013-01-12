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

#include "gsignond-db-secret-database.h"
#include "gsignond-db-error.h"

#include <gsignond/gsignond-secret-storage.h>

#define RETURN_IF_NOT_OPEN(obj, retval) \
    if (gsignond_secret_storage_is_open_db (obj) == FALSE) { \
        GError* last_error = gsignond_db_create_error( \
                            GSIGNOND_DB_ERROR_NOT_OPEN,\
                            "DB Not Open"); \
        g_debug("SecretDB is not available"); \
        gsignond_secret_storage_set_last_error( \
                GSIGNOND_SECRET_STORAGE (obj), last_error); \
        return retval; \
    }

#define GSIGNOND_SECRET_STORAGE_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_TYPE_SECRET_STORAGE, \
                                           GSignondSecretStoragePrivate))

struct _GSignondSecretStoragePrivate
{
    GSignondDbSecretDatabase *database;
    GSignondConfig *config;
};

G_DEFINE_TYPE (GSignondSecretStorage, gsignond_secret_storage,
        G_TYPE_OBJECT);

static void
gsignond_secret_storage_set_last_error (
        GSignondSecretStorage *self,
        GError* error);

enum
{
    PROP_0,
    PROP_CONFIG,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void
_set_property (GObject *object, guint prop_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondSecretStorage *self = GSIGNOND_SECRET_STORAGE (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_assert (self->priv->config == NULL);
            self->priv->config = g_value_dup_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GSignondSecretStorage *self = GSIGNOND_SECRET_STORAGE (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_value_set_object (value, self->priv->config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_gsignond_secret_storage_dispose (GObject *gobject)
{
    g_return_if_fail (GSIGNOND_IS_SECRET_STORAGE (gobject));
    GSignondSecretStorage *self = GSIGNOND_SECRET_STORAGE (gobject);

    /* dispose might be called multiple times, so we must guard against
      * calling g_object_unref() on an invalid GObject.
    */
    if (self->priv->database) {
        g_object_unref (self->priv->database);
        self->priv->database = NULL;
    }

    if (self->priv->config) {
        g_object_unref (self->priv->config);
        self->priv->config = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_secret_storage_parent_class)->dispose (
            gobject);
}

static void
gsignond_secret_storage_class_init (GSignondSecretStorageClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = _set_property;
    gobject_class->get_property = _get_property;
    gobject_class->dispose = _gsignond_secret_storage_dispose;

    properties[PROP_CONFIG] = g_param_spec_object ("config",
                                                   "config",
                                                   "Configuration object",
                                                   GSIGNOND_TYPE_CONFIG,
                                                   G_PARAM_CONSTRUCT_ONLY |
                                                   G_PARAM_READWRITE);
    g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);

    /* virtual methods */
    klass->open_db = gsignond_secret_storage_open_db;
    klass->close_db = gsignond_secret_storage_close_db;
    klass->clear_db = gsignond_secret_storage_clear_db;
    klass->is_open_db = gsignond_secret_storage_is_open_db;
    klass->load_credentials =
            gsignond_secret_storage_load_credentials;
    klass->update_credentials =
            gsignond_secret_storage_update_credentials;
    klass->remove_credentials =
            gsignond_secret_storage_remove_credentials;
    klass->load_data = gsignond_secret_storage_load_data;
    klass->update_data = gsignond_secret_storage_update_data;
    klass->remove_data = gsignond_secret_storage_remove_data;

    g_type_class_add_private (klass, sizeof (GSignondSecretStoragePrivate));
}

static void
gsignond_secret_storage_init (GSignondSecretStorage *self)
{
    self->priv = GSIGNOND_SECRET_STORAGE_GET_PRIVATE (self);
    self->priv->database = NULL;
    self->priv->config = NULL;
}

gboolean
gsignond_secret_storage_open_db (GSignondSecretStorage *self)
{
    const gchar *filename;
    const GHashTable *config_table = NULL;

    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    g_return_val_if_fail (self->priv->config != NULL, FALSE);

    config_table = gsignond_config_get_config_table(self->priv->config);
    g_return_val_if_fail (config_table != NULL, FALSE);

    /* TODO: Fix it when config is updated
     * */
    filename = (const gchar *) g_hash_table_lookup (config_table,
            GSIGNOND_CONFIG_DB_SECRET_DB_FILENAME);
    filename = "/home/imran/.cache/gsignond.db";

    if (!filename) {
        WARN("Invalid db filename...");
        return FALSE;
    }

    if (gsignond_secret_storage_is_open_db (self)) {
        g_debug("SecretDB is already open. Closing first to start again...");
        gsignond_secret_storage_close_db (self);
    }

    self->priv->database = gsignond_db_secret_database_new();

    if (!gsignond_db_sql_database_open (
            GSIGNOND_DB_SQL_DATABASE (self->priv->database),
            filename,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) {
        g_object_unref (self->priv->database);
        self->priv->database = NULL;
        return FALSE;
    }

    return TRUE;
}

gboolean
gsignond_secret_storage_close_db (GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    if (self->priv->database != NULL) {
        gsignond_db_sql_database_close (GSIGNOND_DB_SQL_DATABASE (
                self->priv->database));
        g_object_unref (self->priv->database);
        self->priv->database = NULL;
    }

    return TRUE;
}

gboolean
gsignond_secret_storage_clear_db (GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    return gsignond_db_sql_database_clear (GSIGNOND_DB_SQL_DATABASE (
            self->priv->database));
}

gboolean
gsignond_secret_storage_is_open_db (GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    return ((self->priv->database != NULL) &&
            gsignond_db_sql_database_is_open (GSIGNOND_DB_SQL_DATABASE (
                    self->priv->database)));
}

GSignondCredentials*
gsignond_secret_storage_load_credentials (
        GSignondSecretStorage *self,
        const guint32 id)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    return gsignond_db_secret_database_load_credentials (self->priv->database,
            id);
}

gboolean
gsignond_secret_storage_update_credentials (
        GSignondSecretStorage *self,
        GSignondCredentials *creds)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    return gsignond_db_secret_database_update_credentials (self->priv->database,
            creds);
}

gboolean
gsignond_secret_storage_remove_credentials (
        GSignondSecretStorage *self,
        const guint32 id)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    return gsignond_db_secret_database_remove_credentials (self->priv->database,
            id);
}

gboolean
gsignond_secret_storage_check_credentials (
        GSignondSecretStorage *self,
        GSignondCredentials *creds)
{
    gboolean equal = FALSE;
    GSignondCredentials *stored_creds = NULL;

    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    g_return_val_if_fail (creds != NULL, FALSE);

    GSignondSecretStorageClass *klass =
            GSIGNOND_SECRET_STORAGE_GET_CLASS (self);

    stored_creds = klass->load_credentials (self,
            gsignond_credentials_get_id(creds));

    if (stored_creds) {
        equal = gsignond_credentials_equal(creds, stored_creds);
        g_object_unref (stored_creds);
    }

    return equal;
}

GHashTable*
gsignond_secret_storage_load_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), NULL);
    RETURN_IF_NOT_OPEN (self, NULL);
    return gsignond_db_secret_database_load_data (self->priv->database,
            id, method);
}

gboolean
gsignond_secret_storage_update_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method,
        GHashTable *data)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    return gsignond_db_secret_database_update_data (self->priv->database,
            id, method, data);
}

gboolean
gsignond_secret_storage_remove_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    RETURN_IF_NOT_OPEN (self, FALSE);
    return gsignond_db_secret_database_remove_data (self->priv->database,
            id, method);
}

/**
 * gsignond_secret_storage_set_last_error:
 * @self: instance of #GSignondDbDefaultStorage
 * @error : (transfer full) last occurred #GError
 *
 * sets the last occurred error
 *
 */
void
gsignond_secret_storage_set_last_error (
        GSignondSecretStorage *self,
        GError* error)
{
    g_return_if_fail (GSIGNOND_IS_SECRET_STORAGE (self));
    g_return_if_fail (self->priv->database != NULL);
    gsignond_db_sql_database_set_last_error (
            GSIGNOND_DB_SQL_DATABASE (self->priv->database), error);
}

/**
 * gsignond_secret_storage_get_last_error:
 *
 * @self: instance of #GSignondSecretStorage
 *
 * retrieves the last occurred error
 *
 * Returns: (transfer none) last occurred #GError
 */
const GError *
gsignond_secret_storage_get_last_error (GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), NULL);
    g_return_val_if_fail (self->priv->database != NULL, NULL);
    return gsignond_db_sql_database_get_last_error (
            GSIGNOND_DB_SQL_DATABASE (self->priv->database));
}

/**
 * gsignond_secret_storage_clear_last_error:
 *
 * @self: instance of #GSignondSecretStorage
 *
 * clears the last occurred error
 */
void
gsignond_secret_storage_clear_last_error (GSignondSecretStorage *self)
{
    g_return_if_fail (GSIGNOND_IS_SECRET_STORAGE (self));
    g_return_if_fail (self->priv->database != NULL);
    gsignond_db_sql_database_clear_last_error (
            GSIGNOND_DB_SQL_DATABASE (self->priv->database));
}




