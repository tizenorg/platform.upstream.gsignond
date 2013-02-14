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
#include <gsignond/gsignond-access-control-manager.h>
#include <gsignond/gsignond-security-context.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_AUTH_SERVICE_IFACE          (gsignond_auth_service_iface_get_type ())
#define GSIGNOND_AUTH_SERVICE_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_AUTH_SERVICE_IFACE, GSignondAuthServiceIface))
#define GSIGNOND_IS_AUTH_SERVICE_IFACE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_AUTH_SERVICE_IFACE))
#define GSIGNOND_AUTH_SERVICE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_AUTH_SERVICE_IFACE, GSignondAuthServiceIfaceInterface))

typedef struct _GSignondAuthServiceIface GSignondAuthServiceIface; /* dummy */
typedef struct _GSignondAuthServiceIfaceInterface GSignondAuthServiceIfaceInterface;

struct _GSignondAuthServiceIfaceInterface {
    GTypeInterface parent;

    /**
     * register_new_identity:
     * @auth_service: An instance of #GSignondAuthServiceIface
     * @ctx: Security context
     *
     * Creates new identity object and registers it on the DBus. Returns the
     * dbus object of the newly created identity.
     *
     * Returns: (transfer null) object of the newly created identity if success, @NULL otherwise.
     */
    const gchar * (*register_new_identity) (GSignondAuthServiceIface *auth_service, const GSignondSecurityContext *ctx);

    /**
     * get_identity:
     * @auth_service: An instance of #GSignondAuthServiceIface
     * @id: id to query
     * @ctx: Security context
     * @object_path: (transfers full) holds the object path of the identity
     * @identity_data: (transfers full) holds the identity data
     *
     * Retrives idnetity information stored and object path on DBus with the given id @id.
     *
     * Returns: (transfer null) object of the newly created identity if success, @NULL otherwise.
     */
    const gchar* (*get_identity) (GSignondAuthServiceIface *auth_service, guint32 id, const GSignondSecurityContext *ctx, GVariant **identity_data);

    /**
     * query_methods:
     * @auth_service: An instance of #GSignondAuthServiceIface
     *
     * Retrieves the supported authentication methods on this service object.
     *
     * Returns: (transfer full): list of authentication methods. Caller should call g_strfreev()
     * once done with results.
     */
    gchar ** (*query_methods) (GSignondAuthServiceIface *auth_service);

    /**
     * query_mechanisms:
     * @auth_service: An instance of #GSignondAuthServiceIface
     * @method: authentication method
     *
     * Retrieves the supported authentication mechanisms for given @method on this service object.
     *
     * Returns: (transfer full): list of authentication mechanisms. Caller should call g_strfreev()
     * once done with results.
     */
    gchar ** (*query_mechanisms) (GSignondAuthServiceIface *auth_service, const gchar *method);

    /**
     * query_identities:
     * @auth_service: An instance of #GSignondAuthServiceIface
     * @filter: filter to be applied
     *
     * Query for stored identities which satisifies the given filter @filter.
     *
     * Returns: (transfers full) identity list
     */
    GVariant * (*query_identities) (GSignondAuthServiceIface *auth_service, const GVariant *filter);

    /**
     * clear:
     * @auth_service: An instance of #GSignondAuthServiceIface
     *
     * Clears all the identity information stored.
     *
     * Retruns: @TRUE on success, @FALSE otherwise
     */
    gboolean (*clear) (GSignondAuthServiceIface *auth_service);

    /**
     * get_acm
     * @auth_service: An instance of #GSignondAuthServiceIface
     *
     * Retrives access control manager used by #auth_service.
     *
     * Returns: (transfers none): access control manager
     */
    GSignondAccessControlManager * (*get_acm) (GSignondAuthServiceIface *auth_service);
};

GType gsignond_auth_service_iface_get_type (void);

const gchar * 
gsignond_auth_service_iface_register_new_identity (
                                                GSignondAuthServiceIface *self,
                                                const GSignondSecurityContext *ctx);
const gchar *
gsignond_auth_service_iface_get_identity (GSignondAuthServiceIface *self,
                                          guint32 id,
                                          const GSignondSecurityContext *ctx,
                                          GVariant **identity_data);
char **
gsignond_auth_service_iface_query_methods (GSignondAuthServiceIface *self);
gchar **
gsignond_auth_service_iface_query_mechanisms (GSignondAuthServiceIface *self,
                                              const gchar *method);
GVariant *
gsignond_auth_service_iface_query_identities (GSignondAuthServiceIface *self,
                                              const GVariant *filter);
gboolean
gsignond_auth_service_iface_clear (GSignondAuthServiceIface *self);

GSignondAccessControlManager *
gsignond_auth_service_iface_get_acm (GSignondAuthServiceIface *self);

G_END_DECLS

#endif /* __GSIGNOND_AUTH_SERVICE_IFACE_H_ */
