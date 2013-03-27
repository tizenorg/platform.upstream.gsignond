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

#include <config.h>
#include <check.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>

#include <daemon/dbus/gsignond-dbus.h>
#include <daemon/dbus/gsignond-dbus-auth-service-gen.h>
#include <daemon/dbus/gsignond-dbus-identity-gen.h>
#include <gsignond/gsignond-identity-info.h>
#include <gsignond/gsignond-log.h>

struct IdentityData {
    gchar *key;
    gchar *type;
    void *value;
} data[] = {
        { "UserName", "s", "test_user" },
        { "Secret", "s", "test_pass" },
        { "StoreSecret", "b", (void *)TRUE}
 };

#if HAVE_GTESTDBUS

GTestDBus *dbus = NULL;

static void
setup_daemon (void)
{
    fail_if (g_setenv ("SSO_IDENTITY_TIMEOUT", "60", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_DAEMON_TIMEOUT", "60", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_AUTH_SESSION_TIMEOUT", "60", TRUE) == FALSE);
    fail_if (g_setenv ("SSO_STORAGE_PATH", "/tmp/gsignond", TRUE) == FALSE);

    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    fail_unless (dbus != NULL, "could not create test dbus");

    g_test_dbus_add_service_dir (dbus, GSIGNOND_TEST_DBUS_SERVICE_DIR);

    g_test_dbus_up (dbus);
    g_print ("Server address : %s\n", g_test_dbus_get_bus_address(dbus));
}

static void
teardown_daemon (void)
{
    g_test_dbus_down (dbus);

    g_unsetenv ("SSO_IDENTITY_TIMEOUT");
    g_unsetenv ("SSO_DAEMON_TIMEOUT");
    g_unsetenv ("SSO_AUTH_SESSION_TIMEOUT");
    g_unsetenv ("SSO_STORAGE_PATH");
}
#endif

gboolean validate_identity_info (GVariant *identity_info)
{
    GSignondIdentityInfo *identity = 0;
    const gchar *username = 0;
    if (!identity_info) return FALSE;

    identity = (GSignondIdentityInfo *)gsignond_dictionary_new_from_variant (identity_info);
    if (!identity) return FALSE;

    username = gsignond_identity_info_get_username (identity);

    if (!username || strcmp (username, "test_user")) return FALSE;

    return TRUE;
}

START_TEST (test_register_new_identity)
{
    GError *error = 0;
    gboolean res;
    GSignondDbusAuthService *auth_service = 0;
    gchar *identity_path = 0;
 
    auth_service = gsignond_dbus_auth_service_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        GSIGNOND_SERVICE,
        GSIGNOND_DAEMON_OBJECTPATH,
        NULL, &error);

    fail_if (auth_service == NULL);

    res = gsignond_dbus_auth_service_call_register_new_identity_sync (
        auth_service,
        "test_app",
        &identity_path,
        NULL,
        &error);

    g_object_unref (auth_service);

    fail_if (res == FALSE, "Failed to register identity");
    fail_if (identity_path == NULL);
    g_free (identity_path);
}
END_TEST

START_TEST (test_register_new_identity_with_no_app_context)
{
    GError *error = 0;
    gboolean res;
    GSignondDbusAuthService *auth_service = 0;
    gchar *identity_path = NULL;
 
    auth_service = gsignond_dbus_auth_service_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        GSIGNOND_SERVICE,
        GSIGNOND_DAEMON_OBJECTPATH,
        NULL, &error);

    fail_if (auth_service == NULL);

    res = gsignond_dbus_auth_service_call_register_new_identity_sync (
        auth_service,
        "",
        &identity_path,
        NULL,
        &error);

    g_object_unref (auth_service);

    fail_if (res == FALSE, "Failed to register identity");
    fail_if (identity_path == NULL);
    g_free (identity_path);
}
END_TEST

START_TEST (test_identity_store)
{
    GError *error = NULL; gboolean res = FALSE;
    GSignondDbusAuthService *auth_service = 0;
    GSignondDbusIdentity *identity = 0;
    guint id;
    GVariant *identity_info = NULL;
    gchar *identity_path = NULL;
    GVariantBuilder builder, method_builder;
    int i;
    gchar* mechanisms [] = {"password", NULL};

    auth_service = gsignond_dbus_auth_service_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        GSIGNOND_SERVICE,
        GSIGNOND_DAEMON_OBJECTPATH,
        NULL, &error);

    fail_if (auth_service == NULL);
 
    res = gsignond_dbus_auth_service_call_register_new_identity_sync (
        auth_service,
        "",
        &identity_path,
        NULL,
        &error);

    fail_if (identity_path == NULL);

    g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
    
    for (i=0; i < sizeof(data)/sizeof(struct IdentityData); i++) {
        g_variant_builder_add (&builder, "{sv}", data[i].key, g_variant_new (data[i].type, data[i].value));
    }

    g_variant_builder_init (&method_builder, (const GVariantType *)"a{sas}");
    g_variant_builder_add (&method_builder, "{s^as}", "password", mechanisms);

    g_variant_builder_add (&builder, "{sv}", "AuthMethods", g_variant_builder_end (&method_builder));

    identity_info = g_variant_builder_end (&builder);

    fail_if (identity_info == NULL);

    identity = gsignond_dbus_identity_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        GSIGNOND_SERVICE,
        identity_path,
        NULL, &error);

    res = gsignond_dbus_identity_call_store_sync (
        identity,
        identity_info,
        &id,
        NULL,
        &error);

    g_object_unref (identity);

    fail_if (res == FALSE, "Failed to store identity");
    fail_if (id == 0);

    g_print ("Identity id : %d\n", id);
}
END_TEST

START_TEST(test_identity_get_identity)
{
    GError *error = NULL; gboolean res = FALSE;
    GSignondDbusAuthService *auth_service = 0;
    gint id = 1 ; /* identity id created in test_identity_store */
    GVariant *identity_info = NULL;
    gchar *identity_path = 0;

    auth_service = gsignond_dbus_auth_service_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        GSIGNOND_SERVICE,
        GSIGNOND_DAEMON_OBJECTPATH,
        NULL, &error);

    fail_if (auth_service == NULL);
 
    res = gsignond_dbus_auth_service_call_get_identity_sync(
        auth_service,
        id,
        "test-app",
        &identity_path,
        &identity_info,
        NULL,
        &error);

    fail_if (res == FALSE, "Failed to get identity");
    fail_if (identity_path == NULL);
    fail_if (identity_info == NULL);

    fail_if (validate_identity_info(identity_info) == FALSE);
}
END_TEST

Suite* daemon_suite (void)
{
    Suite *s = suite_create ("Gsignon daemon");
    
    TCase *tc = tcase_create ("Identity");
#if HAVE_GTESTDBUS
    tcase_add_unchecked_fixture (tc, setup_daemon, teardown_daemon);
#endif
    tcase_add_test (tc, test_register_new_identity);
    tcase_add_test (tc, test_register_new_identity_with_no_app_context);
    tcase_add_test (tc, test_identity_store);
    tcase_add_test (tc, test_identity_get_identity);

    suite_add_tcase (s, tc);
    
    return s;
}

int main (void)
{
    int number_failed;
    Suite *s = 0;
    SRunner *sr = 0;
   
    g_type_init ();

    s = daemon_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  
