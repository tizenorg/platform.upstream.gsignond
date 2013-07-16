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
#include <string.h>

#include "gsignond-plugin-proxy.h"
#include "gsignond-plugin-proxy-factory.h"
#include "gsignond-plugin-remote.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-log.h"
#include "common/gsignond-plugin-loader.h"

static GMainLoop *main_loop = NULL;

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
#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif
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
gsignond_auth_session_init (
        GSignondAuthSession *self)
{
}

static void
gsignond_auth_session_class_init (
        GSignondAuthSessionClass *klass)
{
//    GObjectClass *object_class = G_OBJECT_CLASS (klass);
}

gboolean testing_proxy_process = FALSE;
gboolean testing_proxy_process_cancel = FALSE;
gboolean testing_proxy_process_queue = FALSE;
gint proxy_process_queue_results = 0;
gboolean testing_proxy_process_queue_cancel = FALSE;
gint proxy_process_queue_cancel_results = 0;
gboolean testing_proxy_process_cancel_triggered = FALSE;

void
gsignond_auth_session_notify_process_result (
        GSignondAuthSession *iface,
        GSignondSessionData *result,
        gpointer user_data)
{
    int i;

    DBG ("");

    if (testing_proxy_process) {
        testing_proxy_process = FALSE;
        fail_if(g_strcmp0(
            gsignond_session_data_get_username(result), "megauser") != 0);
        fail_if(g_strcmp0(
            gsignond_session_data_get_secret(result), "megapassword") != 0);
        _stop_mainloop ();

    } else if (testing_proxy_process_queue) {
        proxy_process_queue_results++;
        if (proxy_process_queue_results == 1) {
            GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
            GSignondSessionData* data = gsignond_dictionary_new();
            fail_if(data == NULL);
            gsignond_session_data_set_username(data, "megauser");
            gsignond_session_data_set_secret(data, "megapassword");

            gsignond_plugin_proxy_process(proxy, iface, data, NULL, "password",
                    proxy);

            gsignond_plugin_proxy_process(proxy, iface, data, NULL, "password",
                    proxy);
    
            gsignond_dictionary_unref(data);
        }
        if (proxy_process_queue_results == 3) {
            testing_proxy_process_queue = FALSE;
            _stop_mainloop ();
        }
    } else if (testing_proxy_process_queue_cancel) {
        proxy_process_queue_cancel_results++;
        if (proxy_process_queue_cancel_results == 1) {
            GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
            GSignondSessionData* data = gsignond_dictionary_new();
            fail_if(data == NULL);

            for (i = 0; i < 9; i++) {
                gsignond_plugin_proxy_process(proxy, iface, data, NULL, "mech1",
                        proxy);
            }
            gsignond_dictionary_unref(data);
        }
        if (proxy_process_queue_cancel_results == 10) {
            testing_proxy_process_queue_cancel = FALSE;
            _stop_mainloop ();
        }
    } else 
        fail_if(TRUE);    
}

void
gsignond_auth_session_notify_process_error (
        GSignondAuthSession *iface,
        const GError *error,
        gpointer user_data)
{
    DBG ("");

    if (testing_proxy_process_cancel) {
        fail_if(error->code != GSIGNOND_ERROR_SESSION_CANCELED);
        testing_proxy_process_cancel = FALSE;
        _stop_mainloop ();
    } else if (testing_proxy_process_queue_cancel) {
        fail_if(error->code != GSIGNOND_ERROR_SESSION_CANCELED);
        proxy_process_queue_cancel_results++;
    }

}

void 
gsignond_auth_session_notify_store (
        GSignondAuthSession *self,
        GSignondSessionData *session_data)
{
    DBG ("");
    fail_if(TRUE);
}

void 
gsignond_auth_session_notify_user_action_required (
        GSignondAuthSession *self,
        GSignondSignonuiData *session_data)
{
    DBG ("");
    fail_if(TRUE);
}

void 
gsignond_auth_session_notify_refreshed (
        GSignondAuthSession *self,
        GSignondSignonuiData *session_data)
{
    DBG ("");
    fail_if(TRUE);
}

