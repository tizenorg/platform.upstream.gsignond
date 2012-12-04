/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <signal.h>
#include <glib.h>
#include <gio/gio.h>

#include <common/gsignond-config.h>
#include <common/gsignond-log.h>
#include <common/gsignond-credentials.h>
#include <daemon/db/gsignond-db-default-storage.h>
#include <daemon/gsignond-daemon.h>

GSignondDaemon *_daemon = NULL;

static void _signal_handler (int sig)
{
    (void) sig;
}

static void _install_sighandlers()
{
    struct sigaction act;

    act.sa_handler = _signal_handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_RESTART;

    sigaction (SIGHUP, &act, 0);
    sigaction (SIGTERM, &act, 0);
    sigaction (SIGINT, &act, 0);
}

static void
_key_free (
        GString *data)
{
    g_string_free (data, TRUE);
}

static void
_value_free (
        GByteArray *data)
{
    g_byte_array_free (data, TRUE);
}

static GByteArray*
_value_new (
        const guint8 *data)
{
    GByteArray *value = NULL;
    value = g_byte_array_new ();
    value = g_byte_array_append (value,
                data, strlen((const char*)data));
    return value;
}

typedef struct {
    GHashTable *table;
    int status;
} Data;

void
_compare_key_value(
        GString *key,
        GByteArray *value,
        Data *user_data)
{
    GByteArray *value_found = NULL;
    value_found = (GByteArray *)g_hash_table_lookup (user_data->table, key);
    if (value_found &&
        g_strcmp0(value_found->data, value->data) == 0)
        return;
    user_data->status = 0;
}


int main (int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *opt_context = NULL;
    gint ret = 0;
    guint sigint_id =  0;
    GOptionEntry opt_entries[] = {
        {NULL }
    };
    GMainLoop *loop = 0;
    GSignondSecretStorage *sec_storage = NULL;
    GSignondDbDefaultStorage *storage = NULL;
    GSignondConfig *config = NULL;
    GString *un = NULL;
    GString *pass = NULL;
    GSignondCredentials *creds = NULL;
    GSignondCredentials *creds1 = NULL;
    guint32 id = 1, method = 2;
    GHashTable *data = NULL;
    GHashTable *data2 = NULL;
    GHashTableIter iter;
    GString *key =NULL;
    GByteArray *value =NULL;
    Data input;

    g_type_init ();

    opt_context = g_option_context_new ("SSO daemon");
    g_option_context_add_main_entries (opt_context, opt_entries, NULL);
    if (!g_option_context_parse (opt_context, &argc, &argv, &error)) {
        ERR ("Error parsing options: %s", error->message);
        g_error_free (error);
        return -1;
    }

    config = gsignond_config_new ();
    storage = gsignond_db_default_storage_new ();
    sec_storage = GSIGNOND_SECRET_STORAGE (storage);
    if (gsignond_secret_storage_open_db (sec_storage, config)) {
        INFO ("Database open");
    } else {
        WARN ("Database cannot be opened");
    }

    /* test is_open and close functionality */
    if (gsignond_secret_storage_open_db (sec_storage, config)) {
        INFO ("Database opened AGAIN");
    } else {
        WARN ("Database cannot be opened AGAIN");
    }

    creds = gsignond_credentials_new ();
    gsignond_credentials_set_data(creds, id, "user 1", "pass 1");
    /* add credentials */
    if (gsignond_secret_storage_update_credentials (sec_storage, creds)) {
        INFO ("Database credentials UPDATED");
    } else {
        WARN ("Database credentials CANNOT be updated");
    }
    g_object_unref (creds); creds = NULL;
    /* read the added credentials */
    creds = gsignond_secret_storage_load_credentials (sec_storage, id);
    if (creds) {
        INFO ("Database credentials LOADED");
    } else {
        WARN ("Database credentials CANNOT be loaded");
    }
    /* check the credentials*/
    if (gsignond_secret_storage_check_credentials (sec_storage, creds)) {
        INFO ("Database credentials CHECKED");
    } else {
        WARN ("Database credentials CANNOT be checked");
    }
    if (creds) {
        g_object_unref (creds);
    }

    /* remove the added credentials */
    if (gsignond_secret_storage_remove_credentials (sec_storage, id)) {
        INFO ("Database credentials REMOVED");
    } else {
        WARN ("Database credentials CANNOT be removed");
    }

    /* add data to store */
    data = g_hash_table_new_full ((GHashFunc)g_string_hash,
            (GEqualFunc)g_string_equal,
            (GDestroyNotify)_key_free,
            (GDestroyNotify)_value_free);
    g_hash_table_insert (data, g_string_new("key1"), _value_new("value1"));
    g_hash_table_insert (data, g_string_new("key2"), _value_new("value2"));
    g_hash_table_insert (data, g_string_new("key3"), _value_new("value3"));
    g_hash_table_insert (data, g_string_new("key4"), _value_new("value4"));
    g_hash_table_insert (data, g_string_new("key5"), _value_new("value5"));
    if (gsignond_secret_storage_update_data (sec_storage, id, method, data)) {
        INFO ("Database data ADDED");
    } else {
        WARN ("Database data CANNOT be ADDED");
    }

    data2 = gsignond_secret_storage_load_data (sec_storage, id, method);
    if (data2) {
        INFO ("Database data LOADED");
        input.table = data;
        input.status = 1;
        g_hash_table_foreach (data2, (GHFunc)_compare_key_value, &input);
        if (input.status != 1) {
            WARN ("Database data DOES NOT MATCH");
        }
        g_hash_table_unref(data2);

    } else {
        WARN ("Database data CANNOT be LOADED");
    }
    g_hash_table_unref(data);

    if (gsignond_secret_storage_remove_data (sec_storage, id, method)) {
        INFO ("Database data REMOVED");
    } else {
        WARN ("Database data CANNOT be REMOVED");
    }

    if (gsignond_secret_storage_clear_db (sec_storage)) {
        INFO ("Database cleared");
    } else {
        WARN ("Database cannot be cleared");
    }

    if (gsignond_secret_storage_close_db (sec_storage)) {
        INFO ("Database closed");
    } else {
        WARN ("Database cannot be closed");
    }
    g_object_unref(storage);
    g_object_unref(config);

    INFO ("Entering main event loop");
    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);

    if (_daemon) g_object_unref (_daemon);

    return 0;
}
