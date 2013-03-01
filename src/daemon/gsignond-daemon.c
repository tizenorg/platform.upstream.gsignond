/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 - 2013 Intel Corporation.
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

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-extension-interface.h"
#include "daemon/gsignond-identity.h"
#include "daemon/dbus/gsignond-dbus-auth-service-adapter.h"
#include "daemon/db/gsignond-db-credentials-database.h"

#include "gsignond-auth-service-iface.h"
#include "gsignond-daemon.h"

struct _GSignondDaemonPrivate
{
    GSignondConfig      *config;
    GList               *identities;
    GModule             *extension_module;
    GSignondExtension   *extension;
    GSignondStorageManager *storage_manager;
    GSignondSecretStorage *secret_storage;
    GSignondDbCredentialsDatabase *db;
    GSignondAccessControlManager *acm;
    GSignondDbusAuthServiceAdapter *auth_service;
    GSignondPluginProxyFactory *plugin_proxy_factory;
};

static void gsignond_daemon_auth_service_iface_init (gpointer g_iface,
                                                     gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GSignondDaemon, gsignond_daemon, GSIGNOND_TYPE_DISPOSABLE, 0,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_AUTH_SERVICE_IFACE, 
                                               gsignond_daemon_auth_service_iface_init));


#define GSIGNOND_DAEMON_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DAEMON, GSignondDaemonPrivate)

static GObject *self = 0;

