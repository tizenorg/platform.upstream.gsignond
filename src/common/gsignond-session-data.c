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

#include <gsignond/gsignond-session-data.h>

const gchar*
gsignond_session_data_get_username(GSignondSessionData* data)
{
    GVariant* variant = gsignond_dictionary_get(data, "username");
    if (variant == NULL)
        return NULL;
    return g_variant_get_string(variant, NULL);
}

void
gsignond_session_data_set_username(GSignondSessionData* data, 
                                   const gchar* username)
{
    gsignond_dictionary_set(data, "username", g_variant_new_string(username));
}

const gchar*
gsignond_session_data_get_secret(GSignondSessionData* data)
{
    GVariant* variant = gsignond_dictionary_get(data, "secret");
    if (variant == NULL)
        return NULL;
    return g_variant_get_string(variant, NULL);
    
}

void
gsignond_session_data_set_secret(GSignondSessionData* data, 
                                 const gchar* secret)
{
    gsignond_dictionary_set(data, "secret", g_variant_new_string(secret));
}

gboolean
gsignond_session_data_get_query_username(GSignondSessionData* data)
{
    GVariant* variant = gsignond_dictionary_get(data, "query_username");
    if (variant == NULL)
        return FALSE;
    return g_variant_get_boolean(variant);
}

void
gsignond_session_data_set_query_username(GSignondSessionData* data, 
                                         gboolean query_username)
{
    gsignond_dictionary_set(data, "query_username", 
                            g_variant_new_boolean(query_username));
}    

gboolean
gsignond_session_data_get_query_password(GSignondSessionData* data)
{
    GVariant* variant = gsignond_dictionary_get(data, "query_password");
    if (variant == NULL)
        return FALSE;
    return g_variant_get_boolean(variant);
}    

void
gsignond_session_data_set_query_password(GSignondSessionData* data, 
                                         gboolean query_password)
{
    gsignond_dictionary_set(data, "query_password", 
                            g_variant_new_boolean(query_password));
}    

GSignondQueryError gsignond_session_data_get_query_error(
    GSignondSessionData* data)
{
    GVariant* variant = gsignond_dictionary_get(data, "query_error");
    if (variant == NULL)
        return GSIGNOND_QUERY_ERROR_NONE;
    return g_variant_get_int32(variant);
}    

void
gsignond_session_data_set_query_error(GSignondSessionData* data, 
                                         GSignondQueryError error)
{
    gsignond_dictionary_set(data, "query_error", 
                            g_variant_new_int32(error));
}
