/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2013 Intel Corporation.
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

#ifndef _GSIGNOND_ACCESS_CONTROL_MANAGER_H_
#define _GSIGNOND_ACCESS_CONTROL_MANAGER_H_

#include <glib-object.h>

#include <gsignond/gsignond-config.h>
#include <gsignond/gsignond-security-context.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER \
    (gsignond_access_control_manager_get_type ())
#define GSIGNOND_ACCESS_CONTROL_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER, \
                                 GSignondAccessControlManager))
#define GSIGNOND_IS_ACCESS_CONTROL_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER))
#define GSIGNOND_ACCESS_CONTROL_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER, \
                              GSignondAccessControlManagerClass))
#define GSIGNOND_IS_ACCESS_CONTROL_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER))
#define GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER, \
                                GSignondAccessControlManagerClass))

typedef struct _GSignondAccessControlManager
               GSignondAccessControlManager;
typedef struct _GSignondAccessControlManagerClass
               GSignondAccessControlManagerClass;
typedef struct _GSignondAccessControlManagerPrivate
               GSignondAccessControlManagerPrivate;

struct _GSignondAccessControlManager
{
    GObject parent_instance;
    GSignondConfig *config;

    /* private */
    GSignondAccessControlManagerPrivate *priv;
};

struct _GSignondAccessControlManagerClass
{
    GObjectClass parent_class;

    void (*security_context_of_peer) (
                            GSignondAccessControlManager *self,
                            GSignondSecurityContext *peer_ctx,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx);
    gboolean (*peer_is_allowed_to_use_identity) (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx,
                            const GSignondSecurityContextList *identity_acl);
    gboolean (*peer_is_owner_of_identity) (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx);
    gboolean (*acl_is_valid) (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContextList *identity_acl);
    GSignondSecurityContext * (*security_context_of_keychain) (
                            GSignondAccessControlManager *self);
};

GType gsignond_access_control_manager_get_type ();

void
gsignond_access_control_manager_security_context_of_peer (
                            GSignondAccessControlManager *self,
                            GSignondSecurityContext *peer_ctx,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx);

gboolean
gsignond_access_control_manager_peer_is_allowed_to_use_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx,
                            const GSignondSecurityContextList *identity_acl);

gboolean
gsignond_access_control_manager_peer_is_owner_of_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx);

gboolean
gsignond_access_control_manager_acl_is_valid (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContextList *identity_acl);

GSignondSecurityContext *
gsignond_access_control_manager_security_context_of_keychain (
                            GSignondAccessControlManager *self);


G_END_DECLS

#endif  /* _GSIGNOND_ACCESS_CONTROL_MANAGER_H_ */