static GObject*
_constructor (GType type,
              guint n_construct_params,
              GObjectConstructParam *construct_params)
{
    /*
     * Signleton daemon
     */

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
_get_property (GObject *object, guint property_id, GValue *value,
               GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
_set_property (GObject *object, guint property_id, const GValue *value,
               GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

void
_free_identity (gpointer data, gpointer user_data)
{
    (void) user_data;
    GObject *identity = G_OBJECT (data);

    g_object_unref (identity);
}

static void
_dispose (GObject *object)
{
    GSignondDaemon *self = GSIGNOND_DAEMON(object);

    if (self->priv->plugin_proxy_factory) {
        g_object_unref (self->priv->plugin_proxy_factory);
        self->priv->plugin_proxy_factory = NULL;
    }

    if (self->priv->config) {
        g_object_unref (self->priv->config);
        self->priv->config = NULL;
    }

    if (self->priv->auth_service) {
        g_object_unref (self->priv->auth_service);
        self->priv->auth_service = NULL;
    }

    if (self->priv->db) {
 
        if (!gsignond_db_credentials_database_close_secret_storage (
                    self->priv->db)) {
            WARN("gsignond_db_credentials_database_close_secret_storage() failed");
        }   
        g_object_unref (self->priv->db);
        self->priv->db = NULL;
    }

    if (self->priv->extension) {
        if (gsignond_storage_manager_filesystem_is_mounted (
                    self->priv->storage_manager)) {
            if (!gsignond_storage_manager_unmount_filesystem (
                        self->priv->storage_manager)) {
                WARN("gsignond_storage_manager_unmount_filesystem() failed");
            }
        }

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

    if (self->priv->identities) {
        g_list_foreach (self->priv->identities, _free_identity, NULL);
    }

    G_OBJECT_CLASS (gsignond_daemon_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondDaemon *self = GSIGNOND_DAEMON(object);

    if (self->priv->identities) {
        g_list_free (self->priv->identities);
        self->priv->identities = NULL;
    }

    G_OBJECT_CLASS (gsignond_daemon_parent_class)->finalize (object);
}

static gboolean
_init_extension (GSignondDaemon *self)
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
_init_extensions (GSignondDaemon *self)
{
    gboolean res = TRUE;
    gboolean symfound;
    const gchar *ext_path;
    const gchar *ext_name;
    gchar *mod_filename;
    gchar *initf_name;
    GSignondExtensionInit ext_init;

    ext_path = gsignond_config_get_string (self->priv->config, 
        GSIGNOND_CONFIG_GENERAL_EXTENSIONS_DIR);
    ext_name = gsignond_config_get_string (self->priv->config,
        GSIGNOND_CONFIG_GENERAL_EXTENSION);
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

    res = _init_extension (self);

    return res;
}

static gboolean
_init_storage (GSignondDaemon *self)
{
    const gchar *storage_location;

    DBG("Initializing storage");

    if (!gsignond_storage_manager_storage_is_initialized (
                                                 self->priv->storage_manager)) {
        if (!gsignond_storage_manager_initialize_storage (
                                                   self->priv->storage_manager))
            return FALSE;
    }

    storage_location = gsignond_storage_manager_mount_filesystem (
                                                   self->priv->storage_manager);
    gsignond_config_set_string(self->priv->config, 
        GSIGNOND_CONFIG_GENERAL_SECURE_DIR, storage_location);                                                 

    return (storage_location != NULL);
}

static gboolean
_open_database (GSignondDaemon *self)
{
    DBG("Open databases");

    self->priv->db = gsignond_db_credentials_database_new (self->priv->config,
                                                    self->priv->secret_storage);
    if (!self->priv->db)
        return FALSE;

    return gsignond_db_credentials_database_open_secret_storage (
                                                                self->priv->db);
}

static void
gsignond_daemon_init (GSignondDaemon *self)
{
    self->priv = GSIGNOND_DAEMON_PRIV(self);

    self->priv->config = gsignond_config_new ();
    self->priv->identities = NULL;
    self->priv->plugin_proxy_factory = gsignond_plugin_proxy_factory_new(
        self->priv->config);
    
    if (!_init_extensions (self))
        ERR("gsignond_daemon_init_extensions() failed");

    if (!_init_storage (self))
        ERR("gsignond_daemon_init_storage() failed");
    if (!_open_database (self))
        ERR("gisgnond_daemon_open_database() failed");

    self->priv->auth_service =
        gsignond_dbus_auth_service_adapter_new (
                                            GSIGNOND_AUTH_SERVICE_IFACE (self));
}

static void
gsignond_daemon_class_init (GSignondDaemonClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDaemonPrivate));

    object_class->constructor = _constructor;
    object_class->get_property = _get_property;
    object_class->set_property = _set_property;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;
}

static void
_on_identity_disposed (gpointer data, GObject *object)
{
    GSignondDaemon *daemon = GSIGNOND_DAEMON (data);

    daemon->priv->identities = g_list_remove (daemon->priv->identities, object);

    if (g_list_length (daemon->priv->identities) == 0) {
        gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (daemon));
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (daemon), TRUE);
    }
}

static void
_catch_identity (GSignondDaemon *daemon, GSignondIdentity *identity)
{
    daemon->priv->identities = g_list_append (daemon->priv->identities, 
                                              (gpointer) identity);

    g_object_weak_ref (G_OBJECT (identity), _on_identity_disposed, daemon); 

    /* keep alive till this identity object gets disposed */
    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    g_signal_connect_swapped (identity, "store", 
        G_CALLBACK (gsignond_db_credentials_database_update_identity), daemon->priv->db);
    g_signal_connect_swapped (identity, "remove", 
        G_CALLBACK(gsignond_db_credentials_database_remove_identity), daemon->priv->db);
    
}

static const gchar * 
_register_new_identity (GSignondAuthServiceIface *self,
                        const GSignondSecurityContext *ctx,
                        GError **error) 
{
    if (G_LIKELY ((self && GSIGNOND_IS_DAEMON (self)) == 0)) {
        WARN ("assertion failed G_LIKELY(self && GSIGNOND_IS_DAEMON (self) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    GSignondDaemon *daemon = GSIGNOND_DAEMON (self);
    const gchar *app_context = ctx ? gsignond_security_context_get_application_context (ctx) : "";
    gint timeout = gsignond_config_get_integer (daemon->priv->config, GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT);
    GSignondIdentityInfo *info = gsignond_dictionary_new ();
    GSignondIdentity *identity = NULL;
    GSignondSecurityContext *owner = NULL;
    GSignondSecurityContextList *acl = NULL;

    owner = ctx ? gsignond_security_context_copy (ctx)
                : gsignond_security_context_new_from_values ("*", NULL);

    gsignond_identity_info_set_owner (info, owner);

    acl = (GSignondSecurityContextList *)g_list_append (NULL, owner);
    gsignond_identity_info_set_access_control_list (info, acl);
    gsignond_security_context_free (owner);
    g_list_free (acl);

    identity = gsignond_identity_new (self, info, app_context, timeout);

    if (identity == NULL) {
        gsignond_dictionary_free (info);
        ERR("Unable to register new identity");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER, "Internal server error");
        return NULL;
    }

    _catch_identity (daemon, identity);

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (self));

    return gsignond_identity_get_object_path (identity);
}

static const gchar *
_get_identity (GSignondAuthServiceIface *iface,
               const guint32 id,
               const GSignondSecurityContext *ctx,
               GVariant **identity_data,
               GError **error)
{
    if (G_LIKELY ((iface && GSIGNOND_IS_DAEMON (iface)) == 0)) {
        WARN ("assertion G_LIKELY(iface && GSIGNOND_IS_DAEMON (iface) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    GSignondDaemon *self = GSIGNOND_DAEMON (iface);
    GSignondIdentity *identity = NULL;
    GSignondIdentityInfo *identity_info = NULL;
    const gchar *app_context = ctx ? gsignond_security_context_get_application_context (ctx) : "" ;
    gint timeout = 0;

#define VALIDATE_IDENTITY_READ_ACCESS(info, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = self->priv->acm; \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (info); \
    gboolean valid = gsignond_access_control_manager_peer_is_allowed_to_use_identity (acm, ctx, acl); \
    gsignond_security_context_list_free (acl); \
    if (!valid) { \
        WARN ("identity access check failed"); \
        gsignond_dictionary_free (info); \
        if (error) { \
            *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "Can not read identity"); \
        } \
        return ret; \
    } \
}

    if (identity_data) *identity_data = NULL;

    identity_info = gsignond_db_credentials_database_load_identity (
                        self->priv->db, id, TRUE);
    if (!identity_info) {
        if (error)  {
            const GError *err = gsignond_db_credentials_database_get_last_error (self->priv->db);
            *error = err ? g_error_copy (err) : gsignond_get_gerror_for_id (
                        GSIGNOND_ERROR_IDENTITY_NOT_FOUND, "identity not found with id '%d'", id);
        }
        return NULL;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity_info, ctx, NULL);

    timeout = gsignond_config_get_integer (self->priv->config, GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT);
    identity = gsignond_identity_new (iface, identity_info, app_context, timeout);
    if (!identity) {
        gsignond_identity_info_free (identity_info);
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER, "Internal server error");
        return NULL;
    }

    if (identity_data) *identity_data = gsignond_dictionary_to_variant (identity_info);

    _catch_identity (self, identity);

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (self));

    return gsignond_identity_get_object_path (identity);

