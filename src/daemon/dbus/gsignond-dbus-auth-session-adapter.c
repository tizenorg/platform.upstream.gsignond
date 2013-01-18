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

#include "gsignond/gsignond-log.h"
#include "gsignond-dbus-auth-session-adapter.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,
    PROP_IMPL,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDbusAuthSessionAdapterPrivate
{
    GDBusConnection     *connection;
    GSignondAuthSessionIface *parent;
};

G_DEFINE_TYPE (GSignondDbusAuthSessionAdapter, gsignond_dbus_auth_session_adapter, GSIGNOND_DBUS_TYPE_AUTH_SESSION_SKELETON)


#define GSIGNOND_DBUS_AUTH_SESSION_ADAPTER_GET_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DBUS_AUTH_SESSION_ADAPTER, GSignondDbusAuthSessionAdapterPrivate)

static void _handle_query_available_mechanisms (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, const gchar **, gpointer);
static void _handle_process (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, const GVariant *, const gchar *, gpointer);
static void _handle_cancel (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, gpointer);
static void _handle_set_id (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, guint32, gpointer);

static void
gsignond_dbus_auth_session_adapter_set_property (GObject *object,
        guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            gpointer iface = g_value_peek_pointer (value);
            if (iface) {
                if (self->priv->parent) g_object_unref (self->priv->parent);
                self->priv->parent = GSIGNOND_AUTH_SESSION_IFACE (g_object_ref (iface));
            }
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_auth_session_adapter_get_property (GObject *object,
        guint property_id,
        GValue *value, 
        GParamSpec *pspec)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            g_value_set_instance (value, g_object_ref (self->priv->parent));
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_auth_session_adapter_dispose (GObject *object)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    if (self->priv->parent) {
        g_object_unref (self->priv->parent);
        self->priv->parent = NULL;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }
}

static void
gsignond_dbus_auth_session_adapter_finalize (GObject *object)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (object));

    G_OBJECT_CLASS (gsignond_dbus_auth_session_adapter_parent_class)->finalize (object);
}

static void
gsignond_dbus_auth_session_adapter_class_init (GSignondDbusAuthSessionAdapterClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusAuthSessionAdapterPrivate));

    object_class->get_property = gsignond_dbus_auth_session_adapter_get_property;
    object_class->set_property = gsignond_dbus_auth_session_adapter_set_property;
    object_class->dispose = gsignond_dbus_auth_session_adapter_dispose;
    object_class->finalize = gsignond_dbus_auth_session_adapter_finalize;

    properties[PROP_IMPL] = g_param_spec_object ("auth-session-impl",
                                                  "Auth session impl",
                                                  "AuthSessionIface implementation object",
                                                  GSIGNOND_TYPE_AUTH_SESSION_IFACE,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_auth_session_adapter_init (GSignondDbusAuthSessionAdapter *self)
{
    static guint32 object_counter;
    char *object_path = NULL;
    GError *err = 0;

    self->priv = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->parent = 0;


    self->priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        ERR ("Error getting session bus :%s", err->message);
        g_error_free (err);
        return;
    }

    object_path = g_strdup_printf ("%s/AuthSession_%d", GSIGNOND_DAEMON_OBJECTPATH, object_counter++);
    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                           self->priv->connection,
                                           object_path,
                                           &err)) {
        ERR ("failed to register object: %s", err->message);
        g_error_free (err);
        g_free (object_path);
        return ;
    }

    g_free (object_path);

    g_signal_connect (self, "handle-query-available-mechanisms", G_CALLBACK (_handle_query_available_mechanisms), NULL);
    g_signal_connect (self, "handle-process", G_CALLBACK(_handle_process), NULL);
    g_signal_connect (self, "handle-cancel", G_CALLBACK(_handle_cancel), NULL);
    g_signal_connect (self, "handle-set-id", G_CALLBACK(_handle_set_id), NULL);
}

static void
_handle_query_available_mechanisms (GSignondDbusAuthSessionAdapter *self,
                                    GDBusMethodInvocation *invocation,
                                    const gchar **wanted_mechanisms,
                                    gpointer user_data)
{
    GSignondDbusAuthSession *iface = GSIGNOND_DBUS_AUTH_SESSION (self);
    gchar **mechanisms = gsignond_auth_session_iface_query_available_mechanisms (self->priv->parent, wanted_mechanisms);

    if (mechanisms) {
        gsignond_dbus_auth_session_complete_query_available_mechanisms (iface, invocation, (const gchar * const *)mechanisms);
        g_strfreev (mechanisms);
    }
    else {
        /* 
         * TODO: Prepare error
         * GError *err = g_error_new ();
         * g_dbus_method_invocation_return_gerror (invocation, err);
         * g_error_free (err);
         */
    }
}

typedef struct {
    GSignondDbusAuthSessionAdapter *adapter;
    GDBusMethodInvocation *invocation;
    gpointer user_data;
} _AuthSessionDbusInfo;

static void
_on_process_result (GSignondAuthSessionIface *auth_session, const GVariant *result, gpointer user_data)
{
    _AuthSessionDbusInfo *info = (_AuthSessionDbusInfo *) info;
    GSignondDbusAuthSession *iface = GSIGNOND_DBUS_AUTH_SESSION (info->adapter);

    gsignond_dbus_auth_session_complete_process (iface, info->invocation, result);

    g_free (info);
}

static void
_on_process_error (GSignondAuthSessionIface *auth_session, guint error, gpointer user_data)
{
    /*
     * TODO: Prepare error
     * GError *err = g_error_new ();
     * g_dbus_method_invocation_return_gerror (invocation, err);
     * g_error_free (err);
     */
}

static void
_handle_process (GSignondDbusAuthSessionAdapter *self,
                 GDBusMethodInvocation *invocation,
                 const GVariant *session_data,
                 const gchar *mechanisms,
                 gpointer user_data)
{
    _AuthSessionDbusInfo *info = 0;

    gsignond_auth_session_iface_process (self->priv->parent, session_data, mechanisms);

    info = g_new0 (_AuthSessionDbusInfo, 1);
    info->adapter = self;
    info->invocation = invocation;

    g_signal_connect (self->priv->parent, "process-error", G_CALLBACK(_on_process_error), self);
    g_signal_connect (self->priv->parent, "process-result", G_CALLBACK (_on_process_result), self);
}

static void
_handle_cancel (GSignondDbusAuthSessionAdapter *self,
                GDBusMethodInvocation *invocation,
                gpointer user_data)
{
    GSignondDbusAuthSession *iface = GSIGNOND_DBUS_AUTH_SESSION (self);
    
    gsignond_auth_session_iface_cancel (self->priv->parent);

    gsignond_dbus_auth_session_complete_cancel (iface, invocation);
}

static void
_handle_set_id (GSignondDbusAuthSessionAdapter   *self,
                GDBusMethodInvocation *invocation,
                guint32 id,
                gpointer user_data)
{
    GSignondDbusAuthSession *iface = GSIGNOND_DBUS_AUTH_SESSION (self);

    gsignond_auth_session_iface_set_id (self->priv->parent, id);

    gsignond_dbus_auth_session_complete_set_id (iface, invocation);
}

GSignondDbusAuthSessionAdapter *
gsignond_dbus_auth_session_adapter_new (GSignondAuthSessionIface *impl)
{
    return g_object_new (GSIGNOND_TYPE_DBUS_AUTH_SESSION_ADAPTER, "auth-session-impl", impl, NULL);
}
