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

#include "apr_general.h"
#include "apr_shmem.h"
#include "apr_lock.h"
#include "apr_portable.h"
#include "apr_errno.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"

/*
 * This is the Unix implementation of shared memory.
 *
 * Currently, this code supports the following shared memory techniques:
 *
 * - mmap on a temporary file
 * - mmap/shm_open on a temporary file (POSIX.1)
 * - mmap with MAP_ANON (4.4BSD)
 * - mmap /dev/zero (SVR4)
 * - shmget (SysV)
 * - create_area (BeOS)
 */

#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || APR_USE_SHMEM_MMAP_ZERO || APR_USE_SHMEM_MMAP_ANON
#include <sys/mman.h>
#elif APR_USE_SHMEM_SHMGET
#include <sys/ipc.h>
#include <sys/shm.h>
#if !defined(SHM_R)
#define SHM_R 0400
#endif
#if !defined(SHM_W)
#define SHM_W 0200
#endif
#include <sys/file.h>
#elif APR_USE_SHMEM_BEOS
#include <kernel/OS.h>
#endif

struct shmem_t {
    void *mem;
    void *curmem;
    apr_size_t length;
    apr_lock_t *lock;
    char *filename;
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || APR_USE_SHMEM_MMAP_ZERO
    apr_file_t *file; 
#elif APR_USE_SHMEM_MMAP_ANON
    /* Nothing else. */
#elif APR_USE_SHMEM_SHMGET
    apr_os_file_t file;
#elif APR_USE_SHMEM_BEOS
    area_id areaid; 
#endif
};

