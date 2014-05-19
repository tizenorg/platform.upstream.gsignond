/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@linux.intel.com>
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
#include <string.h>

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-credentials.h"
#include "common/db/gsignond-db-error.h"
#include "common/gsignond-identity-info-internal.h"
#include "gsignond-db-credentials-database.h"

#define GSIGNOND_DB_CREDENTIALS_DATABASE_GET_PRIVATE(obj) \
                                       (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                        GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE, \
                                        GSignondDbCredentialsDatabasePrivate))

G_DEFINE_TYPE (GSignondDbCredentialsDatabase, gsignond_db_credentials_database,
        G_TYPE_OBJECT);

struct _GSignondDbCredentialsDatabasePrivate
{
    GSignondDbMetadataDatabase *metadata_db;
};

enum
{
    PROP_0,
    PROP_CONFIG,
    PROP_STORAGE,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void
_set_property (GObject *object, guint prop_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondDbCredentialsDatabase *self =
            GSIGNOND_DB_CREDENTIALS_DATABASE (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_assert (self->config == NULL);
            self->config = g_value_dup_object (value);
            break;
        case PROP_STORAGE:
            g_assert (self->secret_storage == NULL);
            self->secret_storage = g_value_dup_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GSignondDbCredentialsDatabase *self =
            GSIGNOND_DB_CREDENTIALS_DATABASE (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_value_set_object (value, self->config);
            break;
        case PROP_STORAGE:
            g_value_set_object (value, self->secret_storage);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_gsignond_db_credentials_database_dispose (GObject *gobject)
{
    g_return_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (gobject));
    GSignondDbCredentialsDatabase *self =
            GSIGNOND_DB_CREDENTIALS_DATABASE (gobject);

    if (self->config) {
        g_object_unref (self->config);
        self->config = NULL;
    }
    if (self->secret_storage) {
        g_object_unref (self->secret_storage);
        self->secret_storage = NULL;
    }
    if (self->priv->metadata_db) {
        g_object_unref (self->priv->metadata_db);
        self->priv->metadata_db = NULL;
    }
    G_OBJECT_CLASS (gsignond_db_credentials_database_parent_class)->dispose (
            gobject);
}

static void
gsignond_db_credentials_database_class_init (
        GSignondDbCredentialsDatabaseClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = _set_property;
    gobject_class->get_property = _get_property;
    gobject_class->dispose = _gsignond_db_credentials_database_dispose;

    properties[PROP_CONFIG] = g_param_spec_object (
            "config",
            "config",
            "Configuration object",
            GSIGNOND_TYPE_CONFIG,
            G_PARAM_CONSTRUCT_ONLY |
            G_PARAM_READWRITE |
            G_PARAM_STATIC_STRINGS);
    properties[PROP_STORAGE] = g_param_spec_object (
            "storage",
            "storage",
            "Secure Storage object",
            GSIGNOND_TYPE_SECRET_STORAGE,
            G_PARAM_CONSTRUCT_ONLY |
            G_PARAM_READWRITE |
            G_PARAM_STATIC_STRINGS);
    g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);

    g_type_class_add_private (klass,
            sizeof (GSignondDbCredentialsDatabasePrivate));
}

static void
gsignond_db_credentials_database_init (
        GSignondDbCredentialsDatabase *self)
{
    self->priv = GSIGNOND_DB_CREDENTIALS_DATABASE_GET_PRIVATE (self);
    self->config = NULL;
    self->secret_storage = NULL;
    self->priv->metadata_db = NULL;
}

/**
 * gsignond_db_credentials_database_new:
 *
 * @config: (transfer none) the #GSignondConfig object
 * @storage: (transfer none) the #GSignondSecretStorage object
 *
 * Creates new #GSignondDbCredentialsDatabase object
 *
 * Returns: (transfer full) the #GSignondDbCredentialsDatabase object
 */
GSignondDbCredentialsDatabase *
gsignond_db_credentials_database_new (
        GSignondConfig *config,
        GSignondSecretStorage *storage)
{

	GSignondDbCredentialsDatabase *self = NULL;
	self = GSIGNOND_DB_CREDENTIALS_DATABASE (
            g_object_new (GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE,
                    "config",
                    config,
                    "storage",
                    storage,
                    NULL));
	if (self) {
	    self->priv->metadata_db = gsignond_db_metadata_database_new (
	            self->config);
	    if (self->priv->metadata_db) {
	        gsignond_db_metadata_database_open (self->priv->metadata_db);
	    }
	}
	return self;
}

/**
 * gsignond_db_credentials_database_open_secret_storage:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 *
 * Opens (and initializes) secret storage.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_open_secret_storage (
        GSignondDbCredentialsDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);
    g_return_val_if_fail (self->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_open_db (self->secret_storage);
}

/**
 * gsignond_db_credentials_database_close_secret_storage:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 *
 * Closes the secret storage.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_close_secret_storage (
        GSignondDbCredentialsDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);
    g_return_val_if_fail (self->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_close_db (self->secret_storage);
}

/**
 * gsignond_db_credentials_database_is_open_secret_storage:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 *
 * Checks if secret storage is open or not.
 *
 * Returns: TRUE if secret storage is open, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_is_open_secret_storage (
        GSignondDbCredentialsDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);
    g_return_val_if_fail (self->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_is_open_db (self->secret_storage);
}

/**
 * gsignond_db_credentials_database_clear:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 *
 * Clears the credentials database.
 *
 * Returns: TRUE if secret storage is open, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_clear (GSignondDbCredentialsDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);
    g_return_val_if_fail (self->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_clear_db (self->secret_storage) &&
			gsignond_db_sql_database_clear (
					GSIGNOND_DB_SQL_DATABASE (self->priv->metadata_db));
}

/**
 * gsignond_db_credentials_database_load_identity:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @query_secret: whether to query the password or not
 *
 * Fetches the info associated with the specified identity id.
 *
 * Returns: (transfer full) the info #GSignondIdentityInfo if successful,
 * NULL otherwise. When done, it should be freed with
 * gsignond_identity_info_unref (identity)
 */
GSignondIdentityInfo *
gsignond_db_credentials_database_load_identity (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        gboolean query_secret)
{
	GSignondIdentityInfo *identity = NULL;
    gboolean is_un_sec = FALSE;
    gboolean is_pwd_sec =  FALSE;

	g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    identity = gsignond_db_metadata_database_get_identity (
    		self->priv->metadata_db, identity_id);
    if (!identity) 
        return identity;

    if (query_secret &&
    	!gsignond_identity_info_get_is_identity_new (identity) &&
        gsignond_db_credentials_database_is_open_secret_storage (self)) {

        is_un_sec = gsignond_identity_info_get_is_username_secret (identity);
        is_pwd_sec = gsignond_identity_info_get_store_secret (identity);
        if (is_un_sec || is_pwd_sec) {
            GSignondCredentials * creds;
            creds = gsignond_secret_storage_load_credentials (
                    self->secret_storage, identity_id);
            if (creds) {
                if (is_un_sec)
                    gsignond_identity_info_set_username (identity,
                            gsignond_credentials_get_username (creds));
                if (is_pwd_sec)
                    gsignond_identity_info_set_secret (identity,
                            gsignond_credentials_get_password (creds));
                g_object_unref (creds);
            }
        }
    }
    /* Reseting the edit state to NONE as its newly loaded identity */
    gsignond_identity_info_reset_edit_flags (identity, IDENTITY_INFO_PROP_NONE);

	return identity;
}

/**
 * gsignond_db_credentials_database_load_identities:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @filter: (transfer none) filter to apply. Currently supported filters:
 *   ("Owner":GSignondSecurtityContext *context) - Identities matched with this 'context'
 *   ("Type":guint32 type) - Identities matched with 'type'
 *   ("Caption":gchar *caption) - Identties matched/start with 'caption'
 *
 * Fetches the list of the identities.
 *
 * Returns: (transfer full) the list if successful, NULL otherwise.
 * When done list should be freed with gsignond_identity_info_list_free (list)
 */
GSignondIdentityInfoList *
gsignond_db_credentials_database_load_identities (
        GSignondDbCredentialsDatabase *self,
        GSignondDictionary *filter)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

	return gsignond_db_metadata_database_get_identities (
			self->priv->metadata_db, filter);
}

/**
 * gsignond_db_credentials_database_update_identity:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity: the identity info which needs to be inserted to db
 * @store_secret: flag to indicate whether to store the secret or not
 *
 * Updates the identity info in the credentials database.
 *
 * Returns: the id of the updated identity, 0 otherwise.
 */
guint32
gsignond_db_credentials_database_update_identity (
        GSignondDbCredentialsDatabase *self,
        GSignondIdentityInfo* identity)
{
	guint32 id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    id = gsignond_db_metadata_database_update_identity (self->priv->metadata_db,
    		identity);

    if (!id) return 0;

    /* Reseting the edit state to NONE as all the changes are stored in db. */
    gsignond_identity_info_reset_edit_flags (identity, IDENTITY_INFO_PROP_NONE);

    if (gsignond_db_credentials_database_is_open_secret_storage (self)) {
        GSignondCredentials *creds = NULL;
        gboolean un_sec, pwd_sec;
        const gchar *tmp_str = NULL;

    	creds = gsignond_credentials_new ();
    	gsignond_credentials_set_id (creds, id);

        pwd_sec = gsignond_identity_info_get_store_secret (identity) &&
                  (tmp_str = gsignond_identity_info_get_secret (identity));
    	if (pwd_sec) {
    		gsignond_credentials_set_password (creds, tmp_str);
    	}

    	un_sec = gsignond_identity_info_get_is_username_secret (identity) &&
                 (tmp_str = gsignond_identity_info_get_username (identity));
    	if (un_sec) {
    		gsignond_credentials_set_username (creds, tmp_str);
    	}

    	if (un_sec || pwd_sec) {
            DBG ("Add credentials to secret storage");
    		gsignond_secret_storage_update_credentials (
    			self->secret_storage, creds);
    	}
    	g_object_unref (creds);
    }

    return id;
}

/**
 * gsignond_db_credentials_database_remove_identity:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity: the identity info which needs to be removed
 *
 * Removes the identity info from the credentials database.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_remove_identity (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    if (!gsignond_db_credentials_database_is_open_secret_storage (self)) {
        DBG ("Remove failed as DB is not open");
    	return FALSE;
    }
	return gsignond_secret_storage_remove_credentials (
					self->secret_storage,
					identity_id) &&
		   gsignond_db_metadata_database_remove_identity (
					self->priv->metadata_db,
					identity_id);
}

/**
 * gsignond_db_credentials_database_check_secret:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @username: the username of the identity
 * @secret: the secret of the identity
 *
 * Checks the identity info from the credentials database.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_check_secret (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *username,
        const gchar *secret)
{
	GSignondIdentityInfo *identity = NULL;
	gboolean check = FALSE;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    if (!gsignond_db_credentials_database_is_open_secret_storage (self)) {
        DBG ("Check failed as DB is not open");
    	return FALSE;
    }

    identity = gsignond_db_metadata_database_get_identity (
    		self->priv->metadata_db, identity_id);
    if (identity) {
    	GSignondCredentials *creds = NULL;

		creds = gsignond_credentials_new ();
		gsignond_credentials_set_id (creds, identity_id);
		gsignond_credentials_set_password (creds, secret);
		if (gsignond_identity_info_get_is_username_secret (identity)) {
	        DBG ("Check credentials from storage");
			gsignond_credentials_set_username (creds, username);
			check = gsignond_secret_storage_check_credentials (
					self->secret_storage, creds);
		} else {
			gsignond_credentials_set_username (creds, "");
			check = g_strcmp0 (username,
						gsignond_identity_info_get_username (identity)) == 0 &&
					gsignond_secret_storage_check_credentials (
						self->secret_storage, creds);
		}
		g_object_unref (creds);
    	gsignond_identity_info_unref (identity);
    }
	return check;
}

/**
 * gsignond_db_credentials_database_load_data:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @method: the name of the method
 *
 * Fetches the data associated with the identity id and method.
 *
 * Returns: (transfer full) the data if successful, NULL otherwise.
 * When done data should be freed with g_hash_table_unref (data)
 */
GHashTable*
gsignond_db_credentials_database_load_data (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *method)
{
	guint32 method_id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);
    g_return_val_if_fail (method != NULL, NULL);

