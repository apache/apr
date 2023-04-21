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

#include "apr_arch_atomic.h"

#ifdef USE_ATOMICS_BUILTINS

#if defined(__i386__) || defined(__x86_64__) \
    || defined(__s390__) || defined(__s390x__)
#define WEAK_MEMORY_ORDERING 0
#else
#define WEAK_MEMORY_ORDERING 1
#endif

APR_DECLARE(apr_status_t) apr_atomic_init(apr_pool_t *p)
{
#if defined (USE_ATOMICS_GENERIC64)
    return apr__atomic_generic64_init(p);
#else
    return APR_SUCCESS;
#endif
}

APR_DECLARE(apr_uint32_t) apr_atomic_read32(volatile apr_uint32_t *mem)
{
#if HAVE__ATOMIC_BUILTINS
    return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
#elif WEAK_MEMORY_ORDERING
    /* No __sync_load() available => apr_atomic_add32(mem, 0) */
    return __sync_fetch_and_add(mem, 0);
#else
    return *mem;
#endif
}

APR_DECLARE(void) apr_atomic_set32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
#if HAVE__ATOMIC_BUILTINS
    __atomic_store_n(mem, val, __ATOMIC_SEQ_CST);
#elif WEAK_MEMORY_ORDERING
    /* No __sync_store() available => apr_atomic_xchg32(mem, val) */
    __sync_synchronize();
    __sync_lock_test_and_set(mem, val);
#else
    *mem = val;
#endif
}

APR_DECLARE(apr_uint32_t) apr_atomic_add32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
#if HAVE__ATOMIC_BUILTINS
    return __atomic_fetch_add(mem, val, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_add(mem, val);
#endif
}

APR_DECLARE(void) apr_atomic_sub32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
#if HAVE__ATOMIC_BUILTINS
    __atomic_fetch_sub(mem, val, __ATOMIC_SEQ_CST);
#else
    __sync_fetch_and_sub(mem, val);
#endif
}

APR_DECLARE(apr_uint32_t) apr_atomic_inc32(volatile apr_uint32_t *mem)
{
#if HAVE__ATOMIC_BUILTINS
    return __atomic_fetch_add(mem, 1, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_add(mem, 1);
#endif
}

APR_DECLARE(int) apr_atomic_dec32(volatile apr_uint32_t *mem)
{
#if HAVE__ATOMIC_BUILTINS
    return __atomic_sub_fetch(mem, 1, __ATOMIC_SEQ_CST);
#else
    return __sync_sub_and_fetch(mem, 1);
#endif
}

APR_DECLARE(apr_uint32_t) apr_atomic_cas32(volatile apr_uint32_t *mem, apr_uint32_t val,
                                           apr_uint32_t cmp)
{
#if HAVE__ATOMIC_BUILTINS
    __atomic_compare_exchange_n(mem, &cmp, val, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return cmp;
#else
    return __sync_val_compare_and_swap(mem, cmp, val);
#endif
}

APR_DECLARE(apr_uint32_t) apr_atomic_xchg32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
#if HAVE__ATOMIC_BUILTINS
    return __atomic_exchange_n(mem, val, __ATOMIC_SEQ_CST);
#else
    __sync_synchronize();
    return __sync_lock_test_and_set(mem, val);
#endif
}

APR_DECLARE(void*) apr_atomic_casptr(void *volatile *mem, void *ptr, const void *cmp)
{
#if HAVE__ATOMIC_BUILTINS
    __atomic_compare_exchange_n(mem, (void *)&cmp, ptr, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return (void *)cmp;
#else
    return (void *)__sync_val_compare_and_swap(mem, (void *)cmp, ptr);
#endif
}

APR_DECLARE(void*) apr_atomic_xchgptr(void *volatile *mem, void *ptr)
{
#if HAVE__ATOMIC_BUILTINS
    return __atomic_exchange_n(mem, ptr, __ATOMIC_SEQ_CST);
#else
    __sync_synchronize();
    return __sync_lock_test_and_set(mem, ptr);
#endif
}

#endif /* USE_ATOMICS_BUILTINS */
