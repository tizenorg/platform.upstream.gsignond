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

#include <gsignond/gsignond-plugin-interface.h>
#include "gsignond-password-plugin.h"
#include <gsignond/gsignond-error.h>

static void gsignond_plugin_interface_init (GSignondPluginInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GSignondPasswordPlugin, gsignond_password_plugin, 
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_PLUGIN,
                                                gsignond_plugin_interface_init));

static void gsignond_password_plugin_cancel (GSignondPlugin *self)
{
    GError* error = g_error_new(GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_SESSION_CANCELED,
                                "Session canceled");
    gsignond_plugin_error (self, error); 
    g_error_free(error);
}

static void gsignond_password_plugin_request (
    GSignondPlugin *self, GSignondSessionData *session_data)
{

}

static void gsignond_password_plugin_request_initial (
    GSignondPlugin *self, GSignondSessionData *session_data, 
    GSignondDictionary* identity_method_cache,
    const gchar *mechanism)
{
    const gchar* username = gsignond_session_data_get_username(session_data);
    const gchar* secret = gsignond_session_data_get_secret(session_data);
    
    if (secret && secret[0]) {
        GSignondSessionData *response = gsignond_dictionary_new();
        if (username && username[0])
            gsignond_session_data_set_username(response, username);
        gsignond_session_data_set_secret(response, secret);
        gsignond_plugin_response_final(self, response);
        gsignond_dictionary_unref(response);
        return;
    }
    
    GSignondSignonuiData *user_action_data = gsignond_signonui_data_new();
    if (!username || !username[0])
        gsignond_signonui_data_set_query_username(user_action_data, TRUE);
    else {
        gsignond_signonui_data_set_query_username(user_action_data, FALSE);
        gsignond_signonui_data_set_username(user_action_data, username);
    }
    gsignond_signonui_data_set_query_password(user_action_data, TRUE);
    gsignond_plugin_user_action_required(self, user_action_data);
    gsignond_dictionary_unref(user_action_data);
}

static void gsignond_password_plugin_user_action_finished (
    GSignondPlugin *self, 
    GSignondSignonuiData *session_data)
{
    GSignondSignonuiError query_error;
    gboolean res = gsignond_signonui_data_get_query_error(session_data,
                                                          &query_error);
    if (res == FALSE) {
        GError* error = g_error_new(GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_USER_INTERACTION,
                                "userActionFinished did not return an error value");
        gsignond_plugin_error (self, error); 
        g_error_free(error);
        return;
    }
    const gchar* username = gsignond_signonui_data_get_username(session_data);
    const gchar* secret = gsignond_signonui_data_get_password(session_data);
    
    if (query_error == SIGNONUI_ERROR_NONE && 
        username != NULL && 
        secret != NULL) {
        GSignondSessionData *response = gsignond_dictionary_new();
        gsignond_session_data_set_username(response, username);
        gsignond_session_data_set_secret(response, secret);
        gsignond_plugin_response_final(self, response);
        gsignond_dictionary_unref(response);
        return;
    } else if (query_error == SIGNONUI_ERROR_CANCELED) {
        GError* error = g_error_new(GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_SESSION_CANCELED,
                                "Session canceled");
        gsignond_plugin_error (self, error); 
        g_error_free(error);
    } else {
        GError* error = g_error_new(GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_USER_INTERACTION,
                                "userActionFinished error: %d",
                                query_error);
        gsignond_plugin_error (self, error); 
        g_error_free(error);
    }
}

static void gsignond_password_plugin_refresh (
    GSignondPlugin *self, 
    GSignondSignonuiData *session_data)
{
    gsignond_plugin_refreshed(self, session_data);
}

static void
gsignond_plugin_interface_init (GSignondPluginInterface *iface)
{
    iface->cancel = gsignond_password_plugin_cancel;
    iface->request_initial = gsignond_password_plugin_request_initial;
    iface->request = gsignond_password_plugin_request;
    iface->user_action_finished = gsignond_password_plugin_user_action_finished;
    iface->refresh = gsignond_password_plugin_refresh;
}

static void
gsignond_password_plugin_init (GSignondPasswordPlugin *self)
{
    
    
}

enum
{
    PROP_0,
    
    PROP_TYPE,
    PROP_MECHANISMS
};

static void
gsignond_password_plugin_set_property (GObject      *object,
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
gsignond_password_plugin_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
    GSignondPasswordPlugin *password_plugin = GSIGNOND_PASSWORD_PLUGIN (object);
    (void) password_plugin;
    gchar *mechanisms[] = { "password", NULL };
    
    switch (prop_id)
    {
        case PROP_TYPE:
            g_value_set_string (value, "password");
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
gsignond_password_plugin_class_init (GSignondPasswordPluginClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->set_property = gsignond_password_plugin_set_property;
    gobject_class->get_property = gsignond_password_plugin_get_property;
    
    g_object_class_override_property (gobject_class, PROP_TYPE, "type");
    g_object_class_override_property (gobject_class, PROP_MECHANISMS, 
                                      "mechanisms");
}
