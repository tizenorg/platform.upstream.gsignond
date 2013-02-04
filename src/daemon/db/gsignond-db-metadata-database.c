/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@linux.intel.com>
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
#include <string.h>
#include <sys/stat.h>

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-config.h"
#include "common/db/gsignond-db-error.h"
#include "gsignond-db-metadata-database.h"

#define GSIGNOND_DB_METADATA_DEFAULT_DB_FILENAME "metadata.db"

#define RETURN_IF_NOT_OPEN(obj, retval) \
    if (gsignond_db_sql_database_is_open (obj) == FALSE) { \
        GError* last_error = gsignond_db_create_error( \
                            GSIGNOND_DB_ERROR_NOT_OPEN,\
                            "DB Not Open"); \
        DBG("MetadataDB is not available"); \
        gsignond_db_sql_database_set_last_error(obj,\
                                                last_error); \
        return retval; \
    }

#define GSIGNOND_DB_METADATA_DATABASE_GET_PRIVATE(obj) \
                                          (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                           GSIGNOND_DB_TYPE_METADATA_DATABASE, \
                                           GSignondDbMetadataDatabasePrivate))

G_DEFINE_TYPE (GSignondDbMetadataDatabase, gsignond_db_metadata_database,
        GSIGNOND_DB_TYPE_SQL_DATABASE);

struct _GSignondDbMetadataDatabasePrivate
{
};

