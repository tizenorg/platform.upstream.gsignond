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


#include "gsignond/gsignond-plugin-interface.h"
#include "gsignond-plugin-enum-types.h"

G_DEFINE_INTERFACE (GSignondPlugin, gsignond_plugin, 0)

/* signals */
enum
{
    RESULT,
    STORE,
    ERROR,
    USER_ACTION_REQUIRED,
    REFRESHED,
    STATUS_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void gsignond_plugin_default_init (GSignondPluginInterface *g_class)
{
    signals[RESULT] = g_signal_new ("result", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SESSION_DATA);

    signals[STORE] = g_signal_new ("store", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SESSION_DATA);

    signals[ERROR] = g_signal_new ("error", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, G_TYPE_ERROR);
    
    signals[USER_ACTION_REQUIRED] = g_signal_new ("user-action-required", 
        G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SESSION_DATA);

    signals[REFRESHED] = g_signal_new ("refreshed", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SESSION_DATA);

    signals[STATUS_CHANGED] = g_signal_new ("status-changed", 
        G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        2, GSIGNOND_TYPE_PLUGIN_STATE, G_TYPE_STRING);

    g_object_interface_install_property (g_class,
	g_param_spec_string ("type", "Type", "Plugin type", "none", 
                             G_PARAM_READABLE));

     g_object_interface_install_property (g_class,
        g_param_spec_boxed ("mechanisms", "Mechanisms", 
                            "List of plugin mechanisms", 
                            G_TYPE_STRV, G_PARAM_READABLE));
    
}

void gsignond_plugin_cancel (GSignondPlugin *self)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->cancel (self);
}

void gsignond_plugin_abort (GSignondPlugin *self)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->abort (self);
}

void gsignond_plugin_process (GSignondPlugin *self, 
                              GSignondSessionData *session_data, 
                              const gchar *mechanism)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->process (self, session_data, mechanism);
}

void gsignond_plugin_user_action_finished (GSignondPlugin *self, 
                                           GSignondSessionData *session_data)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->user_action_finished (self, 
                                                                session_data);
}

void gsignond_plugin_refresh (GSignondPlugin *self, 
                              GSignondSessionData *session_data)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->refresh (self, session_data);
}

void gsignond_plugin_result (GSignondPlugin *self, 
                             GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[RESULT], 0, session_data);
}

void gsignond_plugin_store (GSignondPlugin *self, 
                            GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[STORE], 0, session_data);
}

void gsignond_plugin_error (GSignondPlugin *self, GError *error)
{
    g_signal_emit (self, signals[ERROR], 0, error);
}

void gsignond_plugin_user_action_required (GSignondPlugin *self, 
                                           GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[USER_ACTION_REQUIRED], 0, session_data);
}

void gsignond_plugin_refreshed (GSignondPlugin *self, 
                                GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[REFRESHED], 0, session_data);
}

void gsignond_plugin_status_changed (GSignondPlugin *self, GSignondPluginState state, 
                                     const gchar *message)
{
    g_signal_emit (self, signals[STATUS_CHANGED], 0, state, message);
}

