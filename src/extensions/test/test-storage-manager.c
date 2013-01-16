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

#include "test-storage-manager.h"

#define EXTENSION_TEST_STORAGE_MANAGER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  EXTENSION_TYPE_TEST_STORAGE_MANAGER, \
                                  ExtensionTestStorageManagerPrivate))

struct _ExtensionTestStorageManagerPrivate
{
};

G_DEFINE_TYPE (ExtensionTestStorageManager,
               extension_test_storage_manager,
               GSIGNOND_TYPE_STORAGE_MANAGER);

static void
extension_test_storage_manager_class_init (
                                        ExtensionTestStorageManagerClass *klass)
{
    (void) klass;

    /*GObjectClass *base = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass,
                              sizeof(ExtensionTestStorageManagerPrivate));*/
}

static void
extension_test_storage_manager_init (ExtensionTestStorageManager *self)
{
    GSignondStorageManager *parent;
    /*self->priv = EXTENSION_TEST_STORAGE_MANAGER_GET_PRIVATE (self);*/

    parent = GSIGNOND_STORAGE_MANAGER (self);
    parent->location = g_strdup (g_getenv ("EXTENSION_TEST_DATADIR"));
    if (!parent->location) {
        parent->location =
            g_build_filename (g_get_tmp_dir (), "gsignond-test", NULL);
    }
}

