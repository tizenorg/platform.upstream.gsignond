/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2013 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@intel.com>
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
#include "gsignond-digest-plugin.h"
#include <gsignond/gsignond-session-data.h>
#include <gsignond/gsignond-plugin-interface.h>
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-log.h>
#include <gsignond/gsignond-plugin-loader.h>
#include <gsignond/gsignond-config.h>

static void check_plugin(GSignondPlugin* plugin)
{
    gchar* type;
    gchar** mechanisms;

    fail_if(plugin == NULL);

    g_object_get(plugin, "type", &type, "mechanisms", &mechanisms, NULL);

    fail_unless(g_strcmp0(type, "digest") == 0);
    fail_unless(g_strcmp0(mechanisms[0], "digest") == 0);
    fail_unless(mechanisms[1] == NULL);

    g_free(type);
    g_strfreev(mechanisms);
}

START_TEST (test_digestplugin_create)
{
    gpointer plugin;

    plugin = g_object_new(GSIGNOND_TYPE_DIGEST_PLUGIN, NULL);
    check_plugin(plugin);
    g_object_unref(plugin);
}
END_TEST

static void
response_callback(
        GSignondPlugin* plugin,
        GSignondSessionData* result,
        gpointer user_data)
{
    GSignondSessionData** user_data_p = user_data;
    *user_data_p = gsignond_dictionary_copy(result);
}

static void
user_action_required_callback(
        GSignondPlugin* plugin,
        GSignondSignonuiData* ui_request,
        gpointer user_data)
{
    GSignondSignonuiData** user_data_p = user_data;
    *user_data_p = gsignond_dictionary_copy(ui_request);
    gsignond_signonui_data_set_username(*user_data_p, "user1");
    gsignond_signonui_data_set_password(*user_data_p, "password1");
}

static void
error_callback(
        GSignondPlugin* plugin,
        GError* error,
        gpointer user_data)
{
    GError** user_data_p = user_data;
    *user_data_p = g_error_copy(error);
}


START_TEST (test_digestplugin_request)
{
    gpointer plugin;
    gboolean query_res;

    plugin = g_object_new(GSIGNOND_TYPE_DIGEST_PLUGIN, NULL);
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GSignondSignonuiData* ui_action = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "response-final", G_CALLBACK(response_callback),
            &result);
    g_signal_connect(plugin, "user-action-required",
            G_CALLBACK(user_action_required_callback), &ui_action);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSessionData* data = gsignond_dictionary_new();

    // set only username and password
    gsignond_session_data_set_username(data, "user1");
    gsignond_session_data_set_secret(data, "password1");

    gsignond_plugin_request_initial(plugin, data, "digest");
    fail_if(result != NULL);
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR,
                                GSIGNOND_ERROR_MISSING_DATA));
    g_error_free(error);
    error = NULL;

    // set all the required stuff so that no ui-action is required
    gsignond_session_data_set_realm(data, "realm1");
    gsignond_dictionary_set_string(data, "Algo", "md5-sess");
    gsignond_dictionary_set_string(data, "Nonce",
            "abg10b1234ee1f0e8b11d0f600bfb0c093");
    gsignond_dictionary_set_string(data, "Method", "GET");
    gsignond_dictionary_set_string(data, "DigestUri", "/test/index.html");

    gsignond_plugin_request_initial(plugin, data, "digest");
    fail_if(result == NULL);
    fail_if(ui_action != NULL);
    fail_if(error != NULL);
    fail_if(g_strcmp0(gsignond_session_data_get_username(result),
            "user1") != 0);
    fail_if(gsignond_dictionary_get_string(result, "Response") == NULL);
    fail_if(gsignond_dictionary_get_string(result, "CNonce") == NULL);
    gsignond_dictionary_unref(result);
    result = NULL;

    //remove secret so that ui action is required
    gsignond_dictionary_remove (data, "Secret");
    gsignond_plugin_request_initial(plugin, data, "digest");
    fail_if(result != NULL);
    fail_if(ui_action == NULL);
    fail_if(error != NULL);
    fail_if(g_strcmp0(gsignond_signonui_data_get_username(ui_action),
            "user1") != 0);
    fail_if(g_strcmp0(gsignond_signonui_data_get_password(ui_action),
            "password1") != 0);
    fail_if(gsignond_dictionary_get_string(ui_action, "Realm") == NULL);
    fail_if(gsignond_dictionary_get_string(ui_action, "DigestUri") == NULL);
    gsignond_signonui_data_get_query_username(ui_action, &query_res);
    fail_if(query_res == FALSE);
    gsignond_signonui_data_get_query_password(ui_action, &query_res);
    fail_if(query_res == FALSE);
    gsignond_dictionary_unref(ui_action);
    ui_action = NULL;

    gsignond_dictionary_unref(data);
    g_object_unref(plugin);
}
END_TEST

