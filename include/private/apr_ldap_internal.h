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

#ifndef APR_LDAP_INTERNAL_H
#define APR_LDAP_INTERNAL_H

#include "apr_private.h"
#include "apr_ldap.h"
#include "apr_skiplist.h"

#ifdef __cplusplus
extern "C" {
#endif

#if APR_HAS_LDAP

/*
 * Include the standard LDAP header files.
 */

#if APR_HAS_MICROSOFT_LDAPSDK
#include <winldap.h>
#include <WinBer.h>
#else
#include <lber.h>
#include <ldap.h>
#endif


/*
 * Make sure the secure LDAP port is defined
 */
#ifndef LDAPS_PORT
#define LDAPS_PORT 636  /* ldaps:/// default LDAP over TLS port */
#endif

/*
 * For ldap function calls that input a size limit on the number of returned elements
 * Some SDKs do not have the define for LDAP_DEFAULT_LIMIT (-1) or LDAP_NO_LIMIT (0)
 * LDAP_DEFAULT_LIMIT is preferred as it allows inheritance from whatever the SDK
 * or process is configured for.
 */
#ifdef LDAP_DEFAULT_LIMIT
#define APR_LDAP_SIZELIMIT LDAP_DEFAULT_LIMIT
#else
#ifdef LDAP_NO_LIMIT
#define APR_LDAP_SIZELIMIT LDAP_NO_LIMIT
#endif
#endif

#ifndef APR_LDAP_SIZELIMIT
#define APR_LDAP_SIZELIMIT 0 /* equivalent to LDAP_NO_LIMIT, and what goes on the wire */
#endif

/*
 * z/OS is missing some defines
 */
#ifndef LDAP_VERSION_MAX
#define LDAP_VERSION_MAX  LDAP_VERSION
#endif
#if APR_HAS_ZOS_LDAPSDK
#define LDAP_VENDOR_NAME "IBM z/OS"
#endif

/*
 * LDAP v2.0 is history.
 */
#if LDAP_VERSION_MAX <= 2
#error Support for LDAP v2.0 toolkits has been removed from apr-util. Please use an LDAP v3.0 toolkit.
#endif 



/* The MS SDK returns LDAP_UNAVAILABLE when the backend has closed the connection
 * between LDAP calls. Protect with APR_HAS_MICROSOFT_LDAPSDK in case someone 
 * manually chooses another SDK on Windows 
 */
#if APR_HAS_MICROSOFT_LDAPSDK
#define APR_LDAP_IS_SERVER_DOWN(s)    ((s) == LDAP_SERVER_DOWN \
                                    || (s) == LDAP_UNAVAILABLE)
#else
#define APR_LDAP_IS_SERVER_DOWN(s)    ((s) == LDAP_SERVER_DOWN)
#endif



/**
 * Macro to detect security related return values.
 */
#if defined(LDAP_INSUFFICIENT_ACCESS)
#define APU_LDAP_INSUFFICIENT_ACCESS LDAP_INSUFFICIENT_ACCESS
#elif defined(LDAP_INSUFFICIENT_RIGHTS)
#define APU_LDAP_INSUFFICIENT_ACCESS LDAP_INSUFFICIENT_RIGHTS
#elif defined(APR_HAS_MICROSOFT_LDAPSDK)
/* The macros above fail to contemplate that LDAP_RETCODE values
 * may be represented by an enum.  autoconf tests would be much
 * more robust.
 */
#define APU_LDAP_INSUFFICIENT_ACCESS LDAP_INSUFFICIENT_RIGHTS
#else
#error The security return codes must be added to support this LDAP toolkit.
#endif

#if defined(LDAP_SECURITY_ERROR)
#define APU_LDAP_SECURITY_ERROR LDAP_SECURITY_ERROR
#else
#define APU_LDAP_SECURITY_ERROR(n)      \
    (LDAP_INAPPROPRIATE_AUTH == n) ? 1 \
    : (LDAP_INVALID_CREDENTIALS == n) ? 1 \
    : (APU_LDAP_INSUFFICIENT_ACCESS == n) ? 1 \
    : 0
#endif


#if APR_HAVE_MODULAR_DSO