void 
gsignond_auth_session_notify_state_changed (
        GSignondAuthSession *self,
        gint state,
        const gchar *message,
        gpointer user_data)
{
    if (testing_proxy_process_cancel &&
            !testing_proxy_process_cancel_triggered &&
            state == GSIGNOND_PLUGIN_STATE_WAITING) {
        GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
        gsignond_plugin_proxy_cancel(proxy, self);
        testing_proxy_process_cancel_triggered = TRUE;
    } else if (testing_proxy_process_queue_cancel &&
            state == GSIGNOND_PLUGIN_STATE_WAITING &&
            proxy_process_queue_cancel_results == 5) {
        GSignondPluginProxy* proxy = GSIGNOND_PLUGIN_PROXY(user_data);
        gsignond_plugin_proxy_cancel(proxy, self);
    }
}

static void
check_plugin_proxy(
        GSignondPluginProxy* proxy,
        gchar *type,
        gchar **mechanisms)
{
    gchar* ptype = NULL;
    gchar** pmechanisms = NULL;
    gint i = 0;

    fail_if(proxy == NULL);

    g_object_get(proxy, "type", &ptype, "mechanisms", &pmechanisms, NULL);
    fail_unless(g_strcmp0(ptype, type) == 0);
    g_free(ptype);

    guint len = g_strv_length (pmechanisms);
    fail_unless (len == g_strv_length (mechanisms));

    for (i=0; i<len; i++) {
        fail_unless(g_strcmp0(pmechanisms[i], mechanisms[i]) == 0);
    }
    if (pmechanisms) {
        g_strfreev(pmechanisms);
    }
}

START_TEST (test_pluginproxy_create)
{
    DBG("test_pluginproxy_create\n");

    gchar *pass_mechs[] = {"password", NULL};

    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);

    GSignondPluginProxy* proxy2 = gsignond_plugin_proxy_new(config,
            "absentplugin");
    fail_if (proxy2 != NULL);

    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    check_plugin_proxy(proxy, "password", pass_mechs);

    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxy_process)
{
    DBG("test_pluginproxy_process\n");

    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);
    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");
    
    GSignondAuthSession* test_auth_session =
            g_object_new(gsignond_auth_session_get_type(), NULL);

    testing_proxy_process = TRUE;

    gsignond_plugin_proxy_process(proxy, test_auth_session, data, NULL, "password",
            proxy);

    _run_mainloop ();

    fail_if(testing_proxy_process);
    
    gsignond_dictionary_unref(data);
    g_object_unref(test_auth_session);
    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxy_process_cancel)
{
    DBG("test_pluginproxy_process_cancel\n");

    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "ssotest");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);

    GSignondAuthSession* test_auth_session = g_object_new(
            gsignond_auth_session_get_type(), NULL);
    
    testing_proxy_process_cancel = TRUE;
    
    gsignond_plugin_proxy_process(proxy, test_auth_session, data, NULL, "mech1",
            proxy);

    _run_mainloop ();

    fail_if(testing_proxy_process_cancel);
    
    gsignond_dictionary_unref(data);
    g_object_unref(test_auth_session);
    g_object_unref(proxy);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxy_process_queue)
{
    DBG("test_pluginproxy_process_queue\n");

    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);
    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");

    GSignondAuthSession* test_auth_session = g_object_new(
            gsignond_auth_session_get_type(), NULL);
    
    testing_proxy_process_queue = TRUE;
    
    gsignond_plugin_proxy_process(proxy, test_auth_session, data, NULL, "password",
            proxy);
    _run_mainloop ();

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
    
    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "ssotest");
    fail_if (proxy == NULL);
    
    GSignondSessionData* data = gsignond_dictionary_new();
    fail_if(data == NULL);

    GSignondAuthSession* test_auth_session = g_object_new(
            gsignond_auth_session_get_type(), NULL);
    
    testing_proxy_process_queue_cancel = TRUE;
    
    gsignond_plugin_proxy_process(proxy, test_auth_session, data, NULL, "mech1",
            proxy);

    _run_mainloop ();

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
    DBG("");
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxyFactory* factory = gsignond_plugin_proxy_factory_new(
            config);
    fail_if(factory == NULL);
    const gchar *pass_method = NULL;
    const gchar** pmethods = NULL;
    
    const gchar** methods = gsignond_plugin_proxy_factory_get_plugin_types(
            factory);
    fail_if(methods == NULL);
    pmethods = methods;
    while (pmethods[0] != NULL) {
        DBG ("Method %s", pmethods[0]);
        if (g_strcmp0 (pmethods[0], "password") == 0) {
            pass_method = pmethods[0];
        }
        pmethods++;
    }
    const gchar** mechanisms =
            gsignond_plugin_proxy_factory_get_plugin_mechanisms(factory,
                    pass_method);
    fail_if(mechanisms == NULL);
    fail_if(strcmp(mechanisms[0], "password") != 0);
    fail_if(mechanisms[1] != NULL);
    
    g_object_unref(factory);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxyfactory_get)
{
    DBG("");
    gchar *pass_mechs[] = {"password", NULL};
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxyFactory* factory = gsignond_plugin_proxy_factory_new(
            config);
    fail_if(factory == NULL);
    
    fail_if(gsignond_plugin_proxy_factory_get_plugin(factory, "absentplugin")
            != NULL);

    GSignondPluginProxy* proxy1 = gsignond_plugin_proxy_factory_get_plugin(
        factory, "password");
    GSignondPluginProxy* proxy2 = gsignond_plugin_proxy_factory_get_plugin(
        factory, "password");
    GSignondPluginProxy* proxy3 = gsignond_plugin_proxy_factory_get_plugin(
        factory, "password");
    fail_if(proxy1 == NULL || proxy2 == NULL || proxy3 == NULL);
    fail_if(proxy1 != proxy3 || proxy1 != proxy2);
    check_plugin_proxy(proxy1, "password", pass_mechs);

    g_object_unref(proxy1);
    g_object_unref(proxy2);
    g_object_unref(proxy3);    
    
    g_object_unref(factory);
    g_object_unref(config);
}
END_TEST

