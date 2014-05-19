/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "gsignond/gsignond-utils.h"
#include "gsignond/gsignond-log.h"

/**
 * SECTION:gsignond-utils
 * @title: Utility functions
 * @short_description: miscellaneous utility functions
 * @include: gsignond/gsignond-utils.h
 *
 * Miscellaneous utility functions are described below.
 */

typedef struct __nonce_ctx_t
{
    gboolean initialized;
    guint32 serial;
    guchar key[32];
    guchar entropy[16];
} _nonce_ctx_t;

static size_t pagesize = 0;
static _nonce_ctx_t _nonce_ctx = { 0, };
G_LOCK_DEFINE_STATIC (_nonce_lock);

/**
 * gsignond_wipe_file:
 * @filename: filename to wipe
 *
 * This function securely wipes the contents of the file, by overwriting it with
 * 0's, then 1's, then random data. The file is then removed.
 *
 * Returns: TRUE if wiping and removal was successful.
 */
gboolean
gsignond_wipe_file (const gchar *filename)
{
    gboolean retval = FALSE;
    int rngfd;
    int wipefd;
    size_t sizeleft;
    size_t writesize;
    ssize_t sizewritten;
    struct stat filestat;
    guint8 *wipebuf;

    if (!pagesize) {
        long confval = sysconf (_SC_PAGE_SIZE);
        if (confval <= 0)
            return FALSE;
        pagesize = (size_t) confval;
    }

    rngfd = open ("/dev/urandom", O_RDONLY);
    if (rngfd < 0)
        return FALSE;

    wipefd = open (filename, O_WRONLY | O_SYNC);
    if (wipefd < 0)
        goto _rng_exit;
    wipebuf = g_malloc (pagesize);
    if (fstat (wipefd, &filestat))
        goto _wipe_exit;

    /* write all 1's */
    sizeleft = filestat.st_size;
    memset (wipebuf, 0xff, pagesize);
    while (sizeleft) {
        writesize = (sizeleft < pagesize) ? sizeleft : pagesize;
        sizewritten = write (wipefd, wipebuf, writesize);
        if (sizewritten != (ssize_t) writesize)
            goto _wipe_exit;
        sizeleft -= sizewritten;
    }

    if (lseek (wipefd, 0, SEEK_SET) == (off_t) -1)
        goto _wipe_exit;

    /* write all 0's */
    sizeleft = filestat.st_size;
    memset (wipebuf, 0x00, pagesize);
    while (sizeleft) {
        writesize = (sizeleft < pagesize) ? sizeleft : pagesize;
        sizewritten = write (wipefd, wipebuf, writesize);
        if (sizewritten != (ssize_t) writesize)
            goto _wipe_exit;
        sizeleft -= sizewritten;
    }

    if (lseek (wipefd, 0, SEEK_SET) == (off_t) -1)
        goto _wipe_exit;

    /* write random */
    sizeleft = filestat.st_size;
    while (sizeleft) {
        writesize = (sizeleft < pagesize) ? sizeleft : pagesize;
        if (read (rngfd, wipebuf, writesize) != (ssize_t) writesize)
            goto _wipe_exit;
        sizewritten = write (wipefd, wipebuf, writesize);
        if (sizewritten != (ssize_t) writesize)
            goto _wipe_exit;
        sizeleft -= sizewritten;
    }

    /* don't leave traces of last pattern to the memory */
    memset (wipebuf, 0x00, pagesize);

    /* remove the file and set return value on success */
    if (unlink (filename) == 0) {
        retval = TRUE;
        DBG ("successfully wiped file %s", filename);
    }

_wipe_exit:
    g_free (wipebuf);
    close (wipefd);
_rng_exit:
    close (rngfd);
    return retval;
}

/**
 * gsignond_wipe_directory:
 * @dirname: directory to wipe
 *
 * This function securely wipes the contents of the directory by calling
 * gsignond_wipe_file() on each file. It also removes links and empty directories but 
 * does not recursively wipe them.
 *
 * Returns: TRUE if wiping and removal was successful.
 */
