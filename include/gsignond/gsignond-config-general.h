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

#ifndef __GSIGNOND_CONFIG_GENERAL_H_
#define __GSIGNOND_CONFIG_GENERAL_H_

/**
 * SECTION:gsignond-config-general
 * @title: General configuration
 * @short_description: gSSO general configuration keys
 * @include: gsignond/gsignond-config.h
 *
 * General configuration keys are defined below. See #GSignondConfig for how to use them.
 */

/**
 * GSIGNOND_CONFIG_GENERAL:
 * 
 * A prefix for general keys. Should be used only when defining new keys.
 */
#define GSIGNOND_CONFIG_GENERAL                 "General"

/**
 * GSIGNOND_CONFIG_GENERAL_STORAGE_PATH:
 * 
 * Base path for #GSignondStorageManager to operate in.
 * 
 * Default value: "/var/db", can be overriden in debug 
 * builds by setting SSO_STORAGE_PATH environment variable.
 */
#define GSIGNOND_CONFIG_GENERAL_STORAGE_PATH    GSIGNOND_CONFIG_GENERAL \
                                                "/StoragePath"
/**
 * GSIGNOND_CONFIG_GENERAL_EXTENSION:
 * 
 * The name of the gSSO extension to use. If not specified, the default 
 * implementation is used (see #GSignondExtension).
 * 
 * Can be overriden in debug 
 * builds by setting SSO_EXTENSION environment variable.
 */
#define GSIGNOND_CONFIG_GENERAL_EXTENSION       GSIGNOND_CONFIG_GENERAL \
                                                "/Extension"
/**
 * GSIGNOND_CONFIG_GENERAL_SECURE_DIR:
 * 
 * Path provided by #GSignondStorageManager for storing user-specific
 * information, including secret and metadata databases.
 *
 * This is a run-time value and any value read from configuration file
 * is overwritten.
 * 
 * Value provided by the default implementation: 
 * #GSIGNOND_CONFIG_GENERAL_STORAGE_PATH + "/gsignond." + g_get_user_name().
 */
#define GSIGNOND_CONFIG_GENERAL_SECURE_DIR      GSIGNOND_CONFIG_GENERAL \
                                                "/SecureDir"
/**
 * GSIGNOND_CONFIG_PLUGIN_TIMEOUT:
 * 
 * A timeout in seconds, after which inactive plugin objects and processes are removed.
 * If not set, the plugin objects will persist for possible reuse.
 * 
 * Can be overriden in debug 
 * builds by setting SSO_PLUGIN_TIMEOUT environment variable.
 */
#define GSIGNOND_CONFIG_PLUGIN_TIMEOUT          GSIGNOND_CONFIG_GENERAL \
                                                "/PluginTimeout"

/**
 * GSIGNOND_CONFIG_GENERAL_KEYCHAIN_SYSCTX:
 *
 * System security context of the keychain UI process 
 * (see #GSignondAccessControlManager).
 *
 * Default value can be defined through configure --enable-keychain flag (see
 * <link linkend="gsignond-building">Building gsignond</link>).
 */
#define GSIGNOND_CONFIG_GENERAL_KEYCHAIN_SYSCTX GSIGNOND_CONFIG_GENERAL \
                                                "/KeychainSystemContext"

#endif /* __GSIGNOND_GENERAL_CONFIG_H_ */