START_TEST (test_pluginproxyfactory_add)
{
    DBG("");
    GSignondConfig* config = gsignond_config_new();
    fail_if(config == NULL);
    
    GSignondPluginProxyFactory* factory = gsignond_plugin_proxy_factory_new(
            config);
    fail_if(factory == NULL);

    GSignondPluginProxy* proxy = gsignond_plugin_proxy_new(config, "password");
    fail_if (proxy == NULL);
    fail_if(gsignond_plugin_proxy_factory_add_plugin(factory, proxy) == FALSE);
    fail_if(gsignond_plugin_proxy_factory_add_plugin(factory, proxy) == TRUE);
    fail_if(gsignond_plugin_proxy_factory_get_plugin(factory, "password")
            != proxy);

    g_object_unref(proxy);
    g_object_unref(factory);
    g_object_unref(config);
}
END_TEST

typedef struct {
    GSignondPluginProxyFactory *factory;
    GSignondPluginProxy *proxy;
} ProxyTimeoutData;

static gboolean
_validate_new_proxy(gpointer userdata)
{
    ProxyTimeoutData *data = (ProxyTimeoutData *)userdata;
    fail_if (data == NULL);

    GSignondPluginProxy *proxy = gsignond_plugin_proxy_factory_get_plugin (data->factory, "ssotest");
    fail_if (proxy == NULL);

    fail_if (proxy == data->proxy, "expected new proxy object, but got cached object");
    g_object_unref(proxy);

    g_free (userdata);

    _stop_mainloop();

    return FALSE;
}

static gboolean
_validate_cached_proxy (gpointer userdata)
{
    ProxyTimeoutData *data = (ProxyTimeoutData *)userdata;
    fail_if (data == NULL);
    
    GSignondPluginProxy *proxy = gsignond_plugin_proxy_factory_get_plugin (data->factory, "ssotest");
    fail_if (proxy == NULL);

    fail_unless (proxy == data->proxy, "expected cached proxy object, but got new object");

    g_object_unref (proxy);

    g_timeout_add (2200, _validate_new_proxy, userdata);

    return FALSE;
}

