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

#include "gsignond-identity-iface.h"
#include "gsignond-identity-enum-types.h"

enum 
{
    SIG_USER_VERIFIED,
    SIG_SECRET_VERIFIED,
    SIG_CREDENTIALS_UPDATED,
    SIG_INFO_UPDATED,
    SIG_MAX
};

guint signals[SIG_MAX];

G_DEFINE_INTERFACE (GSignondIdentityIface, gsignond_identity_iface, G_TYPE_OBJECT)

static gboolean
_dummy_request_credentials_update (GSignondIdentityIface *self,
                                   const gchar *message,
                                   const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) message;
    (void) ctx;

    return FALSE;
}

static GVariant *
_dummy_get_info (GSignondIdentityIface *self, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) ctx;

    return 0;
}

static const gchar *
_dummy_get_auth_session (GSignondIdentityIface *self, const gchar *method, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) method;
    (void) ctx;

    return NULL;
}

static gboolean
_dummy_verify_user (GSignondIdentityIface *self, const GVariant *params, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) params;
    (void) ctx;

    return FALSE;
}

static gboolean
_dummy_verify_secret (GSignondIdentityIface *self, const gchar *secret, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) secret;
    (void) ctx;

    return FALSE;
}

static void
_dummy_remove (GSignondIdentityIface *self, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) ctx;
}

static gboolean
_dummy_sign_out (GSignondIdentityIface *self, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) ctx;

    return FALSE;
}

static guint32
_dummy_store (GSignondIdentityIface *self, const GVariant *info, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) info;
    (void) ctx;

    return 0;
}

static gint32
_dummy_add_reference (GSignondIdentityIface *self, const gchar *reference, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) reference;
    (void) ctx;

    return 0;
}

static gint32
_dummy_remove_reference (GSignondIdentityIface *self, const gchar *reference, const GSignondSecurityContext *ctx)
{
    (void) self;
    (void) reference;
    (void) ctx;

    return 0;
}

static void
gsignond_identity_iface_default_init (GSignondIdentityIfaceInterface *self)
{
    self->request_credentials_update = _dummy_request_credentials_update;
    self->get_info = _dummy_get_info;
    self->get_auth_session = _dummy_get_auth_session;
    self->verify_user = _dummy_verify_user;
    self->verify_secret = _dummy_verify_secret;
    self->remove = _dummy_remove;
    self->sign_out = _dummy_sign_out;
    self->store = _dummy_store;
    self->add_reference = _dummy_add_reference;
    self->remove_reference = _dummy_remove_reference;

    signals[SIG_USER_VERIFIED] =  g_signal_new ("user-verified",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            G_TYPE_ERROR);

    signals[SIG_SECRET_VERIFIED] = g_signal_new ("secret-verified",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            G_TYPE_ERROR);

    signals[SIG_CREDENTIALS_UPDATED] = g_signal_new ("credentials-updated",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            2,
            G_TYPE_UINT,
            G_TYPE_ERROR);

    signals[SIG_INFO_UPDATED] = g_signal_new ("info-updated",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_IDENTITY_CHANGE_TYPE);
}

/**
 * gsignond_identity_iface_request_credentials_update:
 * @self: instance of #GSignondIdentityIface
 * @message: message to be shown to user
 * @ctx: security context of the caller
 *
 * Requests user to update username and password for this identity @identity.
 * Once the process is done, emits "credentials-updated" signal
 *
 * Returns: @TRUE on success, @FALSE otherwise
 */
gboolean
gsignond_identity_iface_request_credentials_update (GSignondIdentityIface *self,
                                                    const gchar *message,
                                                    const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        request_credentials_update (self, message, ctx);
}

/**
 * gsignond_identity_iface_get_info:
 * @self: instance of #GSignondIdentityIface
 * @message: message to be shown to user
 * @ctx: security context of the caller
 *
 * Retrieves the identity information for this identity, in the form of key, value pairs.
 *
 * Returns: (transfers full): identity info
 */
GVariant *
gsignond_identity_iface_get_info (GSignondIdentityIface *self, const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->get_info (self, ctx);
}

/**
 * gsignond_identity_iface_get_info:
 * @self: instance of #GSignondIdentityIface
 * @method: authentication method to use
 * @ctx: security context of the caller
 *
 * Creates new authentication process on #identity using #method, and returns
 * dbus object path for newly created object.
 *
 * Returns: (transfers none): object path if success, NULL otherwise
 */
const gchar *
gsignond_identity_iface_get_auth_session (GSignondIdentityIface *self,
                                          const gchar *method,
                                          const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        get_auth_session (self, method, ctx);
}

/**
 * gsignond_identity_iface_verify_user:
 * @self: instance of #GSignondIdentityIface
 * @params: authentication parameters to be verified
 * @ctx: security context of the caller
 *
 * Initiates user verification process.
 *
 * Returns: @TRUE if success, @FALSE otherwise
 */
