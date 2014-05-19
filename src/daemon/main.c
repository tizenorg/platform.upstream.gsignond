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

#include "config.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <grp.h>
#include <glib-unix.h>
#include <glib.h>
#include <gio/gio.h>

#include "gsignond/gsignond-log.h"
#include "daemon/dbus/gsignond-dbus.h"
#include "daemon/dbus/gsignond-dbus-server.h"

static GSignondDbusServer *_server = NULL;
static guint           _sig_source_id[3];

static void
_on_server_closed (gpointer data, GObject *server)
{
    _server = NULL;
    if (data) g_main_loop_quit ((GMainLoop *)data);
}

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
    GMainLoop *ml = (GMainLoop *) user_data;

    DBG ("Received reload signal");
    g_return_val_if_fail (ml != NULL, FALSE);
    if (_server) {
        g_object_weak_unref (G_OBJECT(_server), _on_server_closed, ml);
        g_object_unref (_server);
        DBG ("Restarting daemon ....");
        _server = gsignond_dbus_server_new ();
        g_object_weak_ref (G_OBJECT(_server), _on_server_closed, ml);
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
                           main_loop,
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

int main (int argc, char **argv)
{
    GError *error = NULL;
    GMainLoop *main_loop = NULL;
    GOptionContext *opt_context = NULL;
    GOptionEntry opt_entries[] = {
        {NULL }
    };
    gid_t daemon_gid;
    struct group *daemon_group;

    DBG ("before: real gid %d effective gid %d", getgid (), getegid ());
    daemon_gid = getgid ();
    daemon_group = getgrnam ("gsignond");
    if (daemon_group)
        daemon_gid = daemon_group->gr_gid;
    if (setegid (daemon_gid))
        WARN ("setegid() failed");
    DBG ("after: real gid %d effective gid %d", getgid (), getegid ());

    DBG ("before: real uid %d effective uid %d", getuid (), geteuid ());
    if (seteuid (getuid ()))
        WARN ("seteuid() failed");
    DBG ("after: real uid %d effective uid %d", getuid (), geteuid ());

#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

#ifdef ENABLE_DEBUG
    g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
#endif

    opt_context = g_option_context_new ("SSO daemon");
    g_option_context_add_main_entries (opt_context, opt_entries, NULL);
    if (!g_option_context_parse (opt_context, &argc, &argv, &error)) {
        ERR ("Error parsing options: %s", error->message);
        g_error_free (error);
        return -1;
    }

    main_loop = g_main_loop_new (NULL, FALSE);

    _server = gsignond_dbus_server_new ();
    g_option_context_free (opt_context);
    if (!_server) {
        return -1;
    }
    g_object_weak_ref (G_OBJECT (_server), _on_server_closed, main_loop);
    _install_sighandlers(main_loop);
#ifdef USE_P2P
    INFO ("server started at : %s", gsignond_dbus_server_get_address (_server));
#endif
    INFO ("Entering main event loop");

    g_main_loop_run (main_loop);

    if(_server) g_object_unref (_server);
 
    if (main_loop) g_main_loop_unref (main_loop);

    DBG("Clean shutdown");
    return 0;
}
