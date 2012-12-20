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
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_CREDENTIALS   \
                                       (gsignond_credentials_get_type ())
#define GSIGNOND_CREDENTIALS(obj)   (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_TYPE_CREDENTIALS, \
                                           GSignondCredentials))
#define GSIGNOND_IS_CREDENTIALS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           GSIGNOND_TYPE_CREDENTIALS))
#define GSIGNOND_CREDENTIALS_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                             GSIGNOND_TYPE_CREDENTIALS, \
                                             GSignondCredentialsClass))
#define GSIGNOND_IS_CREDENTIALS_CLASS(klass) \
                                            (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                             GSIGNOND_TYPE_CREDENTIALS))
#define GSIGNOND_CREDENTIALS_GET_CLASS(obj) \
                                            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                             GSIGNOND_TYPE_CREDENTIALS, \
                                             GSignondCredentialsClass))

typedef struct _GSignondCredentialsPrivate GSignondCredentialsPrivate;

typedef struct {
    GObject parent_instance;

    /*< private >*/
    GSignondCredentialsPrivate *priv;
} GSignondCredentials;

typedef struct {
    GObjectClass parent_class;

} GSignondCredentialsClass;

/* used by GSIGNOND_TYPE_CREDENTIALS */
GType                   gsignond_credentials_get_type (void);

GSignondCredentials*    gisgnond_credentials_new(void);

gboolean                gsignond_credentials_set_data(
                            GSignondCredentials *self,
                            const guint32 id,
                            const gchar* username,
                            const gchar* password);

gboolean                gsignond_credentials_set_id(
                            GSignondCredentials *self,
                            const guint32 id);

guint32                 gsignond_credentials_get_id(
                            GSignondCredentials *self);

gboolean                gsignond_credentials_set_username(
                            GSignondCredentials *self,
                            const gchar* username);

const gchar*            gsignond_credentials_get_username(
                            GSignondCredentials *self);

gboolean                gsignond_credentials_set_password(
                            GSignondCredentials *self,
                            const gchar* password);

const gchar*            gsignond_credentials_get_password(
                            GSignondCredentials *self);

gboolean                gsignond_credentials_equal (
                            GSignondCredentials *one,
                            GSignondCredentials *two);

G_END_DECLS

#endif /* __GSIGNOND_CREDENTIALS_H__ */
