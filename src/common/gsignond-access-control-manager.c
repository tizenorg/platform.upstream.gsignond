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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <gio/gio.h>

#include "config.h"

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-access-control-manager.h"

/**
 * SECTION:gsignond-access-control-manager
 * @short_description: an object that performs access control checks
 * @include: gsignond/gsignond-access-control-manager.h
 *
 * #GSignondAccessControlManager performs access control checks using
 * available system services. gSSO can be configured to use a custom extension
 * that provides a subclassed implementation of #GSignondAccessControlManager
 * (see #GSignondExtension), otherwise a default implementation is used.
 */
/**
 * GSignondAccessControlManager:
 *
 * Opaque #GSignondAccessControlManager data structure.
 */

#define GSIGNOND_ACCESS_CONTROL_MANAGER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER, \
                                  GSignondAccessControlManagerPrivate))

#define DBUS_SERVICE_DBUS "org.freedesktop.DBus"
#define DBUS_PATH_DBUS "/org/freedesktop/DBus"
#define DBUS_INTERFACE_DBUS "org.freedesktop.DBus"
#ifndef GSIGNOND_BUS_TYPE
#   define GSIGNOND_BUS_TYPE G_BUS_TYPE_SESSION
#endif

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

    G_OBJECT_CLASS (gsignond_access_control_manager_parent_class)->dispose (object);
}

static void
_security_context_of_peer (GSignondAccessControlManager *self,
                           GSignondSecurityContext *peer_ctx,
                           int peer_fd, const gchar *peer_service,
                           const gchar *peer_app_ctx)
{
    pid_t remote_pid = 0;
    gchar *procfname;
    char *peerpath;
    ssize_t res;

    (void) self;

    gsignond_security_context_set_system_context (peer_ctx, "");
    gsignond_security_context_set_application_context (peer_ctx,
                                                       peer_app_ctx);

    if (peer_fd >= 0) {
        struct ucred peer_cred;
        socklen_t cred_size = sizeof(peer_cred);

        if (getsockopt (peer_fd, SOL_SOCKET, SO_PEERCRED,
                        &peer_cred, &cred_size) != 0) {
            WARN ("getsockopt() for SO_PEERCRED failed");
            return;
        }
        DBG ("remote peer pid=%d uid=%d gid=%d",
             peer_cred.pid, peer_cred.uid, peer_cred.gid);
        remote_pid = peer_cred.pid;
    } else if (peer_service) {
        GError *error = NULL;
        GDBusConnection *connection;
        GVariant *response = NULL;
        guint32 upid;

        connection = g_bus_get_sync (GSIGNOND_BUS_TYPE, NULL, &error);
        if (!connection) {
            WARN ("failed to open connection to session bus: %s",
                  error->message);
            g_error_free (error);
            return;
        }

        error = NULL;
        response = g_dbus_connection_call_sync (connection,
                                                DBUS_SERVICE_DBUS,
                                                DBUS_PATH_DBUS,
                                                DBUS_INTERFACE_DBUS,
                                                "GetConnectionUnixProcessID",
                                                g_variant_new ("(s)", peer_service),
                                                ((const GVariantType *) "(u)"),
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                &error);

        g_object_unref (connection);

        if (!response) {
            WARN ("request for peer pid failed: %s",
                  error->message);
            g_error_free (error);
            return;
        }
        
        g_variant_get (response, "(u)", &upid);
        DBG ("remote peer service=%s pid=%u", peer_service, upid);
        remote_pid = (pid_t) upid;

        g_variant_unref (response);
    } else return;

    if (!remote_pid)
        return;

    procfname = g_strdup_printf ("/proc/%d/exe", remote_pid);
    peerpath = g_malloc0 (PATH_MAX + 1);
    res = readlink (procfname, peerpath, PATH_MAX);
    g_free (procfname);
    if (res <= 0) {
        WARN ("failed to follow link for pid %d", remote_pid);
        g_free (peerpath);
        return;
    }

    DBG ("identity of pid %d is [%s:%s]", remote_pid, peerpath, peer_app_ctx);
    gsignond_security_context_set_system_context (peer_ctx, peerpath);

    g_free (peerpath);
}

static gboolean
_peer_is_allowed_to_use_identity (GSignondAccessControlManager *self,
                                const GSignondSecurityContext *peer_ctx,
                                const GSignondSecurityContext *owner_ctx,
                                const GSignondSecurityContextList *identity_acl)
{
    GSignondSecurityContext *acl_ctx;

    (void) self;
    (void) owner_ctx;

    for ( ; identity_acl != NULL; identity_acl = g_list_next (identity_acl)) {
        acl_ctx = (GSignondSecurityContext *) identity_acl->data;
        DBG ("ACL check [%p=(%s:%s)] vs [%p=(%s:%s)]",
             acl_ctx,
             gsignond_security_context_get_system_context (acl_ctx),
             gsignond_security_context_get_application_context (acl_ctx),
             peer_ctx,
             gsignond_security_context_get_system_context (peer_ctx),
             gsignond_security_context_get_application_context (peer_ctx));
        if (gsignond_security_context_check (acl_ctx, peer_ctx)) {
            DBG (" - ACL check passed");
            return TRUE;
        }
    }
    DBG (" - ACL check failed");
    return FALSE;
}

