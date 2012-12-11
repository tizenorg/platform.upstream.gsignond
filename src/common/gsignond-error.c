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
#include <string.h>

#include "gsignond-error.h"

GQuark
gsignond_error_quark (void)
{
    static GQuark quark = 0;
    if (quark == 0) {
        quark = g_quark_from_static_string (G_LOG_DOMAIN);
    }
    return quark;
}

GString*
gsignond_concat_domain_and_error (const gchar *str1,
                                  const gchar *str2)
{
    GString *str;
    g_return_val_if_fail (str1 != NULL && str2 != NULL, NULL);
    str = g_string_sized_new (strlen(str1)+strlen(str2)-1);
    g_string_printf (str,"[%s].%s\n",str1,str2);
    return str;
}

GString*
gsignond_prepend_domain_to_error_msg (const GError *err)
{
    GString *msg;
    const gchar *domain;
    g_return_val_if_fail (err != NULL, NULL);
    if (err->message != NULL) {
        domain = g_quark_to_string(err->domain);
        msg = gsignond_concat_domain_and_error(domain, err->message);
    }
    return msg;
}