enum
{
    PROP_0,
    PROP_CONFIG,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void
_set_property (GObject *object, guint prop_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondDbMetadataDatabase *self = GSIGNOND_DB_METADATA_DATABASE (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_assert (self->config == NULL);
            self->config = g_value_dup_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GSignondDbMetadataDatabase *self = GSIGNOND_DB_METADATA_DATABASE (object);

    switch (prop_id) {
        case PROP_CONFIG:
            g_value_set_object (value, self->config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
_gsignond_db_metadata_database_open (
        GSignondDbSqlDatabase *obj,
        const gchar *filename,
        int flags);

static gboolean
_gsignond_db_metadata_database_create (
        GSignondDbSqlDatabase *obj);

static gboolean
_gsignond_db_metadata_database_clear (
        GSignondDbSqlDatabase *obj);

/*!
 * @enum GSignondIdentityFlags
 * Flags for the identity to be stored into db
 */
enum GSignondIdentityFlags {
    GSignondIdentityFlag_Validated = 0x0001,
    GSignondIdentityFlag_RememberSecret = 0x0002,
    GSignondIdentityFlag_UserNameIsSecret = 0x0004,
};

static gint
_compare_strings (
		const gchar* a,
		const gchar* b,
		gpointer data)
{
	(void)data;
	return g_strcmp0 (a,b);
}

static GSequence *
_gsignond_db_metadata_database_list_to_sequence (GList *list)
{
    GSequence *seq = NULL;
    seq = g_sequence_new ((GDestroyNotify)g_free);
    list = g_list_first (list);
    for ( ; list != NULL; list = g_list_next (list)) {
        g_sequence_insert_sorted (seq, (gchar *) list->data,
        		(GCompareDataFunc)_compare_strings, NULL);
    }
    return seq;
}

static gboolean
_gsignond_db_metadata_database_read_identity (
        sqlite3_stmt *stmt,
        GSignondIdentityInfo *identity)
{
    gint flags = 0;

    gsignond_identity_info_set_caption (identity,
            (const gchar *)sqlite3_column_text (stmt, 0));

    flags = sqlite3_column_int (stmt, 2);

    gsignond_identity_info_set_username_secret (identity,
          flags & GSignondIdentityFlag_UserNameIsSecret);

    if (flags & GSignondIdentityFlag_UserNameIsSecret) {
        gsignond_identity_info_set_caption (identity, "");
    } else {
        gsignond_identity_info_set_username (identity,
                    (const gchar *)sqlite3_column_text (stmt, 1));
    }

    gsignond_identity_info_set_store_secret (identity,
            flags & GSignondIdentityFlag_RememberSecret);

    gsignond_identity_info_set_validated (identity,
            flags & GSignondIdentityFlag_Validated);

    gsignond_identity_info_set_identity_type (identity,
            sqlite3_column_int (stmt, 3));

    return TRUE;
}

static gboolean
_gsignond_db_metadata_database_exec (
        GSignondDbMetadataDatabase *self,
        const gchar *query_format,
        ...)
{
    gboolean ret = FALSE;
    gchar *query = NULL;
    va_list args;

    g_return_val_if_fail (query_format != NULL, FALSE);

    va_start (args, query_format);
    query = sqlite3_vmprintf (query_format, args);
    va_end (args);

    ret = gsignond_db_sql_database_exec (
                    GSIGNOND_DB_SQL_DATABASE (self),
                    query);
    sqlite3_free (query);

    return ret;
}


static GSequence *
_gsignond_db_metadata_database_get_sequence (
        GSignondDbMetadataDatabase *self,
        const gchar *query_format,
        ...)
{
    GSequence *seq = NULL;
    GList *list = NULL;
    gchar *query = NULL;
    va_list args;

    g_return_val_if_fail (query_format != NULL, NULL);

    va_start (args, query_format);
    query = sqlite3_vmprintf (query_format, args);
    va_end (args);

    list = gsignond_db_sql_database_query_exec_string_list (
                    GSIGNOND_DB_SQL_DATABASE (self),
                    query);
    sqlite3_free (query);
    seq = _gsignond_db_metadata_database_list_to_sequence (list);
    g_list_free (list); /*list elements are owned by sequence*/

    return seq;
}

guint32
_gsignond_db_metadata_database_update_credentials (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity)
{
    gchar *query = NULL;
    gint flags = 0;
    guint32 type;
    gint id;
    const gchar *caption= NULL, *username = NULL;
    gboolean ret = FALSE;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), 0);
    g_return_val_if_fail (identity != NULL, 0);

    if (gsignond_identity_info_get_validated (identity) )
        flags |= GSignondIdentityFlag_Validated;
    if (gsignond_identity_info_get_store_secret (identity) )
        flags |= GSignondIdentityFlag_RememberSecret;
    if (gsignond_identity_info_get_is_username_secret (identity) ) {
        flags |= GSignondIdentityFlag_UserNameIsSecret;
        username = "";
    } else {
        username = gsignond_identity_info_get_username (identity);
    }
    caption = gsignond_identity_info_get_caption (identity);
    if (!caption || !username)
        return FALSE;

    id = gsignond_identity_info_get_id (identity);
    type = gsignond_identity_info_get_identity_type (identity);
    if (!gsignond_identity_info_get_is_identity_new (identity)) {
        query = sqlite3_mprintf ("UPDATE CREDENTIALS SET caption = %Q, "
                "username = %Q, flags = %u, type = %u WHERE id = %u;",
                caption, username, flags, type, id);
    } else {
        query = sqlite3_mprintf ("INSERT INTO CREDENTIALS "
                "(caption, username, flags, type) "
                "VALUES(%Q, %Q, %u, %u);",
                 caption, username, flags, type);
    }
    ret = gsignond_db_sql_database_exec (
                GSIGNOND_DB_SQL_DATABASE (self),
                query);
    sqlite3_free (query);
    if (!ret) {
        return 0;
    }

    if (gsignond_identity_info_get_is_identity_new (identity)) {
        id = gsignond_db_sql_database_get_last_insert_rowid (
                    GSIGNOND_DB_SQL_DATABASE (self));
    }
    return (guint32)id;
}

gboolean
_gsignond_db_metadata_database_update_realms (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity,
        guint32 id,
        GSequence *realms)
{
    GSequenceIter *iter = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    g_return_val_if_fail (identity != NULL, FALSE);

    if (!realms) {
        DBG ("NULL realms cannot be updated");
        return FALSE;
    }

    if (!gsignond_identity_info_get_is_identity_new (identity)) {
        /* remove realms list */
        DBG ("Remove old realms from DB as identity is not new");
        _gsignond_db_metadata_database_exec (self,
                "DELETE FROM REALMS WHERE identity_id = %u;", id);
    }

    /* realms insert */
    iter = g_sequence_get_begin_iter (realms);
    while (!g_sequence_iter_is_end (iter)) {
        if (!_gsignond_db_metadata_database_exec (self,
                "INSERT OR IGNORE INTO REALMS (identity_id, realm) "
                "VALUES (%u, %Q);", id, (const gchar *)g_sequence_get (iter))) {
            DBG ("Insert realms to DB failed");
            return FALSE;
        }
        iter = g_sequence_iter_next (iter);
    }

    return TRUE;
}

gboolean
_gsignond_db_metadata_database_insert_methods (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity,
        GHashTable *methods)
{
    gchar *query = NULL;
    gboolean ret = FALSE;
    GSequenceIter *mech_iter = NULL;
    GHashTableIter method_iter;
    const gchar *method = NULL;
    GSequence *mechanisms = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    g_return_val_if_fail (identity != NULL, FALSE);

    methods = gsignond_identity_info_get_methods (identity);

    if (!methods || g_hash_table_size (methods) <=0) {
        return FALSE;
    }

    g_hash_table_iter_init (&method_iter, methods);
    while (g_hash_table_iter_next (&method_iter,
                                   (gpointer)&method,
                                   (gpointer)&mechanisms))
    {
        if (!_gsignond_db_metadata_database_exec ( self,
                "INSERT OR IGNORE INTO METHODS (method) "
                "VALUES( %Q );",
                method)) {
            DBG ("Insert methods to DB failed");
            return FALSE;
        }
        /* mechanisms insert */
        mech_iter = g_sequence_get_begin_iter (mechanisms);
        while (!g_sequence_iter_is_end (mech_iter)) {
            if (!_gsignond_db_metadata_database_exec (self,
                    "INSERT OR IGNORE INTO MECHANISMS (mechanism) "
                    "VALUES(%Q);",
                    g_sequence_get (mech_iter))) {
                DBG ("Insert mechanisms to DB failed");
                return FALSE;
            }
            mech_iter = g_sequence_iter_next (mech_iter);
        }
    }

    return TRUE;
}

gboolean
_gsignond_db_metadata_database_update_acl (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity,
        GSignondSecurityContextList *acl)
{
    gchar *query = NULL;
    gboolean ret = FALSE;
    GSignondSecurityContextList *list = NULL;
    GSignondSecurityContext *ctx = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    g_return_val_if_fail (identity != NULL, FALSE);

    for (list = acl;  list != NULL; list = g_list_next (list)) {
        ctx = (GSignondSecurityContext *) list->data;
        _gsignond_db_metadata_database_exec (self,
                "INSERT OR IGNORE INTO SECCTX (sysctx, appctx) "
                "VALUES (%Q, %Q);",
                ctx->sys_ctx, ctx->app_ctx);
    }
    return TRUE;
}

gboolean
_gsignond_db_metadata_database_update_owners (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity,
        GSignondSecurityContextList *owners)
{
    gchar *query = NULL;
    gboolean ret = FALSE;
    GSignondSecurityContextList *list = NULL;
    GSignondSecurityContext *ctx = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    g_return_val_if_fail (identity != NULL, FALSE);

    for (list = owners;  list != NULL; list = g_list_next (list)) {
        ctx = (GSignondSecurityContext *) list->data;
        if (ctx->sys_ctx && strlen (ctx->sys_ctx) > 0) {
            _gsignond_db_metadata_database_exec (self,
                    "INSERT OR IGNORE INTO "
                    "SECCTX (sysctx, appctx) "
                    "VALUES (%Q, %Q);",
                    ctx->sys_ctx, ctx->app_ctx);
        }
    }

    return TRUE;
}

static void
_gsignond_db_metadata_database_dispose (GObject *gobject)
{
    g_return_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (gobject));
    GSignondDbMetadataDatabase *self = GSIGNOND_DB_METADATA_DATABASE (gobject);

    if (self->config) {
        g_object_unref (self->config);
        self->config = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_db_metadata_database_parent_class)->dispose (
            gobject);
}

static void
gsignond_db_metadata_database_class_init (
        GSignondDbMetadataDatabaseClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = _set_property;
    gobject_class->get_property = _get_property;
    gobject_class->dispose = _gsignond_db_metadata_database_dispose;

    properties[PROP_CONFIG] = g_param_spec_object ("config",
                                                   "config",
                                                   "Configuration object",
                                                   GSIGNOND_TYPE_CONFIG,
                                                   G_PARAM_CONSTRUCT_ONLY |
                                                   G_PARAM_READWRITE);
    g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);

    GSignondDbSqlDatabaseClass *sql_class =
            GSIGNOND_DB_SQL_DATABASE_CLASS (klass);

    sql_class->create = _gsignond_db_metadata_database_create;
    sql_class->clear = _gsignond_db_metadata_database_clear;

}

static void
gsignond_db_metadata_database_init (
        GSignondDbMetadataDatabase *self)
{
    self->config = NULL;
}

/**
 * gsignond_db_metadata_database_new:
 *
 * @config: (transfer none) #GSignondConfig config data
 *
 * Creates new #GSignondDbMetadataDatabase object
 *
 * Returns : (transfer full) the #GSignondDbMetadataDatabase object
 */
GSignondDbMetadataDatabase *
gsignond_db_metadata_database_new (GSignondConfig *config)
{
    return GSIGNOND_DB_METADATA_DATABASE (
            g_object_new (GSIGNOND_DB_TYPE_METADATA_DATABASE,
                          "config", config, NULL));
}

static gboolean
_gsignond_db_metadata_database_open (
        GSignondDbSqlDatabase *obj,
        const gchar *filename,
        int flags)
{
    const gchar *dir = NULL;
    gchar *db_dir = NULL;
    gchar *db_filename = NULL;
    gboolean ret = FALSE;
    gint dir_created = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (obj), FALSE);

