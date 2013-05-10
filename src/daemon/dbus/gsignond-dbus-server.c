/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Amarnath Valluri <amarnath.valluri@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <config.h>
#include "gsignond-dbus-server.h"
#include "gsignond-dbus-auth-service-adapter.h"
#include "gsignond-dbus.h"
#include "gsignond/gsignond-log.h"
#include <daemon/gsignond-daemon.h>

enum
{
    PROP_0,

    PROP_ADDRESS,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDbusServerPrivate
{
    GSignondDaemon *daemon;
    GHashTable *auth_services;
#ifdef USE_P2P
    GDBusServer *bus_server;
    gchar *address;
#else
    guint name_owner_id;
#endif
};

G_DEFINE_TYPE (GSignondDbusServer, gsignond_dbus_server, G_TYPE_OBJECT)


#define GSIGNOND_DBUS_SERVER_GET_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DBUS_SERVER, GSignondDbusServerPrivate)

static void _on_connection_closed (GDBusConnection *connection,
                       gboolean         remote_peer_vanished,
                       GError          *error,
                       gpointer         user_data);

static void
_set_property (GObject *object,
        guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    GSignondDbusServer *self = GSIGNOND_DBUS_SERVER (object);

    switch (property_id) {
        case PROP_ADDRESS: {
#ifdef USE_P2P
            self->priv->address = g_value_dup_string (value);
#else
            (void)self;
#endif
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_get_property (GObject *object,
        guint property_id,
        GValue *value, 
        GParamSpec *pspec)
{
    GSignondDbusServer *self = GSIGNOND_DBUS_SERVER (object);

    switch (property_id) {
        case PROP_ADDRESS: {
#ifdef USE_P2P
            g_value_set_string (value, g_dbus_server_get_client_address (
                    self->priv->bus_server));
#else
            (void) self;
            g_value_set_string (value, NULL);
#endif
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_clear_connection (gpointer connection, gpointer value, gpointer userdata)
{
    (void) value;
    g_signal_handlers_disconnect_by_func (connection, _on_connection_closed, userdata);
}

static void
_dispose (GObject *object)
{
    GSignondDbusServer *self = GSIGNOND_DBUS_SERVER (object);

    if (self->priv->auth_services) {
        g_hash_table_foreach (self->priv->auth_services, _clear_connection, self);
        g_hash_table_unref (self->priv->auth_services);
        self->priv->auth_services = NULL;
    }
#ifdef USE_P2P
    if (self->priv->bus_server) {
        if (g_dbus_server_is_active (self->priv->bus_server))
            g_dbus_server_stop (self->priv->bus_server);
        g_object_unref (self->priv->bus_server);
        self->priv->bus_server = NULL;
    }
#else 
    if (self->priv->name_owner_id) {
        g_bus_unown_name (self->priv->name_owner_id);
        self->priv->name_owner_id = 0;
    }
#endif

    if (self->priv->daemon) {
        g_object_unref (self->priv->daemon);
        self->priv->daemon = NULL;
    }
 
    G_OBJECT_CLASS (gsignond_dbus_server_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
#ifdef USE_P2P
    GSignondDbusServer *self = GSIGNOND_DBUS_SERVER (object);
    if (self->priv->address) {
        g_free (self->priv->address);
        self->priv->address = NULL;
    }
#endif
    G_OBJECT_CLASS (gsignond_dbus_server_parent_class)->finalize (object);
}

static void
gsignond_dbus_server_class_init (GSignondDbusServerClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusServerPrivate));

    object_class->get_property = _get_property;
    object_class->set_property = _set_property;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

    properties[PROP_ADDRESS] = g_param_spec_string ("address",
                                                    "server address",
                                                    "Server socket address",
                                                    NULL,
                                                    G_PARAM_READWRITE | 
                                                    G_PARAM_CONSTRUCT_ONLY | 
                                                    G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_server_init (GSignondDbusServer *self)
{
    self->priv = GSIGNOND_DBUS_SERVER_GET_PRIV(self);
#ifdef USE_P2P
    self->priv->bus_server = NULL;
    self->priv->address = NULL;
#else
    self->priv->name_owner_id = 0;
#endif
    self->priv->daemon = gsignond_daemon_new ();
    self->priv->auth_services = g_hash_table_new_full (
        g_direct_hash, g_direct_equal, NULL, g_object_unref);
}

static void
_on_connection_closed (GDBusConnection *connection,
                       gboolean         remote_peer_vanished,
                       GError          *error,
                       gpointer         user_data)
{
    GSignondDbusServer *server = GSIGNOND_DBUS_SERVER (user_data);

    g_signal_handlers_disconnect_by_func (connection, _on_connection_closed, user_data);
    DBG("dbus connection(%p) closed (peer vanished : %d)", connection, remote_peer_vanished);
    if (error) {
       INFO("...reason : %s", error->message);
    }
    g_hash_table_remove (server->priv->auth_services, connection);
}

void
gsignond_dbus_server_start_auth_service (GSignondDbusServer *server, GDBusConnection *connection)
{
    GSignondDbusAuthServiceAdapter *auth_service = NULL;

    DBG("Starting authentication service on connection %p", connection);

    auth_service = gsignond_dbus_auth_service_adapter_new_with_connection (
        g_object_ref (connection), g_object_ref (server->priv->daemon));

    g_hash_table_insert (server->priv->auth_services, connection, auth_service);

    g_signal_connect (connection, "closed", G_CALLBACK(_on_connection_closed), server);
}

const gchar *
gsignond_dbus_server_get_address (GSignondDbusServer *server)
{
    g_return_val_if_fail (server || GSIGNOND_IS_DBUS_SERVER (server), NULL);
#ifdef USE_P2P
    return g_dbus_server_get_client_address (server->priv->bus_server);
#else
    return NULL;
#endif
}

#ifdef USE_P2P
static gboolean
_on_client_request (GDBusServer *dbus_server, GDBusConnection *connection, gpointer userdata)
{
    GSignondDbusServer *server = GSIGNOND_DBUS_SERVER(userdata);

    if (!server) {
        ERR ("memory currpotion");
        return TRUE;
    }

    gsignond_dbus_server_start_auth_service (server, connection);

    return TRUE;
}

GSignondDbusServer * gsignond_dbus_server_new_with_address (const gchar *address)
{
    GError *err = NULL;
    gchar *guid = 0;
    GSignondDbusServer *server = GSIGNOND_DBUS_SERVER (
        g_object_new (GSIGNOND_TYPE_DBUS_SERVER, "address", address, NULL));

    if (!server) return NULL;

    guid = g_dbus_generate_guid ();

    server->priv->bus_server = g_dbus_server_new_sync (server->priv->address,
            G_DBUS_SERVER_FLAGS_NONE, guid, NULL, NULL, &err);

    g_free (guid);

    if (!server->priv->bus_server) {
        ERR ("failed to start server at address '%s':%s", server->priv->address,
                 err->message);
        g_error_free (err);
        
        g_object_unref (server);
     
        return NULL;
    }

    g_signal_connect (server->priv->bus_server, "new-connection", G_CALLBACK(_on_client_request), server);

    g_dbus_server_start (server->priv->bus_server);

    return server;
}

GSignondDbusServer * gsignond_dbus_server_new () {
    return gsignond_dbus_server_new_with_address (GSIGNOND_DBUS_ADDRESS);
}
#else

static void
_on_bus_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    INFO ("bus aquired on connection '%p'", connection);
}

static void
_on_name_lost (GDBusConnection *connection,
               const gchar     *name,
               gpointer         user_data)
{
    INFO ("Lost (or failed to acquire) the name '%s' on the on bus connection '%p'", name, connection);
    if (user_data) g_object_unref (G_OBJECT (user_data));
}

static void
_on_name_acquired (GDBusConnection *connection,
                   const gchar     *name,
                   gpointer         user_data)
{
    GSignondDbusServer *server = GSIGNOND_DBUS_SERVER (user_data);
    INFO ("Acquired the name %s on connection '%p'", name, connection);
    gsignond_dbus_server_start_auth_service (server, connection);
}

GSignondDbusServer * gsignond_dbus_server_new () {
    GSignondDbusServer *server = GSIGNOND_DBUS_SERVER (
            g_object_new (GSIGNOND_TYPE_DBUS_SERVER, NULL));

    server->priv->name_owner_id = g_bus_own_name (GSIGNOND_BUS_TYPE,
            GSIGNOND_SERVICE, 
            G_BUS_NAME_OWNER_FLAGS_REPLACE,
            _on_bus_acquired,
            _on_name_acquired,
            _on_name_lost,
            server, NULL);

    return server;
}
#endif
