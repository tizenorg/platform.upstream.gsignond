/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Amarnaht Valluri <amarnath.valluri@linux.intel.com>
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

#include "config.h"
#include <check.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>

#include <daemon/dbus/gsignond-dbus.h>
#include <daemon/dbus/gsignond-dbus-auth-service-gen.h>
#include <daemon/dbus/gsignond-dbus-identity-gen.h>
#include <daemon/dbus/gsignond-dbus-auth-session-gen.h>
#include <gsignond/gsignond-identity-info.h>
#include <gsignond/gsignond-log.h>

#ifdef USE_P2P
#  ifdef GSIGNOND_SERVICE
#    undef GSIGNOND_SERVICE
#  endif
#  define GSIGNOND_SERVICE NULL
#endif

struct IdentityData {
    gchar *key;
    gchar *type;
    void *value;
} data[] = {
    { "UserName", "s", "test_user" },
    { "Caption", "s", "test_caption" },
    { "Secret", "s", "test_pass" },
    { "StoreSecret", "b", (void *)TRUE}
};
gchar *exe_name = 0;

#if HAVE_GTESTDBUS
GTestDBus *dbus = NULL;
#else
GPid daemon_pid = 0;
#endif

static void
setup_daemon (void)
{
    fail_if (g_setenv ("G_MESSAGES_DEBUG", "all", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_IDENTITY_TIMEOUT", "60", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_DAEMON_TIMEOUT", "60", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_AUTH_SESSION_TIMEOUT", "60", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_STORAGE_PATH", "/tmp/gsignond", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_SECRET_PATH", "/tmp/gsignond", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_KEYCHAIN_SYSCTX", exe_name, TRUE) == FALSE);
    fail_if (g_setenv ("SSO_PLUGIN_TIMEOUT", "5", TRUE) == FALSE);

    DBG ("Programe name : %s\n", exe_name);

    if (system("rm -rf /tmp/gsignond") != 0) {
        DBG("Failed to clean db path : %s\n", strerror(errno));
    }
#if HAVE_GTESTDBUS
    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    fail_unless (dbus != NULL, "could not create test dbus");

    g_test_dbus_add_service_dir (dbus, GSIGNOND_TEST_DBUS_SERVICE_DIR);

    g_test_dbus_up (dbus);
    DBG ("Test dbus server address : %s\n", g_test_dbus_get_bus_address(dbus));
#else
    GError *error = NULL;
#   ifdef USE_P2P
    /* start daemon maually */
    gchar *argv[2];
    gchar *test_daemon_path = g_build_filename (g_getenv("SSO_BIN_DIR"),
            "gsignond", NULL);
    fail_if (test_daemon_path == NULL, "No SSO daemon path found");

    argv[0] = test_daemon_path;
    argv[1] = NULL;
    g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
            &daemon_pid, &error);
    g_free (test_daemon_path);
    fail_if (error != NULL, "Failed to span daemon : %s",
            error ? error->message : "");
    sleep (5); /* 5 seconds */
