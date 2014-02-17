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

/**
 * SECTION:gsignond-plugin-interface
 * @short_description: an interface for implementing GLib-based authentication plugins
 * @include: gsignond/gsignond-plugin-interface.h
 *
 * #GSignondPlugin is an interface for implementing GLib-based authentication plugins.
 * 
 * When creating a plugin, write the #GObject boilerplate code as usual, but
 * 
 * a) declare the type as follows:
 * 
 * |[    G_DEFINE_TYPE_WITH_CODE (GSignondPasswordPlugin, gsignond_password_plugin, 
 *                         G_TYPE_OBJECT,
 *                         G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_PLUGIN,
 *                                                gsignond_plugin_interface_init));
 * ]| 
 * 
 * b) implement <function>gsignond_plugin_interface_init</function> as follows:
 * 
 * |[ static void
 * gsignond_plugin_interface_init (GSignondPluginInterface *iface)
 * {
 *     iface->cancel = gsignond_password_plugin_cancel;
 *     iface->request_initial = gsignond_password_plugin_request_initial;
 *     iface->request = gsignond_password_plugin_request;
 *     iface->user_action_finished = gsignond_password_plugin_user_action_finished;
 *     iface->refresh = gsignond_password_plugin_refresh;
 * }
 * ]|
 * 
 * where the <function>gsignond_password_plugin_cancel</function> etc. are specific implementations of
 * plugin interface methods that every plugin must provide (see below for when
 * and how they're used by the daemon).
 * 
 * c) override #GSignondPlugin:type and #GSignondPlugin:mechanisms property 
 * implementations in the plugin class constructor like this:
 * 
 * |[static void
 * gsignond_password_plugin_class_init (GSignondPasswordPluginClass *klass)
 * {
 *     GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
 *     
 *     gobject_class->set_property = gsignond_password_plugin_set_property;
 *     gobject_class->get_property = gsignond_password_plugin_get_property;
 *     
 *     g_object_class_override_property (gobject_class, PROP_TYPE, "type");
 *     g_object_class_override_property (gobject_class, PROP_MECHANISMS, 
 *                                       "mechanisms");
 * }
 * ]|
 * (naturally, plugin's property setter should ignore attempts to set these properties,
 * and plugin's property getter should provide their values when asked)
 * 
 * <refsect1><title>The plugin API</title></refsect1>
 * 
 * Plugins implement authentication sessions which are controlled through the
 * plugin API. Authentication sessions follow one another so there is only one active
 * session at a time.
 * 
 * The plugin API is a set of methods and signals that should be used in a specific
 * sequence:
 * 
 * - successful authentication session begins with gsignond_plugin_request_initial() and ends
 * with the plugin issuing a #GSignondPlugin::response-final signal
 * - at any point the application can cancel an active session with 
 * gsignond_plugin_cancel()
 * - at any point the plugin can cancel an active session by issuing #GSignondPlugin::error
 * signal, which also provides some details about the cancellation reason.
 * - if a session is active, and the plugin has an intermediate response or needs
 * additional information, it issues #GSignondPlugin::response signal, which the 
 * application should respond to with gsignond_plugin_request() method. This can
 * happen more than once.
 * - if the plugin needs to launch UI interaction with the user, it's issuing
 * #GSignondPlugin::user-action-required signal, which the application should
 * follow up with gsignond_plugin_user_action_finished() method. This can happen
 * more than once as well.
 * - if, during an active UI session, the application needs a UI refresh 
 * (for example, to fetch a new captcha image), it's
 * requested from the plugin with gsignond_plugin_refresh() method, followed
 * by the plugin's response via #GSignondPlugin::refreshed signal. This can happen
 * more than once.
 * - changes in plugin state are reported through #GSignondPlugin::status-changed signal.
 * - if the plugin needs to store information in persistent storage, it issues
 * #GSignondPlugin::store signal. Later, that same information is provided as a
 * parameter to gsignond_plugin_request_initial().
 * 
 * <refsect1><title>Example plugins</title></refsect1>
 * 
 * See example plugin implementation here:
 * <ulink url="https://code.google.com/p/accounts-sso/source/browse/?repo=gsignond#git%2Fsrc%2Fplugins">
 * https://code.google.com/p/accounts-sso/source/browse/?repo=gsignond#git%2Fsrc%2Fplugins</ulink>.
 * 
 * For examples of out of tree plugins, you can have a look at SASL or OAuth plugin
 * implementations:
 * <ulink url="http://code.google.com/p/accounts-sso/source/browse?repo=gsignond-plugin-sasl">
 * http://code.google.com/p/accounts-sso/source/browse?repo=gsignond-plugin-sasl</ulink>.
 * 
 * <ulink url="http://code.google.com/p/accounts-sso/source/browse?repo=gsignond-plugin-oa">
 * http://code.google.com/p/accounts-sso/source/browse?repo=gsignond-plugin-oa</ulink>.
 * 
 */


