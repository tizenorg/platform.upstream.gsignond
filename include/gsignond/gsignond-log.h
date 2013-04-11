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

#ifndef __GSIGNOND_LOG_H_
#define __GSIGNOND_LOG_H_

#include <glib.h>

#define INFO(frmt, args...) g_message("%s:%d %s " frmt , __FILE__, __LINE__, \
        __PRETTY_FUNCTION__, ##args)
#define ERR(frmt, args...)  g_error("%s:%d %s " frmt , __FILE__, __LINE__, \
        __PRETTY_FUNCTION__, ##args)
#define WARN(frmt, args...)  g_warning("%s:%d %s " frmt , __FILE__, __LINE__, \
        __PRETTY_FUNCTION__, ##args)
#define DBG(frmt, args...)  g_debug("%s:%d %s " frmt , __FILE__, __LINE__, \
        __PRETTY_FUNCTION__, ##args)

#endif /* __GSIGNOND_LOG_H_ */
