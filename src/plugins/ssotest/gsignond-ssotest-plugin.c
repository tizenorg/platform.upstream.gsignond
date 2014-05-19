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

#include <gsignond/gsignond-plugin-interface.h>
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-log.h>

#include "gsignond-ssotest-plugin.h"

/**
 * SECTION:gsignond-ssotest-plugin
 * @short_description: a plugin that is used internally in gsignond unit test
 * @include: gsignond/gsignond-ssotest-plugin.h
 *
 * #GSignondSsoTestPlugin is used by gsignond unit tests. It should not be used
 * by applications.
 */
/**
 * GSignondSsoTestPlugin:
 *
 * Opaque #GSignondSsoTestPlugin data structure.
 */
/**
 * GSignondSsoTestPluginClass:
 * @parent_class: the parent class structure
 *
 * Opaque #GSignondSsoTestPluginClass data structure.
 */

#define GSIGNOND_SSOTEST_PLUGIN_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                  GSIGNOND_TYPE_SSOTEST_PLUGIN, \
                                  GSignondSsoTestPluginPrivate))

enum
{
    PROP_0,
    PROP_TYPE,
    PROP_MECHANISMS
};

struct _GSignondSsoTestPluginPrivate
{
    gboolean is_canceled;
};

static const gchar *method = "ssotest";
static const gchar *mechanisms[] = { "mech1", "mech2", "mech3", "BLOB", NULL };

static void gsignond_plugin_interface_init (GSignondPluginInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GSignondSsoTestPlugin, gsignond_ssotest_plugin, 
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_PLUGIN,
                                                gsignond_plugin_interface_init));

static void gsignond_ssotest_plugin_cancel (GSignondPlugin *plugin)
{
    GSignondSsoTestPlugin *self = GSIGNOND_SSOTEST_PLUGIN (plugin);

    self->priv->is_canceled = TRUE;
}

static void gsignond_ssotest_plugin_request_initial (
    GSignondPlugin *plugin, GSignondSessionData *session_data, 
    GSignondDictionary* identity_method_cache,
    const gchar *mechanism)
{
    gint i;
    GSignondSsoTestPlugin *self = GSIGNOND_SSOTEST_PLUGIN (plugin);

    self->priv->is_canceled = FALSE;

    GSignondSessionData *response = gsignond_dictionary_copy (session_data);
    DBG ("response=%p", response);
    gsignond_session_data_set_realm (response, "testRealm_after_test");

    for (i = 0; i < 10; i++) {
        if (!self->priv->is_canceled) {
            gsignond_plugin_status_changed (GSIGNOND_PLUGIN (self),
                                            GSIGNOND_PLUGIN_STATE_WAITING,
                                            "hello from the test plugin");
            g_usleep (1000 * 1000 / 100);
            g_main_context_iteration (NULL, FALSE);
        }
    }
    if (self->priv->is_canceled) {
        INFO ("Operation is canceled");
        GError* error = g_error_new (GSIGNOND_ERROR, 
                                     GSIGNOND_ERROR_SESSION_CANCELED,
                                     "Session canceled");
        gsignond_plugin_error (plugin, error); 
        g_error_free (error);
        gsignond_dictionary_unref (response);
        return;
    }

    if (g_strcmp0 (mechanism, "BLOB") == 0) {
        gsignond_plugin_response_final (plugin, response);
        gsignond_dictionary_unref (response);
        INFO ("mechanism 'BLOB' responded");
        return;
    }

    GHashTableIter iter;
    gchar *key;
    GVariant *value;
    g_hash_table_iter_init (&iter, session_data);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer *) &key, (gpointer *) &value))
        INFO ("Key: %s", key);

    if (g_strcmp0 (mechanism, "mech1") == 0) {
        gsignond_plugin_response_final (plugin, response);
        gsignond_dictionary_unref (response);
        INFO ("mechanism 'mech1' responded");
        return;
    }

    if (g_strcmp0 (mechanism, "mech2") == 0) {
        const gchar* username =
            gsignond_session_data_get_username (session_data);
        GSignondSignonuiData *user_action_data = gsignond_dictionary_new();
        if (username == NULL)
            gsignond_signonui_data_set_query_username (user_action_data, TRUE);
        else
            gsignond_signonui_data_set_username (user_action_data, username);
        gsignond_signonui_data_set_query_password (user_action_data, TRUE);
        gsignond_plugin_user_action_required (plugin, user_action_data);
        gsignond_dictionary_unref (user_action_data);
        gsignond_dictionary_unref (response);
        INFO ("mechanism 'mech2' responded'");
        return;
    }

    INFO ("no response from plugin");
    gsignond_dictionary_unref (response);
}

