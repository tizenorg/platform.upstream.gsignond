/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 - 2014 Intel Corporation.
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

#include <gmodule.h>

#include "config.h"

#include "gsignond-daemon.h"

#include "gsignond/gsignond-config.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-extension-interface.h"
#include "gsignond/gsignond-utils.h"
#include "daemon/gsignond-identity.h"
#include "daemon/db/gsignond-db-credentials-database.h"

struct _GSignondDaemonPrivate
{
    GSignondConfig      *config;
    GHashTable          *identities;
    GModule             *extension_module;
    GSignondExtension   *extension;
    GSignondStorageManager *storage_manager;
    GSignondSecretStorage *secret_storage;
    GSignondDbCredentialsDatabase *db;
    GSignondAccessControlManager *acm;
    GSignondPluginProxyFactory *plugin_proxy_factory;
    GSignondSignonuiProxy *ui;
};

G_DEFINE_TYPE (GSignondDaemon, gsignond_daemon, G_TYPE_OBJECT)


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

void
_free_identity (gpointer data, gpointer user_data)
{
    (void) user_data;
    GObject *identity = G_OBJECT (data);

    DBG ("free identity %p", identity);
    g_object_unref (identity);
}

static void
_dispose (GObject *object)
{
    GSignondDaemon *self = GSIGNOND_DAEMON(object);

    if (self->priv->ui) {
        g_object_unref (self->priv->ui);
        self->priv->ui = NULL;
    }

    if (self->priv->plugin_proxy_factory) {
        g_object_unref (self->priv->plugin_proxy_factory);
        self->priv->plugin_proxy_factory = NULL;
    }

    if (self->priv->identities) {
        g_hash_table_unref (self->priv->identities);
        self->priv->identities = NULL;
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

    if (self->priv->config) {
        g_object_unref (self->priv->config);
        self->priv->config = NULL;
    }

    G_OBJECT_CLASS (gsignond_daemon_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    G_OBJECT_CLASS (gsignond_daemon_parent_class)->finalize (object);
}

static gboolean
_init_extension (GSignondDaemon *self)
{
    DBG ("Initializing extension '%s' %d.%d.%d.%d",
         gsignond_extension_get_name (self->priv->extension),
         (gsignond_extension_get_version (self->priv->extension) >> 24),
         (gsignond_extension_get_version (self->priv->extension) >> 16) & 0xff,
         (gsignond_extension_get_version (self->priv->extension) >> 8) & 0xff,
         gsignond_extension_get_version (self->priv->extension) & 0xff);

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

static void
_on_extension_dispose (gpointer data, GObject *object)
{
    if (data) *(GSignondExtension **)data = NULL;
}

static GSignondExtension * _default_extension_init ()
{
    static GSignondExtension *default_extension = NULL;

    if (!default_extension) {
        default_extension =
            g_object_new (GSIGNOND_TYPE_EXTENSION, NULL);
        
        g_object_weak_ref (G_OBJECT (default_extension),
                           _on_extension_dispose,
                           &default_extension);
    }

    return default_extension;
}


static gboolean
_init_extensions (GSignondDaemon *self)
{
    gboolean res = TRUE;
    gboolean symfound;
    const gchar *ext_path = GSIGNOND_EXTENSIONS_DIR;
    const gchar *ext_name;
    gchar *mod_name;
    gchar *mod_filename;
    gchar *initf_name;
    GSignondExtension* (*ext_init) (void);

#   ifdef ENABLE_DEBUG
    const gchar *env_val = g_getenv ("SSO_EXTENSIONS_DIR");
    if (env_val)
        ext_path = env_val;
#   endif
    ext_name = gsignond_config_get_string (self->priv->config,
        GSIGNOND_CONFIG_GENERAL_EXTENSION);
    if (ext_name && !ext_path) return FALSE;

    if (ext_name && g_strcmp0 (ext_name, "default") != 0) {
        mod_name = g_strdup_printf ("extension-%s", ext_name);
        mod_filename = g_module_build_path (ext_path, mod_name);
        g_free (mod_name);
        if (!mod_filename) return FALSE;
        DBG ("Loading extension '%s'", mod_filename);
        self->priv->extension_module =
            g_module_open (mod_filename, G_MODULE_BIND_LOCAL);
        g_free (mod_filename);
        if (!self->priv->extension_module) {
            DBG("failed to load module : %s", g_module_error());
            return FALSE;
        }
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
        ext_init = _default_extension_init;
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
    if (storage_location == NULL)
        return FALSE;
    gsignond_config_set_string (self->priv->config, 
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

static gboolean
_clear_identity (gpointer idp, gpointer identityp, gpointer user_data)
{
    (void) user_data;

    return gsignond_identity_clear (GSIGNOND_IDENTITY (identityp));
}

static void
gsignond_daemon_init (GSignondDaemon *self)
{
    self->priv = GSIGNOND_DAEMON_PRIV(self);

    self->priv->config = gsignond_config_new ();
    self->priv->identities = g_hash_table_new_full (
            g_direct_hash, g_direct_equal, NULL, NULL);
    self->priv->plugin_proxy_factory = gsignond_plugin_proxy_factory_new(
        self->priv->config);
    
    if (!_init_extensions (self))
        ERR("gsignond_daemon_init_extensions() failed");

    if (!_init_storage (self))
        ERR("gsignond_daemon_init_storage() failed");
    if (!_open_database (self))
        ERR("gisgnond_daemon_open_database() failed");

    self->priv->ui = gsignond_signonui_proxy_new ();
}

static void
gsignond_daemon_class_init (GSignondDaemonClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDaemonPrivate));

    object_class->constructor = _constructor;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;
}

static gboolean
_compare_identity_by_pointer (gpointer key, gpointer value, gpointer dead_object)
{
    return value == dead_object;
}

static void
_on_identity_disposed (gpointer data, GObject *object)
{
    GSignondDaemon *daemon = GSIGNOND_DAEMON (data);

    DBG ("daemon %p identity %p disposed", daemon, object);
    g_hash_table_foreach_remove (daemon->priv->identities,
                _compare_identity_by_pointer, object);
}

guint32
gsignond_daemon_store_identity (GSignondDaemon *daemon, GSignondIdentity *identity)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), 0);
    g_return_val_if_fail (identity && GSIGNOND_IS_IDENTITY(identity), 0);

    GSignondIdentityInfo *info = NULL;
    guint32 id = 0;
    gboolean was_new_identity = FALSE;
    
    info = gsignond_identity_get_identity_info(identity);
    if (!info) return 0;
    
    was_new_identity = gsignond_identity_info_get_is_identity_new (info);

    id = gsignond_db_credentials_database_update_identity (daemon->priv->db, info);

    if (was_new_identity && id) {
        g_hash_table_insert (daemon->priv->identities, GUINT_TO_POINTER(id), identity);
        g_object_weak_ref (G_OBJECT (identity), _on_identity_disposed, daemon);
    }

    return id;
}

gboolean
gsignond_daemon_remove_identity (GSignondDaemon *daemon, guint32 id)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), FALSE);

    return gsignond_db_credentials_database_remove_identity(daemon->priv->db, id);
}

