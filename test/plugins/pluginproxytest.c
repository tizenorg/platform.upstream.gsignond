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
#include <glib-object.h>
#include "gsignond-plugin-proxy.h"
#include "gsignond-plugin-proxy-factory.h"
#include <gsignond/gsignond-plugin-loader.h>
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-log.h>

static void gsignond_auth_session_iface_init (gpointer g_iface);

typedef struct _GSignondTestAuthSession GSignondTestAuthSession;
typedef struct _GSignondTestAuthSessionClass GSignondTestAuthSessionClass;

struct _GSignondTestAuthSession
{
    GObject parent;

};

struct _GSignondTestAuthSessionClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (GSignondTestAuthSession, gsignond_test_auth_session,
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_AUTH_SESSION_IFACE,
                                               gsignond_auth_session_iface_init));

static void
gsignond_test_auth_session_init (GSignondTestAuthSession *self)
{

    
}

static void
gsignond_test_auth_session_class_init (GSignondTestAuthSessionClass *klass)
{
//    GObjectClass *object_class = G_OBJECT_CLASS (klass);

 
}

static void check_plugin(GSignondPlugin* plugin)
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



START_TEST (test_plugin_loader)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);

    GSignondPlugin* absent_plugin = gsignond_load_plugin(config, "absentplugin");
    fail_if(absent_plugin != NULL);
    
    GSignondPlugin* plugin = gsignond_load_plugin(config, "password");
    check_plugin(plugin);    
    
    g_object_unref(plugin);
    g_object_unref(config);
}
END_TEST

static void check_plugin_proxy(GSignondPluginProxy* proxy)
{
    gchar* type;
    gchar** mechanisms;

    fail_if(proxy == NULL);
    
    g_object_get(proxy, "type", &type, "mechanisms", &mechanisms, NULL);
    
    fail_unless(g_strcmp0(type, "password") == 0);
    fail_unless(g_strcmp0(mechanisms[0], "password") == 0);
    fail_unless(mechanisms[1] == NULL);
    
    g_free(type);
    g_strfreev(mechanisms);
}

START_TEST (test_pluginproxy_create)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    check_plugin_proxy(proxy);

    GSignondPluginProxy* proxy2 = gsignond_plugin_proxy_new(config, "absentplugin");
    fail_if (proxy2 != NULL);

    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

gboolean testing_proxy_process = FALSE;
gboolean testing_proxy_process_cancel = FALSE;
gboolean testing_proxy_process_queue = FALSE;
gint proxy_process_queue_results = 0;
gboolean testing_proxy_process_queue_cancel = FALSE;
gint proxy_process_queue_cancel_results = 0;

void
_on_process_result (GSignondAuthSessionIface *iface,
                    GSignondSessionData *result,
                    gpointer user_data
                   )
{
    int i;
    
    if (testing_proxy_process) {
        testing_proxy_process = FALSE;
        fail_if(g_strcmp0(
            gsignond_session_data_get_username(result), "megauser") != 0);
        fail_if(g_strcmp0(
            gsignond_session_data_get_secret(result), "megapassword") != 0);
    } else if (testing_proxy_process_cancel) {
        GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
        gsignond_plugin_proxy_cancel(proxy, iface);
    } else if (testing_proxy_process_queue) {
        proxy_process_queue_results++;
        if (proxy_process_queue_results == 1) {
            GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
            GSignondSessionData* data = gsignond_dictionary_new();
            fail_if(data == NULL);
            gsignond_session_data_set_username(data, "megauser");
            gsignond_session_data_set_secret(data, "megapassword");

            gsignond_plugin_proxy_process(proxy, iface, data, "password");
            gsignond_plugin_proxy_process(proxy, iface, data, "password");
    
            gsignond_dictionary_unref(data);
        }
        if (proxy_process_queue_results == 3) {
            testing_proxy_process_queue = FALSE;
        }
    } else if (testing_proxy_process_queue_cancel) {
        proxy_process_queue_cancel_results++;
        if (proxy_process_queue_cancel_results == 1) {
            GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
            GSignondSessionData* data = gsignond_dictionary_new();
            fail_if(data == NULL);
            gsignond_session_data_set_username(data, "megauser");
            gsignond_session_data_set_secret(data, "megapassword");

            for (i = 0; i < 9; i++)
                gsignond_plugin_proxy_process(proxy, iface, data, "password");
    
            gsignond_dictionary_unref(data);
        }
        if (proxy_process_queue_cancel_results == 5) {
            GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
            gsignond_plugin_proxy_cancel(proxy, iface);
        }
        if (proxy_process_queue_cancel_results == 10) {
            testing_proxy_process_queue_cancel = FALSE;
        }
    } else 
        fail_if(TRUE);    
}

