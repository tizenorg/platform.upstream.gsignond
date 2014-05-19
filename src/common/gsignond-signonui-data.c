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

/**
 * SECTION:gsignond-signonui-data
 * @short_description: definitions for user interaction parameters
 * @title: GSignondSignonuiData
 * @include: gsignond/gsignond-signonui-data.h
 *
 * This file provides commonly used parameters for user interaction during
 * authentication sessions.
 * For each of those a getter and setter is defined, on #GSignondSessionData
 * container. 
 * 
 * This container is used in two directions: by plugins to specify the parameters 
 * for user interaction that is then performed by SignonUI component, and by SignonUI
 * to return the results of that interaction to the plugins. See #GSignondPlugin
 * for the user interaction API from the plugins' perspective.
 * 
 * The parameters that are set by the plugin and read by signon UI are captcha url,
 * caption, confirm, final url, forgot password, forgot password url, message,
 * open url, password, query password, query username, remember
 * password, request id, test reply values, title, username.
 * 
 * The parameters that are returned by signon UI to the plugin are captcha response,
 * password, query error code, remember password, url response.
 */

/**
 * GSignondSignonuiData:
 * 
 * #GSignondSignonuiData is simply a typedef for #GSignondDictionary, which 
 * means the developers may also freely use methods associated with that structure,
 * in particular for creating a #GSignondSignonuiData object with 
 * gsignond_dictionary_new().
 */

/**
 * GSignondSignonuiError:
 * @SIGNONUI_ERROR_NONE: No errors
 * @SIGNONUI_ERROR_GENERAL: Generic error during interaction
 * @SIGNONUI_ERROR_NO_SIGNONUI: Cannot send request to signon-ui
 * @SIGNONUI_ERROR_BAD_PARAMETERS: Signon-Ui cannot create dialog based on the given UiSessionData
 * @SIGNONUI_ERROR_CANCELED: User canceled action. Plugin should not retry automatically after this
 * @SIGNONUI_ERROR_NOT_AVAILABLE: Requested ui is not available. For example browser cannot be started
 * @SIGNONUI_ERROR_BAD_URL: Given url was not valid
 * @SIGNONUI_ERROR_BAD_CAPTCHA: Given captcha image was not valid
 * @SIGNONUI_ERROR_BAD_CAPTCHA_URL: Given url for capctha loading was not valid
 * @SIGNONUI_ERROR_REFRESH_FAILED: Refresh failed
 * @SIGNONUI_ERROR_FORBIDDEN: Showing ui forbidden by ui policy
 * @SIGNONUI_ERROR_FORGOT_PASSWORD: User pressed forgot password
 * 
 * This enum defines errors that may happen during user interaction.
 */


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

/**
 * gsignond_signonui_data_get_captcha_response:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the user's response to a captcha query.
 * 
 * Returns: (transfer none): the string entered by the user in response to a captcha query.
 */
const gchar*
gsignond_signonui_data_get_captcha_response (GSignondSignonuiData *data) 
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_CAPTCHA_RESPONSE);
}

/**
 * gsignond_signonui_data_set_captcha_response:
 * @data: a #GSignondDictionary structure
 * @response: the string entered by the user in response to a captcha query.
 * 
 * A setter for the user's response to a captcha query.
 */
void
gsignond_signonui_data_set_captcha_response (GSignondSignonuiData *data,
                                             const gchar *response)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_CAPTCHA_RESPONSE, response);
}

/**
 * gsignond_signonui_data_get_captcha_url:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the captcha URL.
 * 
 * Returns: (transfer none): the URL to the captcha image to be verified by user.
 */
const gchar*
gsignond_signonui_data_get_captcha_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_CAPTCHA_URL);
}

/**
 * gsignond_signonui_data_set_captcha_url:
 * @data: a #GSignondDictionary structure
 * @url: the URL to the captcha image to be verified by user
 * 
 * A setter for the captcha URL.
 */
void
gsignond_signonui_data_set_captcha_url (GSignondSignonuiData *data,
                                        const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_CAPTCHA_URL, url);
}