#   else
    /* session bus where no GTestBus support */
    GIOChannel *channel = NULL;
    gchar *bus_address = NULL;
    gint tmp_fd = 0;
    gint pipe_fd[2];
    gchar *argv[] = {"dbus-daemon", "--config-file=<<conf-file>>", "--print-address=<<fd>>", NULL};
    gsize len = 0;
    const gchar *dbus_monitor = NULL;

    argv[1] = g_strdup_printf ("--config-file=%s", "gsignond-dbus.conf");

    if (pipe(pipe_fd)== -1) {
        WARN("Failed to open temp file : %s", error->message);
        argv[2] = g_strdup_printf ("--print-address=1");
        g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &daemon_pid, NULL, NULL, &tmp_fd, &error);
    } else {
        tmp_fd = pipe_fd[0];
        argv[2] = g_strdup_printf ("--print-address=%d", pipe_fd[1]);
        g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH|G_SPAWN_LEAVE_DESCRIPTORS_OPEN, NULL, NULL, &daemon_pid, &error);
    }
    fail_if (error != NULL, "Failed to span daemon : %s", error ? error->message : "");
    fail_if (daemon_pid == 0, "Failed to get daemon pid");
    g_free (argv[1]);
    g_free (argv[2]);
    sleep (5); /* 5 seconds */

    channel = g_io_channel_unix_new (tmp_fd);
    g_io_channel_read_line (channel, &bus_address, NULL, &len, &error);
    fail_if (error != NULL, "Failed to daemon address : %s", error ? error->message : "");
    g_io_channel_unref (channel);
    
    if (pipe_fd[0]) close (pipe_fd[0]);
    if (pipe_fd[1]) close (pipe_fd[1]);

    if (bus_address) bus_address[len] = '\0';
    fail_if(bus_address == NULL || strlen(bus_address) == 0);

    if (GSIGNOND_BUS_TYPE == G_BUS_TYPE_SYSTEM)
        fail_if (g_setenv("DBUS_SYSTEM_BUS_ADDRESS", bus_address, TRUE) == FALSE);
    else
        fail_if (g_setenv("DBUS_SESSION_BUS_ADDRESS", bus_address, TRUE) == FALSE);

    DBG ("Daemon Address : %s\n", bus_address);
    g_free (bus_address);

    if ((dbus_monitor = g_getenv("SSO_DBUS_DEBUG")) != NULL && g_strcmp0 (dbus_monitor, "0")) {
    	/* start dbus-monitor */
    	char *argv[] = {"dbus-monitor", "<<bus_type>>", NULL };
        argv[1] = GSIGNOND_BUS_TYPE == G_BUS_TYPE_SYSTEM ? "--system" : "--session" ;
    	g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
    	if (error) {
    		DBG ("Error while running dbus-monitor : %s", error->message);
    		g_error_free (error);
    	}
    }
#   endif

    DBG ("Daemon PID = %d\n", daemon_pid);
#endif
}

static void
teardown_daemon (void)
{
#if HAVE_GTESTDBUS
    g_test_dbus_down (dbus);
#else
    if (daemon_pid) kill (daemon_pid, SIGTERM);
#endif

    g_unsetenv ("SSO_IDENTITY_TIMEOUT");
    g_unsetenv ("SSO_DAEMON_TIMEOUT");
    g_unsetenv ("SSO_AUTH_SESSION_TIMEOUT");
    g_unsetenv ("SSO_STORAGE_PATH");
    g_unsetenv ("SSO_SECRET_PATH");
    g_unsetenv ("SSO_KEYCHAIN_SYSCTX");
}

gboolean _validate_identity_info (GVariant *identity_info)
{
    GSignondIdentityInfo *identity = 0;
    const gchar *username = 0;
    if (!identity_info) return FALSE;

    identity = (GSignondIdentityInfo *)gsignond_dictionary_new_from_variant (identity_info);
    if (!identity) return FALSE;

    username = gsignond_identity_info_get_username (identity);

    gsignond_dictionary_unref (identity);

    if (!username || strcmp (username, "test_user")) return FALSE;

    return TRUE;
}

GVariant * _get_test_identity_data()
{
    GVariantBuilder builder, method_builder;
    const gchar *mechanisms[] = {"mech1","mech2", NULL };
    int i;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);

    for (i=0; i < sizeof(data)/sizeof(struct IdentityData); i++) {
        g_variant_builder_add (&builder, "{sv}", data[i].key, g_variant_new (data[i].type, data[i].value));
    }

    g_variant_builder_init (&method_builder, (const GVariantType *)"a{sas}");
    g_variant_builder_add (&method_builder, "{s^as}", "ssotest", mechanisms);

    g_variant_builder_add (&builder, "{sv}", "AuthMethods", g_variant_builder_end (&method_builder));

    return g_variant_builder_end (&builder);
}

