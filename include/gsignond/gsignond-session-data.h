/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
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

#ifndef __GSIGNOND_SESSION_DATA_H__
#define __GSIGNOND_SESSION_DATA_H__

#include <gsignond/gsignond-dictionary.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_SESSION_DATA (GSIGNOND_TYPE_DICTIONARY)

#define GSIGNOND_SESSION_DATA(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_TYPE_SESSION_DATA, \
                                           GSignondSessionData))
#define GSIGNOND_IS_SESSION_DATA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_TYPE_SESSION_DATA))

typedef GSignondDictionary GSignondSessionData;

/*!
 * Error codes for ui interaction.
 */
typedef enum {
    GSIGNOND_QUERY_ERROR_NONE = 0,        /**< No errors. */
    GSIGNOND_QUERY_ERROR_GENERAL,         /**< Generic error during interaction. */
    GSIGNOND_QUERY_ERROR_NO_SIGNONUI,     /**< Cannot send request to signon-ui. */
    GSIGNOND_QUERY_ERROR_BAD_PARAMETERS,  /**< Signon-Ui cannot create dialog based on
                                   the given UiSessionData. */
    GSIGNOND_QUERY_ERROR_CANCELED,        /**< User canceled action. Plugin should not
                                   retry automatically after this. */
    GSIGNOND_QUERY_ERROR_NOT_AVAILABLE,   /**< Requested ui is not available. For
                                   example browser cannot be started. */
    GSIGNOND_QUERY_ERROR_BAD_URL,         /**< Given url was not valid. */
    GSIGNOND_QUERY_ERROR_BAD_CAPTCHA,     /**< Given captcha image was not valid. */
    GSIGNOND_QUERY_ERROR_BAD_CAPTCHA_URL, /**< Given url for capctha loading was not
                                   valid. */
    GSIGNOND_QUERY_ERROR_REFRESH_FAILED,  /**< Refresh failed. */
    GSIGNOND_QUERY_ERROR_FORBIDDEN,       /**< Showing ui forbidden by ui policy. */
    GSIGNOND_QUERY_ERROR_FORGOT_PASSWORD  /**< User pressed forgot password. */
    //TODO add more errors
} GSignondQueryError;


//FIXME: all the other standard sessiondata and uisessiondata fields 
//should be added
const gchar*
gsignond_session_data_get_username(GSignondSessionData* data);

void
gsignond_session_data_set_username(GSignondSessionData* data, 
                                   const gchar* username);

const gchar*
gsignond_session_data_get_secret(GSignondSessionData* data);

void
gsignond_session_data_set_secret(GSignondSessionData* data, 
                                 const gchar* secret);

gboolean
gsignond_session_data_get_query_username(GSignondSessionData* data);

void
gsignond_session_data_set_query_username(GSignondSessionData* data, 
                                         gboolean query_username);

gboolean
gsignond_session_data_get_query_password(GSignondSessionData* data);

void
gsignond_session_data_set_query_password(GSignondSessionData* data, 
                                         gboolean query_password);

GSignondQueryError gsignond_session_data_get_query_error(
    GSignondSessionData* data);

void
gsignond_session_data_set_query_error(GSignondSessionData* data, 
                                         GSignondQueryError error);

G_END_DECLS

#endif /* __GSIGNOND_SESSION_DATA_H__ */
