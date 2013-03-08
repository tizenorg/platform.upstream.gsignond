/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
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

#include "gsignond-plugin-proxy.h"
#include <gsignond/gsignond-plugin-loader.h>
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-log.h>

#define GSIGNOND_PLUGIN_PROXY_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                 GSIGNOND_TYPE_PLUGIN_PROXY, \
                                 GSignondPluginProxyPrivate)

enum
{
    PROP_0,
    
    PROP_TYPE,
    PROP_MECHANISMS,
    PROP_CONFIG,
    
    N_PROPERTIES
};

struct _GSignondPluginProxyPrivate
{
    GSignondConfig *config;
    gchar* plugin_type;
    GSignondPlugin* plugin;
    GQueue* session_queue;
    GSignondAuthSessionIface* active_session;
    gboolean expecting_request;
};

typedef struct {
    GSignondAuthSessionIface* auth_session;
    GSignondSessionData* session_data;
    gchar* mechanism;
} GSignondProcessData;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (GSignondPluginProxy, gsignond_plugin_proxy, G_TYPE_OBJECT);

static void gsignond_plugin_proxy_response_final_callback(GSignondPlugin* plugin, 
                                                  GSignondSessionData* result,
                                                  gpointer user_data);
static void gsignond_plugin_proxy_response_callback(GSignondPlugin* plugin, 
                                                  GSignondSessionData* result,
                                                  gpointer user_data);
static void gsignond_plugin_proxy_store_callback(GSignondPlugin* plugin, 
                                                  GSignondSessionData* result,
                                                  gpointer user_data);
static void gsignond_plugin_proxy_refreshed_callback(GSignondPlugin* plugin, 
                                                  GSignondSessionData* result,
                                                  gpointer user_data);
static void gsignond_plugin_proxy_user_action_required_callback(
                                          GSignondPlugin* plugin, 
                                          GSignondSessionData* ui_request, 
                                          gpointer user_data);
static void gsignond_plugin_proxy_error_callback(GSignondPlugin* plugin, 
                                                 GError* error,
                                                 gpointer user_data);
static void gsignond_plugin_proxy_status_changed_callback(GSignondPlugin* plugin, 
                                                  GSignondPluginState status,
                                                  const gchar* message,
                                                  gpointer user_data);

static GSignondProcessData* 
gsignond_process_data_new (GSignondAuthSessionIface* auth_session,
                           GSignondSessionData *session_data,
                           const gchar* mechanism) 
{
    GSignondProcessData* data = g_slice_new0 (GSignondProcessData);
    g_object_ref (auth_session);
    data->auth_session = auth_session;
    data->session_data = gsignond_dictionary_copy (session_data);
    data->mechanism = g_strdup (mechanism);
    return data;
}

static void gsignond_process_data_free (GSignondProcessData* data)
{
    g_object_unref (data->auth_session);
    gsignond_dictionary_free (data->session_data);
    g_free (data->mechanism);
    g_slice_free (GSignondProcessData, data);
}

static GObject *
gsignond_plugin_proxy_constructor (GType                  gtype,
                                   guint                  n_properties,
                                   GObjectConstructParam *properties)
{
    GObject *obj;

    /* Always chain up to the parent constructor */
    obj = G_OBJECT_CLASS (gsignond_plugin_proxy_parent_class)->
        constructor (gtype, n_properties, properties);
  
    /* update the object state depending on constructor properties */
    GSignondPluginProxy* self = GSIGNOND_PLUGIN_PROXY (obj);
    GSignondPluginProxyPrivate *priv = self->priv;
    priv->plugin = gsignond_load_plugin (priv->config,
                                         priv->plugin_type);

    if (priv->plugin != NULL) {
        gchar *type;
        g_object_get (priv->plugin, "type", &type, NULL);
        if (g_strcmp0 (type, priv->plugin_type) != 0) {
            g_free (priv->plugin_type);
            priv->plugin_type = NULL;
        }
        g_free (type);

        g_signal_connect(priv->plugin, "response", G_CALLBACK(
            gsignond_plugin_proxy_response_callback), self);
        g_signal_connect(priv->plugin, "response-final", G_CALLBACK(
            gsignond_plugin_proxy_response_final_callback), self);
        g_signal_connect(priv->plugin, "user-action-required", G_CALLBACK(
            gsignond_plugin_proxy_user_action_required_callback), self);
        g_signal_connect(priv->plugin, "error", G_CALLBACK(
            gsignond_plugin_proxy_error_callback), self);
        g_signal_connect(priv->plugin, "store", G_CALLBACK(
            gsignond_plugin_proxy_store_callback), self);
        g_signal_connect(priv->plugin, "refreshed", G_CALLBACK(
            gsignond_plugin_proxy_refreshed_callback), self);
        g_signal_connect(priv->plugin, "status-changed", G_CALLBACK(
            gsignond_plugin_proxy_status_changed_callback), self);
    }
    return obj;
}