guint32
gsignond_daemon_add_identity_reference (GSignondDaemon *daemon, guint32 identity_id, const GSignondSecurityContext *owner, const gchar *reference)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), 0);

    return gsignond_db_credentials_database_insert_reference (daemon->priv->db, identity_id, owner, reference);
}

gboolean
gsignond_daemon_remove_identity_reference (GSignondDaemon *daemon, guint32 identity_id, const GSignondSecurityContext *owner, const gchar *reference)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), FALSE);

    return gsignond_db_credentials_database_remove_reference (daemon->priv->db, identity_id, owner, reference);
}

gboolean
gsignond_daemon_store_identity_data (GSignondDaemon *daemon, guint32 identity_id, const gchar *method, GSignondDictionary *data)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), FALSE);

    return gsignond_db_credentials_database_update_data (daemon->priv->db, identity_id, method, data);
}

gboolean
gsignond_daemon_clear_identity_data (GSignondDaemon *daemon, guint32 identity_id)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), FALSE);

    return gsignond_db_credentials_database_remove_data (daemon->priv->db, identity_id, NULL);
}

GSignondDictionary *
gsignond_daemon_load_identity_data (GSignondDaemon *daemon, guint32 identity_id, const gchar *method)
{
    g_return_val_if_fail (daemon && GSIGNOND_IS_DAEMON (daemon), NULL);
    g_return_val_if_fail (identity_id, NULL);
    g_return_val_if_fail (method, NULL);

    return gsignond_db_credentials_database_load_data (daemon->priv->db, identity_id, method);
}

