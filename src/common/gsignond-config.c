/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *          Amarnath Valluri <amarnath.valluri@linux.intel.com>
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

#include <stdlib.h>
#include <unistd.h>
#include <glib/gstdio.h>

#include "config.h"

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-config-general.h"
#include "gsignond/gsignond-config-dbus.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-dictionary.h"

/**
 * SECTION:gsignond-config
 * @short_description: gSSO configuration information
 * @include: gsignond/gsignond-config.h
 *
 * #GSignondConfig holds configuration information as a set of keys and values
 * (integer or strings). The key names are defined in 
 * <link linkend="gsignond-General-configuration">general config keys</link>,
 * and <link linkend="gsignond-DBus-configuration">DBus config keys</link>.
 * 
 * The configuration is retrieved from the gSSO configuration file. See below 
 * for where the file is searched for.
 * 
 * <refsect1><title>Where the configuration file is searched for</title></refsect1>
 * 
 * If gSSO has been compiled with --enable-debug, then these locations are used,
 * in decreasing order of priority:
 * - GSIGNOND_CONFIG environment variable
 * - g_get_user_config_dir() + "gsignond/gsignond.conf"
 * - each of g_get_system_config_dirs() + "gsignond/gsignond.conf"
 * 
 * Otherwise, the config file location is determined at compilation time as 
 * $(sysconfdir) + "gsignond/gsignond.conf"
 * 
 * <refsect1><title>Example configuration file</title></refsect1>
 * 
 * See example configuration file here:
 * <ulink url="http://code.google.com/p/accounts-sso/source/browse/gsignond.conf.in?repo=gsignond">
 * http://code.google.com/p/accounts-sso/source/browse/gsignond.conf.in?repo=gsignond</ulink>
 */

/**
 * GSignondConfig:
 *
 * Opaque structure for the object.
 */
/**
 * GSignondConfigClass:
 *
 * Opaque structure for the class.
 */


struct _GSignondConfigPrivate
{
    gchar *config_file_path;
    GSignondDictionary *config_table;
};

#define GSIGNOND_CONFIG_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_CONFIG, GSignondConfigPrivate)

G_DEFINE_TYPE (GSignondConfig, gsignond_config, G_TYPE_OBJECT);

static gboolean
_load_config (GSignondConfig *self)
{
    gchar *def_config;
    GError *err = NULL;
    gchar **groups = NULL;
    gsize n_groups = 0;
    int i,j;
    GKeyFile *settings = g_key_file_new ();

#   ifdef ENABLE_DEBUG
    const gchar * const *sysconfdirs;

    if (!self->priv->config_file_path) {
        def_config = g_strdup (g_getenv ("GSIGNOND_CONFIG"));
        if (!def_config)
            def_config = g_build_filename (g_get_user_config_dir(),
                                           "gsignond/gsignond.conf",
                                           NULL);
        if (g_access (def_config, R_OK) == 0) {
            self->priv->config_file_path = def_config;
        } else {
            g_free (def_config);
            sysconfdirs = g_get_system_config_dirs ();
            while (*sysconfdirs != NULL) {
                def_config = g_build_filename (*sysconfdirs,
                                               "gsignond/gsignond.conf",
                                               NULL);
                if (g_access (def_config, R_OK) == 0) {
                    self->priv->config_file_path = def_config;
                    break;
                }
                g_free (def_config);
                sysconfdirs++;
            }
        }
    }
#   else  /* ENABLE_DEBUG */
#   ifndef GSIGNOND_SYSCONF_DIR
#   error "System configuration directory not defined!"
#   endif
    def_config = g_build_filename (GSIGNOND_SYSCONF_DIR,
                                   "gsignond/gsignond.conf",
                                   NULL);
    if (g_access (def_config, R_OK) == 0) {
        self->priv->config_file_path = def_config;
    } else {
        g_free (def_config);
    }
#   endif  /* ENABLE_DEBUG */

    if (self->priv->config_file_path) {
        DBG ("Loading SSO config from %s", self->priv->config_file_path);
        if (!g_key_file_load_from_file (settings,
                                        self->priv->config_file_path,
                                        G_KEY_FILE_NONE, &err)) {
            WARN ("error reading config file at '%s': %s",
                 self->priv->config_file_path, err->message);
            g_error_free (err);
            g_key_file_free (settings);
            return FALSE;
        }
    }

    groups = g_key_file_get_groups (settings, &n_groups);

    for (i = 0; i < n_groups; i++) {
        GError *err = NULL;
        gsize n_keys =0;
        gchar **keys = g_key_file_get_keys (settings,
                                            groups[i],
                                            &n_keys,
                                            &err);
        if (err) {
            WARN ("fail to read group '%s': %s", groups[i], err->message);
            g_error_free (err);
            continue;
        }

        for (j = 0; j < n_keys; j++) {
            gchar *key = g_strdup_printf ("%s/%s", groups[i], keys[j]);
            gchar *value = g_key_file_get_value (settings,
                                                 groups[i],
                                                 keys[j],
                                                 &err);
            if (err) {
                WARN ("fail to read key '%s/%s': %s", groups[i], keys[j], err->message);
                g_error_free (err);
                continue;
            }

            INFO ("found config : '%s/%s' - '%s'", groups[i], keys[j], value);
            gsignond_config_set_string (self, key, value);

            g_free (key);
            g_free (value);
        }

        g_strfreev (keys);
    }

    g_strfreev (groups);

    g_key_file_free (settings);

    return TRUE;
}

