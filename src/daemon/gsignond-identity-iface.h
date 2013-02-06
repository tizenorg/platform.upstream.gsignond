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
#include <gsignond/gsignond-access-control-manager.h>
#include <gsignond/gsignond-security-context.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_IDENTITY_IFACE          (gsignond_identity_iface_get_type ())
#define GSIGNOND_IDENTITY_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_IDENTITY_IFACE, GSignondIdentityIface))
#define GSIGNOND_IS_IDENTITY_IFACE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_IDENTITY_IFACE))
#define GSIGNOND_IDENTITY_IFACE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_IDENTITY_IFACE, GSignondIdentityIfaceInterface))

typedef struct _GSignondIdentityIface GSignondIdentityIface; /* dummy */
typedef struct _GSignondIdentityIfaceInterface GSignondIdentityIfaceInterface;

typedef enum {
    GSIGNOND_IDENTITY_DATA_UPDATED = 1,
    GSIGNOND_IDENTITY_SIGNED_OUT,
    GSIGNOND_IDENTITY_REMOVED
} IdentityChangeType;
typedef IdentityChangeType GSignondIdentityChangeType;

struct _GSignondIdentityIfaceInterface {
    GTypeInterface parent;
    
   /**
     * request_credentials_update:
     * @identity: Instance of #GSignondIdentityIface
     * @message: message to be shown to user
     * @ctx: security context of the caller
     *
     * Requests user to update username and password for this identity @identity.
     * Once the process is done, emits "credentials-updated" signal
     *
     * Returns: @TRUE on success, @FALSE otherwise
     */
    gboolean (*request_credentials_update) (GSignondIdentityIface *identity, const gchar *message, const GSignondSecurityContext *ctx);

    /**
     * get_info:
     * @identity: Instance of #GSignondIdentityIface
     * @ctx: security context of the caller
     *
     * Retrives identity information stroed for this identity.
     *
     * Returns: (transfer full)identity information
     */
    GVariant *(*get_info) (GSignondIdentityIface *identity, const GSignondSecurityContext *ctx);

    /**
     * get_auth_session:
     * @identity: Instance of #GSignondIdentityIface
     * @method: authentication method to use
     * @ctx: security context of the caller
     *
     * Opens an authentication session for #identity using authentication method #method.
     *
     * Returns: (transfers none) Dbus object path of the session object
     */
    const gchar *   (*get_auth_session) (GSignondIdentityIface *identity, const gchar *method, const GSignondSecurityContext *ctx);
 
    /**
     * verify_user:
     * @identity: Instance of #GSignondIdentityIface
     * @params:
     * @ctx: security context of the caller
     *
     * Starts user verification process. Once the verification process completed
     * that is informed via #user_verified.
     *
     * Returns: @TRUE on success, @FALSE otherwise
     */
    gboolean  (*verify_user) (GSignondIdentityIface *identity, const GVariant *params, const GSignondSecurityContext *ctx);

    /**
     * verify_secret:
     * @identity: Instance of #GSignondIdentityIface
     * @secret: secret to verify
     * @ctx: security context of the caller
     *
     * Starts secret verification process. Once the verification process completed
     * that is informed via #secret_verified
     *
     * Returns: @TRUE on success, @FALSE otherwise
     */
    gboolean  (*verify_secret) (GSignondIdentityIface *identity, const gchar *secret, const GSignondSecurityContext *ctx);

    /**
     * remove:
     * @identity: Instance of #GSignondIdentityIface
     * @ctx: security context of the caller
     *
     * Removes identity from database
     */
    void      (*remove) (GSignondIdentityIface *identity, const GSignondSecurityContext *ctx);

    /**
     * store:
     * @identity: Instance of #GSignondIdentityIface
     * @info: information to be stored
     * @ctx: security context of the caller
     *
     * Stores the given identity information @info to database.
     *
     * Returns: id of the identity
     */
    guint32   (*store) (GSignondIdentityIface *identity, const GVariant *info, const GSignondSecurityContext *ctx);

