/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@intel.com>
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
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <gsignond/gsignond-plugin-interface.h>
#include "gsignond-digest-plugin.h"
#include <gsignond/gsignond-error.h>
#include <gsignond/gsignond-log.h>
#include <gsignond/gsignond-utils.h>

/**
 * SECTION:gsignond-digest-plugin
 * @short_description: a plugin that performs HTTP Digest authentication
 * @include: gsignond/gsignond-digest-plugin.h
 *
 * #GSignondDigestPlugin performs HTTP Digest authentication without exposing
 * the password to the application. Digest authentication is described in 
 * <ulink url="http://tools.ietf.org/html/rfc2617">RFC 2617</ulink>.
 * 
 * gsignond_plugin_request_initial() @session_data parameter should include
 * the following string items, whose meaning is described in the RFC: 
 * - username and secret. If they are absent, they are requested from the user
 * via gSSO UI.
 * - realm, allowed realms, "Algo", "Nonce", "Method", "DigestUri" - mandatory items.
 * - "NonceCount", "Qop", "HEntity". "NonceCount" must be present if "Qop" is
 * present, "HEntity" must be present if "Qop" is present and set to "auth-int".
 * 
 * If the plugin has all the data to calculate the digest, it issues 
 * #GSignondPlugin::response-final signal. @session_data in that signal contains
 * the username, "CNonce" item and the digest value under the "Response" key.
 * 
 * If some of the data is incorrect or not available, #GSignondPlugin::error
 * signal is issued instead.
 * 
 * #GSignondPlugin:type property is set to "digest", and #GSignondPlugin:mechanisms 
 * property contains a single entry "digest".
 */
/**
 * GSignondDigestPlugin:
 *
 * Opaque #GSignondDigestPlugin data structure.
 */
/**
 * GSignondDigestPluginClass:
 * @parent_class: the parent class structure
 *
 * Opaque #GSignondDigestPluginClass data structure.
 */


static void gsignond_plugin_interface_init (GSignondPluginInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GSignondDigestPlugin, gsignond_digest_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_PLUGIN,
                                                gsignond_plugin_interface_init));

#define GSIGNOND_DIGEST_PLUGIN_GET_PRIVATE(obj) \
                                       (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                        GSIGNOND_TYPE_DIGEST_PLUGIN, \
                                        GSignondDigestPluginPrivate))

#define DATA_SET_VALUE(data, key, value) \
    if (value) { \
        gsignond_dictionary_set_string(data, key, value); \
    }
#define TO_GUCHAR(data) ((const guchar*)data)

struct _GSignondDigestPluginPrivate
{
    GSignondSessionData *session_data;
};

