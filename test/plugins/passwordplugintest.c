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
#include "gsignond-password-plugin.h"
#include "gsignond/gsignond-session-data.h"
#include "gsignond/gsignond-signonui-data.h"
#include "gsignond/gsignond-plugin-interface.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-config.h"


static void check_plugin(GSignondPlugin* plugin)
{
    gchar* type = NULL;
    gchar** mechanisms = NULL;

    fail_if(plugin == NULL);
    
    g_object_get(plugin, "type", &type, "mechanisms", &mechanisms, NULL);
    
    fail_if(type == NULL || mechanisms == NULL);
    fail_unless(g_strcmp0(type, "password") == 0);
    fail_unless(g_strcmp0(mechanisms[0], "password") == 0);
    fail_unless(mechanisms[1] == NULL);
    
    g_free(type);
    g_strfreev(mechanisms);
}

START_TEST (test_passwordplugin_create)
{
    gpointer plugin;
    
    plugin = g_object_new(GSIGNOND_TYPE_PASSWORD_PLUGIN, NULL);
    check_plugin(plugin);
    g_object_unref(plugin);
}
END_TEST

static void response_callback(GSignondPlugin* plugin, GSignondSessionData* result,
                     gpointer user_data)
{
    GSignondSessionData** user_data_p = user_data;
    *user_data_p = gsignond_dictionary_copy(result);
}

static void user_action_required_callback(GSignondPlugin* plugin, 
                                          GSignondSignonuiData* ui_request, 
                                          gpointer user_data)
{
    GSignondSignonuiData** user_data_p = user_data;
    *user_data_p = gsignond_dictionary_copy(ui_request);
}

static void error_callback(GSignondPlugin* plugin, GError* error,
                     gpointer user_data)
{
    GError** user_data_p = user_data;
    *user_data_p = g_error_copy(error);
}


START_TEST (test_passwordplugin_request)
{
    gpointer plugin;
    
    plugin = g_object_new(GSIGNOND_TYPE_PASSWORD_PLUGIN, NULL);
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GSignondSignonuiData* ui_action = NULL;
    GError* error = NULL;
    gboolean bool_res;

    g_signal_connect(plugin, "response-final", G_CALLBACK(response_callback), &result);
    g_signal_connect(plugin, "user-action-required", 
                     G_CALLBACK(user_action_required_callback), &ui_action);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSessionData* data = gsignond_dictionary_new();

    // username empty, password not empty
    gsignond_session_data_set_secret(data, "megapassword");
    gsignond_plugin_request_initial(plugin, data, NULL, "password");
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
    gsignond_plugin_request_initial(plugin, data, NULL, "password");
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
    gsignond_plugin_request_initial(plugin, data, NULL, "password");
    fail_if(result != NULL);    
    fail_if(ui_action == NULL);
    fail_if(error != NULL);
    fail_if(gsignond_signonui_data_get_query_username(ui_action, &bool_res) == FALSE);
    fail_if(bool_res == FALSE);
    fail_if(gsignond_signonui_data_get_query_password(ui_action, &bool_res) == FALSE);
    fail_if(bool_res == FALSE);    
    gsignond_dictionary_unref(ui_action);
    ui_action = NULL;
    
    //username not empty, password empty
    gsignond_session_data_set_username(data, "megauser");
    gsignond_plugin_request_initial(plugin, data, NULL, "password");
    fail_if(result != NULL);    
    fail_if(ui_action == NULL);
    fail_if(error != NULL);
    fail_if(gsignond_signonui_data_get_query_username(ui_action, &bool_res) == FALSE);
    fail_if(bool_res == TRUE);
    fail_if(gsignond_signonui_data_get_query_password(ui_action, &bool_res) == FALSE);
    fail_if(bool_res == FALSE);    
    gsignond_dictionary_unref(ui_action);
    ui_action = NULL;
    
    gsignond_dictionary_unref(data);
    g_object_unref(plugin);
}
END_TEST

START_TEST (test_passwordplugin_user_action_finished)
{
    gpointer plugin;
    
    plugin = g_object_new(GSIGNOND_TYPE_PASSWORD_PLUGIN, NULL);
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GSignondSignonuiData* ui_action = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "response-final", G_CALLBACK(response_callback), &result);
    g_signal_connect(plugin, "user-action-required", 
                     G_CALLBACK(user_action_required_callback), &ui_action);
    g_signal_connect(plugin, "error", G_CALLBACK(error_callback), &error);

    GSignondSignonuiData* data = gsignond_dictionary_new();
    
    //empty data
    gsignond_plugin_user_action_finished(plugin, data);
    fail_if(result != NULL);    
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_USER_INTERACTION));
    g_error_free(error);
    error = NULL;
    
    // correct values
    gsignond_signonui_data_set_username(data, "megauser");
    gsignond_signonui_data_set_password(data, "megapassword");
    gsignond_signonui_data_set_query_error(data, SIGNONUI_ERROR_NONE);
    gsignond_plugin_user_action_finished(plugin, data);
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
    fail_if(result != NULL);    
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_SESSION_CANCELED));
    g_error_free(error);
    error = NULL;

    // error in ui request
    gsignond_signonui_data_set_query_error(data, SIGNONUI_ERROR_GENERAL);
    gsignond_plugin_user_action_finished(plugin, data);
    fail_if(result != NULL);    
    fail_if(ui_action != NULL);
    fail_if(error == NULL);
    fail_unless(g_error_matches(error, GSIGNOND_ERROR, 
                                GSIGNOND_ERROR_USER_INTERACTION));
    g_error_free(error);
    error = NULL;
    
    gsignond_dictionary_unref(data);
    g_object_unref(plugin);
}
END_TEST

START_TEST (test_passwordplugin_refresh)
{
    gpointer plugin;
    
    plugin = g_object_new(GSIGNOND_TYPE_PASSWORD_PLUGIN, NULL);
    fail_if(plugin == NULL);

    GSignondSessionData* result = NULL;
    GError* error = NULL;

    g_signal_connect(plugin, "refreshed", G_CALLBACK(response_callback), &result);
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

Suite* passwordplugin_suite (void)
{
    Suite *s = suite_create ("Password plugin");
    
    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_passwordplugin_create);
    tcase_add_test (tc_core, test_passwordplugin_request);
    tcase_add_test (tc_core, test_passwordplugin_user_action_finished);
    tcase_add_test (tc_core, test_passwordplugin_refresh);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;

#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    Suite *s = passwordplugin_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  
