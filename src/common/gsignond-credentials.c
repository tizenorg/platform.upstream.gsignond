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

#include "gsignond-credentials.h"

/**
 * gsignond_credentials_new:
 *
 * @id: the identity associated with the credentials.
 *
 * Creates the GSignondCredentials object.
 *
 * Returns: (transfer full) #GSignondCredentials if successful,
 * NULL otherwise.
 */
GSignondCredentials*
gsignond_credentials_new(
        const guint32 id)
{
    GSignondCredentials *creds = NULL;
    creds = (GSignondCredentials *)g_malloc0 (sizeof (GSignondCredentials));
    creds->id = id;
    return creds;
}

/**
 * gsignond_credentials_new_data:
 *
 * @id: the identity associated with the credentials.
 * @username: the username.
 * @password: the password.
 *
 * Creates the GSignondCredentials object.
 *
 * Returns: (transfer full) #GSignondCredentials if successful,
 * NULL otherwise.
 */
GSignondCredentials*
gsignond_credentials_new_data(
        const guint32 id,
        const gchar* username,
        const gchar* password)
{
    GSignondCredentials *creds = NULL;

    g_return_val_if_fail (username != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    creds = gsignond_credentials_new (id);
    creds->username = g_string_new (username);
    creds->password = g_string_new (password);
    return creds;
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
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (username != NULL, FALSE);

    if (self->username) {
        g_string_free (self->username, TRUE);
        self->username = NULL;
    }
    self->username = g_string_new (username);
    return TRUE;
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
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (password != NULL, FALSE);

    if (self->password) {
        g_string_free (self->password, TRUE);
        self->password = NULL;
    }
    self->password = g_string_new (password);
    return TRUE;
}

/**
 * gsignond_credentials_free:
 *
 * Frees the memory allocated for all the elements and the object itself.
 *
 */
void
gsignond_credentials_free(
        GSignondCredentials *creds)
{
    g_return_if_fail (creds != NULL);

    if (creds->username) {
        g_string_free (creds->username, TRUE);
        creds->username = NULL;
    }
    if (creds->password) {
        g_string_free (creds->password, TRUE);
        creds->password = NULL;
    }
    g_free (creds);
    creds = NULL;
}


