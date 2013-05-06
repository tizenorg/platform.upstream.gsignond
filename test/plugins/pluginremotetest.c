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

#include <check.h>
#include <stdlib.h>
#include <errno.h>
#include "gsignond-plugin-remote-private.h"
#include "gsignond-plugin-remote.h"
#include "plugind/gsignond-plugin-daemon.h"
#include <gsignond/gsignond-plugin-interface.h>
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-config.h>
#include <gsignond/gsignond-log.h>

static GMainLoop *main_loop = NULL;
guint child_watch_id = 0;

typedef struct _GSignondAuthSession GSignondAuthSession;
typedef struct _GSignondAuthSessionClass GSignondAuthSessionClass;

struct _GSignondAuthSession
{
    GObject parent;
};

struct _GSignondAuthSessionClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE (GSignondAuthSession, gsignond_auth_session, G_TYPE_OBJECT);

static void
_stop_mainloop ()
{
    if (main_loop) {
        g_main_loop_quit (main_loop);
    }
}

static void
_run_mainloop ()
{
    g_main_loop_run (main_loop);
}

static void
_setup ()
{
    g_type_init ();
    if (main_loop == NULL) {
        main_loop = g_main_loop_new (NULL, FALSE);
    }
}

static void
_teardown ()
{
    if (main_loop) {
        _stop_mainloop ();
        g_main_loop_unref (main_loop);
        main_loop = NULL;
    }
}

static void
gsignond_auth_session_init (
        GSignondAuthSession *self)
{
}

static void
gsignond_auth_session_class_init (
        GSignondAuthSessionClass *klass)
{
}

void
gsignond_auth_session_notify_process_result (
        GSignondAuthSession *iface,
        GSignondSessionData *result,
        gpointer user_data)
{
}

void
gsignond_auth_session_notify_process_error (
        GSignondAuthSession *iface,
        const GError *error,
        gpointer user_data)
{
}

void
gsignond_auth_session_notify_store (
        GSignondAuthSession *self,
        GSignondSessionData *session_data)
{
}

void
gsignond_auth_session_notify_user_action_required (
        GSignondAuthSession *self,
        GSignondSignonuiData *session_data)
{
}

void
gsignond_auth_session_notify_refreshed (
        GSignondAuthSession *self,
        GSignondSignonuiData *session_data)
{
}

void
gsignond_auth_session_notify_state_changed (
        GSignondAuthSession *self,
        gint state,
        const gchar *message,
        gpointer user_data)
{
}

static void
check_plugin(
        GSignondPlugin* plugin)
{
    gchar* type;
    gchar** mechanisms;

    fail_if(plugin == NULL);
    
    g_object_get(plugin, "type", &type, "mechanisms", &mechanisms, NULL);
    
    fail_unless(g_strcmp0(type, "password") == 0);
    fail_unless(g_strcmp0(mechanisms[0], "password") == 0);
    fail_unless(mechanisms[1] == NULL);
    
    g_free(type);
    g_strfreev(mechanisms);
}

static void
response_callback(
        GSignondPlugin* plugin,
        GSignondSessionData* result,
        gpointer user_data)
{
    DBG ("");
    GSignondSessionData** user_data_p = user_data;
    *user_data_p = gsignond_dictionary_copy(result);
    _stop_mainloop ();
}

static void
user_action_required_callback(
        GSignondPlugin* plugin,
        GSignondSignonuiData* ui_request,
        gpointer user_data)
{
    DBG ("");
    GSignondSignonuiData** user_data_p = user_data;
    *user_data_p = gsignond_dictionary_copy(ui_request);
    _stop_mainloop ();
}

static void
error_callback(
        GSignondPlugin* plugin,
        GError* error,
        gpointer user_data)
{
    DBG ("");
    GError** user_data_p = user_data;
    *user_data_p = g_error_copy(error);
    _stop_mainloop ();
}

START_TEST (test_pluginremote_create)
{
    DBG ("");
    GSignondPlugin *plugin = NULL;
    const gchar *plugin_type = "password";

    GSignondConfig* config = gsignond_config_new ();
    fail_if (config == NULL);

    plugin = GSIGNOND_PLUGIN (gsignond_plugin_remote_new(config, plugin_type));

    check_plugin (plugin);
    g_object_unref (config);
    g_object_unref (plugin);
}
END_TEST

static void
_child_watch_cb (
        GPid  pid,
        gint  status,
        gpointer data)
{
    DBG ("plugind GONE");
    _stop_mainloop ();
    g_source_remove (child_watch_id);
    child_watch_id = 0;
}