void
_on_process_error (
                                                GSignondAuthSessionIface *iface,
                                                const GError *error,
                                                gpointer user_data
                  )
{
    if (testing_proxy_process_cancel) {
        fail_if(error->code != GSIGNOND_ERROR_WRONG_STATE);
        testing_proxy_process_cancel = FALSE;
    } else if (testing_proxy_process_queue_cancel) {
        fail_if(error->code != GSIGNOND_ERROR_WRONG_STATE);
    } else
        fail_if(TRUE);
}

void 
_on_process_store (GSignondAuthSessionIface *self, 
                            GSignondSessionData *session_data, gpointer user_data)
{
    fail_if(TRUE);
}

void 
_on_process_user_action_required (GSignondAuthSessionIface *self, 
                                           GSignondSessionData *session_data,
                                           gpointer user_data
                                 )
{
    fail_if(TRUE);
}

void 
_on_process_refreshed (GSignondAuthSessionIface *self, 
                                GSignondSessionData *session_data,
                                gpointer user_data
                                             )
{
    fail_if(TRUE);
}

void 
_on_state_changed (GSignondAuthSessionIface *self, 
                                     gint state, 
                                     const gchar *message,
                                     gpointer user_data
                  )
{
    INFO("AuthSession state changed %d %s", state, message);
}

static void
gsignond_auth_session_iface_init (gpointer g_iface)
{
/*    GSignondAuthSessionIfaceInterface *auth_session_iface =
        (GSignondAuthSessionIfaceInterface *) g_iface;

    auth_session_iface->process = _process;
    auth_session_iface->query_available_mechanisms = _query_available_mechanisms;
    auth_session_iface->cancel = _cancel;
    auth_session_iface->user_action_finished = _user_action_finished;
    auth_session_iface->refresh = _refresh;
    auth_session_iface->get_acm = _get_acm;*/
}


