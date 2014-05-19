/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 * Contact: Elena Reshetova <elena.reshetova@linux.intel.com>
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

#ifndef _TIZEN_ACCESS_CONTROL_MANAGER_H_
#define _TIZEN_ACCESS_CONTROL_MANAGER_H_

#include <glib-object.h>
#include <gsignond/gsignond-extension-interface.h>

G_BEGIN_DECLS

#define EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER \
    (extension_tizen_access_control_manager_get_type ())
#define EXTENSION_TIZEN_ACCESS_CONTROL_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                 EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER, \
                                 ExtensionTizenAccessControlManager))
#define EXTENSION_IS_TIZEN_ACCESS_CONTROL_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                 EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER))
#define EXTENSION_TIZEN_ACCESS_CONTROL_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                              EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER, \
                              ExtensionTizenAccessControlManagerClass))
#define EXTENSION_IS_TIZEN_ACCESS_CONTROL_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                              EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER))
#define EXTENSION_TIZEN_ACCESS_CONTROL_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER, \
                                ExtensionTizenAccessControlManagerClass))

typedef struct _ExtensionTizenAccessControlManager
    ExtensionTizenAccessControlManager;
typedef struct _ExtensionTizenAccessControlManagerClass
    ExtensionTizenAccessControlManagerClass;
typedef struct _ExtensionTizenAccessControlManagerPrivate
    ExtensionTizenAccessControlManagerPrivate;

struct _ExtensionTizenAccessControlManager
{
    GSignondAccessControlManager parent_instance;
    ExtensionTizenAccessControlManagerPrivate *priv;
};

struct _ExtensionTizenAccessControlManagerClass
{
    GSignondAccessControlManagerClass parent_class;
};

GType extension_tizen_access_control_manager_get_type ();

void
extension_tizen_access_control_manager_security_context_of_peer (
                            GSignondAccessControlManager *self,
                            GSignondSecurityContext *peer_ctx,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx);

gboolean
extension_tizen_access_control_manager_peer_is_allowed_to_use_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *identity_owner,
                            const GSignondSecurityContextList *identity_acl);

gboolean
extension_tizen_access_control_manager_peer_is_owner_of_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *identity_owner);

GSignondSecurityContext *
extension_tizen_access_control_manager_security_context_of_keychain (
                            GSignondAccessControlManager *self);

G_END_DECLS

#endif  /* _TIZEN_ACCESS_CONTROL_MANAGER_H_ */

