/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <glib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gprintf.h>

#include "util.h"
#include "downloadtarget.h"
#include "downloadtarget_internal.h"

LrDownloadTargetChecksum *
lr_downloadtargetchecksum_new(LrChecksumType type, const gchar *value)
{
    LrDownloadTargetChecksum *dtch = lr_malloc0(sizeof(*dtch));
    dtch->type = type;
    dtch->value = g_strdup(value);
    return dtch;
}

void
lr_downloadtargetchecksum_free(LrDownloadTargetChecksum *dtch)
{
    if (!dtch) return;
    g_free(dtch->value);
    g_free(dtch);
}

LrDownloadTarget *
lr_downloadtarget_new(LrHandle *handle,
                      const char *path,
                      const char *baseurl,
                      int fd,
                      const char *fn,
                      GSList *possiblechecksums,
                      gint64 expectedsize,
                      gboolean resume,
                      LrProgressCb progresscb,
                      void *cbdata,
                      LrEndCb endcb,
                      LrMirrorFailureCb mirrorfailurecb,
                      void *userdata,
                      gint64 byterangestart,
                      gint64 byterangeend)
{
    LrDownloadTarget *target;

    assert(path);
    assert((fd > 0 && !fn) || (fd < 0 && fn));

    target = lr_malloc0(sizeof(*target));

    if (byterangestart && resume) {
        g_debug("%s: Cannot specify byterangestart and set resume to TRUE "
                "at the same time", __func__);
        return NULL;
    }

    target->handle          = handle;
    target->chunk           = g_string_chunk_new(0);
    target->path            = g_string_chunk_insert(target->chunk, path);
    target->baseurl         = lr_string_chunk_insert(target->chunk, baseurl);
    target->fd              = fd;
    target->fn              = lr_string_chunk_insert(target->chunk, fn);
    target->checksums       = possiblechecksums;
    target->expectedsize    = expectedsize;
    target->resume          = resume;
    target->progresscb      = progresscb;
    target->cbdata          = cbdata;
    target->endcb           = endcb;
    target->mirrorfailurecb = mirrorfailurecb;
    target->rcode           = LRE_UNFINISHED;
    target->userdata        = userdata;
    target->byterangestart  = byterangestart;
    target->byterangeend    = byterangeend;

    return target;
}

void
lr_downloadtarget_free(LrDownloadTarget *target)
{
    if (!target)
        return;

    g_slist_free_full(target->checksums,
                      (GDestroyNotify) lr_downloadtargetchecksum_free);
    g_string_chunk_free(target->chunk);
    lr_free(target);
}

void
lr_downloadtarget_set_error(LrDownloadTarget *target,
                            LrRc code,
                            const char *format,
                            ...)
{
    assert(target);
    assert(code == LRE_OK || format);

    if (format) {
        int ret;
        va_list vl;
        gchar *message = NULL;

        va_start(vl, format);
        ret = g_vasprintf(&message, format, vl);
        va_end(vl);

        if (ret < 0) {
            assert(0);
            target->err = "";
            return;
        }

        target->err = lr_string_chunk_insert(target->chunk, message);
        g_free(message);
    } else {
        target->err = NULL;
    }

    target->rcode = code;
}

void
lr_downloadtarget_set_usedmirror(LrDownloadTarget *target, const char *url)
{
    assert(target);
    target->usedmirror = lr_string_chunk_insert(target->chunk, url);
}

void
lr_downloadtarget_set_effectiveurl(LrDownloadTarget *target, const char *url)
{
    assert(target);
    target->effectiveurl = lr_string_chunk_insert(target->chunk, url);
}
