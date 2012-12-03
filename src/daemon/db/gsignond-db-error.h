/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2011 Intel Corporation.
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
#ifndef __GSIGNOND_DB_ERROR_H__
#define __GSIGNOND_DB_ERROR_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * GSIGNOND_DB_ERROR:
 *
 */
#define GSIGNOND_DB_ERROR   (gsignond_db_error_quark())

typedef enum {
    GSIGNOND_DB_ERROR_NONE,
    GSIGNOND_DB_ERROR_NOT_OPEN,             /*!< The DB is not open */
    GSIGNOND_DB_ERROR_CONNECTION_FAILURE,   /*!< The DB is disconnected */
    GSIGNOND_DB_ERROR_STATEMENT_FAILURE,    /*!< The last command failed */
    GSIGNOND_DB_ERROR_UNKNOWN
} GSignondDbError;

GQuark gsignond_db_error_quark (void);

G_END_DECLS

#endif /* __GSIGNOND_DB_ERROR_H__ */
