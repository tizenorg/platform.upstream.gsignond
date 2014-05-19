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


/**
 * SECTION:gsignond-security-context
 * @title: GSignondSecurityContext
 * @short_description: security context descriptor used in access control checks
 * @include: gsignond/gsignond-security-context.h
 *
 * Security context is a string tuple of system context and application context.
 * 
 * System context can be a binary path, SMACK-label, or MSSF token.
 * 
 * Application context identifies a script or a webpage within an application,
 * and it's used for providing access control to runtime environments (when making an access
 * control decision requires not only a binary identifier, but also information
 * about what the binary is doing).
 *
 * When an application is trying to access the gSSO service, the system context
 * is determined by a specific #GSignondAccessControlManager instance using
 * system services of a specific platform. Application context is set by the
 * application itself. Then both contexts are used by #GSignondAccessControlManager
 * to perform an access control check.
 */

/**
 * GSignondSecurityContext:
 * @sys_ctx: system context
 * @app_ctx: application context
 * 
 * Security context descriptor used for access control checks. System context
 * and application context can contain a wildcard match "*" which has special
 * meaning in gsignond_security_context_match() and
 * gsignond_security_context_check().
 */

/**
 * GSignondSecurityContextList:
 * 
 * GList of #GSignondSecurityContext items.
 */
static void
_security_context_free (gpointer ptr)
{
    GSignondSecurityContext *ctx = (GSignondSecurityContext *) ptr;

    gsignond_security_context_free (ctx);
}

/**
 * gsignond_security_context_new:
 *
 * Allocates a new security context item. System and app context are empty strings.
 *
 * Returns: (transfer full): allocated #GSignondSecurityContext.
 */
GSignondSecurityContext *
gsignond_security_context_new ()
{
    GSignondSecurityContext *ctx;

    ctx = g_slice_new0 (GSignondSecurityContext);
    ctx->sys_ctx = g_strdup ("");
    ctx->app_ctx = g_strdup ("");

    return ctx;
}

/**
 * gsignond_security_context_new_from_values:
 * @system_context: system security context
 * @application_context: application security context
 *
 * Allocates and initializes a new security context item.
 *
 * Returns: (transfer full): allocated #GSignondSecurityContext.
 */
GSignondSecurityContext *
gsignond_security_context_new_from_values (const gchar *system_context,
                                           const gchar *application_context)
{
    GSignondSecurityContext *ctx;

    g_return_val_if_fail (system_context != NULL, NULL);

    ctx = g_slice_new0 (GSignondSecurityContext);
    ctx->sys_ctx = g_strdup (system_context);
    if (application_context)
        ctx->app_ctx = g_strdup (application_context);
    else
        ctx->app_ctx = g_strdup ("");

    return ctx;
}

/**
 * gsignond_security_context_copy:
 * @src_ctx: source security context to copy.
 *
 * Copies a security context item.
 *
 * Returns: (transfer full): a copy of the #GSignondSecurityContext item.
 */
GSignondSecurityContext *
gsignond_security_context_copy (const GSignondSecurityContext *src_ctx)
{
    g_return_val_if_fail (src_ctx != NULL, NULL);

    return gsignond_security_context_new_from_values (src_ctx->sys_ctx,
                                                      src_ctx->app_ctx);
}

/**
 * gsignond_security_context_free:
 * @ctx: #GSignondSecurityContext to be freed.
 *
 * Frees a security context item.
 */
void
gsignond_security_context_free (GSignondSecurityContext *ctx)
{
    if (ctx == NULL) return;

    g_free (ctx->sys_ctx);
    g_free (ctx->app_ctx);
    g_slice_free (GSignondSecurityContext, ctx);
}

/**
 * gsignond_security_context_set_system_context:
 * @ctx: #GSignondSecurityContext item.
 * @system_context: system security context.
 *
 * Sets the system context part of the
 * #GSignondSecurityContext.
 */
void
gsignond_security_context_set_system_context (GSignondSecurityContext *ctx,
                                              const gchar *system_context)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->sys_ctx);
    ctx->sys_ctx = (system_context) ?
        g_strdup (system_context) : g_strdup ("");
}

