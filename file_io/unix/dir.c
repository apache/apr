/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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
#include "apr_strings.h"
#include "apr_portable.h"

static apr_status_t dir_cleanup(void *thedir)
{
    apr_dir_t *dir = thedir;
    if (closedir(dir->dirstruct) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
} 

apr_status_t apr_dir_open(apr_dir_t **new, const char *dirname, apr_pool_t *cont)
{
    /* On some platforms (e.g., Linux+GNU libc), d_name[] in struct 
     * dirent is declared with enough storage for the name.  On other
     * platforms (e.g., Solaris 8 for Intel), d_name is declared as a
     * one-byte array.  Note: gcc evaluates this at compile time.
     */
    apr_size_t dirent_size = 
        (sizeof((*new)->entry->d_name) > 1 ? 
         sizeof(struct dirent) : sizeof (struct dirent) + 255);

    (*new) = (apr_dir_t *)apr_palloc(cont, sizeof(apr_dir_t));

    (*new)->cntxt = cont;
    (*new)->dirname = apr_pstrdup(cont, dirname);
    (*new)->dirstruct = opendir(dirname);
    (*new)->entry = apr_pcalloc(cont, dirent_size);

    if ((*new)->dirstruct == NULL) {
        return errno;
    }    
    else {
        apr_register_cleanup((*new)->cntxt, (void *)(*new), dir_cleanup,
	                    apr_null_cleanup);
        return APR_SUCCESS;
    }
}

apr_status_t apr_dir_close(apr_dir_t *thedir)
{
    apr_status_t rv;

    if ((rv = dir_cleanup(thedir)) == APR_SUCCESS) {
        apr_kill_cleanup(thedir->cntxt, thedir, dir_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

apr_status_t apr_dir_read(apr_finfo_t *finfo, apr_int32_t wanted,
                          apr_dir_t *thedir)
{
    apr_status_t ret = 0;
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS) \
    && !defined(READDIR_IS_THREAD_SAFE)
    dirent *retent;

    ret = readdir_r(thedir->dirstruct, thedir->entry, &retent);

    /* Avoid the Linux problem where at end-of-directory thedir->entry
     * is set to NULL, but ret = APR_SUCCESS.
     */
    if(!ret || thedir->entry != retent)
        ret = APR_ENOENT;
#else
    thedir->entry = readdir(thedir->dirstruct);
    if (thedir->entry == NULL) {
        /* If NULL was returned, this can NEVER be a success. Can it?! */
        if (errno == APR_SUCCESS) {
            ret = APR_ENOENT;
        }
        else
            ret = errno;
    }
#endif

    /* No valid bit flag to test here - do we want one? */
    finfo->fname = NULL;

    if (ret) {
        finfo->valid = 0;
        return ret;
    }

    /* What we already know */
    /* XXX: Optimize here with d_fileno, d_type etc by platform */
    wanted &= ~(APR_FINFO_NAME);
    if (wanted)
    {
        char fspec[PATH_MAX];
        int off;
        apr_cpystrn(fspec, thedir->dirname, sizeof(fspec));
        off = strlen(fspec);
        if (fspec[off - 1] != '/')
            fspec[off++] = '/';
        apr_cpystrn(fspec + off, thedir->entry->d_name, sizeof(fspec) - off);
        /* ??? Or lstat below?  What is it we really want? */
        ret = apr_stat(finfo, fspec, wanted, thedir->cntxt);
    }
    if (!wanted || ret) {
        finfo->cntxt = thedir->cntxt;
        finfo->valid = 0;
    }
    /* We passed a stack name that is now gone */
    finfo->fname = NULL;
    finfo->valid |= APR_FINFO_NAME;
    /* XXX: Optimize here with d_fileno, d_type etc by platform */
    finfo->name = thedir->entry->d_name;
    return APR_SUCCESS;
}

apr_status_t apr_dir_rewind(apr_dir_t *thedir)
{
    rewinddir(thedir->dirstruct);
    return APR_SUCCESS;
}

apr_status_t apr_make_dir(const char *path, apr_fileperms_t perm, apr_pool_t *cont)
{
    mode_t mode = apr_unix_perms2mode(perm);

    if (mkdir(path, mode) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

apr_status_t apr_remove_dir(const char *path, apr_pool_t *cont)
{
    if (rmdir(path) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

apr_status_t apr_get_os_dir(apr_os_dir_t **thedir, apr_dir_t *dir)
{
    if (dir == NULL) {
        return APR_ENODIR;
    }
    *thedir = dir->dirstruct;
    return APR_SUCCESS;
}

apr_status_t apr_put_os_dir(apr_dir_t **dir, apr_os_dir_t *thedir,
                          apr_pool_t *cont)
{
    if ((*dir) == NULL) {
        (*dir) = (apr_dir_t *)apr_pcalloc(cont, sizeof(apr_dir_t));
        (*dir)->cntxt = cont;
    }
    (*dir)->dirstruct = thedir;
    return APR_SUCCESS;
}

  