static gboolean
_peer_is_owner_of_identity (GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx)
{
    (void) self;

    DBG ("Owner check [%p=(%s:%s)] vs [%p=(%s:%s)]",
         owner_ctx,
         gsignond_security_context_get_system_context (owner_ctx),
         gsignond_security_context_get_application_context (owner_ctx),
         peer_ctx,
         gsignond_security_context_get_system_context (peer_ctx),
         gsignond_security_context_get_application_context (peer_ctx));
    return gsignond_security_context_check (owner_ctx, peer_ctx);
}

static gboolean
_acl_is_valid (GSignondAccessControlManager *self,
               const GSignondSecurityContext *peer_ctx,
               const GSignondSecurityContextList *identity_acl)
{
    (void) self;
    (void) peer_ctx;
    (void) identity_acl;

    return TRUE;
}

GSignondSecurityContext *
_security_context_of_keychain (GSignondAccessControlManager *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    const gchar *keychain_sysctx;

    keychain_sysctx = gsignond_config_get_string (
                                      self->config,
                                      GSIGNOND_CONFIG_GENERAL_KEYCHAIN_SYSCTX);
    if (!keychain_sysctx)
#       ifdef KEYCHAIN_SYSCTX
        keychain_sysctx = KEYCHAIN_SYSCTX;
#       else
        keychain_sysctx = "";
#       endif
#   ifdef ENABLE_DEBUG
    const gchar *keychain_env = g_getenv ("SSO_KEYCHAIN_SYSCTX");
    if (keychain_env)
        keychain_sysctx = keychain_env;
#   endif

    return gsignond_security_context_new_from_values (keychain_sysctx, NULL);
}

/**
 * GSignondAccessControlManagerClass:
 * @parent_class: parent class.
 * @security_context_of_peer: an implementation of gsignond_access_control_manager_security_context_of_peer()
 * @peer_is_allowed_to_use_identity: an implementation of gsignond_access_control_manager_peer_is_allowed_to_use_identity()
 * @peer_is_owner_of_identity: an implementation of gsignond_access_control_manager_peer_is_owner_of_identity()
 * @acl_is_valid: an implementation of gsignond_access_control_manager_acl_is_valid()
 * @security_context_of_keychain: an implementation of gsignond_access_control_manager_security_context_of_keychain()
 * 
 * #GSignondAccessControlManagerClass class containing pointers to class methods.
 */
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
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);
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
 * @peer_fd: file descriptor of the peer connection if using peer-to-peer dbus, -1 otherwise.
 * @peer_service: g_dbus_method_invocation_get_sender() of the peer connection, if not using peer-to-peer dbus, NULL otherwise
 * @peer_app_ctx: application context of the peer connection.
 *
 * Retrieves and sets #GSignondSecurityContext of the specified peer.
 * 
 * The default implementation sets the app context as it was passed, and sets 
 * the system context to the binary path of the process that is determined from
 * @peer_fd and @peer_service parameters.
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
 * @peer_ctx: security context of the peer connection.
 * @owner_ctx: security context of the identity owner.
 * @identity_acl: access control list for the identity in question. Includes the @owner_ctx as well.
 *
 * Checks if specified peer is allowed to access the specified identity.
 * 
 * The default implementation goes over items in @identity_acl, using 
 * gsignond_security_context_check() to check them against @peer_ctx.
 *
 * Returns: access is allowed?
 */
gboolean
gsignond_access_control_manager_peer_is_allowed_to_use_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx,
                            const GSignondSecurityContextList *identity_acl)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        peer_is_allowed_to_use_identity (self, peer_ctx, owner_ctx, identity_acl);
}

/**
 * gsignond_access_control_manager_peer_is_owner_of_identity:
 * @self: object instance.
 * @peer_ctx: security context of the peer connection.
 * @owner_ctx: security context of the identity owner.
 *
 * Checks if the peer specified in @peer_ctx is the owner of the identity.
 * 
 * The default implementation is using gsignond_security_context_check() 
 * to check @peer_ctx against @owner_ctx directly.
 *
 * Returns: is owner?
 */
gboolean
gsignond_access_control_manager_peer_is_owner_of_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *owner_ctx)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        peer_is_owner_of_identity (self, peer_ctx, owner_ctx);
}

/**
 * gsignond_access_control_manager_acl_is_valid:
 * @self: object instance.
 * @peer_ctx: security context of the peer connection.
 * @identity_acl: access control list for the identity.
 *
 * Checks if the specified peer is allowed to set the specified access
 * control list. gsignond_access_control_manager_peer_is_owner_of_identity()
 * is used before calling this method to verify identity ownership.
 * 
 * The default implementation always returns TRUE.
 *
 * Returns: access control list is OK?
 */
gboolean
gsignond_access_control_manager_acl_is_valid (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContextList *identity_acl)
{
    return GSIGNOND_ACCESS_CONTROL_MANAGER_GET_CLASS (self)->
        acl_is_valid (self, peer_ctx, identity_acl);
}

/**
 * gsignond_access_control_manager_security_context_of_keychain:
 * @self: object instance.
 *
 * Retrieves security context of the keychain application. Keychain application
 * has a special management access to all stored identities and is able to
 * perform deletion of all identities from storage.
 * 
 * The default implementation returns a context either set in #GSignondConfig, 
 * or if not set, a value specified through a configure --enable-keychain
 * option (see
 * <link linkend="gsignond-building">Building gsignond</link>), or if that is not
 * set either then an empty string "" is returned. 
 * 
 * If gSSO was compiled
 * with --enable-debug and SSO_KEYCHAIN_SYSCTX environment variable is set, then
 * the value of that variable is used to set the returned system context instead.
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