    if (!filename || strlen (filename) <= 0) {
        filename = GSIGNOND_DB_METADATA_DEFAULT_DB_FILENAME;
    }
    dir = g_get_user_data_dir ();
    if (!dir) {
        DBG ("Invalid Metadata DB directory");
        return FALSE;
    }
    db_filename = g_build_filename (dir, "gsignond", filename, NULL);
    if (!db_filename) {
        DBG ("Invalid Metadata DB filename");
        return FALSE;
    }

    db_dir = g_path_get_dirname (db_filename);
    dir_created = g_mkdir_with_parents (db_dir, S_IRWXU);
    g_free (db_dir);
    if (dir_created != 0) {
        DBG ("Metadata DB directory does not exist");
        g_free (db_filename);
        return FALSE;
    }

    ret = gsignond_db_sql_database_open (obj, db_filename, flags);
    g_free (db_filename);
    return ret;
}

static gboolean
_gsignond_db_metadata_database_create (
        GSignondDbSqlDatabase *obj)
{
    const gchar *queries = NULL;
    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (obj), FALSE);
    RETURN_IF_NOT_OPEN (obj, FALSE);

    if (gsignond_db_sql_database_get_db_version(obj,
            "PRAGMA user_version;") > 0) {
        DBG ("Metadata DB is already created");
        return TRUE;
    }

    queries = ""
            "CREATE TABLE CREDENTIALS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "caption TEXT,"
            "username TEXT,"
            "flags INTEGER,"
            "type INTEGER);"

            "CREATE TABLE METHODS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "method TEXT UNIQUE);"

            "CREATE TABLE MECHANISMS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "mechanism TEXT UNIQUE);"

            "CREATE TABLE SECCTX"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "sysctx TEXT,"
            "appctx TEXT,"
            "CONSTRAINT tokc UNIQUE(sysctx, appctx) ON CONFLICT REPLACE);"

            "CREATE INDEX sysidx ON SECCTX(sysctx);"
            "CREATE INDEX appidx ON SECCTX(appctx);"

            "CREATE TABLE REALMS"
            "(identity_id INTEGER CONSTRAINT fk_identity_id "
            "REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "realm TEXT,"
            "hostname TEXT,"
            "PRIMARY KEY (identity_id, realm, hostname));"

            "CREATE TABLE ACL"
            "(rowid INTEGER PRIMARY KEY AUTOINCREMENT,"
            "identity_id INTEGER CONSTRAINT fk_identity_id "
            "REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "method_id INTEGER CONSTRAINT fk_method_id "
            "REFERENCES METHODS(id) ON DELETE CASCADE,"
            "mechanism_id INTEGER CONSTRAINT fk_mechanism_id "
            "REFERENCES MECHANISMS(id) ON DELETE CASCADE,"
            "secctx_id INTEGER CONSTRAINT fk_secctx_id REFERENCES "
            "SECCTX(id) ON DELETE CASCADE);"

            "CREATE TABLE REFS"
            "(identity_id INTEGER CONSTRAINT fk_identity_id "
            "REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "secctx_id INTEGER CONSTRAINT fk_secctx_id REFERENCES "
            "SECCTX(id) ON DELETE CASCADE,"
            "ref TEXT,"
            "PRIMARY KEY (identity_id, secctx_id, ref));"

            /*
             * triggers generated with
             * http:/*www.rcs-comp.com/site/index.php/view/Utilities-
             * SQLite_foreign_key_trigger_generator
             */
            /* insert triggers to force foreign keys support */
            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_REALMS_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [REALMS] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table REALMS violates foreign "
            "key constraint fki_REALMS_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM "
            "CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_REALMS_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [REALMS] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table REALMS violates "
            "foreign key constraint fku_REALMS_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM "
            "CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_REALMS_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM REALMS WHERE REALMS.identity_id = OLD.id; "
            "END; "

            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_ACL_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates "
            "foreign key constraint fki_ACL_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM "
            "CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END;"

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_ACL_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates "
            "foreign key constraint fku_ACL_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM "
            "CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_ACL_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
             "   DELETE FROM ACL WHERE ACL.identity_id = OLD.id; "
            "END; "

            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_ACL_method_id_METHODS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates "
            "foreign key constraint fki_ACL_method_id_METHODS_id') "
            "  WHERE NEW.method_id IS NOT NULL AND (SELECT id FROM "
            "METHODS WHERE id = NEW.method_id) IS NULL; "
            "END; "

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_ACL_method_id_METHODS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates "
            "foreign key constraint fku_ACL_method_id_METHODS_id') "
            "      WHERE NEW.method_id IS NOT NULL AND (SELECT id FROM "
            "METHODS WHERE id = NEW.method_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_ACL_method_id_METHODS_id "
            "BEFORE DELETE ON METHODS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM ACL WHERE ACL.method_id = OLD.id; "
            "END; "

            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_ACL_mechanism_id_MECHANISMS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates foreign "
            "key constraint fki_ACL_mechanism_id_MECHANISMS_id') "
            "  WHERE NEW.mechanism_id IS NOT NULL AND (SELECT id FROM "
            "MECHANISMS WHERE id = NEW.mechanism_id) IS NULL; "
            "END; "

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_ACL_mechanism_id_MECHANISMS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates foreign "
            "key constraint fku_ACL_mechanism_id_MECHANISMS_id') "
            "      WHERE NEW.mechanism_id IS NOT NULL AND (SELECT id FROM "
            "MECHANISMS WHERE id = NEW.mechanism_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_ACL_mechanism_id_MECHANISMS_id "
            "BEFORE DELETE ON MECHANISMS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM ACL WHERE ACL.mechanism_id = OLD.id; "
            "END; "

            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_ACL_secctx_id_SECCTX_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates foreign "
            "key constraint fki_ACL_secctx_id_SECCTX_id') "
            "  WHERE NEW.secctx_id IS NOT NULL AND (SELECT id FROM SECCTX "
            "WHERE id = NEW.secctx_id) IS NULL; "
            "END; "

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_ACL_secctx_id_SECCTX_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates foreign "
            "key constraint fku_ACL_secctx_id_SECCTX_id') "
            "      WHERE NEW.secctx_id IS NOT NULL AND (SELECT id FROM SECCTX "
            "WHERE id = NEW.secctx_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_ACL_secctx_id_SECCTX_id "
            "BEFORE DELETE ON SECCTX "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM ACL WHERE ACL.secctx_id = OLD.id; "
            "END; "

            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_REFS_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [REFS] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table REFS violates foreign "
            "key constraint fki_REFS_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM "
            "CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_REFS_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [REFS] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table REFS violates foreign "
            "key constraint fku_REFS_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM "
            "CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_REFS_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM REFS WHERE REFS.identity_id = OLD.id; "
            "END; "

            /* Foreign Key Preventing insert */
            "CREATE TRIGGER fki_REFS_secctx_id_SECCTX_id "
            "BEFORE INSERT ON [REFS] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table REFS violates foreign "
            "key constraint fki_REFS_secctx_id_SECCTX_id') "
            "  WHERE NEW.secctx_id IS NOT NULL AND (SELECT id FROM SECCTX "
            "WHERE id = NEW.secctx_id) IS NULL; "
            "END; "

            /* Foreign key preventing update */
            "CREATE TRIGGER fku_REFS_secctx_id_SECCTX_id "
            "BEFORE UPDATE ON [REFS] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table REFS violates "
            "foreign key constraint fku_REFS_secctx_id_SECCTX_id') "
            "      WHERE NEW.secctx_id IS NOT NULL AND (SELECT id FROM "
            "SECCTX WHERE id = NEW.secctx_id) IS NULL; "
            "END; "

            /* Cascading Delete */
            "CREATE TRIGGER fkdc_REFS_secctx_id_SECCTX_id "
            "BEFORE DELETE ON SECCTX "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM REFS WHERE REFS.secctx_id = OLD.id; "
            "END; "

            //create OWNER table
            "CREATE TABLE OWNER"
            "(rowid INTEGER PRIMARY KEY AUTOINCREMENT,"
            "identity_id INTEGER CONSTRAINT fk_identity_id "
            "REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "secctx_id INTEGER CONSTRAINT fk_secctx_id REFERENCES SECCTX(id) "
            "ON DELETE CASCADE);"

            //added triggers for OWNER
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_OWNER_secctx_id_SECCTX_id "
            "BEFORE INSERT ON [OWNER] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table OWNER violates "
            "foreign key constraint fki_OWNER_secctx_id_SECCTX_id') "
            "  WHERE NEW.secctx_id IS NOT NULL AND (SELECT id FROM SECCTX "
            "WHERE id = NEW.secctx_id) IS NULL; "
            "END; "
            // Foreign key preventing update
            "CREATE TRIGGER fku_OWNER_secctx_id_SECCTX_id "
            "BEFORE UPDATE ON [OWNER] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table OWNER violates "
            "foreign key constraint fku_OWNER_secctx_id_SECCTX_id') "
            "      WHERE NEW.secctx_id IS NOT NULL AND (SELECT id FROM SECCTX "
            "WHERE id = NEW.secctx_id) IS NULL; "
            "END; "
            // Cascading Delete
            "CREATE TRIGGER fkdc_OWNER_secctx_id_SECCTX_id "
            "BEFORE DELETE ON SECCTX "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM OWNER WHERE OWNER.secctx_id = OLD.id; "
            "END; "

            "PRAGMA user_version = 1;";

    return gsignond_db_sql_database_transaction_exec (obj, queries);
}

