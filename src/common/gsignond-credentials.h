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

#ifndef __GSIGNOND_CREDENTIALS_H__
#define __GSIGNOND_CREDENTIALS_H__

#include <glib.h>

G_BEGIN_DECLS


typedef struct {
    guint32 id; /*the identity associated with the credentials.*/

    GString *username; /*username attached to the id*/

    GString *password; /*password attached to the id*/
} GSignondCredentials;


GSignondCredentials*    gisgnond_credentials_new(
                            const guint32 id);

GSignondCredentials*    gsignond_credentials_new_data(
                            const guint32 id,
                            const gchar* username,
                            const gchar* password);

gboolean                gsignond_credentials_set_username(
                            GSignondCredentials *self,
                            const gchar* username);

gboolean                gsignond_credentials_set_password(
                            GSignondCredentials *self,
                            const gchar* password);

void                    gsignond_credentials_free(
                            GSignondCredentials *creds);

G_END_DECLS

#endif /* __GSIGNOND_CREDENTIALS_H__ */
