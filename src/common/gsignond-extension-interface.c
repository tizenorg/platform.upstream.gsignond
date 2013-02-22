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

static GSignondExtension *default_extension = NULL;
static GSignondStorageManager *storage_manager_inst = NULL;
static GSignondSecretStorage *secret_storage_inst = NULL;
static GSignondAccessControlManager *access_control_manager_inst = NULL;

static void
_dispose (GObject *object)
{
    g_clear_object (&storage_manager_inst);
    g_clear_object (&secret_storage_inst);
    g_clear_object (&access_control_manager_inst);

    G_OBJECT_CLASS (gsignond_extension_parent_class)->dispose (object);

    default_extension = NULL;
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
    (void) self;

    if (!storage_manager_inst) {
        storage_manager_inst =
            g_object_new (GSIGNOND_TYPE_STORAGE_MANAGER,
                          "config", config, NULL);
    }
    return storage_manager_inst;
}

static GSignondSecretStorage *
_get_secret_storage (GSignondExtension *self, GSignondConfig *config)
{
    (void) self;

    if (!secret_storage_inst) {
        secret_storage_inst =
            g_object_new (GSIGNOND_TYPE_SECRET_STORAGE,
                          "config", config, NULL);
    }

    return secret_storage_inst;
}

static GSignondAccessControlManager *
_get_access_control_manager (GSignondExtension *self, GSignondConfig *config)
{
    (void) self;

    if (!access_control_manager_inst) {
        access_control_manager_inst =
            g_object_new (GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER,
                          "config", config, NULL);
    }

    return access_control_manager_inst;
}


static void
gsignond_extension_class_init (GSignondExtensionClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = _dispose;
    klass->get_extension_name = _get_extension_name;
    klass->get_extension_version = _get_extension_version;
    klass->get_storage_manager = _get_storage_manager;
    klass->get_secret_storage = _get_secret_storage;
    klass->get_access_control_manager = _get_access_control_manager;
}

static void
gsignond_extension_init (GSignondExtension *self)
{
}

GSignondExtension * default_extension_init ()
{
    if (!default_extension) {
        default_extension =
            g_object_new (GSIGNOND_TYPE_EXTENSION, NULL);
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