static void
gsignond_plugin_proxy_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (object);
    GSignondPluginProxyPrivate *priv = self->priv;

    switch (property_id)
    {
        case PROP_TYPE:
            g_free (self->priv->plugin_type);
            priv->plugin_type = g_value_dup_string (value);
            break;
        case PROP_CONFIG:
            g_assert (self->priv->config == NULL);
            priv->config = g_value_dup_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gsignond_plugin_proxy_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (object);
    GSignondPluginProxyPrivate *priv = self->priv;
    
    switch (prop_id)
    {
        case PROP_TYPE:
            g_value_set_string (value, priv->plugin_type);
            break;
        case PROP_MECHANISMS:
            g_object_get_property (G_OBJECT(priv->plugin),
                                   "mechanisms", value);
            break;
        case PROP_CONFIG:
            g_value_set_object (value, priv->config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gsignond_plugin_proxy_dispose (GObject *gobject)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (gobject);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session) {
        g_object_unref (priv->active_session);
        priv->active_session = NULL;
    }
    if (priv->plugin) {
        g_object_unref (priv->plugin);
        priv->plugin = NULL;
    }
    if (priv->config) {
        g_object_unref (priv->config);
        priv->config = NULL;
    }

  /* Chain up to the parent class */
  G_OBJECT_CLASS (gsignond_plugin_proxy_parent_class)->dispose (gobject);
}

static void
gsignond_plugin_proxy_finalize (GObject *gobject)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (gobject);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->plugin_type) {
        g_free (priv->plugin_type);
        priv->plugin_type = NULL;
    }
    if (priv->session_queue)
    {
        g_queue_free_full (priv->session_queue,
                           (GDestroyNotify) gsignond_process_data_free);
        priv->session_queue = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_plugin_proxy_parent_class)->finalize (gobject);
}


static void
gsignond_plugin_proxy_class_init (GSignondPluginProxyClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (gobject_class,
                              sizeof (GSignondPluginProxyPrivate)); 

    gobject_class->constructor = gsignond_plugin_proxy_constructor;
    gobject_class->set_property = gsignond_plugin_proxy_set_property;
    gobject_class->get_property = gsignond_plugin_proxy_get_property;
    gobject_class->dispose = gsignond_plugin_proxy_dispose;
    gobject_class->finalize = gsignond_plugin_proxy_finalize;

    obj_properties[PROP_TYPE] =
    g_param_spec_string ("type",
                         "Plugin type",
                         "Set the plugin type for the proxy",
                         "" /* default value */,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);
    
    obj_properties[PROP_MECHANISMS] = g_param_spec_boxed ("mechanisms", 
                            "Mechanisms", 
                            "List of plugin mechanisms", 
                            G_TYPE_STRV, G_PARAM_READABLE);


    obj_properties[PROP_CONFIG] = g_param_spec_object ("config",
                                                   "config",
                                                   "Configuration object",
                                                   GSIGNOND_TYPE_CONFIG,
                                                   G_PARAM_CONSTRUCT_ONLY |
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);
    

    g_object_class_install_properties (gobject_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

static void
gsignond_plugin_proxy_init (GSignondPluginProxy *self)
{
    GSignondPluginProxyPrivate *priv = GSIGNOND_PLUGIN_PROXY_PRIV (self);
    self->priv = priv;

    priv->config = NULL;
    priv->plugin_type = NULL;
    priv->plugin = NULL;
    priv->session_queue = g_queue_new ();
    priv->active_session = NULL;
    priv->expecting_request = FALSE;
}

static const gchar *
gsignond_plugin_proxy_get_plugin_type (GSignondPluginProxy *self)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));

    return self->priv->plugin_type;
}

