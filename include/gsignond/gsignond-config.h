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
#include "gsignond-config-db.h"
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


#ifndef GSIGNOND_PLUGINS_DIR
#   define GSIGNOND_PLUGINS_DIR "/usr/lib/signon"
#endif

#ifndef GSIGNOND_PLUGIN_PREFIX
#   define GSIGNOND_PLUGIN_PREFIX "lib"
#endif

#ifndef GSIGNOND_PLUGIN_SUFFIX
#   define GSIGNOND_PLUGIN_SUFFIX "plugin.so"
#endif

#ifndef GSIGNOND_EXTENSIONS_DIR
#   define GSIGNOND_EXTENSIONS_DIR "/usr/lib"
#endif

struct _GSignondConfig
{
    GObject parent;

    /* priv */
    GSignondConfigPrivate *priv;
};

struct _GSignondConfigClass
{
    GObjectClass parent_class;
};

GType gsignond_config_get_type (void) G_GNUC_CONST;

GSignondConfig * gsignond_config_new ();

gboolean gsignond_config_set_plugins_dir (GSignondConfig *config,
                                          const gchar *dir);
const gchar * gsignond_config_get_plugins_dir (GSignondConfig *config) G_GNUC_CONST;

gboolean gsignond_config_set_extensions_dir (GSignondConfig *config,
                                             const gchar *dir);
const gchar * gsignond_config_get_extensions_dir (GSignondConfig *config) G_GNUC_CONST;

gboolean gsignond_config_set_extension (GSignondConfig *config,
                                        const gchar *extension);
const gchar * gsignond_config_get_extension (GSignondConfig *config) G_GNUC_CONST;

gboolean gsignond_config_set_daemon_timeout (GSignondConfig *config,
                                             guint timeout);
guint gsignond_config_get_daemon_timeout (GSignondConfig *config) G_GNUC_CONST;

gboolean gsignond_config_set_identity_timeout (GSignondConfig *config,
                                               guint timeout);
guint gsignond_config_get_identity_timeout (GSignondConfig *config) G_GNUC_CONST;

gboolean gsignond_config_set_auth_session_timeout (GSignondConfig *config,
                                                   guint timeout);
guint gsignond_config_get_auth_session_timeout (GSignondConfig *config) G_GNUC_CONST;

const GHashTable * gsignond_config_get_config_table (GSignondConfig *config);

G_END_DECLS

#endif /* __GSIGNOND_CONFIG_H_ */
