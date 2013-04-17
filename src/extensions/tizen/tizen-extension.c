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

#include "tizen-extension.h"
#include "tizen-storage-manager.h"
#include "tizen-secret-storage.h"
#include "tizen-access-control-manager.h"

G_DEFINE_TYPE (ExtensionTizen, extension_tizen, GSIGNOND_TYPE_EXTENSION);

static GSignondExtension *tizen_extension = NULL;
static GSignondStorageManager *storage_manager_inst = NULL;
static GSignondSecretStorage *secret_storage_inst = NULL;
static GSignondAccessControlManager *access_control_manager_inst = NULL;

static const gchar *
_get_extension_name (GSignondExtension *self)
{
    (void) self;

    return "tizen";
}

static guint32
_get_extension_version (GSignondExtension *self)
{
    (void) self;

    return 0x01000000;
}

static GSignondStorageManager *
_get_storage_manager (GSignondExtension *self, GSignondConfig *config)
{
    (void) self;

    if (!storage_manager_inst) {
        storage_manager_inst =
            g_object_new (EXTENSION_TYPE_TIZEN_STORAGE_MANAGER,
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
            g_object_new (EXTENSION_TYPE_TIZEN_SECRET_STORAGE,
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
            g_object_new (EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER,
                          "config", config, NULL);
    }

    return access_control_manager_inst;
}

static void
extension_tizen_class_init (ExtensionTizenClass *klass)
{
    GSignondExtensionClass *parent_class = GSIGNOND_EXTENSION_CLASS (klass);

    parent_class->get_extension_name = _get_extension_name;
    parent_class->get_extension_version = _get_extension_version;
    parent_class->get_storage_manager = _get_storage_manager;
    parent_class->get_secret_storage = _get_secret_storage;
    parent_class->get_access_control_manager = _get_access_control_manager;
}

static void
extension_tizen_init (ExtensionTizen *self)
{
}

GSignondExtension *
tizen_extension_init ()
{
    if (!tizen_extension) {
        tizen_extension = g_object_new (EXTENSION_TYPE_TIZEN, NULL);
    }
    return tizen_extension;
}