/**
 * gsignond_security_context_get_system_context:
 * @ctx: #GSignondSecurityContext item.
 * 
 * Get the system context partof the
 * #GSignondSecurityContext.
 *
 * Returns: (transfer none): system context.
 */
const gchar *
gsignond_security_context_get_system_context (
                                             const GSignondSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->sys_ctx;
}

/**
 * gsignond_security_context_set_application_context:
 * @ctx: #GSignondSecurityContext item.
 * @application_context: application security context.
 *
 * Sets the application context part of
 * the #GSignondSecurityContext.
 */
void
gsignond_security_context_set_application_context (
                                               GSignondSecurityContext *ctx,
                                               const gchar *application_context)
{
    g_return_if_fail (ctx != NULL);

    g_free (ctx->app_ctx);
    ctx->app_ctx = (application_context) ?
        g_strdup (application_context) : g_strdup ("");
}

/**
 * gsignond_security_context_get_application_context:
 * @ctx: #GSignondSecurityContext item.
 *
 * Get the application context part of
 * the #GSignondSecurityContext.
 *
 * Returns: (transfer none): application context.
 */
const gchar *
gsignond_security_context_get_application_context (
                                             const GSignondSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->app_ctx;
}

/**
 * gsignond_security_context_to_variant:
 * @ctx: #GSignondSecurityContext item.
 *
 * Build a GVariant of type "(ss)" from a #GSignondSecurityContext item.
 *
 * Returns: (transfer full): GVariant construct of a #GSignondSecurityContext.
 */
GVariant *
gsignond_security_context_to_variant (const GSignondSecurityContext *ctx)
{
    GVariant *variant;

    g_return_val_if_fail (ctx != NULL, NULL);

    variant = g_variant_new ("(ss)",
                             ctx->sys_ctx ? ctx->sys_ctx : "",
                             ctx->app_ctx ? ctx->app_ctx : "");

    return variant;
}

/**
 * gsignond_security_context_from_variant:
 * @variant: GVariant item with a #GSignondSecurityContext construct.
 *
 * Builds a #GSignondSecurityContext item from a GVariant of type "(ss)".
 *
 * Returns: (transfer full): #GSignondSecurityContext item.
 */
GSignondSecurityContext *
gsignond_security_context_from_variant (GVariant *variant)
{
    gchar *sys_ctx = NULL;
    gchar *app_ctx = NULL;
    GSignondSecurityContext *ctx;

    g_return_val_if_fail (variant != NULL, NULL);

    g_variant_get (variant, "(ss)", &sys_ctx, &app_ctx);
    ctx = gsignond_security_context_new_from_values (sys_ctx, app_ctx);
    g_free (sys_ctx);
    g_free (app_ctx);
    return ctx;
}

/**
 * gsignond_security_context_compare:
 * @ctx1: first item to compare.
 * @ctx2: second item to compare.
 *
 * Compare two #GSignondSecurityContext items in a similar way to strcmp().
 *
 * Returns: negative if ctx1 < ctx2, 0 if ctx1 == ctx2 and positive if ctx1 > ctx2.
 */
int
gsignond_security_context_compare (const GSignondSecurityContext *ctx1,
                                   const GSignondSecurityContext *ctx2)
{
    int res;

    if (ctx1 == ctx2) return 0;

    if (ctx1 == NULL)
        return -1;
    if (ctx2 == NULL)
        return 1;

    res = g_strcmp0(ctx1->sys_ctx, ctx2->sys_ctx);
    if (res == 0)
        res = g_strcmp0(ctx1->app_ctx, ctx2->app_ctx);

    return res;
}

/**
 * gsignond_security_context_match:
 * @ctx1: first item to compare.
 * @ctx2: second item to compare.
 *
 * Compare two #GSignondSecurityContext items match.
 *
 * Returns: TRUE if contexts are equal or if either side has a wildcard match for 
 * system context, or if system contexts are equal and either side has a wildcard
 * match for the app context,
 * otherwise FALSE. Two NULL contexts match.
 */
