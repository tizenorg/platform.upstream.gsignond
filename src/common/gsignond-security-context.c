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

#include "gsignond/signon-security-context.h"

static void
_security_context_free (gpointer ptr)
{
    SignonSecurityContext *ctx = (SignonSecurityContext *) ptr;

    signon_security_context_free (ctx);
}

/**
 * signon_security_context_new:
 *
 * Allocates a new security context item.
 *
 * Returns: (transfer full) allocated #SignonSecurityContext.
 */
SignonSecurityContext *
signon_security_context_new ()
{
    SignonSecurityContext *ctx;

    ctx = g_new0 (SignonSecurityContext, 1);
    ctx->sys_ctx = g_strdup ("");
    ctx->app_ctx = g_strdup ("*");

    return ctx;
}

/**
 * signon_security_context_new_from_vaues:
 * @system_context: system security context (such as SMACK/MSSF label/token).
 * @application_context: application security context (such as a script name).
 *
 * Allocates and initializes a new security context item.
 *
 * Returns: (transfer full) allocated #SignonSecurityContext.
 */
SignonSecurityContext *
signon_security_context_new_from_values (const gchar *system_context,
                                         const gchar *application_context)
{
    SignonSecurityContext *ctx;

    g_return_val_if_fail (system_context != NULL, NULL);

    ctx = g_new0 (SignonSecurityContext, 1);
    ctx->sys_ctx = g_strdup (system_context);
    if (application_context)
        ctx->app_ctx = g_strdup (application_context);
    else
        ctx->app_ctx = g_strdup ("*");

    return ctx;
}

/**
 * signon_security_context_copy:
 * @src_ctx: source security context to copy.
 *
 * Copy a security context item.
 *
 * Returns: (transfer full) a copy of the #SignonSecurityContex item.
 */
SignonSecurityContext *
signon_security_context_copy (const SignonSecurityContext *src_ctx)
{
    g_return_val_if_fail (src_ctx != NULL, NULL);

    return signon_security_context_new_from_values (src_ctx->sys_ctx,
                                                    src_ctx->app_ctx);
}

/**
 * signon_security_context_free:
 * @ctx: #SignonSecurityContext to be freed.
 *
 * Frees a security context item.
 */
void
signon_security_context_free (SignonSecurityContext *ctx)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->sys_ctx);
    g_free (ctx->app_ctx);
    g_free (ctx);
}

/**
 * signon_security_context_set_system_context:
 * @ctx: #SignonSecurityContext item.
 * @system_context: system security context.
 *
 * Sets the system context part (such as SMACK label or MSSF token) of the
 * #SignonSecurityContext.
 */
void
signon_security_context_set_system_context (SignonSecurityContext *ctx,
                                            const gchar *system_context)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->sys_ctx);
    ctx->sys_ctx = g_strdup (system_context);
}

/**
 * signon_security_context_get_system_context:
 * @ctx: #SignonSecurityContext item.
 * 
 * Get the system context part (such as SMACK label or MSSF token) of the
 * #SignonSecurityContext.
 *
 * Returns: (transfer none) system context.
 */
const gchar *
signon_security_context_get_system_context (SignonSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->sys_ctx;
}

/**
 * signon_security_context_set_application_context:
 * @ctx: #SignonSecurityContext item.
 * @application_context: application security context.
 *
 * Sets the application context part (such as a script name or a web page) of
 * the #SignonSecurityContext.
 */
void
signon_security_context_set_application_context (SignonSecurityContext *ctx,
                                              const gchar *application_context)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->app_ctx);
    ctx->app_ctx = g_strdup (application_context);
}

/**
 * signon_security_context_get_application_context:
 * @ctx: #SignonSecurityContext item.
 *
 * Get the application context part (such as script name or a web page) of
 * the #SignonSecurityContext.
 *
 * Returns: (transfer none) application context.
 */
const gchar *
signon_security_context_get_application_context (SignonSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->app_ctx;
}

/**
 * signon_security_context_build_variant:
 * @list: #SignonSecurityContextList item.
 *
 * Builds a GVariant of type "a(ss)" from a GList of #SignonSecurityContext
 * items.
 *
 * Returns: (transfer full) GVariant construct of a #SignonSecurityContextList.
 */
GVariant *
signon_security_context_build_variant (const SignonSecurityContextList *list)
{
    GVariantBuilder *builder;
    GVariant *variant;
    SignonSecurityContext *ctx;

    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    for ( ; list != NULL; list = g_list_next (list))
    {
        ctx = (SignonSecurityContext *) list->data;
        g_variant_builder_add (builder, "(ss)", ctx->sys_ctx, ctx->app_ctx);
    }
    variant = g_variant_builder_end(builder);
    return variant;
}

/**
 * signon_security_context_deconstruct_variant:
 * @variant: GVariant item with a list of security context tuples
 *
 * Builds a GList of #SignonSecurityContext items from a GVariant of type
 * "a(ss)".
 *
 * Returns: (transfer full) #SignonSecurityContextList item.
 */
SignonSecurityContextList *
signon_security_context_deconstruct_variant (GVariant *variant)
{
    SignonSecurityContextList *list = NULL;
    GVariantIter iter;
    gchar *sys_ctx;
    gchar *app_ctx;

    g_return_val_if_fail (variant != NULL, NULL);

    g_variant_iter_init (&iter, variant);
    while (g_variant_iter_next (&iter, "(ss)", &sys_ctx, &app_ctx))
    {
        list = g_list_append (
            list, signon_security_context_new_from_values (sys_ctx, app_ctx));
    }

    return list;
}

/**
 * signon_security_context_list_copy:
 * @src_list: source #SignonSecurityContextList.
 *
 * Copies a GList of #SignonSecurityContext items.
 *
 * Returns: (transfer full) #SignonSecurityContextList item.
 */
SignonSecurityContextList *
signon_security_context_list_copy (const SignonSecurityContextList *src_list)
{
    SignonSecurityContext *ctx;
    SignonSecurityContextList *dst_list = NULL;

    for ( ; src_list != NULL; src_list = g_list_next (src_list))
    {
        ctx = (SignonSecurityContext *) src_list->data;
        dst_list = g_list_append (
            dst_list, signon_security_context_copy (ctx));
    }

    return dst_list;
}

/**
 * signon_security_context_list_free:
 * @seclist: (transfer full) #SignonSecurityContextList item.
 *
 * Frees all items and the GList of #SignonSecurityContext.
 */
void
signon_security_context_list_free (SignonSecurityContextList *seclist)
{
    g_list_free_full (seclist, _security_context_free);
}