/**
 * gsignond_config_get_integer:
 * @self: an instance of #GSignondConfig
 * @key: the key name
 * 
 * Get an integer configuration value.
 * 
 * Returns: the value corresponding to the key as an integer. If the key does not
 * exist or cannot be converted to the integer, 0 is returned.
 */
gint
gsignond_config_get_integer (GSignondConfig *self, const gchar *key)
{
    const gchar *str_value = gsignond_config_get_string (self, key);
    return (gint) (str_value ? atoi (str_value) : 0);
}

/**
 * gsignond_config_set_integer:
 * @self: an instance of #GSignondConfig
 * @key: the key name
 * @value: the value
 * 
 * Sets the configuration value to the provided integer.
 */
void
gsignond_config_set_integer (GSignondConfig *self, const gchar *key,
                             gint value) 
{
    gchar *s_value = 0;
    g_return_if_fail (self && GSIGNOND_IS_CONFIG (self));

    s_value = g_strdup_printf ("%d", value);
    if (!s_value) return;

    gsignond_config_set_string (self, (gpointer) key, s_value);

    g_free (s_value);

}

/**
 * gsignond_config_get_string:
 * @self: an instance of #GSignondConfig
 * @key: the key name
 * 
 * Get a string configuration value.
 * 
 * Returns: (transfer none): the value corresponding to the key as string. If the key does not
 * exist, NULL is returned.
 */
const gchar *
gsignond_config_get_string (GSignondConfig *self, const gchar *key)
{
    g_return_val_if_fail (self && GSIGNOND_IS_CONFIG (self), NULL);

    GVariant* value = gsignond_dictionary_get (self->priv->config_table,
                                               (gpointer) key);
    if (!value) return NULL;

    return g_variant_get_string (value, NULL);
}

/**
 * gsignond_config_set_string:
 * @self: an instance of #GSignondConfig
 * @key: the key name
 * @value: (transfer none): the value
 * 
 * Sets the configuration value to the provided string.
 */
void
gsignond_config_set_string (GSignondConfig *self, const gchar *key,
                            const gchar *value) 
{
    g_return_if_fail (self && GSIGNOND_IS_CONFIG (self));

    gsignond_dictionary_set (self->priv->config_table,
                             (gpointer) key,
                             g_variant_new_string (value));

}

static void
gsignond_config_dispose (GObject *object)
{
    GSignondConfig *self = 0;
    g_return_if_fail (object && GSIGNOND_IS_CONFIG (object));

    self = GSIGNOND_CONFIG (object);

    if (self->priv->config_table) {
        gsignond_dictionary_unref (self->priv->config_table);
        self->priv->config_table = NULL;
    }

    G_OBJECT_CLASS (gsignond_config_parent_class)->dispose (object);
}

static void
gsignond_config_finalize (GObject *object)
{
    GSignondConfig *self = 0;
    g_return_if_fail (object && GSIGNOND_IS_CONFIG (object));

    self = GSIGNOND_CONFIG (object);

    if (self->priv->config_file_path) {
        g_free (self->priv->config_file_path);
        self->priv->config_file_path = NULL;
    }

    G_OBJECT_CLASS (gsignond_config_parent_class)->finalize (object);
}

static void
gsignond_config_init (GSignondConfig *self)
{
    self->priv = GSIGNOND_CONFIG_PRIV (self);

    self->priv->config_file_path = NULL;
    self->priv->config_table = gsignond_dictionary_new();
    
    if (!_load_config (self))
        WARN ("load configuration failed, using default settings");
}

static void
gsignond_config_class_init (GSignondConfigClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondConfigPrivate));

    object_class->dispose = gsignond_config_dispose;
    object_class->finalize = gsignond_config_finalize;

}

/**
 * gsignond_config_new:
 * 
 * Create a #GSignondConfig object.
 * 
 * Returns: an instance of #GSignondConfig. gSSO extensions should not use this
 * as they're already provided with a config object when they're created.
 */
GSignondConfig *
gsignond_config_new ()
{
    return GSIGNOND_CONFIG (g_object_new (GSIGNOND_TYPE_CONFIG, NULL));
}

