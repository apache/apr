/* Copyright 2000-2005 The Apache Software Foundation or its licensors, as
 * applicable.
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

#ifndef APR_RELEASE_H
#define APR_RELEASE_H

/**
 * @file apr_release.h
 * @brief APR Versioning Constants
 * 
 * APR's Versioning Constants
 *
 * This file was introduced to support C Preprocessing without the 
 * heavyweight C API or apr.h prerequisite, and can only be expected 
 * to exist in 1.2, 2.x and later versions of APR.
 *
 * Do NOT directly include this file when targeting APR 0.x or 1.x,
 * include apr_version.h instead.  
 */

/* The numeric compile-time version constants. These constants are the
 * authoritative version numbers for APR. 
 */

/** major version 
 * Major API changes that could cause compatibility problems for older
 * programs such as structure size changes.  No binary compatibility is
 * possible across a change in the major version.
 */
#define APR_MAJOR_VERSION       1

/** minor version
 * Minor API changes that do not cause binary compatibility problems.
 * Reset to 0 when upgrading APR_MAJOR_VERSION
 */
#define APR_MINOR_VERSION       2

/** patch level 
 * The Patch Level never includes API changes, simply bug fixes.
 * Reset to 0 when upgrading APR_MINOR_VERSION
 */
#define APR_PATCH_VERSION       0

/** 
 * The symbol APR_IS_DEV_VERSION is only defined for internal,
 * "development" copies of APR.  It is undefined for released versions
 * of APR.
 */
#define APR_IS_DEV_VERSION


#if defined(APR_IS_DEV_VERSION) || defined(DOXYGEN)
/** Internal: string form of the "is dev" flag */
#define APR_IS_DEV_STRING "-dev"
#else
#define APR_IS_DEV_STRING ""
#endif

/* APR_STRINGIFY is defined here, and also in apr_general.h, so wrap it */
#ifndef APR_STRINGIFY
/** Properly quote a value as a string in the C preprocessor */
#define APR_STRINGIFY(n) APR_STRINGIFY_HELPER(n)
/** Helper macro for APR_STRINGIFY */
#define APR_STRINGIFY_HELPER(n) #n
#endif

/** The formatted string of APR's version */
#define APR_VERSION_STRING \
     APR_STRINGIFY(APR_MAJOR_VERSION) "." \
     APR_STRINGIFY(APR_MINOR_VERSION) "." \
     APR_STRINGIFY(APR_PATCH_VERSION) \
     APR_IS_DEV_STRING

/** An alternative formatted string of APR's version */
/* macro for Win32 .rc files using numeric csv representation */
#define APR_VERSION_STRING_CSV APR_MAJOR_VERSION ##, \
                             ##APR_MINOR_VERSION ##, \
                             ##APR_PATCH_VERSION

#endif /* ndef APR_RELEASE_H */