static void gsignond_ssotest_plugin_user_action_finished (
    GSignondPlugin *plugin, 
    GSignondSignonuiData *session_data)
{
    g_return_if_fail (GSIGNOND_IS_SSOTEST_PLUGIN (plugin));

    GSignondSignonuiError query_error;
    gboolean res = gsignond_signonui_data_get_query_error(session_data,
                                                          &query_error);
    if (res == FALSE) {
        GError* error = g_error_new(GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_USER_INTERACTION,
                                "userActionFinished did not return an error value");
        gsignond_plugin_error (plugin, error); 
        g_error_free(error);
    }

    const gchar* username = gsignond_signonui_data_get_username (session_data);
    const gchar* secret = gsignond_signonui_data_get_password (session_data);
    
    if (query_error == SIGNONUI_ERROR_NONE && 
        username != NULL && 
        secret != NULL) {
        GSignondSessionData *response = gsignond_dictionary_new ();
        gsignond_session_data_set_username (response, username);
        gsignond_session_data_set_secret (response, secret);
        gsignond_plugin_response_final (plugin, response);
        gsignond_dictionary_unref (response);
        return;
    } else if (query_error == SIGNONUI_ERROR_CANCELED) {
        GError* error = g_error_new (GSIGNOND_ERROR, 
                                     GSIGNOND_ERROR_SESSION_CANCELED,
                                     "user_action_finished: canceled");
        gsignond_plugin_error (plugin, error); 
        g_error_free(error);
    } else if (query_error == SIGNONUI_ERROR_FORBIDDEN) {
        GError* error = g_error_new (GSIGNOND_ERROR, 
                                     GSIGNOND_ERROR_NOT_AUTHORIZED,
                                     "user_action_finished: forbidden");
        gsignond_plugin_error (plugin, error); 
        g_error_free(error);
    } else {
        GError* error = g_error_new(GSIGNOND_ERROR, 
                                    GSIGNOND_ERROR_USER_INTERACTION,
                                    "user_action_finished error: %d",
                                    query_error);
        gsignond_plugin_error (plugin, error); 
        g_error_free(error);
    }
}

static void gsignond_ssotest_plugin_refresh (
    GSignondPlugin *plugin, 
    GSignondSignonuiData *ui_data)
{
    gsignond_plugin_refreshed (plugin, ui_data);
}

static void
gsignond_plugin_interface_init (GSignondPluginInterface *iface)
{
    iface->cancel = gsignond_ssotest_plugin_cancel;
    iface->request_initial = gsignond_ssotest_plugin_request_initial;
    iface->user_action_finished = gsignond_ssotest_plugin_user_action_finished;
    iface->refresh = gsignond_ssotest_plugin_refresh;
}

static void
gsignond_ssotest_plugin_init (GSignondSsoTestPlugin *plugin)
{
    GSignondSsoTestPlugin *self = GSIGNOND_SSOTEST_PLUGIN (plugin);

    self->priv = GSIGNOND_SSOTEST_PLUGIN_GET_PRIVATE (self);
}

static void
gsignond_ssotest_plugin_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
    switch (property_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gsignond_ssotest_plugin_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
    g_return_if_fail (GSIGNOND_IS_SSOTEST_PLUGIN (object));
    
    switch (prop_id)
    {
        case PROP_TYPE:
            g_value_set_string (value, method);
            break;
        case PROP_MECHANISMS:
            g_value_set_boxed (value, mechanisms);
            break;
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gsignond_ssotest_plugin_class_init (GSignondSsoTestPluginClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->set_property = gsignond_ssotest_plugin_set_property;
    gobject_class->get_property = gsignond_ssotest_plugin_get_property;
    
    g_object_class_override_property (gobject_class, PROP_TYPE, "type");
    g_object_class_override_property (gobject_class, PROP_MECHANISMS, 
                                      "mechanisms");

    g_type_class_add_private (gobject_class,
                              sizeof (GSignondSsoTestPluginPrivate));
}

