/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
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

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <mntent.h>
#include <ecryptfs.h>

#include "tizen-storage-manager.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-utils.h"

#define EXTENSION_TIZEN_STORAGE_MANAGER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  EXTENSION_TYPE_TIZEN_STORAGE_MANAGER, \
                                  ExtensionTizenStorageManagerPrivate))

/* these are limited by ecryptfs */
#define KEY_BYTES 16
#define KEY_CIPHER "aes"

struct _ExtensionTizenStorageManagerPrivate
{
    gchar *cdir;
    gchar fekey[ECRYPTFS_MAX_PASSPHRASE_BYTES + 1];
    gchar fesalt[ECRYPTFS_SALT_SIZE + 1];
    gchar ksig[ECRYPTFS_SIG_SIZE_HEX + 1];
};

enum
{
    PROP_0,
    PROP_FEKEY,
    PROP_FESALT,
    N_PROPERTIES,
    PROP_CONFIG
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (ExtensionTizenStorageManager,
               extension_tizen_storage_manager,
               GSIGNOND_TYPE_STORAGE_MANAGER);

static void
_set_config (ExtensionTizenStorageManager *self, GSignondConfig *config)
{
    GSignondStorageManager *parent = GSIGNOND_STORAGE_MANAGER (self);
    g_assert (parent->config == NULL);
    g_assert (self->priv->cdir == NULL);
    parent->config = config;

    parent->location = g_strdup (gsignond_config_get_string (config,
                                                             GSIGNOND_CONFIG_GENERAL_SECURE_DIR));
    if (!parent->location)
        parent->location = g_build_filename (g_get_user_data_dir (),
                                             "gsignond", NULL);

    self->priv->cdir = g_strdup_printf ("%s.efs", parent->location);
    DBG ("location %s encryption point %s", parent->location, self->priv->cdir);
}

static void
_set_property (GObject *object, guint prop_id, const GValue *value,
               GParamSpec *pspec)
{
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (object);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    switch (prop_id) {
        case PROP_CONFIG:
            _set_config (self, GSIGNOND_CONFIG (g_value_dup_object (value)));
            break;
        case PROP_FEKEY:
            g_strlcpy (priv->fekey,
                       g_value_get_string (value),
                       sizeof(priv->fekey));
            break;
        case PROP_FESALT:
            g_strlcpy (priv->fesalt,
                       g_value_get_string (value),
                       sizeof(priv->fesalt));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (object);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    switch (prop_id) {
        case PROP_CONFIG:
            g_value_set_object (value,
                                GSIGNOND_STORAGE_MANAGER (self)->config);
            break;
        case PROP_FEKEY:
            g_value_set_string (value, priv->fekey);
            break;
        case PROP_FESALT:
            g_value_set_string (value, priv->fesalt);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    G_OBJECT_CLASS (extension_tizen_storage_manager_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (object);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    if (priv)
        memset(priv->ksig, 0x00, sizeof(priv->ksig));
    g_free (priv->cdir);

    G_OBJECT_CLASS (extension_tizen_storage_manager_parent_class)->finalize (object);
}

static gboolean
_initialize_storage (GSignondStorageManager *parent)
{
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (parent);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    g_return_val_if_fail (parent->location, FALSE);
    DBG ("create mount point %s", parent->location);
    if (g_mkdir_with_parents (parent->location, S_IRWXU))
        return FALSE;

    g_return_val_if_fail (priv->cdir, FALSE);
    DBG ("create storage point %s", priv->cdir);
    if (g_mkdir_with_parents (priv->cdir, S_IRWXU))
        return FALSE;

    return TRUE;
}

static gboolean
_storage_is_initialized (GSignondStorageManager *parent)
{
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (parent);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    g_return_val_if_fail (priv->cdir, FALSE);

    if (g_access (priv->cdir, 0) || g_access (parent->location, 0))
        return FALSE;

    return TRUE;
}

static const gchar *
_mount_filesystem (GSignondStorageManager *parent)
{
    gchar *retval = NULL;
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (parent);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    DBG ("add passphrase to kernel keyring");
    if (ecryptfs_add_passphrase_key_to_keyring (priv->ksig,
                                                priv->fekey,
                                                priv->fesalt) < 0)
        return NULL;

    gchar *mntopts = g_strdup_printf (
                                      "ecryptfs_check_dev_ruid" \
                                      ",ecryptfs_cipher=%s" \
                                      ",ecryptfs_key_bytes=%d" \
                                      ",ecryptfs_unlink_sigs" \
                                      ",ecryptfs_sig=%s",
                                      KEY_CIPHER, KEY_BYTES,
                                      priv->ksig);
    DBG ("mount options: %s", mntopts);
    uid_t uid = getuid ();
    gid_t gid = getgid ();
    setreuid (-1, 0);
    setregid (-1, 0);
    DBG ("perform mount %s -> %s", priv->cdir, parent->location);
    if (mount (priv->cdir, parent->location,
               "ecryptfs", MS_NOSUID | MS_NODEV, mntopts)) {
        INFO ("mount failed %d: %s", errno, strerror(errno));
        goto _mount_exit;
    }

    DBG ("mount succeeded at %s", parent->location);
    retval = parent->location;

_mount_exit:
    g_free (mntopts);
    setreuid (-1, uid);
    setregid (-1, gid);

    return retval;
}

static gboolean
_unmount_filesystem (GSignondStorageManager *parent)
{
    g_return_val_if_fail (parent != NULL, FALSE);

    uid_t uid = getuid ();
    gid_t gid = getgid ();
    setreuid (-1, 0);
    setregid (-1, 0);
    umount (parent->location);
    setreuid (-1, uid);
    setregid (-1, gid);

    return TRUE;
}

static gboolean
_filesystem_is_mounted (GSignondStorageManager *parent)
{
    gboolean retval = FALSE;
    FILE *mntf = setmntent("/proc/mounts", "r");
    g_return_val_if_fail (mntf != NULL, FALSE);
    
    struct mntent *me;
    while ((me = getmntent(mntf))) {
        if (g_strcmp0 (parent->location, me->mnt_dir) == 0) {
            retval = TRUE;
            break;
        }
    }

    endmntent(mntf);

    return retval;
}

static gboolean
_delete_storage (GSignondStorageManager *parent)
{
    ExtensionTizenStorageManager *self =
        EXTENSION_TIZEN_STORAGE_MANAGER (parent);
    ExtensionTizenStorageManagerPrivate *priv = self->priv;

    g_return_val_if_fail (priv->cdir, FALSE);
    g_return_val_if_fail (!_filesystem_is_mounted(parent), FALSE);

    return (gsignond_wipe_directory (priv->cdir) &&
            gsignond_wipe_directory (parent->location));
}

static void
extension_tizen_storage_manager_class_init (
                                      ExtensionTizenStorageManagerClass *klass)
{
    GObjectClass *base = G_OBJECT_CLASS (klass);

    base->set_property = _set_property;
    base->get_property = _get_property;
    base->dispose = _dispose;
    base->finalize = _finalize;

    properties[PROP_FEKEY] = g_param_spec_string ("fekey",
                                                  "fekey",
                                                  "File encryption key",
                                                  "0123456789",
                                                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    properties[PROP_FESALT] = g_param_spec_string ("fesalt",
                                                   "fesalt",
                                                   "File encryption salt",
                                                   "9876543210",
                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_properties (base, N_PROPERTIES, properties);
    g_object_class_override_property (base, PROP_CONFIG, "config");

    g_type_class_add_private (klass,
                              sizeof(ExtensionTizenStorageManagerPrivate));

    GSignondStorageManagerClass *parent_class =
        GSIGNOND_STORAGE_MANAGER_CLASS (klass);
    parent_class->initialize_storage = _initialize_storage;
    parent_class->delete_storage = _delete_storage;
    parent_class->storage_is_initialized = _storage_is_initialized;
    parent_class->mount_filesystem = _mount_filesystem;
    parent_class->unmount_filesystem = _unmount_filesystem;
    parent_class->filesystem_is_mounted = _filesystem_is_mounted;
}

static void
extension_tizen_storage_manager_init (ExtensionTizenStorageManager *self)
{
    ExtensionTizenStorageManagerPrivate *priv =
        EXTENSION_TIZEN_STORAGE_MANAGER_GET_PRIVATE (self);
    self->priv = priv;

    g_strlcpy (priv->fekey, "1234567890", sizeof(priv->fekey));
    g_strlcpy (priv->fesalt, "0987654321", sizeof(priv->fesalt));
}

