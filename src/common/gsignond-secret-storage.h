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

#ifndef __GSIGNOND_SECRET_STORAGE_H__
#define __GSIGNOND_SECRET_STORAGE_H__

#include <glib.h>
#include <glib-object.h>

#include "gsignond-config.h"
#include "gsignond-credentials.h"

G_BEGIN_DECLS

/*
 * Type macros.
 */
#define GSIGNOND_TYPE_SECRET_STORAGE   \
                                       (gsignond_secret_storage_get_type ())
#define GSIGNOND_SECRET_STORAGE(obj)   (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_TYPE_SECRET_STORAGE, \
                                           GSignondSecretStorage))
#define GSIGNOND_IS_SECRET_STORAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           GSIGNOND_TYPE_SECRET_STORAGE))
#define GSIGNOND_SECRET_STORAGE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                             GSIGNOND_TYPE_SECRET_STORAGE, \
                                             GSignondSecretStorageClass))
#define GSIGNOND_IS_SECRET_STORAGE_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                             GSIGNOND_TYPE_SECRET_STORAGE))
#define GSIGNOND_SECRET_STORAGE_GET_CLASS(obj) \
                                            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                             GSIGNOND_TYPE_SECRET_STORAGE, \
                                             GSignondSecretStorageClass))

typedef struct _GSignondSecretStoragePrivate GSignondSecretStoragePrivate;

typedef struct {
    GObject parent_instance;

    /*< private >*/
    GSignondSecretStoragePrivate *priv;
} GSignondSecretStorage;

typedef struct {
    GObjectClass parent_class;

    /**
     * open_db:
     * @self: instance of #GSignondSecretStorage
     *
     * @configuration: A dictionary of configuration options, dependent on
     * the specific storage implementation.
     *
     * Opens (and initializes) DB. The implementation should take
     * care of creating the DB, if it doesn't exist.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*open_db) (GSignondSecretStorage *self,
                         GSignondConfig* configuration);

    /**
     * close_db:
     * @self: instance of #GSignondSecretStorage
     *
     * Closes the secrets DB. To reopen it, call open_db().
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*close_db) (GSignondSecretStorage *self);

    /**
     * clear_db:
     * @self: instance of #GSignondSecretStorage
     *
     * Removes all stored secrets.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*clear_db) (GSignondSecretStorage *self);

    /**
     * is_open_db:
     * @self: instance of #GSignondSecretStorage
     *
     * Checks if the db is open or not.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*is_open_db) (GSignondSecretStorage *self);

    /**
     * load_credentials:
     * @self: instance of #GSignondSecretStorage
     * @id: the identity whose credentials are being loaded.
     *
     * Loads the credentials.
     *
     * Returns: (transfer full) #GSignondCredentials if successful,
     * NULL otherwise.
     */
    GSignondCredentials* (*load_credentials) (GSignondSecretStorage *self,
                                              const guint32 id);

    /**
     * update_credentials:
     * @self: instance of #GSignondSecretStorage
     * @creds: the credentials that are being updated.
     *
     * Stores/updates the credentials for the given identity.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*update_credentials) (GSignondSecretStorage *self,
                                    GSignondCredentials* creds);

    /**
     * remove_credentials:
     * @self: instance of #GSignondSecretStorage
     * @id: the identity whose credentials are being updated.
     *
     * Remove the credentials for the given identity.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*remove_credentials) (GSignondSecretStorage *self,
                                    const guint32 id);

    /**
     * check_credentials:
     * @self: instance of #GSignondSecretStorage
     * @creds: the credentials that are being checked.
     *
     * Checks whether the given credentials are correct for the
     * given identity.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*check_credentials) (GSignondSecretStorage *self,
                                   GSignondCredentials* creds);

    /**
     * load_data:
     * @self: instance of #GSignondSecretStorage
     * @id: the identity whose credentials are being fetched.
     * @method: the authentication method the data is used for.
     *
     * Loads secret data.
     *
     * Returns: (transfer full) #GHashTable dictionary with the data.
     */
    GHashTable* (*load_data) (GSignondSecretStorage *self,
                              const guint32 id,
                              const guint32 method);

    /**
     * update_data:
     * @self: instance of #GSignondSecretStorage
     * @id: the identity whose credentials are being fetched.
     * @method: the authentication method the data is used for.
     * @data: #GHashTable dictionary with the data.
     *
     * Stores/replaces secret data. Calling this method replaces any data
     * which was previously stored for the given id/method.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*update_data) (GSignondSecretStorage *self,
                             const guint32 id,
                             const guint32 method,
                             GHashTable *data);
    /**
     * remove_data:
     * @self: instance of #GSignondSecretStorage
     * @id: the identity whose credentials are being checked.
     * @method: the authentication method the data is used for.
     *
     * Removes secret data.
     *
     * Returns: TRUE if successful, FALSE otherwise.
     */
    gboolean (*remove_data) (GSignondSecretStorage *self,
                             const guint32 id,
                             const guint32 method);

} GSignondSecretStorageClass;

/* used by GSIGNOND_TYPE_SECRET_STORAGE */
GType           gsignond_secret_storage_get_type (
                    void);

gboolean        gsignond_secret_storage_open_db (
                    GSignondSecretStorage *self,
                    GSignondConfig* configuration);

gboolean        gsignond_secret_storage_close_db (
                    GSignondSecretStorage *self);

gboolean        gsignond_secret_storage_clear_db (
                    GSignondSecretStorage *self);

gboolean        gsignond_secret_storage_is_open_db (
                    GSignondSecretStorage *self);

GSignondCredentials*
                gsignond_secret_storage_load_credentials (
                    GSignondSecretStorage *self, const guint32 id);

gboolean        gsignond_secret_storage_update_credentials (
                    GSignondSecretStorage *self, GSignondCredentials* creds);

gboolean        gsignond_secret_storage_remove_credentials (
                    GSignondSecretStorage *self, const guint32 id);

gboolean        gsignond_secret_storage_check_credentials (
                    GSignondSecretStorage *self, GSignondCredentials* creds);

GHashTable*     gsignond_secret_storage_load_data (
                    GSignondSecretStorage *self, const guint32 id,
                    const guint32 method);

gboolean        gsignond_secret_storage_update_data (
                    GSignondSecretStorage *self, const guint32 id,
                    const guint32 method, GHashTable *data);

gboolean        gsignond_secret_storage_remove_data (
                    GSignondSecretStorage *self, const guint32 id,
                    const guint32 method);

G_END_DECLS

#endif /* __GSIGNOND_SECRET_STORAGE_H__ */
