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

#include "apr.h"
#include "apr_private.h"
#include "apr_portable.h"
#include "apr_arch_proc_mutex.h"
#include "apr_arch_thread_mutex.h"

APR_DECLARE(apr_status_t) apr_proc_mutex_create(apr_proc_mutex_t **mutex,
                                                const char *fname,
                                                apr_lockmech_e mech,
                                                apr_pool_t *pool)
{
    apr_status_t ret;
    apr_proc_mutex_t *new_mutex = NULL;
    new_mutex = (apr_proc_mutex_t *)apr_pcalloc(pool, sizeof(apr_proc_mutex_t));
	
	if(new_mutex ==NULL) {
        return APR_ENOMEM;
    }     
    
    new_mutex->pool = pool;
    ret = apr_thread_mutex_create(&(new_mutex->mutex), APR_THREAD_MUTEX_DEFAULT, pool);

    if (ret == APR_SUCCESS)
        *mutex = new_mutex;

    return ret;
}

APR_DECLARE(apr_status_t) apr_proc_mutex_child_init(apr_proc_mutex_t **mutex,
                                                    const char *fname,
                                                    apr_pool_t *pool)
{
    return APR_SUCCESS;
}
    
APR_DECLARE(apr_status_t) apr_proc_mutex_lock(apr_proc_mutex_t *mutex)
{
    if (mutex)
        return apr_thread_mutex_lock(mutex->mutex);
    return APR_ENOLOCK;
}

APR_DECLARE(apr_status_t) apr_proc_mutex_trylock(apr_proc_mutex_t *mutex)
{
    if (mutex)
        return apr_thread_mutex_trylock(mutex->mutex);
    return APR_ENOLOCK;
}

APR_DECLARE(apr_status_t) apr_proc_mutex_unlock(apr_proc_mutex_t *mutex)
{
    if (mutex)
        return apr_thread_mutex_unlock(mutex->mutex);
    return APR_ENOLOCK;
}

APR_DECLARE(apr_status_t) apr_proc_mutex_cleanup(void *mutex)
{
    return apr_proc_mutex_destroy(mutex);
}

APR_DECLARE(apr_status_t) apr_proc_mutex_destroy(apr_proc_mutex_t *mutex)
{
    if (mutex)
        return apr_thread_mutex_destroy(mutex->mutex);
    return APR_ENOLOCK;
}

APR_DECLARE(const char *) apr_proc_mutex_lockfile(apr_proc_mutex_t *mutex)
{
    return NULL;
}

APR_DECLARE(const char *) apr_proc_mutex_name(apr_proc_mutex_t *mutex)
{
    return "netwarethread";
}

APR_DECLARE(const char *) apr_proc_mutex_defname(void)
{
    return "netwarethread";
}

APR_POOL_IMPLEMENT_ACCESSOR(proc_mutex)

/* Implement OS-specific accessors defined in apr_portable.h */

apr_status_t apr_os_proc_mutex_get(apr_os_proc_mutex_t *ospmutex,
                                   apr_proc_mutex_t *pmutex)
{
    if (pmutex)
        ospmutex = pmutex->mutex->mutex;
    return APR_ENOLOCK;
}

apr_status_t apr_os_proc_mutex_put(apr_proc_mutex_t **pmutex,
                                   apr_os_proc_mutex_t *ospmutex,
                                   apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

