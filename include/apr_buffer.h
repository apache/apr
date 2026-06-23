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

/**
 * @file apr_buffer.h
 * @brief  APR-UTIL Buffer
 */
#ifndef APR_BUFFER_H
#define APR_BUFFER_H

/**
 * @defgroup APR_Util_Buffer Buffer handling
 *
 * An APR buffer is a structure that can contain a zero terminated string, or
 * a non zero terminated block of memory, and allow such structures to be
 * passed around and handled in a memory efficient way.
 *
 * We allow two buffers to be compared without duplicating strings. Memory
 * buffers can be converted to string buffers safely. The contents of buffers
 * can be copied into and out of other systems like caches using memory
 * allocation callbacks.
 * @ingroup APR_Util
 * @{
 */

#include "apr.h"
#include "apr_pools.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * When passing a string to apr_buffer_str_create or apr_buffer_str_set, this
 * value can be passed to indicate a string with unknown length, and have
 * apr_buffer_str_create and apr_buffer_str_set compute the length automatically.
 */
#define APR_BUFFER_STRING     (-1)


/**
 * Perform no encoding on memory buffers during apr_buffer_pstrcat().
 */
#define APR_BUFFER_PLAIN    0
/**
 * Perform base64 encoding on memory buffers during apr_buffer_pstrcat().
 */
#define APR_BUFFER_BASE64  1


/**
 * Structure for efficiently tracking a buffer that could contain
 * a zero terminated string, or a fixed length non zero string.
 */
typedef struct
{
    /** pointer to the data, which could be a string or a memory block. */
    union {
        char *str;
        void *mem;
    } d;

    /** is the data zero terminated */
    unsigned int zero_terminated:1;

    /** size of the data, excluding terminating zero */
#if defined(SIZE_MAX) && SIZE_MAX == APR_UINT64_MAX
    apr_size_t size:63;
#elif defined(SIZE_MAX) && SIZE_MAX == APR_UINT32_MAX
    apr_size_t size:31;
#else
#error sizeof size_t is neither 64 nor 32 bit (SIZE_MAX not defined)
#endif

} apr_buffer_t;


/**
 * Set a apr_buffer_t with non zero terminated memory.
 *
 * @param buf The buffer to allocate to
 * @param mem The memory buffer to assign to the buffer
 * @param len The length of the memory buffer
 * @return APR_SUCCESS, or APR_EINVAL if len overflows.
 */
APR_DECLARE(apr_status_t) apr_buffer_mem_set(apr_buffer_t *buf,
                                             void *mem, apr_size_t len)
                                             __attribute__((nonnull(1)));


/**
 * Create a apr_buffer_t containing non zero terminated memory.
 *
 * The buffer structure is allocated from the pool, while the contents are
 * stored as is. It is the responsibility of the caller to ensure the
 * contents have a lifetime as long as the pool.
 * @param mb The memory buffer returned
 * @param pool The pool to allocate from
 * @param mem The memory to assign to the buffer
 * @param len The length of the memory
 * @return Returns APR_ENOMEM if we could not allocate enough memory,
 *         APR_EINVAL if len overflows, otherwise APR_SUCCESS.
 */
APR_DECLARE(apr_status_t) apr_buffer_mem_create(apr_buffer_t **mb,
                                                apr_pool_t *pool,
                                                void *mem, apr_size_t len)
                                                __attribute__((nonnull(1,2)));

/**
 * Set a apr_buffer_t with a zero terminated string.
 *
 * @param buf The buffer to assign the data to.
 * @param str The zero terminated string to assign to the buffer.
 * @param len The length of the string without terminating zero, or
 * APR_BUFFER_STRING to have the length calculated.
 * @return APR_SUCCESS, or APR_EINVAL if len overflows.
 */
APR_DECLARE(apr_status_t) apr_buffer_str_set(apr_buffer_t *buf,
                                             char *str, apr_ssize_t len)
                                             __attribute__((nonnull(1)));

/**
 * Create a apr_buffer_t containing a zero terminated string.
 *
 * The buffer structure is allocated from the pool, while the contents are
 * stored as is. It is the responsibility of the caller to ensure the
 * contents have a lifetime as long as the pool.
 * @param sb The string buffer returned
 * @param pool The pool to allocate from.
 * @param str The string to assign to the buffer.
 * @param len The length of the string, or APR_BUFFER_STRING to have the length
 * calculated.
 * @return Returns APR_ENOMEM if we could not allocate enough memory,
 *         APR_EINVAL if len overflows, otherwise APR_SUCCESS.
 */
