/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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

#include "fileio.h"
#ifdef FAST_STAT
#include "fsio.h"
#endif
#include "nks/dirio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_errno.h"
#include "apr_hash.h"

static apr_filetype_e filetype_from_mode(mode_t mode)
{
    apr_filetype_e type = APR_NOFILE;

    if (S_ISREG(mode))
        type = APR_REG;
    if (S_ISDIR(mode))
        type = APR_DIR;
    if (S_ISCHR(mode))
        type = APR_CHR;
    if (S_ISBLK(mode))
        type = APR_BLK;
    if (S_ISFIFO(mode))
        type = APR_PIPE;
    if (S_ISLNK(mode))
        type = APR_LNK;
    if (S_ISSOCK(mode))
        type = APR_SOCK;
    return type;
}

static void fill_out_finfo(apr_finfo_t *finfo, struct stat *info,
                           apr_int32_t wanted)
{ 
    finfo->valid = APR_FINFO_MIN | APR_FINFO_IDENT | APR_FINFO_NLINK;
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

#ifndef FAST_STAT
char *case_filename(apr_pool_t *pPool, const char *szFile)
{
    char *casedFileName = NULL;
    char name[1024];
    int rc;

    rc = realname(szFile, name);
    if (rc == 0) {
        casedFileName = apr_pstrdup(pPool, name);
    }
    else
    {
        char *s;
        s = strrchr(szFile, '/');
        if (!s)
            s = strrchr(szFile, ':');
        if (s) {
            casedFileName = apr_pstrdup(pPool, &s[1]);
        }
    }
    return casedFileName;
}
#endif


APR_DECLARE(apr_status_t) apr_file_info_get(apr_finfo_t *finfo, 
                                            apr_int32_t wanted,
                                            apr_file_t *thefile)
{
    struct stat info;

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

#ifdef FAST_STAT
int cstat (const char *path, struct stat *buf, char **casedName, apr_pool_t *pool)
{
    apr_hash_t *statCache = (apr_hash_t *)getStatCache(CpuCurrentProcessor);
    apr_pool_t *gPool = (apr_pool_t *)getGlobalPool(CpuCurrentProcessor);
    apr_stat_entry_t *stat_entry;
    struct stat *info;
    apr_time_t now = apr_time_now();
    NXPathCtx_t pathCtx = 0;
    char *key;
    int ret;
    int found = 0;

    *casedName = NULL;
    errno = 0;

    /* If there isn't a global pool then just stat the file
       and return */
    if (!gPool) {
        char poolname[50];

        if (apr_pool_create(&gPool, NULL) != APR_SUCCESS) {
            getcwdpath(NULL, &pathCtx, CTX_ACTUAL_CWD);
            ret = getstat(pathCtx, path, buf, ST_STAT_BITS|ST_NAME_BIT);
            if (ret == 0) {
                *casedName = apr_pstrdup (pool, buf->st_name);
                return 0;
            }
            else {
                errno = ret;
                return -1;
            }
        }

        sprintf (poolname, "cstat_mem_pool_%d", CpuCurrentProcessor);
        apr_pool_tag(gPool, apr_pstrdup(gPool, poolname));

        setGlobalPool(gPool, CpuCurrentProcessor);
    }

    /* If we have a statCache hash table then use it.
       Otherwise we need to create it and initialized it
       with a new mutex lock. */
    if (!statCache) {
        statCache = apr_hash_make(gPool);
        setStatCache((void*)statCache, CpuCurrentProcessor);
    }

    /* If we have a statCache then try to pull the information
       from the cache.  Otherwise just stat the file and return.*/
    if (statCache) {
        stat_entry = (apr_stat_entry_t*) apr_hash_get(statCache, path, APR_HASH_KEY_STRING);
        /* If we got an entry then check the expiration time.  If the entry
           hasn't expired yet then copy the information and return. */
        if (stat_entry) {
            if ((now - stat_entry->expire) <= APR_USEC_PER_SEC) {
                memcpy (buf, &(stat_entry->info), sizeof(struct stat));
                *casedName = apr_pstrdup (pool, stat_entry->casedName);
                found = 1;
            }
        }

        /* Since we are creating a separate stat cache for each processor, we
           don't need to worry about locking the hash table before manipulating
           it. */
        if (!found) {
            /* Bind the thread to the current cpu so that we don't wake
               up on some other cpu and try to manipulate the wrong cache. */
            NXThreadBind (CpuCurrentProcessor);

            /* If we don't have a stat_entry then create one, copy
               the data and add it to the hash table. */
            if (!stat_entry) {
                char *dirPath = NULL, *fname = NULL;
                char *ptr;
                int err, len;
                char pathbuf[256];

                getcwdpath(pathbuf, &pathCtx, CTX_ACTUAL_CWD);
                ret = getstat(pathCtx, path, buf, ST_STAT_BITS|ST_NAME_BIT);

                if (ret) {
                    NXThreadBind (NX_THR_UNBOUND);
                    errno = ret;
                    return -1;
    			}

                if (filetype_from_mode(buf->st_mode) == APR_DIR) {
                    dirPath = apr_pstrdup (pool, path);
                    len = strlen (dirPath) - strlen(buf->st_name);
                    dirPath[len-1] = '\0';
                }
                else if (filetype_from_mode(buf->st_mode) == APR_REG) {
                    dirPath = apr_pstrdup (pool, path);
                    ptr = strrchr (dirPath, '/');
                    if (ptr) {
                        *ptr = '\0';
                    }
                }

/* xxx Need to handle error codes here */
                err = NXCreatePathContext(pathCtx, dirPath, 0, NULL, &pathCtx);

                key = apr_pstrdup (gPool, path);
                stat_entry = apr_palloc (gPool, sizeof(apr_stat_entry_t));
                memcpy (&(stat_entry->info), buf, sizeof(struct stat));
                stat_entry->casedName = (stat_entry->info).st_name;
                *casedName = apr_pstrdup(pool, stat_entry->casedName);
                stat_entry->expire = now;
                if (err == 0) {
                    stat_entry->pathCtx = pathCtx;
                }
                else {
                    stat_entry->pathCtx = 0;
                }
                apr_hash_set(statCache, key, APR_HASH_KEY_STRING, stat_entry);
            }
            else {
                NXDirAttrNks_t dirInfo;

                /* If we have a path context then get the info the fast way.  Otherwise 
                   just default to getting the stat info from stat() */
                if (stat_entry->pathCtx) {
                    ret = getstat(stat_entry->pathCtx, stat_entry->casedName, buf, 
                                  ST_MODE_BIT|ST_ATIME_BIT|ST_MTIME_BIT|ST_CTIME_BIT|ST_SIZE_BIT|ST_NAME_BIT);
                }
                else {
                    char pathbuf[256];
                    getcwdpath(pathbuf, &pathCtx, CTX_ACTUAL_CWD);
                    ret = getstat(pathCtx, path, buf, 
                                  ST_MODE_BIT|ST_ATIME_BIT|ST_MTIME_BIT|ST_CTIME_BIT|ST_SIZE_BIT|ST_NAME_BIT);
                }
    
                if (ret) {
                    NXThreadBind (NX_THR_UNBOUND);
                    errno = ret;
                    return -1;
                }
                else {
                    (stat_entry->info).st_atime.tv_sec = (buf->st_atime).tv_sec;
                    (stat_entry->info).st_mtime.tv_sec = (buf->st_mtime).tv_sec;
                    (stat_entry->info).st_ctime.tv_sec = (buf->st_ctime).tv_sec;
                    (stat_entry->info).st_size = buf->st_size;
                    (stat_entry->info).st_mode = buf->st_mode;
                    memcpy ((stat_entry->info).st_name, buf->st_name, sizeof(buf->st_name));
                    memcpy (buf, &(stat_entry->info), sizeof(struct stat));
                }

                /* If we do have a stat_entry then it must have expired.  Just
                   copy the data and reset the expiration. */
                *casedName = apr_pstrdup(pool, stat_entry->casedName);
                stat_entry->expire = now;
            }
            NXThreadBind (NX_THR_UNBOUND);
        }
    }
    else {
        getcwdpath(NULL, &pathCtx, CTX_ACTUAL_CWD);
        ret = getstat(pathCtx, path, buf, ST_STAT_BITS|ST_NAME_BIT);
        if (ret == 0) {
            *casedName = apr_pstrdup(pool, buf->st_name);
        }
        else {
            errno = ret;
            return -1;
        }
    }
    return 0;
}
#else
int cstat (const char *path, struct stat *buf, char **casedName, apr_pool_t *pool)
{
    apr_hash_t *statCache = (apr_hash_t *)getStatCache(CpuCurrentProcessor);
    apr_pool_t *gPool = (apr_pool_t *)getGlobalPool(CpuCurrentProcessor);
    apr_stat_entry_t *stat_entry;
    struct stat *info;
    apr_time_t now = apr_time_now();
    char *key;
    int ret;
    int found = 0;

    *casedName = NULL;

    /* If there isn't a global pool then just stat the file
       and return */
    if (!gPool) {
        char poolname[50];

        if (apr_pool_create(&gPool, NULL) != APR_SUCCESS) {
            ret = stat(path, buf);
            if (ret == 0)
                *casedName = case_filename(pool, path);
            return ret;
        }

        sprintf (poolname, "cstat_mem_pool_%d", CpuCurrentProcessor);
        apr_pool_tag(gPool, poolname);

        setGlobalPool(gPool, CpuCurrentProcessor);
    }

    /* If we have a statCache hash table then use it.
       Otherwise we need to create it and initialized it
       with a new mutex lock. */
    if (!statCache) {
        statCache = apr_hash_make(gPool);
        setStatCache((void*)statCache, CpuCurrentProcessor);
    }

    /* If we have a statCache then try to pull the information
       from the cache.  Otherwise just stat the file and return.*/
    if (statCache) {
        stat_entry = (apr_stat_entry_t*) apr_hash_get(statCache, path, APR_HASH_KEY_STRING);
        /* If we got an entry then check the expiration time.  If the entry
           hasn't expired yet then copy the information and return. */
        if (stat_entry) {
            if ((now - stat_entry->expire) <= APR_USEC_PER_SEC) {
                memcpy (buf, &(stat_entry->info), sizeof(struct stat));
                if (stat_entry->casedName)
                    *casedName = apr_pstrdup (pool, stat_entry->casedName);
                else
                    *casedName = case_filename(pool, path);
                found = 1;
            }
        }

        /* Since we are creating a separate stat cache for each processor, we
           don't need to worry about locking the hash table before manipulating
           it. */
        if (!found) {
            /* Bind the thread to the current cpu so that we don't wake
               up on some other cpu and try to manipulate the wrong cache. */
            NXThreadBind (CpuCurrentProcessor);
            ret = stat(path, buf);
            if (ret == 0) {
                *casedName = case_filename(pool, path);
                /* If we don't have a stat_entry then create one, copy
                   the data and add it to the hash table. */
                if (!stat_entry) {
                    key = apr_pstrdup (gPool, path);
                    stat_entry = apr_palloc (gPool, sizeof(apr_stat_entry_t));
                    memcpy (&(stat_entry->info), buf, sizeof(struct stat));
                    if (*casedName)
                        stat_entry->casedName = apr_pstrdup (gPool, *casedName);
                    stat_entry->expire = now;
                    apr_hash_set(statCache, key, APR_HASH_KEY_STRING, stat_entry);
                }
                else {
                    /* If we do have a stat_entry then it must have expired.  Just
                       copy the data and reset the expiration. */
                    memcpy (&(stat_entry->info), buf, sizeof(struct stat));

                    /* If we have a casedName and don't have a cached name or the names don't
                       compare, then cache the name. */
                    if (*casedName && (!stat_entry->casedName || strcmp(*casedName, stat_entry->casedName))) {
                        stat_entry->casedName = apr_pstrdup (gPool, *casedName);
                    }
                    stat_entry->expire = now;
                }
                NXThreadBind (NX_THR_UNBOUND);
            }
            else{
                NXThreadBind (NX_THR_UNBOUND);
                return ret;
			}
        }
    }
    else {
        ret = stat(path, buf);
        if (ret == 0)
            *casedName = case_filename(pool, path);
        return ret;
    }
    return 0;
}
#endif

APR_DECLARE(apr_status_t) apr_stat(apr_finfo_t *finfo, 
                                   const char *fname, 
                                   apr_int32_t wanted, apr_pool_t *pool)
{
    struct stat info;
    int srv;
    char *casedName = NULL;

    srv = cstat(fname, &info, &casedName, pool);

    if (srv == 0) {
        finfo->pool = pool;
        finfo->fname = fname;
        fill_out_finfo(finfo, &info, wanted);
        if (wanted & APR_FINFO_LINK)
            wanted &= ~APR_FINFO_LINK;
        if (wanted & APR_FINFO_NAME) {
            if (casedName) {
                finfo->name = casedName;
                finfo->valid |= APR_FINFO_NAME;
            }
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