/**
 * gsignond_signonui_data_get_caption:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the caption string. Caption tells the user which 
 * application/credentials/provider is requestion authentication.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_caption (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_CAPTION);
}

/**
 * gsignond_signonui_data_set_caption:
 * @data: a #GSignondDictionary structure
 * @caption: the caption string
 * 
 * A setter for the caption string. Caption tells the user which 
 * application/credentials/provider is requestion authentication.
 * 
 */
void
gsignond_signonui_data_set_caption (GSignondSignonuiData *data,
                                    const gchar *caption)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_CAPTION, caption);
}

/**
 * gsignond_signonui_data_get_confirm:
 * @data: a #GSignondDictionary structure
 * @confirm: the value for the property is written here
 * 
 * A getter for the confirm mode. In confirm mode the user is asked to enter
 * an old password (which is compared to the supplied password), and a new password twice
 * (which is returned).
 * 
 * Returns: whether this property exists in the @data dictionary or not.
 */
gboolean
gsignond_signonui_data_get_confirm (GSignondSignonuiData *data,
                                    gboolean *confirm)
{
    return gsignond_dictionary_get_boolean (data, SIGNONUI_KEY_CONFIRM, confirm);
}

/**
 * gsignond_signonui_data_set_confirm:
 * @data: a #GSignondDictionary structure
 * @confirm: the value for the property
 * 
 * A setter for the confirm mode. In confirm mode the user is asked to enter
 * an old password (which is compared to the supplied password), and a new password twice
 * (which is returned).
 * 
 */
void
gsignond_signonui_data_set_confirm (GSignondSignonuiData *data,
                                    gboolean confirm)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_CONFIRM, confirm);
}

/**
 * gsignond_signonui_data_get_final_url:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the final URL. When the signon UI detects that the user is at 
 * the final URL (possibly with additional query or fragment parameters), it 
 * will close the window and return the full URL via url response property.
 * This is used by redirection-based authentication, such as OAuth.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_final_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_FINAL_URL);
}

/**
 * gsignond_signonui_data_set_final_url:
 * @data: a #GSignondDictionary structure
 * @url: the final url
 * 
 * A setter for the final URL. When the signon UI detects that the user is at 
 * the final URL (possibly with additional query or fragment parameters), it 
 * will close the window and return the full URL via url response property.
 * This is used by redirection-based authentication, such as OAuth.
 * 
 */
void
gsignond_signonui_data_set_final_url (GSignondSignonuiData *data,
                                      const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_FINAL_URL, url);
}

/**
 * gsignond_signonui_data_get_forgot_password:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the forgot password string which is shown to the user as a link to
 * reset the password or remind him of the password.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_forgot_password (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, 
                                            SIGNONUI_KEY_FORGOT_PASSWORD);
}

/**
 * gsignond_signonui_data_set_forgot_password:
 * @data: a #GSignondDictionary structure
 * @forgot: the forgot password string
 * 
 * A setter for the forgot password string, which is shown to the user as a link to
 * reset the password or remind him of the password.
 * 
 * Returns: (transfer none): 
 */
void
gsignond_signonui_data_set_forgot_password (GSignondSignonuiData *data,
                                            const gchar* forgot)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_FORGOT_PASSWORD, forgot);
}

/**
 * gsignond_signonui_data_get_forgot_password_url:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the forgot password URL, where the user can reset or request a 
 * reminder of the password.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_forgot_password_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_FORGOT_PASSWORD_URL);
}

/**
 * gsignond_signonui_data_set_forgot_password_url:
 * @data: a #GSignondDictionary structure
 * @url: the forgot password URL
 * 
 * A setter for the forgot password URL, where the user can reset or request a 
 * reminder of the password.
 * 
 * Returns: (transfer none)
 */
void
gsignond_signonui_data_set_forgot_password_url (GSignondSignonuiData *data,
                                                const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_FORGOT_PASSWORD_URL, url);
}

/**
 * gsignond_signonui_data_get_message:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the message which is show to the user in the signon UI dialog.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_message (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_MESSAGE);
}

/**
 * gsignond_signonui_data_set_message:
 * @data: a #GSignondDictionary structure
 * @message: the message
 * 
 * A setter for the message which is show to the user in the signon UI dialog.
 * 
 */
