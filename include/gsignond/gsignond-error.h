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

/* inclusion guard */
#ifndef __GSIGNOND_ERROR_H__
#define __GSIGNOND_ERROR_H__

#include <glib.h>

G_BEGIN_DECLS

#define GSIGNOND_ERROR_DOMAIN "gsignond"

/**
 * GSIGNOND_ERROR:
 *
 */
#define GSIGNOND_ERROR   (gsignond_error_quark())

typedef enum {
    GSIGNOND_ERROR_NONE,

    GSIGNOND_ERROR_UNKNOWN = 1,               /**< Catch-all for errors not distinguished
                                        by another code. */
    GSIGNOND_ERROR_INTERNAL_SERVER = 2,        /**< Signon Daemon internal error. */
    GSIGNOND_ERROR_INTERNAL_COMMUNICATION = 3, /**< Communication with Signon Daemon
                                     error. */
    GSIGNOND_ERROR_PERMISSION_DENIED = 4,      /**< The operation cannot be performed due to
                                        insufficient client permissions. */
    GSIGNOND_ERROR_ENCRYPTION_FAILURE,         /**< Failure during data
                                     encryption/decryption. */
    GSIGNOND_ERROR_AUTH_SERVICE_ERR = 100,           /* Placeholder to rearrange enumeration
                                         - AuthService specific */
    GSIGNOND_ERROR_METHOD_NOT_KNOWN,            /**< The method with this name is not
                                     found. */
    GSIGNOND_ERROR_SERVICE_NOT_AVAILABLE,       /**< The service is temporarily
                                     unavailable. */
    GSIGNOND_ERROR_INVALID_QUERY,              /**< Parameters for the query are invalid. */
    GSIGNOND_ERROR_IDENTITY_ERR = 200,              /* Placeholder to rearrange enumeration
                                         - Identity specific */
    GSIGNOND_ERROR_METHOD_NOT_AVAILABLE,        /**< The requested method is not available. */
    GSIGNOND_ERROR_IDENTITY_NOT_FOUND,          /**< The identity matching this Identity
                                     object was not found on the service. */
    GSIGNOND_ERROR_STORE_FAILED,               /**< Storing credentials failed. */
    GSIGNOND_ERROR_REMOVE_FAILED,              /**< Removing credentials failed. */
    GSIGNOND_ERROR_SIGN_OUT_FAILED,             /**< SignOut failed. */
    GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED, /**< Identity operation was canceled by
                                     user. */
    GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE,   /**< Query failed. */
    GSIGNOND_ERROR_REFERENCE_NOT_FOUND,         /**< Trying to remove nonexistent
                                     reference. */
    GSIGNOND_ERROR_AUTH_SESSION_ERR = 300,      /* Placeholder to rearrange enumeration
                                     - AuthSession/AuthPluginInterface
                                     specific */
    GSIGNOND_ERROR_MECHANISM_NOT_AVAILABLE,     /**< The requested mechanism is not
                                     available. */
    GSIGNOND_ERROR_MISSING_DATA,               /**< The SessionData object does not contain
                                        necessary information. */
    GSIGNOND_ERROR_INVALID_CREDENTIALS,        /**< The supplied credentials are invalid for
                                        the mechanism implementation. */
    GSIGNOND_ERROR_NOT_AUTHORIZED,             /**< Authorization failed. */
    GSIGNOND_ERROR_WRONG_STATE,                /**< An operation method has been called in
                                        a wrong state. */
    GSIGNOND_ERROR_OPERATION_NOT_SUPPORTED,     /**< The operation is not supported by the
                                        mechanism implementation. */
    GSIGNOND_ERROR_NO_CONNECTION,              /**< No Network connetion. */
    GSIGNOND_ERROR_NETWORK,                   /**< Network connetion failed. */
    GSIGNOND_ERROR_SSL,                       /**< Ssl connection failed. */
    GSIGNOND_ERROR_RUNTIME,                   /**< Casting SessionData into subclass
                                     failed */
    GSIGNOND_ERROR_SESSION_CANCELED,           /**< Challenge was cancelled. */
    GSIGNOND_ERROR_TIMED_OUT,                  /**< Challenge was timed out. */
    GSIGNOND_ERROR_USER_INTERACTION,           /**< User interaction dialog failed */
    GSIGNOND_ERROR_OPERATION_FAILED,           /**< Temporary failure in authentication. */
    GSIGNOND_ERROR_ENCRYPTION_FAILED,          /**< @deprecated Failure during data
                                     encryption/decryption. */
    GSIGNOND_ERROR_TOS_NOT_ACCEPTED,            /**< User declined Terms of Service. */
    GSIGNOND_ERROR_FORGOT_PASSWORD,            /**< User requested reset password
                                     sequence. */
    GSIGNOND_ERROR_METHOD_OR_MECHANISM_NOT_ALLOWED, /**< Method or mechanism not allowed for
                                       this identity. */
    GSIGNOND_ERROR_INCORRECT_DATE,             /**< Date time incorrect on device. */
    GSIGNOND_ERROR_USER_ERR = 400                   /* Placeholder to rearrange enumeration
                                         - User space specific */
   
} GSignondError;

#define gsignond_gerr(error, handler) \
    G_STMT_START {                 \
        GString* msg = gsignond_prepend_domain_to_error_msg(error); \
        handler(msg->str); \
        g_string_free(msg, TRUE); \
    } G_STMT_END\

#define gsignond_error_gerr(err)       gsignond_gerr(err, g_error)

#define gsignond_critical_gerr(err)    gsignond_gerr(err, g_critical)

#define gsignond_warning_gerr(err)     gsignond_gerr(err, g_warning)

#define gsignond_message_gerr(err)     gsignond_gerr(err, g_message)

#define gsignond_debug_gerr(err)       gsignond_gerr(err, g_debug)

GQuark
gsignond_error_quark (void);

GString*
gsignond_concat_domain_and_error (
        const gchar *str1,
        const gchar *str2);

GString*
gsignond_prepend_domain_to_error_msg (const GError *err);

G_END_DECLS

#endif /* __GSIGNOND_ERROR_H__ */