#undef VALIDATE_IDENTITY_READ_ACCESS
}

static const gchar ** 
_query_methods (GSignondAuthServiceIface *self, GError **error)
{
    if (G_LIKELY ((self && GSIGNOND_IS_DAEMON (self)) == 0)) {
        WARN ("assertion G_LIKELY ((self && GSIGNOND_IS_DAEMON(self)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    GSignondDaemon *daemon = GSIGNOND_DAEMON (self);

    return gsignond_plugin_proxy_factory_get_plugin_types (
            daemon->priv->plugin_proxy_factory);
}

static const gchar ** 
_query_mechanisms (GSignondAuthServiceIface *self, const gchar *method, GError **error) 
{
    if (G_LIKELY ((self && GSIGNOND_IS_DAEMON (self)) == 0)) {
        WARN ("assertion G_LIKELY (self && GSIGNOND_IS_DAEMON(self)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    GSignondDaemon *daemon = GSIGNOND_DAEMON (self);

    const gchar **mechanisms = gsignond_plugin_proxy_factory_get_plugin_mechanisms (
            daemon->priv->plugin_proxy_factory, method);

    if (!mechanisms || mechanisms[0] == NULL) {
        DBG("no mechanisms found for method '%s'", method);
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_METHOD_NOT_KNOWN, "method '%s' not found", method);
    }

    return mechanisms;
}

static GVariant * 
_query_identities (GSignondAuthServiceIface *self, const GVariant *filter, GError **error)
{
    if (G_LIKELY ((self && GSIGNOND_IS_DAEMON (self)) == 0)) {
        WARN ("assertion G_LIKELY (self && GSIGNOND_IS_DAEMON(self)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }
    (void)filter;

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (self));

    return NULL;
}

static gboolean 
_clear (GSignondAuthServiceIface *self, GError **error)
{
    if (G_LIKELY ((self && GSIGNOND_IS_DAEMON (self)) == 0)) {
        WARN ("assertion G_LIKELY (self && GSIGNOND_IS_DAEMON(self)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (self));

    return FALSE;
}

static GSignondAccessControlManager *
_get_acm (GSignondAuthServiceIface *iface)
{
    GSignondDaemon *self = GSIGNOND_DAEMON (iface);

    g_return_val_if_fail (self != NULL, NULL);

    return self->priv->acm;
}

static void
gsignond_daemon_auth_service_iface_init (gpointer g_iface, gpointer iface_data)
{
    GSignondAuthServiceIfaceInterface *auth_service_iface =
        (GSignondAuthServiceIfaceInterface *) g_iface;

    (void) iface_data;

    auth_service_iface->register_new_identity = _register_new_identity;
    auth_service_iface->get_identity = _get_identity;
    auth_service_iface->query_identities = _query_identities;
    auth_service_iface->query_methods = _query_methods;
    auth_service_iface->query_mechanisms = _query_mechanisms;
    auth_service_iface->clear = _clear;
    auth_service_iface->get_acm = _get_acm;
}

/**
 * gsignond_daemon_new:
 *
 * Returns: (transfer full): newly created object of type #GSignondDaemon
 */
GSignondDaemon *
gsignond_daemon_new ()
{
    GSignondDaemon *self = GSIGNOND_DAEMON(g_object_new (GSIGNOND_TYPE_DAEMON, NULL));
    guint timeout = gsignond_config_get_integer (self->priv->config, GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT);
    if (timeout) {
        gsignond_disposable_set_timeout (GSIGNOND_DISPOSABLE (self), timeout);
    }

    return self;
}

guint
gsignond_daemon_identity_timeout (GSignondDaemon *self)
{
    return gsignond_config_get_integer (self->priv->config,
        GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT);
}

guint
gsignond_daemon_auth_session_timeout (GSignondDaemon *self)
{
    return gsignond_config_get_integer (self->priv->config,
        GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT);
}

GSignondAccessControlManager *
gsignond_daemon_get_access_control_manager (GSignondDaemon *self)
{
    g_assert (self != NULL);
    g_assert (self->priv != NULL);

    return self->priv->acm;
}

GSignondPluginProxyFactory *
gsignond_daemon_get_plugin_proxy_factory (GSignondDaemon *self)
{
    g_assert (self != NULL);
    g_assert (self->priv != NULL);

    return self->priv->plugin_proxy_factory;
}

GSignondConfig *
gsignond_daemon_get_config (GSignondDaemon *self)
{
    g_assert (self != NULL);
    g_assert (self->priv != NULL);

    return self->priv->config;
}

GSignondAccessControlManager *
gsignond_get_access_control_manager ()
{
    return gsignond_daemon_get_access_control_manager(GSIGNOND_DAEMON(self));
}

GSignondPluginProxyFactory *
gsignond_get_plugin_proxy_factory ()
{
    return gsignond_daemon_get_plugin_proxy_factory(GSIGNOND_DAEMON(self));
}

GSignondConfig *
gsignond_get_config ()
{
    return gsignond_daemon_get_config(GSIGNOND_DAEMON(self));
}
