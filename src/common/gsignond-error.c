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

#include "gsignond/gsignond-error.h"
#include <string.h>
#include <gio/gio.h>

/**
 * SECTION:gsignond-error
 * @short_description: error definitions and utilities
 * @title: Errors
 * @include: gsignond/gsignond-error.h
 *
 * This file provides GSignond error definitions and utilities.
 * When creating an error, use #GSIGNOND_ERROR for the error domain and errors 
 * from #GSignondError for the error code.
 * 
 * |[    GError* err = g_error_new(GSIGNOND_ERROR, GSIGNOND_ERROR_MISSING_DATA,
 *     "Not enough data");
 * ]| 
 */

/**
 * GSIGNOND_ERROR:
 *
 * This macro should be used when creating a #GError in GSignond plugins and extensions.
 * (for example with g_error_new() )
 */

/**
 * GSignondError:
 * @GSIGNOND_ERROR_NONE: No error
 * @GSIGNOND_ERROR_UNKNOWN: Catch-all for errors not distinguished by another code.
 * @GSIGNOND_ERROR_INTERNAL_SERVER: Signon Daemon internal error. 
 * @GSIGNOND_ERROR_INTERNAL_COMMUNICATION: Communication with Signon Daemon error. 
 * @GSIGNOND_ERROR_PERMISSION_DENIED: The operation cannot be performed due to insufficient client permissions. 
 * @GSIGNOND_ERROR_ENCRYPTION_FAILURE: Failure during data encryption/decryption. 
 * @GSIGNOND_ERROR_AUTH_SERVICE_ERR: Placeholder to rearrange enumeration - AuthService specific 
 * @GSIGNOND_ERROR_METHOD_NOT_KNOWN: The method with this name is not found.
 * @GSIGNOND_ERROR_SERVICE_NOT_AVAILABLE: The service is temporarily unavailable. 
 * @GSIGNOND_ERROR_INVALID_QUERY: Parameters for the query are invalid. 
 * @GSIGNOND_ERROR_IDENTITY_ERR: Placeholder to rearrange enumeration - Identity specific 
 * @GSIGNOND_ERROR_METHOD_NOT_AVAILABLE: The requested method is not available. 
 * @GSIGNOND_ERROR_IDENTITY_NOT_FOUND: The identity matching this Identity object was not found on the service. 
 * @GSIGNOND_ERROR_STORE_FAILED: Storing credentials failed. 
 * @GSIGNOND_ERROR_REMOVE_FAILED: Removing credentials failed. 
 * @GSIGNOND_ERROR_SIGN_OUT_FAILED: SignOut failed. 
 * @GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED: Identity operation was canceled by user. 
 * @GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE: Query failed. 
 * @GSIGNOND_ERROR_REFERENCE_NOT_FOUND: Trying to remove nonexistent reference. 
 * @GSIGNOND_ERROR_AUTH_SESSION_ERR: Placeholder to rearrange enumeration - AuthSession/PluginInterface specific 
 * @GSIGNOND_ERROR_MECHANISM_NOT_AVAILABLE: The requested mechanism is not available. 
 * @GSIGNOND_ERROR_MISSING_DATA: The SessionData object does not contain necessary information. 
 * @GSIGNOND_ERROR_INVALID_CREDENTIALS: The supplied credentials are invalid for the mechanism implementation. 
 * @GSIGNOND_ERROR_NOT_AUTHORIZED: Authorization failed. 
 * @GSIGNOND_ERROR_WRONG_STATE: An operation method has been called in a wrong state. 
 * @GSIGNOND_ERROR_OPERATION_NOT_SUPPORTED: The operation is not supported by the mechanism implementation. 
 * @GSIGNOND_ERROR_NO_CONNECTION: No Network connetion. 
 * @GSIGNOND_ERROR_NETWORK: Network connetion failed. 
 * @GSIGNOND_ERROR_SSL: Ssl connection failed. 
 * @GSIGNOND_ERROR_RUNTIME: Casting SessionData into subclass failed 
 * @GSIGNOND_ERROR_SESSION_CANCELED: Challenge was cancelled. 
 * @GSIGNOND_ERROR_TIMED_OUT: Challenge was timed out. 
 * @GSIGNOND_ERROR_USER_INTERACTION: User interaction dialog failed 
 * @GSIGNOND_ERROR_OPERATION_FAILED: Temporary failure in authentication. 
 * @GSIGNOND_ERROR_ENCRYPTION_FAILED: Failure during data encryption/decryption. 
 * @GSIGNOND_ERROR_TOS_NOT_ACCEPTED: User declined Terms of Service. 
 * @GSIGNOND_ERROR_FORGOT_PASSWORD: User requested reset password sequence. 
 * @GSIGNOND_ERROR_METHOD_OR_MECHANISM_NOT_ALLOWED: Method or mechanism not allowed for this identity. 
 * @GSIGNOND_ERROR_INCORRECT_DATE: Date time incorrect on device. 
 * @GSIGNOND_ERROR_USER_ERR: Placeholder to rearrange enumeration - User space specific 
 * 
 * This enum provides a list of errors that plugins and extensions can use.
 * 
 */

/**
 * gsignond_get_gerror_for_id:
 * @err: A #GSignondError specifying the error
 * @message: Format string for the error message
 * @...: parameters for the error string
 * 
 * A helper macro that creates a #GError with the proper gsignond domain
 */

#define GSIGNOND_ERROR_DOMAIN "gsignond"
#define _ERROR_PREFIX "com.google.code.AccountsSSO.gSingleSignOn.Error"

