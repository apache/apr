/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "shm.h"

#include "apr_general.h"
#include "apr_errno.h"
#include "apr_user.h"
#include "apr_strings.h"

/*
#define APR_WANT_MEMFUNC
#include "apr_want.h"
*/

/*
#include "apr_portable.h"
*/

#if 0
#if APR_HAVE_SHMEM_SHMGET
/* The metadata that is stored in the file that we use to rendevous
 * with the segment in unrelated processes. */
struct apr_shm_shmget_metadata {
    apr_size_t reqsize;   /* requested size of the segment */
};
#endif /* APR_HAVE_SHMEM_SHMGET */
#endif

APR_DECLARE(apr_status_t) apr_shm_create(apr_shm_t **m,
                                         apr_size_t reqsize, 
                                         const char *filename,
                                         apr_pool_t *pool)
{
    apr_shm_t *new_m;
    apr_status_t status;
#if APR_USE_SHMEM_SHMGET
    struct shmid_ds shmbuf;
    apr_uid_t uid;
    apr_gid_t gid;
#endif
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || \
    APR_USE_SHMEM_MMAP_ZERO
    int tmpfd;
#endif
#if APR_USE_SHMEM_SHMGET || APR_USE_SHMEM_SHMGET_ANON
    apr_size_t nbytes;
    apr_file_t *file;   /* file where metadata is stored */
    int shmid;
#endif

    /* FIXME: associate this thing with a pool and set up a destructor
     * to call detach. */

    /* Check if they want anonymous or name-based shared memory */
    if (filename == NULL) {
#if APR_USE_SHMEM_MMAP_ZERO || APR_USE_SHMEM_MMAP_ANON
        new_m = apr_palloc(pool, sizeof(apr_shm_t));
        if (!new_m) {
            return APR_ENOMEM;
        }
        new_m->pool = pool;
        new_m->reqsize = reqsize;
        new_m->realsize = reqsize + sizeof(apr_size_t); /* room for metadata */
        new_m->filename = NULL;
    
#if APR_USE_SHMEM_MMAP_ZERO
        status = apr_file_open(&new_m->file, "/dev/zero", APR_READ | APR_WRITE, 
                               APR_OS_DEFAULT, pool);
        if (status != APR_SUCCESS) {
            return status;
        }
        status = apr_os_file_get(&tmpfd, new_m->file);
        if (status != APR_SUCCESS) {
            return status;
        }

        new_m->base = mmap(NULL, new_m->realsize, PROT_READ|PROT_WRITE,
                           MAP_SHARED, tmpfd, 0);
        if (new_m->base == MAP_FAILED) {
            return errno;
        }

        /* No need to keep the file open after we map it. */
        close(tmpfd);

        /* store the real size in the metadata */
        *(apr_size_t*)(new_m->base) = new_m->realsize;
        /* metadata isn't usable */
        new_m->usable = new_m->base + sizeof(apr_size_t);

        *m = new_m;
        return APR_SUCCESS;

#elif APR_USE_SHMEM_MMAP_ANON
        new_m->base = mmap(NULL, reqsize, PROT_READ|PROT_WRITE,
                           MAP_ANON|MAP_SHARED, -1, 0);
        if (new_m->base == MAP_FAILED) {
            return errno;
        }

        /* store the real size in the metadata */
        *(apr_size_t*)(new_m->base) = new_m->realsize;
        /* metadata isn't usable */
        new_m->usable = (char *)new_m->base + sizeof(apr_size_t);

        *m = new_m;
        return APR_SUCCESS;

#endif /* APR_USE_SHMEM_MMAP_ZERO */
#endif /* APR_USE_SHMEM_MMAP_ZERO || APR_USE_SHMEM_MMAP_ANON */
#if APR_USE_SHMEM_SHMGET_ANON

        new_m = apr_palloc(pool, sizeof(apr_shm_t));
        if (!new_m) {
            return APR_ENOMEM;
        }
        new_m->pool = pool;
        new_m->reqsize = reqsize;
        new_m->realsize = reqsize;
        new_m->filename = NULL;

        if ((shmid = shmget(IPC_PRIVATE, new_m->realsize,
                            SHM_R | SHM_W | IPC_CREAT)) < 0) {
            return errno;
        }

        if ((new_m->base = shmat(shmid, NULL, 0)) < 0) {
            return errno;
        }

        new_m->usable = new_m->base;

        if (shmctl(shmid, IPC_STAT, &shmbuf) == -1) {
            return errno;
        }
        apr_current_userid(&uid, &gid, pool);
        shmbuf.shm_perm.uid = uid;
        shmbuf.shm_perm.gid = gid;
        if (shmctl(shmid, IPC_SET, &shmbuf) == -1) {
            return errno;
        }

        new_m->shmid = shmid;

        *m = new_m;
        return APR_SUCCESS;
#endif /* APR_USE_SHMEM_SHMGET_ANON */
        /* It is an error if they want anonymous memory but we don't have it. */
        return APR_ENOTIMPL; /* requested anonymous but we don't have it */
    }

    /* Name-based shared memory */
    else {
        new_m = apr_palloc(pool, sizeof(apr_shm_t));
        if (!new_m) {
            return APR_ENOMEM;
        }
        new_m->pool = pool;
        new_m->reqsize = reqsize;
        new_m->filename = apr_pstrdup(pool, filename);

#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM
        new_m->realsize = reqsize + sizeof(apr_size_t); /* room for metadata */
        /* FIXME: Ignore error for now. *
         * status = apr_file_remove(file, pool);*/
        status = APR_SUCCESS;
    
#if APR_USE_SHMEM_MMAP_TMP
        /* FIXME: Is APR_OS_DEFAULT sufficient? */
        status = apr_file_open(&new_m->file, filename, 
                               APR_READ | APR_WRITE | APR_CREATE,
                               APR_OS_DEFAULT, pool);
        if (status != APR_SUCCESS) {
            return status;
        }
        if ((status = apr_os_file_get(&tmpfd, new_m->file)) != APR_SUCCESS) {
            return status;
        }
        status = apr_file_trunc(new_m->file, new_m->realsize);
        if (status != APR_SUCCESS) {
            /* FIXME: should we unlink the file here? */
            /* FIXME: should we close the file here? */
            return status;
        }
#elif APR_USE_SHMEM_MMAP_SHM
        /* FIXME: Is APR_OS_DEFAULT sufficient? */
        tmpfd = shm_open(filename, O_RDWR | O_CREAT, APR_OS_DEFAULT);
        if (tmpfd == -1) {
            return errno;
        }

        apr_os_file_put(&new_m->file, &tmpfd, pool); 
        /* FIXME: check for errors */

        status = apr_file_trunc(new_m->file, new_m->realsize);
        if (status != APR_SUCCESS) {
            shm_unlink(filename); /* we're failing, remove the object */
            return status;
        }
#endif /* APR_USE_SHMEM_MMAP_SHM */
        new_m->base = mmap(NULL, reqsize, PROT_READ|PROT_WRITE,
                           MAP_SHARED, tmpfd, 0);

        /* FIXME: check for error */

        /* FIXME: close the file (can we close the file if we're using
         *        shm_open? */

        /* store the real size in the metadata */
        *(apr_size_t*)(new_m->base) = new_m->realsize;
        /* metadata isn't usable */
        new_m->usable = new_m->base + sizeof(apr_size_t);

        *m = new_m;
        return APR_SUCCESS;

#endif /* APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM */

#if APR_USE_SHMEM_SHMGET
        new_m->realsize = reqsize;

        if ((shmid = shmget(ftok(filename, 1), new_m->realsize,
                            SHM_R | SHM_W | IPC_CREAT)) < 0) {
            return errno;
        }

        new_m->base = shmat(shmid, NULL, 0);
        /* FIXME: Handle errors. */
        new_m->usable = new_m->base;

        if (shmctl(shmid, IPC_STAT, &shmbuf) == -1) {
            return errno;
        }
        apr_current_userid(&uid, &gid, pool);
        shmbuf.shm_perm.uid = uid;
        shmbuf.shm_perm.gid = gid;
        if (shmctl(shmid, IPC_SET, &shmbuf) == -1) {
            return errno;
        }

        new_m->shmid = shmid;

        /* FIXME: APR_OS_DEFAULT is too permissive, switch to 600 I think. */
        status = apr_file_open(&file, filename, 
                               APR_WRITE | APR_CREATE,
                               APR_OS_DEFAULT, pool);
        if (status != APR_SUCCESS) {
            return status;
        }

        nbytes = sizeof(reqsize);
        status = apr_file_write(file, (const void *)&reqsize,
                                &nbytes);
        if (status != APR_SUCCESS) {
            return status;
        }
        status = apr_file_close(file);
        if (status != APR_SUCCESS) {
            return status;
        }

        /* Remove the segment once use count hits zero. */
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            return errno;
        }

        *m = new_m; 
        return APR_SUCCESS;

#endif /* APR_USE_SHMEM_SHMGET */
    }

    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shm_t *m)
{
    /* FIXME: do cleanups based on what was allocated, not what was
     *        defined at runtime. */
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || APR_USE_SHMEM_MMAP_ZERO
    munmap(m->base, m->realsize);
    apr_file_close(m->file);
#elif APR_USE_SHMEM_MMAP_ANON
    munmap(m->base, m->realsize);
#elif APR_USE_SHMEM_SHMGET || APR_USE_SHMEM_SHMGET_ANON
    shmdt(m->base);
#endif

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_attach(apr_shm_t **m,
                                         const char *filename,
                                         apr_pool_t *pool)
{
    apr_status_t status;

    if (filename == NULL) {
#if APR_USE_SHMEM_MMAP_ZERO || APR_USE_SHMEM_MMAP_ANON
        /* If they want anonymous memory they shouldn't call attach. */
        return APR_EGENERAL;
#else
        return APR_ENOTIMPL;
#endif
    }
    else {
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM
        int tmpfd;
        struct stat buf;
        apr_shm_t *new_m;

        new_m = apr_palloc(pool, sizeof(apr_shm_t));
        if (!new_m) {
            return APR_ENOMEM;
        }
        new_m->pool = pool;
        new_m->reqsize = reqsize;
        new_m->realsize = reqsize + sizeof(apr_size_t); /* room for metadata */
        new_m->filename = apr_pstrdup(pool, filename);

        /* FIXME: open the file, read the length, mmap the segment,
         *        close the file, reconstruct the apr_shm_t. */
        status = apr_file_open(&new_m->file, filename, 
                               APR_READ | APR_WRITE | APR_CREATE,
                               APR_OS_DEFAULT, pool);
        if (status != APR_SUCCESS) {
            return status;
        }
        if ((status = apr_os_file_get(&tmpfd, new_m->file)) != APR_SUCCESS) {
            return status;
        }
        

        return APR_ENOTIMPL;

#elif APR_USE_SHMEM_SHMGET
        apr_shm_t *new_m;
        apr_file_t *file;   /* file where metadata is stored */
        apr_size_t nbytes;

        new_m = apr_palloc(pool, sizeof(apr_shm_t));
        if (!new_m) {
            return APR_ENOMEM;
        }

        /* FIXME: does APR_OS_DEFAULT matter for reading? */
        status = apr_file_open(&file, filename, 
                               APR_READ, APR_OS_DEFAULT, pool);
        if (status != APR_SUCCESS) {
            return status;
        }

        nbytes = sizeof(new_m->reqsize);
        status = apr_file_read(file, (void *)&(new_m->reqsize),
                               &nbytes);
        if (status != APR_SUCCESS) {
            return status;
        }
        status = apr_file_close(file);
        if (status != APR_SUCCESS) {
            return status;
        }

        new_m->pool = pool;
        if ((new_m->shmid = shmget(ftok(filename, 1), 0,
                                   SHM_R | SHM_W)) < 0) {
            return errno;
        }
        new_m->base = shmat(new_m->shmid, NULL, 0);
        /* FIXME: handle errors */
        new_m->usable = new_m->base;
        new_m->realsize = new_m->reqsize;

        *m = new_m;
        return APR_SUCCESS;

#else
        return APR_ENOTIMPL;
#endif
    }
}

APR_DECLARE(apr_status_t) apr_shm_detach(apr_shm_t *m)
{
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM
    /* FIXME: munmap the segment. */
    return APR_ENOTIMPL;
#elif APR_USE_SHMEM_SHMGET
    /* FIXME: shmdt. */
    return APR_ENOTIMPL;
#else
    return APR_ENOTIMPL;
#endif
}

APR_DECLARE(void *) apr_shm_baseaddr_get(const apr_shm_t *m)
{
    return m->usable;
}

APR_DECLARE(apr_size_t) apr_shm_size_get(const apr_shm_t *m)
{
    return m->reqsize;
}

APR_POOL_IMPLEMENT_ACCESSOR(shm)

