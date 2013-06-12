/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@intel.com>
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

#include <config.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <glib-unix.h>
#include <glib.h>
#include <gio/gio.h>
#include <sys/prctl.h>

#include "gsignond/gsignond-log.h"
#include "daemon/dbus/gsignond-dbus.h"
#include "gsignond-plugin-daemon.h"

static GSignondPluginDaemon *_daemon = NULL;
static guint _sig_source_id[3];

static void
_on_daemon_closed (gpointer data, GObject *server)
{
    _daemon = NULL;
    DBG ("Daemon closed");
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

static void 
_install_sighandlers (GMainLoop *main_loop)
{
    GSource *source = NULL;
    GMainContext *ctx = g_main_loop_get_context (main_loop);

    source = g_unix_signal_source_new (SIGTERM);
    g_source_set_callback (source,
                           _handle_quit_signal,
                           main_loop,
                           NULL);
    _sig_source_id[0] = g_source_attach (source, ctx);

    source = g_unix_signal_source_new (SIGINT);
    g_source_set_callback (source,
                           _handle_quit_signal,
                           main_loop,
                           NULL);
    _sig_source_id[1] = g_source_attach (source, ctx);

    source = g_unix_signal_source_new (SIGHUP);
    g_source_set_callback (source,
                           _handle_quit_signal,
                           main_loop,
                           NULL);
    _sig_source_id[2] = g_source_attach (source, ctx);

    if (prctl(PR_SET_PDEATHSIG, SIGHUP))
        WARN ("failed to set parent death signal");
}

int main (int argc, char **argv)
{
    GError *error = NULL;
    GMainLoop *main_loop = NULL;
    GOptionContext *opt_context = NULL;
    gchar **plugin_args = NULL;
    gint up_signal = -1;
    gint in_fd = 0, out_fd = 1;
    GOptionEntry opt_entries[] = {
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &plugin_args,
                "Plugin Args", NULL},
        {NULL}
    };

    /* Duplicates stdin,stdout,stderr descriptors and point the descriptors
     * to /dev/null to avoid anyone writing to descriptors before initial
     * "plugind-is-ready" notification is sent to gsignond
     * */
    in_fd = dup(0);
    if (in_fd == -1) {
        WARN ("Failed to dup stdin : %s(%d)", strerror(errno), errno);
        in_fd = 0;
    }
    if (!freopen("/dev/null", "r+", stdin)) {
        WARN ("Unable to redirect stdin to /dev/null");
    }

    out_fd = dup(1);
    if (out_fd == -1) {
        WARN ("Failed to dup stdout : %s(%d)", strerror(errno), errno);
        out_fd = 1;
    }

    /* Reattach stderr to stdout */
    dup2 (2, 1);

#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    opt_context = g_option_context_new ("<plugin_path> <plugin_name>");
    g_option_context_set_summary (opt_context, "gSSO helper plugin daemon");
    g_option_context_add_main_entries (opt_context, opt_entries, NULL);
    g_option_context_set_ignore_unknown_options (opt_context, TRUE);
    g_option_context_parse (opt_context, &argc, &argv, &error);
    g_option_context_free (opt_context);
    if (error) {
        WARN ("Error in arguments parsing: %s", error->message);
        g_error_free (error);
    }
    if (!plugin_args || !plugin_args[0] || !plugin_args[1]) {
        WARN ("plugin path or plugin type missing");
        if (write (out_fd, "0", sizeof(char)) == -1)
            WARN ("Unable to write down notification to stdout");
        if (in_fd != 0) close (in_fd);
        if (out_fd != 1) close (out_fd);
        if (plugin_args) g_strfreev(plugin_args);
        return -1;
    }

    _daemon = gsignond_plugin_daemon_new (plugin_args[0], plugin_args[1], in_fd,
            out_fd);
    g_strfreev(plugin_args);
    if (_daemon == NULL) {
        if (write (out_fd, "0", sizeof(char)) == -1)
            WARN ("Unable to write down notification to stdout");
        if (in_fd != 0) close (in_fd);
        if (out_fd != 1) close (out_fd);
        return -1;
    }

    main_loop = g_main_loop_new (NULL, FALSE);
    g_object_weak_ref (G_OBJECT (_daemon), _on_daemon_closed, main_loop);
    _install_sighandlers (main_loop);

    /* Notification for gsignond that plugind is up and ready */
    up_signal = write (out_fd, "1", sizeof(char));

    if (up_signal == -1) {
        g_main_loop_unref (main_loop);
        g_object_unref (_daemon);
        return -1;
    }

    DBG ("Entering main event loop");

    g_main_loop_run (main_loop);

    if(_daemon) {
        g_object_unref (_daemon);
    }

    if (main_loop) {
        g_main_loop_unref (main_loop);
    }

    return 0;
}