GSignondIdentity *
gsignond_daemon_register_new_identity (GSignondDaemon *daemon,
                                       const GSignondSecurityContext *ctx,
                                       GError **error) 
{
    if (!daemon || !GSIGNOND_IS_DAEMON (daemon) || !ctx) {
        WARN ("assertion failed (daemon && GSIGNOND_IS_DAEMON (daemon)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    GSignondIdentityInfo *info = gsignond_identity_info_new ();
    GSignondIdentity *identity = NULL;
    GSignondSecurityContext *owner = NULL;
    GSignondSecurityContextList *acl = NULL;

    owner = gsignond_security_context_copy (ctx);

    gsignond_identity_info_set_owner (info, owner);

    acl = (GSignondSecurityContextList *)g_list_append (NULL, owner);
    gsignond_identity_info_set_access_control_list (info, acl);
    gsignond_security_context_free (owner);
    g_list_free (acl);

    DBG("register_new_identity: cache size : %d", g_hash_table_size(daemon->priv->identities));
    identity = gsignond_identity_new (daemon, info);
    if (identity == NULL) {
        gsignond_identity_info_unref (info);
        ERR("Unable to register new identity");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER, "Internal server error");
        return NULL;
    }
    DBG(" --- registered new identity : %p", identity);
    return identity;
}

GSignondIdentity *
gsignond_daemon_get_identity (GSignondDaemon *daemon,
                              const guint32 id,
                              const GSignondSecurityContext *ctx,
                              GError **error)
{
    if (!daemon || !GSIGNOND_IS_DAEMON (daemon)) {
        WARN ("assertion (daemon && GSIGNOND_IS_DAEMON (daemon)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }
    if (id <= 0) {
        WARN ("client provided invalid identity id");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Invalid identity id");
        return NULL;
    }

    GSignondIdentity *identity = NULL;
    GSignondIdentityInfo *identity_info = NULL;

#define VALIDATE_IDENTITY_X_ACCESS(info, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = daemon->priv->acm; \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (info); \
    GSignondSecurityContext *owner = gsignond_identity_info_get_owner (info); \
    gboolean valid = gsignond_access_control_manager_peer_is_allowed_to_use_identity (acm, ctx, owner, acl); \
    gsignond_security_context_free (owner); \
    gsignond_security_context_list_free (acl); \
    if (!valid) { \
        WARN ("identity access check failed"); \
        gsignond_identity_info_unref (info); \
        if (error) { \
            *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "Can not read identity"); \
        } \
        return ret; \
    } \
}
    DBG("Get identity for id '%d'\n cache size : %d", id, g_hash_table_size(daemon->priv->identities));
    identity = GSIGNOND_IDENTITY(g_hash_table_lookup (daemon->priv->identities, GUINT_TO_POINTER(id)));
    if (identity) {
        identity_info = gsignond_identity_get_identity_info (identity);
        gsignond_identity_info_ref (identity_info);
        VALIDATE_IDENTITY_X_ACCESS (identity_info, ctx, NULL);
        DBG ("using cased Identity '%p' for id %d", identity, id);
        gsignond_identity_info_unref (identity_info);

        return GSIGNOND_IDENTITY (g_object_ref (identity));
    }

    /* identity not found in cache, so try to load form db */
    identity_info = gsignond_db_credentials_database_load_identity (
                            daemon->priv->db, id, TRUE);
    if (!identity_info) {
        if (error)  {
            const GError *err = gsignond_db_credentials_database_get_last_error (daemon->priv->db);
            *error = err ? g_error_copy (err) : gsignond_get_gerror_for_id (
                        GSIGNOND_ERROR_IDENTITY_NOT_FOUND, "identity not found with id '%d'", id);
        }
        return NULL;
    }

    VALIDATE_IDENTITY_X_ACCESS (identity_info, ctx, NULL);

    identity = gsignond_identity_new (daemon, identity_info);
    if (!identity) {
        gsignond_identity_info_unref (identity_info);
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER, "Internal server error");
        return NULL;
    }

    g_hash_table_insert (daemon->priv->identities, GUINT_TO_POINTER(id), identity);
    g_object_weak_ref (G_OBJECT (identity), _on_identity_disposed, daemon);

    DBG("created new identity '%p' for id '%d'", identity, id);

    return identity;

#undef VALIDATE_IDENTITY_READ_ACCESS
}

