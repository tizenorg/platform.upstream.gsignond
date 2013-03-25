/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Amarnath Valluri<amarnath.valluri@linux.intel.com>
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

#include <gsignond/gsignond-signonui-data.h>

#define SIGNONUI_KEY_CAPTCHA_RESPONSE "CaptchaResponse"
#define SIGNONUI_KEY_CAPTCHA_URL "CaptchaUrl"
#define SIGNONUI_KEY_CAPTION "Caption"
#define SIGNONUI_KEY_CONFIRM "Confirm"
#define SIGNONUI_KEY_FINAL_URL "FinalUrl"
#define SIGNONUI_KEY_FORGOT_PASSWORD "ForgotPassword"
#define SIGNONUI_KEY_FORGOT_PASSWORD_URL "ForgotPasswordUrl"
#define SIGNONUI_KEY_MESSAGE "Message"
#define SIGNONUI_KEY_OPEN_URL "OpenUrl"
#define SIGNONUI_KEY_PASSWORD "Secret"
#define SIGNONUI_KEY_QUERY_ERROR_CODE "QueryErrorCode"
#define SIGNONUI_KEY_QUERY_PASSWORD "QueryPassword"
#define SIGNONUI_KEY_QUERY_USERNAME "QueryUserName"
#define SIGNONUI_KEY_REMEMBER_PASSWORD "RememberPassword"
#define SIGNONUI_KEY_REQUEST_ID "RequestId"
#define SIGNONUI_KEY_TEST_REPLY_VALUES "TestReplyValues"
#define SIGNONUI_KEY_TITLE "Title"
#define SIGNONUI_KEY_URL_RESPONSE "UrlResponse"
#define SIGNONUI_KEY_USERNAME "UserName"

const gchar*
gsignond_signonui_data_get_captcha_response (GSignondSignonuiData *data) 
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_CAPTCHA_RESPONSE);
}

void
gsignond_signonui_data_set_captcha_response (GSignondSignonuiData *data,
                                             const gchar *response)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_CAPTCHA_RESPONSE, response);
}

const gchar*
gsignond_signonui_data_get_captcha_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_CAPTCHA_URL);
}

void
gsignond_signonui_data_set_captcha_url (GSignondSignonuiData *data,
                                        const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_CAPTCHA_URL, url);
}

const gchar*
gsignond_signonui_data_get_caption (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_CAPTION);
}

void
gsignond_signonui_data_set_caption (GSignondSignonuiData *data,
                                    const gchar *caption)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_CAPTION, caption);
}

gboolean
gsignond_signonui_data_get_confirm (GSignondSignonuiData *data,
                                    gboolean *confirm)
{
    return gsignond_dictionary_get_boolean (data, SIGNONUI_KEY_CONFIRM, confirm);
}

void
gsignond_signonui_data_set_confirm (GSignondSignonuiData *data,
                                    gboolean confirm)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_CONFIRM, confirm);
}

const gchar*
gsignond_signonui_data_get_final_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_FINAL_URL);
}

void
gsignond_signonui_data_set_final_url (GSignondSignonuiData *data,
                                      const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_FINAL_URL, url);
}

gboolean
gsignond_signonui_data_get_forgot_password (GSignondSignonuiData *data,
                                            gboolean *forgot_password)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_FORGOT_PASSWORD,
                                            forgot_password);
}

void
gsignond_signonui_data_set_forgot_password (GSignondSignonuiData *data,
                                            gboolean forgot)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_FORGOT_PASSWORD, forgot);
}

const gchar*
gsignond_signonui_data_get_forgot_password_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_FORGOT_PASSWORD_URL);
}

void
gsignond_signonui_data_set_forgot_password_url (GSignondSignonuiData *data,
                                                const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_FORGOT_PASSWORD_URL, url);
}

const gchar*
gsignond_signonui_data_get_message (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_MESSAGE);
}

void
gsignond_signonui_data_set_message (GSignondSignonuiData *data,
                                    const gchar *message)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_MESSAGE, message);
}

const gchar*
gsignond_signonui_data_get_open_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_OPEN_URL);
}

void
gsignond_signonui_data_set_open_url (GSignondSignonuiData *data,
                                     const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_OPEN_URL, url);
}

const gchar*
gsignond_signonui_data_get_password (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_PASSWORD);
}

void
gsignond_signonui_data_set_password (GSignondSignonuiData *data,
                                     const gchar *password)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_PASSWORD, password);
}

gboolean
gsignond_signonui_data_get_query_error (GSignondSignonuiData *data,
                                        GSignondSignonuiError *error)
{
    return gsignond_dictionary_get_uint32 (data, 
                                          SIGNONUI_KEY_QUERY_ERROR_CODE, 
                                          error);
}

void
gsignond_signonui_data_set_query_error (GSignondSignonuiData *data,
                                        GSignondSignonuiError error)
{
    gsignond_dictionary_set_uint32 (data, SIGNONUI_KEY_QUERY_ERROR_CODE, error);
}

gboolean
gsignond_signonui_data_get_query_password (GSignondSignonuiData *data,
                                            gboolean *query_password)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_QUERY_PASSWORD,
                                            query_password);
}

void
gsignond_signonui_data_set_query_password (GSignondSignonuiData *data,
                                           gboolean query)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_QUERY_PASSWORD, query);
}

gboolean
gsignond_signonui_data_get_query_username (GSignondSignonuiData *data,
                                            gboolean *query_username)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_QUERY_USERNAME,
                                            query_username);
}

void
gsignond_signonui_data_set_query_username (GSignondSignonuiData *data,
                                           gboolean query)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_QUERY_USERNAME, query);
}

gboolean
gsignond_signonui_data_get_remember_password (GSignondSignonuiData *data,
                                            gboolean *remember_password)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_REMEMBER_PASSWORD,
                                            remember_password);
}

void
gsignond_signonui_data_set_remember_password (GSignondSignonuiData *data,
                                              gboolean remember)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_REMEMBER_PASSWORD, remember);
}

const gchar*
gsignond_signonui_data_get_request_id (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_REQUEST_ID);
}

void
gsignond_signonui_data_set_request_id (GSignondSignonuiData *data,
                                       const gchar *id)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_REQUEST_ID, id);
}

const gchar*
gsignond_signonui_data_get_test_reply (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_TEST_REPLY_VALUES);
}

void
gsignond_signonui_data_set_test_reply (GSignondSignonuiData *data,
                                       const gchar *reply)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_TEST_REPLY_VALUES, reply);
}

const gchar*
gsignond_signonui_data_get_title (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_TITLE);
}

void
gsignond_signonui_data_set_title (GSignondSignonuiData *data,
                                  const gchar* title)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_TITLE, title);
}

const gchar*
gsignond_signonui_data_get_url_response (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_URL_RESPONSE);
}

void
gsignond_signonui_data_set_url_response (GSignondSignonuiData *data,
                                         const gchar *response)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_URL_RESPONSE, response);
}

const gchar*
gsignond_signonui_data_get_username (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_USERNAME);
}

void
gsignond_signonui_data_set_username (GSignondSignonuiData *data,
                                     const gchar *username)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_USERNAME, username);
}