void
gsignond_signonui_data_set_message (GSignondSignonuiData *data,
                                    const gchar *message)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_MESSAGE, message);
}

/**
 * gsignond_signonui_data_get_open_url:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the URL that should be opened by signon UI.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_open_url (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_OPEN_URL);
}

/**
 * gsignond_signonui_data_set_open_url:
 * @data: a #GSignondDictionary structure
 * @url: the url to open
 * 
 * A setter for the URL that should be opened by signon UI.
 * 
 */
void
gsignond_signonui_data_set_open_url (GSignondSignonuiData *data,
                                     const gchar *url)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_OPEN_URL, url);
}

/**
 * gsignond_signonui_data_get_password:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the password string.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_password (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_PASSWORD);
}

/**
 * gsignond_signonui_data_set_password:
 * @data: a #GSignondDictionary structure
 * @password: the password string
 * 
 * A setter for the password string.
 * 
 */
void
gsignond_signonui_data_set_password (GSignondSignonuiData *data,
                                     const gchar *password)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_PASSWORD, password);
}

/**
 * gsignond_signonui_data_get_query_error:
 * @data: a #GSignondDictionary structure
 * @error: the error is written here
 * 
 * A getter for the UI interaction error. Signon UI sets this to @SIGNONUI_ERROR_NONE if
 * there were no errors.
 * 
 * Returns: whether this property exists in the @data dictionary or not.
 */
gboolean
gsignond_signonui_data_get_query_error (GSignondSignonuiData *data,
                                        GSignondSignonuiError *error)
{
    return gsignond_dictionary_get_uint32 (data, 
                                          SIGNONUI_KEY_QUERY_ERROR_CODE, 
                                          error);
}

/**
 * gsignond_signonui_data_set_query_error:
 * @data: a #GSignondDictionary structure
 * @error: the error
 * 
 * A setter for the UI interaction error. Signon UI sets this to @SIGNONUI_ERROR_NONE if
 * there were no errors.
 * 
 */
void
gsignond_signonui_data_set_query_error (GSignondSignonuiData *data,
                                        GSignondSignonuiError error)
{
    gsignond_dictionary_set_uint32 (data, SIGNONUI_KEY_QUERY_ERROR_CODE, error);
}

/**
 * gsignond_signonui_data_get_query_password:
 * @data: a #GSignondDictionary structure
 * @query_password: the property is written here
 * 
 * A getter for the query password property. It indicates whether the signon UI
 * should ask the user for a password (and return it in the password property).
 * 
 * Returns: whether this property exists in the @data dictionary or not.
 */
gboolean
gsignond_signonui_data_get_query_password (GSignondSignonuiData *data,
                                            gboolean *query_password)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_QUERY_PASSWORD,
                                            query_password);
}

/**
 * gsignond_signonui_data_set_query_password:
 * @data: a #GSignondDictionary structure
 * @query: the property value
 * 
 * A setter for the query password property. It indicates whether the signon UI
 * should ask the user for a password (and return it in the password property).
 * 
 */
void
gsignond_signonui_data_set_query_password (GSignondSignonuiData *data,
                                           gboolean query)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_QUERY_PASSWORD, query);
}

/**
 * gsignond_signonui_data_get_query_username:
 * @data: a #GSignondDictionary structure
 * @query_username: the property is written here
 * 
 * A getter for the query username property. It indicates whether the signon UI
 * should ask the user for a username (and return it in the username property).
 * 
 * Returns: whether this property exists in the @data dictionary or not.
 */
gboolean
gsignond_signonui_data_get_query_username (GSignondSignonuiData *data,
                                            gboolean *query_username)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_QUERY_USERNAME,
                                            query_username);
}

/**
 * gsignond_signonui_data_set_query_username:
 * @data: a #GSignondDictionary structure
 * @query: the property value
 * 
 * A setter for the query username property. It indicates whether the signon UI
 * should ask the user for a username (and return it in the username property).
 * 
 */
void
gsignond_signonui_data_set_query_username (GSignondSignonuiData *data,
                                           gboolean query)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_QUERY_USERNAME, query);
}

