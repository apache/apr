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

#include "apr.h"
#include "apr_strings.h"
#include "locks.h"
#include "fileio.h" /* for apr_mkstemp() */

#if APR_HAS_SYSVSEM_SERIALIZE

static struct sembuf op_on;
static struct sembuf op_off;

static void sysv_setup(void)
{
    op_on.sem_num = 0;
    op_on.sem_op = -1;
    op_on.sem_flg = SEM_UNDO;
    op_off.sem_num = 0;
    op_off.sem_op = 1;
    op_off.sem_flg = SEM_UNDO;
}

static apr_status_t sysv_cleanup(void *lock_)
{
    apr_lock_t *lock=lock_;
    union semun ick;
    
    if (lock->interproc != -1) {
        ick.val = 0;
        semctl(lock->interproc, 0, IPC_RMID, ick);
    }
    return APR_SUCCESS;
}    

static apr_status_t sysv_create(apr_lock_t *new, const char *fname)
{
    union semun ick;
    
    new->interproc = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);

    if (new->interproc < 0) {
        sysv_cleanup(new);
        return errno;
    }
    ick.val = 1;
    if (semctl(new->interproc, 0, SETVAL, ick) < 0) {
        sysv_cleanup(new);
        return errno;
    }
    new->curr_locked = 0;
    apr_pool_cleanup_register(new->pool, (void *)new, sysv_cleanup, 
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

static apr_status_t sysv_acquire(apr_lock_t *lock)
{
    int rc;

    do {
        rc = semop(lock->interproc, &op_on, 1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        return errno;
    }
    lock->curr_locked = 1;
    return APR_SUCCESS;
}

static apr_status_t sysv_release(apr_lock_t *lock)
{
    int rc;

    do {
        rc = semop(lock->interproc, &op_off, 1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        return errno;
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

static apr_status_t sysv_destroy(apr_lock_t *lock)
{
    apr_status_t stat;

    if ((stat = sysv_cleanup(lock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, sysv_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

static apr_status_t sysv_child_init(apr_lock_t **lock, apr_pool_t *cont, const char *fname)
{
    return APR_SUCCESS;
}

const apr_unix_lock_methods_t apr_unix_sysv_methods =
{
#if APR_PROCESS_LOCK_IS_GLOBAL || !APR_HAS_THREADS
    APR_PROCESS_LOCK_MECH_IS_GLOBAL,
#else
    0,
#endif
    sysv_create,
    sysv_acquire,
    NULL, /* no rw lock */
    NULL, /* no rw lock */
    sysv_release,
    sysv_destroy,
    sysv_child_init
};

#endif /* SysV sem implementation */

#if APR_HAS_PROC_PTHREAD_SERIALIZE

static void proc_pthread_setup(void)
{
}

static apr_status_t proc_pthread_cleanup(void *lock_)
{
    apr_lock_t *lock=lock_;
    apr_status_t stat;

    if (lock->curr_locked == 1) {
        if ((stat = pthread_mutex_unlock(lock->pthread_interproc))) {
#ifdef PTHREAD_SETS_ERRNO
            stat = errno;
#endif
            return stat;
        } 
        if (munmap((caddr_t)lock->pthread_interproc, sizeof(pthread_mutex_t))){
            return errno;
        }
    }
    return APR_SUCCESS;
}    

static apr_status_t proc_pthread_create(apr_lock_t *new, const char *fname)
{
    apr_status_t stat;
    int fd;
    pthread_mutexattr_t mattr;

    fd = open("/dev/zero", O_RDWR);
    if (fd < 0) {
        return errno;
    }

    new->pthread_interproc = (pthread_mutex_t *)mmap((caddr_t) 0, 
                                                     sizeof(pthread_mutex_t), 
                                                     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (new->pthread_interproc == (pthread_mutex_t *) (caddr_t) -1) {
        return errno;
    }
    close(fd);
    if ((stat = pthread_mutexattr_init(&mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        proc_pthread_cleanup(new);
        return stat;
    }
    if ((stat = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        proc_pthread_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutex_init(new->pthread_interproc, &mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        proc_pthread_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutexattr_destroy(&mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        proc_pthread_cleanup(new);
        return stat;
    }

    new->curr_locked = 0;
    apr_pool_cleanup_register(new->pool, (void *)new, proc_pthread_cleanup, 
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

static apr_status_t proc_pthread_acquire(apr_lock_t *lock)
{
    apr_status_t stat;

    if ((stat = pthread_mutex_lock(lock->pthread_interproc))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    }
    lock->curr_locked = 1;
    return APR_SUCCESS;
}

static apr_status_t proc_pthread_release(apr_lock_t *lock)
{
    apr_status_t stat;

    if ((stat = pthread_mutex_unlock(lock->pthread_interproc))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

static apr_status_t proc_pthread_destroy(apr_lock_t *lock)
{
    apr_status_t stat;
    if ((stat = proc_pthread_cleanup(lock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, proc_pthread_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

static apr_status_t proc_pthread_child_init(apr_lock_t **lock, apr_pool_t *cont, const char *fname)
{
    return APR_SUCCESS;
}

const apr_unix_lock_methods_t apr_unix_proc_pthread_methods =
{
    APR_PROCESS_LOCK_MECH_IS_GLOBAL,
    proc_pthread_create,
    proc_pthread_acquire,
    NULL, /* no rw lock */
    NULL, /* no rw lock */
    proc_pthread_release,
    proc_pthread_destroy,
    proc_pthread_child_init
};

#endif

#if APR_HAS_FCNTL_SERIALIZE

static struct flock lock_it;
static struct flock unlock_it;

static apr_status_t fcntl_release(apr_lock_t *);

static void fcntl_setup(void)
{
    lock_it.l_whence = SEEK_SET;        /* from current point */
    lock_it.l_start = 0;                /* -"- */
    lock_it.l_len = 0;                  /* until end of file */
    lock_it.l_type = F_WRLCK;           /* set exclusive/write lock */
    lock_it.l_pid = 0;                  /* pid not actually interesting */
    unlock_it.l_whence = SEEK_SET;      /* from current point */
    unlock_it.l_start = 0;              /* -"- */
    unlock_it.l_len = 0;                /* until end of file */
    unlock_it.l_type = F_UNLCK;         /* set exclusive/write lock */
    unlock_it.l_pid = 0;                /* pid not actually interesting */
}

static apr_status_t fcntl_cleanup(void *lock_)
{
    apr_lock_t *lock=lock_;

    if (lock->curr_locked == 1) {
        return fcntl_release(lock);
    }
    return APR_SUCCESS;
}    

static apr_status_t fcntl_create(apr_lock_t *new, const char *fname)
{
    if (fname) {
        new->fname = apr_pstrdup(new->pool, fname);
        new->interproc = open(new->fname, O_CREAT | O_WRONLY | O_EXCL, 0644);
    }
    else {
        new->fname = apr_pstrdup(new->pool, "/tmp/aprXXXXXX");
        new->interproc = apr_mkstemp(new->fname);
    }

    if (new->interproc < 0) {
        fcntl_cleanup(new);
        return errno;
    }

    new->curr_locked=0;
    unlink(new->fname);
    apr_pool_cleanup_register(new->pool, (void*)new, fcntl_cleanup, 
                              apr_pool_cleanup_null);
    return APR_SUCCESS; 
}

static apr_status_t fcntl_acquire(apr_lock_t *lock)
{
    int rc;

    do {
        rc = fcntl(lock->interproc, F_SETLKW, &lock_it);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        return errno;
    }
    lock->curr_locked=1;
    return APR_SUCCESS;
}

static apr_status_t fcntl_release(apr_lock_t *lock)
{
    int rc;

    do {
        rc = fcntl(lock->interproc, F_SETLKW, &unlock_it);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        return errno;
    }
    lock->curr_locked=0;
    return APR_SUCCESS;
}

static apr_status_t fcntl_destroy(apr_lock_t *lock)
{
    apr_status_t stat;
    if ((stat = fcntl_cleanup(lock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, fcntl_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

static apr_status_t fcntl_child_init(apr_lock_t **lock, apr_pool_t *cont, 
                                     const char *fname)
{
    return APR_SUCCESS;
}

const apr_unix_lock_methods_t apr_unix_fcntl_methods =
{
#if APR_PROCESS_LOCK_IS_GLOBAL || !APR_HAS_THREADS
    APR_PROCESS_LOCK_MECH_IS_GLOBAL,
#else
    0,
#endif
    fcntl_create,
    fcntl_acquire,
    NULL, /* no rw lock */
    NULL, /* no rw lock */
    fcntl_release,
    fcntl_destroy,
    fcntl_child_init
};

#endif /* fcntl implementation */

#if APR_HAS_FLOCK_SERIALIZE

static apr_status_t flock_release(apr_lock_t *);

static void flock_setup(void)
{
}

static apr_status_t flock_cleanup(void *lock_)
{
    apr_lock_t *lock=lock_;

    if (lock->curr_locked == 1) {
        return flock_release(lock);
    }
    unlink(lock->fname);
    return APR_SUCCESS;
}    

static apr_status_t flock_create(apr_lock_t *new, const char *fname)
{
    if (fname) {
        new->fname = apr_pstrdup(new->pool, fname);
        new->interproc = open(new->fname, O_CREAT | O_WRONLY | O_EXCL, 0600);
    }
    else {
        new->fname = apr_pstrdup(new->pool, "/tmp/aprXXXXXX");
        new->interproc = apr_mkstemp(new->fname);
    }

    if (new->interproc < 0) {
        flock_cleanup(new);
        return errno;
    }
    new->curr_locked = 0;
    apr_pool_cleanup_register(new->pool, (void *)new, flock_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

static apr_status_t flock_acquire(apr_lock_t *lock)
{
    int rc;

    do {
        rc = flock(lock->interproc, LOCK_EX);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        return errno;
    }
    lock->curr_locked = 1;
    return APR_SUCCESS;
}

static apr_status_t flock_release(apr_lock_t *lock)
{
    int rc;

    do {
        rc = flock(lock->interproc, LOCK_UN);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        return errno;
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

static apr_status_t flock_destroy(apr_lock_t *lock)
{
    apr_status_t stat;
    if ((stat = flock_cleanup(lock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, flock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

static apr_status_t flock_child_init(apr_lock_t **lock, apr_pool_t *cont, 
                                     const char *fname)
{
    apr_lock_t *new;

    new = (apr_lock_t *)apr_palloc(cont, sizeof(apr_lock_t));

    memcpy(new, *lock, sizeof *new);
    new->pool = cont;
    new->fname = apr_pstrdup(cont, fname);
    new->interproc = open(new->fname, O_WRONLY, 0600);
    if (new->interproc == -1) {
        flock_destroy(new);
        return errno;
    }
    *lock = new;
    return APR_SUCCESS;
}

const apr_unix_lock_methods_t apr_unix_flock_methods =
{
#if APR_PROCESS_LOCK_IS_GLOBAL || !APR_HAS_THREADS
    APR_PROCESS_LOCK_MECH_IS_GLOBAL,
#else
    0,
#endif
    flock_create,
    flock_acquire,
    NULL, /* no rw lock */
    NULL, /* no rw lock */
    flock_release,
    flock_destroy,
    flock_child_init
};

#endif /* flock implementation */

void apr_unix_setup_lock(void)
{
#if APR_HAS_SYSVSEM_SERIALIZE
    sysv_setup();
#endif
#if APR_HAS_PROC_PTHREAD_SERIALIZE
    proc_pthread_setup();
#endif
#if APR_HAS_FCNTL_SERIALIZE
    fcntl_setup();
#endif
#if APR_HAS_FLOCK_SERIALIZE
    flock_setup();
#endif
}
