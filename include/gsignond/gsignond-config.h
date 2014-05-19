/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
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

#ifndef __GSIGNOND_CONFIG_H_
#define __GSIGNOND_CONFIG_H_

#include <glib.h>
#include <glib-object.h>

#include "gsignond-config-general.h"
#include "gsignond-config-dbus.h"

G_BEGIN_DECLS

#define GSIGNOND_TYPE_CONFIG            (gsignond_config_get_type())
#define GSIGNOND_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_CONFIG, GSignondConfig))
#define GSIGNOND_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_CONFIG, GSignondConfigClass))
#define GSIGNOND_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_CONFIG))
#define GSIGNOND_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_CONFIG))
#define GSIGNOND_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_CONFIG, GSignondConfigClass))

typedef struct _GSignondConfig GSignondConfig;
typedef struct _GSignondConfigClass GSignondConfigClass;
typedef struct _GSignondConfigPrivate GSignondConfigPrivate;

struct _GSignondConfig
{
    GObject parent;

    /* priv */
    GSignondConfigPrivate *priv;
};

struct _GSignondConfigClass
{
    /*< private >*/
    GObjectClass parent_class;
};

GType gsignond_config_get_type (void) G_GNUC_CONST;

GSignondConfig * gsignond_config_new ();

gint
gsignond_config_get_integer (GSignondConfig *self, const gchar *key);

void
gsignond_config_set_integer (GSignondConfig *self, const gchar *key,
                             gint value) ;
const gchar*
gsignond_config_get_string (GSignondConfig *self, const gchar *key);

void
gsignond_config_set_string (GSignondConfig *self, const gchar *key,
                             const gchar *value); 

G_END_DECLS

#endif /* __GSIGNOND_CONFIG_H_ */
