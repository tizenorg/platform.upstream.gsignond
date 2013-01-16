/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
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

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-config-general.h"
#include "gsignond/gsignond-config-db.h"
#include "gsignond/gsignond-config-dbus.h"
#include "gsignond/gsignond-log.h"

enum
{
    PROP_0,
    PROP_CONFIG_TABLE,         /* key-value pair of all config options */
    PROP_CONFIG_FILE_PATH,     /* configuration file path */
    PROP_PLUGINS_DIR,          /* plugins direcotry location */
    PROP_EXTENSIONS_DIR,       /* extensions directory location */
    PROP_EXTENSION,            /* active extension */
    PROP_DAEMON_TIMEOUT,       /* daemon dbus timeout */
    PROP_IDENTITY_TIMEOUT,     /* identities dbus timeout */
    PROP_AUTH_SESSION_TIMEOUT, /* session dbus timeout */
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES];

struct _GSignondConfigPrivate
{
    gchar *config_file_path;
    GHashTable *config_table;
};

#define GSIGNOND_CONFIG_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_CONFIG, GSignondConfigPrivate)

G_DEFINE_TYPE (GSignondConfig, gsignond_config, G_TYPE_OBJECT);


static gboolean gsignond_config_load (GSignondConfig *config);
static gboolean gsignond_config_load_environment (GSignondConfig *config);

static gint
gsignond_config_get_integer (GSignondConfig *config, const gchar *key)
{
    const gchar *value = g_hash_table_lookup (config->priv->config_table, key);
    if (!value) return 0;

    return (guint) atoi (value);
}

static gboolean
gsignond_config_set_integer (GSignondConfig *config, const gchar *key,
                             gint value) 
{
    gchar *s_value = 0;
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG(config), FALSE);

    s_value = g_strdup_printf ("%d", value);
    if (!s_value) return FALSE;

    g_hash_table_replace (config->priv->config_table,
                          (gpointer) key,
                          (gpointer) s_value);

    g_free (s_value);

    return FALSE;
}


