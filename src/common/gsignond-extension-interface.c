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

#include "gsignond/gsignond-extension-interface.h"

G_DEFINE_TYPE (GSignondExtension, gsignond_extension, G_TYPE_OBJECT);

#define GSIGNOND_EXTENSION_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_EXTENSION, GSignondExtensionPrivate)

struct _GSignondExtensionPrivate 
{
    GSignondAccessControlManager *access_control_manager;
    GSignondStorageManager *storage_manager;
    GSignondSecretStorage *secret_storage;
};

static void
_dispose (GObject *object)
{
    GSignondExtensionPrivate *priv = GSIGNOND_EXTENSION (object)->priv;

    g_clear_object (&priv->access_control_manager);
    g_clear_object (&priv->secret_storage);
    g_clear_object (&priv->storage_manager);

    G_OBJECT_CLASS (gsignond_extension_parent_class)->dispose (object);
}

static const gchar *
_get_extension_name (GSignondExtension *self)
{
    (void) self;

    return "default";
}

static guint32
_get_extension_version (GSignondExtension *self)
{
    (void) self;

    return 0;
}

static GSignondStorageManager *
_get_storage_manager (GSignondExtension *self, GSignondConfig *config)
{
    g_return_val_if_fail (self && GSIGNOND_IS_EXTENSION (self), NULL);

    GSignondExtensionPrivate *priv = self->priv;

    if (!priv->storage_manager) {
        priv->storage_manager =
            g_object_new (GSIGNOND_TYPE_STORAGE_MANAGER,
                          "config", config, NULL);
    }

    return priv->storage_manager;
}

static GSignondSecretStorage *
_get_secret_storage (GSignondExtension *self, GSignondConfig *config)
{
    g_return_val_if_fail (self && GSIGNOND_IS_EXTENSION (self), NULL);

    GSignondExtensionPrivate *priv = self->priv;

    if (!priv->secret_storage) {
        priv->secret_storage =
            g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
                          "config", config, NULL);
    }

    return priv->secret_storage;
}

static GSignondAccessControlManager *
_get_access_control_manager (GSignondExtension *self, GSignondConfig *config)
{
    g_return_val_if_fail (self && GSIGNOND_IS_EXTENSION (self), NULL);

    GSignondExtensionPrivate *priv = self->priv;

    if (!priv->access_control_manager) {
        priv->access_control_manager =
            g_object_new (GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER,
                          "config", config, NULL);
    }

    return priv->access_control_manager;
}

static void
gsignond_extension_class_init (GSignondExtensionClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_type_class_add_private (g_klass, sizeof (GSignondExtensionPrivate));

    g_klass->dispose = _dispose;
    klass->get_extension_name = _get_extension_name;
    klass->get_extension_version = _get_extension_version;
    klass->get_storage_manager = _get_storage_manager;
    klass->get_secret_storage = _get_secret_storage;
    klass->get_access_control_manager = _get_access_control_manager;
}

static void
gsignond_extension_init (GSignondExtension *self)
{
    self->priv = GSIGNOND_EXTENSION_PRIV (self);

    self->priv->access_control_manager = NULL;
    self->priv->storage_manager = NULL;
    self->priv->secret_storage = NULL;
}

static void
_on_extension_dispose (gpointer data, GObject *object)
{
    if (data) *(GSignondExtension **)data = NULL;
}

GSignondExtension * default_extension_init ()
{
    static GSignondExtension *default_extension = NULL;

    if (!default_extension) {
        default_extension =
            g_object_new (GSIGNOND_TYPE_EXTENSION, NULL);
        
        g_object_weak_ref (G_OBJECT (default_extension),
                           _on_extension_dispose,
                           &default_extension);
    }

    return default_extension;
}

/**
 * gsignond_extension_get_name:
 * @self: object instance.
 *
 * Get human readable name of the extension.
 *
 * Returns: (transfer none) name of the extension.
 */
const gchar *
gsignond_extension_get_name (GSignondExtension *self)
{
    return GSIGNOND_EXTENSION_GET_CLASS (self)->get_extension_name (self);
}

/**
 * gsignond_extension_get_version:
 * @self: object instance.
 *
 * Get version of the extension, split into four bytes in order from MSB to LSB;
 * major, minor, patchlevel, build.
 */
guint32
gsignond_extension_get_version (GSignondExtension *self)
{
    return GSIGNOND_EXTENSION_GET_CLASS (self)->get_extension_version (self);
}

/**
 * gsignond_extension_get_storage_manager:
 * @self: object instance.
 * @config: configuration object instance.
 *
 * Factory method to get a singleton storage manager object.
 *
 * Returns: (transfer none) storage manager object instance.
 */
GSignondStorageManager *
gsignond_extension_get_storage_manager (GSignondExtension *self,
                                        GSignondConfig *config)
{
    return GSIGNOND_EXTENSION_GET_CLASS (self)->
        get_storage_manager (self, config);
}

/**
 * gsignond_extension_get_secret_storage:
 * @self: object instance.
 * @config: configuration object instance.
 *
 * Factory method to get a singleton secret storage object.
 *
 * Returns: (transfer none) secret storage object instance.
 */
GSignondSecretStorage *
gsignond_extension_get_secret_storage (GSignondExtension *self,
                                       GSignondConfig *config)
{
    return GSIGNOND_EXTENSION_GET_CLASS (self)->
        get_secret_storage (self, config);
}

/**
 * gsignond_extension_get_access_control_manager:
 * @self: object instance.
 * @config: configuration object instance.
 *
 * Factory method to get a singleton access control manager object.
 *
 * Returns: (transfer none) access control manager object instance.
 */
GSignondAccessControlManager *
gsignond_extension_get_access_control_manager (GSignondExtension *self,
                                               GSignondConfig *config)
{
    return GSIGNOND_EXTENSION_GET_CLASS (self)->
        get_access_control_manager (self, config);
}

