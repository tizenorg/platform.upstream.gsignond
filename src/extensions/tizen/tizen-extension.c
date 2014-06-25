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

#define EXTENSION_TIZEN_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), EXTENSION_TYPE_TIZEN, ExtensionTizenPrivate)

struct _ExtensionTizenPrivate
{
    GSignondAccessControlManager *access_control_manager;
    GSignondStorageManager *storage_manager;
    GSignondSecretStorage *secret_storage;
};

static void
_dispose (GObject *obj)
{
    ExtensionTizen *self = EXTENSION_TIZEN (obj);
    if (!self) return;

    ExtensionTizenPrivate *priv = self->priv;

    if (priv) {
        g_clear_object (&priv->access_control_manager);
        g_clear_object (&priv->secret_storage);
        g_clear_object (&priv->storage_manager);
    }

    G_OBJECT_CLASS (extension_tizen_parent_class)->dispose (obj);
}

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
    g_return_val_if_fail (self && EXTENSION_IS_TIZEN(self), NULL);

    ExtensionTizenPrivate *priv = EXTENSION_TIZEN(self)->priv;

    if (!priv->storage_manager) {
        priv->storage_manager =
            g_object_new (EXTENSION_TYPE_TIZEN_STORAGE_MANAGER,
                          "config", config, NULL);
    }
    return priv->storage_manager;
}

static GSignondSecretStorage *
_get_secret_storage (GSignondExtension *self, GSignondConfig *config)
{
    g_return_val_if_fail (self && EXTENSION_IS_TIZEN(self), NULL);

    ExtensionTizenPrivate *priv = EXTENSION_TIZEN(self)->priv;

    if (!priv->secret_storage) {
        priv->secret_storage =
            g_object_new (EXTENSION_TYPE_TIZEN_SECRET_STORAGE,
                          "config", config, NULL);
    }

    return priv->secret_storage;
}

static GSignondAccessControlManager *
_get_access_control_manager (GSignondExtension *self, GSignondConfig *config)
{
    g_return_val_if_fail (self && EXTENSION_IS_TIZEN(self), NULL);

    ExtensionTizenPrivate *priv = EXTENSION_TIZEN(self)->priv;

    if (!priv->access_control_manager) {
        priv->access_control_manager =
            g_object_new (EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER,
                          "config", config, NULL);
    }

    return priv->access_control_manager;
}

static void
extension_tizen_class_init (ExtensionTizenClass *klass)
{
    GSignondExtensionClass *parent_class = GSIGNOND_EXTENSION_CLASS (klass);

    g_type_class_add_private (G_OBJECT_CLASS(klass), sizeof (ExtensionTizenPrivate));

    G_OBJECT_CLASS (klass)->dispose = _dispose;
    parent_class->get_extension_name = _get_extension_name;
    parent_class->get_extension_version = _get_extension_version;
    parent_class->get_storage_manager = _get_storage_manager;
    parent_class->get_secret_storage = _get_secret_storage;
    parent_class->get_access_control_manager = _get_access_control_manager;
}

static void
extension_tizen_init (ExtensionTizen *self)
{
    self->priv = EXTENSION_TIZEN_PRIV (self);

    self->priv->storage_manager = NULL;
    self->priv->secret_storage = NULL;
    self->priv->access_control_manager = NULL;
}

static void
_on_object_dispose (gpointer data, GObject *object)
{
    if (data) *(ExtensionTizen **)data = NULL;
}

GSignondExtension *
tizen_extension_init ()
{
    static GSignondExtension *tizen_extension  = NULL;

    if (!tizen_extension) {
        tizen_extension = g_object_new (EXTENSION_TYPE_TIZEN, NULL);

        g_object_weak_ref (G_OBJECT (tizen_extension), _on_object_dispose, &tizen_extension);
    }

    return tizen_extension;
}