/**
 * GSignondPluginState:
 * @GSIGNOND_PLUGIN_STATE_NONE: State unknown
 * @GSIGNOND_PLUGIN_STATE_RESOLVING: Resolving remote server host name
 * @GSIGNOND_PLUGIN_STATE_CONNECTING: Connecting to remote server
 * @GSIGNOND_PLUGIN_STATE_SENDING_DATA: Sending data to remote server
 * @GSIGNOND_PLUGIN_STATE_WAITING: Waiting for reply from remote server
 * @GSIGNOND_PLUGIN_STATE_USER_PENDING: Waiting for response from user
 * @GSIGNOND_PLUGIN_STATE_REFRESHING: Refreshing ui request
 * @GSIGNOND_PLUGIN_STATE_PROCESS_PENDING: Request has been queued
 * @GSIGNOND_PLUGIN_STATE_STARTED: Request has been dequeued
 * @GSIGNOND_PLUGIN_STATE_CANCELING: Canceling current process
 * @GSIGNOND_PLUGIN_STATE_DONE: Process is finished
 * @GSIGNOND_PLUGIN_STATE_HOLDING: Holding long non-expired token
 * 
 * The plugin provides state updates by emitting #GSignondPlugin::status-changed
 * signal with this enum and a string describing what happened.
 */

/**
 * GSignondPlugin:
 *
 * Opaque #GSignondPlugin data structure.
 */
G_DEFINE_INTERFACE (GSignondPlugin, gsignond_plugin, 0)

/**
 * GSignondPluginInterface:
 * @parent: parent interface type.
 * @cancel: implementation of gsignond_plugin_cancel()
 * @request_initial: implementation of gsignond_plugin_request_initial()
 * @request: implementation of gsignond_plugin_request()
 * @user_action_finished: implementation of gsignond_plugin_user_action_finished()
 * @refresh: implementation of gsignond_plugin_refresh()
 * 
 * #GSignondPluginInterface interface containing pointers to methods that all
 * plugin implementations should provide.
 */

/* signals */
enum
{
    RESPONSE,
    RESPONSE_FINAL,
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
    /**
     * GSignondPlugin::response:
     * @plugin: the plugin which emitted the signal
     * @session_data: a #GSignondSessionData containing signal parameters
     * 
     * This signal is issued by the plugin when it wants to provide an intermediate
     * response to the application or needs additional information from the application.
     * 
     * After issuing this signal the plugin expects a gsignond_plugin_response() call.
     */
    signals[RESPONSE] = g_signal_new ("response", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SESSION_DATA);

    /**
     * GSignondPlugin::response-final:
     * @plugin: the plugin which emitted the signal
     * @session_data: a #GSignondSessionData containing signal parameters
     * 
     * This signal is issued by the plugin when it has completed the authentication
     * sequence and is used to provide the final response to the application.
     * 
     * After issuing this signal the plugin is idle and is ready for a new
     * authentication session.
     */
    signals[RESPONSE_FINAL] = g_signal_new ("response-final", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SESSION_DATA);