GVariant * _create_identity_info_with_data (const gchar *username,
                                            const gchar *caption, 
                                            gint type, 
                                            const gchar *methods[],
                                            const gchar **mechanisms[])
{
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);

    if(username) g_variant_builder_add (&builder, "{sv}", "UserName", g_variant_new_string (username));
    if(caption) g_variant_builder_add (&builder, "{sv}", "Caption", g_variant_new_string (caption));
    if (type != 0) g_variant_builder_add (&builder, "{sv}", "Type", g_variant_new_int32(type));
    if (methods && mechanisms) {
        GVariantBuilder method_builder;
        int i;
 
        g_variant_builder_init (&method_builder, (const GVariantType *)"a{sas}");

        for (i=0; methods[i]; i++) {
            g_variant_builder_add (&method_builder, "{s^as}", methods[i], mechanisms[i]);
        }

        g_variant_builder_add (&builder, "{sv}", "AuthMethods", g_variant_builder_end (&method_builder));
    }

    return g_variant_builder_end (&builder);
}


GDBusConnection * _get_bus_connection (GError **error)
{
#if USE_P2P
    gchar address[128];

    g_snprintf (address, 127, GSIGNOND_DBUS_ADDRESS, g_get_user_runtime_dir());
    return g_dbus_connection_new_for_address_sync (
        address,
        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
        NULL,
        NULL,
        error);
#else
    return g_bus_get_sync (GSIGNOND_BUS_TYPE, NULL, error);
#endif
}

GSignondDbusAuthService * _get_auth_service (GDBusConnection *connection,
                                             GError **error)
{
    return gsignond_dbus_auth_service_proxy_new_sync (
                connection,
                G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                GSIGNOND_SERVICE,
                GSIGNOND_DAEMON_OBJECTPATH,
                NULL, error);
}

GSignondDbusIdentity * _get_identity_for_path (GDBusConnection *connection,
                                               const gchar *identity_path,
                                               GError **error)
{
    return gsignond_dbus_identity_proxy_new_sync (
        connection,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
        GSIGNOND_SERVICE,
        identity_path,
        NULL, error);
}

GSignondDbusAuthSession * _get_auth_session_for_path (GDBusConnection *connection,
                                                      const gchar *session_path,
                                                      GError **error)
{
    return gsignond_dbus_auth_session_proxy_new_sync (
        connection,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
        GSIGNOND_SERVICE,
        session_path,
        NULL, error);
}

GSignondDbusIdentity * _register_identity (GSignondDbusAuthService *auth_service,
                                           const gchar *app_context,
                                           GError **error) 
{
    GDBusConnection *connection = NULL;
    GSignondDbusIdentity *identity = NULL;
    gchar *identity_path = NULL;

    gboolean res = gsignond_dbus_auth_service_call_register_new_identity_sync (
        auth_service,
        app_context,
        &identity_path,
        NULL,
        error);

    if (res == FALSE) {
        DBG (" ERROR :: %s", error ? (*error)->message : "");
        return NULL;
    }

    connection = g_dbus_proxy_get_connection (G_DBUS_PROXY (auth_service));
    identity = _get_identity_for_path (connection, identity_path, error);

    g_free (identity_path);

    return identity;
}

GSignondDbusIdentity * _get_identity (GSignondDbusAuthService *auth_service,
                                      guint32 id,
                                      const gchar *app_context,
                                      GVariant **identity_info,
                                      GError **error)
{
    gboolean res;
    gchar *identity_path = NULL;
    GDBusConnection *connection = NULL;
    GSignondDbusIdentity *identity = NULL;
    
    res = gsignond_dbus_auth_service_call_get_identity_sync(
        auth_service,
        id,
        app_context,        
        &identity_path,
        identity_info,
        NULL,
        error);

    if (res == FALSE || !identity_path) {
        DBG ("ERROR :: %s", error ? (*error)->message : "");
        return NULL;
    }

    connection = g_dbus_proxy_get_connection (G_DBUS_PROXY (auth_service));
    identity = _get_identity_for_path (connection, identity_path, error);

    g_free (identity_path);

    return identity;
}

