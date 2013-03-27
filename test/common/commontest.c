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
#include <gsignond/gsignond-session-data.h>
#include <gsignond/gsignond-plugin-loader.h>
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-log.h>


START_TEST (test_session_data)
{
    GSignondSessionData* data;
    GSignondSessionData* data_from_variant;
    GSignondSessionData* data_from_copy;
    GVariant* variant;
    
    data = gsignond_dictionary_new();
    fail_if(data == NULL);
    
    fail_unless(gsignond_session_data_get_username(data) == NULL);
    fail_unless(gsignond_session_data_get_secret(data) == NULL);

    gsignond_session_data_set_username(data, "megauser");
    gsignond_session_data_set_secret(data, "megapassword");
    
    fail_unless(g_strcmp0(gsignond_session_data_get_username(data), 
                          "megauser") == 0);
    fail_unless(g_strcmp0(gsignond_session_data_get_secret(data), 
                          "megapassword") == 0);

    gsignond_session_data_set_username(data, "usermega");
    fail_unless(g_strcmp0(gsignond_session_data_get_username(data), 
                          "usermega") == 0);
    
    data_from_copy = gsignond_dictionary_copy(data);
    fail_if(data_from_copy == NULL);

    fail_unless(g_strcmp0(gsignond_session_data_get_username(data_from_copy), 
                          "usermega") == 0);
    fail_unless(g_strcmp0(gsignond_session_data_get_secret(data_from_copy), 
                          "megapassword") == 0);

    variant = gsignond_dictionary_to_variant(data);
    fail_if(variant == NULL);
    data_from_variant = gsignond_dictionary_new_from_variant(variant);
    fail_if(data_from_variant == NULL);

    fail_unless(g_strcmp0(gsignond_session_data_get_username(data_from_variant), 
                          "usermega") == 0);
    fail_unless(g_strcmp0(gsignond_session_data_get_secret(data_from_variant), 
                          "megapassword") == 0);
    
    g_variant_unref(variant);
    gsignond_dictionary_unref(data_from_variant);
    gsignond_dictionary_unref(data_from_copy);
    gsignond_dictionary_unref(data);
}
END_TEST

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


Suite* common_suite (void)
{
    Suite *s = suite_create ("Common library");
    
    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_session_data);
    tcase_add_test (tc_core, test_plugin_loader);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;
    
    g_type_init();
    
    Suite *s = common_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  
