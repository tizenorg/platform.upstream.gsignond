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

GString*
gsignond_concat_domain_and_error (
        const gchar *str1,
        const gchar *str2)
{
    GString *str = NULL;
    g_return_val_if_fail (str1 != NULL && str2 != NULL, NULL);
    str = g_string_sized_new (strlen(str1)+strlen(str2)-1);
    g_string_printf (str,"[%s].%s\n",str1,str2);
    return str;
}

GString*
gsignond_prepend_domain_to_error_msg (const GError *err)
{
    GString *msg = NULL;
    const gchar *domain = NULL;
    g_return_val_if_fail (err != NULL, NULL);
    if (err->message != NULL) {
        domain = g_quark_to_string(err->domain);
        msg = gsignond_concat_domain_and_error(domain, err->message);
    }
    return msg;
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