const gchar ** 
gsignond_daemon_query_methods (GSignondDaemon *daemon, GError **error)
{
    if (!daemon || !GSIGNOND_IS_DAEMON (daemon)) {
        WARN ("assertion (daemon && GSIGNOND_IS_DAEMON(daemon)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    return gsignond_plugin_proxy_factory_get_plugin_types (
            daemon->priv->plugin_proxy_factory);
}

const gchar ** 
gsignond_daemon_query_mechanisms (GSignondDaemon *daemon, const gchar *method, GError **error) 
{
    if (!daemon || !GSIGNOND_IS_DAEMON (daemon)) {
        WARN ("assertion (daemon && GSIGNOND_IS_DAEMON(daemon)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    const gchar **mechanisms = gsignond_plugin_proxy_factory_get_plugin_mechanisms (
            daemon->priv->plugin_proxy_factory, method);

    if (!mechanisms || mechanisms[0] == NULL) {
        DBG("no mechanisms found for method '%s'", method);
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_METHOD_NOT_KNOWN, "method '%s' not found", method);
    }

    return mechanisms;
}

static gboolean
_check_keychain_access (GSignondDaemon *self,
                        const GSignondSecurityContext *ctx,
                        GError **error)
{
    GSignondAccessControlManager *acm = self->priv->acm;
    GSignondSecurityContext *keychain =
        gsignond_access_control_manager_security_context_of_keychain (acm);
    gboolean has_access =
        (gsignond_security_context_compare (keychain, ctx) == 0);
    DBG ("keychain access compare [%s:%s] vs [%s:%s] = %d",
         gsignond_security_context_get_system_context (keychain),
         gsignond_security_context_get_application_context (keychain),
         gsignond_security_context_get_system_context (ctx),
         gsignond_security_context_get_application_context (ctx),
         has_access);
    gsignond_security_context_free (keychain);
    if (!has_access) {
        WARN ("keychain access check failed");
        if (error) {
            *error = gsignond_get_gerror_for_id (
                                    GSIGNOND_ERROR_PERMISSION_DENIED,
                                    "Can not access keychain functionality");
        }
    }
    return has_access;
}

GSignondIdentityInfoList *
gsignond_daemon_query_identities (GSignondDaemon *self,
                                  GVariant *filter,
                                  const GSignondSecurityContext *ctx,
                                  GError **error)
{
    if (!self || !GSIGNOND_IS_DAEMON (self)) {
        WARN ("assertion (self && GSIGNOND_IS_DAEMON(self)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN,
        		                                        "Unknown error");
        return NULL;
    }

    GSignondIdentityInfoList *identities  = NULL;
    GSignondDictionary *filter_map =
    		gsignond_dictionary_new_from_variant (filter);

    if (!_check_keychain_access (self, ctx, NULL)) {
    	/* Other than 'keychain' app, can only get identities owned by it. */
    	gsignond_dictionary_set (filter_map, "Owner",
    			gsignond_security_context_to_variant (ctx));
    }

    identities = gsignond_db_credentials_database_load_identities (
                      self->priv->db, filter_map);

    gsignond_dictionary_unref (filter_map);

    if (!identities) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN,
        		                                        "Not found");
    }

    return identities;
}

