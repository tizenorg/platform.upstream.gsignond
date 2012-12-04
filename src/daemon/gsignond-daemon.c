/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
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

#include <sys/socket.h>
#include <gio/gio.h>

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-extension-interface.h"
#include "daemon/dbus/gsignond-dbus-auth-service-adapter.h"

#include "gsignond-auth-service-iface.h"
#include "gsignond-daemon.h"

enum 
{
    PROP_0,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDaemonPrivate
{
    GIOChannel          *sig_channel;
    GSignondConfig      *config;
    GModule             *extension_module;
    GSignondExtension   *extension;
    GSignondStorageManager *storage_manager;
    GSignondSecretStorage *secret_storage;
    GSignondAccessControlManager *acm;
    GSignondDbusAuthServiceAdapter *auth_service;
};

static void gsignond_daemon_auth_service_iface_init (gpointer g_iface,
                                                     gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GSignondDaemon, gsignond_daemon, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_AUTH_SERVICE_IFACE, 
                                               gsignond_daemon_auth_service_iface_init));


#define GSIGNOND_DAEMON_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DAEMON, GSignondDaemonPrivate)

static const gchar* gsignond_daemon_register_new_identity(
                                                 GSignondAuthServiceIface *self,
                                                 const GVariant *app_cntxt);
static gboolean gsignond_daemon_get_identity (GSignondAuthServiceIface *self,
                                              const guint32 id,
                                              const GVariant *app_context,
                                              gchar **object_path,
                                              GVariant **identity_data);
static const gchar * gsignond_daemon_get_auth_session_object_path (
                                                 GSignondAuthServiceIface *self,
                                                 const guint32 id,
                                                 const gchar *type,
                                                 const GVariant *app_context);
static gchar ** gsignond_daemon_query_methods (GSignondAuthServiceIface *self);
static gchar ** gsignond_daemon_query_mechanisms (
                                                 GSignondAuthServiceIface *self,
                                                 const gchar *method);
/* "(@aa{sv})" */
static GVariant * gsignond_daemon_query_identities (
                                                 GSignondAuthServiceIface *self,
                                                 const GVariant *filter);
static gboolean gsignond_daemon_clear (GSignondAuthServiceIface *self);


static gboolean gsignond_daemon_init_extension (GSignondDaemon *daemon);
static gboolean gsignond_daemon_init_extensions (GSignondDaemon *daemon);
static gboolean gsignond_daemon_init_storage (GSignondDaemon *daemon);

static sig_fd[2];

static gboolean 
_handle_unix_signal (GIOChannel *channel,
                     GIOCondition condition,
                     gpointer data)
{
    GSignondDaemon *self = GSIGNOND_DAEMON (data);

    int signal;
    int ret = read (sig_fd[1], &signal, sizeof(signal));

    switch (signal) {
        case SIGHUP: {
            DBG ("Received SIGHUP");
            //TODO: restart daemon
            break;
        }
        case SIGTERM: {
            DBG ("Received SIGTERM");
            //TODO: stop daemon
            break;
        }
        case SIGINT:  {
            DBG ("Received SIGINT");
            //TODO: stop daemon
            break;
        }
        default: break;
    }

    return TRUE;
}

static void
_setup_signal_handlers (GSignondDaemon *self)
{
    g_return_if_fail (self);

    if (socketpair (AF_UNIX, SOCK_STREAM, 0, sig_fd) != 0) {
        ERR( "Couldn't create HUP socketpair");
        return;
    }

    self->priv->sig_channel = g_io_channel_unix_new (sig_fd[0]);
    g_io_add_watch (self->priv->sig_channel, 
                    G_IO_IN, 
                    _handle_unix_signal, 
                    self);

}

static void 
_signal_handler (int signal)
{
    write(sig_fd[0], &signal, sizeof(signal));
}

static GObject*
gsignond_daemon_constructor (GType type,
                             guint n_construct_params,
                             GObjectConstructParam *construct_params)
{
    /*
     * Signleton daemon
     */
    static GObject *self = 0;

    if (!self) {
        self = G_OBJECT_CLASS (gsignond_daemon_parent_class)->constructor (
                  type, n_construct_params, construct_params);
        
        g_object_add_weak_pointer (self, (gpointer) &self);
    }
    else {
        g_object_ref (self);
    }

    return self;
}