/*
 * Test cases
 */
START_TEST (test_register_new_identity)
{
    GError *error = 0;
    GDBusConnection *connection = NULL;
    GSignondDbusAuthService *auth_service = NULL;
    GSignondDbusIdentity *identity = NULL;

    connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "failed to get bus connection : %s", error ? error->message : "(null)");

    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "failed to get auth_service : %s", error ? error->message : "");

    identity = _register_identity (auth_service, "test_app", &error);
    fail_if (identity == NULL, "Failed to register identity : %s", error ? error->message : "");

    g_object_unref (identity);
    g_object_unref (auth_service);
    g_object_unref (connection);
}
END_TEST

START_TEST (test_register_new_identity_with_no_app_context)
{
    GError *error = NULL;
    GSignondDbusAuthService *auth_service = NULL;
    GSignondDbusIdentity *identity = NULL;
    GDBusConnection *connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
    fail_if (error != NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
 
    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "failed to get auth_service : %s", error ? error->message : "");

    identity = _register_identity (auth_service, "", &error);
    fail_if (identity == NULL, "Failed to register identity : %s", error ? error->message : "");

    g_object_unref (identity);
    g_object_unref (auth_service);
    g_object_unref (connection);
}
END_TEST

START_TEST (test_identity_store)
{
    GError *error = NULL; gboolean res = FALSE;
    GSignondDbusAuthService *auth_service = 0;
    GSignondDbusIdentity *identity = 0;
    guint id;
    GVariant *identity_info = NULL;

    GDBusConnection *connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
    fail_if (error != NULL, "failed to get bus connection : %s", error ? error->message : "(null)");

    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "failed to get auth_service : %s", error ? error->message : "");

    identity_info = _get_test_identity_data ();
    fail_if (identity_info == NULL, "Failed to get test identity data");

    identity = _register_identity (auth_service, "test_app", &error);
    fail_if (identity == NULL, "Failed to register identity : %s", error ? error->message : "");

    res = gsignond_dbus_identity_call_store_sync (identity, identity_info, 
                                    &id, NULL, &error); 
    fail_if (res == FALSE, "Failed to store identity : %s", error ? error->message : "");
    fail_if (id == 0);

    DBG ("Identity id : %d\n", id);

    g_object_unref (identity);
    g_object_unref (auth_service);
    g_object_unref (connection);
}
END_TEST

START_TEST(test_identity_get_identity)
{
    GError *error = NULL;
    GSignondDbusAuthService *auth_service = 0;
    guint32 id = 1; /* identity id created in test_identity_store */
    GVariant *identity_info = NULL;
    GSignondDbusIdentity *identity = NULL;
    GDBusConnection *connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
    fail_if (error != NULL, "failed to get bus connection : %s", error ? error->message : "(null)");

    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "failed to get auth_service : %s", error ? error->message : "");

    identity = _get_identity (auth_service, id, "test_app", &identity_info, &error);
    fail_if (identity == NULL, "Failed to get identity for id '%u' : %s", id, error ? error->message : "");
    fail_if (identity_info == NULL);

    fail_if (_validate_identity_info(identity_info) == FALSE);

    g_object_unref (auth_service);
    g_object_unref (identity);
    g_object_unref (connection);
}
END_TEST

START_TEST(test_clear_database)
{
    GError *error = 0;
    gboolean res, ret;
    GSignondDbusAuthService *auth_service = 0;
    GDBusConnection *connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
    fail_if (error != NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
 
    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "failed to get auth_service : %s", error ? error->message : "");

    res = gsignond_dbus_auth_service_call_clear_sync (
        auth_service,
        &ret,
        NULL,
        &error);
    fail_if (res == FALSE || ret == FALSE, "Failed to wipe databases : %s", error ? error->message : "");

    g_object_unref (auth_service);
    g_object_unref (connection);
}
END_TEST

