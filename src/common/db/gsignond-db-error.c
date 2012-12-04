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

#include "gsignond-db-error.h"

#define GSIGNOND_DB_ERROR_DOMAIN_STR   "gsignond_db"

GQuark
gsignond_db_error_quark (void)
{
    static gsize quark = 0;

    if (g_once_init_enter (&quark)) {
        GQuark domain =
                g_quark_from_static_string (GSIGNOND_DB_ERROR_DOMAIN_STR);
        g_assert (sizeof (GQuark) <= sizeof (gsize));

        g_once_init_leave (&quark, domain);
    }

    return (GQuark) quark;
}

GError *
gsignond_db_create_error (
        GSignondDbError code,
        const gchar* msg)
{
    GError *error = NULL;

    error = g_error_new (GSIGNOND_DB_ERROR,
                         code,
                         msg);
    return error;
}

