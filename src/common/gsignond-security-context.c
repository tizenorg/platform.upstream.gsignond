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
    ctx->app_ctx = g_strdup ("");

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
        ctx->app_ctx = g_strdup ("");

    return ctx;
}

/**
 * gsignond_security_context_copy:
 * @src_ctx: source security context to copy.
 *
 * Copy a security context item.
 *
 * Returns: (transfer full) a copy of the #GSignonDSecurityContext item.
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
gsignond_security_context_get_system_context (
                                             const GSignonDSecurityContext *ctx)
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
gsignond_security_context_set_application_context (
                                               GSignonDSecurityContext *ctx,
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
gsignond_security_context_get_application_context (
                                             const GSignonDSecurityContext *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);

    return ctx->app_ctx;
}

/**
 * signon_security_conetxt_to_variant:
 * @ctx: #GSignonDSecurityContext item.
 *
 * Build a GVariant of type "as" from a #GSignonDSecurityContext item.
 *
 * Returns: (transfer full) GVariant construct of a #GSignonDSecurityContext.
 */
GVariant *
gsignond_security_context_to_variant (const GSignonDSecurityContext *ctx)
{
    GVariantBuilder builder;
    GVariant *variant;

    g_return_val_if_fail (ctx != NULL, NULL);

    /* clean way, but what is the supported QDBus item? */
    /*variant = g_variant_new ("(ss)", ctx->sys_ctx, ctx->app_ctx);*/

    g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add_value (&builder, (ctx->sys_ctx) ?
                                 g_variant_new_string (ctx->sys_ctx) :
                                 g_variant_new_string (""));
    g_variant_builder_add_value (&builder, (ctx->app_ctx) ?
                                 g_variant_new_string (ctx->app_ctx) :
                                 g_variant_new_string (""));
    variant = g_variant_builder_end (&builder);

    return variant;
}

/**
 * gsignond_security_context_from_variant:
 * @variant: GVariant item with a #GSignonDSecurityContext construct.
 *
 * Builds a #GSignonDSecurityContext item from a GVariant of type "as".
 *
 * Returns: (transfer full) #GSignonDSecurityContext item.
 */
GSignonDSecurityContext *
gsignond_security_context_from_variant (GVariant *variant)
{
    const gchar *sys_ctx = NULL;
    const gchar *app_ctx = NULL;
    GVariantIter iter;
    GVariant *value;

    g_return_val_if_fail (variant != NULL, NULL);

    g_variant_iter_init (&iter, variant);
    value = g_variant_iter_next_value (&iter);
    if (value) {
        sys_ctx = g_variant_get_string (value, NULL);
        value = g_variant_iter_next_value (&iter);
        if (value)
            app_ctx = g_variant_get_string (value, NULL);
    }

    return gsignond_security_context_new_from_values (sys_ctx, app_ctx);
}

/**
 * gsignond_security_context_compare:
 * @ctx1: first item to compare.
 * @ctx2: second item to compare.
 *
 * Compare two #GSignonDSecurityContext items similar in a way to strcmp().
 *
 * Returns: negative if ctx1 < ctx2, 0 if ctx1 == ctx2 and positive if ctx1 > ctx2.
 */
int
gsignond_security_context_compare (const GSignonDSecurityContext *ctx1,
                                   const GSignonDSecurityContext *ctx2)
{
    int res;

    if (ctx1 == ctx2) return 0;

    g_return_val_if_fail (ctx1 != NULL, -1);
    g_return_val_if_fail (ctx2 != NULL, 1);

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
 * Compare two #GSignonDSecurityContext items match.
 *
 * Returns: TRUE if contexts are equal or either side has wildcard match, otherwise FALSE. Two NULL contexts match.
 */
gboolean
gsignond_security_context_match (const GSignonDSecurityContext *ctx1,
                                 const GSignonDSecurityContext *ctx2)
{
    if (ctx1 == ctx2) return TRUE;

    g_return_val_if_fail (ctx1 != NULL && ctx2 != NULL, FALSE);

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
 * Check if item @test is covered by @reference.
 *
 * Returns: TRUE if contexts are equal or wildcards of the @reference arguments match, otherwise FALSE. If either or both contexts are NULL, FALSE is returned.
 */
gboolean
gsignond_security_context_check (const GSignonDSecurityContext *reference,
                                 const GSignonDSecurityContext *test)
{
    g_return_val_if_fail (reference != NULL && test != NULL, FALSE);

    if (g_strcmp0(reference->sys_ctx, "*")) return TRUE;
    if (g_strcmp0(reference->sys_ctx, test->app_ctx) == 0) {
        if (g_strcmp0(reference->app_ctx, "*")) return TRUE;
        if (g_strcmp0(reference->app_ctx, test->app_ctx) == 0) return TRUE;
    }

    return FALSE;
}

/**
 * gsignond_security_context_list_to_variant:
 * @list: #GSignonDSecurityContextList item.
 *
 * Builds a GVariant of type "aas" from a GList of #GSignonDSecurityContext
 * items.
 *
 * Returns: (transfer full) GVariant construct of a #GSignonDSecurityContextList.
 */
GVariant *
gsignond_security_context_list_to_variant (
                                        const GSignonDSecurityContextList *list)
{
    GVariantBuilder builder;
    GVariant *variant;
    GSignonDSecurityContext *ctx;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
    for ( ; list != NULL; list = g_list_next (list)) {
        ctx = (GSignonDSecurityContext *) list->data;
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
 * Builds a GList of #GSignonDSecurityContext items from a GVariant of type
 * "aas".
 *
 * Returns: (transfer full) #GSignonDSecurityContextList item.
 */
GSignonDSecurityContextList *
gsignond_security_context_list_from_variant (GVariant *variant)
{
    GSignonDSecurityContextList *list = NULL;
    GVariantIter iter;
    GVariant *value;

    g_return_val_if_fail (variant != NULL, NULL);

    g_variant_iter_init (&iter, variant);
    while ((value = g_variant_iter_next_value (&iter))) {
        list = g_list_append (list,
                              gsignond_security_context_from_variant (value));
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
        dst_list = g_list_append (dst_list,
                                  gsignond_security_context_copy (ctx));
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