GSignondPluginProxy* 
gsignond_plugin_proxy_new (GSignondConfig *config, const gchar *plugin_type)
{
    GSignondPluginProxy* proxy = g_object_new (GSIGNOND_TYPE_PLUGIN_PROXY,
                                               "config", config,
                                               "type", plugin_type,
                                               NULL);
    if (g_strcmp0 (plugin_type,
                   gsignond_plugin_proxy_get_plugin_type (proxy)) == 0)
        return proxy;

    g_object_unref (proxy);
    return NULL;
}

static void
gsignond_plugin_proxy_process_queue (GSignondPluginProxy *self)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));

    GSignondPluginProxyPrivate *priv = self->priv;
    GSignondProcessData* next_data = g_queue_pop_head (priv->session_queue);
    if (next_data) {
        priv->expecting_request = FALSE;
        priv->active_session = next_data->auth_session;
        g_object_ref (priv->active_session);
        gsignond_auth_session_iface_notify_state_changed (
                                            priv->active_session,
                                            GSIGNOND_PLUGIN_STATE_STARTED,
                                            "The request is being processed.");
        gsignond_plugin_request_initial (priv->plugin,
                                         next_data->session_data, 
                                         next_data->mechanism);
        gsignond_process_data_free (next_data);
    }
}

void gsignond_plugin_proxy_process (GSignondPluginProxy *self, 
                                    GSignondAuthSessionIface *session,
                                    GSignondSessionData *session_data, 
                                    const gchar *mechanism)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));
    g_assert (GSIGNOND_IS_AUTH_SESSION_IFACE (session));

    GSignondPluginProxyPrivate *priv = self->priv;

    if (session == priv->active_session && priv->expecting_request == TRUE) {
        priv->expecting_request = FALSE;
        // mechanism is discarded if this is not an initial request
        gsignond_plugin_request (priv->plugin, session_data);
        return;
    }

    g_queue_push_tail (priv->session_queue,
                       gsignond_process_data_new (session,
                                                  session_data,
                                                  mechanism));
    gsignond_auth_session_iface_notify_state_changed (
                                         session,
                                         GSIGNOND_PLUGIN_STATE_PROCESS_PENDING,
                                         "The request has been queued.");
    if (priv->active_session == NULL) {
        gsignond_plugin_proxy_process_queue (self);
    }
}

static gint
gsignond_plugin_proxy_compare_process_data (gconstpointer process_data,
                                            gconstpointer auth_session)
{
    g_return_val_if_fail (process_data && auth_session, 0);

    if (auth_session == ((GSignondProcessData*) process_data)->auth_session)
        return 0;
    else
        return 1;
}

static GSignondProcessData*
gsignond_plugin_proxy_find_by_session_iface (GSignondPluginProxy *self, 
                                             GSignondAuthSessionIface *session)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));

    return (GSignondProcessData*) g_queue_find_custom (
                                    self->priv->session_queue, 
                                    session,
                                    gsignond_plugin_proxy_compare_process_data);
}

void 
gsignond_plugin_proxy_cancel (GSignondPluginProxy *self, 
                              GSignondAuthSessionIface *session)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));

    GSignondPluginProxyPrivate *priv = self->priv;

    /* cancel active session */
    if (session == priv->active_session) {
        gsignond_plugin_cancel (priv->plugin);
        g_object_unref (priv->active_session);
        priv->active_session = NULL;
        gsignond_plugin_proxy_process_queue (self);
    } else { /* cancel by de-queue */
        GSignondProcessData* data =
            gsignond_plugin_proxy_find_by_session_iface (self, session);
        if (!data) {
            GError* error = g_error_new (GSIGNOND_ERROR, 
                                         GSIGNOND_ERROR_WRONG_STATE,
                                         "Canceling an unknown session");
            gsignond_auth_session_iface_notify_process_error (session, error);
            g_error_free (error);
            return;
        }
        g_queue_remove (priv->session_queue, data);
        gsignond_process_data_free (data);
    }
}

