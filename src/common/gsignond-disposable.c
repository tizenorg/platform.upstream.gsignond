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
    guint timeout;       /* timeout in seconds */
    volatile gint  keep_obj_counter; /* keep object request counter */
    guint timer_id;      /* timer source id */
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
            g_value_set_boolean (value, gsignond_disposable_get_auto_dispose(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    g_return_if_fail (object && GSIGNOND_IS_DISPOSABLE (object));

    GSignondDisposable *self = GSIGNOND_DISPOSABLE (object);

    DBG ("%s DISPOSE", G_OBJECT_TYPE_NAME (self));
    if (self->priv->timer_id) {
        DBG (" - TIMER CLEAR");
        g_source_remove (self->priv->timer_id);
        self->priv->timer_id = 0;
    }

    G_OBJECT_CLASS (gsignond_disposable_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    g_return_if_fail (object && GSIGNOND_IS_DISPOSABLE (object));

    DBG ("FINALIZE");
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
                           G_MAXUINT,
                           0,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_AUTO_DISPOSE] =
        g_param_spec_boolean ("auto-dispose",
                              "Auto dispose",
                              "auto dispose",
                              TRUE,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

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
    self->priv->timeout = 0;
    g_atomic_int_set(&self->priv->keep_obj_counter, 0);

    DBG ("INIT");
}

static gboolean
_auto_dispose (gpointer user_data)
{
    g_return_val_if_fail (user_data && GSIGNOND_IS_DISPOSABLE (user_data), FALSE);

    GSignondDisposable *self = GSIGNOND_DISPOSABLE (user_data);
    g_signal_emit (self, signals[SIG_DISPOSING], 0);
    /* destroy object */
    DBG ("%s AUTO DISPOSE", G_OBJECT_TYPE_NAME (self));
    g_object_unref (G_OBJECT (self));
    return FALSE;
}

static gboolean
_timer_dispose (gpointer user_data)
{
    g_return_val_if_fail (user_data && GSIGNOND_IS_DISPOSABLE (user_data), FALSE);
    GSignondDisposable *self = GSIGNOND_DISPOSABLE (user_data);

    DBG ("%s TIMER DISPOSE", G_OBJECT_TYPE_NAME (self));
    /* clear out timer since we are already inside timer cb */
    self->priv->timer_id = 0;

    return _auto_dispose (user_data);
}

static void
_update_timer (GSignondDisposable *self)
{
    DBG("%s (%p): keep_obj_counter : %d, timeout : %d", 
                  G_OBJECT_TYPE_NAME(self),
                  self,
                  self->priv->keep_obj_counter, 
                  self->priv->timeout);
    if (g_atomic_int_get(&self->priv->keep_obj_counter) == 0) {
        if (self->priv->timeout) {
            DBG("Setting object timeout to %d", self->priv->timeout);
            self->priv->timer_id = g_timeout_add_seconds (self->priv->timeout,
                                                          _timer_dispose,
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

    if (g_atomic_int_get(&self->priv->keep_obj_counter) == 0 && dispose) return;

    g_atomic_int_add (&self->priv->keep_obj_counter, !dispose ? +1 : -1);

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
gsignond_disposable_delete_later (GSignondDisposable *self)
{
    if (self->priv->timer_id)
            g_source_remove (self->priv->timer_id);

    INFO ("Object '%s' (%p) about to dispose...",
          G_OBJECT_TYPE_NAME (self), self);
    self->priv->timer_id = g_idle_add (_auto_dispose, self);
}

gboolean
gsignond_disposable_get_auto_dispose (GSignondDisposable *self)
{
    g_return_val_if_fail (self && GSIGNOND_IS_DISPOSABLE(self), FALSE);

    return g_atomic_int_get(&self->priv->keep_obj_counter) == 0 ;
}