static void
gsignond_config_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    GSignondConfig *self = GSIGNOND_CONFIG (object);

    switch (property_id)
    {
        case PROP_CONFIG_TABLE:
            g_value_set_pointer (value, self->priv->config_table);
            break;
        case PROP_CONFIG_FILE_PATH:
            g_value_set_string (value, self->priv->config_file_path);
            break;
        case PROP_PLUGINS_DIR:
            g_value_set_string (value, gsignond_config_get_plugins_dir (self));
            break;
        case PROP_EXTENSIONS_DIR:
            g_value_set_string (value,
                                gsignond_config_get_extensions_dir (self));
            break;
        case PROP_EXTENSION:
            g_value_set_string (value,
                                gsignond_config_get_extension (self));
            break;
        case PROP_DAEMON_TIMEOUT:
            g_value_set_uint (value,
                              gsignond_config_get_daemon_timeout (self));
            break;
        case PROP_IDENTITY_TIMEOUT:
            g_value_set_uint (value,
                              gsignond_config_get_identity_timeout (self));
            break;
        case PROP_AUTH_SESSION_TIMEOUT:
            g_value_set_uint (value,
                              gsignond_config_get_auth_session_timeout (self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_config_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    GSignondConfig *self = GSIGNOND_CONFIG (object);

    switch (property_id)
    {
        case PROP_CONFIG_FILE_PATH:
            self->priv->config_file_path =
                g_strdup (g_value_get_string (value));
            gsignond_config_load (self);
            break;
        case PROP_PLUGINS_DIR:
            gsignond_config_set_plugins_dir (self, g_value_get_string (value));
            break;
        case PROP_EXTENSIONS_DIR:
            gsignond_config_set_extensions_dir (self,
                                                g_value_get_string (value));
            break;
        case PROP_EXTENSION:
            gsignond_config_set_extension (self,
                                           g_value_get_string (value));
            break;
        case PROP_DAEMON_TIMEOUT:
            gsignond_config_set_daemon_timeout (self, g_value_get_uint (value));
            break;
        case PROP_IDENTITY_TIMEOUT:
            gsignond_config_set_identity_timeout (self,
                                                  g_value_get_uint (value));
            break;
        case PROP_AUTH_SESSION_TIMEOUT:
            gsignond_config_set_auth_session_timeout (self,
                                                      g_value_get_uint (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_config_dispose (GObject *object)
{
    GSignondConfig *self = 0;
    g_return_if_fail (object && GSIGNOND_IS_CONFIG (object));

    self = GSIGNOND_CONFIG (object);

    if (self->priv->config_table) {
        g_hash_table_unref (self->priv->config_table);
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
    self->priv->config_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                      g_free, g_free);
}

static void
gsignond_config_class_init (GSignondConfigClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondConfigPrivate));

    object_class->get_property = gsignond_config_get_property;
    object_class->set_property = gsignond_config_set_property;
    object_class->dispose = gsignond_config_dispose;
    object_class->finalize = gsignond_config_finalize;

    properties[PROP_CONFIG_TABLE] =
        g_param_spec_pointer ("config-table",
                              "configuration key-value pairs",
                              "All configuration options",
                              G_PARAM_READABLE);

    properties[PROP_CONFIG_FILE_PATH] =
        g_param_spec_string ("config-file-path",
                             "Daemon Config Path",
                             "Daemon configuration file path",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    properties[PROP_PLUGINS_DIR] =
        g_param_spec_string ("plugins-dir",
                             "Plugins Directory",
                             "Plugins direcotry",
                             GSIGNOND_PLUGINS_DIR, 
                             G_PARAM_READWRITE);

    properties[PROP_EXTENSIONS_DIR] =
        g_param_spec_string ("extensions-dir",
                             "Extensions Directroy",
                             "Extensions directory",
                             GSIGNOND_EXTENSIONS_DIR, 
                             G_PARAM_READWRITE);

    properties[PROP_EXTENSION] =
        g_param_spec_string ("extension",
                             "Active extension",
                             "Active extension",
                             "",
                             G_PARAM_READWRITE);

    properties[PROP_DAEMON_TIMEOUT] =
        g_param_spec_uint ("daemon-timeout",
                           "Daemon Timeout",
                           "Daemon timeout",
                           0, /* TODO: replace with minimum timeout */
                           G_MAXINT, /* TODO: replace with maximum timeout */
                           0, /* no timeout */
                           G_PARAM_READWRITE);

    properties[PROP_IDENTITY_TIMEOUT] =
        g_param_spec_uint ("identity-timeout",
                           "Identity Timeout",
                           "Identity timeout",
                           0, /* TODO: replace with minimum timeout */
                           G_MAXUINT, /* TODO: replace with maximum timeout */
                           300, /* 5 minutes */
                           G_PARAM_READWRITE);

    properties[PROP_AUTH_SESSION_TIMEOUT] =
        g_param_spec_uint ("auth-session-timeout",
                           "Auth Session Timeout",
                           "Authentication session timeout",
                           0, /* TODO: replace with minimum timeout */
                           G_MAXUINT, /* TODO: replace with maximum timeout */
                           300, /* 5 minutes */
                           G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static gboolean
gsignond_config_load (GSignondConfig *self)
{
    gchar *def_config;
    const gchar * const *sysconfdirs;
    GError *err = NULL;
    gchar **groups = NULL;
    gsize n_groups = 0;
    int i,j;
    GKeyFile *settings = g_key_file_new ();

    if (!self->priv->config_file_path) {
        def_config = g_getenv ("GSIGNOND_CONFIG");
        if (!def_config)
            def_config = g_build_filename (g_get_user_config_dir(),
                                           "gsignond/gsignond.conf",
                                           NULL);
        if (g_access (def_config, R_OK) == 0) {
            self->priv->config_file_path = def_config;
        } else {
            sysconfdirs = g_get_system_config_dirs ();
            while (*sysconfdirs != NULL) {
                g_free (def_config);
                def_config = g_build_filename (*sysconfdirs,
                                               "gsignond/gsignond.conf",
                                               NULL);
                if (g_access (def_config, R_OK) == 0) {
                    self->priv->config_file_path = def_config;
                    break;
                }
                sysconfdirs++;
            }
        }
    }

    if (self->priv->config_file_path) {
        DBG ("Loading SSO config from %s", self->priv->config_file_path);
        if (!g_key_file_load_from_file (settings, self->priv->config_file_path,
                                        G_KEY_FILE_NONE, &err)) {
            WARN ("error reading config file at '%s': %s",
                 self->priv->config_file_path, err->message);
            g_error_free (err);
            g_key_file_free (settings);
            return FALSE;
        }
    }

    groups = g_key_file_get_groups (settings, &n_groups);

    for (i = 0; i<n_groups; i++) {
        GError *err = NULL;
        gsize n_keys =0;
        gchar **keys = g_key_file_get_keys (settings, groups[i], &n_keys, &err);

        if (err) {
            WARN ("fail to read group '%s': %s", groups[i], err->message);
            g_error_free (err);
            continue;
        }

        for (j=0; j<n_keys; j++) {
            gchar *key = g_strdup_printf ("%s/%s", groups[i], keys[j]);
            gchar *value = g_key_file_get_value (settings, groups[i], keys[j],
                                                 &err);

            if (err) {
                WARN ("fail to read key '%s/%s': %s", groups[i], keys[j], err->message);
                g_error_free (err);
                continue;
            }

            INFO ("found config : '%s/%s' - '%s'", groups[i], keys[j], value);

            g_hash_table_insert (self->priv->config_table, key, value);

            g_free (key);
            g_free (value);
        }

        g_strfreev (keys);
    }

    g_strfreev (groups);

    g_key_file_free (settings);

    /*
     * FIXME: Find the right place to load environment
     */
    gsignond_config_load_environment (self);

    return TRUE;
}

static gboolean
gsignond_config_load_environment (GSignondConfig *config)
{
    const char *e_val = 0;
    guint timeout = 0;
    gint level = 0;
    
    e_val = g_getenv ("SSO_DAEMON_TIMEOUT");
    if (e_val && (timeout = atoi(e_val)))
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT,
                             (gpointer) e_val);

    e_val = g_getenv ("SSO_IDENTITY_TIMEOUT");
    if (e_val && (timeout = atoi(e_val)))
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT,
                             (gpointer) e_val);

    e_val = g_getenv ("SSO_AUTHSESSION_TIMEOUT");
    if (e_val && (timeout = atoi(e_val)))
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT,
                             (gpointer) e_val);

    e_val = g_getenv ("SSO_LOGGING_LEVEL");
    if (e_val && (level = atoi(e_val)))
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_LOG_LEVEL,
                             (gpointer) e_val);
        //set_logging_level (level);
    
    e_val = g_getenv ("SSO_PLUGINS_DIR");
    if (e_val) 
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR,
                             (gpointer) e_val);

    e_val = g_getenv ("SSO_EXTENSIONS_DIR");
    if (e_val) 
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR,
                             (gpointer) e_val);

    e_val = g_getenv ("SSO_EXTENSION");
    if (e_val)
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_EXTENSION,
                             (gpointer) e_val);

    e_val = g_getenv ("SSO_STORAGE_PATH");
    if (e_val) {
        g_hash_table_insert (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_STORAGE_PATH,
                             (gpointer) e_val);
        /* cam_config_set_storage_path (e_val); */
    }
}

