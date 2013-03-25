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

#ifndef __GSIGNOND_DBUS_H_
#define __GSIGNOND_DBUS_H_

/*
 * Common DBUS definitions
 */
#define GSIGNOND_DBUS_ADDRESS            "unix:abstract=gsignond"
#define GSIGNOND_SERVICE_PREFIX          "com.google.code.AccountsSSO.gSingleSignOn"
#define GSIGNOND_SERVICE                 GSIGNOND_SERVICE_PREFIX
#define GSIGNOND_DAEMON_OBJECTPATH       "/com/google/code/AccountsSSO/gSingleSignOn"
#define GSIGNOND_DAEMON_INTERFACE        GSIGNOND_SERVICE_PREFIX ".AuthService"
#define GSIGNOND_IDENTITY_INTERFACE      GSIGNOND_SERVICE_PREFIX ".Identity"
#define GSIGNOND_AUTH_SESSION_INTERFACE  GSIGNOND_SERVICE_PREFIX ".AuthSession"

#define SIGNONUI_SERVICE                 "org.tizen.SSO"
#define SIGNONUI_OBJECTPATH              "/org/tizen/SSO/SignonUi"
#define SIGNONUI_INTERFACE               "org.tizen.SSO.singlesignonui"

#endif /* __GSIGNOND_DBUS_H_ */
