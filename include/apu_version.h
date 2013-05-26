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

#ifndef APU_VERSION_H
#define APU_VERSION_H

/**
 * @file apu_version.h
 * @brief APR-util Versioning Interface
 * 
 * APR-util's Version
 *
 * There are several different mechanisms for accessing the version. There
 * is a string form, and a set of numbers; in addition, there are constants
 * which can be compiled into your application, and you can query the library
 * being used for its actual version.
 *
 * Note that it is possible for an application to detect that it has been
 * compiled against a different version of APU by use of the compile-time
 * constants and the use of the run-time query function.
 *
 * APU version numbering follows the guidelines specified in:
 *
 *     http://apr.apache.org/versioning.html
 */


#include "apr_version.h"

/* The numeric compile-time version constants. These constants are the
 * authoritative version numbers for APU. This file remains as strictly
 * a compatibility stub.
 */

/** major version 
 * Major API changes that could cause compatibility problems for older
 * programs such as structure size changes.  No binary compatibility is
 * possible across a change in the major version.
 * In 2.0, for legacy support, this is an identity of the APR version.
 * @deprecated @see APR_MAJOR_VERSION
 */
#define APU_MAJOR_VERSION       APR_MAJOR_VERSION

/** minor version
 * Minor API changes that do not cause binary compatibility problems.
 * Reset to 0 when upgrading APU_MAJOR_VERSION
 * In 2.0, for legacy support, this is an identity of the APR version.
 * @deprecated @see APR_MINOR_VERSION
 */
#define APU_MINOR_VERSION       APR_MINOR_VERSION

/** patch level 
 * The Patch Level never includes API changes, simply bug fixes.
 * Reset to 0 when upgrading APR_MINOR_VERSION
 * In 2.0, for legacy support, this is an identity of the APR version.
 * @deprecated @see APR_PATCH_VERSION
 */
#define APU_PATCH_VERSION       APR_PATCH_VERSION

/** 
 * The symbol APU_IS_DEV_VERSION is only defined for internal,
 * "development" copies of APU.  It is undefined for released versions
 * of APU.
 * In 2.0, for legacy support, this is an identity of the APR version.
 * @deprecated @see APR_IS_DEV_VERSION
 */
#ifdef APR_IS_DEV_VERSION
#  define APU_IS_DEV_VERSION
#endif

/** Internal: string form of the "is dev" flag */
#define APU_IS_DEV_STRING       APR_IS_DEV_STRING

#endif /* ndef APU_VERSION_H */
