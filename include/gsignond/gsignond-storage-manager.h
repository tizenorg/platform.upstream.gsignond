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

#ifndef _GSIGNOND_STORAGE_MANAGER_H_
#define _GSIGNOND_STORAGE_MANAGER_H_

#include <glib.h>
#include <glib-object.h>

#include <gsignond/gsignond-config.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_STORAGE_MANAGER \
    (gsignond_storage_manager_get_type ())
#define GSIGNOND_STORAGE_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_STORAGE_MANAGER, \
                                 GSignondStorageManager))
#define GSIGNOND_IS_STORAGE_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_STORAGE_MANAGER))
#define GSIGNOND_STORAGE_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_STORAGE_MANAGER, \
                              GSignondStorageManagerClass))
#define GSIGNOND_IS_STORAGE_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_STORAGE_MANAGER))
#define GSIGNOND_STORAGE_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_STORAGE_MANAGER, \
                                GSignondStorageManagerClass))

typedef struct _GSignondStorageManager GSignondStorageManager;
typedef struct _GSignondStorageManagerClass GSignondStorageManagerClass;
typedef struct _GSignondStorageManagerPrivate GSignondStorageManagerPrivate;

struct _GSignondStorageManager
{
    GObject parent_instance;
    GSignondConfig *config;
    gchar *location;
    GSignondStorageManagerPrivate *priv;
};

struct _GSignondStorageManagerClass
{
    GObjectClass parent_class;

    /**
     * initialize_storage:
     *
     * See #gsignond_storage_manager_initialize_storage.
     */
    gboolean (*initialize_storage) (GSignondStorageManager *self);
    /**
     * delete_storage:
     *
     * See #gsignond_storage_manager_delete_storage.
     */
    gboolean (*delete_storage) (GSignondStorageManager *self);
    /**
     * storage_is_initialized:
     *
     * See #gsignond_storage_manager_storage_is_initialized.
     */
    gboolean (*storage_is_initialized) (GSignondStorageManager *self);
    /**
     * mount_filesystem:
     *
     * See #gsignond_storage_manager_mount_filesystem.
     */
    const gchar * (*mount_filesystem) (GSignondStorageManager *self);
    /**
     * unmount_filesystem:
     *
     * See #gsignond_storage_manager_unmount_filesystem.
     */
    gboolean (*unmount_filesystem) (GSignondStorageManager *self);
    /**
     * filesystem_is_mounted:
     *
     * See #gsignond_storage_manager_filesystem_is_mounted.
     */
    gboolean (*filesystem_is_mounted) (GSignondStorageManager *self);
};

GType gsignond_storage_manager_get_type ();

gboolean
gsignond_storage_manager_initialize_storage (GSignondStorageManager *self);

gboolean
gsignond_storage_manager_delete_storage (GSignondStorageManager *self);

gboolean
gsignond_storage_manager_storage_is_initialized (GSignondStorageManager *self);

const gchar *
gsignond_storage_manager_mount_filesystem (GSignondStorageManager *self);

gboolean
gsignond_storage_manager_unmount_filesystem (GSignondStorageManager *self);

gboolean
gsignond_storage_manager_filesystem_is_mounted (GSignondStorageManager *self);

G_END_DECLS

#endif  /* _GSIGNOND_STORAGE_MANAGER_H_ */