gboolean 
gsignond_daemon_clear (GSignondDaemon *self,
                       const GSignondSecurityContext *ctx,
                       GError **error)
{
    if (!self || !GSIGNOND_IS_DAEMON (self)) {
        WARN ("assertion (self && GSIGNOND_IS_DAEMON(self)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    if (!_check_keychain_access (self, ctx, error))
        return FALSE;

    gboolean retval = TRUE;
    GSignondDaemonPrivate *priv = self->priv;

    DBG ("destroy all identities");
    g_hash_table_foreach_remove (priv->identities, _clear_identity, self);
    if (g_hash_table_size (priv->identities) > 0) {
        WARN ("g_hash_table_foreach_remove(identities) failed for some items");
        retval = FALSE;
    }
    DBG ("perform internal clear");
    if (!gsignond_db_credentials_database_clear (priv->db)) {
        WARN ("gsignond_db_credentials_database_clear() failed");
        retval = FALSE;
    }

    DBG ("close databases");
    if (!gsignond_db_credentials_database_close_secret_storage (priv->db)) {
        WARN ("gsignond_db_credentials_database_close_secret_storage() failed");
        retval = FALSE;
    }   
    g_object_unref (priv->db);
    priv->db = NULL;

    DBG ("clear data");
    if (!gsignond_storage_manager_delete_storage (priv->storage_manager)) {
        WARN ("gsignond_storage_manager_delete_storage() failed");
        retval = FALSE;
    }

    DBG ("re-create storage");
    if (!_init_storage (self)) {
        WARN ("_init_storage() failed");
        retval = FALSE;
    }

    if (!_open_database (self)) {
        WARN ("_open_database() failed");
        retval = FALSE;
    }

    return retval;
}

/**
 * gsignond_daemon_new:
 *
 * Returns: (transfer full): newly created object of type #GSignondDaemon
 */
GSignondDaemon *
gsignond_daemon_new ()
{
    return  GSIGNOND_DAEMON(g_object_new (GSIGNOND_TYPE_DAEMON, NULL));
}

guint
gsignond_daemon_get_timeout (GSignondDaemon *self)
{
    return gsignond_config_get_integer (self->priv->config,
        GSIGNOND_CONFIG_DBUS_DAEMON_TIMEOUT);
}

guint
gsignond_daemon_get_identity_timeout (GSignondDaemon *self)
{
    return gsignond_config_get_integer (self->priv->config,
        GSIGNOND_CONFIG_DBUS_IDENTITY_TIMEOUT);
}

guint
gsignond_daemon_get_auth_session_timeout (GSignondDaemon *self)
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

gboolean
gsignond_daemon_show_dialog (GSignondDaemon *self,
                             GObject *caller,
                             GSignondSignonuiData *ui_data,
                             GSignondSignonuiProxyQueryDialogCb handler,
                             GSignondSignonuiProxyRefreshCb refresh_handler,
                             gpointer userdata)
{
    g_return_val_if_fail (self && GSIGNOND_IS_DAEMON(self), FALSE);

    return gsignond_signonui_proxy_query_dialog (self->priv->ui, caller, ui_data, handler, refresh_handler, userdata);
}

gboolean
gsignond_daemon_refresh_dialog (GSignondDaemon *self,
                                GObject *caller,
                                GSignondSignonuiData *ui_data,
                                GSignondSignonuiProxyRefreshDialogCb handler,
                                gpointer userdata)
{
    g_return_val_if_fail (self && GSIGNOND_IS_DAEMON(self), FALSE);

    return gsignond_signonui_proxy_refresh_dialog (self->priv->ui, caller, ui_data, handler, userdata);
}

gboolean
gsignond_daemon_cancel_dialog (GSignondDaemon *self,
                               GObject *caller,
                               GSignondSignonuiProxyCancelRequestCb handler,
                               gpointer userdata)
{
    g_return_val_if_fail (self && GSIGNOND_IS_DAEMON(self), FALSE);

    return gsignond_signonui_proxy_cancel_request (self->priv->ui, caller, handler, userdata);
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