    /**
     * GSignondPlugin::store:
     * @plugin: the plugin which emitted the signal
     * @data: a #GSignondDictionary containing data to place in persistent storage
     * 
     * This signal is issued by the plugin when it has data to store in persistant
     * storage. The same data would later be provided to plugin via 
     * gsignond_plugin_request_initial @identity_method_cache parameter.
     */
    signals[STORE] = g_signal_new ("store", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_DICTIONARY);

    /**
     * GSignondPlugin::error:
     * @plugin: the plugin which emitted the signal
     * @error: the details of the error
     * 
     * This signal is issued by the plugin when an error has occured, or the
     * plugin otherwise has a reason to cancel the authentication session. The
     * @error should be specified according to 
     * <link linkend="gsignond-Errors">GSignond errors.</link>
     * 
     */
    signals[ERROR] = g_signal_new ("error", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, G_TYPE_ERROR);
    
    /**
     * GSignondPlugin::user-action-required:
     * @plugin: the plugin which emitted the signal
     * @ui_data: parameters for UI interaction
     * 
     * This signal is issued by the plugin when it needs a UI interaction with
     * the user to happen. When the interaction is complete, gsignond_plugin_user_action_finished()
     * should be issued.
     */
    signals[USER_ACTION_REQUIRED] = g_signal_new ("user-action-required", 
        G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SIGNONUI_DATA);

    /**
     * GSignondPlugin::refreshed:
     * @plugin: the plugin which emitted the signal
     * @ui_data: parameters for UI refresh
     * 
     * This signal is issued by the plugin when the UI interaction is ongoing
     * and the UI needs to be refreshed. This can be used for example to update
     * captcha image in the UI.
     */
    signals[REFRESHED] = g_signal_new ("refreshed", G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        1, GSIGNOND_TYPE_SIGNONUI_DATA);

    /**
     * GSignondPlugin::status-changed:
     * @plugin: the plugin which emitted the signal
     * @state: the plugin state
     * @message: the message that accompanies the state change
     * 
     * This signal is issued by the plugin when plugin state has changed. This
     * can be used by applications to report authentication progress.
     */
    signals[STATUS_CHANGED] = g_signal_new ("status-changed", 
        G_TYPE_FROM_CLASS (g_class),
        G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
        2, GSIGNOND_TYPE_PLUGIN_STATE, G_TYPE_STRING);

    /**
     * GSignondPlugin:type:
     * 
     * This property holds a plugin type, or authentication method it implements
     * (for example "oauth" or "sasl").
     */
    g_object_interface_install_property (g_class, g_param_spec_string ("type",
            "Type", "Plugin type", "none",
            G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

    /**
     * GSignondPlugin:mechanisms:
     * 
     * This property holds a list of authentication mechanisms that the plugin
     * implements, all specified within the authentication method. For example,
     * OAuth plugin could implement "oauth1" and "oauth2" mechanisms.
     */
    g_object_interface_install_property (g_class, g_param_spec_boxed (
            "mechanisms", "Mechanisms", "List of plugin mechanisms",
            G_TYPE_STRV, G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
    
}

/**
 * gsignond_plugin_cancel:
 * @self: plugin instance
 * 
 * This method cancels an ongoing authentication session. The plugin implementations
 * should issue a #GSignondPlugin::error signal with #GSIGNOND_ERROR_SESSION_CANCELED
 * error, and prepare for a new authentication session.
 */
void gsignond_plugin_cancel (GSignondPlugin *self)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->cancel (self);
}

/**
 * gsignond_plugin_request_initial:
 * @self: plugin instance 
 * @session_data: parameters for the session
 * @identity_method_cache: data from persistent storage, saved previously via
 * #GSignondPlugin::store signal
 * @mechanism: mechanism to use for the authentication
 * 
 * This method starts a new authentication session.
 */
void gsignond_plugin_request_initial (GSignondPlugin *self, 
                              GSignondSessionData *session_data, 
                              GSignondDictionary *identity_method_cache,
                              const gchar *mechanism)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->request_initial (self, session_data,
            identity_method_cache,
            mechanism);
}

/**
 * gsignond_plugin_request:
 * @self: plugin instance 
 * @session_data: additional parameters for the session
 * 
 * This method provides the plugin with additional parameters for the session
 * after the plugin has asked for it via #GSignondPlugin::response signal.
 */
void gsignond_plugin_request (GSignondPlugin *self, 
                              GSignondSessionData *session_data)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->request (self, session_data);
}

/**
 * gsignond_plugin_user_action_finished:
 * @self: plugin instance 
 * @ui_data: results of UI interaction
 * 
 * This method provides the plugin with the results of UI interaction
 * after the plugin has asked for it via #GSignondPlugin::user-action-required signal.
 */
void gsignond_plugin_user_action_finished (GSignondPlugin *self, 
                                           GSignondSignonuiData *ui_data)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->user_action_finished (self, 
                                                                ui_data);
}

