/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef _GSIGNOND_SECURITY_CONTEXT_H_
#define _GSIGNOND_SECURITY_CONTEXT_H_

#include <glib.h>

G_BEGIN_DECLS

/**
 * Security context descriptor.
 *
 * Practically a string tuple.
 *
 * @sys_ctx: system context, such as SMACK-label, MSSF token or just a
 *           binary path.
 * @app_ctx: application context, such as a script or a web page.
 */
typedef struct _GSignonDSecurityContext
{
    gchar *sys_ctx;
    gchar *app_ctx;
} GSignonDSecurityContext;

/**
 * GList of #GSignonDSecurityContext items.
 */
typedef GList GSignonDSecurityContextList;

GSignonDSecurityContext * gsignond_security_context_new ();
GSignonDSecurityContext * gsignond_security_context_new_from_values (
                                            const gchar *system_context,
                                            const gchar *application_context);
void gsignond_security_context_free (GSignonDSecurityContext *ctx);
GSignonDSecurityContext * gsignond_security_context_copy (
                                        const GSignonDSecurityContext *src_ctx);
void gsignond_security_context_set_system_context (GSignonDSecurityContext *ctx,
                                                 const gchar *system_context);
const gchar * gsignond_security_context_get_system_context (
                                                GSignonDSecurityContext *ctx);
void gsignond_security_context_set_application_context (
                                            GSignonDSecurityContext *ctx,
                                            const gchar *application_context);
const gchar * gsignond_security_context_get_application_context (
                                                GSignonDSecurityContext *ctx);

GVariant * gsignond_security_context_build_variant (
                                    const GSignonDSecurityContextList *list);
GSignonDSecurityContextList * gsignond_security_context_deconstruct_variant (
                                                            GVariant *variant);

GSignonDSecurityContextList * gsignond_security_context_list_copy (
                                const GSignonDSecurityContextList *src_list);
void gsignond_security_context_list_free (GSignonDSecurityContextList *seclist);

G_END_DECLS

#endif  /* _GSIGNOND_SECURITY_CONTEXT_H_ */

