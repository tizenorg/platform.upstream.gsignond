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

#include "test-secret-storage.h"

#define EXTENSION_TEST_SECRET_STORAGE_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  EXTENSION_TYPE_TEST_SECRET_STORAGE, \
                                  ExtensionTestSecretStoragePrivate))

struct _ExtensionTestSecretStoragePrivate
{
};

G_DEFINE_TYPE (ExtensionTestSecretStorage,
               extension_test_secret_storage,
               GSIGNOND_TYPE_SECRET_STORAGE);

static void
extension_test_secret_storage_class_init (
                                         ExtensionTestSecretStorageClass *klass)
{
    (void) klass;

    /*GObjectClass *base = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass,
                              sizeof(ExtensionTestSecretStoragePrivate));*/
}

static void
extension_test_secret_storage_init (ExtensionTestSecretStorage *self)
{
    /*self->priv = EXTENSION_TEST_SECRET_STORAGE_GET_PRIVATE (self);*/
}

