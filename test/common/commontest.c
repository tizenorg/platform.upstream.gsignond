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
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <glib-unix.h>
#include "gsignond/gsignond-session-data.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-utils.h"
#include "common/gsignond-identity-info.h"
#include "common/gsignond-pipe-stream.h"
#include "common/gsignond-plugin-loader.h"

static GSequence*
_sequence_new (gchar *data)
{
    GSequence *value = NULL;
    value = g_sequence_new (NULL);
    g_sequence_append (value, (guint8 *)data);
    return value;
}

static gboolean
_compare_sequences (
        GSequence *one,
        GSequence *two)
{
    GSequenceIter *iter1 = NULL, *iter2 = NULL;
    gboolean equal = TRUE;

    if (one == NULL && two == NULL)
        return TRUE;

    if ((one != NULL && two == NULL) ||
        (one == NULL && two != NULL) ||
        (g_sequence_get_length (one) != g_sequence_get_length (two)))
        return FALSE;

    if (one == two)
        return TRUE;

    iter1 = g_sequence_get_begin_iter (one);
    while (!g_sequence_iter_is_end (iter1)) {
        iter2 = g_sequence_get_iter_at_pos (two,
                    g_sequence_iter_get_position (iter1));
        if (g_strcmp0 (g_sequence_get (iter1), g_sequence_get (iter2)) != 0) {
            equal = FALSE;
            break;
        }
        iter1 = g_sequence_iter_next (iter1);
    }

    return equal;
}

START_TEST (test_pipe_stream)
{
    GSignondPipeStream *stream = NULL;
    gint pipefd[2];
    fail_unless (pipe (pipefd) == 0);
    stream = gsignond_pipe_stream_new (pipefd[0], pipefd[1], TRUE);
    fail_if (stream == NULL);
    g_object_unref (stream);
}
END_TEST

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