    /**
     * sign_out:
     * @identity: Instance of #GSignondIdentityIface
     * @ctx: security context of the caller
     *
     * Returns: @TRUE on success, @FALSE otherwise.
     */
    gboolean  (*sign_out) (GSignondIdentityIface *identity, const GSignondSecurityContext *ctx);

    /**
     * add_reference:
     * @identity: Instance of #GSignondIdentityIface
     * @reference: named referece
     * @ctx: security context of the caller
     *
     * Adds a named referece to identity.
     *
     * Returns: identity id
     */
    gint32    (*add_reference) (GSignondIdentityIface *identity, const gchar *reference, const GSignondSecurityContext *ctx);

    /**
     * remove_reference:
     * @identity: Instance of #GSignondIdentityIface
     * @reference: name of the reference to be removed
     * @ctx: security context of the caller
     *
     * Removes named reference @reference form list of references of identity @identity.
     *
     * Returns: identity id
     */
    gint32    (*remove_reference) (GSignondIdentityIface *identity, const gchar *reference, const GSignondSecurityContext *ctx);

    /**
     * get_acm:
     * @identity: Instance of #GSignondIdentityIface
     *
     * Retrieves pointer to access control manager.
     *
     * Returns: (transfer none): 
     */
    GSignondAccessControlManager * (*get_acm) (GSignondIdentityIface *identity);
};

GType gsignond_identity_iface_get_type (void);

gboolean
gsignond_identity_iface_request_credentials_update (
                                                    GSignondIdentityIface *self,
                                                    const gchar *message,
                                                    const GSignondSecurityContext *ctx);
GVariant * 
gsignond_identity_iface_get_info (GSignondIdentityIface *self,
                                  const GSignondSecurityContext *ctx);

const gchar *
gsignond_identity_iface_get_auth_session (GSignondIdentityIface *iface,
                                          const gchar *method,
                                          const GSignondSecurityContext *ctx);

gboolean 
gsignond_identity_iface_verify_user (GSignondIdentityIface *self,
                                     const GVariant *params,
                                     const GSignondSecurityContext *ctx);

gboolean 
gsignond_identity_iface_verify_secret (GSignondIdentityIface *self,
                                       const gchar *secret,
                                       const GSignondSecurityContext *ctx);

void 
gsignond_identity_iface_remove (GSignondIdentityIface *self,
                                const GSignondSecurityContext *ctx);

gboolean 
gsignond_identity_iface_sign_out (GSignondIdentityIface *self,
                                  const GSignondSecurityContext *ctx);

guint32 
gsignond_identity_iface_store (GSignondIdentityIface *self,
                               const GVariant *info,
                               const GSignondSecurityContext *ctx);

gint32 
gsignond_identity_iface_add_reference (GSignondIdentityIface *self,
                                       const gchar *reference,
                                       const GSignondSecurityContext *ctx);

gint32 
gsignond_identity_iface_remove_reference (GSignondIdentityIface *self,
                                          const gchar *reference,
                                          const GSignondSecurityContext *ctx);

GSignondAccessControlManager *
gsignond_identity_iface_get_acm (GSignondIdentityIface *self);

void 
gsignond_identity_iface_notify_user_verified (GSignondIdentityIface *identity, 
                                              const GError *error);

void
gsignond_identity_iface_notify_secret_verified (GSignondIdentityIface *identity, 
                                                const GError *error);

void
gsignond_identity_iface_notify_credentials_updated (GSignondIdentityIface *identity,
                                                    guint32 identity_id,
                                                    const GError *error);

void
gsignond_identity_iface_notify_info_updated (GSignondIdentityIface *self,
                                             GSignondIdentityChangeType change);

G_END_DECLS

#endif /* __GSIGNOND_IDENTITY_IFACE_H_ */