gboolean
gsignond_identity_iface_verify_user (GSignondIdentityIface *self,
                                     const GVariant *params,
                                     const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        verify_user (self, params, ctx);
}

/**
 * gsignond_identity_iface_verify_secret:
 * @self: instance of #GSignondIdentityIface
 * @secret: secret to be verified
 * @ctx: security context of the caller
 *
 * Initiates secret verification process.
 *
 * Returns: @TRUE if success, @FALSE otherwise
 */
gboolean
gsignond_identity_iface_verify_secret (GSignondIdentityIface *self,
                                       const gchar *secret,
                                       const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        verify_secret (self, secret, ctx);
}

/**
 * gsignond_identity_iface_remove:
 * @self: instance of #GSignondIdentityIface
 * @ctx: security context of the caller
 *
 * Removes identity information from underlined database
 */
void
gsignond_identity_iface_remove (GSignondIdentityIface *self,
                                const GSignondSecurityContext *ctx)
{
    GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->remove (self, ctx);
}

/**
 * gsignond_identity_iface_sign_out:
 * @self: instance of #GSignondIdentityIface
 * @ctx: security context of the caller
 *
 * TODO: add description
 *
 * Returns: @TRUE if success, @FALSE otherwise
 */
gboolean
gsignond_identity_iface_sign_out (GSignondIdentityIface *self,
                                  const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->sign_out (self, ctx);
}

/**
 * gsignond_identity_iface_store:
 * @self: instance of #GSignondIdentityIface
 * @info: identity information to store
 * @ctx: security context of the caller
 *
 * Saves given identity information #info to database
 *
 * Returns: identity id
 */
guint32
gsignond_identity_iface_store (GSignondIdentityIface *self,
                               const GVariant *info,
                               const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->store (self, info, ctx);
}

/**
 * gsignond_identity_iface_add_reference:
 * @self: instance of #GSignondIdentityIface
 * @reference: reference name
 * @ctx: security context of the caller
 *
 * Adds a named reference #reference to identity
 *
 * Returns: identity id
 */
gint32
gsignond_identity_iface_add_reference (GSignondIdentityIface *self,
                                       const gchar *reference,
                                       const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        add_reference (self, reference, ctx);
}

/**
 * gsignond_identity_iface_remove_reference:
 * @self: instance of #GSignondIdentityIface
 * @reference: reference name
 * @ctx: security context of the caller
 *
 * Removes a named reference #reference to identity
 *
 * Returns: identity id
 */
gint32
gsignond_identity_iface_remove_reference (GSignondIdentityIface *self,
                                          const gchar *reference,
                                          const GSignondSecurityContext *ctx)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        add_reference (self, reference, ctx);
}

GSignondAccessControlManager *
gsignond_identity_iface_get_acm (GSignondIdentityIface *self)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        get_acm (self);
}

/**
 * gsignond_identity_iface_notify_user_verified:
 * @self: instance of #GSignondIdentityIface
 * @error: instance of #GError, error if any, that was occured in user verification process, otherwise NULL
 *
 * Emits "user-verified" signal
 */
void
gsignond_identity_iface_notify_user_verified (GSignondIdentityIface *self,
                                              const GError *error)
{
    g_signal_emit (self, signals[SIG_USER_VERIFIED], 0, error);
}

/**
 * gsignond_identity_iface_notify_secret_verified:
 * @self: instance of #GSignondIdentityIface
 * @error: instance of #GError, error if any, that was occured in secret verification process, otherwise NULL
 *
 * Emits "secret-verified" signal
 */
void
gsignond_identity_iface_notify_secret_verified (GSignondIdentityIface *self,
                                                const GError *error)
{
    g_signal_emit (self, signals[SIG_SECRET_VERIFIED], 0, error);
}

/**
 * gsignond_identity_iface_notify_credentials_updated:
 * @self: instance of #GSignondIdentityIface
 * @identity_id: id of the identity for which credentials updated
 * @error: instance of #GError, error if any, that was occured in updating credentials, otherwise NULL
 *
 * Emits "credentials-updated" signal along with identity id.
 */
void
gsignond_identity_iface_notify_credentials_updated (GSignondIdentityIface *self,
                                                    guint32 identity_id,
                                                    const GError *error)
{
    g_signal_emit (self, signals[SIG_CREDENTIALS_UPDATED], 0 , identity_id, error);
}

/**
 * gsignond_identity_iface_notify_info_updated:
 * @self: instance of #GSignondIdentityIface
 * @ichange: type of chagne 
 *
 * Emits "info-updated" signal along with type of change. This should be called by implementes
 * when identity info updated.
 */
void
gsignond_identity_iface_notify_info_updated (GSignondIdentityIface *self,
                                             GSignondIdentityChangeType change)
{
    g_signal_emit (self, signals[SIG_INFO_UPDATED], 0, change);
}