    if (identity_id == 0 ||
    	!gsignond_db_credentials_database_is_open_secret_storage (self)) {
        DBG ("Load data failed - invalid id (%d)/secret storage not opened",
                identity_id);
    	return NULL;
    }

    method_id = gsignond_db_metadata_database_get_method_id (
    				self->priv->metadata_db,
    				method);
    if (method_id == 0) {
        DBG ("Load data failed - invalid method id");
    	return NULL;
    }
	return gsignond_secret_storage_load_data (self->secret_storage,
			identity_id, method_id);
}

/**
 * gsignond_db_credentials_database_update_data:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @method: the name of the method
 * @data: the data to be stored
 *
 * Stores/updates the data associated with the identity id and method.
 *
 * Returns: (transfer full) the data if successful, NULL otherwise.
 * When done data should be freed with g_hash_table_unref (data)
 */
gboolean
gsignond_db_credentials_database_update_data (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *method,
        GHashTable *data)
{
	guint32 method_id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);
    g_return_val_if_fail (method != NULL && data != NULL, FALSE);

    if (identity_id == 0 ||
    	!gsignond_db_credentials_database_is_open_secret_storage (self)) {
        DBG ("Update data failed - invalid id(%d)/secret storage not opened",
                identity_id);
    	return FALSE;
    }

    method_id = gsignond_db_metadata_database_get_method_id (
    				self->priv->metadata_db,
    				method);
    if (method_id == 0) {
    	if (!gsignond_db_metadata_database_insert_method (
    	    				self->priv->metadata_db,
    	    				method,
    	    				&method_id)) {
            DBG ("Update data failed - insertion of method to DB failed");
    		return FALSE;
    	}
    }
	return gsignond_secret_storage_update_data (self->secret_storage,
			identity_id, method_id, data);
}

