/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apr_arch_file_io.h"
#include "fsio.h"
#include "nks/dirio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_errno.h"
#include "apr_hash.h"
#include "apr_thread_rwlock.h"

/*#define APR_HAS_PSA*/

static apr_filetype_e filetype_from_mode(mode_t mode)
{
    apr_filetype_e type = APR_NOFILE;

    if (S_ISREG(mode))
        type = APR_REG;
    else if (S_ISDIR(mode))
        type = APR_DIR;
    else if (S_ISCHR(mode))
        type = APR_CHR;
    else if (S_ISBLK(mode))
        type = APR_BLK;
    else if (S_ISFIFO(mode))
        type = APR_PIPE;
    else if (S_ISLNK(mode))
        type = APR_LNK;
    else if (S_ISSOCK(mode))
        type = APR_SOCK;
    else
        type = APR_UNKFILE;
    return type;
}

static void fill_out_finfo(apr_finfo_t *finfo, struct stat *info,
                           apr_int32_t wanted)
{ 
    finfo->valid = APR_FINFO_MIN | APR_FINFO_IDENT | APR_FINFO_NLINK 
                    | APR_FINFO_OWNER | APR_FINFO_PROT;
    finfo->protection = apr_unix_mode2perms(info->st_mode);
    finfo->filetype = filetype_from_mode(info->st_mode);
    finfo->user = info->st_uid;
    finfo->group = info->st_gid;
    finfo->size = info->st_size;
    finfo->inode = info->st_ino;
    finfo->device = info->st_dev;
    finfo->nlink = info->st_nlink;
    apr_time_ansi_put(&finfo->atime, info->st_atime.tv_sec);
    apr_time_ansi_put(&finfo->mtime, info->st_mtime.tv_sec);
    apr_time_ansi_put(&finfo->ctime, info->st_ctime.tv_sec);
    /* ### needs to be revisited  
     * if (wanted & APR_FINFO_CSIZE) {
     *   finfo->csize = info->st_blocks * 512;
     *   finfo->valid |= APR_FINFO_CSIZE;
     * }
     */
}

APR_DECLARE(apr_status_t) apr_file_info_get(apr_finfo_t *finfo, 
                                            apr_int32_t wanted,
                                            apr_file_t *thefile)
{
    struct stat info;

    if (thefile->buffered) {
        apr_status_t rv = apr_file_flush(thefile);
        if (rv != APR_SUCCESS)
            return rv;
    }

    if (fstat(thefile->filedes, &info) == 0) {
        finfo->pool = thefile->pool;
        finfo->fname = thefile->fname;
        fill_out_finfo(finfo, &info, wanted);
        return (wanted & ~finfo->valid) ? APR_INCOMPLETE : APR_SUCCESS;
    }
    else {
        return errno;
    }
}

APR_DECLARE(apr_status_t) apr_file_perms_set(const char *fname, 
                                             apr_fileperms_t perms)
{
    mode_t mode = apr_unix_perms2mode(perms);

    if (chmod(fname, mode) == -1)
        return errno;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_attrs_set(const char *fname,
                                             apr_fileattrs_t attributes,
                                             apr_fileattrs_t attr_mask,
                                             apr_pool_t *pool)
{
    apr_status_t status;
    apr_finfo_t finfo;

    status = apr_stat(&finfo, fname, APR_FINFO_PROT, pool);
    if (!APR_STATUS_IS_SUCCESS(status))
        return status;

    /* ### TODO: should added bits be umask'd? */
    if (attr_mask & APR_FILE_ATTR_READONLY)
    {
        if (attributes & APR_FILE_ATTR_READONLY)
        {
            finfo.protection &= ~APR_UWRITE;
            finfo.protection &= ~APR_GWRITE;
            finfo.protection &= ~APR_WWRITE;
        }
        else
        {
            /* ### umask this! */
            finfo.protection |= APR_UWRITE;
            finfo.protection |= APR_GWRITE;
            finfo.protection |= APR_WWRITE;
        }
    }

    if (attr_mask & APR_FILE_ATTR_EXECUTABLE)
    {
        if (attributes & APR_FILE_ATTR_EXECUTABLE)
        {
            /* ### umask this! */
            finfo.protection |= APR_UEXECUTE;
            finfo.protection |= APR_GEXECUTE;
            finfo.protection |= APR_WEXECUTE;
        }
        else
        {
            finfo.protection &= ~APR_UEXECUTE;
            finfo.protection &= ~APR_GEXECUTE;
            finfo.protection &= ~APR_WEXECUTE;
        }
    }

    return apr_file_perms_set(fname, finfo.protection);
}

#ifndef APR_HAS_PSA
static apr_status_t stat_cache_cleanup(void *data)
{
    apr_pool_t *p = (apr_pool_t *)getGlobalPool();
    apr_hash_index_t *hi;
    apr_hash_t *statCache = (apr_hash_t*)data;
	char *key;
    apr_ssize_t keylen;
    NXPathCtx_t pathctx;

    for (hi = apr_hash_first(p, statCache); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, (const void**)&key, &keylen, (void**)&pathctx);

        if (pathctx) {
            NXFreePathContext(pathctx);
        }
    }

    return APR_SUCCESS;
}

