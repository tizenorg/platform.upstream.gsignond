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

#include "gsignond-secret-storage.h"

#define GSIGNOND_SECRET_STORAGE_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_TYPE_SECRET_STORAGE, \
                                           GSignondSecretStoragePrivate))

struct _GSignondSecretStoragePrivate
{
};

G_DEFINE_TYPE (GSignondSecretStorage, gsignond_secret_storage,
        G_TYPE_OBJECT);

gboolean
gsignond_secret_storage_open_db (
        GSignondSecretStorage *self,
        GSignondConfig* configuration)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->open_db (self,
            configuration);
}

gboolean
gsignond_secret_storage_close_db (
        GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->close_db (self);
}

gboolean
gsignond_secret_storage_clear_db (
        GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->clear_db (self);
}

gboolean
gsignond_secret_storage_is_open_db (
        GSignondSecretStorage *self)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->is_open_db (self);
}

GSignondSecretStorageCredentials*
gsignond_secret_storage_load_credentials (
        GSignondSecretStorage *self,
        const guint32 id)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->load_credentials (self,
            id);
}

gboolean
gsignond_secret_storage_update_credentials (
        GSignondSecretStorage *self,
        const guint32 id,
        const GString *username,
        const GString *password)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->update_credentials (
            self, id, username, password);
}

gboolean
gsignond_secret_storage_remove_credentials (
        GSignondSecretStorage *self,
        const guint32 id)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->remove_credentials (
            self, id);
}

gboolean
gsignond_secret_storage_check_credentials (
        GSignondSecretStorage *self,
        const guint32 id,
        GString *username,
        GString *password)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->check_credentials (self,
            id, username, password);
}

GHashTable*
gsignond_secret_storage_load_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), NULL);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->load_data (self,
            id, method);
}

gboolean
gsignond_secret_storage_update_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method,
        const GHashTable *data)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->update_data (self,
            id, method, data);
}

gboolean
gsignond_secret_storage_remove_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method)
{
    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);

    return GSIGNOND_SECRET_STORAGE_GET_CLASS (self)->remove_data (self,
            id, method);
}

GSignondSecretStorageCredentials*
gsignond_secret_storage_credentials_new(
        void)
{
    GSignondSecretStorageCredentials *creds = NULL;
    creds = (GSignondSecretStorageCredentials *)g_malloc0(
                sizeof(GSignondSecretStorageCredentials));
    return creds;
}

void
gsignond_secret_storage_credentials_free(
        GSignondSecretStorageCredentials *creds)
{
    g_return_if_fail (creds != NULL);

    if (creds->username) {
        g_string_free(creds->username, TRUE);
        creds->username = NULL;
    }
    if (creds->password) {
        g_string_free(creds->password, TRUE);
        creds->password = NULL;
    }
    g_free(creds);
    creds = NULL;
}

static gboolean
_gsignond_secret_storage_check_credentials (
        GSignondSecretStorage *self,
        const guint32 id,
        GString *username,
        GString *password)
{
    gboolean comp_uname = FALSE;
    gboolean comp_pwd = FALSE;
    GSignondSecretStorageCredentials *stored_creds = NULL;

    GSignondSecretStorageClass *klass =
            GSIGNOND_SECRET_STORAGE_GET_CLASS (self);

    g_return_val_if_fail (GSIGNOND_IS_SECRET_STORAGE (self), FALSE);
    stored_creds = klass->load_credentials (self, id);
    if (stored_creds) {
        if (stored_creds->username != NULL) {
            comp_uname = g_string_equal(stored_creds->username, username);
        }
        if (stored_creds->password != NULL) {
            comp_pwd = g_string_equal(stored_creds->password, password);
        }
        gsignond_secret_storage_credentials_free (stored_creds);
    }

    return comp_uname && comp_pwd;
}

static void
gsignond_secret_storage_class_init (
        GSignondSecretStorageClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    /* pure virtual methods */
    klass->open_db = NULL;
    klass->close_db = NULL;
    klass->clear_db = NULL;
    klass->is_open_db = NULL;
    klass->load_credentials = NULL;
    klass->update_credentials = NULL;
    klass->remove_credentials = NULL;
    klass->load_data = NULL;
    klass->update_data = NULL;
    klass->remove_data = NULL;

    /* virtual methods */
    klass->check_credentials = _gsignond_secret_storage_check_credentials;
}

static void
gsignond_secret_storage_init (
        GSignondSecretStorage *self)
{
}


