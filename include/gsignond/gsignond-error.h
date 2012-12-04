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

/* inclusion guard */
#ifndef __GSIGNOND_ERROR_H__
#define __GSIGNOND_ERROR_H__

#include <glib.h>

G_BEGIN_DECLS

#define G_LOG_DOMAIN "gsignond"

/**
 * GSIGNOND_ERROR:
 *
 */
#define GSIGNOND_ERROR   (gsignond_error_quark())

typedef enum {
    GSIGNOND_ERROR_NONE,
    /* Add error codes */
} GSignondError;

#define gsignond_gerr(error, handler) \
    G_STMT_START {                 \
        GString* msg = gsignond_prepend_domain_to_error_msg(error); \
        handler(msg->str); \
        g_string_free(msg, TRUE); \
    } G_STMT_END\

#define gsignond_error_gerr(err)       gsignond_gerr(err, g_error)

#define gsignond_critical_gerr(err)    gsignond_gerr(err, g_critical)

#define gsignond_warning_gerr(err)     gsignond_gerr(err, g_warning)

#define gsignond_message_gerr(err)     gsignond_gerr(err, g_message)

#define gsignond_debug_gerr(err)       gsignond_gerr(err, g_debug)

GQuark
gsignond_error_quark (void);

GString*
gsignond_concat_domain_and_error (
        const gchar *str1,
        const gchar *str2);

GString*
gsignond_prepend_domain_to_error_msg (const GError *err);

G_END_DECLS

#endif /* __GSIGNOND_ERROR_H__ */
