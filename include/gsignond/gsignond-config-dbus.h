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

#ifndef __GSIGNOND_CONFIG_DBUS_H_
#define __GSIGNOND_CONFIG_DBUS_H_

#define GSIGNOND_CONFIG_DBUS_TIMEOUTS  "ObjectTimeouts"

#define GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT     GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                "/DaemonTimeout"
#define GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT   GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                "/IdentityTimeout"
#define GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                  "/AuthSessionTimeout"
#define GSIGNOND_CONFIG_DBUS_PLUGIN_TIMEOUT     GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                  "/PluginTimeout"
#endif /* __GSIGNOND_CONFIG_DBUS_H_ */