gboolean
gsignond_wipe_directory (const gchar *dirname)
{
    gboolean retval = FALSE;
    gboolean wiperes;
    const gchar *filename;
    gchar *filepath;
    GDir *dirctx;
    struct stat stat_entry;

    DBG ("wipe directory %s", dirname);
    dirctx = g_dir_open (dirname, 0, NULL);
    if (!dirctx)
        return FALSE;
    while ((filename = g_dir_read_name (dirctx))) {
        filepath = g_build_filename (dirname, filename, NULL);
        if (lstat(filepath, &stat_entry))
            goto _dir_exit;
        if (S_ISDIR (stat_entry.st_mode) ||
            S_ISLNK (stat_entry.st_mode)) {
            DBG ("remove directory or link %s", filepath);
            wiperes = (remove (filepath) == 0);
        } else {
            DBG ("wipe file %s", filepath);
            wiperes = gsignond_wipe_file (filepath);
        }
        g_free (filepath);
        if (!wiperes)
            goto _dir_exit;
    }
    retval = TRUE;

_dir_exit:
    g_dir_close (dirctx);
    return retval;
}

static gboolean
_init_nonce_gen ()
{
    if (G_LIKELY(_nonce_ctx.initialized))
        return TRUE;

    int fd;

    fd = open ("/dev/urandom", O_RDONLY);
    if (fd < 0)
        goto init_exit;
    if (read (fd, _nonce_ctx.key, sizeof (_nonce_ctx.key)) !=
        sizeof (_nonce_ctx.key))
        goto init_close;
    if (read (fd, _nonce_ctx.entropy, sizeof(_nonce_ctx.entropy)) !=
        sizeof (_nonce_ctx.entropy))
        goto init_close;

    _nonce_ctx.serial = 0;

    _nonce_ctx.initialized = TRUE;

init_close:
    close (fd);

init_exit:
    return _nonce_ctx.initialized;
}

/**
 * gsignond_generate_nonce:
 *
 * This function generates a random secure nonce using SHA1 HMAC.
 *
 * Returns: (transfer full): the nonce in lowercase hexadecimal format, 40 bytes long.
 */
gchar *
gsignond_generate_nonce ()
{
    GHmac *hmac;
    gchar *nonce = NULL;
    struct timespec ts;

    G_LOCK (_nonce_lock);

    if (G_UNLIKELY (!_init_nonce_gen()))
        goto nonce_exit;

    hmac = g_hmac_new (G_CHECKSUM_SHA1,
                       _nonce_ctx.key, sizeof (_nonce_ctx.key));
    g_hmac_update (hmac, _nonce_ctx.entropy, sizeof (_nonce_ctx.entropy));
    _nonce_ctx.serial++;
    g_hmac_update (hmac,
                   (const guchar *) &_nonce_ctx.serial,
                   sizeof (_nonce_ctx.serial));
    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
        g_hmac_update (hmac, (const guchar *) &ts, sizeof (ts));
    memset (&ts, 0x00, sizeof(ts));
    nonce = g_strdup (g_hmac_get_string (hmac));
    g_hmac_unref (hmac);

nonce_exit:
    G_UNLOCK (_nonce_lock);

    return nonce;
}

static gint
_compare_strings (
		const gchar* a,
		const gchar* b,
		gpointer data)
{
	(void)data;
	return g_strcmp0 (a,b);
}

/**
 * gsignond_sequence_to_variant:
 * @seq: Sequence of strings to convert
 *
 * Convert a string sequence to a variant.
 *
 * Returns: (transfer full): #GVariant of type "as".
 */
GVariant *
gsignond_sequence_to_variant (GSequence *seq)
{
    GSequenceIter * iter = NULL;
    GVariant *var = NULL;
    GVariantBuilder builder;

    if (!seq) return NULL;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_STRING_ARRAY);
    iter = g_sequence_get_begin_iter (seq);
    while (!g_sequence_iter_is_end (iter)) {
        const gchar * d = g_sequence_get (iter);
        g_variant_builder_add (&builder, "s", d);
        iter = g_sequence_iter_next (iter);
    }
    var = g_variant_builder_end (&builder);
    return var;
}