static void
gsignond_daemon_get_property (GObject *object, 
                              guint property_id,
                              GValue *value, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
gsignond_daemon_set_property (GObject *object, 
                              guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
gsignond_daemon_dispose (GObject *object)
{
    GSignondDaemon *self = GSIGNOND_DAEMON(object);

    if (self->priv->sig_channel) {
        g_io_channel_unref (self->priv->sig_channel);
        self->priv->sig_channel = NULL;
    }

    if (self->priv->config) {
        g_object_unref (self->priv->config);
        self->priv->config = NULL;
    }

    if (self->priv->auth_service) {
        g_object_unref (self->priv->auth_service);
        self->priv->auth_service = NULL;
    }

    if (self->priv->extension) {
        self->priv->storage_manager = NULL;
        self->priv->secret_storage = NULL;
        self->priv->acm = NULL;
        g_object_unref (self->priv->extension);
        self->priv->extension = NULL;
    }

    if (self->priv->extension_module) {
        g_module_close (self->priv->extension_module);
        self->priv->extension_module = NULL;
    }

    G_OBJECT_CLASS (gsignond_daemon_parent_class)->dispose (object);
}

static void
gsignond_daemon_finalize (GObject *object)
{
    GSignondDaemon *self = GSIGNOND_DAEMON(object);

    close(sig_fd[0]);
    close(sig_fd[1]);

    if (!gsignond_secret_storage_close_db (self->priv->secret_storage)) {
        WARN("gsignond_secret_storage_close_db() failed");
    }

    if (gsignond_storage_manager_filesystem_is_mounted (
                                                 self->priv->storage_manager)) {
        if (!gsignond_storage_manager_unmount_filesystem (
                                                 self->priv->storage_manager)) {
            WARN("gsignond_storage_manager_unmount_filesystem() failed");
        }
    }

    G_OBJECT_CLASS (gsignond_daemon_parent_class)->finalize (object);
}

static void
gsignond_daemon_init (GSignondDaemon *self)
{
    GError *err = NULL;
    self->priv = GSIGNOND_DAEMON_PRIV(self);

    _setup_signal_handlers (self);

    self->priv->config = gsignond_config_new ();

    gsignond_daemon_init_extensions (self);

    gsignond_daemon_init_storage (self);

    self->priv->auth_service =
        gsignond_dbus_auth_service_adapter_new (
                                            GSIGNOND_AUTH_SERVICE_IFACE (self));
}

static void
gsignond_daemon_class_init (GSignondDaemonClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDaemonPrivate));

    object_class->constructor = gsignond_daemon_constructor;
    object_class->get_property = gsignond_daemon_get_property;
    object_class->set_property = gsignond_daemon_set_property;
    object_class->dispose = gsignond_daemon_dispose;
    object_class->finalize = gsignond_daemon_finalize;

    //g_object_class_install_properties (object_class,
    //                                   N_PROPERTIES,
    //                                   properties);
}

static void
gsignond_daemon_auth_service_iface_init (gpointer g_iface, gpointer iface_data)
{
    GSignondAuthServiceIfaceInterface *auth_service_iface = (GSignondAuthServiceIfaceInterface *)g_iface;

    (void)iface_data;

    auth_service_iface->register_new_identity =
        gsignond_daemon_register_new_identity;
    auth_service_iface->get_identity = gsignond_daemon_get_identity;
    auth_service_iface->get_auth_session_object_path =
        gsignond_daemon_get_auth_session_object_path;
    auth_service_iface->query_identities = gsignond_daemon_query_identities;
    auth_service_iface->query_methods = gsignond_daemon_query_methods;
    auth_service_iface->query_mechanisms = gsignond_daemon_query_mechanisms;
    auth_service_iface->clear = gsignond_daemon_clear;
}

static gboolean
gsignond_daemon_init_extension (GSignondDaemon *self)
{
    guint32 ext_ver = gsignond_extension_get_version (self->priv->extension);

    DBG ("Initializing extension '%s' %d.%d.%d.%d",
         gsignond_extension_get_name (self->priv->extension),
         (ext_ver >> 24),
         (ext_ver >> 16) & 0xff,
         (ext_ver >> 8) & 0xff,
         ext_ver & 0xff);

    self->priv->storage_manager =
        gsignond_extension_get_storage_manager (self->priv->extension,
                                                self->priv->config);
    self->priv->secret_storage =
        gsignond_extension_get_secret_storage (self->priv->extension,
                                               self->priv->config);
    self->priv->acm =
        gsignond_extension_get_access_control_manager (self->priv->extension,
                                                       self->priv->config);

    g_return_val_if_fail (self->priv->storage_manager &&
                          self->priv->secret_storage &&
                          self->priv->acm,
                          FALSE);

    return TRUE;
}

