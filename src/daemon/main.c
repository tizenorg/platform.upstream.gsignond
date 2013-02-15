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
#include <sys/socket.h>
#include <glib.h>
#include <gio/gio.h>

#include "gsignond/gsignond-log.h"
#include "daemon/gsignond-daemon.h"
#include "daemon/dbus/gsignond-dbus.h"

static GSignondDaemon *_daemon;
static GIOChannel     *_sig_channel;
static int             _sig_fd[2];

static gboolean 
_handle_unix_signal (GIOChannel *channel,
                     GIOCondition condition,
                     gpointer data)
{
    int signal = 0;
    int ret = read (_sig_fd[1], &signal, sizeof(signal));

    if (ret == -1) {
        ERR ("failed to read signal value: %s", strerror(errno));
        return FALSE;
    }

    switch (signal) {
        case SIGHUP: {
            DBG ("Received SIGHUP");
            //TODO: restart daemon
            break;
        }
        case SIGTERM: {
            DBG ("Received SIGTERM");
            //TODO: stop daemon
            break;
        }
        case SIGINT:  {
            DBG ("Received SIGINT");
            //TODO: stop daemon
            break;
        }
        default: break;
    }

    return TRUE;
}

static void
_setup_signal_handlers ()
{
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, _sig_fd) != 0) {
        ERR( "Couldn't create HUP socketpair");
        return;
    }

    _sig_channel = g_io_channel_unix_new (_sig_fd[0]);
    g_io_add_watch (_sig_channel, 
                    G_IO_IN, 
                    _handle_unix_signal,
                    NULL);
}

static void
_unset_signal_handlers ()
{
    if (_sig_channel) {
        g_io_channel_unref (_sig_channel);
        _sig_channel = 0;
    }

    close (_sig_fd[0]);
    close (_sig_fd[1]);
}

static void 
_signal_handler (int signal)
{
    int ret = write (_sig_fd[0], &signal, sizeof(signal));

    (void) ret;
}

static void 
_install_sighandlers ()
{
    struct sigaction act;

    _setup_signal_handlers();

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
    guint name_owner_id = 0;
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

    _install_sighandlers();

    loop = g_main_loop_new (NULL, FALSE);

    name_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
            GSIGNOND_SERVICE, // "com.google.code.AccountsSSO.SingleSignOn",
            G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
            G_BUS_NAME_OWNER_FLAGS_REPLACE,
            _on_bus_acquired,
            _on_name_acquired,
            _on_name_lost,
            loop, NULL);

    INFO ("Entering main event loop");

    g_main_loop_run (loop);

    _unset_signal_handlers ();
    g_bus_unown_name (name_owner_id);

    if (_daemon) g_object_unref (_daemon);

    return 0;
}
