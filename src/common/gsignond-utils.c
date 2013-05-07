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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "gsignond/gsignond-utils.h"
#include "gsignond/gsignond-log.h"


static size_t pagesize = 0;


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
    if (rngfd < 0)
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

