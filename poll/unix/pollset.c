/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef WIN32
/* POSIX defines 1024 for the FD_SETSIZE */
#define FD_SETSIZE 1024
#endif

#include "apr.h"
#include "apr_poll.h"
#include "apr_time.h"
#include "apr_portable.h"
#include "apr_atomic.h"
#include "apr_arch_file_io.h"
#include "apr_arch_networkio.h"
#include "apr_arch_poll_private.h"
#include "apr_arch_inherit.h"

static apr_pollset_method_e pollset_default_method = POLLSET_DEFAULT_METHOD;

static apr_status_t pollset_cleanup(void *p)
{
    apr_pollset_t *pollset = (apr_pollset_t *) p;
    if (pollset->provider->cleanup) {
        (*pollset->provider->cleanup)(pollset);
    }
    if (pollset->flags & APR_POLLSET_WAKEABLE) {
#if WAKEUP_USES_PIPE
        apr_poll_close_wakeup_pipe(pollset->wakeup_pipe);
#else
        apr_poll_close_wakeup_socket(pollset->wakeup_socket);
#endif
    }

    return APR_SUCCESS;
}

#if defined(HAVE_KQUEUE)
extern const apr_pollset_provider_t *apr_pollset_provider_kqueue;
#endif
#if defined(HAVE_PORT_CREATE)
extern const apr_pollset_provider_t *apr_pollset_provider_port;
#endif
#if defined(HAVE_EPOLL)
extern const apr_pollset_provider_t *apr_pollset_provider_epoll;
#endif
#if defined(HAVE_AIO_MSGQ)
extern const apr_pollset_provider_t *apr_pollset_provider_aio_msgq;
#endif
#if defined(HAVE_POLL)
extern const apr_pollset_provider_t *apr_pollset_provider_poll;
#endif
extern const apr_pollset_provider_t *apr_pollset_provider_select;

static const apr_pollset_provider_t *pollset_provider(apr_pollset_method_e method)
{
    const apr_pollset_provider_t *provider = NULL;
    switch (method) {
        case APR_POLLSET_KQUEUE:
#if defined(HAVE_KQUEUE)
            provider = apr_pollset_provider_kqueue;
#endif
        break;
        case APR_POLLSET_PORT:
#if defined(HAVE_PORT_CREATE)
            provider = apr_pollset_provider_port;
#endif
        break;
        case APR_POLLSET_EPOLL:
#if defined(HAVE_EPOLL)
            provider = apr_pollset_provider_epoll;
#endif
        break;
        case APR_POLLSET_AIO_MSGQ:
#if defined(HAVE_AIO_MSGQ)
            provider = apr_pollset_provider_aio_msgq;
#endif
        break;
        case APR_POLLSET_POLL:
#if defined(HAVE_POLL)
            provider = apr_pollset_provider_poll;
#endif
        break;
        case APR_POLLSET_SELECT:
            provider = apr_pollset_provider_select;
        break;
        case APR_POLLSET_DEFAULT:
        break;
    }
    return provider;
}

