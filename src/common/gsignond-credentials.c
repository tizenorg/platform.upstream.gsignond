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

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-credentials.h"

#define GSIGNOND_CREDENTIALS_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_TYPE_CREDENTIALS, \
                                           GSignondCredentialsPrivate))

G_DEFINE_TYPE (GSignondCredentials, gsignond_credentials, G_TYPE_OBJECT);

struct _GSignondCredentialsPrivate {
    guint32 id; /*the identity associated with the credentials.*/

    GString *username; /*username attached to the id*/

    GString *password; /*password attached to the id*/
};


static void
_gsignond_credentials_finalize (GObject *gobject)
{
    GSignondCredentials *self = GSIGNOND_CREDENTIALS (gobject);

    if (self->priv->username) {
        g_string_free (self->priv->username, TRUE);
        self->priv->username = NULL;
    }
    if (self->priv->password) {
        g_string_free (self->priv->password, TRUE);
        self->priv->password = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_credentials_parent_class)->finalize (gobject);
}

static void
gsignond_credentials_class_init (GSignondCredentialsClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = _gsignond_credentials_finalize;

    g_type_class_add_private (klass, sizeof (GSignondCredentialsPrivate));
}

static void
gsignond_credentials_init (GSignondCredentials *self)
{
    self->priv = GSIGNOND_CREDENTIALS_GET_PRIVATE (self);
    self->priv->id = 0;
    self->priv->username = NULL;
    self->priv->password = NULL;
}

/**
 * gsignond_credentials_new:
 *
 * Creates new #GSignondCredentials object
 * Returns : (transfer full) the #GSignondCredentials object
 *
 */
GSignondCredentials *
gsignond_credentials_new ()
{
    return GSIGNOND_CREDENTIALS (
            g_object_new (GSIGNOND_TYPE_CREDENTIALS, NULL));
}

/**
 * gsignond_credentials_set_data:
 *
 * @self: the object whose data is to be set.
 * @id: the identity associated with the credentials.
 * @username: the username.
 * @password: the password.
 *
 * Sets the data of the object.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_credentials_set_data(
        GSignondCredentials *self,
        const guint32 id,
        const gchar* username,
        const gchar* password)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), FALSE);

    self->priv->id = id;
    return gsignond_credentials_set_username (self, username) &&
           gsignond_credentials_set_password (self, password);
}

/**
 * gsignond_credentials_set_id:
 *
 * @self: the object whose id is to be set.
 * @id: the id.
 *
 * Sets the id of the GSignondCredentials object
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_credentials_set_id(
        GSignondCredentials *self,
        const guint32 id)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), FALSE);
    self->priv->id = id;
    return TRUE;
}

/**
 * gsignond_credentials_get_id:
 *
 * @self: the object whose id is to be set.
 *
 * Returns the id from the #GSignondCredentials object
 *
 * Returns: the id if the object is valid, NULL otherwise.
 */
guint32
gsignond_credentials_get_id(GSignondCredentials *self)
{
    /* TODO: define proper invalid id */
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), 0);
    return self->priv->id;
}

/**
 * gsignond_credentials_set_username:
 *
 * @self: the object whose username is to be set.
 * @username: the username.
 *
 * Sets the username of the GSignondCredentials object; old username is
 * freed if it exits
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_credentials_set_username(
        GSignondCredentials *self,
        const gchar* username)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), FALSE);

    if (self->priv->username) {
        g_string_free (self->priv->username, TRUE);
        self->priv->username = NULL;
    }
    if (username) {
        self->priv->username = g_string_new (username);
    }
    return TRUE;
}

/**
 * gsignond_credentials_get_username:
 *
 * @self: the object whose username is to be set.
 *
 * Returns the username from the #GSignondCredentials object
 *
 * Returns: the username if the object is valid, NULL otherwise.
 */
const gchar*
gsignond_credentials_get_username(GSignondCredentials *self)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), NULL);
    return self->priv->username ? self->priv->username->str : NULL;
}

/**
 * gsignond_credentials_set_password:
 *
 * @self: the object whose password is to be set.
 * @password: the password.
 *
 * Sets the password of the GSignondCredentials object; old password is
 * freed if it exits
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_credentials_set_password(
        GSignondCredentials *self,
        const gchar* password)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), FALSE);

    if (self->priv->password) {
        g_string_free (self->priv->password, TRUE);
        self->priv->password = NULL;
    }
    if (password) {
        self->priv->password = g_string_new (password);
    }
    return TRUE;
}

/**
 * gsignond_credentials_get_password:
 *
 * @self: the object whose password is to be set.
 *
 * Returns the password from the #GSignondCredentials object
 *
 * Returns: the password if the object is valid, NULL otherwise.
 */
const gchar*
gsignond_credentials_get_password(GSignondCredentials *self)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (self), NULL);
    return self->priv->password? self->priv->password->str : NULL;
}

/**
 * gsignond_credentials_equal:
 *
 * @one: the first credential to be compared.
 * @two: the second credential to be compared.
 *
 * Compares elements of two GSignondCredentials object for equality.
 *
 * Returns: TRUE if id, username and password are same for both credential
 * objects, FALSE otherwise.
 */
gboolean
gsignond_credentials_equal (
        GSignondCredentials *one,
        GSignondCredentials *two)
{
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (one), FALSE);
    g_return_val_if_fail (GSIGNOND_IS_CREDENTIALS (two), FALSE);

    if ((one != NULL && two == NULL) ||
        (one == NULL && two != NULL))
        return FALSE;

    if ( (one == two) ||
         ( (one->priv->id == two->priv->id) &&
           (one->priv->username && two->priv->username) &&
           g_string_equal(one->priv->username, two->priv->username) &&
           g_string_equal(one->priv->password, two->priv->password)) ) {
        return TRUE;
    }

    return FALSE;
}