/**
 * gsignond_db_credentials_database_remove_data:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @method: the name of the method
 *
 * Fetches the data associated with the identity id and method.
 *
 * Returns: (transfer full) the data if successful, NULL otherwise.
 * When done data should be freed with g_hash_table_unref (data)
 */
gboolean
gsignond_db_credentials_database_remove_data (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const gchar *method)
{
	guint32 method_id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    if (identity_id == 0 ||
    	!gsignond_db_credentials_database_is_open_secret_storage (self)) {
        DBG ("Remove data failed - invalid id (%d)/secret storage not opened",
                identity_id);
    	return FALSE;
    }

    if (method && strlen (method) > 0) {
        method_id = gsignond_db_metadata_database_get_method_id (
        				self->priv->metadata_db,
        				method);
        if (method_id == 0) {
            DBG ("Remove data failed - method not found");
        	return FALSE;
        }
    }
	return gsignond_secret_storage_remove_data (self->secret_storage,
			identity_id, method_id);
}

/**
 * gsignond_db_credentials_database_get_methods:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @sec_ctx: the security context
 *
 * Fetches the list of the methods associated with the specified identity id.
 *
 * Returns: (transfer full) the list if successful, NULL otherwise.
 * When done list should be freed with g_list_free_full (list, g_free)
 */