static gboolean
gsignond_daemon_init_extensions (GSignondDaemon *self)
{
    gboolean res = TRUE;
    gboolean symfound;
    GError *err = 0;
    const gchar *ext_path;
    const gchar *ext_name;
    gchar *mod_filename;
    gchar *initf_name;
    GSignondExtensionInit ext_init;

    ext_path = gsignond_config_get_extensions_dir (self->priv->config);
    ext_name = gsignond_config_get_extension (self->priv->config);
    if (ext_name && !ext_path) return FALSE;

    if (ext_name && g_strcmp0 (ext_name, "default") != 0) {
        mod_filename = g_module_build_path (ext_path, ext_name);
        if (!mod_filename) return FALSE;
        DBG ("Loading extension '%s'", mod_filename);
        self->priv->extension_module =
            g_module_open (mod_filename, G_MODULE_BIND_LOCAL);
        g_free (mod_filename);
        if (!self->priv->extension_module) return FALSE;
        initf_name = g_strdup_printf ("%s_extension_init", ext_name);
        symfound = g_module_symbol (self->priv->extension_module,
                                    initf_name,
                                    (gpointer *) &ext_init);
        g_free(initf_name);
        if (!symfound) {
            g_module_close (self->priv->extension_module);
            self->priv->extension_module = NULL;
            return FALSE;
        }
    } else {
        ext_init = default_extension_init;
    }
    self->priv->extension = ext_init ();
    g_return_val_if_fail (self->priv->extension &&
                          GSIGNOND_IS_EXTENSION (self->priv->extension),
                          FALSE);

    res = gsignond_daemon_init_extension (self);

    return res;
}

static gboolean
gsignond_daemon_init_storage (GSignondDaemon *self)
{
    const gchar *storage_location;
    GHashTable *config_table;

    DBG("Initializing storage");

    if (!gsignond_storage_manager_storage_is_initialized (
                                                 self->priv->storage_manager)) {
        if (!gsignond_storage_manager_initialize_storage (
                                                   self->priv->storage_manager))
            return FALSE;
    }

    storage_location = gsignond_storage_manager_mount_filesystem (
                                                   self->priv->storage_manager);
    config_table = gsignond_config_get_config_table (self->priv->config);
    g_assert (config_table != NULL);
    g_hash_table_replace (config_table, GSIGNOND_CONFIG_GENERAL_SECURE_DIR,
                          g_strdup (storage_location));

    return (storage_location != NULL);
}

guint gsignond_daemon_identity_timeout (GSignondDaemon *self)
{
    return gsignond_config_get_identity_timeout (self->priv->config);
}

guint gsignond_daemon_auth_session_timeout (GSignondDaemon *self)
{
    return gsignond_config_get_auth_session_timeout (self->priv->config);
}

static const gchar * 
gsignond_daemon_register_new_identity (GSignondAuthServiceIface *self,
                                       const GVariant *app_context) {
    (void)self;
    (void)app_context;
    return NULL;
}

static gboolean 
gsignond_daemon_get_identity (GSignondAuthServiceIface *self, const guint32 id,
                              const GVariant *app_context, gchar **object_path,
                              GVariant **identity_data)
{
    (void)self;
    (void)id;
    (void)app_context;

    if (object_path) *object_path = NULL;
    if (identity_data) *identity_data = NULL;

    return FALSE;
}

static const gchar * 
gsignond_daemon_get_auth_session_object_path (GSignondAuthServiceIface *self,
                                              const guint32 id,
                                              const gchar *type,
                                              const GVariant *app_context)
{
    (void)self;
    (void)id;
    (void)type;
    (void)app_context;

    return NULL;
}

static gchar ** 
gsignond_daemon_query_methods (GSignondAuthServiceIface *self)
{
    (void) self;

    /*
     * returning test methods 
     */
    gchar **methods = g_strsplit ("test_method_1:test_method_2", ":" ,2);

    return methods;
}

static gchar ** 
gsignond_daemon_query_mechanisms (GSignondAuthServiceIface *self, const gchar *method) 
{
    (void)self;
    (void)method;

    return NULL;
}

static GVariant * 
gsignond_daemon_query_identities (GSignondAuthServiceIface *self, const GVariant *filter)
{
    (void)self;
    (void)filter;

    return NULL;
}

static gboolean 
gsignond_daemon_clear (GSignondAuthServiceIface *self)
{
    (void)self;

    return FALSE;
}

GSignondDaemon *
gsignond_daemon_new ()
{
    return GSIGNOND_DAEMON(g_object_new (GSIGNOND_TYPE_DAEMON, NULL));
}