static void _on_session_unregistered (GSignondDbusAuthSession *sesssion, gpointer userdata)
{
    gboolean *out = (gboolean*) userdata;
    g_return_if_fail (out);

    *out = TRUE;
}

static void _on_identity_updated (GSignondDbusIdentity *identity, gint change_type, gpointer userdata)
{
    gboolean *out = (gboolean *)userdata;
    g_return_if_fail (out);

    if (change_type == 2 /* GSIGNOND_IDENTITY_SIGNED_OUT */) 
        *out = TRUE;
}

static void _on_sign_out_reply (GSignondDbusIdentity *sender, GAsyncResult *reply, gpointer data)
{
    GError *error = NULL;

    gboolean res = FALSE, ret = FALSE;
    
    ret = gsignond_dbus_identity_call_sign_out_finish (sender, &res, reply, &error);

    fail_if (ret == FALSE, "failed to finish signout, %s", error ? error->message : "");
    fail_if (res == FALSE, "failed to call signout on identity : %s", error ? error->message : "");

    g_main_loop_quit ((GMainLoop *)data);
    g_main_loop_unref((GMainLoop *)data);
}

START_TEST(test_identity_signout)
{
    GError *error = 0;
    gboolean res;
    GSignondDbusAuthService *auth_service = 0;
    GSignondDbusIdentity *identity = 0;
    GSignondDbusAuthSession *auth_session = 0;
    GVariant *identity_info = NULL;
    gchar *session_path = NULL;
    guint id;
    gboolean identity_signed_out = FALSE;
    gboolean session_unregistered = FALSE;
    GMainLoop *loop = NULL;
    GDBusConnection *connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "failed to get bus connection : %s", error ? error->message : "(null)");
    fail_if (error != NULL, "failed to get bus connection : %s", error ? error->message : "(null)");

    loop = g_main_loop_new (NULL, FALSE);

    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "failed to get auth_service : %s", error ? error->message : "");

    identity = _register_identity (auth_service, "", &error);
    fail_if (identity == NULL, "Failed to register new identity : %s", error ? error->message : "");

    identity_info = _get_test_identity_data ();
    fail_if (identity_info == NULL);

    res = gsignond_dbus_identity_call_store_sync (identity, identity_info,
                                                  &id, NULL, &error);
    fail_if (res == FALSE, "Failed to store identity");
    fail_if (id == 0);

    g_signal_connect (identity, "info-updated", G_CALLBACK(_on_identity_updated), &identity_signed_out);

    res = gsignond_dbus_identity_call_get_auth_session_sync (
            identity, "ssotest", &session_path, NULL, &error);

    fail_if (res == FALSE, "Failed to create authentication session on identity for method 'ssotest', error : %s",
        error ? error->message : "");
    fail_if (session_path == NULL, "(null) session_path");

    auth_session = _get_auth_session_for_path (connection, session_path, &error);

    g_free (session_path);
    fail_if (error != NULL, "failed to created session proxy for path '%s', error: %s", 
        session_path, error ? error->message : "");
    fail_if (auth_session == NULL, "(null) session object");

    g_signal_connect (auth_session, "unregistered", G_CALLBACK (_on_session_unregistered), &session_unregistered);

    /* Call SignOut on identity */
    gsignond_dbus_identity_call_sign_out (identity, NULL, (GAsyncReadyCallback)_on_sign_out_reply, loop);

    g_main_loop_run (loop);

    fail_unless (session_unregistered == TRUE, "Session unregistred not reached");
    fail_unless (identity_signed_out == TRUE, "Identity signed out signal not reached");

    g_object_unref (auth_service);
    g_object_unref (identity);
    g_object_unref (auth_session);
    g_object_unref (connection);
}
END_TEST

