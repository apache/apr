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

#include "apr_arch_threadproc.h"

static struct beos_key key_table[BEOS_MAX_DATAKEYS];
static struct beos_private_data *beos_data[BEOS_MAX_DATAKEYS];
static sem_id lock;

APR_DECLARE(apr_status_t) apr_threadkey_private_create(apr_threadkey_t **key,
                                       void (*dest)(void *), apr_pool_t *pool)
{
    (*key) = (apr_threadkey_t *)apr_palloc(pool, sizeof(apr_threadkey_t));
    if ((*key) == NULL) {
        return APR_ENOMEM;
    }

    (*key)->pool = pool;
    	
	acquire_sem(lock);
	for ((*key)->key=0; (*key)->key < BEOS_MAX_DATAKEYS; (*key)->key++){
		if (key_table[(*key)->key].assigned == 0){
			key_table[(*key)->key].assigned = 1;
			key_table[(*key)->key].destructor = dest;
			release_sem(lock);
			return APR_SUCCESS;
		}				

	}
	release_sem(lock);
    return APR_ENOMEM;
}

APR_DECLARE(apr_status_t) apr_threadkey_private_get(void **new, apr_threadkey_t *key)
{
	thread_id tid;
	int i, index=0;
	tid = find_thread(NULL);
	for (i=0;i<BEOS_MAX_DATAKEYS;i++){
		if (beos_data[i]->data){
			/* it's been used */
			if (beos_data[i]->td == tid){
				index = i;
			}
		}
	}
	if (index == 0){
		/* no storage for thread so we can't get anything... */
		return APR_ENOMEM;
	}

	if ((key->key < BEOS_MAX_DATAKEYS) && (key_table)){
		acquire_sem(key_table[key->key].lock);
		if (key_table[key->key].count){
			(*new) = (void*)beos_data[index]->data[key->key];
		} else {
			(*new) = NULL;
		}
		release_sem(key_table[key->key].lock);
	} else {
		(*new) = NULL;
	}
	return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadkey_private_set(void *priv, apr_threadkey_t *key)
{
	thread_id tid;
	int i,index = 0, ret = 0;

	tid = find_thread(NULL);	
	for (i=0; i < BEOS_MAX_DATAKEYS; i++){
		if (beos_data[i]->data){
			if (beos_data[i]->td == tid){index = i;}
		}
	}
	if (index==0){
		/* not yet been allocated */
		for (i=0; i< BEOS_MAX_DATAKEYS; i++){
			if (! beos_data[i]->data){
				/* we'll take this one... */
				index = i;
				beos_data[i]->data = (const void **)malloc(sizeof(void *) * BEOS_MAX_DATAKEYS);
				memset((void *)beos_data[i]->data, 0, sizeof(void *) * BEOS_MAX_DATAKEYS);
				beos_data[i]->count = (int)malloc(sizeof(int));
				beos_data[i]->td = (thread_id)malloc(sizeof(thread_id));
				beos_data[i]->td = tid;
			}
		}
	}
	if (index == 0){
		/* we're out of luck.. */
		return APR_ENOMEM;
	}
	if ((key->key < BEOS_MAX_DATAKEYS) && (key_table)){
		acquire_sem(key_table[key->key].lock);
		if (key_table[key->key].count){
			if (beos_data[index]->data[key->key] == NULL){
				if (priv != NULL){
					beos_data[index]->count++;
					key_table[key->key].count++;
				}
			} else {
				if (priv == NULL){
					beos_data[index]->count--;
					key_table[key->key].count--;
				}
			}
			beos_data[index]->data[key->key] = priv;
			ret = 1;
		} else {
			ret = 0;
		}
		release_sem(key_table[key->key].lock);
	}
	if (ret)
    	return APR_SUCCESS;
	return APR_ENOMEM;
}

APR_DECLARE(apr_status_t) apr_threadkey_private_delete(apr_threadkey_t *key)
{
	if (key->key < BEOS_MAX_DATAKEYS){
		acquire_sem(key_table[key->key].lock);
		if (key_table[key->key].count == 1){
			key_table[key->key].destructor = NULL;
			key_table[key->key].count = 0;
		}
		release_sem(key_table[key->key].lock);
	} else {
		return APR_ENOMEM;
	}
	return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadkey_data_get(void **data, const char *key,
                                                 apr_threadkey_t *threadkey)
{
    return apr_pool_userdata_get(data, key, threadkey->pool);
}

APR_DECLARE(apr_status_t) apr_threadkey_data_set(void *data, const char *key,
                                                 apr_status_t (*cleanup) (void *),
                                                 apr_threadkey_t *threadkey)
{
    return apr_pool_userdata_set(data, key, cleanup, threadkey->pool);
}

APR_DECLARE(apr_status_t) apr_os_threadkey_get(apr_os_threadkey_t *thekey, apr_threadkey_t *key)
{
    *thekey = key->key;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_threadkey_put(apr_threadkey_t **key, 
                                               apr_os_threadkey_t *thekey, apr_pool_t *pool)
{
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    if ((*key) == NULL) {
        (*key) = (apr_threadkey_t *)apr_pcalloc(pool, sizeof(apr_threadkey_t));
        (*key)->pool = pool;
    }
    (*key)->key = *thekey;
    return APR_SUCCESS;
}
