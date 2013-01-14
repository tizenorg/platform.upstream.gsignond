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
#include <gsignond/gsignond-log.h>
#include <gsignond/gsignond-credentials.h>
#include <common/db/gsignond-db-error.h>

#include "gsignond-db-credentials-database.h"

#define GSIGNOND_DB_CREDENTIALS_DATABASE_GET_PRIVATE(obj) \
                                       (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                        GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE, \
                                        GSignondDbCredentialsDatabasePrivate))

G_DEFINE_TYPE (GSignondDbCredentialsDatabase, gsignond_db_credentials_database,
        G_TYPE_OBJECT);

struct _GSignondDbCredentialsDatabasePrivate
{
    GSignondSecretStorage *secret_storage;
    GSignondDbMetadataDatabase *metadata_db;
};

static void
_gsignond_db_credentials_database_dispose (GObject *gobject)
{
    g_return_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (gobject));
    GSignondDbCredentialsDatabase *self =
            GSIGNOND_DB_CREDENTIALS_DATABASE (gobject);

    if (self->priv->secret_storage) {
        g_object_unref (self->priv->secret_storage);
        self->priv->secret_storage = NULL;
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

    gobject_class->finalize = _gsignond_db_credentials_database_dispose;

    g_type_class_add_private (klass,
            sizeof (GSignondDbCredentialsDatabasePrivate));
}

static void
gsignond_db_credentials_database_init (
        GSignondDbCredentialsDatabase *self)
{
    self->priv = GSIGNOND_DB_CREDENTIALS_DATABASE_GET_PRIVATE (self);
    self->priv->metadata_db = gsignond_db_metadata_database_new ();
    self->priv->secret_storage = NULL;
}

/**
 * gsignond_db_credentials_database_new:
 *
 * @storage: (transfer full) the #GSignondSecretStorage object
 *
 * Creates new #GSignondDbCredentialsDatabase object
 *
 * Returns: (transfer full) the #GSignondDbCredentialsDatabase object
 */
GSignondDbCredentialsDatabase *
gsignond_db_credentials_database_new (GSignondSecretStorage *storage)
{

	GSignondDbCredentialsDatabase *self = NULL;

	self = GSIGNOND_DB_CREDENTIALS_DATABASE (
            g_object_new (GSIGNOND_DB_TYPE_CREDENTIALS_DATABASE,
                         NULL));
	self->priv->secret_storage = storage;
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
    g_return_val_if_fail (self->priv->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_open_db (self->priv->secret_storage);
}

/**
 * gsignond_db_credentials_database_open_secret_storage:
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
    g_return_val_if_fail (self->priv->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_close_db (self->priv->secret_storage);
}

/**
 * gsignond_db_credentials_database_open_secret_storage:
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
    g_return_val_if_fail (self->priv->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_is_open_db (self->priv->secret_storage);
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
    g_return_val_if_fail (self->priv->secret_storage != NULL, FALSE);

	return gsignond_secret_storage_clear_db (self->priv->secret_storage) &&
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
 * gsignond_identity_info_free (identity)
 */
GSignondIdentityInfo *
gsignond_db_credentials_database_load_identity (
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id,
        gboolean query_secret)
{
	GSignondIdentityInfo *identity = NULL;

	g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    identity = gsignond_db_metadata_database_get_identity (
    		self->priv->metadata_db, identity_id);

    if (identity && query_secret &&
    	!gsignond_identity_info_get_is_identity_new (identity) &&
    	gsignond_db_credentials_database_is_open_secret_storage (self)) {
    	GSignondCredentials * creds;

    	creds = gsignond_secret_storage_load_credentials (
    			self->priv->secret_storage, identity_id);
    	if (creds) {
    		gsignond_identity_info_set_username (identity,
    				gsignond_credentials_get_username (creds));
    		gsignond_identity_info_set_secret (identity,
    				gsignond_credentials_get_password (creds));
    	}
    }

	return identity;
}

/**
 * gsignond_db_credentials_database_load_identities:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 *
 * Fetches the list of the identities.
 *
 * Returns: (transfer full) the list if successful, NULL otherwise.
 * When done list should be freed with gsignond_identity_info_list_free (list)
 */
GSignondIdentityInfoList *
gsignond_db_credentials_database_load_identities (
        GSignondDbCredentialsDatabase *self)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

	return gsignond_db_metadata_database_get_identities (
			self->priv->metadata_db);
}

