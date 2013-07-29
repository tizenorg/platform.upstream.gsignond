/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2013 Intel Corporation.
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

#ifndef __GSIGNOND_IDENTITY_INFO_H__
#define __GSIGNOND_IDENTITY_INFO_H__

#include <glib.h>
#include <glib-object.h>
#include <gsignond/gsignond-security-context.h>
#include <gsignond/gsignond-dictionary.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_IDENTITY_INFO (GSIGNOND_TYPE_DICTIONARY)

#define GSIGNOND_IDENTITY_INFO(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GSIGNOND_TYPE_IDENTITY_INFO, \
                                           GSignondIdentityInfo))
#define GSIGNOND_IS_IDENTITY_INFO(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           GSIGNOND_TYPE_IDENTITY_INFO))

typedef GSignondDictionary GSignondIdentityInfo;
typedef GList GSignondIdentityInfoList;

GSignondIdentityInfo *
gsignond_identity_info_new (void);

GSignondIdentityInfo *
gsignond_identity_info_new_from_variant (GVariant *variatn_mp);

GVariant *
gsignond_identity_info_to_variant (GSignondIdentityInfo *info);

GSignondIdentityInfo *
gsignond_identity_info_copy (GSignondIdentityInfo *info);

void
gsignond_identity_info_ref (GSignondIdentityInfo *info);

void
gsignond_identity_info_unref (GSignondIdentityInfo *info);

guint32
gsignond_identity_info_get_id (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_id (
        GSignondIdentityInfo *info,
        guint32 id);

gboolean
gsignond_identity_info_get_is_identity_new (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_identity_new (GSignondIdentityInfo *info);

const gchar *
gsignond_identity_info_get_username (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_username (
        GSignondIdentityInfo *info,
        const gchar *username);

gboolean
gsignond_identity_info_remove_username (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_get_is_username_secret (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_username_secret (
        GSignondIdentityInfo *info,
        gboolean username_secret);

const gchar *
gsignond_identity_info_get_secret (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_secret (
        GSignondIdentityInfo *info,
        const gchar *secret);

gboolean
gsignond_identity_info_remove_secret (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_get_store_secret (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_store_secret (
        GSignondIdentityInfo *info,
        gboolean store_secret);

const gchar *
gsignond_identity_info_get_caption (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_caption (
        GSignondIdentityInfo *info,
        const gchar *caption);

GSequence *
gsignond_identity_info_get_realms (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_realms (
        GSignondIdentityInfo *info,
        GSequence *realms);

GHashTable *
gsignond_identity_info_get_methods (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_methods (
        GSignondIdentityInfo *info,
        GHashTable *methods);

GSequence *
gsignond_identity_info_get_mechanisms (
        GSignondIdentityInfo *info,
        const gchar *method);

gboolean
gsignond_identity_info_remove_method (
        GSignondIdentityInfo *info,
        const gchar *method);

GSignondSecurityContextList *
gsignond_identity_info_get_access_control_list (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_access_control_list (
        GSignondIdentityInfo *info,
        const GSignondSecurityContextList *acl);

GSignondSecurityContext *
gsignond_identity_info_get_owner (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_owner (
        GSignondIdentityInfo *info,
        const GSignondSecurityContext *owner);

gboolean
gsignond_identity_info_get_validated (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_validated (
        GSignondIdentityInfo *info,
        gboolean validated);

guint32
gsignond_identity_info_get_identity_type (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_identity_type (
        GSignondIdentityInfo *info,
        guint32 type);

gboolean
gsignond_identity_info_compare (
        GSignondIdentityInfo *info,
        GSignondIdentityInfo *other);

void
gsignond_identity_info_list_free (GSignondIdentityInfoList *list);

G_END_DECLS

#endif /* __GSIGNOND_IDENTITY_INFO_H__ */