APR_DECLARE(apr_status_t) apr_shm_init(apr_shmem_t **m, apr_size_t reqsize, 
                                       const char *filename, apr_pool_t *pool)
{
    apr_shmem_t *new_m;
    void *mem;
#if APR_USE_SHMEM_SHMGET
    struct shmid_ds shmbuf;
    apr_uid_t uid;
    apr_gid_t gid;
#endif
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || APR_USE_SHMEM_MMAP_ZERO
    apr_status_t status;
#endif
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || \
    APR_USE_SHMEM_MMAP_ZERO || APR_USE_SHMEM_SHMGET
    int tmpfd;
#endif

    new_m = apr_palloc(pool, sizeof(apr_shmem_t));
    if (!new_m)
        return APR_ENOMEM;

/* These implementations are very similar except for opening the file. */
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || APR_USE_SHMEM_MMAP_ZERO
    /* FIXME: Ignore error for now. *
     * status = apr_file_remove(filename, pool);*/
    status = APR_SUCCESS;
    
#if APR_USE_SHMEM_MMAP_TMP
    /* FIXME: Is APR_OS_DEFAULT sufficient? */
    status = apr_file_open(&new_m->file, filename, 
                         APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
                         pool);
    if (status != APR_SUCCESS)
        return APR_EGENERAL;

    status = apr_os_file_get(&tmpfd, new_m->file);
    status = apr_file_trunc(new_m->file, reqsize);
    if (status != APR_SUCCESS)
        return APR_EGENERAL;

#elif APR_USE_SHMEM_MMAP_SHM
    /* FIXME: Is APR_OS_DEFAULT sufficient? */
    tmpfd = shm_open(filename, O_RDWR | O_CREAT, APR_OS_DEFAULT);
    if (tmpfd == -1)
        return errno;

    apr_os_file_put(&new_m->file, &tmpfd, pool); 
    status = apr_file_trunc(new_m->file, reqsize);
    if (status != APR_SUCCESS)
    {
        shm_unlink(filename);
        return APR_EGENERAL;
    }
#elif APR_USE_SHMEM_MMAP_ZERO
    status = apr_file_open(&new_m->file, "/dev/zero", APR_READ | APR_WRITE, 
                         APR_OS_DEFAULT, pool);
    if (status != APR_SUCCESS)
        return APR_EGENERAL;
    status = apr_os_file_get(&tmpfd, new_m->file);
#endif

    mem = mmap(NULL, reqsize, PROT_READ|PROT_WRITE, MAP_SHARED, tmpfd, 0);

#elif APR_USE_SHMEM_MMAP_ANON
    mem = mmap(NULL, reqsize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
#elif APR_USE_SHMEM_SHMGET
    tmpfd = shmget(IPC_PRIVATE, reqsize, (SHM_R|SHM_W|IPC_CREAT));
    if (tmpfd == -1)
        return errno;

    new_m->file = tmpfd;

    mem = shmat(new_m->file, NULL, 0);

    /* FIXME: Handle errors. */
    if (shmctl(new_m->file, IPC_STAT, &shmbuf) == -1)
        return errno;

    apr_current_userid(&uid, &gid, pool);
    shmbuf.shm_perm.uid = uid;
    shmbuf.shm_perm.gid = gid;

    if (shmctl(new_m->file, IPC_SET, &shmbuf) == -1)
        return errno;

#elif APR_USE_SHMEM_BEOS
    new_m->area_id = create_area("mm", (void*)&mem, B_ANY_ADDRESS, reqsize, 
                                 B_LAZY_LOCK, B_READ_AREA|B_WRITE_AREA);
    /* FIXME: error code? */
    if (new_m->area_id < 0)
        return APR_EGENERAL;

#endif

    new_m->mem = mem;
    new_m->curmem = mem;
    new_m->length = reqsize;

    apr_lock_create(&new_m->lock, APR_MUTEX, APR_CROSS_PROCESS, NULL, pool);
    if (!new_m->lock)
        return APR_EGENERAL;

    *m = new_m; 
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shmem_t *m)
{
#if APR_USE_SHMEM_MMAP_TMP || APR_USE_SHMEM_MMAP_SHM || APR_USE_SHMEM_MMAP_ZERO
    munmap(m->mem, m->length);
    apr_file_close(m->file);
#elif APR_USE_SHMEM_MMAP_ANON
    munmap(m->mem, m->length);
#elif APR_USE_SHMEM_SHMGET
    shmdt(m->mem);
#elif APR_USE_SHMEM_BEOS
    delete_area(new_m->area_id);
#endif

    return APR_SUCCESS;
}

APR_DECLARE(void *) apr_shm_malloc(apr_shmem_t *m, apr_size_t reqsize)
{
    void *new;
    new = NULL;

    apr_lock_acquire(m->lock);
    /* Do we have enough space? */
    if (((char *)m->curmem - (char *)m->mem + reqsize) <= m->length)
    {
        new = m->curmem;
        m->curmem = (char *)m->curmem + reqsize;
    }
    apr_lock_release(m->lock);
    return new;
}

APR_DECLARE(void *) apr_shm_calloc(apr_shmem_t *m, apr_size_t reqsize) 
{
    void *new = apr_shm_malloc(m, reqsize);
    if (new)
        memset(new, '\0', reqsize);
    return new;
}

APR_DECLARE(apr_status_t) apr_shm_free(apr_shmem_t *shared, void *entity)
{
    /* Without a memory management scheme within our shared memory, it
     * is impossible to implement free. */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_name_get(apr_shmem_t *c, 
                                           apr_shm_name_t **name)
{
#if APR_USES_ANONYMOUS_SHM
    *name = NULL;
    return APR_ANONYMOUS;
/* Currently, we are not supporting name based shared memory on Unix
 * systems.  This may change in the future however, so I will leave
 * this in here for now.  Plus, this gives other platforms a good idea
 * of how to proceed.
 */
#elif APR_USES_FILEBASED_SHM
#elif APR_USES_KEYBASED_SHM
#endif
}

APR_DECLARE(apr_status_t) apr_shm_name_set(apr_shmem_t *c, 
                                           apr_shm_name_t *name)
{
#if APR_USES_ANONYMOUS_SHM
    return APR_ANONYMOUS;
/* Currently, we are not supporting name based shared memory on Unix
 * systems.  This may change in the future however, so I will leave
 * this in here for now.  Plus, this gives other platforms a good idea
 * of how to proceed.
 */
#elif APR_USES_FILEBASED_SHM
#elif APR_USES_KEYBASED_SHM
#endif
}

APR_DECLARE(apr_status_t) apr_shm_open(apr_shmem_t *c)
{
#if APR_USES_ANONYMOUS_SHM

/* we don't need to open shared memory segments in child segments, so 
 * just return immediately.
 */
    return APR_SUCCESS;
/* Currently, we are not supporting name based shared memory on Unix
 * systems.  This may change in the future however, so I will leave
 * this in here for now.  Plus, this gives other platforms a good idea
 * of how to proceed.
 */
#elif APR_USES_FILEBASED_SHM
#elif APR_USES_KEYBASED_SHM
#endif
}

APR_DECLARE(apr_status_t) apr_shm_avail(apr_shmem_t *m, apr_size_t *size)
{
    apr_status_t status;

    status = APR_ENOSHMAVAIL;

    apr_lock_acquire(m->lock);

    *size = m->length - ((char *)m->curmem - (char *)m->mem);
    if (*size)
        status = APR_SUCCESS;

    apr_lock_release(m->lock);
    return status;
}