START_TEST (test_digestplugin_user_action_finished)
{
    gpointer plugin;

    plugin = g_object_new(GSIGNOND_TYPE_DIGEST_PLUGIN, NULL);
    fail_if(plugin == NULL);

    GSignondSessionData *result = NULL, *data = NULL;
    GSignondSignonuiData *ui_action = NULL, *ui_data = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "response-final", G_CALLBACK(response_callback),
            &result);
    g_signal_connect(plugin, "user-action-required",
            G_CALLBACK(user_action_required_callback), &ui_action);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    ui_data = gsignond_signonui_data_new();
    gsignond_signonui_data_set_query_error(ui_data, SIGNONUI_ERROR_NONE);

    //empty data
    gsignond_plugin_user_action_finished(plugin, ui_data);
    fail_if(result != NULL);
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR,
            GSIGNOND_ERROR_USER_INTERACTION));
    g_error_free(error);
    error = NULL;

    // user cancelled
    gsignond_signonui_data_set_query_error(ui_data, SIGNONUI_ERROR_CANCELED);
    gsignond_plugin_user_action_finished(plugin, ui_data);
    fail_if(result != NULL);
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR,
                                GSIGNOND_ERROR_SESSION_CANCELED));
    g_error_free(error);
    error = NULL;

    // error in ui request
    gsignond_signonui_data_set_query_error(ui_data, SIGNONUI_ERROR_GENERAL);
    gsignond_plugin_user_action_finished(plugin, ui_data);
    fail_if(result != NULL);
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR,
                                GSIGNOND_ERROR_USER_INTERACTION));
    g_error_free(error);
    error = NULL;

    // correct values but no session data
    gsignond_signonui_data_set_username (ui_data, "user1");
    gsignond_signonui_data_set_password (ui_data, "password1");
    gsignond_signonui_data_set_query_error (ui_data, SIGNONUI_ERROR_NONE);
    gsignond_plugin_user_action_finished (plugin, ui_data);
    fail_if(result != NULL);
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR,
            GSIGNOND_ERROR_USER_INTERACTION));
    g_error_free(error);
    error = NULL;

    //correct values
    data = gsignond_dictionary_new ();
    gsignond_session_data_set_username (data, "user1");
    gsignond_session_data_set_realm (data, "realm1");
    gsignond_dictionary_set_string (data, "Algo", "md5-sess");
    gsignond_dictionary_set_string (data, "Nonce",
            "abg10b1234ee1f0e8b11d0f600bfb0c093");
    gsignond_dictionary_set_string (data, "Method", "GET");
    gsignond_dictionary_set_string (data, "DigestUri", "/test/index.html");
    gsignond_plugin_request_initial (plugin, data, "digest");
    gsignond_dictionary_unref (data); data = NULL;

    gsignond_plugin_user_action_finished (plugin, ui_data);
    fail_if (result == NULL);
    fail_if (error != NULL);
    fail_if(ui_action == NULL);
    fail_if(g_strcmp0(gsignond_session_data_get_username(result),
            "user1") != 0);
    fail_if(gsignond_dictionary_get_string(result, "Response") == NULL);
    fail_if(gsignond_dictionary_get_string(result, "CNonce") == NULL);
    gsignond_dictionary_unref(result);
    result = NULL;
    gsignond_dictionary_unref(ui_action);
    ui_action = NULL;

    gsignond_dictionary_unref (ui_data);
    g_object_unref (plugin);
}
END_TEST

START_TEST (test_digestplugin_refresh)
{
    gpointer plugin;

    plugin = g_object_new(GSIGNOND_TYPE_DIGEST_PLUGIN, NULL);
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "refreshed", G_CALLBACK(response_callback),
            &result);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSessionData* data = gsignond_dictionary_new();
    gsignond_plugin_refresh(plugin, data);
    fail_if(result == NULL);
    fail_if(error != NULL);
    gsignond_dictionary_unref(result);
    result = NULL;

    gsignond_dictionary_unref(data);
    g_object_unref(plugin);
}
END_TEST

Suite* digestplugin_suite (void)
{
    Suite *s = suite_create ("Digest plugin");

    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_digestplugin_create);
    tcase_add_test (tc_core, test_digestplugin_request);
    tcase_add_test (tc_core, test_digestplugin_user_action_finished);
    tcase_add_test (tc_core, test_digestplugin_refresh);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;

    g_type_init();

    Suite *s = digestplugin_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

