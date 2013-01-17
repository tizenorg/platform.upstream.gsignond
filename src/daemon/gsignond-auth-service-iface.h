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

#ifndef __GSIGNOND_AUTH_SERVICE_IFACE_H_
#define __GSIGNOND_AUTH_SERVICE_IFACE_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_AUTH_SERVICE_IFACE          (gsignond_auth_service_iface_get_type ())
#define GSIGNOND_AUTH_SERVICE_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_AUTH_SERVICE_IFACE, GSignondAuthServiceIface))
#define GSIGNOND_IS_AUTH_SERVICE_IFACE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_AUTH_SERVICE_IFACE))
#define GSIGNOND_AUTH_SERVICE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_AUTH_SERVICE_IFACE, GSignondAuthServiceIfaceInterface))

typedef struct _GSignondAuthServiceIface GSignondAuthServiceIface; /* dummy */
typedef struct _GSignondAuthServiceIfaceInterface GSignondAuthServiceIfaceInterface;

struct _GSignondAuthServiceIfaceInterface {
    GTypeInterface parent;

    const gchar * (*register_new_identity) (GSignondAuthServiceIface *iface, const GVariant *app_context);
    gboolean (*get_identity) (GSignondAuthServiceIface *iface, guint32 id, const GVariant *app_context, gchar **object_path, GVariant **identity_data);
    const gchar * (*get_auth_session_object_path) (GSignondAuthServiceIface *iface, guint32 id, const gchar *type, const GVariant *app_context);
    gchar ** (*query_methods) (GSignondAuthServiceIface *iface);
    gchar ** (*query_mechanisms) (GSignondAuthServiceIface *iface, const gchar *method);
    GVariant * (*query_identities) (GSignondAuthServiceIface *iface, const GVariant *filter);
    gboolean (*clear) (GSignondAuthServiceIface *iface);

};

GType gsignond_auth_service_iface_get_type (void);

const gchar * gsignond_auth_service_iface_register_new_identity (
                                                GSignondAuthServiceIface *self,
                                                const GVariant *app_context);
gboolean gsignond_auth_service_iface_get_identity (
                                                GSignondAuthServiceIface *self,
                                                guint32 id,
                                                const GVariant *app_context, 
                                                gchar **object_path,
                                                GVariant **identity_data);
const gchar * gsignond_auth_service_iface_get_auth_session_object_path (
                                                GSignondAuthServiceIface *self,
                                                guint32 id,
                                                const gchar *type,
                                                const GVariant *app_context);
gchar ** gsignond_auth_service_iface_query_methods (
                                                GSignondAuthServiceIface *self);
gchar ** gsignond_auth_service_iface_query_mechanisms (
                                                GSignondAuthServiceIface *self,
                                                const gchar *method);
GVariant * gsignond_auth_service_iface_query_identities (
                                                GSignondAuthServiceIface *self,
                                                const GVariant *filter);
gboolean gsignond_auth_service_iface_clear (GSignondAuthServiceIface *self);

G_END_DECLS

#endif /* __GSIGNOND_AUTH_SERVICE_IFACE_H_ */
