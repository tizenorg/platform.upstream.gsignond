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
#include "gsignond/gsignond-log.h"
#include "common/db/gsignond-db-error.h"
#include "gsignond-db-secret-cache.h"

#define GSIGNOND_DB_SECRET_CACHE_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_DB_TYPE_SECRET_CACHE, \
                                           GSignondDbSecretCachePrivate))

G_DEFINE_TYPE (GSignondDbSecretCache, gsignond_db_secret_cache,
        G_TYPE_OBJECT);

typedef struct {
    GSignondCredentials *creds;
    gboolean store_password;
    GHashTable *blob_data;
} AuthCache;

struct _GSignondDbSecretCachePrivate {
    GHashTable *cache;
};

static AuthCache*
_gsignond_db_auth_cache_new (void)
{
    AuthCache *auth_cache = NULL;
    auth_cache = (AuthCache *)g_malloc0 (sizeof (AuthCache));
    auth_cache->blob_data = NULL;
    auth_cache->creds = NULL;
    return auth_cache;
}

static void
_gsignond_db_auth_cache_free (AuthCache *auth_cache)
{
    g_return_if_fail (auth_cache != NULL);

    if (auth_cache->creds) {
        g_object_unref (auth_cache->creds);
        auth_cache->creds = NULL;
    }
    if (auth_cache->blob_data) {
        g_hash_table_unref (auth_cache->blob_data);
        auth_cache->blob_data = NULL;
    }
    g_free (auth_cache);
    auth_cache = NULL;
}


static void
_gsignond_db_secret_cache_dispose (
        GObject *gobject)
{
    g_return_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (gobject));
    GSignondDbSecretCache *self = GSIGNOND_DB_SECRET_CACHE (gobject);

    /* dispose might be called multiple times, so we must guard against
      * calling g_object_unref() on an invalid GObject.
    */
    if (self->priv->cache) {
        g_hash_table_unref (self->priv->cache);
        self->priv->cache = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_db_secret_cache_parent_class)->dispose (
            gobject);
}

static void
_gsignond_db_secret_cache_finalize (GObject *gobject)
{
    GSignondDbSecretCache *self = GSIGNOND_DB_SECRET_CACHE (gobject);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_db_secret_cache_parent_class)->finalize (
            gobject);
}

static void
gsignond_db_secret_cache_class_init (GSignondDbSecretCacheClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = _gsignond_db_secret_cache_dispose;
    gobject_class->finalize = _gsignond_db_secret_cache_finalize;

    g_type_class_add_private (klass, sizeof (GSignondDbSecretCachePrivate));
}

static void
gsignond_db_secret_cache_init (GSignondDbSecretCache *self)
{
    self->priv = GSIGNOND_DB_SECRET_CACHE_GET_PRIVATE (self);
    self->priv->cache =  g_hash_table_new_full ((GHashFunc)g_int_hash,
                            (GEqualFunc)g_int_equal,
                            (GDestroyNotify)g_free,
                            (GDestroyNotify)_gsignond_db_auth_cache_free);
}

/**
 * gsignond_db_secret_cache_new:
 *
 * Creates new #GSignondDbSecretCache object
 *
 * Returns : (transfer full) the #GSignondDbSecretCache object
 */
GSignondDbSecretCache *
gsignond_db_secret_cache_new ()
{
    return GSIGNOND_DB_SECRET_CACHE (
            g_object_new (GSIGNOND_DB_TYPE_SECRET_CACHE,
                         NULL));
}

/**
 * gsignond_db_secret_cache_get_credentials:
 *
 * @self: instance of #GSignondSecretCache
 * @id: the identity whose credentials are being fetched.
 *
 * Gets the credentials from the cache.
 *
 * Returns: (transfer none) #GSignondCredentials if successful,
 * NULL otherwise. When done use g_object_unref (creds) to release the
 * reference.
 */
GSignondCredentials*
gsignond_db_secret_cache_get_credentials (
        GSignondDbSecretCache *self,
        const guint32 id)
{
    AuthCache *value = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (self), NULL);

    value = (AuthCache *) g_hash_table_lookup (self->priv->cache, &id);
    if (value) {
        return g_object_ref (value->creds);
    }
    return NULL;
}

