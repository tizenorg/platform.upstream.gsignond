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

#include <sys/smack.h>
#include <glib.h>
#include <gio/gio.h>

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
                              
    klass->security_context_of_peer = extension_tizen_access_control_manager_security_context_of_peer;
    klass->peer_is_allowed_to_use_identity = extension_tizen_access_control_manager_peer_is_allowed_to_use_identity;
    klass->peer_is_owner_of_identity = extension_tizen_access_control_manager_peer_is_owner_of_identity;
    klass->security_context_of_keychain = extension_tizen_access_control_manager_security_context_of_keychain;
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
    char* label = NULL; // fetched Smack label of a peer
    GDBusConnection *connection;
    GError *error = NULL;
    GDBusProxy *proxy;
    GVariant * response = NULL;
    
    (void) self;
    
    if (peer_fd != -1)
    {
        smack_new_label_from_socket(peer_fd, &label);
        if (label) 
        {
            peer_ctx = gsignond_security_context_new_from_values ((const gchar*)label, peer_app_ctx);
        }
        
        return;
        
    } else if (peer_service != NULL) {

        connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
        if (connection == NULL)
        {
            g_printerr ("Failed to open connection to bus: %s\n",
                        error->message);
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
        if (proxy == NULL)
        {
            g_printerr ("Error creating proxy: %s\n", error->message);
            g_error_free (error);
            return;
        }

        /* Call getConnectionSmackContext method, wait for reply */

        error = NULL;
        response = g_dbus_proxy_call_sync (proxy, "GetConnectionSmackContext",  
                                           g_variant_new ("s", peer_service),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);
        if (response == NULL)
        {
            g_printerr ("Error: %s\n", error->message);
            g_object_unref (proxy);
            g_error_free (error);
            return;
        }
        
        g_variant_get (response, "s", &label);
        g_print ("Obtained label from dbus: %s\n", label);
        
        if (label)
            peer_ctx = gsignond_security_context_new_from_values ((const gchar*)label, peer_app_ctx);
            
        g_object_unref (proxy);
        return;
    } 

}

gboolean
extension_tizen_access_control_manager_peer_is_allowed_to_use_identity (
                            GSignondAccessControlManager *self,
                            const GSignondSecurityContext *peer_ctx,
                            const GSignondSecurityContext *identity_owner,
                            const GSignondSecurityContextList *identity_acl)
{
    GSignondSecurityContext* iterator = NULL;
    const gchar *peer_system_ctx = gsignond_security_context_get_system_context(peer_ctx);
    const gchar *owner_system_ctx = gsignond_security_context_get_system_context(identity_owner);
    
    (void) self;
    
    for ( ; identity_acl != NULL; identity_acl = g_list_next (identity_acl))
    {
        iterator = (GSignondSecurityContext *) identity_acl->data;
        if (gsignond_security_context_match(iterator, peer_ctx))
        { // we have a match in acl, now we need to check is smack allows such access 
            if (smack_have_access(peer_system_ctx, owner_system_ctx, "x"))
                return TRUE;
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

    return gsignond_security_context_compare(peer_ctx, identity_owner);
}

GSignondSecurityContext *
extension_tizen_access_control_manager_security_context_of_keychain (
                                             GSignondAccessControlManager *self)
{
    (void) self;

    return gsignond_security_context_new_from_values (keychainAppId, NULL);
}
