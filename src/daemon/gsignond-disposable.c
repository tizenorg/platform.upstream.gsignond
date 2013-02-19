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

#include "gsignond-disposable.h"
#include "gsignond/gsignond-log.h"

struct _GSignondDisposablePrivate
{
    guint    timeout;       /* timeout in seconds */
    gboolean auto_dispose; /* auto dispose */
    gint64   last_active_time; /* last active time */
    guint    timer_id;      /* timer source id */
};

enum {
    PROP_0,
    PROP_TIMEOUT,
    PROP_AUTO_DISPOSE,
    PROP_MAX,
};

enum {
    SIG_DISPOSING,
    SIG_MAX
};

static GParamSpec *properties[PROP_MAX];
static guint       signals[SIG_MAX];

#define GSIGNOND_DISPOSABLE_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DISPOSABLE, GSignondDisposablePrivate)

G_DEFINE_ABSTRACT_TYPE (GSignondDisposable, gsignond_disposable, G_TYPE_OBJECT);

static void
_set_property (GObject *object,
               guint property_id,
               const GValue *value,
               GParamSpec *pspec)
{
    g_return_if_fail (object && GSIGNOND_IS_DISPOSABLE (object));

    GSignondDisposable *self = GSIGNOND_DISPOSABLE (object);

    switch (property_id) {
        case PROP_TIMEOUT:
            gsignond_disposable_set_timeout (self, g_value_get_uint (value));
            break;
        case PROP_AUTO_DISPOSE:
            gsignond_disposable_set_auto_dispose (self, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_get_property (GObject *object,
               guint property_id,
               GValue *value,
               GParamSpec *pspec)
{
    g_return_if_fail (object && GSIGNOND_IS_DISPOSABLE (object));

    GSignondDisposable *self = GSIGNOND_DISPOSABLE (object);

    switch (property_id) {
        case PROP_TIMEOUT:
            g_value_set_int (value, self->priv->timeout);
            break;
        case PROP_AUTO_DISPOSE:
            g_value_set_boolean (value, self->priv->auto_dispose);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    g_return_if_fail (object && GSIGNOND_IS_DISPOSABLE (object));

    G_OBJECT_CLASS (gsignond_disposable_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    g_return_if_fail (object && GSIGNOND_IS_DISPOSABLE (object));

    GSignondDisposable *self = GSIGNOND_DISPOSABLE (object);

    if (self->priv->timer_id) {
        g_source_remove (self->priv->timer_id);
        self->priv->timer_id = 0;
    }

    G_OBJECT_CLASS (gsignond_disposable_parent_class)->finalize (object);
}

static void
gsignond_disposable_class_init (GSignondDisposableClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDisposablePrivate));

    object_class->set_property = _set_property;
    object_class->get_property = _get_property;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

    properties[PROP_TIMEOUT] = 
        g_param_spec_uint ("timeout",
                           "Object timeout",
                           "object timeout",
                           0,
                           G_MAXINT,
                           0,
                           G_PARAM_READWRITE /*| G_PARAM_CONSTRUCT_ONLY*/);

    properties[PROP_AUTO_DISPOSE] =
        g_param_spec_boolean ("auto-dispose",
                              "Auto dispose",
                              "auto dispose",
                              TRUE,
                              G_PARAM_READWRITE/* | G_PARAM_CONSTRUCT*/);

    g_object_class_install_properties (object_class, PROP_MAX, properties);

    signals[SIG_DISPOSING] = g_signal_new ("disposing",
            GSIGNOND_TYPE_DISPOSABLE,
            G_SIGNAL_RUN_FIRST| G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            0,
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            0,
            G_TYPE_NONE);
}

static void
gsignond_disposable_init (GSignondDisposable *self)
{
    self->priv = GSIGNOND_DISPOSABLE_PRIV (self);

    self->priv->timer_id = 0;
    self->priv->last_active_time = 0l;
    self->priv->timeout = 0;
    self->priv->auto_dispose = TRUE;

    gsignond_disposable_set_keep_in_use (self);
}

static gboolean
_auto_dispose (gpointer user_data)
{
    g_return_val_if_fail (user_data && GSIGNOND_IS_DISPOSABLE (user_data), FALSE);

    GSignondDisposable *self = GSIGNOND_DISPOSABLE (user_data);
    gint64 current_time;

    current_time = g_get_monotonic_time ();
    DBG("current time : %ld, last seen activity : %ld", 
          current_time,
          self->priv->last_active_time);
    /* check if object idle timeout reached */
    if ((current_time - self->priv->last_active_time)
               > self->priv->timeout) {
        DBG("Disposing object : %s(%d)", 
            G_OBJECT_TYPE_NAME(self),
            G_OBJECT(self)->ref_count);
        g_signal_emit (self, signals[SIG_DISPOSING], 0);
        /* destroy object */
        g_object_unref (G_OBJECT (self));
        return FALSE;
    }

    /* Try for next iteration, NOTE: This case shouldn't arise */
    return TRUE;
}

static void
_update_timer (GSignondDisposable *self)
{
    DBG("%s: auto_dispose : %d, timout : %d", 
                  G_OBJECT_TYPE_NAME(self),
                  self->priv->auto_dispose, 
                  self->priv->timeout);
    if (self->priv->auto_dispose) {
        if (self->priv->timeout) {
            INFO("Setting object timeout to %d", self->priv->timeout);
            self->priv->timer_id = g_timeout_add_seconds (self->priv->timeout,
                                                          _auto_dispose,
                                                          self);
        }
    }
    else if (self->priv->timer_id) {
        g_source_remove (self->priv->timer_id);
        self->priv->timer_id = 0;
    }
}

void
gsignond_disposable_set_auto_dispose (GSignondDisposable *self,
                                      gboolean dispose)
{
    g_return_if_fail (self && GSIGNOND_IS_DISPOSABLE (self));

    if (self->priv->auto_dispose == dispose) return;

    self->priv->auto_dispose = dispose;

    _update_timer (self);
}

void
gsignond_disposable_set_timeout (GSignondDisposable *self,
                                 guint timeout)
{
    g_return_if_fail (self && GSIGNOND_IS_DISPOSABLE (self));

    if (self->priv->timeout == timeout) return;

    self->priv->timeout = timeout;

    _update_timer (self);
}

void
gsignond_disposable_set_keep_in_use (GSignondDisposable *self)
{
    self->priv->last_active_time = g_get_monotonic_time ();

    /* check if need to reset timer */
    if (!self->priv->auto_dispose || !self->priv->timeout) return ;

    if (self->priv->timer_id) 
        g_source_remove (self->priv->timer_id);

    INFO ("Resetting timer for object '%s'.", G_OBJECT_TYPE_NAME (self));
    self->priv->timer_id = g_timeout_add_seconds (self->priv->timeout, 
                                                  _auto_dispose, self);
}