APR_DECLARE(apr_status_t) apr_pollset_create_ex(apr_pollset_t **ret_pollset,
                                                apr_uint32_t size,
                                                apr_pool_t *p,
                                                apr_uint32_t flags,
                                                apr_pollset_method_e method)
{
    apr_status_t rv;
    apr_pollset_t *pollset;
    const apr_pollset_provider_t *provider = NULL;

    *ret_pollset = NULL;

 #ifdef WIN32
    /* Favor WSAPoll. */
    if (method == APR_POLLSET_DEFAULT) {
        method = APR_POLLSET_POLL;
    }
 #endif

    if (method == APR_POLLSET_DEFAULT)
        method = pollset_default_method;
    while (provider == NULL) {
        provider = pollset_provider(method);
        if (!provider) {
            if ((flags & APR_POLLSET_NODEFAULT) == APR_POLLSET_NODEFAULT)
                return APR_ENOTIMPL;
            if (method == pollset_default_method)
                return APR_ENOTIMPL;
            method = pollset_default_method;
        }
    }
    if (flags & APR_POLLSET_WAKEABLE) {
        /* Add room for wakeup descriptor */
        size++;
    }

    pollset = apr_palloc(p, sizeof(*pollset));
    pollset->nelts = 0;
    pollset->nalloc = size;
    pollset->pool = p;
    pollset->flags = flags;
    pollset->provider = provider;
    pollset->wakeup_set = 0;

    rv = (*provider->create)(pollset, size, p, flags);
    if (rv == APR_ENOTIMPL) {
        if (method == pollset_default_method) {
            return rv;
        }
        provider = pollset_provider(pollset_default_method);
        if (!provider) {
            return APR_ENOTIMPL;
        }
        rv = (*provider->create)(pollset, size, p, flags);
        if (rv != APR_SUCCESS) {
            return rv;
        }
        pollset->provider = provider;
    }
    else if (rv != APR_SUCCESS) {
        return rv;
    }
    if (flags & APR_POLLSET_WAKEABLE) {
#if WAKEUP_USES_PIPE
        /* Create wakeup pipe */
        if ((rv = apr_poll_create_wakeup_pipe(pollset->pool, &pollset->wakeup_pfd,
                                              pollset->wakeup_pipe))
                != APR_SUCCESS) {
            return rv;
        }
#else
        /* Create wakeup socket */
        if ((rv = apr_poll_create_wakeup_socket(pollset->pool, &pollset->wakeup_pfd,
                                                pollset->wakeup_socket))
            != APR_SUCCESS) {
            return rv;
        }
#endif
        if ((rv = apr_pollset_add(pollset, &pollset->wakeup_pfd)) != APR_SUCCESS) {
            return rv;
        }
    }
    if ((flags & APR_POLLSET_WAKEABLE) || provider->cleanup)
        apr_pool_cleanup_register(p, pollset, pollset_cleanup,
                                  apr_pool_cleanup_null);

    *ret_pollset = pollset;
    return APR_SUCCESS;
}

APR_DECLARE(const char *) apr_pollset_method_name(apr_pollset_t *pollset)
{
    return pollset->provider->name;
}

APR_DECLARE(const char *) apr_poll_method_defname(void)
{
    const apr_pollset_provider_t *provider = NULL;

    provider = pollset_provider(pollset_default_method);
    if (provider)
        return provider->name;
    else
        return "unknown";
}

APR_DECLARE(apr_status_t) apr_pollset_create(apr_pollset_t **pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags)
{
    apr_pollset_method_e method = APR_POLLSET_DEFAULT;
    return apr_pollset_create_ex(pollset, size, p, flags, method);
}

APR_DECLARE(apr_status_t) apr_pollset_destroy(apr_pollset_t * pollset)
{
    if (pollset->flags & APR_POLLSET_WAKEABLE ||
        pollset->provider->cleanup)
        return apr_pool_cleanup_run(pollset->pool, pollset,
                                    pollset_cleanup);
    else
        return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_wakeup(apr_pollset_t *pollset)
{
    if (!(pollset->flags & APR_POLLSET_WAKEABLE))
        return APR_EINIT;

    if (apr_atomic_cas32(&pollset->wakeup_set, 1, 0) == 0) {
#if WAKEUP_USES_PIPE
        return apr_file_putc(1, pollset->wakeup_pipe[1]);
#else
        apr_size_t len = 1;
        return apr_socket_send(pollset->wakeup_socket[1], "\1", &len);
#endif
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_add(apr_pollset_t *pollset,
                                          const apr_pollfd_t *descriptor)
{
    return (*pollset->provider->add)(pollset, descriptor);
}

APR_DECLARE(apr_status_t) apr_pollset_remove(apr_pollset_t *pollset,
                                             const apr_pollfd_t *descriptor)
{
    return (*pollset->provider->remove)(pollset, descriptor);
}

APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    return (*pollset->provider->poll)(pollset, timeout, num, descriptors);
}