START_TEST (test_identity_info)
{
    guint32 id = 125;
    guint32 type = 456;
    const gchar *username = "username1";
    const gchar *secret = "secret1";
    const gchar *caption = "caption1";
    GSignondIdentityInfo *identity = NULL;
    GSignondIdentityInfo *identity2 = NULL;
    GSignondSecurityContextList *ctx_list = NULL, *list = NULL;
    GSignondSecurityContext *ctx, *ctx1, *ctx2, *ctx3 ;
    GHashTable *methods = NULL, *methods2;
    GSequence *seq1 = NULL, *seq_realms, *seq21, *mechs;
    GList *list2;

    identity = gsignond_identity_info_new ();
    fail_if (identity == NULL);

    fail_unless (gsignond_identity_info_get_id (identity) == 0);
    fail_unless (gsignond_identity_info_get_is_identity_new (identity)== TRUE);
    fail_unless (gsignond_identity_info_get_username (identity) == NULL);
    fail_unless (gsignond_identity_info_get_is_username_secret (
                identity) == FALSE);
    fail_unless (gsignond_identity_info_get_secret (identity) == NULL);
    fail_unless (gsignond_identity_info_get_store_secret (identity) == FALSE);
    fail_unless (gsignond_identity_info_get_caption (identity) == NULL);
    fail_unless (gsignond_identity_info_get_realms (identity) == NULL);
    fail_unless (gsignond_identity_info_get_methods (identity) == NULL);
    fail_unless (gsignond_identity_info_get_mechanisms (
                identity, "testmech") == NULL);
    fail_unless (gsignond_identity_info_get_access_control_list (
                identity) == NULL);
    fail_unless (gsignond_identity_info_get_owner (identity) == NULL);
    fail_unless (gsignond_identity_info_get_validated (identity) == FALSE);
    fail_unless (gsignond_identity_info_get_identity_type (identity) == 0);

    fail_unless (gsignond_identity_info_set_id (identity, id) == TRUE);

    fail_unless (id == gsignond_identity_info_get_id (identity));

    fail_unless (gsignond_identity_info_set_identity_new (identity) == TRUE);

    fail_unless (gsignond_identity_info_get_is_identity_new (
                identity) == TRUE);

    fail_unless (gsignond_identity_info_set_username (
                identity, NULL) == TRUE);

    fail_unless (gsignond_identity_info_get_username (identity) == NULL);

    fail_unless (gsignond_identity_info_set_username (
                identity, username) == TRUE);

    fail_unless (g_strcmp0 (username, gsignond_identity_info_get_username (
                    identity)) == 0);

    fail_unless (gsignond_identity_info_set_username_secret (
                identity, TRUE) == TRUE);

    fail_unless (gsignond_identity_info_get_is_username_secret (
                identity) == TRUE);

    fail_unless (gsignond_identity_info_set_secret (identity, NULL) == TRUE);

    fail_unless (gsignond_identity_info_get_secret (identity) == NULL);

    fail_unless (gsignond_identity_info_set_secret (identity, secret) == TRUE);

    fail_unless (g_strcmp0 (secret, gsignond_identity_info_get_secret (
                    identity)) == 0);

    fail_unless (gsignond_identity_info_set_store_secret (
                identity, TRUE) == TRUE);

    fail_unless (gsignond_identity_info_get_store_secret (
                identity) == TRUE);

    fail_unless (gsignond_identity_info_set_caption (identity, NULL) == TRUE);

    fail_unless (gsignond_identity_info_get_caption (identity) == NULL);

    fail_unless (gsignond_identity_info_set_caption (
                identity, caption) == TRUE);

    fail_unless (g_strcmp0 (caption, gsignond_identity_info_get_caption (
                    identity)) == 0);

    /*realms*/
    seq_realms = _sequence_new("realms1");
    fail_unless (gsignond_identity_info_set_realms (
                identity, seq_realms) == TRUE);

    seq1 = gsignond_identity_info_get_realms (identity);
    fail_if (seq1 == NULL);
    fail_unless (_compare_sequences (seq1, seq_realms) == TRUE);
    g_sequence_free (seq1); seq1 = NULL;
    g_sequence_free (seq_realms);

    /*methods*/
    methods = g_hash_table_new_full ((GHashFunc)g_str_hash,
            (GEqualFunc)g_str_equal,
            (GDestroyNotify)NULL,
            (GDestroyNotify)g_sequence_free);
    seq1 = _sequence_new("mech11"); g_sequence_append (seq1, "mech12");
    fail_unless (gsignond_identity_info_set_methods (
                identity, methods) == TRUE);
    g_hash_table_insert (methods, "method1", seq1);
    g_hash_table_insert (methods, "method2", _sequence_new("mech21"));
    g_hash_table_insert (methods, "method3", _sequence_new("mech31"));
    g_hash_table_insert (methods, "method4", _sequence_new("mech41"));
    fail_unless (gsignond_identity_info_set_methods (
                identity, methods) == TRUE);

    methods2 = gsignond_identity_info_get_methods (identity);
    fail_if (methods2 == NULL);
    seq21 = g_hash_table_lookup (methods, "method1");
    fail_if (seq21 == NULL);
    fail_unless (_compare_sequences (seq1, seq21) == TRUE);
    g_hash_table_unref (methods2);
    g_hash_table_unref (methods);

    fail_unless (gsignond_identity_info_get_mechanisms (
                identity, "method20") == NULL);

    mechs = gsignond_identity_info_get_mechanisms (
            identity, "method1");
    fail_if (mechs == NULL);
    g_sequence_free (mechs);

    fail_unless (gsignond_identity_info_remove_method (
                identity, "method20") == FALSE);
    fail_unless (gsignond_identity_info_remove_method (
                identity, "method4") == TRUE);

    /*acl*/
    ctx1 = gsignond_security_context_new_from_values ("sysctx1", "appctx1");
    ctx2 = gsignond_security_context_new_from_values ("sysctx2", "appctx2");
    ctx3 = gsignond_security_context_new_from_values ("sysctx3", "appctx3");
    ctx_list = g_list_append (ctx_list,ctx1);
    ctx_list = g_list_append (ctx_list,ctx2);
    ctx_list = g_list_append (ctx_list,ctx3);
    fail_unless (gsignond_identity_info_set_access_control_list (
                identity, ctx_list) == TRUE);

    list = gsignond_identity_info_get_access_control_list (identity);
    fail_if (list == NULL);
    list2 = g_list_nth (list, 0);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx1) == 0);
    list2 = g_list_nth (list, 1);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx2) == 0);
    list2 = g_list_nth (list, 2);
    ctx = (GSignondSecurityContext *) list2->data;
    fail_unless (gsignond_security_context_compare (ctx, ctx3) == 0);
    gsignond_security_context_list_free (list); list = NULL;

    /*owners*/
    fail_unless (gsignond_identity_info_set_owner (
                identity, ctx1) == TRUE);
    ctx = gsignond_identity_info_get_owner (identity);
    fail_if (ctx == NULL);
    fail_unless (gsignond_security_context_compare (ctx, ctx1) == 0);
    gsignond_security_context_free (ctx); ctx = NULL;

    fail_unless (gsignond_identity_info_set_validated (
                identity, FALSE) == TRUE);

    fail_unless (gsignond_identity_info_get_validated (identity) == FALSE);

    fail_unless (gsignond_identity_info_set_identity_type (
                identity, type) == TRUE);

    fail_unless (type == gsignond_identity_info_get_identity_type (identity));

    /*copy*/
    identity2 = gsignond_identity_info_copy (identity);
    fail_if (identity2 == NULL);
    fail_unless (gsignond_identity_info_compare (identity, identity2) == TRUE);
    gsignond_identity_info_unref (identity2);
    fail_unless (gsignond_identity_info_compare (identity, identity) == TRUE);

    gsignond_security_context_list_free (ctx_list); ctx_list = NULL;

    gsignond_identity_info_unref (identity);
}
END_TEST

START_TEST (test_is_host_in_domain)
{
    fail_unless(gsignond_is_host_in_domain("somehost", "") == TRUE);
    fail_unless(gsignond_is_host_in_domain("", "somedomain") == FALSE);
    fail_unless(gsignond_is_host_in_domain("", "") == TRUE);
    fail_unless(gsignond_is_host_in_domain("somehost", "otherdomain") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost", "somehost") == TRUE);
    fail_unless(gsignond_is_host_in_domain("somehost.com", "otherdomain.com") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost.com", "othersomehost.com") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost.com", "host.com") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost.com", "somehost.com") == TRUE);
    fail_unless(gsignond_is_host_in_domain("somehost.com", "subhost.somehost.com") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost.somedomain.com", "otherdomain.com") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost.somedomain.com", "somehost.otherdomain.com") == FALSE);
    fail_unless(gsignond_is_host_in_domain("somehost.somedomain.com", "somedomain.com") == TRUE);
}
END_TEST

Suite* common_suite (void)
{
    Suite *s = suite_create ("Common library");
    
    /* Core test case */
    TCase *tc_core = tcase_create ("Tests");
    tcase_add_test (tc_core, test_identity_info);
    tcase_add_test (tc_core, test_pipe_stream);
    tcase_add_test (tc_core, test_session_data);
    tcase_add_test (tc_core, test_plugin_loader);
    tcase_add_test (tc_core, test_is_host_in_domain);
    suite_add_tcase (s, tc_core);
    return s;
}

int main (void)
{
    int number_failed;

#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    Suite *s = common_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
  
