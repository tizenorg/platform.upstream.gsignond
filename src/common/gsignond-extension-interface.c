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

/**
 * SECTION:gsignond-extension-interface
 * @short_description: provides platform adaptation functionality
 * @include: gsignond/gsignond-plugin-interface.h
 *
 * #GSignondExtension provides access to platform adaptation functionality. It
 * contains getter methods for default implementations of #GSignondAccessControlManager, 
 * #GSignondSecretStorage and #GSignondStorageManager.
 *
 * gSSO can be adapted to a specific platform environment by implementing a 
 * custom extension module. The following steps need to be taken:
 * 
 * a) subclass and re-implement some (or all) of the functionality of the above
 * three classes.
 * 
 * b) subclass #GSignondExtension and provide implementations of its getter methods for those
 * of the adaptation classes that have been changed.
 * 
 * d) provide a function <function>GSignondExtension * extensionname_extension_init(void)</function>
 * that returns an instance of the #GSignondExtension subclass.
 * 
 * c) build and install these implementations as a gSSO extension module and
 * configure gSSO to use it.
 * 
 * Examples of custom extensions can be seen here:
 * <ulink url="https://code.google.com/p/accounts-sso/source/browse/?repo=gsignond#git%2Fsrc%2Fextensions">
 * https://code.google.com/p/accounts-sso/source/browse/?repo=gsignond#git%2Fsrc%2Fextensions</ulink>
 * and gSSO configuration is described in #GSignondConfig.
 */
/**
 * GSignondExtension:
 *
 * Opaque #GSignondExtension data structure.
 */

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

/**
 * GSignondExtensionClass:
 * @parent_class: the parent class
 * @get_extension_name: implementation of gsignond_extension_get_name()
 * @get_extension_version: implementation of gsignond_extension_get_version()
 * @get_storage_manager: implementation of gsignond_extension_get_storage_manager()
 * @get_secret_storage: implementation of gsignond_extension_get_secret_storage()
 * @get_access_control_manager: implementation of gsignond_extension_get_access_control_manager()
 * 
 * #GSignondExtensionClass class containing pointers to class methods.
 */
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

/**
 * gsignond_extension_get_name:
 * @self: object instance.
 *
 * Get a human readable name of the extension. Default implementation
 * returns "default".
 *
 * Returns: (transfer none): name of the extension.
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
 * major, minor, patchlevel, build. Default implementation returns 0.
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
 * Factory method to get a singleton storage manager object. See 
 * #GSignondStorageManager for the description of the default implementation.
 *
 * Returns: (transfer none): storage manager object instance.
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
 * Factory method to get a singleton secret storage object. See 
 * #GSignondSecretStorage for the description of the default implementation.
 *
 * Returns: (transfer none): secret storage object instance.
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
 * Factory method to get a singleton access control manager object. See 
 * #GSignondAccessControlManager for the description of the default implementation.
 *
 * Returns: (transfer none): access control manager object instance.
 */
GSignondAccessControlManager *
gsignond_extension_get_access_control_manager (GSignondExtension *self,
                                               GSignondConfig *config)
{
    return GSIGNOND_EXTENSION_GET_CLASS (self)->
        get_access_control_manager (self, config);
}