/**
 * gsignond_variant_to_sequence:
 * @var: Variant of "as" to convert
 *
 * Convert variant containing string array to sequence.
 *
 * Returns: (transfer full): #GSequence of strings
 */
GSequence *
gsignond_variant_to_sequence (GVariant *var)
{
    GVariantIter iter;
    GSequence *seq = NULL;
    gchar *item = NULL;

    if (!var) return NULL;

    seq = g_sequence_new ((GDestroyNotify)g_free);
    g_variant_iter_init (&iter, var);
    while (g_variant_iter_next (&iter, "s", &item)) {
        g_sequence_insert_sorted (seq,
                                  item,
                                  (GCompareDataFunc) _compare_strings,
                                  NULL);
    }
    return seq;
}

/**
 * gsignond_sequence_to_array:
 * @seq: Sequence of strings to convert
 *
 * Convert sequence of strings to null-terminated string array.
 *
 * Returns: (transfer full): Null-terminated array of strings
 */
gchar **
gsignond_sequence_to_array (GSequence *seq)
{
    gchar **items, **temp;
    GSequenceIter *iter;

    if (!seq) return NULL;

    items = g_malloc0 ((g_sequence_get_length (seq) + 1) * sizeof (gchar *));
    temp = items;
    for (iter = g_sequence_get_begin_iter (seq);
         iter != g_sequence_get_end_iter (seq);
         iter = g_sequence_iter_next (iter)) {
        *temp = g_sequence_get (iter);
        temp++;
    }
    return items;
}

/**
 * gsignond_array_to_sequence:
 * @items: (transfer full): Null-terminated array of strings to convert
 *
 * Convert null-terminated array of strings to a sequence.
 *
 * Returns: (transfer full): #GSequence of strings
 */
GSequence *
gsignond_array_to_sequence (gchar **items)
{
    gchar **item_iter = items;
    GSequence *seq = NULL;

    if (!items) return NULL;

    seq = g_sequence_new ((GDestroyNotify) g_free);
    while (*item_iter) {
        g_sequence_insert_sorted (seq,
                                  *item_iter,
                                  (GCompareDataFunc) _compare_strings,
                                  NULL);
        item_iter++;
    }
    g_free (items);
    return seq;
}

/**
 * gsignond_copy_array_to_sequence:
 * @items: Null-terminated array of strings to copy
 *
 * Copy null-terminated array of strings to a sequence.
 *
 * Returns: (transfer full): #GSequence of strings
 */
GSequence *
gsignond_copy_array_to_sequence (const gchar **items)
{
    GSequence *seq = NULL;

    if (!items) return NULL;

    seq = g_sequence_new ((GDestroyNotify) g_free);
    while (*items) {
        g_sequence_insert_sorted (seq,
                                  g_strdup (*items),
                                  (GCompareDataFunc) _compare_strings,
                                  NULL);
        items++;
    }
    return seq;
}

/**
 * gsignond_is_host_in_domain:
 * @domain: a domain name
 * @host: a host name
 *
 * Checks if @host belongs to @domain.
 *
 * Returns: the result
 */
gboolean 
gsignond_is_host_in_domain(const gchar *host, const gchar *domain)
{
    gchar** domain_parts = g_strsplit(domain, ".", 0);
    gchar** host_parts = g_strsplit(host, ".", 0);
    gchar** truncated_host_parts = host_parts;
    
    guint domain_parts_n = g_strv_length(domain_parts);
    guint host_parts_n = g_strv_length(host_parts);
    
    gint extra_host_parts_n = host_parts_n - domain_parts_n;
    
    while (extra_host_parts_n > 0) {
        truncated_host_parts++;
        extra_host_parts_n--;
    }
    gchar* truncated_host = g_strjoinv(".", truncated_host_parts);
    gint result = g_strcmp0(domain, truncated_host);
    
    g_free(truncated_host);
    g_strfreev(host_parts);
    g_strfreev(domain_parts);
    
    return result == 0 ? TRUE : FALSE;
}