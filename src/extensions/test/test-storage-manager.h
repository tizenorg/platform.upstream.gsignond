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

#ifndef _TEST_EXTENSION_H_
#define _TEST_EXTENSION_H_

#include <glib-object.h>
#include <gsignond/gsignond-extension-interface.h>

G_BEGIN_DECLS

#define EXTENSION_TYPE_TEST_STORAGE_MANAGER \
    (extension_test_storage_manager_get_type ())
#define EXTENSION_TEST_STORAGE_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXTENSION_TYPE_TEST_STORAGE_MANAGER, \
                                 ExtensionTestStorageManager))
#define EXTENSION_IS_TEST(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXTENSION_TYPE_TEST_STORAGE_MANAGER))
#define EXTENSION_TEST_STORAGE_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), EXTENSION_TYPE_TEST_STORAGE_MANAGER, \
                              ExtensionTestStorageManagerClass))
#define EXTENSION_IS_TEST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), EXTENSION_TYPE_TEST_STORAGE_MANAGER))
#define EXTENSION_TEST_STORAGE_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), EXTENSION_TYPE_TEST_STORAGE_MANAGER, \
                                ExtensionTestStorageManagerClass))

typedef struct _ExtensionTestStorageManager
    ExtensionTestStorageManager;
typedef struct _ExtensionTestStorageManagerClass
    ExtensionTestStorageManagerClass;
typedef struct _ExtensionTestStorageManagerPrivate
    ExtensionTestStorageManagerPrivate;

struct _ExtensionTestStorageManager
{
    GSignondStorageManager parent_instance;
    ExtensionTestStorageManagerPrivate *priv;
};

struct _ExtensionTestStorageManagerClass
{
    GSignondStorageManagerClass parent_class;
};

GType extension_test_storage_manager_get_type ();

G_END_DECLS

#endif  /* _TEST_EXTENSION_H_ */

