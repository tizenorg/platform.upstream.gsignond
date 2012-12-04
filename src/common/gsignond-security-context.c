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

#include "gsignond/gsignond-security-context.h"

static void
_security_context_free (gpointer ptr)
{
    GSignonDSecurityContext *ctx = (GSignonDSecurityContext *) ptr;

    gsignond_security_context_free (ctx);
}

/**
 * gsignond_security_context_new:
 *
 * Allocates a new security context item.
 *
 * Returns: (transfer full) allocated #GSignonDSecurityContext.
 */
GSignonDSecurityContext *
gsignond_security_context_new ()
{
    GSignonDSecurityContext *ctx;

    ctx = g_new0 (GSignonDSecurityContext, 1);
    ctx->sys_ctx = g_strdup ("");
    ctx->app_ctx = g_strdup ("*");

    return ctx;
}

/**
 * gsignond_security_context_new_from_vaues:
 * @system_context: system security context (such as SMACK/MSSF label/token).
 * @application_context: application security context (such as a script name).
 *
 * Allocates and initializes a new security context item.
 *
 * Returns: (transfer full) allocated #GSignonDSecurityContext.
 */
GSignonDSecurityContext *
gsignond_security_context_new_from_values (const gchar *system_context,
                                           const gchar *application_context)
{
    GSignonDSecurityContext *ctx;

    g_return_val_if_fail (system_context != NULL, NULL);

    ctx = g_new0 (GSignonDSecurityContext, 1);
    ctx->sys_ctx = g_strdup (system_context);
    if (application_context)
        ctx->app_ctx = g_strdup (application_context);
    else
        ctx->app_ctx = g_strdup ("*");

    return ctx;
}

/**
 * gsignond_security_context_copy:
 * @src_ctx: source security context to copy.
 *
 * Copy a security context item.
 *
 * Returns: (transfer full) a copy of the #SignonSecurityContex item.
 */
GSignonDSecurityContext *
gsignond_security_context_copy (const GSignonDSecurityContext *src_ctx)
{
    g_return_val_if_fail (src_ctx != NULL, NULL);

    return gsignond_security_context_new_from_values (src_ctx->sys_ctx,
                                                    src_ctx->app_ctx);
}

/**
 * gsignond_security_context_free:
 * @ctx: #GSignonDSecurityContext to be freed.
 *
 * Frees a security context item.
 */
void
gsignond_security_context_free (GSignonDSecurityContext *ctx)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->sys_ctx);
    g_free (ctx->app_ctx);
    g_free (ctx);
}

/**
 * gsignond_security_context_set_system_context:
 * @ctx: #GSignonDSecurityContext item.
 * @system_context: system security context.
 *
 * Sets the system context part (such as SMACK label or MSSF token) of the
 * #GSignonDSecurityContext.
 */
void
gsignond_security_context_set_system_context (GSignonDSecurityContext *ctx,
                                              const gchar *system_context)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->sys_ctx);
    ctx->sys_ctx = g_strdup (system_context);
}

/**
 * gsignond_security_context_get_system_context:
 * @ctx: #GSignonDSecurityContext item.
 * 
 * Get the system context part (such as SMACK label or MSSF token) of the
 * #GSignonDSecurityContext.
 *
 * Returns: (transfer none) system context.
 */
const gchar *
gsignond_security_context_get_system_context (GSignonDSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->sys_ctx;
}

/**
 * gsignond_security_context_set_application_context:
 * @ctx: #GSignonDSecurityContext item.
 * @application_context: application security context.
 *
 * Sets the application context part (such as a script name or a web page) of
 * the #GSignonDSecurityContext.
 */
void
gsignond_security_context_set_application_context (GSignonDSecurityContext *ctx,
                                               const gchar *application_context)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->app_ctx);
    ctx->app_ctx = g_strdup (application_context);
}

/**
 * gsignond_security_context_get_application_context:
 * @ctx: #GSignonDSecurityContext item.
 *
 * Get the application context part (such as script name or a web page) of
 * the #GSignonDSecurityContext.
 *
 * Returns: (transfer none) application context.
 */
const gchar *
gsignond_security_context_get_application_context (GSignonDSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->app_ctx;
}

/**
 * gsignond_security_context_build_variant:
 * @list: #GSignonDSecurityContextList item.
 *
 * Builds a GVariant of type "a(ss)" from a GList of #GSignonDSecurityContext
 * items.
 *
 * Returns: (transfer full) GVariant construct of a #GSignonDSecurityContextList.
 */
GVariant *
gsignond_security_context_build_variant (
                                        const GSignonDSecurityContextList *list)
{
    GVariantBuilder *builder;
    GVariant *variant;
    GSignonDSecurityContext *ctx;

    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    for ( ; list != NULL; list = g_list_next (list)) {
        ctx = (GSignonDSecurityContext *) list->data;
        g_variant_builder_add (builder, "(ss)", ctx->sys_ctx, ctx->app_ctx);
    }
    variant = g_variant_builder_end(builder);
    return variant;
}

/**
 * gsignond_security_context_deconstruct_variant:
 * @variant: GVariant item with a list of security context tuples
 *
 * Builds a GList of #GSignonDSecurityContext items from a GVariant of type
 * "a(ss)".
 *
 * Returns: (transfer full) #GSignonDSecurityContextList item.
 */
GSignonDSecurityContextList *
gsignond_security_context_deconstruct_variant (GVariant *variant)
{
    GSignonDSecurityContextList *list = NULL;
    GVariantIter iter;
    gchar *sys_ctx;
    gchar *app_ctx;

    g_return_val_if_fail (variant != NULL, NULL);

    g_variant_iter_init (&iter, variant);
    while (g_variant_iter_next (&iter, "(ss)", &sys_ctx, &app_ctx)) {
        list = g_list_append (
            list, gsignond_security_context_new_from_values (sys_ctx, app_ctx));
    }

    return list;
}

/**
 * gsignond_security_context_list_copy:
 * @src_list: source #GSignonDSecurityContextList.
 *
 * Copies a GList of #GSignonDSecurityContext items.
 *
 * Returns: (transfer full) #GSignonDSecurityContextList item.
 */
GSignonDSecurityContextList *
gsignond_security_context_list_copy (
                                    const GSignonDSecurityContextList *src_list)
{
    GSignonDSecurityContext *ctx;
    GSignonDSecurityContextList *dst_list = NULL;

    for ( ; src_list != NULL; src_list = g_list_next (src_list)) {
        ctx = (GSignonDSecurityContext *) src_list->data;
        dst_list = g_list_append (
            dst_list, gsignond_security_context_copy (ctx));
    }

    return dst_list;
}

/**
 * gsignond_security_context_list_free:
 * @seclist: (transfer full) #GSignonDSecurityContextList item.
 *
 * Frees all items and the GList of #GSignonDSecurityContext.
 */
void
gsignond_security_context_list_free (GSignonDSecurityContextList *seclist)
{
    g_list_free_full (seclist, _security_context_free);
}