APR_DECLARE(apr_status_t) apr_buffer_str_create(apr_buffer_t **sb,
                                                apr_pool_t *pool,
                                                char *str, apr_ssize_t len)
                                                __attribute__((nonnull(1)));

/**
 * Create a apr_buffer_t containing a NULL payload.
 *
 * The buffer structure is allocated from the pool.
 * @param nb The null buffer returned
 * @param pool The pool to allocate from.
 * @return Returns APR_ENOMEM if we could not allocate enough memory,
 *         otherwise APR_SUCCESS.
 */
APR_DECLARE(apr_status_t) apr_buffer_null_create(apr_buffer_t **nb,
                                                 apr_pool_t *pool)
                                                 __attribute__((nonnull(1)));


/**
 * Does the buffer contain a NULL buffer.
 *
 * If the internal buffer is NULL, 1 is returned, otherwise 0.
 *
 * @param buf The buffer.
 * @return Returns 1 if buffer is null, otherwise 0.
 */
APR_DECLARE(int) apr_buffer_is_null(const apr_buffer_t *buf)
                                    __attribute__((nonnull(1)));


/** 
 * Does the buffer contain a zero terminated string.
 *
 * If the buffer is already zero terminated, 1 is returned, otherwise 0.
 *    
 * @param buf The buffer.
 * @return Returns 1 if zero terminated, otherwise 0.
 */
APR_DECLARE(int) apr_buffer_is_str(const apr_buffer_t *buf)
                                   __attribute__((nonnull(1)));


/**
 * Return the zero terminated string from a buffer containing a
 * string.
 *
 * If the buffer contains a string, the original string
 * is returned.
 *
 * If the buffer contains non zero terminated memory, NULL will be
 * returned.
 *
 * Use this function when we want to be sure you're dealing with
 * a string, and want to avoid duplication.
 * @param buf The string/memory buffer.
 * @return The zero terminated string. Returns NULL if the buffer
 * contains memory.
 */
APR_DECLARE(char *) apr_buffer_str(const apr_buffer_t *buf)
                                   __attribute__((nonnull(1)));


/**
 * Return a copy of the buffer as a zero terminated string allocated from
 * a pool.
 *
 * The memory or string buffer will be copied, as appropriate.
 *
 * Use this function when you need the buffer to become a string with
 * the lifetime of the pool provided.
 * @param pool The pool to allocate from.
 * @param buf The buffer.
 * @return The zero terminated string. Returns NULL if we could not
 *         allocate memory.
 */
APR_DECLARE(char *) apr_buffer_pstrdup(apr_pool_t *pool, const apr_buffer_t *buf)
                                       __attribute__((nonnull(1,2)));


/**
 * Return the non zero terminated string/memory buffer.
 *
 * If a size is provided, the size of the buffer without the terminating zero
 * will be returned.
 *
 * Use this function when you need to pass the content of the buffer to an
 * API requiring an area of memory and a length.
 * @param buf The string/memory buffer.
 * @param size Location to write the size to.
 * @return The memory buffer.
 */
APR_DECLARE(void *) apr_buffer_mem(const apr_buffer_t *buf, apr_size_t *size)
                                   __attribute__((nonnull(1)));


/**
 * Return a copy of the content of a buffer as non zero terminated memory
 * allocated from a pool.
 *
 * If a size is provided, the size of the buffer will be returned.
 * @param pool The pool to allocate from.
 * @param buf The string/memory buffer.
 * @param size Location to write the size to.
 * @return The zero memory buffer.
 */
APR_DECLARE(void *) apr_buffer_pmemdup(apr_pool_t *pool, const apr_buffer_t *buf, apr_size_t *size)
                                             __attribute__((nonnull(1,2)));


/**
 * Return the buffer length.
 *
 * The size of the underlying buffer is returned, excluding the terminating
 * zero if present.
 *
 * Use this function to know the length of the data in the buffer.
 * @param buf The string/memory buffer.
 * @return The size of the buffer, excluding terminating zero if present.
 */
APR_DECLARE(apr_size_t) apr_buffer_len(const apr_buffer_t *buf)
                                       __attribute__((nonnull(1)));


/**
 * Return the allocated length.
 *
 * The size of the underlying buffer is returned, including the terminating
 * zero if present.
 *
 * Use this function when you need to know how much memory the buffer is
 * taking up.
 * @param buf The string/memory buffer.
 * @return The size of the buffer, including terminating zero if present.
 */
APR_DECLARE(apr_size_t) apr_buffer_allocated(const apr_buffer_t *buf)
                                             __attribute__((nonnull(1)));


