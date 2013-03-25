/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Amarnath Valluri<amarnath.valluri@linux.intel.com>
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

#ifndef __GSIGNOND_SIGNONUI_PROXY_H__
#define __GSIGNOND_SIGNONUI_PROXY_H__

#include <glib-object.h>
#include <gsignond/gsignond-signonui-data.h>

#define GSIGNOND_TYPE_SIGNONUI_PROXY             (gsignond_signonui_proxy_get_type ())
#define GSIGNOND_SIGNONUI_PROXY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_SIGNONUI_PROXY, GSignondSignonuiProxy))
#define GSIGNOND_IS_SIGNONUI_PROXY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_SIGNONUI_PROXY))
#define GSIGNOND_SIGNONUI_PROXY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_SIGNONUI_PROXY, GSignondSignonuiProxyClass))
#define GSIGNOND_IS_SIGNONUI_PROXY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_SIGNONUI_PROXY))
#define GSIGNOND_SIGNONUI_PROXY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_SIGNONUI_PROXY, GSignondSignonuiProxyClass))

typedef struct _GSignondSignonuiProxy      GSignondSignonuiProxy;
typedef struct _GSignondSignonuiProxyClass GSignondSignonuiProxyClass;
typedef struct _GSignondSignonuiProxyPrivate GSignondSignonuiProxyPrivate;

typedef void (*GSignondSignonuiProxyQueryDialogCb)(GSignondSignonuiData *reply, GError *error, gpointer userdata);
typedef void (*GSignondSignonuiProxyRefreshCb)(GSignondSignonuiData *ui_data, gpointer userdata);
typedef void (*GSignondSignonuiProxyRefreshDialogCb) (GError *error, gpointer userdata);
typedef void (*GSignondSignonuiProxyCancelRequestCb) (GError *error, gpointer userdata);

struct _GSignondSignonuiProxy
{
    GObject parent;

    /* Private */
    GSignondSignonuiProxyPrivate *priv;
};

struct _GSignondSignonuiProxyClass
{
    GObjectClass parent_class;
};

GType gsignond_signonui_proxy_get_type (void);

GSignondSignonuiProxy *
gsignond_signonui_proxy_new ();

gboolean
gsignond_signonui_proxy_query_dialog (GSignondSignonuiProxy *proxy,
                                      GObject *caller,
                                      GSignondSignonuiData *ui_data,
                                      GSignondSignonuiProxyQueryDialogCb callback,
                                      GSignondSignonuiProxyRefreshCb refresh_cb,
                                      gpointer userdata);

gboolean
gsignond_signonui_proxy_refresh_dialog (GSignondSignonuiProxy *proxy,
                                        GObject *caller,
                                        GSignondSignonuiData *ui_data,
                                        GSignondSignonuiProxyRefreshDialogCb callback,
                                        gpointer userdata);

gboolean
gsignond_signonui_proxy_cancel_request (GSignondSignonuiProxy *proxy,
                                        GObject *caller,
                                        GSignondSignonuiProxyCancelRequestCb callback,
                                        gpointer userdata);

#endif //__GSIGNOND_SIGNONUI_PROXY_H__
