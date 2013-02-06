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

#include <check.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>

#include <daemon/dbus/gsignond-dbus.h>
#include <daemon/dbus/gsignond-dbus-auth-service-gen.h>

START_TEST (test_register_new_identity)
{
    GVariant *reply = 0;
    GError *error = 0;
    GTestDBus *dbus = NULL;
    GMainLoop *main_loop = 0;
    char *identity_path = NULL;
    gboolean res;
    GSignondDbusAuthService *auth_service = 0;

    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    fail_unless (dbus != NULL, "could not create test dbus");

    g_test_dbus_add_service_dir (dbus, GSIGNOND_TEST_DBUS_SERVICE_DIR);
    
    g_test_dbus_up (dbus);

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

    fail_if (res == FALSE, "Failed to register identity");
    //fail_if (error != NULL, error->message);
    fail_if (identity_path == NULL);
    g_free (identity_path);

    g_object_unref (auth_service);
}
END_TEST

Suite* daemon_suite (void)
{
    Suite *s = suite_create ("Gsignon daemon");
    
    TCase *tc = tcase_create ("Identity");
    tcase_add_test (tc, test_register_new_identity);

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

    srunner_run_all(sr, CK_NORMAL);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  