void
gsignond_plugin_proxy_user_action_finished (GSignondPluginProxy *self,
                                            GSignondSessionData *session_data)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));

    if (self->priv->active_session == NULL) {
        ERR("Error: 'user_action_finished' requested for plugin %s but no \
            active session", self->priv->plugin_type);
        return;
    }
    gsignond_plugin_user_action_finished (self->priv->plugin, session_data);
}

void
gsignond_plugin_proxy_refresh (GSignondPluginProxy *self, 
                               GSignondSessionData *session_data)
{
    g_assert (GSIGNOND_IS_PLUGIN_PROXY (self));

    if (self->priv->active_session == NULL) {
        ERR("Error: 'refresh' requested for plugin %s but no active session",
            self->priv->plugin_type);
        return;
    }
    gsignond_plugin_refresh (self->priv->plugin, session_data);
}

static void
gsignond_plugin_proxy_response_final_callback (GSignondPlugin *plugin,
                                               GSignondSessionData *result,
                                               gpointer user_data)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported 'response_final', but no active session \
            in plugin proxy", priv->plugin_type);
        return;
    }
    // This avoids problems if cancel() is called from AuthSession handler
    GSignondAuthSessionIface* active_session = priv->active_session;
    priv->active_session = NULL;
    gsignond_auth_session_iface_notify_process_result (active_session, result);
    g_object_unref (active_session);
    gsignond_plugin_proxy_process_queue (self);
}

static void
gsignond_plugin_proxy_response_callback(GSignondPlugin *plugin, 
                                        GSignondSessionData *result,
                                        gpointer user_data)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported 'response', but no active session \
            in plugin proxy", priv->plugin_type);
        return;
    }
    priv->expecting_request = TRUE;
    gsignond_auth_session_iface_notify_process_result (priv->active_session,
                                                       result);
}

static void
gsignond_plugin_proxy_store_callback (GSignondPlugin *plugin, 
                                      GSignondSessionData *result,
                                      gpointer user_data)
{    
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported 'store', but no active session \
            in plugin proxy", priv->plugin_type);
        return;
    }
    gsignond_auth_session_iface_notify_store (priv->active_session, result);
}

static void
gsignond_plugin_proxy_refreshed_callback (GSignondPlugin *plugin, 
                                          GSignondSessionData *result,
                                          gpointer user_data)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported 'refreshed', but no active session \
            in plugin proxy", priv->plugin_type);
        return;
    }
    gsignond_auth_session_iface_notify_refreshed (priv->active_session, result);
}

static void
gsignond_plugin_proxy_user_action_required_callback (
                                               GSignondPlugin *plugin, 
                                               GSignondSessionData *ui_request, 
                                               gpointer user_data)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported 'user_action_required', but no active session \
            in plugin proxy", priv->plugin_type);
        return;
    }
    gsignond_auth_session_iface_notify_user_action_required(
        priv->active_session, ui_request);
}

static void
gsignond_plugin_proxy_error_callback (GSignondPlugin* plugin, 
                                      GError* error,
                                      gpointer user_data)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported error %s, but no active session \
            in plugin proxy", priv->plugin_type, error->message);
        return;
    }
    // This avoids problems if cancel() is called from AuthSession handler
    GSignondAuthSessionIface *active_session = priv->active_session;
    priv->active_session = NULL;
    gsignond_auth_session_iface_notify_process_error (active_session, error);
    g_object_unref (active_session);
    gsignond_plugin_proxy_process_queue (self);
}

static void
gsignond_plugin_proxy_status_changed_callback (GSignondPlugin *plugin, 
                                               GSignondPluginState state,
                                               const gchar *message,
                                               gpointer user_data)
{
    GSignondPluginProxy *self = GSIGNOND_PLUGIN_PROXY (user_data);
    GSignondPluginProxyPrivate *priv = self->priv;

    if (priv->active_session == NULL) {
        ERR("Error: plugin %s reported change in state %d with message %s, \
            but no active session in plugin proxy", priv->plugin_type, state,
            message);
        return;
    }
    gsignond_auth_session_iface_notify_state_changed (priv->active_session,
                                                      (gint) state, message);
}

