/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Amarnath Valluri <amarnath.valluri@linux.intel.com>
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

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <glib-unix.h>
#include <glib.h>
#include <gio/gio.h>

#include "gsignond/gsignond-log.h"
#include "daemon/gsignond-daemon.h"
#include "daemon/dbus/gsignond-dbus.h"

static GSignondDaemon *_daemon;
static guint           _sig_source_id[3];

static gboolean
_handle_quit_signal (gpointer user_data)
{
    GMainLoop *ml = (GMainLoop *) user_data;

    g_return_val_if_fail (ml != NULL, FALSE);
    DBG ("Received quit signal");
    if (ml) g_main_loop_quit (ml);

    return FALSE;
}

static gboolean
_handle_reload_signal (gpointer user_data)
{
    GSignondDaemon **daemon = (GSignondDaemon **) user_data;

    DBG ("Received reload signal");
    g_return_val_if_fail (daemon != NULL, FALSE);
    if (*daemon) {
        g_object_unref (*daemon);
        DBG ("Restarting daemon ....");
        *daemon = gsignond_daemon_new ();
    }

    return TRUE;
}

static void 
_install_sighandlers (GMainLoop *main_loop)
{
    GSource *source = NULL;
    GMainContext *ctx = g_main_loop_get_context (main_loop);

    source = g_unix_signal_source_new (SIGHUP);
    g_source_set_callback (source,
                           _handle_reload_signal,
                           &_daemon,
                           NULL);
    _sig_source_id[0] = g_source_attach (source, ctx);
    source = g_unix_signal_source_new (SIGTERM);
    g_source_set_callback (source,
                           _handle_quit_signal,
                           main_loop,
                           NULL);
    _sig_source_id[1] = g_source_attach (source, ctx);
    source = g_unix_signal_source_new (SIGINT);
    g_source_set_callback (source,
                           _handle_quit_signal,
                           main_loop,
                           NULL);
    _sig_source_id[2] = g_source_attach (source, ctx);
}

static void
_on_bus_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    INFO ("Connected to the session bus");
    if (connection != NULL) 
        _daemon = gsignond_daemon_new ();
}

static void
_on_name_lost (GDBusConnection *connection,
               const gchar     *name,
               gpointer         user_data)
{
    GMainLoop *ml = (GMainLoop *) user_data;
    INFO ("Lost (or failed to acquire) the name '%s' on the session message bus", name);
    if (ml) g_main_loop_quit (ml);
}

static void
_on_name_acquired (GDBusConnection *connection,
                   const gchar     *name,
                   gpointer         user_data)
{
    INFO ("Acquired the name %s on the session message bus", name);
}

int main (int argc, char **argv)
{
    GError *error = NULL;
    GMainLoop *main_loop = NULL;
    GOptionContext *opt_context = NULL;
    guint name_owner_id = 0;
    GOptionEntry opt_entries[] = {
        {NULL }
    };

    g_type_init ();

    opt_context = g_option_context_new ("SSO daemon");
    g_option_context_add_main_entries (opt_context, opt_entries, NULL);
    if (!g_option_context_parse (opt_context, &argc, &argv, &error)) {
        ERR ("Error parsing options: %s", error->message);
        g_error_free (error);
        return -1;
    }

    main_loop = g_main_loop_new (NULL, FALSE);
    _install_sighandlers(main_loop);

    name_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
            GSIGNOND_SERVICE, // "com.google.code.AccountsSSO.SingleSignOn",
            G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
            G_BUS_NAME_OWNER_FLAGS_REPLACE,
            _on_bus_acquired,
            _on_name_acquired,
            _on_name_lost,
            main_loop, NULL);

    INFO ("Entering main event loop");

    g_main_loop_run (main_loop);

    g_bus_unown_name (name_owner_id);

    if (_daemon) g_object_unref (G_OBJECT (_daemon));
    if (main_loop) g_main_loop_unref (main_loop);

    return 0;
}
