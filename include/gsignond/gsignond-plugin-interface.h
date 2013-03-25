/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
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

#ifndef _GSIGNOND_PLUGIN_H_
#define _GSIGNOND_PLUGIN_H_

#include <glib.h>
#include <glib-object.h>
#include <gsignond/gsignond-session-data.h>
#include <gsignond/gsignond-signonui-data.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_PLUGIN    (gsignond_plugin_get_type ())
#define GSIGNOND_PLUGIN(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_PLUGIN, GSignondPlugin))
#define GSIGNOND_IS_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_PLUGIN))
#define GSIGNOND_PLUGIN_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_PLUGIN, GSignondPluginInterface))

typedef struct _GSignondPlugin GSignondPlugin; /* dummy object */
typedef struct _GSignondPluginInterface GSignondPluginInterface;

/**
 * GSignondPluginState:
 * @GSIGNOND_PLUGIN_STATE_NONE: State unknown
 * @SIGNOND_PLUGIN_STATE_RESOLVING: Resolving remote server host name
 * @GSIGNOND_PLUGIN_STATE_CONNECTING: Connecting to remote server
 * @GSIGNOND_PLUGIN_STATE_SENDING_DATA: Sending data to remote server
 * @GSIGNOND_PLUGIN_STATE_WAITING: Waiting for reply from remote server
 * @GSIGNOND_PLUGIN_STATE_USER_PENDING: Waiting for response from user
 * @GSIGNOND_PLUGIN_STATE_REFRESHING: Refreshing ui request
 * @GSIGNOND_PLUGIN_STATE_PROCESS_PENDING: Request has been queued
 * @GSIGNOND_PLUGIN_STATE_STARTED: Request has been dequeued
 * @GSIGNOND_PLUGIN_STATE_CANCELING: Canceling current process
 * @GSIGNOND_PLUGIN_STATE_DONE: Process is finished
 * @GSIGNOND_PLUGIN_STATE_HOLDING: Holding long non-expired token
 * @GSIGNOND_PLUGIN_STATE_CUSTOM: Custom message
 */
typedef enum {
    GSIGNOND_PLUGIN_STATE_NONE = 0,
    GSIGNOND_PLUGIN_STATE_RESOLVING,
    GSIGNOND_PLUGIN_STATE_CONNECTING,
    GSIGNOND_PLUGIN_STATE_SENDING_DATA,
    GSIGNOND_PLUGIN_STATE_WAITING,
    GSIGNOND_PLUGIN_STATE_USER_PENDING,
    GSIGNOND_PLUGIN_STATE_REFRESHING,
    GSIGNOND_PLUGIN_STATE_PROCESS_PENDING,
    GSIGNOND_PLUGIN_STATE_STARTED,
    GSIGNOND_PLUGIN_STATE_CANCELING,
    GSIGNOND_PLUGIN_STATE_DONE,
    GSIGNOND_PLUGIN_STATE_HOLDING,
} PluginState;
typedef PluginState GSignondPluginState;

struct _GSignondPluginInterface {
    GTypeInterface parent;

    void (*cancel) (GSignondPlugin *self);
    void (*abort) (GSignondPlugin *self);
    void (*request_initial) (GSignondPlugin *self, 
                     GSignondSessionData *session_data, 
                     const gchar *mechanism);
    void (*request) (GSignondPlugin *self, 
                     GSignondSessionData *session_data);
    void (*user_action_finished) (GSignondPlugin *self, 
                                  GSignondSessionData *session_data);
    void (*refresh) (GSignondPlugin *self, 
                     GSignondSessionData *session_data);
};

GType gsignond_plugin_get_type (void);

/* Methods */
void 
gsignond_plugin_cancel (GSignondPlugin *self);
void 
gsignond_plugin_abort (GSignondPlugin *self);
void 
gsignond_plugin_request_initial (GSignondPlugin *self, 
                              GSignondSessionData *session_data, 
                              const gchar *mechanism);
void 
gsignond_plugin_request (GSignondPlugin *self, 
                              GSignondSessionData *session_data);
void 
gsignond_plugin_user_action_finished (GSignondPlugin *self, 
                                      GSignondSignonuiData *ui_data);
void 
gsignond_plugin_refresh (GSignondPlugin *self, 
                         GSignondSignonuiData *ui_data);

/* Signals */
void 
gsignond_plugin_response (GSignondPlugin *self, 
                             GSignondSessionData *session_data);
void 
gsignond_plugin_response_final (GSignondPlugin *self, 
                             GSignondSessionData *session_data);
void 
gsignond_plugin_store (GSignondPlugin *self, 
                            GSignondSessionData *session_data);
void 
gsignond_plugin_error (GSignondPlugin *self, GError *error);
void 
gsignond_plugin_user_action_required (GSignondPlugin *self, 
                                      GSignondSignonuiData *ui_data);
void 
gsignond_plugin_refreshed (GSignondPlugin *self, 
                           GSignondSignonuiData *ui_data);
void 
gsignond_plugin_status_changed (GSignondPlugin *self, 
                                     GSignondPluginState state, 
                                     const gchar *message);

G_END_DECLS

#endif /* _GSIGNOND_PLUGIN_H_ */
