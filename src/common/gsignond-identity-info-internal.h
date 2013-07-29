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

#ifndef __GSIGNOND_IDENTITY_INFO_INTERNAL_H__
#define __GSIGNOND_IDENTITY_INFO_INTERNAL_H__

#include <glib.h>
#include "gsignond-identity-info.h"

G_BEGIN_DECLS

#define GSIGNOND_IDENTITY_INFO_NEW_IDENTITY         0

/*
 * Identity info strings
 * */
#define GSIGNOND_IDENTITY_INFO_ID                    "Id"
#define GSIGNOND_IDENTITY_INFO_USERNAME              "UserName"
#define GSIGNOND_IDENTITY_INFO_SECRET                "Secret"
#define GSIGNOND_IDENTITY_INFO_STORESECRET           "StoreSecret"
#define GSIGNOND_IDENTITY_INFO_CAPTION               "Caption"
#define GSIGNOND_IDENTITY_INFO_REALMS                "Realms"
#define GSIGNOND_IDENTITY_INFO_AUTHMETHODS           "AuthMethods"
#define GSIGNOND_IDENTITY_INFO_OWNER                 "Owner"
#define GSIGNOND_IDENTITY_INFO_ACL                   "ACL"
#define GSIGNOND_IDENTITY_INFO_TYPE                  "Type"
#define GSIGNOND_IDENTITY_INFO_REFCOUNT              "RefCount"
#define GSIGNOND_IDENTITY_INFO_VALIDATED             "Validated"
#define GSIGNOND_IDENTITY_INFO_USERNAME_IS_SECRET    "UserNameSecret"

#define GSIGNOND_IDENTITY_INFO_EDIT_FLAGS            "EditFlags"

typedef enum {
    IDENTITY_INFO_PROP_NONE = 0x0000,
    
    IDENTITY_INFO_PROP_ID = 0x0001,
    IDENTITY_INFO_PROP_TYPE = 0x0002,
    IDENTITY_INFO_PROP_CAPTION = 0x0004,
    IDENTITY_INFO_PROP_USERNAME = 0x0008,
    IDENTITY_INFO_PROP_SECRET = 0x0010,
    IDENTITY_INFO_PROP_STORE_SECRET = 0x0020,
    IDENTITY_INFO_PROP_USERNAME_IS_SECRET = 0x0040,
    IDENTITY_INFO_PROP_OWNER = 0x0080,
    IDENTITY_INFO_PROP_ACL = 0x0100,
    IDENTITY_INFO_PROP_METHODS = 0x0200,
    IDENTITY_INFO_PROP_REALMS = 0x0400,
    IDENTITY_INFO_PROP_REF_COUNT = 0x0800,
    IDENTITY_INFO_PROP_VALIDATED = 0x1000,

    IDENTITY_INFO_PROP_ALL = 0x1ffff

} GSignondIdentityInfoPropFlags;

GSignondIdentityInfoPropFlags
gsignond_identity_info_get_edit_flags (GSignondIdentityInfo *info);

gboolean
gsignond_identity_info_set_edit_flags (GSignondIdentityInfo *info,
                                       GSignondIdentityInfoPropFlags flags);

gboolean
gsignond_identity_info_reset_edit_flags (GSignondIdentityInfo *info,
                                         GSignondIdentityInfoPropFlags flags);

gboolean
gsignond_identity_info_unset_edit_flags (GSignondIdentityInfo *info,
                                         GSignondIdentityInfoPropFlags flags);


G_END_DECLS

#endif /* __GSIGNOND_IDENTITY_INFO_INTERNAL_H__ */
