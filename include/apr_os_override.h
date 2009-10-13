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


#ifndef APR_OS_OVERRIDE_H
#define APR_OS_OVERRIDE_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef DARWIN_10

#undef APR_HAS_LARGE_FILES
#undef APR_SIZEOF_VOIDP
#undef APR_INT64_T_FMT
#undef APR_UINT64_T_FMT
#undef APR_UINT64_T_HEX_FMT

#ifdef __LP64__
 #define APR_HAS_LARGE_FILES  0
 #define APR_SIZEOF_VOIDP     8
 #define APR_INT64_T_FMT      "ld"
 #define APR_UINT64_T_FMT     "lu"
 #define APR_UINT64_T_HEX_FMT "lx"
#else
 #define APR_HAS_LARGE_FILES  1
 #define APR_SIZEOF_VOIDP     4
 #define APR_INT64_T_FMT      "lld"
 #define APR_UINT64_T_FMT     "llu"
 #define APR_UINT64_T_HEX_FMT "llx"
#endif

#undef APR_IS_BIGENDIAN
#ifdef __BIG_ENDIAN__
 #define APR_IS_BIGENDIAN	1
#else
 #define APR_IS_BIGENDIAN	0
#endif

/*
 * ./i386/_types.h:typedef long long __int64_t;
 * ./sys/_types.h:typedef __int64_t	__darwin_off_t;
 * ./sys/types.h:typedef __darwin_off_t		off_t;
 * So off_t is always long long
 */
#undef APR_OFF_T_FMT
#define APR_OFF_T_FMT "lld"

#endif /* DARWIN_10 */

#ifdef __cplusplus
}
#endif

#endif /* APR_OS_OVERRIDE_H */