static gboolean
_gsignond_db_metadata_database_clear (
        GSignondDbSqlDatabase *obj)
{
    const gchar *queries = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (obj), FALSE);
    RETURN_IF_NOT_OPEN (obj, FALSE);

    queries = ""
            "DELETE FROM CREDENTIALS;"
            "DELETE FROM METHODS;"
            "DELETE FROM MECHANISMS;"
            "DELETE FROM ACL;"
            "DELETE FROM REALMS;"
            "DELETE FROM SECCTX;"
            "DELETE FROM OWNER;";

    return gsignond_db_sql_database_transaction_exec (obj, queries);
}

/**
 * gsignond_db_metadata_database_open:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 *
 * Opens a connection to DB.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gboolean
gsignond_db_metadata_database_open (GSignondDbMetadataDatabase *self)
{
    const gchar *filename = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);

    if (gsignond_db_sql_database_is_open (GSIGNOND_DB_SQL_DATABASE (self)))
        return TRUE;

    filename = gsignond_config_get_string (self->config,
            GSIGNOND_CONFIG_DB_METADATA_DB_FILENAME);

    return _gsignond_db_metadata_database_open (
            GSIGNOND_DB_SQL_DATABASE (self), filename,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
}

/**
 * gsignond_db_metadata_database_insert_method:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @method: the method to be inserted
 * @method_id: (transfer none) id of the method inserted
 *
 * Inserts the method into the db.
 *
 * Returns: TRUE if successful, FALSE otherwise
 */
