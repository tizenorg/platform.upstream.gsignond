/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
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

#ifndef __GSIGNOND_CONFIG_DB_H_
#define __GSIGNOND_CONFIG_DB_H_

#define GSIGNOND_CONFIG_DB_SSO                  "Storage"
#define GSIGNOND_CONFIG_DB_FILE_SYSTEM_NAME     GSIGNOND_CONFIG_DB_SSO \
                                                "/FileSystemName"
#define GSIGNOND_CONFIG_DB_SIZE                 GSIGNOND_CONFIG_DB_SSO \
                                                "/Size"
#define GSIGNOND_CONFIG_DB_FILE_SYSTEM_TYPE     GSIGNOND_CONFIG_DB_SSO \
                                                "/FileSystemType"
#define GSIGNOND_CONFIG_DB_SECRET_DB_FILENAME   GSIGNOND_CONFIG_DB_SSO \
                                                "/SecretDBFilename"
#define GSIGNOND_CONFIG_DB_METADATA_DB_FILENAME GSIGNOND_CONFIG_DB_SSO \
                                                "/MetadataDBFilename"

#endif /* __GSIGNOND_DB_CONFIG_H_ */
