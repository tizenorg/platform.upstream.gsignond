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

#ifndef __GSIGNOND_IDENTITY_IFACE_H_
#define __GSIGNOND_IDENTITY_IFACE_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_IDENTITY_IFACE          (gsignond_identity_iface_get_type ())
#define GSIGNOND_IDENTITY_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_IDENTITY_IFACE, GSignondIdentityIface))
#define GSIGNOND_IS_IDENTITY_IFACE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_IDENTITY_IFACE))
#define GSIGNOND_IDENTITY_IFACE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_IDENTITY_IFACE, GSignondIdentityIfaceInterface))

typedef struct _GSignondIdentityIface GSignondIdentityIface; /* dummy */
typedef struct _GSignondIdentityIfaceInterface GSignondIdentityIfaceInterface;

struct _GSignondIdentityIfaceInterface {
    GTypeInterface parent;

    guint32   (*request_credentials_update) (GSignondIdentityIface *iface, const gchar *message);
    GVariant *(*get_info) (GSignondIdentityIface *iface);
    gboolean  (*verify_user) (GSignondIdentityIface *iface, const GVariant *params);
    gboolean  (*verify_secret) (GSignondIdentityIface *iface, const gchar *secret);
    void      (*remove) (GSignondIdentityIface *iface);
    gboolean  (*sign_out) (GSignondIdentityIface *iface);
    guint32   (*store) (GSignondIdentityIface *iface, const GVariant *info);
    gint32    (*add_reference) (GSignondIdentityIface *iface, const gchar *reference);
    gint32    (*remove_reference) (GSignondIdentityIface *iface, const gchar *reference);

};

GType gsignond_identity_iface_get_type (void);

guint32 gsignond_identity_iface_request_credentials_update (
                                                    GSignondIdentityIface *self,
                                                    const gchar *message);
GVariant * gsignond_identity_iface_get_info (GSignondIdentityIface *self);
gboolean gsignond_identity_iface_verify_user (GSignondIdentityIface *self,
                                              const GVariant *params);
gboolean gsignond_identity_iface_verify_secret (GSignondIdentityIface *self,
                                                const gchar *secret);
void gsignond_identity_iface_remove (GSignondIdentityIface *self);
gboolean gsignond_identity_iface_sign_out (GSignondIdentityIface *self);
guint32 gsignond_identity_iface_store (GSignondIdentityIface *self,
                                       const GVariant *info);
gint32 gsignond_identity_iface_add_reference (GSignondIdentityIface *self,
                                              const gchar *reference);
gint32 gsignond_identity_iface_remove_reference (GSignondIdentityIface *self,
                                                 const gchar *reference);

G_END_DECLS

#endif /* __GSIGNOND_IDENTITY_IFACE_H_ */