gboolean
gsignond_db_metadata_database_insert_method (
        GSignondDbMetadataDatabase *self,
        const gchar *method,
        guint32 *method_id)
{
    gchar *query = NULL;
    gboolean ret = FALSE;
    *method_id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    g_return_val_if_fail (method != NULL, FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), FALSE);

    query = sqlite3_mprintf ("INSERT INTO METHODS (method) "
                             "VALUES (%Q);",
                             method);
    ret = gsignond_db_sql_database_transaction_exec (
            GSIGNOND_DB_SQL_DATABASE (self), query);
    sqlite3_free (query);
    if (ret) {
        DBG ("Retrieve method id for the inserted method");
        *method_id = gsignond_db_metadata_database_get_method_id (self, method);
    }
    return ret;
}

/**
 * gsignond_db_metadata_database_get_method_id:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @method: the method to be fetched
 *
 * Fetches the id of the specified method.
 *
 * Returns: the method if successful, 0 otherwise
 */
guint32
gsignond_db_metadata_database_get_method_id (
        GSignondDbMetadataDatabase *self,
        const gchar *method)
{
    gchar *query = NULL;
    gboolean ret = FALSE;
    guint32 method_id = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    g_return_val_if_fail (method != NULL, FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), method_id);

    query = sqlite3_mprintf ("SELECT id FROM METHODS "
                             "WHERE method = %Q;",
                             method);
    ret = gsignond_db_sql_database_query_exec_int (
                GSIGNOND_DB_SQL_DATABASE (self),
                query,
                &method_id);
    sqlite3_free (query);

    return method_id;
}

/**
 * gsignond_db_metadata_database_get_methods:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity
 * @sec_ctx: the security context
 *
 * Fetches the list of the methods with the specified identity id.
 *
 * Returns: (transfer full) the list if successful, NULL otherwise.
 * When done list should be freed with g_list_free_full (list, g_free)
 */
GList *
gsignond_db_metadata_database_get_methods (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        GSignondSecurityContext* sec_ctx)
{
    gchar *query = NULL;
    GList *methods = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), NULL);
    g_return_val_if_fail (sec_ctx != NULL, NULL);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), NULL);

    if (sec_ctx->sys_ctx && strlen (sec_ctx->sys_ctx) <= 0) {
        query = sqlite3_mprintf ("SELECT DISTINCT METHODS.method FROM "
                    "( ACL JOIN METHODS ON ACL.method_id = METHODS.id ) "
                    "WHERE ACL.identity_id = %u;",
                    identity_id);
    } else {
        query = sqlite3_mprintf ("SELECT DISTINCT METHODS.method FROM "
                "( ACL JOIN METHODS ON ACL.method_id = METHODS.id ) "
                "WHERE ACL.identity_id = %u AND ACL.secctx_id = "
                "(SELECT id FROM SECCTX "
                "WHERE sysctx = %Q AND appctx = %Q);",
                identity_id, sec_ctx->sys_ctx, sec_ctx->app_ctx);
    }

    methods = gsignond_db_sql_database_query_exec_string_list (
                    GSIGNOND_DB_SQL_DATABASE (self),
                    query);
    sqlite3_free (query);

    return methods;
}

/**
 * gsignond_db_metadata_database_update_identity:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity: the identity #GSignondIdentityInfo object
 *
 * Updates the database with the data in the identity.
 *
 * Returns: the id of the identity if successful, 0 otherwise
 */