/**
 * gsignond_db_secret_cache_update_credentials:
 * @self: instance of #GSignondSecretCache
 * @creds: (transfer full) the credentials to be updated.
 * @store_password: flag to store the password or not.
 *
 * Updates the credentials for the given identity to the cache.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_secret_cache_update_credentials (
        GSignondDbSecretCache *self,
        GSignondCredentials *creds,
        gboolean store_password)
{
    guint32 id = 0;
    AuthCache *value = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (self), FALSE);
    g_return_val_if_fail (creds != NULL, FALSE);

    id = gsignond_credentials_get_id(creds);
    if (id == 0) {
        INFO ("Not adding credentials to SecretCache as id is 0");
        return TRUE;
    }

    value = (AuthCache *) g_hash_table_lookup (self->priv->cache, &id);
    if (value) {
        DBG ("Removing existing credentials");
        if (value->creds) g_object_unref (value->creds);
        value->creds = g_object_ref (creds);
    } else {
        guint32 *cred_id = NULL;
        value = _gsignond_db_auth_cache_new ();
        value->creds = g_object_ref (creds);
        cred_id = g_malloc (sizeof (guint32));
        *cred_id = id;
        g_hash_table_insert (self->priv->cache, cred_id, value);
    }
    value->store_password = store_password;
    return TRUE;
}

/**
 * gsignond_db_secret_cache_get_data:
 *
 * @self: instance of #GSignondSecretCache
 * @id: the identity whose credentials are being fetched.
 * @method: the authentication method the data is used for.
 *
 * Gets the data from the cache.
 *
 * Returns: (transfer none) #GHashTable  (gchar*, GBytes*) dictionary with the
 * data; returns NULL if fails. When done use g_hash_table_unref (data) to
 * release the reference
 */
GHashTable *
gsignond_db_secret_cache_get_data (
        GSignondDbSecretCache *self,
        const guint32 id,
        const guint32 method)
{
    AuthCache *value = NULL;
    GHashTable *blob = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (self), NULL);

    value = (AuthCache *) g_hash_table_lookup (self->priv->cache, &id);
    if (value && value->blob_data) {
        DBG ("Credentials exist - check method blob");
        blob = (GHashTable *) g_hash_table_lookup (value->blob_data, &method);
        if (blob) {
            return g_hash_table_ref (blob);
        }
    }
    return NULL;
}

/**
 * gsignond_db_secret_cache_update_data:
 * @self: instance of #GSignondSecretCache
 * @id: the identity whose credentials are being fetched.
 * @method: the authentication method the data is used for.
 * @data: (transfer full) #GHashTable (gchar*, GBytes*) dictionary with the data
 *
 * Updates the data to the cache.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_secret_cache_update_data (
        GSignondDbSecretCache *self,
        const guint32 id,
        const guint32 method,
        GHashTable *data)
{
    AuthCache *value = NULL;
    GHashTable *blob = NULL;
    guint32 *methodid = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (self), FALSE);
    g_return_val_if_fail (data != NULL, FALSE);

    if (id == 0) {
        INFO ("Not adding data to SecretCache as id is 0");
        return TRUE;
    }

    value = (AuthCache *) g_hash_table_lookup (self->priv->cache,
            &id);
    methodid = (guint32 *)g_malloc (sizeof (guint32));
    *methodid = method;
    if (value && !value->blob_data) {
        DBG ("Create new as no blob data exists for the identity");
        value->blob_data = g_hash_table_new_full ((GHashFunc)g_int_hash,
                                    (GEqualFunc)g_int_equal,
                                    (GDestroyNotify)g_free,
                                    (GDestroyNotify)g_hash_table_unref);
    }
    if (value == NULL) {
        DBG ("Create new cache entry as it does not exist already");
        guint32 *cacheid = NULL;
        value = _gsignond_db_auth_cache_new ();
        g_hash_table_insert (value->blob_data, methodid,
                g_hash_table_ref (data));
        cacheid = (guint32 *)g_malloc (sizeof (guint32));
        *cacheid = id;
        g_hash_table_insert (self->priv->cache, cacheid, value);
    } else {
        g_hash_table_replace (value->blob_data, methodid,
                g_hash_table_ref (data));
    }
    return TRUE;
}

/**
 * gsignond_db_secret_cache_write_to_storage:
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Writes the cache to secret storage.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_secret_cache_write_to_storage (
        GSignondDbSecretCache *self,
        GSignondSecretStorage *storage)
{
    GHashTableIter iter, iter1;
    guint32 id, method;
    AuthCache *auth_cache = NULL;
    GHashTable *blob = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (self), FALSE);

    g_hash_table_iter_init (&iter, self->priv->cache);
    while (g_hash_table_iter_next (&iter,(gpointer *) &id,
            (gpointer *) &auth_cache)) {

        /* Store the credentials */
        gsignond_secret_storage_update_credentials(storage,
                auth_cache->creds);

        /* Store any binary blobs */
        g_hash_table_iter_init (&iter1, auth_cache->blob_data);
        while (g_hash_table_iter_next (&iter1,(gpointer *) &method,
                (gpointer *) &blob)) {
            gsignond_secret_storage_update_data(storage,
                    id, method, blob);
        }
    }
    return TRUE;
}

/**
 * gsignond_db_secret_cache_clear:
 * @self: instance of #GSignondDbSqlDatabase
 *
 * Clears the cache.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_secret_cache_clear (GSignondDbSecretCache *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_SECRET_CACHE (self), FALSE);
    g_hash_table_remove_all (self->priv->cache);
    return TRUE;
}


