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

#include "apr_strings.h"
#include "apr_arch_file_io.h"

/* A file to put ALL of the accessor functions for apr_file_t types. */

APR_DECLARE(apr_status_t) apr_file_name_get(const char **fname,
                                           apr_file_t *thefile)
{
    *fname = thefile->fname;
    return APR_SUCCESS;
}

APR_DECLARE(apr_int32_t) apr_file_flags_get(apr_file_t *f)
{
    return f->flags;
}

#if !defined(OS2) && !defined(WIN32)
mode_t apr_unix_perms2mode(apr_fileperms_t perms)
{
    mode_t mode = 0;

    if (perms & APR_UREAD)
        mode |= S_IRUSR;
    if (perms & APR_UWRITE)
        mode |= S_IWUSR;
    if (perms & APR_UEXECUTE)
        mode |= S_IXUSR;

    if (perms & APR_GREAD)
        mode |= S_IRGRP;
    if (perms & APR_GWRITE)
        mode |= S_IWGRP;
    if (perms & APR_GEXECUTE)
        mode |= S_IXGRP;

    if (perms & APR_WREAD)
        mode |= S_IROTH;
    if (perms & APR_WWRITE)
        mode |= S_IWOTH;
    if (perms & APR_WEXECUTE)
        mode |= S_IXOTH;

    return mode;
}

apr_fileperms_t apr_unix_mode2perms(mode_t mode)
{
    apr_fileperms_t perms = 0;

    if (mode & S_IRUSR)
        perms |= APR_UREAD;
    if (mode & S_IWUSR)
        perms |= APR_UWRITE;
    if (mode & S_IXUSR)
        perms |= APR_UEXECUTE;

    if (mode & S_IRGRP)
        perms |= APR_GREAD;
    if (mode & S_IWGRP)
        perms |= APR_GWRITE;
    if (mode & S_IXGRP)
        perms |= APR_GEXECUTE;

    if (mode & S_IROTH)
        perms |= APR_WREAD;
    if (mode & S_IWOTH)
        perms |= APR_WWRITE;
    if (mode & S_IXOTH)
        perms |= APR_WEXECUTE;

    return perms;
}
#endif

APR_DECLARE(apr_status_t) apr_file_data_get(void **data, const char *key,
                                           apr_file_t *file)
{    
    return apr_pool_userdata_get(data, key, file->pool);
}

APR_DECLARE(apr_status_t) apr_file_data_set(apr_file_t *file, void *data,
                                           const char *key,
                                           apr_status_t (*cleanup)(void *))
{    
    return apr_pool_userdata_set(data, key, cleanup, file->pool);
}
