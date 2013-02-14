/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Amarnath Valluri <amarnath.valluri@linux.intel.com>
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

#ifndef __GSIGNOND_IDENTITY_H_
#define __GSIGNOND_IDENTITY_H_

#include <glib.h>
#include <glib-object.h>

#include <gsignond/gsignond-identity-info.h>

#include "gsignond-auth-service-iface.h"

G_BEGIN_DECLS

#define GSIGNOND_TYPE_IDENTITY            (gsignond_identity_get_type())
#define GSIGNOND_IDENTITY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_IDENTITY, GSignondIdentity))
#define GSIGNOND_IDENTITY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_IDENTITY, GSignondIdentityClass))
#define GSIGNOND_IS_IDENTITY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_IDENTITY))
#define GSIGNOND_IS_IDENTITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_IDENTITY))
#define GSIGNOND_IDENTITY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_IDENTITY, GSignondIdentityClass))

typedef struct _GSignondIdentity GSignondIdentity;
typedef struct _GSignondIdentityClass GSignondIdentityClass;
typedef struct _GSignondIdentityPrivate GSignondIdentityPrivate;

struct _GSignondIdentity
{
    GObject parent;

    /* priv */
    GSignondIdentityPrivate *priv;
};

struct _GSignondIdentityClass
{
    GObjectClass parent_class;
};

GType gsignond_identity_get_type (void) G_GNUC_CONST;

GSignondIdentity * gsignond_identity_new (GSignondAuthServiceIface *owner,
                                          GSignondIdentityInfo *info,
                                          const gchar *app_context,
                                          gint timeout);

guint32 gsignond_identity_get_id (GSignondIdentity *identity);

gboolean gsignond_identity_set_id (GSignondIdentity *identity, guint32 id);

const gchar *
gsignond_identity_get_object_path (GSignondIdentity *identity);

G_END_DECLS

#endif /* __GSIGNOND_IDENTITY_H_ */
