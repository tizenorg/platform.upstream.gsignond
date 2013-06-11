/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@linux.intel.com>
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

#ifndef __GSIGNOND_PLUGIN_REMOTE_PRIVATE_H__
#define __GSIGNOND_PLUGIN_REMOTE_PRIVATE_H__

#include <glib.h>
#include <daemon/dbus/gsignond-dbus-remote-plugin-gen.h>
#include "daemon/dbus/gsignond-dbus.h"

G_BEGIN_DECLS

struct _GSignondPluginRemotePrivate
{
    GDBusConnection   *connection;
    GSignondDbusRemotePlugin *dbus_plugin_proxy;
    gchar *plugin_type;
    gchar **plugin_mechanisms;
    GPid cpid;
    guint child_watch_id;

    GIOChannel *err_watch_ch;
    guint err_watch_id;

    GMainLoop *main_loop;
    gboolean is_plugind_up;

    gboolean unref_in_down_cb;

    /* Signals */
    guint signal_response;
    guint signal_response_final;
    guint signal_store;
    guint signal_error;
    guint signal_user_action_required;
    guint signal_refreshed;
    guint signal_status_changed;
};

G_END_DECLS

#endif /* __GSIGNOND_PLUGIN_REMOTE_PRIVATE_H__ */
