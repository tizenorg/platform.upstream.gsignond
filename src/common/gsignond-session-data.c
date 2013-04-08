/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2013 Intel Corporation.
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

#include <gsignond/gsignond-session-data.h>

const gchar *
gsignond_session_data_get_username (GSignondSessionData *data)
{
    return gsignond_dictionary_get_string (data, "UserName");
}

void
gsignond_session_data_set_username (GSignondSessionData *data, 
                                    const gchar *username)
{
    gsignond_dictionary_set_string (data, "UserName", username);
}

const gchar *
gsignond_session_data_get_secret (GSignondSessionData *data)
{
    return gsignond_dictionary_get_string (data, "Secret");
}

void
gsignond_session_data_set_secret (GSignondSessionData *data, 
                                  const gchar *secret)
{
    gsignond_dictionary_set_string (data, "Secret", secret);
}

const gchar *
gsignond_session_data_get_realm (GSignondSessionData *data)
{
    return gsignond_dictionary_get_string (data, "Realm");
}

void
gsignond_session_data_set_realm (GSignondSessionData *data,
                                 const gchar *realm)
{
    gsignond_dictionary_set_string (data, "Realm", realm);
}

const gchar *
gsignond_session_data_get_caption (GSignondSessionData *data)
{
    return gsignond_dictionary_get_string (data, "Caption");
}

void
gsignond_session_data_set_caption (GSignondSessionData *data,
                                   const gchar *caption)
{
    gsignond_dictionary_set_string (data, "Caption", caption);
}

gboolean
gsignond_session_data_get_renew_token (GSignondSessionData *data,
                                       gboolean *renew_token)
{
    return gsignond_dictionary_get_boolean (data, "RenewToken", renew_token);
}

void
gsignond_session_data_set_renew_token (GSignondSessionData *data,
                                       gboolean renew_token)
{
    gsignond_dictionary_set_boolean (data, "RenewToken", renew_token);
}

gboolean
gsignond_session_data_get_ui_policy (GSignondSessionData *data,
                                     guint32 *ui_policy)
{
    return gsignond_dictionary_get_uint32 (data, "UiPolicy", ui_policy);
}

void
gsignond_session_data_set_ui_policy (GSignondSessionData *data, 
                                     guint32 ui_policy)
{
    gsignond_dictionary_set_uint32 (data, "UiPolicy", ui_policy);
}    

const gchar *
gsignond_session_data_get_network_proxy (GSignondSessionData *data)
{
    return gsignond_dictionary_get_string (data, "NetworkProxy");
}

void
gsignond_session_data_set_network_proxy (GSignondSessionData *data,
                                         const gchar *network_proxy)
{
    gsignond_dictionary_set_string (data, "NetworkProxy", network_proxy);
}

gboolean
gsignond_session_data_get_network_timeout (GSignondSessionData *data,
                                           guint32 *network_timeout)
{
    return gsignond_dictionary_get_uint32 (data, "NetworkTimeout",
                                           network_timeout);
}

void
gsignond_session_data_set_network_timeout (GSignondSessionData *data,
                                           guint32 network_timeout)
{
    gsignond_dictionary_set_uint32 (data, "NetworkTimeout",
                                    network_timeout);
}

gboolean
gsignond_session_data_get_window_id (GSignondSessionData *data,
                                     guint32 *window_id)
{
    return gsignond_dictionary_get_uint32 (data, "WindowId", window_id);
}

void
gsignond_session_data_set_window_id (GSignondSessionData *data,
                                     guint32 window_id)
{
    gsignond_dictionary_set_uint32 (data, "WindowId", window_id);
}