START_TEST (test_pluginproxyfactory_proxy_timeout)
{
    DBG("test_pluginproxyfactory_proxy_timeout\n");
    GSignondPluginProxyFactory *factory = NULL;
    GSignondPluginProxy *proxy1 = NULL, *proxy2 = NULL;
    GSignondConfig *config = NULL;

    /* CASE 1: proxy timeout disabled */
    g_setenv ("SSO_PLUGIN_TIMEOUT", "0", TRUE);
  
    config = gsignond_config_new();
    fail_if(config == NULL);

    factory = gsignond_plugin_proxy_factory_new ( config);
    fail_if (factory == NULL);

    proxy1 = gsignond_plugin_proxy_factory_get_plugin (factory, "ssotest");
    fail_if (proxy1 == NULL);
    g_object_unref (proxy1);

    proxy2 = gsignond_plugin_proxy_factory_get_plugin (factory, "ssotest");
    fail_if (proxy2 == NULL);

    fail_unless (proxy1 == proxy2, "got new plugin proxy object, "
                                   "where expected cached object(%p,%p)",
                                    proxy1, proxy2);
    g_object_unref (proxy2);

    g_object_unref (config);
    g_object_unref (factory);

    /* CASE 2: proxy timeout enbled */ 
    g_setenv ("SSO_PLUGIN_TIMEOUT", "1", TRUE);

    config = gsignond_config_new();
    fail_if(config == NULL);

    factory = gsignond_plugin_proxy_factory_new (config);
    fail_if (factory == NULL);

    proxy1 = gsignond_plugin_proxy_factory_get_plugin (factory, "ssotest");
    fail_if (proxy1 == NULL);
    g_object_unref (proxy1);

    ProxyTimeoutData *data = g_new0(ProxyTimeoutData, 1);
    data->factory = factory;
    data->proxy = proxy1;
    g_timeout_add_seconds (2, _validate_new_proxy, (gpointer)data);

    _run_mainloop ();

    g_object_unref(config);
    g_object_unref(factory);

    /* CASE 3: proxy timeout enable - request recently closed plugin */
    g_setenv ("SSO_PLUGIN_TIMEOUT", "2", TRUE);
    config = gsignond_config_new ();
    fail_if (config == NULL);

    factory = gsignond_plugin_proxy_factory_new(config);
    fail_if (factory == NULL);

    proxy1 = gsignond_plugin_proxy_factory_get_plugin(factory, "ssotest");
    fail_if (proxy1 == NULL);
    g_object_unref (proxy1);

    ProxyTimeoutData *data1 = g_new0(ProxyTimeoutData, 1);
    data1->factory = factory;
    data1->proxy = proxy1;

    g_timeout_add_seconds (1, _validate_cached_proxy, (gpointer)data1);

    _run_mainloop();

    g_object_unref(config);
    g_object_unref(factory);
}
END_TEST

Suite* pluginproxy_suite (void)
{
    Suite *s = suite_create ("Plugin proxy");
    
    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    tcase_add_checked_fixture (tc_core, _setup, _teardown);

    tcase_set_timeout (tc_core, 10);
    tcase_add_test (tc_core, test_pluginproxy_create);
    tcase_add_test (tc_core, test_pluginproxy_process);
    tcase_add_test (tc_core, test_pluginproxy_process_cancel);
    tcase_add_test (tc_core, test_pluginproxy_process_queue);
    tcase_add_test (tc_core, test_pluginproxy_process_queue_cancel);
    tcase_add_test (tc_core, test_pluginproxyfactory_methods_and_mechanisms);
    tcase_add_test (tc_core, test_pluginproxyfactory_get);
    tcase_add_test (tc_core, test_pluginproxyfactory_add);
    tcase_add_test (tc_core, test_pluginproxyfactory_proxy_timeout);

    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;
    
    Suite *s = pluginproxy_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  

