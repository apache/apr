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

#include "locks.h"

#if defined (USE_SYSVSEM_SERIALIZE)  

static struct sembuf op_on;
static struct sembuf op_off;

void ap_unix_setup_lock(void)
{
    op_on.sem_num = 0;
    op_on.sem_op = -1;
    op_on.sem_flg = SEM_UNDO;
    op_off.sem_num = 0;
    op_off.sem_op = 1;
    op_off.sem_flg = SEM_UNDO;
}

static ap_status_t lock_cleanup(void *lock_)
{
    ap_lock_t *lock=lock_;
    union semun ick;
    
    if (lock->interproc != -1) {
        ick.val = 0;
        semctl(lock->interproc, 0, IPC_RMID, ick);
    }
    return APR_SUCCESS;
}    

ap_status_t ap_unix_create_inter_lock(ap_lock_t *new)
{
    union semun ick;
    
    new->interproc = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);

    if (new->interproc < 0) {
        lock_cleanup(new);
        return errno;
    }
    ick.val = 1;
    if (semctl(new->interproc, 0, SETVAL, ick) < 0) {
        lock_cleanup(new);
        return errno;
    }
    new->curr_locked = 0;
    ap_register_cleanup(new->cntxt, (void *)new, lock_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_unix_lock_inter(ap_lock_t *lock)
{
    lock->curr_locked = 1;
    if (semop(lock->interproc, &op_on, 1) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

ap_status_t ap_unix_unlock_inter(ap_lock_t *lock)
{
    if (semop(lock->interproc, &op_off, 1) < 0) {
        return errno;
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

ap_status_t ap_unix_destroy_inter_lock(ap_lock_t *lock)
{
    ap_status_t stat;

    if ((stat = lock_cleanup(lock)) == APR_SUCCESS) {
        ap_kill_cleanup(lock->cntxt, lock, lock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

ap_status_t ap_unix_child_init_lock(ap_lock_t **lock, ap_pool_t *cont, char *fname)
{
    return APR_SUCCESS;
}

#elif defined (USE_PROC_PTHREAD_SERIALIZE)  

void ap_unix_setup_lock(void)
{
}

static ap_status_t lock_cleanup(void *lock_)
{
    ap_lock_t *lock=lock_;

    if (lock->curr_locked == 1) {
        if (pthread_mutex_unlock(lock->interproc)) {
            return errno;
        } 
        if (munmap((caddr_t)lock->interproc, sizeof(pthread_mutex_t))){
            return errno;
        }
    }
    return APR_SUCCESS;
}    

ap_status_t ap_unix_create_inter_lock(ap_lock_t *new)
{
    ap_status_t stat;
    int fd;
    pthread_mutexattr_t mattr;

    fd = open("/dev/zero", O_RDWR);
    if (fd < 0) {
        return errno;
    }

    new->interproc = (pthread_mutex_t *)mmap((caddr_t) 0, 
                              sizeof(pthread_mutex_t), 
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (new->interproc == (pthread_mutex_t *) (caddr_t) -1) {
        return errno;
    }
    close(fd);
    if ((stat = pthread_mutexattr_init(&mattr))) {
        lock_cleanup(new);
        return stat;
    }
    if ((stat = pthread_mutexattr_setpshared(&mattr, 
                                              PTHREAD_PROCESS_SHARED))) {
        lock_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutex_init(new->interproc, &mattr))) {
        lock_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutexattr_destroy(&mattr))) {
        lock_cleanup(new);
        return stat;
    }

    new->curr_locked = 0;
    ap_register_cleanup(new->cntxt, (void *)new, lock_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_unix_lock_inter(ap_lock_t *lock)
{
    ap_status_t stat;
    lock->curr_locked = 1;
    if (stat = pthread_mutex_lock(lock->interproc)) {
        return stat;
    }
    return APR_SUCCESS;
}

ap_status_t ap_unix_unlock_inter(ap_lock_t *lock)
{
    ap_status_t stat;

    if (stat = pthread_mutex_unlock(lock->interproc)) {
        return stat;
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

ap_status_t ap_unix_destroy_inter_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if ((stat = lock_cleanup(lock)) == APR_SUCCESS) {
        ap_kill_cleanup(lock->cntxt, lock, lock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

ap_status_t ap_unix_child_init_lock(ap_lock_t **lock, ap_pool_t *cont, char *fname)
{
    return APR_SUCCESS;
}

#elif defined (USE_FCNTL_SERIALIZE)  

static struct flock lock_it;
static struct flock unlock_it;

void ap_unix_setup_lock(void)
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

static ap_status_t lock_cleanup(void *lock_)
{
    ap_lock_t *lock=lock_;

    if (lock->curr_locked == 1) {
        if (fcntl(lock->interproc, F_SETLKW, &unlock_it) < 0) {
            return errno;
        }
        lock->curr_locked=0;
    }
    return APR_SUCCESS;
}    

ap_status_t ap_unix_create_inter_lock(ap_lock_t *new)
{
    new->interproc = open(new->fname, O_CREAT | O_WRONLY | O_EXCL, 0644);

    if (new->interproc < 0) {
        lock_cleanup(new);
        return errno;
    }

    new->curr_locked=0;
    unlink(new->fname);
    ap_register_cleanup(new->cntxt, new, lock_cleanup, ap_null_cleanup);
    return APR_SUCCESS; 
}

ap_status_t ap_unix_lock_inter(ap_lock_t *lock)
{
    lock->curr_locked=1;
    if (fcntl(lock->interproc, F_SETLKW, &lock_it) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

ap_status_t ap_unix_unlock_inter(ap_lock_t *lock)
{
    if (fcntl(lock->interproc, F_SETLKW, &unlock_it) < 0) {
        return errno;
    }
    lock->curr_locked=0;
    return APR_SUCCESS;
}

ap_status_t ap_unix_destroy_inter_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if ((stat = lock_cleanup(lock)) == APR_SUCCESS) {
        ap_kill_cleanup(lock->cntxt, lock, lock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

ap_status_t ap_unix_child_init_lock(ap_lock_t **lock, ap_pool_t *cont, 
                                    const char *fname)
{
    return APR_SUCCESS;
}

#elif defined (USE_FLOCK_SERIALIZE)

void ap_unix_setup_lock(void)
{
}

static ap_status_t lock_cleanup(void *lock_)
{
    ap_lock_t *lock=lock_;

    if (lock->curr_locked == 1) {
        if (flock(lock->interproc, LOCK_UN) < 0) {
            return errno;
        }
        lock->curr_locked = 0;
    }
    unlink(lock->fname);
    return APR_SUCCESS;
}    

ap_status_t ap_unix_create_inter_lock(ap_lock_t *new)
{
    new->interproc = open(new->fname, O_CREAT | O_WRONLY | O_EXCL, 0600);

    if (new->interproc < 0) {
        lock_cleanup(new);
        return errno;
    }
    new->curr_locked = 0;
    ap_register_cleanup(new->cntxt, (void *)new, lock_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_unix_lock_inter(ap_lock_t *lock)
{
    lock->curr_locked = 1;
    if (flock(lock->interproc, LOCK_EX) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

ap_status_t ap_unix_unlock_inter(ap_lock_t *lock)
{
    if (flock(lock->interproc, LOCK_UN) < 0) {
        return errno;
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

ap_status_t ap_unix_destroy_inter_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if ((stat = lock_cleanup(lock)) == APR_SUCCESS) {
        ap_kill_cleanup(lock->cntxt, lock, lock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

ap_status_t ap_unix_child_init_lock(ap_lock_t **lock, ap_pool_t *cont, 
                            const char *fname)
{
    ap_lock_t *new;

    new = (ap_lock_t *)ap_palloc(cont, sizeof(ap_lock_t));

    new->fname = ap_pstrdup(cont, fname);
    new->interproc = open(new->fname, O_CREAT | O_WRONLY | O_EXCL, 0600);
    if (new->interproc == -1) {
        destroy_inter_lock(new);
        return errno;
    }
    return APR_SUCCESS;
}

#else
/* No inter-process mutex on this platform.  Use at your own risk */
#define create_inter_lock(x, y)
#define lock_inter(x, y)
#define unlock_inter(x, y)
#define destroy_inter_lock(x, y)
#define child_init_lock(x, y, z)
#endif
