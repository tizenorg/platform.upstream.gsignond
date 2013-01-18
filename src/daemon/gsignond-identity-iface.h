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
    
   /**
     * request_credentials_update:
     * @identity: Instance of #GSignondIdentityIface
     * @message: message to be shown to user
     *
     * Requests user to update username and password for this identity @identity.
     *
     * Returns: identity id.
     * FIXME: What is the use of returning identity id
     */
    guint32   (*request_credentials_update) (GSignondIdentityIface *identity, const gchar *message);

    /**
     * get_info:
     * @identity: Instance of #GSignondIdentityIface
     *
     * Retrives identity information stroed for this identity.
     *
     * Returns: (transfer full)identity information
     */
    GVariant *(*get_info) (GSignondIdentityIface *identity);

    /**
     * get_auth_session:
     * @identity: Instance of #GSignondIdentityIface
     * @method: authentication method to use
     *
     * Opens an authentication session for #identity using authentication method #method.
     *
     * Returns: (transfers none) Dbus object path of the session object
     */
    const gchar *   (*get_auth_session) (GSignondIdentityIface *identity, const gchar *method);
 
    /**
     * verify_user:
     * @identity: Instance of #GSignondIdentityIface
     * @params:
     *
     * Starts user verification process. Once the verification process completed
     * that is informed via #user_verified.
     *
     * Returns: @TRUE on success, @FALSE otherwise
     */
    gboolean  (*verify_user) (GSignondIdentityIface *identity, const GVariant *params);

    /**
     * verify_secret:
     * @identity: Instance of #GSignondIdentityIface
     * @secret: secret to verify
     *
     * Starts secret verification process. Once the verification process completed
     * that is informed via #secret_verified
     *
     * Returns: @TRUE on success, @FALSE otherwise
     */
    gboolean  (*verify_secret) (GSignondIdentityIface *identity, const gchar *secret);

    /**
     * remove:
     * @identity: Instance of #GSignondIdentityIface
     *
     * Removes identity from database
     */
    void      (*remove) (GSignondIdentityIface *identity);

    /**
     * store:
     * @identity: Instance of #GSignondIdentityIface
     * @info: information to be stored
     *
     * Stores the given identity information @info to database.
     *
     * Returns: id of the identity
     */
    guint32   (*store) (GSignondIdentityIface *identity, const GVariant *info);

    /**
     * sign_out:
     * @identity: Instance of #GSignondIdentityIface
     *
     * Returns: @TRUE on success, @FALSE otherwise.
     */
    gboolean  (*sign_out) (GSignondIdentityIface *identity);

    /**
     * add_reference:
     * @identity: Instance of #GSignondIdentityIface
     * @reference: named referece
     *
     * Adds a named referece to identity.
     *
     * Returns: identity id
     */
    gint32    (*add_reference) (GSignondIdentityIface *identity, const gchar *reference);

    /**
     * remove_reference:
     * @identity: Instance of #GSignondIdentityIface
     * @reference: name of the reference to be removed
     *
     * Removes named reference @reference form list of references of identity @identity.
     *
     * Returns: identity id
     */
    gint32    (*remove_reference) (GSignondIdentityIface *identity, const gchar *reference);

    /**
     * user_verified:
     * ®identity: Instance of #GSignondIdentityIface
     * @error: (transfers none): instance of #GError, in case of error occured.
     *
     * Function to be called in responce to #verify_user, when user verification process completed.
     */
    void      (*user_verified) (GSignondIdentityIface *identity, const GError *error);

    /**
     * secret_verified
     * ®identity: Instance of #GSignondIdentityIface
     * @error: (transfers none): instance of #GError, in case of error occured.
     *
     * Function to be called in responce to #verify_secret, when user verification process completed.
     */
    void      (*secret_verified) (GSignondIdentityIface *identity, const GError *error);
};

GType gsignond_identity_iface_get_type (void);

guint32 
gsignond_identity_iface_request_credentials_update (
                                                    GSignondIdentityIface *self,
                                                    const gchar *message);
GVariant * 
gsignond_identity_iface_get_info (GSignondIdentityIface *self);

const gchar *
gsignond_identity_iface_get_auth_session (GSignondIdentityIface *iface,
                                          const gchar *method);

gboolean 
gsignond_identity_iface_verify_user (GSignondIdentityIface *self,
                                     const GVariant *params);

gboolean 
gsignond_identity_iface_verify_secret (GSignondIdentityIface *self,
                                       const gchar *secret);

void 
gsignond_identity_iface_remove (GSignondIdentityIface *self);

gboolean 
gsignond_identity_iface_sign_out (GSignondIdentityIface *self);

guint32 
gsignond_identity_iface_store (GSignondIdentityIface *self,
                               const GVariant *info);

gint32 
gsignond_identity_iface_add_reference (GSignondIdentityIface *self,
                                       const gchar *reference);

gint32 
gsignond_identity_iface_remove_reference (GSignondIdentityIface *self,
                                          const gchar *reference);

void 
gsignond_identity_iface_notify_user_verified (GSignondIdentityIface *identity, 
                                              const GError *error);

void
gsignond_identity_iface_notify_secret_verified (GSignondIdentityIface *identity, 
                                                const GError *error);

void gsignond_identity_iface_notify_user_verified (GSignondIdentityIface *identity, 
                                                   const GError *error);

void gsignond_identity_iface_notify_secret_verified (GSignondIdentityIface *identity, 
                                                     const GError *error);

G_END_DECLS

#endif /* __GSIGNOND_IDENTITY_IFACE_H_ */