static gchar *
_gsignond_digest_plugin_compute_md5_digest (
        const gchar* algo,
        const gchar* username,
        const gchar* realm,
        const gchar* secret,
        const gchar* nonce,
        const gchar* nonce_count,
        const gchar* cnonce,
        const gchar* qop,
        const gchar* method,
        const gchar* digest_uri,
        const gchar* hentity)
{
    GChecksum *a1 = NULL, *a2 = NULL, *response = NULL;
    const gchar *ha1 = NULL, *ha2 = NULL;
    gchar *hresponse = NULL;

    a1 = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (a1, TO_GUCHAR(username), strlen(username));
    g_checksum_update (a1, TO_GUCHAR(":"), 1);
    g_checksum_update (a1, TO_GUCHAR(realm), strlen(realm));
    g_checksum_update (a1, TO_GUCHAR(":"), 1);
    g_checksum_update (a1, TO_GUCHAR(secret), strlen(secret));

    if (g_strcmp0 (algo, "md5-sess") == 0) {
        GChecksum *a1_sess = NULL;
        a1_sess = g_checksum_new (G_CHECKSUM_MD5);
        ha1 = g_checksum_get_string (a1);
        g_checksum_update (a1_sess, TO_GUCHAR(ha1), strlen(ha1));
        g_checksum_update (a1_sess, TO_GUCHAR(":"), 1);
        g_checksum_update (a1_sess, TO_GUCHAR(nonce), strlen(nonce));
        g_checksum_update (a1_sess, TO_GUCHAR(":"), 1);
        g_checksum_update (a1_sess, TO_GUCHAR(cnonce), strlen(cnonce));
        g_checksum_free (a1);
        a1 = a1_sess;
    }

    a2 = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (a2, TO_GUCHAR(method), strlen(method));
    g_checksum_update (a2, TO_GUCHAR(":"), 1);
    g_checksum_update (a2, TO_GUCHAR(digest_uri), strlen(digest_uri));
    if (qop && g_strcmp0 (qop, "auth-int") == 0 && hentity) {
        g_checksum_update (a2, TO_GUCHAR(":"), 1);
        g_checksum_update (a2, TO_GUCHAR(hentity), strlen(hentity));
    }
    ha1 = g_checksum_get_string (a1);
    ha2 = g_checksum_get_string (a2);

    response = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (response, TO_GUCHAR(ha1), strlen(ha1));
    g_checksum_update (response, TO_GUCHAR(":"), 1);
    g_checksum_update (response, TO_GUCHAR(nonce), strlen(nonce));
    g_checksum_update (response, TO_GUCHAR(":"), 1);
    if (qop) {
        g_checksum_update (response, TO_GUCHAR(nonce_count),
                strlen(nonce_count));
        g_checksum_update (response, TO_GUCHAR(":"), 1);
        g_checksum_update (response, TO_GUCHAR(cnonce), strlen(cnonce));
        g_checksum_update (response, TO_GUCHAR(":"), 1);
        g_checksum_update (response, TO_GUCHAR(qop), strlen(qop));
        g_checksum_update (response, TO_GUCHAR(":"), 1);
    }
    g_checksum_update (response, TO_GUCHAR(ha2), strlen(ha2));
    hresponse = g_strdup(g_checksum_get_string (response));
    g_checksum_free (response);
    g_checksum_free (a2);
    g_checksum_free (a1);
    return hresponse;
}

static void
gsignond_digest_plugin_cancel (GSignondPlugin *self)
{
    GError* error = g_error_new(GSIGNOND_ERROR,
                                GSIGNOND_ERROR_SESSION_CANCELED,
                                "Session cancelled");
    gsignond_plugin_error (self, error);
    g_error_free(error);
}

static void
gsignond_digest_plugin_request (
    GSignondPlugin *self,
    GSignondSessionData *session_data)
{
}

/* difference with g_strcmp0() here is that two NULLs don't compare equal */
static gint
_compare_realm (gconstpointer a, gconstpointer b, gpointer user_data)
{
    const gchar *realm1 = (const gchar *) a;
    const gchar *realm2 = (const gchar *) b;
    (void) user_data;
    if (realm1 == NULL)
        return -1;
    if (realm2 == NULL)
        return 1;

    return g_strcmp0 (realm1, realm2);
}