/**
 * gsignond_signonui_data_get_remember_password:
 * @data: a #GSignondDictionary structure
 * @remember_password: the property is written here
 * 
 * A getter for whether the password should be remembered.
 * 
 * Returns: whether this property exists in the @data dictionary or not.
 */
gboolean
gsignond_signonui_data_get_remember_password (GSignondSignonuiData *data,
                                            gboolean *remember_password)
{
    return gsignond_dictionary_get_boolean (data, 
                                            SIGNONUI_KEY_REMEMBER_PASSWORD,
                                            remember_password);
}

/**
 * gsignond_signonui_data_set_remember_password:
 * @data: a #GSignondDictionary structure
 * @remember: the property value
 * 
 * A setter for whether the password should be remembered.
 * 
 */
void
gsignond_signonui_data_set_remember_password (GSignondSignonuiData *data,
                                              gboolean remember)
{
    gsignond_dictionary_set_boolean (data, SIGNONUI_KEY_REMEMBER_PASSWORD, remember);
}

/**
 * gsignond_signonui_data_get_request_id:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the dialog request id. The id identifies the dialog so that it
 * can be refreshed or updated.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_request_id (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_REQUEST_ID);
}

/**
 * gsignond_signonui_data_set_request_id:
 * @data: a #GSignondDictionary structure
 * @id: request id
 * 
 * A setter for the dialog request id. The id identifies the dialog so that it
 * can be refreshed or updated.
 * 
 */
void
gsignond_signonui_data_set_request_id (GSignondSignonuiData *data,
                                       const gchar *id)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_REQUEST_ID, id);
}

/**
 * gsignond_signonui_data_get_test_reply:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the test reply values. It's used only by the signon ui 
 * implementations to test themselves.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_test_reply (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_TEST_REPLY_VALUES);
}

/**
 * gsignond_signonui_data_set_test_reply:
 * @data: a #GSignondDictionary structure
 * @reply: test reply values
 * 
 * A setter for the test reply values. It's used only by the signon ui 
 * implementations to test themselves.
 * 
 */
void
gsignond_signonui_data_set_test_reply (GSignondSignonuiData *data,
                                       const gchar *reply)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_TEST_REPLY_VALUES, reply);
}

/**
 * gsignond_signonui_data_get_title:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the UI dialog title.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_title (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_TITLE);
}

/**
 * gsignond_signonui_data_set_title:
 * @data: a #GSignondDictionary structure
 * @title: the title
 * 
 * A setter for the UI dialog title.
 * 
 */
void
gsignond_signonui_data_set_title (GSignondSignonuiData *data,
                                  const gchar* title)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_TITLE, title);
}

/**
 * gsignond_signonui_data_get_url_response:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the response URL. If the final URL was set in the request to the signon UI, and the signon UI
 * detects that it has been reached, then the full final URL is returned using
 * this property. This is used by redirection-based authentication such as OAauth.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_url_response (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_URL_RESPONSE);
}

/**
 * gsignond_signonui_data_set_url_response:
 * @data: a #GSignondDictionary structure
 * @response: the response URL
 * 
 * A getter for the response URL. If the final URL was set in the request to the 
 * signon UI, and the signon UI
 * detects that it has been reached, then the full final URL is returned using
 * this property. This is used by redirection-based authentication such as OAauth.
 * 
 * Returns: (transfer none)
 */
void
gsignond_signonui_data_set_url_response (GSignondSignonuiData *data,
                                         const gchar *response)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_URL_RESPONSE, response);
}

/**
 * gsignond_signonui_data_get_username:
 * @data: a #GSignondDictionary structure
 * 
 * A getter for the username string.
 * 
 * Returns: (transfer none)
 */
const gchar*
gsignond_signonui_data_get_username (GSignondSignonuiData *data)
{
    return gsignond_dictionary_get_string (data, SIGNONUI_KEY_USERNAME);
}

/**
 * gsignond_signonui_data_set_username:
 * @data: a #GSignondDictionary structure
 * @username: the username string
 * 
 * A setter for the username string.
 * 
 */
void
gsignond_signonui_data_set_username (GSignondSignonuiData *data,
                                     const gchar *username)
{
    gsignond_dictionary_set_string (data, SIGNONUI_KEY_USERNAME, username);
}

