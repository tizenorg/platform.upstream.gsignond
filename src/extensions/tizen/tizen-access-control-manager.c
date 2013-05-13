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

#include <stdlib.h>
#include <sys/smack.h>
#include <glib.h>
#include <gio/gio.h>

#include "gsignond/gsignond-log.h"
#include "tizen-access-control-manager.h"

#define EXTENSION_TIZEN_ACCESS_CONTROL_MANAGER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  EXTENSION_TYPE_TIZEN_ACCESS_CONTROL_MANAGER, \
                                  ExtensionTizenAccessControlManagerPrivate))

#define DBUS_SERVICE_DBUS "org.freedesktop.DBus"
#define DBUS_PATH_DBUS "/org/freedesktop/DBus"
#define DBUS_INTERFACE_DBUS "org.freedesktop.DBus"

static const gchar keychainAppId[] = "gSignond::keychain";

struct _ExtensionTizenAccessControlManagerPrivate
{
};

G_DEFINE_TYPE (ExtensionTizenAccessControlManager,
               extension_tizen_access_control_manager,
               GSIGNOND_TYPE_ACCESS_CONTROL_MANAGER);

static void
extension_tizen_access_control_manager_class_init (
                              ExtensionTizenAccessControlManagerClass *klass)
{
   
    /*GObjectClass *base = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass,
                              sizeof(ExtensionTestAccessControlManagerPrivate));*/

    GSignondAccessControlManagerClass *parent =
        GSIGNOND_ACCESS_CONTROL_MANAGER_CLASS (klass);
                              
    parent->security_context_of_peer = extension_tizen_access_control_manager_security_context_of_peer;
    parent->peer_is_allowed_to_use_identity = extension_tizen_access_control_manager_peer_is_allowed_to_use_identity;
    parent->peer_is_owner_of_identity = extension_tizen_access_control_manager_peer_is_owner_of_identity;
    parent->security_context_of_keychain = extension_tizen_access_control_manager_security_context_of_keychain;
}

static void
extension_tizen_access_control_manager_init (
                                       ExtensionTizenAccessControlManager *self)
{
    /*self->priv = EXTENSION_TEST_ACCESS_CONTROL_MANAGER_GET_PRIVATE (self);*/
}

void
extension_tizen_access_control_manager_security_context_of_peer (
                            GSignondAccessControlManager *self,
                            GSignondSecurityContext *peer_ctx,
                            int peer_fd, const gchar *peer_service,
                            const gchar *peer_app_ctx)
{
    (void) self;

    gsignond_security_context_set_system_context (peer_ctx, "");
    gsignond_security_context_set_application_context (peer_ctx,
                                                       peer_app_ctx);
    if (peer_fd != -1) {
        char *label = NULL;

        smack_new_label_from_socket(peer_fd, &label);
        if (label) {
            gsignond_security_context_set_system_context (peer_ctx,
                                                          label);
            free (label);
        }
    } else if (peer_service != NULL) {
        GError *error = NULL;
        GDBusConnection *connection;
        GDBusProxy *proxy;
        GVariant *response = NULL;
        const gchar *label;

        connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
        if (connection == NULL) {
            WARN ("Failed to open connection to bus: %s", error->message);
            g_error_free (error);
            return;
        }
        
        error = NULL;
        proxy = g_dbus_proxy_new_sync (connection,
                                       G_DBUS_PROXY_FLAGS_NONE, 
                                       NULL,
                                       DBUS_SERVICE_DBUS,
                                       DBUS_PATH_DBUS,
                                       DBUS_INTERFACE_DBUS,
                                       NULL,
                                       &error);
        if (proxy == NULL) {
            WARN ("Error creating proxy: %s", error->message);
            g_error_free (error);
            goto _dbus_connection_exit;
        }

        /* Call getConnectionSmackContext method, wait for reply */

        error = NULL;
        response = g_dbus_proxy_call_sync (proxy,
                                           "GetConnectionSmackContext",  
                                           g_variant_new ("(s)", peer_service),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);
        if (response == NULL) {
            WARN ("Error: %s", error->message);
            g_error_free (error);
            goto _dbus_proxy_exit;
        }
        
        label = g_variant_get_string (response, NULL);
        DBG ("Obtained label from dbus: %s", label);
        if (label)
            gsignond_security_context_set_system_context (peer_ctx,
                                                          label);

        g_variant_unref (response);

_dbus_proxy_exit:
        g_object_unref (proxy);
_dbus_connection_exit:
        g_object_unref (connection);
    } 
}

gboolean
extension_tizen_access_control_manager_peer_is_allowed_to_use_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *identity_owner,
                            const GSignondSecurityContextList *identity_acl)
{
    GSignondSecurityContext* acl_ctx = NULL;
    const gchar *peer_system_ctx = gsignond_security_context_get_system_context(peer_ctx);
    const gchar *owner_system_ctx = gsignond_security_context_get_system_context(identity_owner);
    
    (void) self;
    
    for ( ; identity_acl != NULL; identity_acl = g_list_next (identity_acl)) {
        acl_ctx = (GSignondSecurityContext *) identity_acl->data;
        if (gsignond_security_context_check (acl_ctx, peer_ctx)) {
            // we have a match in acl, now we need to check is smack allows such access
            /* smack_have_access() returns
             * 1 = have access,
             * 0 = no access,
             * -1 = error
             */
            return (smack_have_access (peer_system_ctx, owner_system_ctx, "x") == 1) ? TRUE : FALSE;
        }
    }
    
    return FALSE;
}

gboolean
extension_tizen_access_control_manager_peer_is_owner_of_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *identity_owner)
{
    (void) self;

    return (gsignond_security_context_compare (peer_ctx, identity_owner) == 0);
}

GSignondSecurityContext *
extension_tizen_access_control_manager_security_context_of_keychain (
                                             GSignondAccessControlManager *self)
{
    (void) self;

    return gsignond_security_context_new_from_values (keychainAppId, NULL);
}