static void
_gsignond_digest_plugin_return_digest (GSignondPlugin *plugin,
                                       const gchar *username,
                                       const gchar *secret,
                                       GSignondDictionary *session_data)
{
    g_return_if_fail (plugin != NULL);
    g_return_if_fail (GSIGNOND_IS_DIGEST_PLUGIN (plugin));

    GSignondSessionData *response = NULL;
    GSequenceIter *iter;
    GSequence* allowed_realms =
        gsignond_session_data_get_allowed_realms (session_data);
    const gchar* realm = gsignond_session_data_get_realm (session_data);
    const gchar* algo = gsignond_dictionary_get_string (session_data,
                "Algo");
    const gchar* nonce = gsignond_dictionary_get_string (session_data,
                "Nonce");
    const gchar* nonce_count = gsignond_dictionary_get_string (session_data,
                "NonceCount");
    const gchar* qop = gsignond_dictionary_get_string (session_data,
                "Qop");
    const gchar* method = gsignond_dictionary_get_string (session_data,
                "Method");
    const gchar* digest_uri = gsignond_dictionary_get_string (session_data,
                "DigestUri");
    const gchar* hentity = gsignond_dictionary_get_string (session_data,
                "HEntity");

    if (!allowed_realms) {
        GError* error = g_error_new (GSIGNOND_ERROR,
                                     GSIGNOND_ERROR_MISSING_DATA,
                                     "Missing realm list");
        gsignond_plugin_error (plugin, error);
        g_error_free (error);
        return;
    }
    iter = g_sequence_lookup (allowed_realms,
                              (gpointer) realm,
                              _compare_realm,
                              NULL);
    g_sequence_free (allowed_realms);
    if (!iter) {
        GError* error = g_error_new (GSIGNOND_ERROR,
                                     GSIGNOND_ERROR_NOT_AUTHORIZED,
                                     "Unauthorized realm");
        gsignond_plugin_error (plugin, error);
        g_error_free (error);
        return;
    }

    gchar *cnonce = gsignond_generate_nonce ();
    if (!cnonce) {
        GError* error = g_error_new (GSIGNOND_ERROR,
                                     GSIGNOND_ERROR_MISSING_DATA,
                                     "Error in generating nonce");
        gsignond_plugin_error (plugin, error);
        g_error_free (error);
        return;
    }

    if ((!realm || !algo  || !nonce  || !method  || !digest_uri)
        || (qop && g_strcmp0 (qop, "auth-int") == 0 && !hentity)
        || (qop && !nonce_count)) {
        GError* error = g_error_new (GSIGNOND_ERROR,
        		GSIGNOND_ERROR_MISSING_DATA, "Missing Session Data");
        gsignond_plugin_error (plugin, error);
        g_error_free (error);
        return;
    }
    gchar *digest = _gsignond_digest_plugin_compute_md5_digest(algo,
            username, realm, secret, nonce, nonce_count, cnonce, qop, method,
            digest_uri, hentity);

    response = gsignond_dictionary_new();
    gsignond_session_data_set_username(response, username);
    gsignond_dictionary_set_string(response, "CNonce", cnonce);
    g_free (cnonce);
    gsignond_dictionary_set_string(response, "Response", digest);
    g_free(digest);

    gsignond_plugin_response_final(plugin, response);
    gsignond_dictionary_unref(response);
}

static void
gsignond_digest_plugin_request_initial (
    GSignondPlugin *plugin,
    GSignondSessionData *session_data,
    GSignondDictionary *identity_method_cache,
    const gchar *mechanism)
{
    g_return_if_fail (plugin != NULL);
    g_return_if_fail (GSIGNOND_IS_DIGEST_PLUGIN (plugin));

    GSignondDigestPlugin *self = GSIGNOND_DIGEST_PLUGIN (plugin);
    GSignondDigestPluginPrivate *priv = self->priv;

    g_return_if_fail (priv != NULL);

    const gchar *username = gsignond_session_data_get_username(session_data);
    const gchar *secret = gsignond_session_data_get_secret(session_data);
    
    if (username != NULL && secret != NULL) {
        _gsignond_digest_plugin_return_digest(plugin, username, secret, session_data);
        return;
    }

    if (priv->session_data) {
        gsignond_dictionary_unref (priv->session_data);
        priv->session_data = NULL;
    }
    gsignond_dictionary_ref (session_data);
    priv->session_data = session_data;

    GSignondSignonuiData *user_action_data = gsignond_dictionary_new ();
    DATA_SET_VALUE (user_action_data, "Realm", 
                    gsignond_session_data_get_realm (session_data));
    DATA_SET_VALUE (user_action_data, "DigestUri", 
                    gsignond_dictionary_get_string (session_data, "DigestUri"));
    gsignond_signonui_data_set_query_username (user_action_data, TRUE);
    gsignond_signonui_data_set_query_password (user_action_data, TRUE);
    gsignond_plugin_user_action_required (plugin, user_action_data);
    gsignond_dictionary_unref (user_action_data);
}