GList *
gsignond_db_credentials_database_get_methods (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        GSignondSecurityContext* sec_ctx)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    return gsignond_db_metadata_database_get_methods (self->priv->metadata_db,
    		identity_id, sec_ctx);
}

/**
 * gsignond_db_credentials_database_insert_reference:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @ref_owner: the owner security context
 * @reference: reference for the given identity
 *
 * Insert reference into the database for the given identity id.
 *
 * Returns: TRUE if successful,FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_insert_reference (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    return gsignond_db_metadata_database_insert_reference (
    		self->priv->metadata_db, identity_id, ref_owner,reference);
}

/**
 * gsignond_db_credentials_database_remove_reference:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @ref_owner: the owner security context
 * @reference: reference for the given identity
 *
 * Removes reference from the database for the given identity id.
 *
 * Returns: TRUE if successful,FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_remove_reference (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    return gsignond_db_metadata_database_remove_reference (
            self->priv->metadata_db, identity_id, ref_owner, reference);
}

/**
 * gsignond_db_credentials_database_get_references:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity
 * @ref_owner: the owner security context
 *
 * Gets references from the database for the given identity id.
 *
 * Returns: (transfer full) the list #GList if successful,
 * NULL otherwise. When done the list should be freed with
 * g_list_free_full (list, g_free)
 */
GList *
gsignond_db_credentials_database_get_references (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext* ref_owner)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    return gsignond_db_metadata_database_get_references (
    		self->priv->metadata_db, identity_id, ref_owner);
}

/**
 * gsignond_db_credentials_database_get_accesscontrol_list:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity whose access control list is needed
 *
 * Gets all the access control list from the database into a list.
 *
 * Returns: (transfer full) the list #GSignondSecurityContextList if successful,
 * NULL otherwise. When done the list should be freed with
 * gsignond_identity_info_list_free
 */
GSignondSecurityContextList *
gsignond_db_credentials_database_get_accesscontrol_list(
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    return gsignond_db_metadata_database_get_accesscontrol_list (
    		self->priv->metadata_db, identity_id);
}

/**
 * gsignond_db_credentials_database_get_owner:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity whose owner list is needed
 *
 * Gets the onwer of the identity referred by @identity_id from the database.
 *
 * Returns: (transfer full) the list #GSignondSecurityContext if successful,
 * NULL otherwise. When done the list should be freed with
 * gsignond_identity_info_unref
 */
GSignondSecurityContext *
gsignond_db_credentials_database_get_owner(
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    return gsignond_db_metadata_database_get_owner (
    		self->priv->metadata_db, identity_id);
}

/**
 * gsignond_db_credentials_database_get_identity_owner:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity whose owner is needed
 *
 * Gets the owner from the database for the given identity id.
 *
 * Returns: (transfer full) the #GSignondSecurityContext if successful,
 * NULL otherwise. When done the context, it should be freed with
 * gsignond_identity_info_unref
 */
GSignondSecurityContext *
gsignond_db_credentials_database_get_identity_owner (
		GSignondDbCredentialsDatabase *self,
        const guint32 identity_id)
{
	GSignondSecurityContext *ctx = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    ctx = gsignond_db_metadata_database_get_owner (
        		self->priv->metadata_db, identity_id);
    return ctx;
}

const GError *
gsignond_db_credentials_database_get_last_error (
    GSignondDbCredentialsDatabase *self)
{
    g_return_val_if_fail (self && GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    return gsignond_db_sql_database_get_last_error (
        GSIGNOND_DB_SQL_DATABASE (self->priv->metadata_db));
}

