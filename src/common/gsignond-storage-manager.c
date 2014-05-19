/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <unistd.h>
#include <sys/stat.h>

#include <glib/gstdio.h>

#include "config.h"

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-storage-manager.h"
#include "gsignond/gsignond-utils.h"

/**
 * SECTION:gsignond-storage-manager
 * @short_description: manages encrypted disk storage for storing the secret database
 * @include: gsignond/gsignond-plugin-interface.h
 *
 * #GSignondStorageManager manages encrypted disk storage for storing the
 * databases. The default implementation maintains a simple per-user
 * directory accessible only to root and gsignond group, but gSSO can be
 * configured to use a custom extension that provides a subclassed
 * implementation of #GSignondStorageManager
 * (see #GSignondExtension for instructions and pointers to examples).
 */
/**
 * GSignondStorageManager:
 *
 * Opaque #GSignondStorageManager data structure.
 */
 

#define GSIGNOND_STORAGE_MANAGER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  GSIGNOND_TYPE_STORAGE_MANAGER, \
                                  GSignondStorageManagerPrivate))

struct _GSignondStorageManagerPrivate
{
};

enum
{
    PROP_0,
    PROP_CONFIG,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (GSignondStorageManager, gsignond_storage_manager, G_TYPE_OBJECT);

static void
_set_config (GSignondStorageManager *self, GSignondConfig *config)
{
    g_assert (self->config == NULL);
    self->config = config;

    gchar *user_dir = g_strdup_printf ("gsignond.%s", g_get_user_name ());
    const gchar *storage_path = gsignond_config_get_string (
                                       self->config,
                                       GSIGNOND_CONFIG_GENERAL_STORAGE_PATH);
    if (!storage_path) {
        storage_path = BASE_STORAGE_DIR;
        DBG ("storage path not configured, using default location");
    }
#   ifdef ENABLE_DEBUG
    const gchar *env_val = g_getenv("SSO_STORAGE_PATH");
    if (env_val)
        storage_path = env_val;
#   endif
    self->location = g_build_filename (storage_path, user_dir, NULL);
    g_free (user_dir);
    DBG ("secure dir %s", self->location);
}

static void
_set_property (GObject *object, guint prop_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondStorageManager *self =
        GSIGNOND_STORAGE_MANAGER (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_assert (self->config == NULL);
            _set_config (self, GSIGNOND_CONFIG (g_value_dup_object (value)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GSignondStorageManager *self =
        GSIGNOND_STORAGE_MANAGER (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_value_set_object (value, self->config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    GSignondStorageManager *self =
        GSIGNOND_STORAGE_MANAGER (object);

    /* unmount mounted filesystem */
    if (gsignond_storage_manager_filesystem_is_mounted (self)) {
        gsignond_storage_manager_unmount_filesystem (self);
    }

    if (self->config) {
        g_object_unref (self->config);
        self->config = NULL;
    }

    G_OBJECT_CLASS (gsignond_storage_manager_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondStorageManager *self = 
        GSIGNOND_STORAGE_MANAGER (object);

    if (self->location) {
        g_free (self->location);
        self->location = NULL;
    }

    G_OBJECT_CLASS (gsignond_storage_manager_parent_class)->finalize (object);
}

static gboolean
_initialize_storage (GSignondStorageManager *self)
{
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (self->location, FALSE);

    if (g_access (self->location, R_OK) == 0)
        return TRUE;

    gboolean res = FALSE;

    uid_t uid = getuid ();
    if (seteuid (0))
        WARN ("seteuid() failed");

    if (g_mkdir_with_parents (self->location, S_IRWXU | S_IRWXG))
        goto init_exit;
    if (chown (self->location, 0, getegid ()))
        WARN ("chown() failed");
    if (chmod (self->location, S_IRWXU | S_IRWXG))
        WARN ("chmod() failed");
    res = TRUE;

init_exit:
    if (seteuid (uid))
        WARN ("seteuid failed");

    return res;
}

static gboolean
_delete_storage (GSignondStorageManager *self)
{
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (self->location, FALSE);

    return gsignond_wipe_directory (self->location);
}

static gboolean
_storage_is_initialized (GSignondStorageManager *self)
{
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (self->location, FALSE);

    if (g_access (self->location, 0))  /* 0 should equal to F_OK */
        return FALSE;

    return TRUE;
}

static const gchar *
_mount_filesystem (GSignondStorageManager *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return self->location;
}

static gboolean
_unmount_filesystem (GSignondStorageManager *self)
{
    g_return_val_if_fail (self != NULL, FALSE);

    return TRUE;
}

static gboolean
_filesystem_is_mounted (GSignondStorageManager *self)
{
    return _storage_is_initialized (self);
}

/**
 * GSignondStorageManagerClass:
 * @parent_class: parent class.
 * @initialize_storage: an implementation of gsignond_storage_manager_initialize_storage()
 * @delete_storage: an implementation of gsignond_storage_manager_delete_storage()
 * @storage_is_initialized: an implementation of gsignond_storage_manager_storage_is_initialized()
 * @mount_filesystem: an implementation of gsignond_storage_manager_mount_filesystem()
 * @unmount_filesystem: an implementation of gsignond_storage_manager_unmount_filesystem()
 * @filesystem_is_mounted: an implementation of gsignond_storage_manager_filesystem_is_mounted()
 * 
 * #GSignondStorageManagerClass class containing pointers to class methods.
 */
static void
gsignond_storage_manager_class_init (GSignondStorageManagerClass *klass)
{
    GObjectClass *base = G_OBJECT_CLASS (klass);

    base->set_property = _set_property;
    base->get_property = _get_property;
    base->dispose = _dispose;
    base->finalize = _finalize;
    properties[PROP_CONFIG] = g_param_spec_object ("config",
                                                   "config",
                                                   "Configuration object",
                                                   GSIGNOND_TYPE_CONFIG,
                                                   G_PARAM_CONSTRUCT_ONLY|
                                                   G_PARAM_READWRITE|
                                                   G_PARAM_STATIC_STRINGS);
    g_object_class_install_properties (base, N_PROPERTIES, properties);

    /*g_type_class_add_private (klass, sizeof(GSignondStorageManagerPrivate));*/
 
    klass->initialize_storage = _initialize_storage;
    klass->delete_storage = _delete_storage;
    klass->storage_is_initialized = _storage_is_initialized;
    klass->mount_filesystem = _mount_filesystem;
    klass->unmount_filesystem = _unmount_filesystem;
    klass->filesystem_is_mounted = _filesystem_is_mounted;
}

static void
gsignond_storage_manager_init (GSignondStorageManager *self)
{
    /*self->priv = GSIGNOND_STORAGE_MANAGER_GET_PRIVATE (self);*/

    self->location = NULL;
    self->config = NULL;
}

/**
 * gsignond_storage_manager_initialize_storage:
 * @self: object instance.
 *
 * Initialize encryption storage. This means making sure that the 
 * necessary directories under #GSIGNOND_CONFIG_GENERAL_STORAGE_PATH exist and are accessible.
 *
 * Returns: success?
 */
gboolean
gsignond_storage_manager_initialize_storage (GSignondStorageManager *self)
{
    return GSIGNOND_STORAGE_MANAGER_GET_CLASS (self)->
        initialize_storage (self);
}

/**
 * gsignond_storage_manager_delete_storage:
 * @self: object instance.
 *
 * Destroys all the encryption keys and wipes the storage. gsignond_wipe_directory()
 * is typically used for the latter.
 *
 * Returns: success?
 */
gboolean
gsignond_storage_manager_delete_storage (GSignondStorageManager *self)
{
    return GSIGNOND_STORAGE_MANAGER_GET_CLASS (self)->
        delete_storage (self);
}

/**
 * gsignond_storage_manager_storage_is_initialized:
 * @self: object instance.
 *
 * Checks if the storage has been initialized.
 *
 * Returns: storage has been initialized?
 */
gboolean
gsignond_storage_manager_storage_is_initialized (GSignondStorageManager *self)
{
    return GSIGNOND_STORAGE_MANAGER_GET_CLASS (self)->
        storage_is_initialized (self);
}

/**
 * gsignond_storage_manager_mount_filesystem:
 * @self: object instance.
 *
 * Mounts an encrypted storage and returns the filesystem path of the storage
 * mount point. This path will be set in #GSignondConfig as 
 * #GSIGNOND_CONFIG_GENERAL_SECURE_DIR and used to access the secret database via
 * #GSignondSecretStorage.
 * 
 * The default implemenation does nothing, and immediately returns the path for the 
 * secret database.
 *
 * Returns: (transfer none): path of the storage mount point.
 */
const gchar *
gsignond_storage_manager_mount_filesystem (GSignondStorageManager *self)
{
    return GSIGNOND_STORAGE_MANAGER_GET_CLASS (self)->
        mount_filesystem (self);
}

/**
 * gsignond_storage_manager_unmount_filesystem:
 * @self: object instance.
 *
 * Unmounts a previously mounted encrypted storage filesystem.
 *
 * Returns: success?
 */
gboolean
gsignond_storage_manager_unmount_filesystem (GSignondStorageManager *self)
{
    return GSIGNOND_STORAGE_MANAGER_GET_CLASS (self)->
        unmount_filesystem (self);
}

/**
 * gsignond_storage_manager_filesystem_is_mounted:
 * @self: object instance.
 *
 * Checks if the encrypted storage filesystem is currently mounted.
 *
 * Returns: filesystem is currently mounted?
 */
gboolean
gsignond_storage_manager_filesystem_is_mounted (GSignondStorageManager *self)
{
    return GSIGNOND_STORAGE_MANAGER_GET_CLASS (self)->
        filesystem_is_mounted (self);
}