static void
gsignond_digest_plugin_user_action_finished (
    GSignondPlugin *plugin,
    GSignondSignonuiData *signonui_data)
{
    g_return_if_fail (plugin != NULL);
    g_return_if_fail (GSIGNOND_IS_DIGEST_PLUGIN (plugin));

    GSignondDigestPlugin *self = GSIGNOND_DIGEST_PLUGIN (plugin);
    GSignondDigestPluginPrivate *priv = self->priv;
    g_return_if_fail (priv != NULL);

    GSignondSessionData *session_data = NULL;
    GSignondSignonuiError query_error;
    gboolean res = gsignond_signonui_data_get_query_error(signonui_data,
            &query_error);
    if (res == FALSE) {
        GError* error = g_error_new(GSIGNOND_ERROR,
                                GSIGNOND_ERROR_USER_INTERACTION,
                                "userActionFinished did not return an error value");
        gsignond_plugin_error (plugin, error);
        g_error_free(error);
    }

    const gchar* username = gsignond_signonui_data_get_username(signonui_data);
    const gchar* secret = gsignond_signonui_data_get_password(signonui_data);
    
    session_data = priv->session_data;

    if (query_error == SIGNONUI_ERROR_NONE &&
        username != NULL && 
        secret != NULL &&
        session_data != NULL) {
        _gsignond_digest_plugin_return_digest(plugin, username, secret, session_data);
    } else if (query_error == SIGNONUI_ERROR_CANCELED) {
        gsignond_digest_plugin_cancel (plugin);
    } else {
        GError* error = g_error_new(GSIGNOND_ERROR, 
                GSIGNOND_ERROR_USER_INTERACTION, "userActionFinished error: %d",
                query_error);
        gsignond_plugin_error (plugin, error);
        g_error_free(error);
    }
}

static void
gsignond_digest_plugin_refresh (
    GSignondPlugin *self, 
    GSignondSessionData *session_data)
{
    gsignond_plugin_refreshed(self, session_data);
}

static void
gsignond_plugin_interface_init (GSignondPluginInterface *iface)
{
    iface->cancel = gsignond_digest_plugin_cancel;
    iface->request_initial = gsignond_digest_plugin_request_initial;
    iface->request = gsignond_digest_plugin_request;
    iface->user_action_finished = gsignond_digest_plugin_user_action_finished;
    iface->refresh = gsignond_digest_plugin_refresh;
}

static void
gsignond_digest_plugin_init (GSignondDigestPlugin *self)
{
    GSignondDigestPluginPrivate *priv =
        GSIGNOND_DIGEST_PLUGIN_GET_PRIVATE (self);
    self->priv = priv;

    priv->session_data = NULL;

}

enum
{
    PROP_0,
    PROP_TYPE,
    PROP_MECHANISMS
};

static void
gsignond_digest_plugin_set_property (
        GObject      *object,
        guint         property_id,
        const GValue *value,
        GParamSpec   *pspec)
{
    switch (property_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gsignond_digest_plugin_get_property (
        GObject    *object,
        guint       prop_id,
        GValue     *value,
        GParamSpec *pspec)
{
    GSignondDigestPlugin *digest_plugin = GSIGNOND_DIGEST_PLUGIN (object);
    (void) digest_plugin;
    gchar *mechanisms[] = { "digest", NULL };
    
    switch (prop_id)
    {
        case PROP_TYPE:
            g_value_set_string (value, "digest");
            break;
        case PROP_MECHANISMS:
            g_value_set_boxed (value, mechanisms);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gsignond_digest_plugin_dispose (GObject *gobject)
{
    g_return_if_fail (GSIGNOND_IS_DIGEST_PLUGIN (gobject));
    GSignondDigestPlugin *self = GSIGNOND_DIGEST_PLUGIN (gobject);
    g_return_if_fail (self->priv != NULL);

    if (self->priv->session_data) {
        gsignond_dictionary_unref (self->priv->session_data);
        self->priv->session_data = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_digest_plugin_parent_class)->dispose (
            gobject);
}

static void
gsignond_digest_plugin_class_init (GSignondDigestPluginClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->set_property = gsignond_digest_plugin_set_property;
    gobject_class->get_property = gsignond_digest_plugin_get_property;
    gobject_class->dispose = gsignond_digest_plugin_dispose;
    
    g_object_class_override_property (gobject_class, PROP_TYPE, "type");
    g_object_class_override_property (gobject_class, PROP_MECHANISMS,
            "mechanisms");

    g_type_class_add_private (klass, sizeof (GSignondDigestPluginPrivate));
}