guint32
gsignond_db_metadata_database_update_identity (
        GSignondDbMetadataDatabase *self,
        GSignondIdentityInfo *identity)
{
    GSignondDbSqlDatabase *sql = NULL;
    guint32 id = 0;
    guint32 ret = 0;
    gchar * query = NULL;
    GHashTable *methods = NULL;
    GSequence *realms = NULL;
    GSignondSecurityContextList *acl = NULL, *owners = NULL, *list = NULL;
    GHashTableIter method_iter;
    const gchar *method = NULL;
    GSequence *mechanisms = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), 0);
    g_return_val_if_fail (identity != NULL, 0);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), id);

    sql = GSIGNOND_DB_SQL_DATABASE (self);
    if (!gsignond_db_sql_database_start_transaction (sql)) {
        return 0;
    }

    /* credentials */
    id = _gsignond_db_metadata_database_update_credentials (self, identity);
    if (id == 0) {
        DBG ("Update credentials failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        return 0;
    }

    /* methods */
    methods = gsignond_identity_info_get_methods (identity);
    if (!_gsignond_db_metadata_database_insert_methods (self, identity,
            methods)) {
        DBG ("Update methods failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        goto finished;
    }

    /* realms */
    realms = gsignond_identity_info_get_realms (identity);
    if (!_gsignond_db_metadata_database_update_realms (self,
            identity, id, realms)) {
        DBG ("Update realms failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        goto finished;
    }

    /* acl */
    acl = gsignond_identity_info_get_access_control_list (identity);
    if (!_gsignond_db_metadata_database_update_acl (self, identity, acl)) {
        DBG ("Update acl failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        goto finished;
    }

    /* owners */
    owners = gsignond_identity_info_get_owner_list (identity);
    if (!_gsignond_db_metadata_database_update_owners (self, identity, owners)){
        DBG ("Update owner list failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        goto finished;
    }

    if (!gsignond_identity_info_get_is_identity_new (identity)) {
        DBG ("Remove old acl and owner list as identity is not new");
        /* remove acl */
        _gsignond_db_metadata_database_exec (self,
                "DELETE FROM ACL WHERE identity_id = %u;", id);

        /* remove owner */
        _gsignond_db_metadata_database_exec (self,
                "DELETE FROM OWNER WHERE identity_id = %u;", id);
    }

    /* ACL insert, this will do basically identity level ACL */
    g_hash_table_iter_init (&method_iter, methods);
    while (g_hash_table_iter_next (&method_iter, (gpointer)&method,
            (gpointer)&mechanisms)) {

        if (g_list_length (acl) > 0) {
            for (list = acl;  list != NULL; list = g_list_next (list)) {
                GSequenceIter *mech_iter = NULL;
                GSignondSecurityContext *ctx = NULL;

                ctx = (GSignondSecurityContext *) list->data;
                mech_iter = g_sequence_get_begin_iter (mechanisms);
                while (!g_sequence_iter_is_end (mech_iter)) {
                    _gsignond_db_metadata_database_exec (self,
                            "INSERT OR REPLACE INTO ACL "
                            "(identity_id, method_id, mechanism_id, secctx_id) "
                            "VALUES ( %u, "
                            "( SELECT id FROM METHODS WHERE method = %Q ),"
                            "( SELECT id FROM MECHANISMS WHERE mechanism= %Q ),"
                            " ( SELECT id FROM SECCTX WHERE sysctx = %Q "
                            "AND appctx = %Q));",
                            id, method, g_sequence_get (mech_iter),
                            ctx->sys_ctx, ctx->app_ctx);
                    mech_iter = g_sequence_iter_next (mech_iter);
                }
                if (g_sequence_get_length (mechanisms) <= 0) {
                    _gsignond_db_metadata_database_exec (self,
                            "INSERT OR REPLACE INTO ACL "
                            "(identity_id, method_id, secctx_id) "
                            "VALUES ( %u, "
                            "( SELECT id FROM METHODS WHERE method = %Q),"
                            "( SELECT id FROM SECCTX WHERE sysctx = %Q AND "
                            "appctx = %Q ));",
                            id, method, ctx->sys_ctx, ctx->app_ctx);
                }
            }

        } else {
            GSequenceIter *mech_iter = NULL;
            mech_iter = g_sequence_get_begin_iter (mechanisms);
            while (!g_sequence_iter_is_end (mech_iter)) {
                _gsignond_db_metadata_database_exec (self,
                        "INSERT OR REPLACE INTO ACL "
                        "(identity_id, method_id, mechanism_id) "
                        "VALUES ( %u, "
                        "( SELECT id FROM METHODS WHERE method = %Q ),"
                        "( SELECT id FROM MECHANISMS WHERE mechanism= %Q ));",
                        id, method, g_sequence_get (mech_iter));
                mech_iter = g_sequence_iter_next (mech_iter);
            }
            if (g_sequence_get_length (mechanisms) <= 0) {
                _gsignond_db_metadata_database_exec (self,
                        "INSERT OR REPLACE INTO ACL (identity_id, method_id) "
                        "VALUES ( %u, "
                        "( SELECT id FROM METHODS WHERE method = %Q ));",
                        id, method );
            }
        }
    }
    /* insert acl in case where methods are missing */
    if (g_hash_table_size (methods) <= 0) {
        for (list = acl;  list != NULL; list = g_list_next (list)) {
            GSignondSecurityContext *ctx = NULL;

            ctx = (GSignondSecurityContext *) list->data;
            _gsignond_db_metadata_database_exec (self,
                    "INSERT OR REPLACE INTO ACL "
                    "(identity_id, secctx_id) "
                    "VALUES ( %u, "
                    "( SELECT id FROM SECCTX WHERE sysctx = %Q AND "
                    "appctx = %Q));",
                    id, ctx->sys_ctx, ctx->app_ctx);
        }
    }

    /* insert owner list */
    for (list = owners;  list != NULL; list = g_list_next (list)) {
        GSignondSecurityContext *ctx = NULL;
        ctx = (GSignondSecurityContext *) list->data;
        _gsignond_db_metadata_database_exec (self,
                "INSERT OR REPLACE INTO OWNER "
                "(identity_id, secctx_id) "
                "VALUES ( %u, "
                "( SELECT id FROM SECCTX WHERE sysctx = %Q AND appctx = %Q ));",
                id, ctx->sys_ctx, ctx->app_ctx);
    }

    if (gsignond_db_sql_database_commit_transaction (sql)) {
        DBG ("Identity updated");
        ret = id;
    }

finished:
    if (methods) g_hash_table_unref (methods);
    if (realms) g_sequence_free (realms);
    if (acl) gsignond_security_context_list_free (acl);
    if (owners) gsignond_security_context_list_free (owners);

    return ret;
}

/**
 * gsignond_db_metadata_database_update_identity:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity
 *
 * Reads the identity data from the database based on the given id.
 *
 * Returns: (transfer full) the #GSignondIdentityInfo identity if successful,
 * NULL otherwise.
 */
GSignondIdentityInfo *
gsignond_db_metadata_database_get_identity (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id)
{
    GSignondIdentityInfo *identity = NULL;
    gchar *query = NULL;
    gint rows = 0, i;
    GSequence *realms = NULL, *mechanisms = NULL;
    GHashTable *methods = NULL, *tuples = NULL;
    GHashTableIter iter;
    gchar *method = NULL;
    gint method_id = 0;
    GSignondSecurityContextList *acl = NULL, *owners = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), NULL);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), NULL);

    identity = gsignond_identity_info_new ();
    query = sqlite3_mprintf ("SELECT caption, username, flags, type "
                             "FROM credentials WHERE id = %u;",
                             identity_id);
    rows = gsignond_db_sql_database_query_exec (GSIGNOND_DB_SQL_DATABASE (self),
            query, (GSignondDbSqlDatabaseQueryCallback)
            _gsignond_db_metadata_database_read_identity,
            identity);
    sqlite3_free (query);
    if (G_UNLIKELY (rows <= 0)) {
        DBG ("Fetch credentials failed");
        gsignond_identity_info_free (identity);
        return NULL;
    }
    gsignond_identity_info_set_id (identity, identity_id);

    /*realms*/
    realms = _gsignond_db_metadata_database_get_sequence (self,
            "SELECT realm FROM REALMS "
            "WHERE identity_id = %u;",
            identity_id);
    if (realms) {
        gsignond_identity_info_set_realms (identity, realms);
        g_sequence_free (realms);
    }

    /*acl*/
    acl = gsignond_db_metadata_database_get_accesscontrol_list (self,
            identity_id);
    if (acl) {
        gsignond_identity_info_set_access_control_list (identity, acl);
        gsignond_security_context_list_free (acl);
    }

    /*owners*/
    owners = gsignond_db_metadata_database_get_owner_list (self,
            identity_id);
    if (owners) {
        gsignond_identity_info_set_owner_list (identity, owners);
        gsignond_security_context_list_free (owners);
    }

    /*methods*/
    query = sqlite3_mprintf ("SELECT DISTINCT ACL.method_id, METHODS.method "
            "FROM ( ACL JOIN METHODS ON ACL.method_id = METHODS.id ) "
            "WHERE ACL.identity_id = %u;",
            identity_id);
    tuples = gsignond_db_sql_database_query_exec_int_string_tuple (
                    GSIGNOND_DB_SQL_DATABASE (self),
                    query);
    sqlite3_free (query);

    if (tuples) {
        methods = g_hash_table_new_full ((GHashFunc)g_str_hash,
                (GEqualFunc)g_str_equal,
                (GDestroyNotify)g_free,
                (GDestroyNotify)g_sequence_free);
        g_hash_table_iter_init(&iter, tuples);
        while (g_hash_table_iter_next (&iter, (gpointer *)&method_id,
                (gpointer *)&method)) {
            /*mechanisms*/
            mechanisms = _gsignond_db_metadata_database_get_sequence (self,
                    "SELECT DISTINCT MECHANISMS.mechanism FROM "
                    "( MECHANISMS JOIN ACL ON ACL.mechanism_id = MECHANISMS.id ) "
                    "WHERE ACL.method_id = %u AND ACL.identity_id = %u;",
                    method_id, identity_id);
            g_hash_table_insert(methods, g_strdup(method), mechanisms);
        }
        g_hash_table_destroy (tuples);
        gsignond_identity_info_set_methods (identity, methods);
        g_hash_table_destroy (methods);
    }

    return identity;
}