START_TEST (test_pluginproxy_process)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);
    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");
    
    GSignondTestAuthSession* test_auth_session = g_object_new(gsignond_test_auth_session_get_type(), NULL);

    testing_proxy_process = TRUE;
    g_signal_connect (test_auth_session, "process-error", G_CALLBACK(_on_process_error), NULL);
    g_signal_connect (test_auth_session, "process-result", G_CALLBACK(_on_process_result), proxy);
    g_signal_connect (test_auth_session, "process-store", G_CALLBACK(_on_process_store), NULL);
    g_signal_connect (test_auth_session, "process-user-action-required", 
                      G_CALLBACK(_on_process_user_action_required), NULL);
    g_signal_connect (test_auth_session, "process-refreshed", G_CALLBACK(_on_process_refreshed), NULL);
    g_signal_connect (test_auth_session, "state-changed", G_CALLBACK(_on_state_changed), NULL);

    gsignond_plugin_proxy_process(proxy, GSIGNOND_AUTH_SESSION_IFACE(test_auth_session), data, "password");
    fail_if(testing_proxy_process);
    
    gsignond_dictionary_unref(data);
    g_object_unref(test_auth_session);
    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxy_process_cancel)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);
    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");

    GSignondTestAuthSession* test_auth_session = g_object_new(gsignond_test_auth_session_get_type(), NULL);
    
    testing_proxy_process_cancel = TRUE;
    g_signal_connect (test_auth_session, "process-error", G_CALLBACK(_on_process_error), NULL);
    g_signal_connect (test_auth_session, "process-result", G_CALLBACK(_on_process_result), proxy);
    g_signal_connect (test_auth_session, "process-store", G_CALLBACK(_on_process_store), NULL);
    g_signal_connect (test_auth_session, "process-user-action-required", 
                      G_CALLBACK(_on_process_user_action_required), NULL);
    g_signal_connect (test_auth_session, "process-refreshed", G_CALLBACK(_on_process_refreshed), NULL);
    g_signal_connect (test_auth_session, "state-changed", G_CALLBACK(_on_state_changed), NULL);
    
    gsignond_plugin_proxy_process(proxy, GSIGNOND_AUTH_SESSION_IFACE(test_auth_session), data, "password");
    fail_if(testing_proxy_process_cancel);
    
    gsignond_dictionary_unref(data);
    g_object_unref(test_auth_session);
    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxy_process_queue)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);
    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");

    GSignondTestAuthSession* test_auth_session = g_object_new(gsignond_test_auth_session_get_type(), NULL);
    
    testing_proxy_process_queue = TRUE;
    g_signal_connect (test_auth_session, "process-error", G_CALLBACK(_on_process_error), NULL);
    g_signal_connect (test_auth_session, "process-result", G_CALLBACK(_on_process_result), proxy);
    g_signal_connect (test_auth_session, "process-store", G_CALLBACK(_on_process_store), NULL);
    g_signal_connect (test_auth_session, "process-user-action-required", 
                      G_CALLBACK(_on_process_user_action_required), NULL);
    g_signal_connect (test_auth_session, "process-refreshed", G_CALLBACK(_on_process_refreshed), NULL);
    g_signal_connect (test_auth_session, "state-changed", G_CALLBACK(_on_state_changed), NULL);
    
    gsignond_plugin_proxy_process(proxy, GSIGNOND_AUTH_SESSION_IFACE(test_auth_session), data, "password");
    fail_if(testing_proxy_process_queue);
    fail_if(proxy_process_queue_results < 3);

    gsignond_dictionary_unref(data);
    g_object_unref(test_auth_session);
    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxy_process_queue_cancel)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);
    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");

    GSignondTestAuthSession* test_auth_session = g_object_new(gsignond_test_auth_session_get_type(), NULL);
    
    testing_proxy_process_queue_cancel = TRUE;
    g_signal_connect (test_auth_session, "process-error", G_CALLBACK(_on_process_error), NULL);
    g_signal_connect (test_auth_session, "process-result", G_CALLBACK(_on_process_result), proxy);
    g_signal_connect (test_auth_session, "process-store", G_CALLBACK(_on_process_store), NULL);
    g_signal_connect (test_auth_session, "process-user-action-required", 
                      G_CALLBACK(_on_process_user_action_required), NULL);
    g_signal_connect (test_auth_session, "process-refreshed", G_CALLBACK(_on_process_refreshed), NULL);
    g_signal_connect (test_auth_session, "state-changed", G_CALLBACK(_on_state_changed), NULL);
    
    gsignond_plugin_proxy_process(proxy, GSIGNOND_AUTH_SESSION_IFACE(test_auth_session), data, "password");
    fail_if(testing_proxy_process_queue_cancel);
    fail_if(proxy_process_queue_cancel_results != 10);

    gsignond_dictionary_unref(data);
    g_object_unref(test_auth_session);
    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxyfactory_methods_and_mechanisms)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxyFactory* factory = gsignond_plugin_proxy_factory_new(config);
    fail_if(factory == NULL);
    
    const gchar** methods = gsignond_plugin_proxy_factory_get_plugin_types(factory);
    fail_if(methods == NULL);
    fail_if(strcmp(methods[0], "password") != 0);
    fail_if(methods[1] != NULL);
    
    const gchar** mechanisms = gsignond_plugin_proxy_factory_get_plugin_mechanisms(factory, methods[0]);
    fail_if(mechanisms == NULL);
    fail_if(strcmp(mechanisms[0], "password") != 0);
    fail_if(mechanisms[1] != NULL);
    

    g_object_unref(factory);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxyfactory_get)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxyFactory* factory = gsignond_plugin_proxy_factory_new(config);
    fail_if(factory == NULL);
    
    fail_if(gsignond_plugin_proxy_factory_get_plugin(factory, 123, "absentplugin") != NULL);

    GSignondPluginProxy* proxy1 = gsignond_plugin_proxy_factory_get_plugin(
        factory, 123, "password");
    GSignondPluginProxy* proxy2 = gsignond_plugin_proxy_factory_get_plugin(
        factory, 456, "password");
    GSignondPluginProxy* proxy3 = gsignond_plugin_proxy_factory_get_plugin(
        factory, 123, "password");
    fail_if(proxy1 == NULL || proxy2 == NULL || proxy3 == NULL);
    fail_if(proxy1 != proxy3 || proxy1 == proxy2);
    check_plugin_proxy(proxy1);
    check_plugin_proxy(proxy2);

    g_object_unref(factory);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxyfactory_add)
{
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxyFactory* factory = gsignond_plugin_proxy_factory_new(config);
    fail_if(factory == NULL);

    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    fail_if(gsignond_plugin_proxy_factory_add_plugin(factory, 123, proxy) == FALSE);
    fail_if(gsignond_plugin_proxy_factory_add_plugin(factory, 123, proxy) == TRUE);
    fail_if(gsignond_plugin_proxy_factory_get_plugin(factory, 123, "password") != proxy);

    g_object_unref(proxy);
    g_object_unref(factory);
    g_object_unref(config);
}
END_TEST


Suite* pluginproxy_suite (void)
{
    Suite *s = suite_create ("Plugin proxy");
    
    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_plugin_loader);
    tcase_add_test (tc_core, test_pluginproxy_create);
    tcase_add_test (tc_core, test_pluginproxy_process);
    tcase_add_test (tc_core, test_pluginproxy_process_cancel);
    //FIXME: we need an asynchronous or remote testing plugin to really test 
    // cancellation and queueuing. Password plugin is totally synchronous.
    tcase_add_test (tc_core, test_pluginproxy_process_queue);
    tcase_add_test (tc_core, test_pluginproxy_process_queue_cancel);
    tcase_add_test (tc_core, test_pluginproxyfactory_methods_and_mechanisms);
    tcase_add_test (tc_core, test_pluginproxyfactory_get);
    tcase_add_test (tc_core, test_pluginproxyfactory_add);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;
    
    g_type_init();
    
    Suite *s = pluginproxy_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  

