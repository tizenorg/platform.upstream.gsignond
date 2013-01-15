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
/*
 * TODO: Currently logging is directed to stdio, should resolve this properly
 */
#define INFO(frmt, args...) g_print("[I] %s:%d  " frmt "\n", __func__, __LINE__, ##args)
#define ERR(frmt, args...)  g_print("[E] %s:%d  " frmt "\n", __func__, __LINE__, ##args)
#define WARN(frmt, args...)  g_print("[W] %s:%d  " frmt "\n", __func__, __LINE__, ##args)
#define DBG(frmt, args...)  g_print("[D] %s:%d  " frmt "\n", __func__, __LINE__, ##args)

#endif /* __GSIGNOND_LOG_H_ */