/**
 * gsignond_db_metadata_database_get_identities:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 *
 * Reads all the identities from the database into a list.
 *
 * Returns: (transfer full) the list #GSignondIdentityInfoList if successful,
 * NULL otherwise. When done the list should be freed with
 * gsignond_identity_info_list_free
 */
GSignondIdentityInfoList *
gsignond_db_metadata_database_get_identities (GSignondDbMetadataDatabase *self)
{
    GSignondIdentityInfoList *identities = NULL;
    gchar *query = NULL;
    GArray *ids = NULL;
    gint rows = 0, i;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), NULL);

    query = sqlite3_mprintf ("SELECT id FROM credentials ORDER BY id");
    ids = gsignond_db_sql_database_query_exec_int_array (
                GSIGNOND_DB_SQL_DATABASE (self),
                query);
    sqlite3_free (query);
    if (!ids) {
        DBG ("No identity found");
        return NULL;
    }

    for (i=0; i < ids->len; i++) {
        GSignondIdentityInfo *identity = NULL;
        identity = gsignond_db_metadata_database_get_identity (self,
                g_array_index (ids, gint, i));
        if (identity) {
            identities = g_list_append (identities, identity);
        }
    }
    g_array_free (ids, TRUE);
    return identities;
}

/**
 * gsignond_db_metadata_database_remove_identity:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity
 *
 * Removes the identity data from the database based on the given id.
 *
 * Returns: TRUE if successful,FALSE otherwise.
 */
gboolean
gsignond_db_metadata_database_remove_identity (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id)
{
    gchar *queries = NULL;
    gboolean ret = FALSE;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), FALSE);

    queries = sqlite3_mprintf ("DELETE FROM CREDENTIALS WHERE id = %u;"
                               "DELETE FROM ACL WHERE identity_id = %u;"
                               "DELETE FROM REALMS WHERE identity_id = %u;"
                               "DELETE FROM owner WHERE identity_id = %u;",
                               identity_id, identity_id,
                               identity_id, identity_id);
    ret = gsignond_db_sql_database_transaction_exec (
            GSIGNOND_DB_SQL_DATABASE (self), queries);
    sqlite3_free (queries);

    return ret;
}

/**
 * gsignond_db_metadata_database_insert_reference:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity
 * @ref_owner: the owner security context
 * @reference: reference for the given identity
 *
 * Insert reference into the database for the given identity id.
 *
 * Returns: TRUE if successful,FALSE otherwise.
 */
gboolean
gsignond_db_metadata_database_insert_reference (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference)
{
    GSignondDbSqlDatabase *sql = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), 0);
    g_return_val_if_fail (ref_owner != NULL && reference != NULL, FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), FALSE);

    sql = GSIGNOND_DB_SQL_DATABASE (self);
    if (!gsignond_db_sql_database_start_transaction (sql)) {
        DBG ("Start transaction failed");
        return FALSE;
    }

    if (!_gsignond_db_metadata_database_exec (self,
            "INSERT OR IGNORE INTO SECCTX (sysctx, appctx) "
            "VALUES ( %Q, %Q );", ref_owner->sys_ctx, ref_owner->app_ctx)) {
        DBG ("Insertion SECCTX to DB failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        return FALSE;
    }
    if (!_gsignond_db_metadata_database_exec (self,
            "INSERT OR REPLACE INTO REFS "
            "(identity_id, secctx_id, ref) "
            "VALUES ( %u, "
            "( SELECT id FROM SECCTX "
            "WHERE sysctx = %Q AND appctx = %Q), %Q );",
            identity_id, ref_owner->sys_ctx, ref_owner->app_ctx, reference)) {
        DBG ("Insertion to REFS failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        return FALSE;
    }

    return gsignond_db_sql_database_commit_transaction (sql);
}

/**
 * gsignond_db_metadata_database_remove_reference:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity
 * @ref_owner: the owner security context
 * @reference: reference for the given identity
 *
 * Removes reference from the database for the given identity id.
 *
 * Returns: TRUE if successful,FALSE otherwise.
 */
gboolean
gsignond_db_metadata_database_remove_reference (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext *ref_owner,
        const gchar *reference)
{
    GSignondDbSqlDatabase *sql = NULL;
    GList *refs = NULL;
    gboolean ret = TRUE;
    guint len = 0;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), 0);
    g_return_val_if_fail (ref_owner != NULL, FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), FALSE);

    sql = GSIGNOND_DB_SQL_DATABASE (self);
    if (!gsignond_db_sql_database_start_transaction (sql)) {
        DBG ("Start transaction failed");
        return FALSE;
    }

    refs = gsignond_db_metadata_database_get_references (self,
            identity_id, ref_owner);

    len = g_list_length (refs);
    if (reference && !g_list_find_custom (refs, reference,
    		(GCompareFunc)g_strcmp0))
        ret = FALSE;
    g_list_free_full (refs, (GDestroyNotify)g_free);
    if (len <= 0 || !ret) {
        DBG ("No ref found");
        gsignond_db_sql_database_rollback_transaction (sql);
        return FALSE;
    }

    if (!reference || strlen (reference) <= 0) {
        ret = _gsignond_db_metadata_database_exec (self,
                "DELETE FROM REFS "
                "WHERE identity_id = %u AND "
                "secctx_id = ( SELECT id FROM SECCTX "
                "WHERE sysctx = %Q AND appctx = %Q );",
                identity_id, ref_owner->sys_ctx, ref_owner->app_ctx);
    } else {
        ret = _gsignond_db_metadata_database_exec (self,
                "DELETE FROM REFS "
                "WHERE identity_id = %u AND "
                "secctx_id = ( SELECT id FROM SECCTX "
                "WHERE sysctx = %Q AND appctx = %Q ) "
                "AND ref = :ref;",
                identity_id, ref_owner->sys_ctx, ref_owner->app_ctx, reference);
    }
    if (!ret) {
        DBG ("Delete refs from DB failed");
        gsignond_db_sql_database_rollback_transaction (sql);
        return FALSE;
    }

    return gsignond_db_sql_database_commit_transaction (sql);
}