START_TEST (test_pluginremote_plugind_create)
{
    DBG ("");
    GSignondPlugin *plugin = NULL;
    const gchar *plugin_type = "password";
    GSignondPluginRemotePrivate* priv = NULL;
    gint cpid = 0;

    GSignondConfig* config = gsignond_config_new ();
    fail_if (config == NULL);
    
    plugin = GSIGNOND_PLUGIN (gsignond_plugin_remote_new(config, plugin_type));
    fail_if (plugin == NULL);
    priv = (GSignondPluginRemotePrivate *)GSIGNOND_PLUGIN_REMOTE (plugin)->priv;

    fail_unless (priv->child_watch_id > 0);
    fail_unless (priv->cpid > 0);
    cpid = priv->cpid;

    child_watch_id = g_child_watch_add (cpid,
            (GChildWatchFunc)_child_watch_cb, plugin);

    g_object_unref (plugin);
    g_object_unref (config);

    _run_mainloop ();

    fail_unless (child_watch_id == 0);
    fail_unless (kill (cpid, 0) != 0);
}
END_TEST

START_TEST (test_pluginremote_plugind_kill)
{
    DBG ("");
    GSignondPlugin *plugin = NULL;
    const gchar *plugin_type = "password";
    GSignondPluginRemotePrivate* priv = NULL;

    GSignondConfig* config = gsignond_config_new ();
    fail_if (config == NULL);

    plugin = GSIGNOND_PLUGIN (gsignond_plugin_remote_new(config, plugin_type));
    fail_if (plugin == NULL);
    priv = (GSignondPluginRemotePrivate *)GSIGNOND_PLUGIN_REMOTE (plugin)->priv;

    fail_unless (priv->child_watch_id > 0);
    fail_unless (priv->cpid > 0);
    fail_unless (kill (priv->cpid, 0) == 0);

    child_watch_id = g_child_watch_add (priv->cpid,
            (GChildWatchFunc)_child_watch_cb, plugin);

    kill (priv->cpid, SIGTERM);
    _run_mainloop ();

    fail_unless (child_watch_id == 0);

    g_object_unref (plugin);
    g_object_unref (config);
}
END_TEST

START_TEST (test_pluginremote_request)
{
    DBG ("");
    GSignondPlugin *plugin = NULL;
    const gchar *plugin_type = "password";

    GSignondConfig* config = gsignond_config_new ();
    fail_if(config == NULL);
    
    plugin = GSIGNOND_PLUGIN (gsignond_plugin_remote_new(config, plugin_type));
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GSignondSignonuiData* ui_action = NULL;
    GError* error = NULL;
    gboolean bool_res;

    g_signal_connect(plugin, "response-final", G_CALLBACK(response_callback),
            &result);
    g_signal_connect(plugin, "user-action-required", 
                     G_CALLBACK(user_action_required_callback), &ui_action);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSessionData* data = gsignond_dictionary_new ();

    // username empty, password not empty
    gsignond_session_data_set_secret(data, "megapassword");
    gsignond_plugin_request_initial(plugin, data, "password");
    _run_mainloop ();

    fail_if(result == NULL);    
    fail_if(ui_action != NULL);
    fail_if(error != NULL);
    fail_if(gsignond_session_data_get_username(result) != NULL);
    fail_if(g_strcmp0(
        gsignond_session_data_get_secret(result), "megapassword") != 0);
    gsignond_dictionary_unref(result);
    result = NULL;
    
    // username and password not empty
    gsignond_session_data_set_username(data, "megauser");
    gsignond_plugin_request_initial(plugin, data, "password");
    _run_mainloop ();

    fail_if(result == NULL);    
    fail_if(ui_action != NULL);
    fail_if(error != NULL);
    fail_if(g_strcmp0(
        gsignond_session_data_get_username(result), "megauser") != 0);
    fail_if(g_strcmp0(
        gsignond_session_data_get_secret(result), "megapassword") != 0);
    gsignond_dictionary_unref(result);
    result = NULL;
    
    //username and password empty
    gsignond_dictionary_unref(data);
    data = gsignond_dictionary_new();
    gsignond_plugin_request_initial(plugin, data, "password");
    _run_mainloop ();

    fail_if(result != NULL);    
    fail_if(ui_action == NULL);
    fail_if(error != NULL);
    fail_if(gsignond_signonui_data_get_query_username(ui_action, &bool_res)
            == FALSE);
    fail_if(bool_res == FALSE);
    fail_if(gsignond_signonui_data_get_query_password(ui_action, &bool_res)
            == FALSE);
    fail_if(bool_res == FALSE);    
    gsignond_dictionary_unref(ui_action);
    ui_action = NULL;
    
    //username not empty, password empty
    gsignond_session_data_set_username(data, "megauser");
    gsignond_plugin_request_initial(plugin, data, "password");
    _run_mainloop ();

    fail_if(result != NULL);    
    fail_if(ui_action == NULL);
    fail_if(error != NULL);
    fail_if(gsignond_signonui_data_get_query_username(ui_action, &bool_res)
            == FALSE);
    fail_if(bool_res == TRUE);
    fail_if(gsignond_signonui_data_get_query_password(ui_action, &bool_res)
            == FALSE);
    fail_if(bool_res == FALSE);    
    gsignond_dictionary_unref(ui_action);
    ui_action = NULL;
    
    gsignond_dictionary_unref(data);

    g_object_unref(config);
    g_object_unref(plugin);
}
END_TEST

