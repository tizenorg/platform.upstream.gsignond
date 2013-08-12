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

#ifndef _GSIGNOND_EXTENSION_INTERFACE_H_
#define _GSIGNOND_EXTENSION_INTERFACE_H_

#include <glib-object.h>

#include <gsignond/gsignond-config.h>
#include <gsignond/gsignond-storage-manager.h>
#include <gsignond/gsignond-secret-storage.h>
#include <gsignond/gsignond-access-control-manager.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_EXTENSION \
    (gsignond_extension_get_type ())
#define GSIGNOND_EXTENSION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_EXTENSION, \
                                 GSignondExtension))
#define GSIGNOND_IS_EXTENSION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_EXTENSION))
#define GSIGNOND_EXTENSION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_EXTENSION, \
                              GSignondExtensionClass))
#define GSIGNOND_IS_EXTENSION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_EXTENSION))
#define GSIGNOND_EXTENSION_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_EXTENSION, \
                                GSignondExtensionClass))

typedef struct _GSignondExtension GSignondExtension;
typedef struct _GSignondExtensionClass GSignondExtensionClass;
typedef struct _GSignondExtensionPrivate GSignondExtensionPrivate;
typedef GSignondExtension * (*GSignondExtensionInit) (void);

struct _GSignondExtension
{
    GObject parent_instance;
    GSignondExtensionPrivate *priv;
};

struct _GSignondExtensionClass
{
    GObjectClass parent_class;

    /**
     * get_extension_name:
     * @self: object instance.
     *
     * Get human readable name of the extension.
     * 
     * Returns: (transfer none): name of the extension.
     */
    const gchar * (*get_extension_name) (GSignondExtension *self);
    /**
     * get_extension_version:
     * @self: object instance.
     *
     * Get version of the extension, split into four bytes in order from MSB
     * to LSB; major, minor, patchlevel, build.
     */
    guint32 (*get_extension_version) (GSignondExtension *self);
    /**
     * get_storage_manager:
     * @self: object instance.
     * @config: configuration object instance.
     *
     * Factory method to get a singleton storage manager object.
     *
     * Returns: storage manager object instance.
     */
    GSignondStorageManager * (*get_storage_manager) (GSignondExtension *self,
                                                     GSignondConfig *config);
    /**
     * get_secret_storage:
     * @self: object instance.
     * @config: configuration object instance.
     *
     * Factory method to get a singleton secret storage object.
     *
     * Returns: secret storage object instance.
     */
    GSignondSecretStorage * (*get_secret_storage) (GSignondExtension *self,
                                                   GSignondConfig *config);
    /**
     * get_access_control_manager:
     * @self: object instance.
     * @config: configuration object instance.
     *
     * Factory method to get a singleton access control manager object.
     *
     * Returns: access control manager object instance.
     */
    GSignondAccessControlManager * (*get_access_control_manager) (
                                                    GSignondExtension *self,
                                                    GSignondConfig *config);
};

GType gsignond_extension_get_type ();

const gchar *
gsignond_extension_get_name (GSignondExtension *self);

guint32
gsignond_extension_get_version (GSignondExtension *self);

GSignondStorageManager *
gsignond_extension_get_storage_manager (GSignondExtension *self,
                                        GSignondConfig *config);

GSignondSecretStorage *
gsignond_extension_get_secret_storage (GSignondExtension *self,
                                       GSignondConfig *config);

GSignondAccessControlManager *
gsignond_extension_get_access_control_manager (GSignondExtension *self,
                                               GSignondConfig *config);

G_END_DECLS

#endif  /* _GSIGNOND_EXTENSION_INTERFACE_H_ */