/**
 * gsignond_db_metadata_database_get_references:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity
 * @ref_owner: the owner security context
 *
 * Gets references from the database for the given identity id.
 *
 * Returns: (transfer full) the list #GList if successful,
 * NULL otherwise. When done the list should be freed with
 * g_list_free_full (list, g_free)
 */
GList *
gsignond_db_metadata_database_get_references (
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id,
        const GSignondSecurityContext* ref_owner)
{
    gchar *query = NULL;
    GList *list = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), NULL);
    g_return_val_if_fail (ref_owner != NULL, NULL);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), NULL);

    if (!ref_owner->sys_ctx || strlen (ref_owner->sys_ctx) <= 0) {
        query = sqlite3_mprintf ("SELECT ref FROM REFS "
                                 "WHERE identity_id = %u;",
                                 identity_id);
    } else {
        query = sqlite3_mprintf ("SELECT ref FROM REFS "
                "WHERE identity_id = %u AND "
                "secctx_id = (SELECT id FROM SECCTX "
                "WHERE sysctx = %Q AND appctx = %Q );",
                identity_id, ref_owner->sys_ctx, ref_owner->app_ctx );
    }
    list = gsignond_db_sql_database_query_exec_string_list (
            GSIGNOND_DB_SQL_DATABASE (self),
            query);
    sqlite3_free (query);
    return list;
}

/**
 * gsignond_db_metadata_database_get_accesscontrol_list:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity whose access control list is needed
 *
 * Gets all the access control list from the database into a list.
 *
 * Returns: (transfer full) the list #GSignondSecurityContextList if successful,
 * NULL otherwise. When done the list should be freed with
 * gsignond_identity_info_list_free
 */
GSignondSecurityContextList *
gsignond_db_metadata_database_get_accesscontrol_list(
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id)
{
    GSignondSecurityContextList *list = NULL;
    GHashTable *tuples = NULL;
    gchar *query = NULL;
    GHashTableIter iter;
    const gchar *sysctx = NULL, *appctx = NULL;
    GSignondSecurityContext *ctx = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), NULL);

    query = sqlite3_mprintf ("SELECT sysctx, appctx FROM SECCTX "
            "WHERE id IN "
            "(SELECT secctx_id FROM ACL WHERE identity_id = %u);",
            identity_id);
    tuples = gsignond_db_sql_database_query_exec_string_tuple (
                    GSIGNOND_DB_SQL_DATABASE (self),
                    query);
    sqlite3_free (query);

    if (tuples) {
        g_hash_table_iter_init(&iter, tuples);
        while (g_hash_table_iter_next (&iter, (gpointer *)&sysctx,
                (gpointer *)&appctx)) {
            ctx = gsignond_security_context_new_from_values (sysctx, appctx);
            list = g_list_append (list, ctx);
        }
        g_hash_table_unref (tuples);
    }
    return list;
}

/**
 * gsignond_db_metadata_database_get_owner_list:
 *
 * @self: instance of #GSignondDbMetadataDatabase
 * @identity_id: the id of the identity whose owner list is needed
 *
 * Gets all the onwer list from the database into a list.
 *
 * Returns: (transfer full) the list #GSignondSecurityContextList if successful,
 * NULL otherwise. When done the list should be freed with
 * gsignond_identity_info_list_free
 */
GSignondSecurityContextList *
gsignond_db_metadata_database_get_owner_list(
        GSignondDbMetadataDatabase *self,
        const guint32 identity_id)
{
    GSignondSecurityContextList *list = NULL;
    GHashTable *tuples = NULL;
    gchar *query = NULL;
    GHashTableIter iter;
    const gchar *sysctx = NULL, *appctx = NULL;
    GSignondSecurityContext *ctx = NULL;

    g_return_val_if_fail (GSIGNOND_DB_IS_METADATA_DATABASE (self), FALSE);
    RETURN_IF_NOT_OPEN (GSIGNOND_DB_SQL_DATABASE (self), NULL);

    query = sqlite3_mprintf ("SELECT sysctx, appctx FROM SECCTX "
            "WHERE id IN "
            "(SELECT secctx_id FROM OWNER WHERE identity_id = %u);",
            identity_id);
    tuples = gsignond_db_sql_database_query_exec_string_tuple (
                    GSIGNOND_DB_SQL_DATABASE (self),
                    query);
    sqlite3_free (query);

    if (tuples) {
        g_hash_table_iter_init(&iter, tuples);
        while (g_hash_table_iter_next (&iter, (gpointer *)&sysctx,
                (gpointer *)&appctx)) {
            ctx = gsignond_security_context_new_from_values (sysctx, appctx);
            list = g_list_append (list, ctx);
        }
        g_hash_table_unref (tuples);
    }
    return list;
}