/* For LDAP internal builds, wrap our LDAP namespace */

struct apr__ldap_dso_fntable {
    int (*info)(apr_pool_t *pool, apu_err_t **err);
    apr_status_t (*initialise)(apr_pool_t *pool, apr_ldap_t **ldap,
                               apu_err_t *err);
    apr_status_t (*option_get)(apr_pool_t *pool, apr_ldap_t *ldap, int option,
                               apr_ldap_opt_t *outvalue, apu_err_t *err);
    apr_status_t (*option_set)(apr_pool_t *pool, apr_ldap_t *ldap, int option,
                               const apr_ldap_opt_t *invalue, apu_err_t *err);
    apr_status_t (*connect)(apr_pool_t *pool, apr_ldap_t *ldap,
                            apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*prepare)(apr_pool_t *pool, apr_ldap_t *ldap,
                            apr_ldap_prepare_cb prepare_cb,
                            void *prepare_ctx);
    apr_status_t (*process)(apr_pool_t *pool, apr_ldap_t *ldap,
                            apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*result)(apr_pool_t *pool, apr_ldap_t *ldap,
                           apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*poll)(apr_pool_t *pool, apr_ldap_t *ldap, apr_pollcb_t *poll,
                         apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*bind)(apr_pool_t *pool, apr_ldap_t *ldap,
                         const char *mech, apr_ldap_bind_interact_cb *interact_cb,
                         void *interact_ctx, apr_interval_time_t timeout,
                         apr_ldap_bind_cb bind_cb, void *bind_ctx,
                         apu_err_t *err);
    apr_status_t (*compare)(apr_pool_t *pool, apr_ldap_t *ldap,
                            const char *dn, const char *attr,
                            const apr_buffer_t *bval,
                            apr_array_header_t *serverctrls,
                            apr_array_header_t *clientctrls,
                            apr_interval_time_t timeout,
                            apr_ldap_compare_cb compare_cb, void *ctx, apu_err_t *err);
    apr_status_t (*search)(apr_pool_t *pool, apr_ldap_t *ldap, const char *dn,
                           apr_ldap_search_scope_e scope, const char *filter,
                           const char **attrs, apr_ldap_switch_e attrsonly,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout, apr_ssize_t sizelimit,
                           apr_ldap_search_result_cb search_result_cb,          
                           apr_ldap_search_entry_cb search_entry_cb,                                          
                           void *search_ctx, apu_err_t *err);
    apr_status_t (*add)(apr_pool_t *pool, apr_ldap_t *ldap,
                        const char *dn, apr_array_header_t *adds,
                        apr_array_header_t *serverctrls,
                        apr_array_header_t *clientctrls,
                        apr_interval_time_t timeout,
                        apr_ldap_add_cb add_cb, void *ctx, apu_err_t *err);
    apr_status_t (*modify)(apr_pool_t *pool, apr_ldap_t *ldap,
                           const char *dn, apr_array_header_t *mods,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout,
                           apr_ldap_modify_cb modify_cb, void *ctx, apu_err_t *err);
    apr_status_t (*rename)(apr_pool_t *pool, apr_ldap_t *ldap,
                           const char *dn, const char *newrdn, const char *newparent,
                           apr_ldap_rename_e flags,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout,
                           apr_ldap_rename_cb rename_cb, void *ctx, apu_err_t *err);
    apr_status_t (*delete)(apr_pool_t *pool, apr_ldap_t *ldap,
                           const char *dn,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout,
                           apr_ldap_delete_cb delete_cb, void *ctx, apu_err_t *err);
    apr_status_t (*extended)(apr_pool_t *pool, apr_ldap_t *ldap,
                             const char *dn, apr_buffer_t *data,
                             apr_array_header_t *serverctrls,
                             apr_array_header_t *clientctrls,
                             apr_interval_time_t timeout,
                             apr_ldap_extended_cb ext_cb, void *ctx, apu_err_t *err);
    apr_status_t (*unbind)(apr_ldap_t *ldap, apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls, apu_err_t *err);
};

#endif /* APR_HAVE_MODULAR_DSO */


#endif

#ifdef __cplusplus
}
#endif

#endif

