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

#include <gsignond/gsignond-config.h>
#include <gsignond/gsignond-credentials.h>

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
    GSignondConfig *config;
    GSignondSecretStoragePrivate *priv;
} GSignondSecretStorage;

typedef struct {
    GObjectClass parent_class;

    /**
     * open_db:
     *
     * See #gsignond_secret_storage_open_db.
     */
    gboolean
    (*open_db) (GSignondSecretStorage *self);

    /**
     * close_db:
     *
     * See #gsignond_secret_storage_close_db.
     */
    gboolean
    (*close_db) (GSignondSecretStorage *self);

    /**
     * clear_db:
     *
     * See #gsignond_secret_storage_clear_db.
     */
    gboolean
    (*clear_db) (GSignondSecretStorage *self);

    /**
     * is_open_db:
     *
     * See #gsignond_secret_storage_is_open_db.
     */
    gboolean
    (*is_open_db) (GSignondSecretStorage *self);

    /**
     * load_credentials:
     *
     * See #gsignond_secret_storage_load_credentials.
     */
    GSignondCredentials*
    (*load_credentials) (
            GSignondSecretStorage *self,
            const guint32 id);

    /**
     * update_credentials:
     *
     * See #gsignond_secret_storage_update_credentials.
     */
    gboolean
    (*update_credentials) (
            GSignondSecretStorage *self,
            GSignondCredentials* creds);

    /**
     * remove_credentials:
     *
     * See #gsignond_secret_storage_remove_credentials.
     */
    gboolean
    (*remove_credentials) (
            GSignondSecretStorage *self,
            const guint32 id);

    /**
     * check_credentials:
     *
     * See #gsignond_secret_storage_check_credentials.
     */
    gboolean
    (*check_credentials) (
            GSignondSecretStorage *self,
            GSignondCredentials* creds);

    /**
     * load_data:
     *
     * See #gsignond_secret_storage_load_data.
     *
     */
    GHashTable*
    (*load_data) (
            GSignondSecretStorage *self,
            const guint32 id,
            const guint32 method);

    /**
     * update_data:
     *
     * See #gsignond_secret_storage_update_data.
     */
    gboolean
    (*update_data) (
            GSignondSecretStorage *self,
            const guint32 id,
            const guint32 method,
            GHashTable *data);

    /**
     * remove_data:
     *
     * See #gsignond_secret_storage_remove_data.
     */
    gboolean
    (*remove_data) (
            GSignondSecretStorage *self,
            const guint32 id,
            const guint32 method);

} GSignondSecretStorageClass;

/* used by GSIGNOND_TYPE_SECRET_STORAGE */
GType
gsignond_secret_storage_get_type (void);

gboolean
gsignond_secret_storage_open_db (GSignondSecretStorage *self);

gboolean
gsignond_secret_storage_close_db (GSignondSecretStorage *self);

gboolean
gsignond_secret_storage_clear_db (GSignondSecretStorage *self);

gboolean
gsignond_secret_storage_is_open_db (GSignondSecretStorage *self);

GSignondCredentials*
gsignond_secret_storage_load_credentials (
        GSignondSecretStorage *self,
        const guint32 id);

gboolean
gsignond_secret_storage_update_credentials (
        GSignondSecretStorage *self,
        GSignondCredentials* creds);

gboolean
gsignond_secret_storage_remove_credentials (
        GSignondSecretStorage *self,
        const guint32 id);

gboolean
gsignond_secret_storage_check_credentials (
        GSignondSecretStorage *self,
        GSignondCredentials* creds);

GHashTable*
gsignond_secret_storage_load_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method);

gboolean
gsignond_secret_storage_update_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method,
        GHashTable *data);

gboolean
gsignond_secret_storage_remove_data (
        GSignondSecretStorage *self,
        const guint32 id,
        const guint32 method);

const GError*
gsignond_secret_storage_get_last_error (GSignondSecretStorage *self);

void
gsignond_secret_storage_clear_last_error (GSignondSecretStorage *self);


G_END_DECLS

#endif /* __GSIGNOND_SECRET_STORAGE_H__ */
