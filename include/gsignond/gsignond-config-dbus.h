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

/**
 * SECTION:gsignond-config-dbus
 * @title: DBus configuration
 * @short_description: gSSO dbus configuration keys
 * @include: gsignond/gsignond-config.h
 *
 * DBus configuration keys are defined below. See #GSignondConfig for how to use them.
 */

/**
 * GSIGNOND_CONFIG_DBUS_TIMEOUTS:
 * 
 * A prefix for dbus timeout keys. Should be used only when defining new keys.
 */
#define GSIGNOND_CONFIG_DBUS_TIMEOUTS  "ObjectTimeouts"
/**
 * GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT:
 * 
 * A timeout in seconds, after which the gSSO daemon will exit. If not set, 
 * the daemon will not exit. Has no effect if P2P DBus is in use.
 * 
 * Can be overriden in debug 
 * builds by setting SSO_DAEMON_TIMEOUT environment variable.
 */
#define GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT     GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                "/DaemonTimeout"
/**
 * GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT:
 * 
 * A timeout in seconds, after which inactive identity dbus objects will be removed.
 * If not set, the dbus objects will persist.
 * 
 * Can be overriden in debug 
 * builds by setting SSO_IDENTITY_TIMEOUT environment variable.
 */
#define GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT   GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                "/IdentityTimeout"
/**
 * GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT:
 * 
 * A timeout in seconds, after which inactive authentication session dbus objects will be removed.
 * If not set, the dbus objects will persist.
 * 
 * Can be overriden in debug 
 * builds by setting SSO_AUTH_SESSION_TIMEOUT environment variable.
 */
#define GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT GSIGNOND_CONFIG_DBUS_TIMEOUTS \
                                                  "/AuthSessionTimeout"
#endif /* __GSIGNOND_CONFIG_DBUS_H_ */