int cstat (NXPathCtx_t ctx, char *path, struct stat *buf, unsigned long requestmap, apr_pool_t *p)
{
    apr_pool_t *gPool = (apr_pool_t *)getGlobalPool();
    apr_hash_t *statCache = NULL;
    apr_thread_rwlock_t *rwlock = NULL;

    NXPathCtx_t pathctx = 0;
    char *ptr = NULL, *tr;
    int len = 0, x;
    char *ppath;
    char *pinfo;

    if (ctx == 1) {

        /* If there isn't a global pool then just stat the file
           and return */
        if (!gPool) {
            char poolname[50];
    
            if (apr_pool_create(&gPool, NULL) != APR_SUCCESS) {
                return getstat(ctx, path, buf, requestmap);
            }
    
            setGlobalPool(gPool);
            apr_pool_tag(gPool, apr_pstrdup(gPool, "cstat_mem_pool"));
    
            statCache = apr_hash_make(gPool);
            apr_pool_userdata_set ((void*)statCache, "STAT_CACHE", stat_cache_cleanup, gPool);

            apr_thread_rwlock_create(&rwlock, gPool);
            apr_pool_userdata_set ((void*)rwlock, "STAT_CACHE_LOCK", apr_pool_cleanup_null, gPool);
        }
        else {
            apr_pool_userdata_get((void**)&statCache, "STAT_CACHE", gPool);
            apr_pool_userdata_get((void**)&rwlock, "STAT_CACHE_LOCK", gPool);
        }

        if (!gPool || !statCache || !rwlock) {
            return getstat(ctx, path, buf, requestmap);
        }
    
        for (x = 0,tr = path;*tr != '\0';tr++,x++) {
            if (*tr == '\\' || *tr == '/') {
                ptr = tr;
                len = x;
            }
            if (*tr == ':') {
                ptr = "\\";
                len = x;
            }
        }
    
        if (ptr) {
            ppath = apr_pstrndup (p, path, len);
            strlwr(ppath);
            if (ptr[1] != '\0') {
                ptr++;
            }
            /* If the path ended in a trailing slash then our result path
               will be a single slash. To avoid stat'ing the root with a
               slash, we need to make sure we stat the current directory
               with a dot */
            if (((*ptr == '/') || (*ptr == '\\')) && (*(ptr+1) == '\0')) {
                pinfo = apr_pstrdup (p, ".");
            }
            else {
                pinfo = apr_pstrdup (p, ptr);
            }
        }
    
        /* If we have a statCache then try to pull the information
           from the cache.  Otherwise just stat the file and return.*/
        if (statCache) {
            apr_thread_rwlock_rdlock(rwlock);
            pathctx = (NXPathCtx_t) apr_hash_get(statCache, ppath, APR_HASH_KEY_STRING);
            apr_thread_rwlock_unlock(rwlock);
            if (pathctx) {
                return getstat(pathctx, pinfo, buf, requestmap);
            }
            else {
                int err;

                err = NXCreatePathContext(0, ppath, 0, NULL, &pathctx);
                if (!err) {
                    apr_thread_rwlock_wrlock(rwlock);
                    apr_hash_set(statCache, apr_pstrdup(gPool,ppath) , APR_HASH_KEY_STRING, (void*)pathctx);
                    apr_thread_rwlock_unlock(rwlock);
                    return getstat(pathctx, pinfo, buf, requestmap);
                }
            }
        }
    }
    return getstat(ctx, path, buf, requestmap);
}
#endif

APR_DECLARE(apr_status_t) apr_stat(apr_finfo_t *finfo, 
                                   const char *fname, 
                                   apr_int32_t wanted, apr_pool_t *pool)
{
    struct stat info;
    int srv;
    NXPathCtx_t pathCtx = 0;

    getcwdpath(NULL, &pathCtx, CTX_ACTUAL_CWD);
#ifdef APR_HAS_PSA
	srv = getstat(pathCtx, (char*)fname, &info, ST_STAT_BITS|ST_NAME_BIT);
#else
    srv = cstat(pathCtx, (char*)fname, &info, ST_STAT_BITS|ST_NAME_BIT, pool);
#endif
    errno = srv;

    if (srv == 0) {
        finfo->pool = pool;
        finfo->fname = fname;
        fill_out_finfo(finfo, &info, wanted);
        if (wanted & APR_FINFO_LINK)
            wanted &= ~APR_FINFO_LINK;
        if (wanted & APR_FINFO_NAME) {
            finfo->name = apr_pstrdup(pool, info.st_name);
            finfo->valid |= APR_FINFO_NAME;
        }
        return (wanted & ~finfo->valid) ? APR_INCOMPLETE : APR_SUCCESS;
    }
    else {
#if !defined(ENOENT) || !defined(ENOTDIR)
#error ENOENT || ENOTDIR not defined; please see the
#error comments at this line in the source for a workaround.
        /*
         * If ENOENT || ENOTDIR is not defined in one of the your OS's
         * include files, APR cannot report a good reason why the stat()
         * of the file failed; there are cases where it can fail even though
         * the file exists.  This opens holes in Apache, for example, because
         * it becomes possible for someone to get a directory listing of a 
         * directory even though there is an index (eg. index.html) file in 
         * it.  If you do not have a problem with this, delete the above 
         * #error lines and start the compile again.  If you need to do this,
         * please submit a bug report to http://www.apache.org/bug_report.html
         * letting us know that you needed to do this.  Please be sure to 
         * include the operating system you are using.
         */
        /* WARNING: All errors will be handled as not found
         */
#if !defined(ENOENT) 
        return APR_ENOENT;
#else
        /* WARNING: All errors but not found will be handled as not directory
         */
        if (errno != ENOENT)
            return APR_ENOENT;
        else
            return errno;
#endif
#else /* All was defined well, report the usual: */
        return errno;
#endif
    }
}

/* Perhaps this becomes nothing but a macro?
 */
APR_DECLARE(apr_status_t) apr_lstat(apr_finfo_t *finfo, const char *fname,
                      apr_int32_t wanted, apr_pool_t *pool)
{
    return apr_stat(finfo, fname, wanted | APR_FINFO_LINK, pool);
}

