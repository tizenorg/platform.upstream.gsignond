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

#ifndef __GSIGNOND_SESSION_DATA_H__
#define __GSIGNOND_SESSION_DATA_H__

#include <gsignond/gsignond-dictionary.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_SESSION_DATA (GSIGNOND_TYPE_DICTIONARY)

#define GSIGNOND_SESSION_DATA(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                 GSIGNOND_TYPE_SESSION_DATA, \
                                 GSignondSessionData))
#define GSIGNOND_IS_SESSION_DATA(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                 GSIGNOND_TYPE_SESSION_DATA))

typedef GSignondDictionary GSignondSessionData;

typedef enum {
    GSIGNOND_UI_POLICY_DEFAULT = 0,
    GSIGNOND_UI_POLICY_REQUEST_PASSWORD,
    GSIGNOND_UI_POLICY_NO_USER_INTERACTION,
    GSIGNOND_UI_POLICY_VALIDATION
} GSignondUiPolicy;


const gchar *
gsignond_session_data_get_username (GSignondSessionData *data);

void
gsignond_session_data_set_username (GSignondSessionData *data, 
                                    const gchar *username);

const gchar *
gsignond_session_data_get_secret (GSignondSessionData *data);

void
gsignond_session_data_set_secret (GSignondSessionData *data, 
                                  const gchar *secret);

const gchar *
gsignond_session_data_get_realm (GSignondSessionData *data);

void
gsignond_session_data_set_realm (GSignondSessionData *data,
                                 const gchar *realm);

const gchar *
gsignond_session_data_get_caption (GSignondSessionData *data);

void
gsignond_session_data_set_caption (GSignondSessionData *data,
                                   const gchar *caption);

gboolean
gsignond_session_data_get_renew_token (GSignondSessionData *data,
                                       gboolean *renew_token);

void
gsignond_session_data_set_renew_token (GSignondSessionData *data,
                                       gboolean renew_token);

gboolean
gsignond_session_data_get_ui_policy (GSignondSessionData *data,
                                     GSignondUiPolicy *ui_policy);

void
gsignond_session_data_set_ui_policy (GSignondSessionData *data,
                                     GSignondUiPolicy ui_policy);

const gchar *
gsignond_session_data_get_network_proxy (GSignondSessionData *data);

void
gsignond_session_data_set_network_proxy (GSignondSessionData *data,
                                         const gchar *network_proxy);

gboolean
gsignond_session_data_get_network_timeout (GSignondSessionData *data,
                                           guint32 *network_timeout);

void
gsignond_session_data_set_network_timeout (GSignondSessionData *data,
                                           guint32 network_timeout);

gboolean
gsignond_session_data_get_window_id (GSignondSessionData *data,
                                     guint32 *window_id);

void
gsignond_session_data_set_window_id (GSignondSessionData *data,
                                     guint32 window_id);


G_END_DECLS

#endif /* __GSIGNOND_SESSION_DATA_H__ */