gboolean
gsignond_security_context_match (const GSignondSecurityContext *ctx1,
                                 const GSignondSecurityContext *ctx2)
{
    if (ctx1 == ctx2) return TRUE;

    if (ctx1 == NULL || ctx2 == NULL)
         return FALSE;

    if (g_strcmp0(ctx1->sys_ctx, "*") == 0 ||
        g_strcmp0(ctx2->sys_ctx, "*") == 0) return TRUE;

    if (g_strcmp0(ctx1->sys_ctx, ctx2->sys_ctx) == 0) {
        if (g_strcmp0(ctx1->app_ctx, "*") == 0 ||
            g_strcmp0(ctx2->app_ctx, "*") == 0) return TRUE;
        if (g_strcmp0(ctx1->app_ctx, ctx2->app_ctx) == 0) return TRUE;
    }

    return FALSE;
}

/**
 * gsignond_security_context_check:
 * @reference: reference security context item to check against.
 * @test: security context item to be checked.
 *
 * Check if @test is covered by @reference.
 *
 * Returns: TRUE if contexts are equal or the @reference has a wildcard
 * system context, or if system contexts are equal and @reference has a wildcard
 * application context, otherwise FALSE. If either or both contexts are NULL, 
 * FALSE is returned.
 */
gboolean
gsignond_security_context_check (const GSignondSecurityContext *reference,
                                 const GSignondSecurityContext *test)
{
    if (reference == NULL || test == NULL)
         return FALSE;

    if (g_strcmp0(reference->sys_ctx, "*") == 0) return TRUE;
    if (g_strcmp0(reference->sys_ctx, test->sys_ctx) == 0) {
        if (g_strcmp0(reference->app_ctx, "*") == 0) return TRUE;
        if (g_strcmp0(reference->app_ctx, test->app_ctx) == 0) return TRUE;
    }

    return FALSE;
}

/**
 * gsignond_security_context_list_to_variant:
 * @list: #GSignondSecurityContextList item.
 *
 * Builds a GVariant of type "a(ss)" from a GList of #GSignondSecurityContext
 * items.
 *
 * Returns: (transfer full): GVariant construct of a #GSignondSecurityContextList.
 */
GVariant *
gsignond_security_context_list_to_variant (
                                        const GSignondSecurityContextList *list)
{
    GVariantBuilder builder;
    GVariant *variant;
    GSignondSecurityContext *ctx;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
    for ( ; list != NULL; list = g_list_next (list)) {
        ctx = (GSignondSecurityContext *) list->data;
        g_variant_builder_add_value (
                                    &builder,
                                    gsignond_security_context_to_variant (ctx));
    }
    variant = g_variant_builder_end (&builder);

    return variant;
}

/**
 * gsignond_security_context_list_from_variant:
 * @variant: GVariant item with a list of security context tuples.
 *
 * Builds a GList of #GSignondSecurityContext items from a GVariant of type
 * "a(ss)".
 *
 * Returns: (transfer full): #GSignondSecurityContextList item.
 */
GSignondSecurityContextList *
gsignond_security_context_list_from_variant (GVariant *variant)
{
    GSignondSecurityContextList *list = NULL;
    GVariantIter iter;
    GVariant *value;

    g_return_val_if_fail (variant != NULL, NULL);

    g_variant_iter_init (&iter, variant);
    while ((value = g_variant_iter_next_value (&iter))) {
        list = g_list_append (list,
                              gsignond_security_context_from_variant (value));
        g_variant_unref (value);
    }

    return list;
}

/**
 * gsignond_security_context_list_copy:
 * @src_list: source #GSignondSecurityContextList.
 *
 * Copies a GList of #GSignondSecurityContext items.
 *
 * Returns: (transfer full): #GSignondSecurityContextList item.
 */
GSignondSecurityContextList *
gsignond_security_context_list_copy (
                                    const GSignondSecurityContextList *src_list)
{
    GSignondSecurityContext *ctx;
    GSignondSecurityContextList *dst_list = NULL;

    for ( ; src_list != NULL; src_list = g_list_next (src_list)) {
        ctx = (GSignondSecurityContext *) src_list->data;
        dst_list = g_list_append (dst_list,
                                  gsignond_security_context_copy (ctx));
    }

    return dst_list;
}

/**
 * gsignond_security_context_list_free:
 * @seclist: (transfer full): #GSignondSecurityContextList item.
 *
 * Frees all items and the GList of #GSignondSecurityContext.
 */
void
gsignond_security_context_list_free (GSignondSecurityContextList *seclist)
{
    g_list_free_full (seclist, _security_context_free);
}

