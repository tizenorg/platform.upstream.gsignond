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

#include "gsignond/gsignond-access-control-manager.h"

#define GSIGNOND_ACCESS_CONTROL_MANAGER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER, \
                                  GSignondAccessControlManagerPrivate))

struct _GSignondAccessControlManagerPrivate
{
};

enum
{
    PROP_0,
    PROP_CONFIG,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (GSignondAccessControlManager, gsignond_access_control_manager,
               G_TYPE_OBJECT);

static void
_set_property (GObject *object, guint prop_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondAccessControlManager *self =
        GSIGNOND_ACCESS_CONTROL_MANAGER (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_assert (self->config == NULL);
            self->config = g_value_dup_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GSignondAccessControlManager *self =
        GSIGNOND_ACCESS_CONTROL_MANAGER (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_value_set_object (value, self->config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    GSignondAccessControlManager *self =
        GSIGNOND_ACCESS_CONTROL_MANAGER (object);

    if (self->config) {
        g_object_unref (self->config);
        self->config = NULL;
    }

    G_OBJECT_GET_CLASS (object)->dispose (object);
}

static void
_security_context_of_peer (GSignondAccessControlManager *self,
                           GSignondSecurityContext *peer_ctx,
                           int peer_fd, const gchar *peer_service,
                           const gchar *peer_app_ctx)
{
    (void) self;
    (void) peer_fd;
    (void) peer_app_ctx;

    gsignond_security_context_set_system_context(peer_ctx, "");
    gsignond_security_context_set_application_context(peer_ctx, "");
}

static gboolean
_peer_is_allowed_to_use_identity (GSignondAccessControlManager *self,
                                int peer_fd, const gchar *peer_service,
                                const gchar *peer_app_ctx,
                                const GSignondSecurityContextList *identity_acl)
{
    (void) self;
    (void) peer_fd;
    (void) peer_app_ctx;
    (void) identity_acl;

    return TRUE;
}

static gboolean
_peer_is_owner_of_identity (GSignondAccessControlManager *self,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx,
                            const GSignondSecurityContext *identity_owner)
{
    (void) self;
    (void) peer_fd;
    (void) peer_app_ctx;
    (void) identity_owner;

    return TRUE;
}

static gboolean
_acl_is_valid (GSignondAccessControlManager *self,
               int peer_fd, const gchar *peer_service,
               const gchar *peer_app_ctx,
               const GSignondSecurityContextList *identity_acl)
{
    (void) self;
    (void) peer_fd;
    (void) peer_app_ctx;
    (void) identity_acl;

    return TRUE;
}

GSignondSecurityContext *
_security_context_of_keychain (GSignondAccessControlManager *self)
{
    (void) self;

    return gsignond_security_context_new_from_values ("", "");
}

static void
gsignond_access_control_manager_class_init (
                                       GSignondAccessControlManagerClass *klass)
{
    GObjectClass *base = G_OBJECT_CLASS (klass);

    base->set_property = _set_property;
    base->get_property = _get_property;
    base->dispose = _dispose;
    properties[PROP_CONFIG] = g_param_spec_object ("config",
                                                   "config",
                                                   "Configuration object",
                                                   GSIGNOND_TYPE_CONFIG,
                                                   G_PARAM_CONSTRUCT_ONLY|
                                                   G_PARAM_READWRITE);
    g_object_class_install_properties (base, N_PROPERTIES, properties);

    /*g_type_class_add_private (klass,
                              sizeof(GSignondAccessControlManagerPrivate));*/

    klass->security_context_of_peer = _security_context_of_peer;
    klass->peer_is_allowed_to_use_identity = _peer_is_allowed_to_use_identity;
    klass->peer_is_owner_of_identity = _peer_is_owner_of_identity;
    klass->acl_is_valid = _acl_is_valid;
    klass->security_context_of_keychain = _security_context_of_keychain;
}

static void
gsignond_access_control_manager_init (GSignondAccessControlManager *self)
{
    /*self->priv = GSIGNOND_ACCESS_CONTROL_MANAGER_GET_PRIVATE (self);*/

    self->config = NULL;
}

/**
 * gsignond_access_control_manager_security_context_of_peer:
 * @self: object instance.
 * @peer_ctx: instance of security context to be set.
 * @peer_fd: file descriptor of the peer connection.
 * @peer_app_ctx: application context of the peer connection.
 *
 * Retrieves #GSignondSecurityContext of the specified peer.
 */
void
gsignond_access_control_manager_security_context_of_peer (
                            GSignondAccessControlManager *self,
                            GSignondSecurityContext *peer_ctx,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx)
{
    GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        security_context_of_peer (self, peer_ctx, peer_fd,
                                  peer_service, peer_app_ctx);
}

/**
 * gsignond_access_control_manager_peer_is_allowed_to_use_identity:
 * @self: object instance.
 * @peer_fd: file descriptor of the peer connection.
 * @peer_app_ctx: application context of the peer connection.
 * @identity_acl: access control list for the identity in question.
 *
 * Checks if specified peer is allowed to access the specified identity.
 *
 * Returns: access is allowed?
 */
gboolean
gsignond_access_control_manager_peer_is_allowed_to_use_identity (
                            GSignondAccessControlManager *self,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx,
                            const GSignondSecurityContextList *identity_acl)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        peer_is_allowed_to_use_identity (self, peer_fd, peer_service,
                                         peer_app_ctx, identity_acl);
}

/**
 * gsignond_access_control_manager_peer_is_owner_of_identity:
 * @self: object instance.
 * @peer_fd: file descriptor of the peer connection.
 * @peer_app_ctx: application context of the peer connection.
 * @identity_owner: security context of the identity owner.
 *
 * Checks if the specified peer is owner of the identity.
 *
 * Returns: is owner?
 */
gboolean
gsignond_access_control_manager_peer_is_owner_of_identity (
                            GSignondAccessControlManager *self,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx,
                            const GSignondSecurityContext *identity_owner)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        peer_is_owner_of_identity (self,  peer_fd, peer_service,
                                   peer_app_ctx, identity_owner);
}

/**
 * gsignond_access_control_manager_acl_is_valid:
 * @self: object instance.
 * @peer_fd: file descriptor of the peer connection.
 * @peer_app_ctx: application context of the peer connection.
 * @identity_acl: access control list for the identity.
 *
 * Checks if the specified peer is allowed to set the specified access
 * control list.
 *
 * Returns: access control list is OK?
 */
gboolean
gsignond_access_control_manager_acl_is_valid (
                            GSignondAccessControlManager *self,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx,
                            const GSignondSecurityContextList *identity_acl)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        acl_is_valid (self, peer_fd, peer_service, peer_app_ctx, identity_acl);
}

/**
 * gsignond_access_control_manager_security_context_of_keychain:
 * @self: object instance.
 *
 * Retrieves security context of the keychain application. Keychain application
 * has a special management access to all stored identities.
 *
 * Returns: security context of the keychain application.
 */
GSignondSecurityContext *
gsignond_access_control_manager_security_context_of_keychain (
                            GSignondAccessControlManager *self)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        security_context_of_keychain (self);
}

