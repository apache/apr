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


#ifndef APR_DARWIN_TYPES_H
#define APR_DARWIN_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Darwin 10's default compiler (gcc42) builds for both 64 and
 * 32 bit architectures unless specifically told not to.
 * In those cases, we need to override types depending on how
 * we're being built at compile time
 */
#ifdef DARWIN_10

#undef APR_OFF_T_STRFN
#undef APR_INT64_STRFN
#undef SIZEOF_LONG
#undef SIZEOF_SIZE_T
#undef SIZEOF_SSIZE_T
#undef SIZEOF_VOIDP

#ifdef __LP64__
 #define APR_INT64_STRFN strtol
 #define SIZEOF_LONG    8
 #define SIZEOF_SIZE_T  8
 #define SIZEOF_SSIZE_T 8
 #define SIZEOF_VOIDP   8
#else
 #define APR_INT64_STRFN strtoll
 #define SIZEOF_LONG    4
 #define SIZEOF_SIZE_T  4
 #define SIZEOF_SSIZE_T 4
 #define SIZEOF_VOIDP   4
#endif

/*
 * ./i386/_types.h:typedef long long __int64_t;
 * ./sys/_types.h:typedef __int64_t	__darwin_off_t;
 * ./sys/types.h:typedef __darwin_off_t		off_t;
 * So off_t is always long long
 */
#undef APR_OFF_T_STRFN
#define APR_OFF_T_STRFN strtoll
 

#undef SETPGRP_VOID
#ifdef __DARWIN_UNIX03
 #define SETPGRP_VOID 1
#else
/* #undef SETPGRP_VOID */
#endif
 
#endif /* DARWIN_10 */

#ifdef __cplusplus
}
#endif

#endif /* APR_DARWIN_TYPES_H */