/**
 * gsignond_plugin_refresh:
 * @self: plugin instance 
 * @ui_data: UI refresh parameters
 * 
 * This method asks the plugin to refresh the UI. The plugin responds with
 * #GSignondPlugin::refreshed signal.
 */
void gsignond_plugin_refresh (GSignondPlugin *self, 
                              GSignondSignonuiData *ui_data)
{
    g_return_if_fail (GSIGNOND_IS_PLUGIN (self));
    
    GSIGNOND_PLUGIN_GET_INTERFACE (self)->refresh (self, ui_data);
}

/**
 * gsignond_plugin_response:
 * @self: plugin instance
 * @session_data: session data
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::response
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_response (GSignondPlugin *self, 
                             GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[RESPONSE], 0, session_data);
}

/**
 * gsignond_plugin_response_final:
 * @self: plugin instance
 * @session_data: session data
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::response-final
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_response_final (GSignondPlugin *self, 
                             GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[RESPONSE_FINAL], 0, session_data);
}

/**
 * gsignond_plugin_store:
 * @self: plugin instance
 * @identity_method_cache: data to store
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::store
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_store (GSignondPlugin *self, 
                            GSignondDictionary *identity_method_cache)
{
    g_signal_emit (self, signals[STORE], 0, identity_method_cache);
}

/**
 * gsignond_plugin_error:
 * @self: plugin instance
 * @error: the error
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::error
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_error (GSignondPlugin *self, GError *error)
{
    g_signal_emit (self, signals[ERROR], 0, error);
}

/**
 * gsignond_plugin_user_action_required:
 * @self: plugin instance
 * @ui_data: UI data
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::user-action-required
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_user_action_required (GSignondPlugin *self, 
                                           GSignondSignonuiData *ui_data)
{
    g_signal_emit (self, signals[USER_ACTION_REQUIRED], 0, ui_data);
}

/**
 * gsignond_plugin_refreshed:
 * @self: plugin instance
 * @ui_data: UI data
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::refreshed
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_refreshed (GSignondPlugin *self, 
                                GSignondSignonuiData *ui_data)
{
    g_signal_emit (self, signals[REFRESHED], 0, ui_data);
}

/**
 * gsignond_plugin_status_changed:
 * @self: plugin instance
 * @state: the new state
 * @message: the message
 * 
 * Plugin implementations should use this to issue #GSignondPlugin::status-changed
 * signal. This method should not be used otherwise.
 */
void gsignond_plugin_status_changed (GSignondPlugin *self,
        GSignondPluginState state, const gchar *message)
{
    g_signal_emit (self, signals[STATUS_CHANGED], 0, state, message);
}