gboolean
gsignond_config_set_plugins_dir (GSignondConfig *config, const gchar *dir)
{
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG (config), FALSE);
    g_return_val_if_fail (dir, FALSE);

    const gchar *plugins_dir =
        g_hash_table_lookup (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR);

    if (plugins_dir && !g_strcmp0 (plugins_dir, dir)) {
        return FALSE;
    }

    g_hash_table_replace (config->priv->config_table,
                          GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR, (gpointer) dir);
    g_object_notify_by_pspec (G_OBJECT (config), properties[PROP_PLUGINS_DIR]);

    return TRUE;
}

const gchar *
gsignond_config_get_plugins_dir (GSignondConfig *config)
{
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG (config), 0);

    return (const gchar *) g_hash_table_lookup (config->priv->config_table,
                                           GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR);
}

gboolean
gsignond_config_set_extensions_dir (GSignondConfig *config, const gchar *dir)
{
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG (config), FALSE);
    g_return_val_if_fail (dir, FALSE);

    const gchar *extensions_dir =
        g_hash_table_lookup (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR);

    if (extensions_dir && !g_strcmp0 (extensions_dir, dir)) {
        return FALSE;
    }

    g_hash_table_replace (config->priv->config_table,
                          GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR,
                          (gpointer) dir);
    g_object_notify_by_pspec (G_OBJECT (config),
                              properties[PROP_EXTENSIONS_DIR]);

    return TRUE;
}

