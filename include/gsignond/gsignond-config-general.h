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
 * GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR:
 * 
 * The path where gSSO is looking for available extension modules.
 * 
 * Default value: $(pkglibdir)/extensions, can be overriden in debug 
 * builds by setting SSO_EXTENSIONS_DIR environment variable.
 */
#define GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR  GSIGNOND_CONFIG_GENERAL \
                                                "/ExtensionsDir"
/**
 * GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR:
 * 
 * The path where gSSO is looking for available plugins.
 * 
 * Default value: $(pkglibdir)/plugins, can be overriden in debug 
 * builds by setting SSO_PLUGINS_DIR environment variable.
 */
#define GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR     GSIGNOND_CONFIG_GENERAL \
                                                "/PluginsDir"
/**
 * GSIGNOND_CONFIG_GENERAL_BIN_DIR:
 * 
 * The path where gSSO is looking for the gsignond-plugind binary when it needs
 * to start a plugin process. Can be used for finding other supplementary binaries in 
 * the future.
 * 
 * Default value: $(bindir), can be overriden in debug 
 * builds by setting SSO_BIN_DIR environment variable.
 */
#define GSIGNOND_CONFIG_GENERAL_BIN_DIR         GSIGNOND_CONFIG_GENERAL \
                                                "/BinDir"
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

#endif /* __GSIGNOND_GENERAL_CONFIG_H_ */