/**
 * gsignond_db_credentials_database_insert_identity:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity: the identity info which needs to be inserted to db
 * @store_secret: flag to indicate whether to store the secret or not
 *
 * Inserts the identity as new into the credentials database.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_insert_identity (
        GSignondDbCredentialsDatabase *self,
        GSignondIdentityInfo* identity,
        gboolean store_secret)
{
	GSignondIdentityInfo* new_identity = NULL;
	gboolean ret = FALSE;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    new_identity = gsignond_identity_info_copy (identity);
    if (!gsignond_identity_info_get_is_identity_new (new_identity)) {
    	gsignond_identity_info_set_identity_new (new_identity);
    }

	ret = gsignond_db_credentials_database_update_identity (self, new_identity,
			store_secret);
	gsignond_identity_info_free (new_identity);
	return ret;
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
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_credentials_database_update_identity (
        GSignondDbCredentialsDatabase *self,
        GSignondIdentityInfo* identity,
        gboolean store_secret)
{
	guint32 id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), FALSE);

    id = gsignond_db_metadata_database_update_identity (self->priv->metadata_db,
    		identity);

    if (id != 0 &&
        store_secret &&
    	gsignond_db_credentials_database_is_open_secret_storage (self)) {
        GSignondCredentials *creds = NULL;
    	gboolean un_sec, pwd_sec;

    	creds = gsignond_credentials_new ();
    	pwd_sec = gsignond_identity_info_get_store_secret (identity);
    	if (pwd_sec) {
    		gsignond_credentials_set_password (creds,
    			gsignond_identity_info_get_secret (identity));
    	}

    	un_sec = gsignond_identity_info_get_is_username_secret (identity);
    	if (un_sec) {
    		gsignond_credentials_set_username (creds,
    			gsignond_identity_info_get_username (identity));
    	}

    	if (un_sec || pwd_sec) {
    		gsignond_secret_storage_update_credentials (
    			self->priv->secret_storage, creds);
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
    	return FALSE;
    }
	return gsignond_secret_storage_remove_credentials (
					self->priv->secret_storage,
					identity_id) &&
		   gsignond_db_metadata_database_remove_identity (
					self->priv->metadata_db,
					identity_id);
}

/**
 * gsignond_db_credentials_database_check_secret:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @username: the username of the identity
 * @secret: the secret of the identity
 *
 * Removes the identity info from the credentials database.
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
			gsignond_credentials_set_username (creds, username);
			check = gsignond_secret_storage_check_credentials (
					self->priv->secret_storage, creds);
		} else {
			gsignond_credentials_set_username (creds, "");
			check = g_strcmp0 (username,
						gsignond_identity_info_get_username (identity)) == 0 &&
					gsignond_secret_storage_check_credentials (
						self->priv->secret_storage, creds);
		}
		g_object_unref (creds);
    	gsignond_identity_info_free (identity);
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
    	return NULL;
    }

    method_id = gsignond_db_metadata_database_get_method_id (
    				self->priv->metadata_db,
    				method);
    if (method_id == 0) {
    	return NULL;
    }
	return gsignond_secret_storage_load_data (self->priv->secret_storage,
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
    		return FALSE;
    	}
    }
	return gsignond_secret_storage_update_data (self->priv->secret_storage,
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
    g_return_val_if_fail (method != NULL, FALSE);

    if (identity_id == 0 ||
    	!gsignond_db_credentials_database_is_open_secret_storage (self)) {
    	return FALSE;
    }

    if (method && strlen (method) > 0) {
        method_id = gsignond_db_metadata_database_get_method_id (
        				self->priv->metadata_db,
        				method);
        if (method_id == 0) {
        	return FALSE;
        }
    }
	return gsignond_secret_storage_remove_data (self->priv->secret_storage,
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

    return gsignond_db_metadata_database_remove_reference (self->priv->metadata_db,
    		identity_id, ref_owner, reference);
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
 * gsignond_db_credentials_database_get_owner_list:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity whose owner list is needed
 *
 * Gets all the onwer list from the database into a list.
 *
 * Returns: (transfer full) the list #GSignondSecurityContextList if successful,
 * NULL otherwise. When done the list should be freed with
 * gsignond_identity_info_list_free
 */
GSignondSecurityContextList *
gsignond_db_credentials_database_get_owner_list(
        GSignondDbCredentialsDatabase *self,
        const guint32 identity_id)
{
    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    return gsignond_db_metadata_database_get_owner_list (
    		self->priv->metadata_db, identity_id);
}

/**
 * gsignond_db_credentials_database_get_owner:
 *
 * @self: instance of #GSignondDbCredentialsDatabase
 * @identity_id: the id of the identity whose owner is needed
 *
 * Gets the owner from the database for the given identity id.
 *
 * Returns: (transfer full) the #GSignondSecurityContext if successful,
 * NULL otherwise. When done the context, it should be freed with
 * gsignond_identity_info_free
 */
GSignondSecurityContext *
gsignond_db_credentials_database_get_owner (
		GSignondDbCredentialsDatabase *self,
        const guint32 identity_id)
{
	GSignondSecurityContextList *list = NULL;
	GSignondSecurityContext *ctx = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_CREDENTIALS_DATABASE (self), NULL);

    list = gsignond_db_metadata_database_get_owner_list (
        		self->priv->metadata_db, identity_id);
    ctx = (GSignondSecurityContext *) g_list_first (list);
    list = g_list_remove (list, ctx);
    gsignond_security_context_list_free (list);
    return ctx;
}