START_TEST (test_pluginremote_user_action_finished)
{
    DBG ("");
    GSignondPlugin *plugin = NULL;
    const gchar *plugin_type = "password";

    GSignondConfig* config = gsignond_config_new ();
    fail_if(config == NULL);
    
    plugin = GSIGNOND_PLUGIN (gsignond_plugin_remote_new(config, plugin_type));
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GSignondSignonuiData* ui_action = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "response-final", G_CALLBACK(response_callback),
            &result);
    g_signal_connect(plugin, "user-action-required", 
                     G_CALLBACK(user_action_required_callback), &ui_action);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSignonuiData* data = gsignond_dictionary_new();
    
    //empty data
    gsignond_plugin_user_action_finished(plugin, data);
    _run_mainloop ();

    fail_if(result != NULL);    
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless (error->code == GSIGNOND_ERROR_USER_INTERACTION);
    g_error_free(error);
    error = NULL;
    
    // correct values
    gsignond_signonui_data_set_username(data, "megauser");
    gsignond_signonui_data_set_password(data, "megapassword");
    gsignond_signonui_data_set_query_error(data, SIGNONUI_ERROR_NONE);
    gsignond_plugin_user_action_finished(plugin, data);
    _run_mainloop ();

    fail_if(result == NULL);    
    fail_if(ui_action != NULL);
    fail_if(error != NULL);
    fail_if(g_strcmp0(
        gsignond_session_data_get_username(result), "megauser") != 0);
    fail_if(g_strcmp0(
        gsignond_session_data_get_secret(result), "megapassword") != 0);
    gsignond_dictionary_unref(result);
    result = NULL;

    // user canceled
    gsignond_signonui_data_set_query_error(data, SIGNONUI_ERROR_CANCELED);
    gsignond_plugin_user_action_finished(plugin, data);
    _run_mainloop ();

    fail_if(result != NULL);    
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless (error->code == GSIGNOND_ERROR_SESSION_CANCELED);
    g_error_free(error);
    error = NULL;

    // error in ui request
    gsignond_signonui_data_set_query_error(data, SIGNONUI_ERROR_GENERAL);
    gsignond_plugin_user_action_finished(plugin, data);
    _run_mainloop ();

    fail_if(result != NULL);    
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless (error->code == GSIGNOND_ERROR_USER_INTERACTION);
    g_error_free(error);
    error = NULL;
    
    gsignond_dictionary_unref(data);

    g_object_unref(config);
    g_object_unref(plugin);
}
END_TEST

START_TEST (test_pluginremote_refresh)
{
    DBG ("");
    GSignondPlugin *plugin = NULL;
    const gchar *plugin_type = "password";

    GSignondConfig* config = gsignond_config_new ();
    fail_if(config == NULL);
    
    plugin = GSIGNOND_PLUGIN (gsignond_plugin_remote_new(config, plugin_type));
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "refreshed", G_CALLBACK(response_callback),
            &result);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSessionData* data = gsignond_dictionary_new();
    gsignond_plugin_refresh(plugin, data);
    _run_mainloop ();

    fail_if(result == NULL);    
    fail_if(error != NULL);
    gsignond_dictionary_unref(result);
    result = NULL;
    
    gsignond_dictionary_unref(data);

    g_object_unref(config);
    g_object_unref(plugin);
}
END_TEST

START_TEST (test_plugind_daemon)
{
    DBG ("");
    GSignondPluginDaemon *daemon = NULL;
    const gchar *plugin_type = "password";

    GSignondConfig* config = gsignond_config_new ();
    fail_if(config == NULL);

    gchar *plugin_path = g_module_build_path (gsignond_config_get_string (
                config, GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR), "nonexisting");
    fail_if (plugin_path == NULL);
    daemon = gsignond_plugin_daemon_new (plugin_path, "nonexisting");
    g_free (plugin_path);
    fail_if (daemon != NULL);

    plugin_path = g_module_build_path (gsignond_config_get_string (
            config, GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR), plugin_type);
    fail_if (plugin_path == NULL);
    daemon = gsignond_plugin_daemon_new (plugin_path, plugin_type);
    g_free (plugin_path);
    fail_if (daemon == NULL);
    g_object_unref (daemon);
    daemon = NULL;
    g_object_unref(config);
}
END_TEST

Suite* pluginremote_suite (void)
{
    Suite *s = suite_create ("Plugin remote");
    
    /* Core test case */
    TCase *tc_core = tcase_create ("RemoteTests");
    tcase_add_checked_fixture (tc_core, _setup, _teardown);
    tcase_add_test (tc_core, test_pluginremote_create);
    tcase_add_test (tc_core, test_pluginremote_plugind_create);
    tcase_add_test (tc_core, test_pluginremote_plugind_kill);
    tcase_add_test (tc_core, test_pluginremote_request);
    tcase_add_test (tc_core, test_pluginremote_user_action_finished);
    tcase_add_test (tc_core, test_pluginremote_refresh);
    tcase_add_test (tc_core, test_plugind_daemon);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;
    
    g_type_init();
    
    Suite *s = pluginremote_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  
