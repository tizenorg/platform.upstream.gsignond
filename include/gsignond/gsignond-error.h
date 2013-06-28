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

#define GSIGNOND_ERROR   (gsignond_error_quark())

typedef enum {
    GSIGNOND_ERROR_NONE,

    GSIGNOND_ERROR_UNKNOWN = 1,
    GSIGNOND_ERROR_INTERNAL_SERVER = 2,
    GSIGNOND_ERROR_INTERNAL_COMMUNICATION = 3,
    GSIGNOND_ERROR_PERMISSION_DENIED = 4,
    GSIGNOND_ERROR_ENCRYPTION_FAILURE,

    GSIGNOND_ERROR_AUTH_SERVICE_ERR = 100,      /* Placeholder to rearrange enumeration - AuthService specific */
    GSIGNOND_ERROR_METHOD_NOT_KNOWN,
    GSIGNOND_ERROR_SERVICE_NOT_AVAILABLE,
    GSIGNOND_ERROR_INVALID_QUERY,

    GSIGNOND_ERROR_IDENTITY_ERR = 200,          /* Placeholder to rearrange enumeration - Identity specific */
    GSIGNOND_ERROR_METHOD_NOT_AVAILABLE,
    GSIGNOND_ERROR_IDENTITY_NOT_FOUND,
    GSIGNOND_ERROR_STORE_FAILED,
    GSIGNOND_ERROR_REMOVE_FAILED,
    GSIGNOND_ERROR_SIGN_OUT_FAILED,
    GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED,
    GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE,
    GSIGNOND_ERROR_REFERENCE_NOT_FOUND,

    GSIGNOND_ERROR_AUTH_SESSION_ERR = 300,     /* Placeholder to rearrange enumeration - AuthSession/PluginInterface specific */
    GSIGNOND_ERROR_MECHANISM_NOT_AVAILABLE,
    GSIGNOND_ERROR_MISSING_DATA,
    GSIGNOND_ERROR_INVALID_CREDENTIALS,
    GSIGNOND_ERROR_NOT_AUTHORIZED,
    GSIGNOND_ERROR_WRONG_STATE,
    GSIGNOND_ERROR_OPERATION_NOT_SUPPORTED,
    GSIGNOND_ERROR_NO_CONNECTION,
    GSIGNOND_ERROR_NETWORK,
    GSIGNOND_ERROR_SSL,
    GSIGNOND_ERROR_RUNTIME,
    GSIGNOND_ERROR_SESSION_CANCELED,
    GSIGNOND_ERROR_TIMED_OUT,
    GSIGNOND_ERROR_USER_INTERACTION,
    GSIGNOND_ERROR_OPERATION_FAILED,
    GSIGNOND_ERROR_ENCRYPTION_FAILED,
    GSIGNOND_ERROR_TOS_NOT_ACCEPTED,
    GSIGNOND_ERROR_FORGOT_PASSWORD,
    GSIGNOND_ERROR_METHOD_OR_MECHANISM_NOT_ALLOWED,
    GSIGNOND_ERROR_INCORRECT_DATE,

    GSIGNOND_ERROR_USER_ERR = 400
   
} GSignondError;

GQuark
gsignond_error_quark (void);

GError *
gsignond_error_new_from_variant (
        GVariant *var);

GVariant *
gsignond_error_to_variant (
        GError *error);

#define gsignond_get_gerror_for_id(err, message, args...) \
    g_error_new (gsignond_error_quark(), err, message, ##args);

G_END_DECLS

#endif /* __GSIGNOND_ERROR_H__ */