const gchar *
gsignond_config_get_extensions_dir (GSignondConfig *config)
{
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG (config), 0);

    return (const gchar *) g_hash_table_lookup (config->priv->config_table,
                                        GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR);
}

gboolean
gsignond_config_set_extension (GSignondConfig *config, const gchar *extension)
{
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG (config), FALSE);
    g_return_val_if_fail (extension, FALSE);

    const gchar *cur_ext =
        g_hash_table_lookup (config->priv->config_table,
                             GSIGNOND_CONFIG_GENERAL_EXTENSION);

    if (cur_ext && !g_strcmp0 (cur_ext, extension)) {
        return FALSE;
    }

    g_hash_table_replace (config->priv->config_table,
                          GSIGNOND_CONFIG_GENERAL_EXTENSION,
                          (gpointer) extension);

    return TRUE;
}

const gchar *
gsignond_config_get_extension (GSignondConfig *config)
{
    g_return_val_if_fail (config && GSIGNOND_IS_CONFIG (config), 0);

    return (const gchar *) g_hash_table_lookup (config->priv->config_table,
                                             GSIGNOND_CONFIG_GENERAL_EXTENSION);}

gboolean
gsignond_config_set_daemon_timeout (GSignondConfig *config, guint timeout)
{
    gboolean res =
        gsignond_config_set_integer (config,
                                     GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT,
                                     timeout);
    
    if (res) g_object_notify_by_pspec (G_OBJECT (config),
                                       properties[PROP_DAEMON_TIMEOUT]);

    return res;
}

guint
gsignond_config_get_daemon_timeout (GSignondConfig *config)
{
    return gsignond_config_get_integer (config,
                                        GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT);
}

gboolean
gsignond_config_set_identity_timeout (GSignondConfig *config, guint timeout)
{
    gboolean res =
        gsignond_config_set_integer (config,
                                     GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT,
                                     timeout);

    if (res) g_object_notify_by_pspec (G_OBJECT (config),
                                       properties[PROP_IDENTITY_TIMEOUT]);

    return res;
}

guint
gsignond_config_get_identity_timeout (GSignondConfig *config)
{
    return gsignond_config_get_integer (config,
                                        GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT);
}

gboolean
gsignond_config_set_auth_session_timeout (GSignondConfig *config, guint timeout)
{
    gboolean res =
        gsignond_config_set_integer (config,
                                     GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT,
                                     timeout);

    if (res) g_object_notify_by_pspec (G_OBJECT (config),
                                       properties[PROP_AUTH_SESSION_TIMEOUT]);

    return res;
}

guint
gsignond_config_get_auth_session_timeout (GSignondConfig *config)
{
    return gsignond_config_get_integer (config,
                                     GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT);
}

const GHashTable *
gsignond_config_get_config_table (GSignondConfig *config)
{
    return config->priv->config_table;
}

GSignondConfig *
gsignond_config_new ()
{
    return GSIGNOND_CONFIG (g_object_new (GSIGNOND_TYPE_CONFIG, NULL));
}

