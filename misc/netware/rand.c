/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define APR_WANT_MEMFUNC
#include "apr_want.h"
#include "apr_general.h"
#include "apr_private.h"

#if APR_HAS_RANDOM

#include <nks/plat.h>

APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char *buf, 
#ifdef APR_ENABLE_FOR_1_0
                                                    apr_size_t length)
#else
                                                    int length)
#endif
{
    return NXSeedRandom(length, buf);
}

#endif /* APR_HAS_RANDOM */
