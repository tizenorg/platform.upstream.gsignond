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

#include "gsignond/gsignond-log.h"
#include "daemon/gsignond-daemon.h"

GSignondDaemon *_daemon;

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
    GMainLoop *loop = (GMainLoop *) user_data;
    INFO ("Lost (or failed to acquire) the name '%s' on the session message bus", name);
    g_main_loop_quit (loop);
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
    GOptionContext *opt_context = NULL;
    gint ret = 0;
    guint name_owner_id = 0;
    guint sigint_id =  0;
    GOptionEntry opt_entries[] = {
        {NULL }
    };
    GMainLoop *loop = 0;

    g_type_init ();

    opt_context = g_option_context_new ("SSO daemon");
    g_option_context_add_main_entries (opt_context, opt_entries, NULL);
    if (!g_option_context_parse (opt_context, &argc, &argv, &error)) {
        ERR ("Error parsing options: %s", error->message);
        g_error_free (error);
        return -1;
    }

    //_install_sighandlers();

    loop = g_main_loop_new (NULL, FALSE);

    name_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
            "com.google.code.AccountsSSO.SingleSignOn",
            G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
            G_BUS_NAME_OWNER_FLAGS_REPLACE,
            _on_bus_acquired,
            _on_name_acquired,
            _on_name_lost,
            loop, NULL);

    INFO ("Entering main event loop");

    g_main_loop_run (loop);

    if (_daemon) g_object_unref (_daemon);

    return 0;
}