GDBusErrorEntry _gsignond_errors[] = 
{
    {GSIGNOND_ERROR_UNKNOWN, _ERROR_PREFIX".Unknown"},
    {GSIGNOND_ERROR_INTERNAL_SERVER, _ERROR_PREFIX".InternalServerError"},
    {GSIGNOND_ERROR_INTERNAL_COMMUNICATION, _ERROR_PREFIX".InternalCommunicationError"},
    {GSIGNOND_ERROR_PERMISSION_DENIED, _ERROR_PREFIX".PermissionDenied"},
    {GSIGNOND_ERROR_ENCRYPTION_FAILURE, _ERROR_PREFIX".EncryptionFailure"},

    {GSIGNOND_ERROR_METHOD_NOT_KNOWN, _ERROR_PREFIX".MethodNotKnown"},
    {GSIGNOND_ERROR_SERVICE_NOT_AVAILABLE, _ERROR_PREFIX".ServiceNotAvailable"},
    {GSIGNOND_ERROR_INVALID_QUERY, _ERROR_PREFIX".InvalidQuery"},

    {GSIGNOND_ERROR_METHOD_NOT_AVAILABLE, _ERROR_PREFIX".MethodNotAvailable"},
    {GSIGNOND_ERROR_IDENTITY_NOT_FOUND, _ERROR_PREFIX".IdentityNotFound"},
    {GSIGNOND_ERROR_STORE_FAILED, _ERROR_PREFIX".IdentityStoreFailed"},
    {GSIGNOND_ERROR_REMOVE_FAILED, _ERROR_PREFIX".IdentityRemoveFailed"},
    {GSIGNOND_ERROR_SIGN_OUT_FAILED, _ERROR_PREFIX".SignOutFailed"},
    {GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED, _ERROR_PREFIX".OperationCancled"},
    {GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE, _ERROR_PREFIX".CredentialsNotAvailable"},
    {GSIGNOND_ERROR_REFERENCE_NOT_FOUND, _ERROR_PREFIX".ReferenceNotFound"},

    {GSIGNOND_ERROR_MECHANISM_NOT_AVAILABLE, _ERROR_PREFIX".MechanismNotAvailable"},
    {GSIGNOND_ERROR_MISSING_DATA, _ERROR_PREFIX".MissingData"},
    {GSIGNOND_ERROR_INVALID_CREDENTIALS, _ERROR_PREFIX".InvalidCredentials"},
    {GSIGNOND_ERROR_NOT_AUTHORIZED, _ERROR_PREFIX".NotAutherized"},
    {GSIGNOND_ERROR_WRONG_STATE, _ERROR_PREFIX".WrongState"},
    {GSIGNOND_ERROR_OPERATION_NOT_SUPPORTED, _ERROR_PREFIX".OperationNotSupported"},
    {GSIGNOND_ERROR_NO_CONNECTION, _ERROR_PREFIX".NoConnection"},
    {GSIGNOND_ERROR_NETWORK, _ERROR_PREFIX".NoNetwork"},
    {GSIGNOND_ERROR_SSL, _ERROR_PREFIX".SSLError"},
    {GSIGNOND_ERROR_RUNTIME, _ERROR_PREFIX".RuntimeError"},
    {GSIGNOND_ERROR_SESSION_CANCELED, _ERROR_PREFIX".Cancled"},
    {GSIGNOND_ERROR_TIMED_OUT, _ERROR_PREFIX".TimedOut"},
    {GSIGNOND_ERROR_USER_INTERACTION, _ERROR_PREFIX".UserInteractionFailed"},
    {GSIGNOND_ERROR_OPERATION_FAILED, _ERROR_PREFIX".OperationFailed"},
    {GSIGNOND_ERROR_ENCRYPTION_FAILED, _ERROR_PREFIX".EncryptionFailed"},
    {GSIGNOND_ERROR_TOS_NOT_ACCEPTED, _ERROR_PREFIX".TOSNotAccepted"},

    {GSIGNOND_ERROR_FORGOT_PASSWORD, _ERROR_PREFIX".ForgotPassword"},
    {GSIGNOND_ERROR_METHOD_OR_MECHANISM_NOT_ALLOWED, _ERROR_PREFIX".MethodOrMechanismNotAllowed"},
    {GSIGNOND_ERROR_INCORRECT_DATE, _ERROR_PREFIX".IncorrectDate"},
} ;

/**
 * gsignond_error_quark:
 * 
 * Creates and returns a domain for GSignond errors.
 */
GQuark
gsignond_error_quark (void)
{
    static volatile gsize quark_volatile = 0;

    g_dbus_error_register_error_domain (GSIGNOND_ERROR_DOMAIN,
                                        &quark_volatile,
                                        _gsignond_errors,
                                        G_N_ELEMENTS (_gsignond_errors));

    return (GQuark) quark_volatile;
}

/**
 * gsignond_error_new_from_variant:
 * @var: instance of #GVariant
 *
 * Converts the GVariant to GError.
 *
 * Returns: (transfer full) #GError object if successful, NULL otherwise.
 */
GError *
gsignond_error_new_from_variant (
        GVariant *var)
{
    GError *error = NULL;
    gchar *message;
    GQuark domain;
    gint code;

    if (!var) {
        return NULL;
    }

    g_variant_get (var, "(uis)", &domain, &code, &message);
    error = g_error_new_literal (domain, code, message);
    g_free (message);
    return error;
}

/**
 * gsignond_error_to_variant:
 * @error: instance of #GError
 *
 * Converts the GError to GVariant.
 *
 * Returns: (transfer full) #GVariant object if successful, NULL otherwise.
 */
GVariant *
gsignond_error_to_variant (
        GError *error)
{
    if (!error) {
        return NULL;
    }

    return g_variant_new ("(uis)", error->domain, error->code, error->message);
}

