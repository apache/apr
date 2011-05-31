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

#include "apr.h"
#include "apr_dso.h"
#include "apu.h"

#ifndef APU_INTERNAL_H
#define APU_INTERNAL_H

#if APR_HAVE_MODULAR_DSO

#ifdef __cplusplus
extern "C" {
#endif

/* For modular dso loading, an internal interlock to allow us to
 * continue to initialize modules by multiple threads, the caller
 * of apu_dso_load must lock first, and not unlock until any init
 * finalization is complete.
 */
apr_status_t apu_dso_init(apr_pool_t *pool);

apr_status_t apu_dso_mutex_lock(void);
apr_status_t apu_dso_mutex_unlock(void);

apr_status_t apu_dso_load(apr_dso_handle_t **dso, apr_dso_handle_sym_t *dsoptr,
                          const char *module, const char *modsym,
                          apr_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* APR_HAVE_MODULAR_DSO */

#endif /* APU_INTERNAL_H */