START_TEST(test_query_identities)
{
    GDBusConnection *connection = NULL;
    GSignondDbusAuthService *auth_service = NULL;
    GSignondDbusIdentity *identity = NULL;
    GVariant *v_info = NULL;
    GSignondIdentityInfo *info1 = NULL, *info2 = NULL, *info3 = NULL, *tmp_info = NULL;
    GSignondDictionary *filter = NULL;
    GVariant *v_identities = NULL;
    const gchar *methods[] = { "ssotest", NULL };
    const gchar *mech[] = {"mech1", "mech2", NULL};
    const gchar **mechanisms[] = { mech };
    gboolean res;
    guint32 id = 0;
    GError *error = NULL;

    connection = _get_bus_connection (&error);
    fail_if (connection == NULL, "Failed to get bus connection : %s", error ? error->message : "");

    auth_service = _get_auth_service (connection, &error);
    fail_if (auth_service == NULL, "Failed to get auth_service : %s", error ? error->message : "");

    /* created identity1 */
    identity = _register_identity (auth_service, "app_context_A", &error);
    fail_if (identity == NULL, "Failed to register new identity : %s", error ? error->message : "");

    v_info = _create_identity_info_with_data ("user1", "caption1", 1, methods, mechanisms);
    fail_if (v_info == NULL);
    res = gsignond_dbus_identity_call_store_sync (identity, v_info, &id, NULL, &error);
    fail_if (res == FALSE || id == 0, "Failed to store identity : %s", error ? error->message : "");
    g_object_unref (identity);

    identity = _get_identity (auth_service, id, "app_context_A", &v_info, &error);
    fail_if (identity == NULL || v_info == NULL, "Failed to load identity for id '%d' : %s", id, error ? error->message : "");
    g_object_unref (identity);
    info1 = gsignond_dictionary_new_from_variant (v_info);

    /* created identity2 */
    identity = _register_identity (auth_service, "app_context_B", &error);
    fail_if (identity == NULL, "Failed to register new identity : %s", error ? error->message : "");

    v_info = _create_identity_info_with_data ("user2", "caption2", 2, methods, mechanisms);
    fail_if (v_info == NULL);
    res = gsignond_dbus_identity_call_store_sync (identity, v_info, &id, NULL, &error);
    fail_if (res == FALSE || id == 0, "Failed to store identity : %s", error ? error->message : "");
    g_object_unref (identity);

    identity = _get_identity (auth_service, id, "app_context_B", &v_info, &error);
    fail_if (identity == NULL || v_info == NULL, "Failed to load identity for id '%d' : %s", id, error ? error->message : "");
    g_object_unref (identity);
    info2 = gsignond_dictionary_new_from_variant (v_info);

    /* create identity3 */
    identity = _register_identity (auth_service, "app_context_A", &error);
    fail_if (identity == NULL, "Failed to register new identity : %s", error ? error->message : "");

    v_info = _create_identity_info_with_data ("user2", "caption3", 2, methods, mechanisms);
    fail_if (v_info == NULL);
    res = gsignond_dbus_identity_call_store_sync (identity, v_info, &id, NULL, &error);
    fail_if (res == FALSE || id == 0, "Failed to store identity : %s", error ? error->message : "");
    g_object_unref (identity);

    identity = _get_identity (auth_service, id, "app_context_A", &v_info, &error);
    fail_if (identity == NULL || v_info == NULL, "Failed to load identity for id '%d' : %s", id, error ? error->message : "");
    g_object_unref (identity);
    info3 = gsignond_dictionary_new_from_variant (v_info);

    /* query identities for app-context: app_context_A */
    v_identities = NULL;
    filter = gsignond_dictionary_new();
    res = gsignond_dbus_auth_service_call_query_identities_sync (auth_service,
            gsignond_dictionary_to_variant (filter),
            "app_context_A", &v_identities, NULL, &error);
    gsignond_dictionary_unref (filter);
    fail_if (res == FALSE || !v_identities, "Failed to query identities for "
                           "app context 'app_context_A' : %s",
                           error ? error->message : "");
    fail_if (g_variant_n_children (v_identities) != 2, 
        "Expected no of identities '%d', got '%d'", 2,
        g_variant_n_children(v_identities));
    /* validated query results */
    tmp_info = gsignond_dictionary_new_from_variant (
                            g_variant_get_child_value (v_identities, 0));
    fail_if (gsignond_identity_info_compare (info1, tmp_info) == FALSE);
    gsignond_identity_info_unref (tmp_info);

    tmp_info = gsignond_dictionary_new_from_variant (
                            g_variant_get_child_value (v_identities, 1));
    fail_if (gsignond_identity_info_compare (info3, tmp_info) == FALSE);
    gsignond_identity_info_unref (tmp_info);

    /* query identities for app-context: app_context_B, Identity type : 2 */
    v_identities = NULL;
    filter = gsignond_dictionary_new();
    gsignond_dictionary_set_int32 (filter, "Type", 2);
    res = gsignond_dbus_auth_service_call_query_identities_sync (auth_service,
            gsignond_dictionary_to_variant (filter),
            "app_context_B", &v_identities, NULL, &error);
    gsignond_dictionary_unref (filter);
    fail_if (res == FALSE || !v_identities, "Failed to query identities for "
                           "app context 'app_context_B' and Type: 2 : %s",
                           error ? error->message : "");
    /* validated query results */
    fail_if (g_variant_n_children (v_identities) != 1, 
        "Expected no of identities '%d', got '%d'", 1,
        g_variant_n_children(v_identities));
    tmp_info = gsignond_dictionary_new_from_variant (
                            g_variant_get_child_value (v_identities, 0));
    fail_if (gsignond_identity_info_compare (info2, tmp_info) == FALSE);
    gsignond_identity_info_unref (tmp_info);

    /* query identities for app-context: app_context_A, Caption: "cap*" */
    v_identities = NULL;
    filter = gsignond_dictionary_new();
    gsignond_dictionary_set_string (filter, "Caption", "cap");
    res = gsignond_dbus_auth_service_call_query_identities_sync (auth_service,
            gsignond_dictionary_to_variant (filter),
            "app_context_B", &v_identities, NULL, &error);
    gsignond_dictionary_unref (filter);
    fail_if (res == FALSE || !v_identities, "Failed to query identities for "
                           "app context 'app_context_A' : %s",
                           error ? error->message : "");
    /* validated query results */
    fail_if (g_variant_n_children (v_identities) != 1, 
        "Expected no of identities '%d', got '%d'", 1,
        g_variant_n_children(v_identities));
    tmp_info = gsignond_dictionary_new_from_variant (
                            g_variant_get_child_value (v_identities, 0));
    fail_if (gsignond_identity_info_compare 
            (info2, tmp_info) == FALSE);
    gsignond_identity_info_unref (tmp_info);

    gsignond_identity_info_unref (info1);
    gsignond_identity_info_unref (info2);
    gsignond_identity_info_unref (info3);

    g_object_unref (auth_service);
    g_object_unref (connection);
}
END_TEST

Suite* daemon_suite (void)
{
    Suite *s = suite_create ("Gsignon daemon");
    
    TCase *tc = tcase_create ("Identity");

    tcase_add_unchecked_fixture (tc, setup_daemon, teardown_daemon);

    tcase_add_test (tc, test_register_new_identity);
    tcase_add_test (tc, test_register_new_identity_with_no_app_context);
    tcase_add_test (tc, test_identity_store);
    tcase_add_test (tc, test_identity_get_identity);
    tcase_add_test (tc, test_clear_database);
    tcase_add_test (tc, test_identity_signout);
    tcase_add_test (tc, test_query_identities);

    suite_add_tcase (s, tc);

    return s;
}

int main (int argc, char *argv[])
{
    int number_failed;
    Suite *s = 0;
    SRunner *sr = 0;
   
#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    exe_name = argv[0];

    s = daemon_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
