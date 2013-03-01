/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Amarnath Valluri <amarnath.valluri@linux.intel.com>
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

#ifndef __GSIGNOND_DISPOSABLE_H_
#define __GSIGNOND_DISPOSABLE_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_DISPOSABLE            (gsignond_disposable_get_type())
#define GSIGNOND_DISPOSABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_DISPOSABLE, GSignondDisposable))
#define GSIGNOND_DISPOSABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_DISPOSABLE, GSignondDisposableClass))
#define GSIGNOND_IS_DISPOSABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_DISPOSABLE))
#define GSIGNOND_IS_DISPOSABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_DISPOSABLE))
#define GSIGNOND_DISPOSABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_DISPOSABLE, GSignondDisposableClass))

typedef struct _GSignondDisposable GSignondDisposable;
typedef struct _GSignondDisposableClass GSignondDisposableClass;
typedef struct _GSignondDisposablePrivate GSignondDisposablePrivate;

struct _GSignondDisposable
{
    GObject parent;

    /* priv */
    GSignondDisposablePrivate *priv;
};

struct _GSignondDisposableClass
{
    GObjectClass parent_class;
};

GType gsignond_disposable_get_type (void) G_GNUC_CONST;

void
gsignond_disposable_set_keep_in_use (GSignondDisposable *disposable);

void
gsignond_disposable_set_auto_dispose (GSignondDisposable *disposable,
                                      gboolean dispose);

void
gsignond_disposable_set_timeout (GSignondDisposable *self,
                                 guint timeout);

void
gsignond_disposable_delete_later (GSignondDisposable *self);

G_END_DECLS

#endif /* __GSIGNOND_DISPOSABLE_H_ */
