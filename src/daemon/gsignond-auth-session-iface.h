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

#ifndef __GSIGNOND_AUTH_SESSION_IFACE_H_
#define __GSIGNOND_AUTH_SESSION_IFACE_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_AUTH_SESSION_IFACE          (gsignond_auth_session_iface_get_type ())
#define GSIGNOND_AUTH_SESSION_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_AUTH_SESSION_IFACE, GSignondAuthSessionIface))
#define GSIGNOND_IS_AUTH_SESSION_IFACE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_AUTH_SESSION_IFACE))
#define GSIGNOND_AUTH_SESSION_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_AUTH_SESSION_IFACE, GSignondAuthSessionIfaceInterface))

typedef struct _GSignondAuthSessionIface GSignondAuthSessionIface; /* dummy */
typedef struct _GSignondAuthSessionIfaceInterface GSignondAuthSessionIfaceInterface;

struct _GSignondAuthSessionIfaceInterface {
    GTypeInterface parent;

    gchar **   (*query_available_mechanisms) (GSignondAuthSessionIface *iface, const gchar **wanted_mechansims);
    GVariant * (*process) (GSignondAuthSessionIface *iface, const GVariant *session_data, const gchar *mechanism);
    void       (*cancel) (GSignondAuthSessionIface *iface);
    void       (*set_id) (GSignondAuthSessionIface *iface, guint32 id);
};

GType gsignond_auth_session_iface_get_type (void);

gchar ** gsignond_auth_session_iface_query_available_mechanisms (
                                            GSignondAuthSessionIface *self,
                                            const gchar **wanted_mechanisms);
GVariant * gsignond_auth_session_iface_process (GSignondAuthSessionIface *self,
                                                const GVariant *session_data,
                                                const gchar *mechanism);
void gsignond_auth_session_iface_cancel (GSignondAuthSessionIface *self);
void gsignond_auth_session_iface_set_id (GSignondAuthSessionIface *self,
                                         guint32 id);

G_END_DECLS

#endif /* __GSIGNOND_AUTH_SESSION_IFACE_H_ */