/**
 * Function called to allocate memory in the buffer functions.
 *
 * This allows buffers to be copied into and out of shared memory, or memory
 * from other systems.
 */
typedef void *(*apr_buffer_alloc)(void *ctx, apr_size_t size);

/**
 * Return a copy of an array of memory buffers.
 *
 * This function allows you to make a copy of one or more buffers, controlling
 * the memory allocation yourself.
 *
 * Use this function to copy buffers, and the contents of the buffers, into and
 * out of a cache.
 * @param out The duplicated buffer array. If APR_ENOMEM is returned the
 * array may be partially duplicated, it is up to the caller to free any
 * memory allocated, and to pass zeroed buffers if needed.
 * @param in The string/memory buffer.
 * @param alloc The function callback to allocate memory for the buffer
 * @param ctx Context to pass to the callback function
 * @param nelts Number of buffers to duplicate
 * @return APR_ENONMEM if the alloc function returned NULL, otherwise APR_SUCCESS
 */
APR_DECLARE(apr_status_t) apr_buffer_arraydup(apr_buffer_t **out,
                                              const apr_buffer_t *in,
                                              apr_buffer_alloc alloc, void *ctx,
                                              int nelts)
                                              __attribute__((nonnull(1,2)));

/**
 * Return a copy of a string/memory buffer.
 *
 * This function allows you to make a copy of a buffer, controlling
 * the memory allocation yourself.
 *
 * Use this function to copy a buffer, and the content of the buffer, into and
 * out of a cache.
 *
 * @param out The duplicated buffer. If APR_ENOMEM is returned the buffer may
 * be partially duplicated, it is up to the caller to free any memory
 * allocated, and to pass zeroed buffers if needed.
 * @param in The string/memory buffer.
 * @param alloc The function callback to allocate memory for the buffer
 * @param ctx Context to pass to the callback function
 * @return APR_ENONMEM if the alloc function returned NULL, otherwise APR_SUCCESS
 */
APR_DECLARE(apr_status_t) apr_buffer_dup(apr_buffer_t **out,
                                         const apr_buffer_t *in,
                                         apr_buffer_alloc alloc, void *ctx)
                                         __attribute__((nonnull(1,2)));

/**
 * Copy the contents a buffer into another buffer.
 *
 * This function allows you to make a copy of the contents of a buffer, into
 * and out of a cache.
 *
 * If the source buffer is NULL, the destination buffer will be assigned NULL
 * as content.
 *
 * If the memory allocator callback is NULL, the contents of the source buffer
 * will be assigned to the destination buffer as is.
 *
 * @param dst The first buffer
 * @param src The second buffer
 * @param alloc The function callback to allocate memory for the buffer
 * @param ctx The context for the callback
 * @return Returns dst, or NULL if alloc is provided and returns NULL.
 */
APR_DECLARE(apr_buffer_t *) apr_buffer_cpy(apr_buffer_t *dst,
                                           const apr_buffer_t *src,
                                           apr_buffer_alloc alloc, void *ctx)
                                           __attribute__((nonnull(1)));

/**
 * Compare two possibly NULL buffers for equality.
 *
 * Each buffer can be either a string or memory buffer, or NULL.
 *
 * Two NULL buffers are considered equal.
 *
 * A string buffer and a memory buffer are considered equal if the length
 * excluding any trailing zero is equal, and the contents without the trailing
 * zero are the same.
 * @param dst The first buffer
 * @param src The second buffer
 * @return Positive, negative, or zero, depending on whether src is greater
 *         than, less than, or equal to dst.
 */
APR_DECLARE(int) apr_buffer_cmp(const apr_buffer_t *dst,
                                const apr_buffer_t *src);

/**
 * Concatenate multiple buffers and return a string.
 *
 * If the buffer contains a string, it will be copied across as is, memory
 * buffers will be transformed by the flags specified before concatenation.
 *
 * This function can be used with an apr_array_header_t.
 *
 * @param p The pool from which to allocate
 * @param buf The buffers to concatenate
 * @param nelts The number of buffers to concatenate
 * @param sep The optional separator between strings
 * @param flags Allow memory buffers to be transformed before concatenation.
 *              APR_BUFFER_NONE copies memory buffer as is. APR_BUFFER_BASE64
 *              applies base64 encoding to the memory buffer.
 * @param nbytes (output) strlen of new string (pass in NULL to omit)
 * @return The new string
 */
APR_DECLARE(char *) apr_buffer_pstrncat(apr_pool_t *p, const apr_buffer_t *buf,
                                        int nelts, const char *sep, int flags,
                                        apr_size_t *nbytes);

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* APR_BUFFER_H */

