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
#include "apu.h"
#include "apr_private.h"

#if APR_HAVE_MODULAR_DSO
#define APU_DSO_LDAP_BUILD
#endif

#include "apr_ldap.h"
#include "apr_ldap_internal.h"
#include "apu_internal.h"
#include "apu_ldap_internal.h"
#include "apr_errno.h"
#include "apr_poll.h"
#include "apr_pools.h"
#include "apr_portable.h"
#include "apr_cstr.h"
#include "apr_strings.h"
#include "apr_escape.h"
#include "apr_hash.h"

#define APR_WANT_MEMFUNC
#include "apr_want.h"

#if APR_HAS_LDAP


#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#else
#ifdef HAVE_SASL_H
#include <sasl.h>
#endif
#endif

#include <assert.h>

/* Older APIs don't define this from RFC4520 */
#ifndef LDAP_MOD_INCREMENT
#define LDAP_MOD_INCREMENT 3
#endif

/* Older APIs use LDAP_RES_MODRDN instead of LDAP_RES_RENAME */
#ifndef LDAP_RES_RENAME
#define LDAP_RES_RENAME LDAP_RES_MODRDN
#endif

/* Older APIs use LDAP_RES_REFERRAL instead of LDAP_RES_SEARCH_REFERENCE */
#ifndef LDAP_RES_SEARCH_REFERENCE
#define LDAP_RES_SEARCH_REFERENCE LDAP_RES_REFERRAL
#endif

#if APR_HAS_MICROSOFT_LDAPSDK
#define MSGID_T ULONG
#define TO_BV_LEN(a) (ULONG)(a)
#else
#define MSGID_T int
#define TO_BV_LEN(a) (a)
#endif

struct apr_ldap_t {
    apr_pool_t *pool;
#if !APR_HAS_OPENLDAP_LDAPSDK
    const char *uri;
#endif
    LDAP *ld;
    apr_socket_t *socket;
    apr_skiplist *results;
    apr_array_header_t *abandons;
    apr_array_header_t *prepares;
    LDAPControl **serverctrls;
    LDAPControl **clientctrls;
    apu_err_t err;
    apr_status_t status;
};


typedef struct apr_ldap_prepare_t {
    apr_pool_t *pool;
    apr_ldap_prepare_cb cb;
    void *ctx;
} apr_ldap_prepare_t;


typedef struct apr_ldap_result_t {
    apr_pool_t *pool;
    apr_ldap_t *ld;
    const char *mech;
    const char *rmech;
    LDAPMessage *message;
    MSGID_T msgid;
    int msgtype;
    union {
        apr_ldap_bind_cb bind;
        apr_ldap_compare_cb compare;
        apr_ldap_search_result_cb search;
        apr_ldap_add_cb add;
        apr_ldap_modify_cb modify;
        apr_ldap_rename_cb rename;
        apr_ldap_delete_cb delete;
        apr_ldap_extended_cb ext;
    } cb;
    union {
        apr_ldap_search_entry_cb search;
    } entry_cb;
    void *ctx;
    apr_size_t nentries;
} apr_ldap_result_t;


static apr_status_t apr_ldap_status(int rc)
{

    if (LDAP_SUCCESS == rc) {
        return APR_SUCCESS;
    }

    switch (rc) {
    case LDAP_SUCCESS:
        return APR_SUCCESS;

    case LDAP_LOCAL_ERROR:
        return APR_LOCAL_ERROR;

    case LDAP_ENCODING_ERROR:
        return APR_ENCODING_ERROR;

    case LDAP_DECODING_ERROR:
        return APR_DECODING_ERROR;

    case LDAP_TIMEOUT:
        return APR_TIMEOUT;

    case LDAP_AUTH_UNKNOWN:
        return APR_AUTH_UNKNOWN;

    case LDAP_FILTER_ERROR:
        return APR_FILTER_ERROR;

    case LDAP_USER_CANCELLED:
        return APR_USER_CANCELLED;

    case LDAP_PARAM_ERROR:
        return APR_PARAM_ERROR;

    case LDAP_NO_MEMORY:
        return APR_NO_MEMORY;

    case LDAP_CONNECT_ERROR:
        return APR_CONNECT_ERROR;

    case LDAP_NOT_SUPPORTED:
        return APR_NOT_SUPPORTED;

    case LDAP_CONTROL_NOT_FOUND:
        return APR_CONTROL_NOT_FOUND;

    case LDAP_NO_RESULTS_RETURNED:
        return APR_NO_RESULTS_RETURNED;

    case LDAP_MORE_RESULTS_TO_RETURN:
        return APR_MORE_RESULTS_TO_RETURN;

    case LDAP_CLIENT_LOOP:
        return APR_CLIENT_LOOP;

    case LDAP_REFERRAL_LIMIT_EXCEEDED:
        return APR_REFERRAL_LIMIT_EXCEEDED;

#ifdef LDAP_X_CONNECTING
    case LDAP_X_CONNECTING:
        return APR_CONNECTING;
#endif

#if defined(LDAP_SERVER_DOWN)
    case LDAP_SERVER_DOWN:
        return APR_SERVER_DOWN;
#endif

#if defined(LDAP_UNAVAILABLE)    
    case LDAP_UNAVAILABLE:
        return APR_UNAVAILABLE;
#endif

#ifdef LDAP_X_PROXY_AUTHZ_FAILURE
    case LDAP_X_PROXY_AUTHZ_FAILURE:
        return APR_PROXY_AUTH;
#endif

    case LDAP_INAPPROPRIATE_AUTH:
        return APR_INAPPROPRIATE_AUTH;

    case LDAP_INVALID_CREDENTIALS:
        return APR_INVALID_CREDENTIALS;

#ifdef LDAP_INSUFFICIENT_ACCESS
    /* openldap */
    case LDAP_INSUFFICIENT_ACCESS:
        return APR_INSUFFICIENT_ACCESS;
#endif

#ifdef LDAP_INSUFFICIENT_RIGHTS
    /* microsoftsdk */
    case LDAP_INSUFFICIENT_RIGHTS:
        return APR_INSUFFICIENT_ACCESS;
#endif

#ifdef LDAP_CONSTRAINT_VIOLATION
    case LDAP_CONSTRAINT_VIOLATION:
        return APR_CONSTRAINT_VIOLATION;
#endif

#ifdef LDAP_OBJECT_CLASS_VIOLATION
    case LDAP_OBJECT_CLASS_VIOLATION:
        return APR_OBJECT_CLASS_VIOLATION;
#endif

    case LDAP_COMPARE_TRUE:
        return APR_COMPARE_TRUE;

    case LDAP_COMPARE_FALSE:
        return APR_COMPARE_FALSE;

    case LDAP_NO_SUCH_OBJECT:
        return APR_NO_SUCH_OBJECT;

    case LDAP_NO_SUCH_ATTRIBUTE:
        return APR_NO_SUCH_ATTRIBUTE;

    case LDAP_ALREADY_EXISTS:
        return APR_ALREADY_EXISTS;

    case LDAP_OPERATIONS_ERROR:
        return APR_OPERATIONS_ERROR;

    case LDAP_PROTOCOL_ERROR:
        return APR_PROTOCOL_ERROR;

    case LDAP_TIMELIMIT_EXCEEDED:
        return APR_TIMELIMIT_EXCEEDED;

    case LDAP_SIZELIMIT_EXCEEDED:
        return APR_SIZELIMIT_EXCEEDED;

    default:
        return APR_UTIL_START_STATUS + 200 + rc;
    }

}


/**
 * APR LDAP info function
 *
 * This function returns a string describing the LDAP toolkit
 * currently in use. The string is placed inside result_err->reason.
 */
APU_DECLARE_LDAP(int) apr_ldap_info(apr_pool_t *pool,
                                    apu_err_t **result_err)
{
    apu_err_t *result;

    if (!(result = *result_err)) {
        result = (apu_err_t *)apr_pcalloc(pool, sizeof(apu_err_t));
        *result_err = result;
    }

    result->reason = "APR LDAP: Built with "
                     LDAP_VENDOR_NAME
                     " LDAP SDK";
    return APR_SUCCESS;
}

static apr_status_t results_cleanup(void *dptr);

static apr_status_t prepare_cleanup(void *dptr)
{
    apr_ldap_prepare_t *prepare = dptr;

    prepare->pool = NULL;
    prepare->cb = NULL;
    prepare->ctx = NULL;

    return APR_SUCCESS;
}

static apr_status_t apr_ldap_cleanup(void *dptr)
{
    if (dptr) {

        apr_ldap_t *ldap = dptr;

        if (ldap->results) {
            results_cleanup(ldap->results);
            ldap->results = NULL;
        }

        if (ldap->ld) {
#if APR_HAS_OPENLDAP_LDAPSDK
            ldap->status = ldap_unbind_ext(ldap->ld, ldap->serverctrls, ldap->clientctrls);
#else
            ldap->status = ldap_unbind(ldap->ld);
#endif
            ldap->serverctrls = NULL;
            ldap->clientctrls = NULL;
            ldap->ld = NULL;
        }

        while (ldap->prepares->nelts) {
            apr_ldap_prepare_t *prepare = apr_array_pop(ldap->prepares);
            if (prepare->pool) {
                apr_pool_cleanup_run(prepare->pool, prepare, prepare_cleanup);
            }
        }

    }

    return APR_SUCCESS;
}

static int result_comp(void *a, void *b)
{
    MSGID_T m1 = ((apr_ldap_result_t *)a)->msgid;
    MSGID_T m2 = ((apr_ldap_result_t *)b)->msgid;
    return (m1 == m2) ? 0 : ((m1 < m2) ? -1 : 1);
}


APU_DECLARE_LDAP(apr_status_t) apr_ldap_initialise(apr_pool_t *pool,
                                                   apr_ldap_t **ldap,
                                                   apu_err_t *result)
{
    memset(result, 0, sizeof(*result));

    *ldap = apr_pcalloc(pool, sizeof(apr_ldap_t));
    if (!*ldap) {
        return APR_ENOMEM;
    }

    (*ldap)->pool = pool;

    apr_skiplist_init(&(*ldap)->results, pool);
    apr_skiplist_set_compare((*ldap)->results, result_comp, result_comp);

    (*ldap)->abandons = apr_array_make(pool, 1, sizeof(MSGID_T));
    (*ldap)->prepares = apr_array_make(pool, 1, sizeof(apr_ldap_prepare_t));

    apr_pool_cleanup_register(pool, (*ldap), apr_ldap_cleanup,
                              apr_pool_cleanup_null);

    return APR_SUCCESS;
}


static apr_status_t option_set_uri(apr_ldap_t *ldap, const char *uri,
                                   apu_err_t *err)
{
    LDAP *ld = NULL;
    int rc = LDAP_SUCCESS;

    if (!ldap || ldap->ld) {
        /* already initialised? say no */
        return APR_EINVAL;
    }

#if APR_HAS_OPENLDAP_LDAPSDK

    rc = ldap_initialize(&ld, uri);

#else

    {
        apr_ldap_url_desc_t *urld;
        apr_status_t status;
        int secure;

        status = apr_ldap_url_parse(ldap->pool, uri, &(urld), &(err));
        if (status != APR_SUCCESS) {
            return status;
        }

        secure = apr_ldap_is_ldaps_url(uri);

#if APR_HAS_MICROSOFT_LDAPSDK
        ld = ldap_sslinit((char *)urld->lud_host, urld->lud_port, secure);
#else
        ld = ldap_init((char *)urld->lud_host, urld->lud_port);
#endif

        ldap->uri = apr_pstrdup(ldap->pool, uri);

    }

#endif

    if (rc != LDAP_SUCCESS) {

        err->rc = rc;
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: Could not initialise";

        return APR_EINVAL;
    }

    else {

        ldap->ld = ld;

    }

    return APR_SUCCESS;
}









/**
 * Handle APR_LDAP_OPT_REBIND_PROC
 *
 * OpenLDAP and Tivoli clients have diverging implementations for rebinding.
 *
 * OpenLDAP calls us back, expecting us to do the bind ourselves. Tivoli
 * calls us back asking for SASL parameters so it can do the bind for us.
 */

#if 0

/* Should we support rebind at all?
 * - Tivoli and OpenLDAP have wildly divergent implementations
 * - Microsoft does not support rebind at all
 * - We want async behaviour, openldap callback forces us to be synchronous
 *
 * Caller must detect referral and chase themselves
 *
 * For now, no.
 */

#if APR_HAS_TIVOLI_LDAPSDK

/* LDAP_rebindproc() Tivoli LDAP style
 *     Rebind callback function. Called when chasing referrals. See API docs.
 * ON ENTRY:
 *     ld       Pointer to an LDAP control structure. (input only)
 *     binddnp  Pointer to an Application DName used for binding (in *or* out)
 *     passwdp  Pointer to the password associated with the DName (in *or* out)
 *     methodp  Pointer to the Auth method (output only)
 *     freeit   Flag to indicate if this is a lookup or a free request (input only)
 */
static int LDAP_rebindproc(LDAP *ld, char **binddnp, char **passwdp, int *methodp, int freeit)
{
    if (!freeit) {
        apr_ldap_rebind_entry_t *my_conn;

        *methodp = LDAP_AUTH_SIMPLE;
        my_conn = apr_ldap_rebind_lookup(ld);

        if ((my_conn) && (my_conn->bindDN != NULL)) {
            *binddnp = strdup(my_conn->bindDN);
            *passwdp = strdup(my_conn->bindPW);
        } else {
            *binddnp = NULL;
            *passwdp = NULL;
        }
    } else {
        if (*binddnp) {
            free(*binddnp);
        }
        if (*passwdp) {
            free(*passwdp);
        }
    }

    return LDAP_SUCCESS;
}

static int option_set_rebind_proc(apr_pool_t *pool, LDAP *ldap, const void *invalue,
                                  apu_err_t *result)
{
    ldap_set_rebind_proc(ld, (LDAPRebindProc)LDAP_rebindproc);
    return APR_SUCCESS;
}

#elif APR_HAS_OPENLDAP_LDAPSDK

/* LDAP_rebindproc() openLDAP V3 style
 * ON ENTRY:
 *     ld       Pointer to an LDAP control structure. (input only)
 *     url      Unused in this routine
 *     request  Unused in this routine
 *     msgid    Unused in this routine
 *     params   Unused in this routine
 *
 *     or
 *
 *     ld       Pointer to an LDAP control structure. (input only)
 *     url      Unused in this routine
 *     request  Unused in this routine
 *     msgid    Unused in this routine
 */
static int LDAP_rebindproc(LDAP *ld, LDAP_CONST char *url, ber_tag_t request,
                           ber_int_t msgid, void *params)
{
    apr_ldap_rebind_entry_t *my_conn;
    const char *bindDN = NULL;
    const char *bindPW = NULL;

    my_conn = apr_ldap_rebind_lookup(ld);

    if ((my_conn) && (my_conn->bindDN != NULL)) {
        bindDN = my_conn->bindDN;
        bindPW = my_conn->bindPW;
    }

    return (ldap_bind_s(ld, bindDN, bindPW, LDAP_AUTH_SIMPLE));
}

static int option_set_rebind_proc(apr_pool_t *pool, LDAP *ldap, const void *invalue,
                                   apu_err_t *result)
{
    ldap_set_rebind_proc(ld, LDAP_rebindproc, NULL);
    return APR_SUCCESS;
}

#else

static int option_set_rebind_proc(apr_pool_t *pool, LDAP *ldap, const void *invalue,
                                   apu_err_t *result)
{  
    return APR_ENOTIPL;
}

#endif

#endif


/**
 * Handle APR_LDAP_OPT_TLS
 *
 * This function sets the type of TLS to be applied to this connection.
 * The options are:
 * APR_LDAP_NONE: no encryption
 * APR_LDAP_SSL: SSL encryption (ldaps://)
 * APR_LDAP_STARTTLS: STARTTLS encryption
 * APR_LDAP_STOPTLS: Stop existing TLS connecttion
 */
static int option_set_tls(LDAP *ldap, const void *invalue,
                          apu_err_t *result)
{
#if APR_HAS_LDAP_SSL /* compiled with ssl support */

    int tls = * (const int *)invalue;

    /* OpenLDAP SDK */
#if APR_HAS_OPENLDAP_LDAPSDK
#ifdef LDAP_OPT_X_TLS
    if (tls == APR_LDAP_SSL) {
        int SSLmode = LDAP_OPT_X_TLS_HARD;
        result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS, &SSLmode);
        if (result->rc != LDAP_SUCCESS) {
            result->reason = "LDAP: ldap_set_option failed. "
                             "Could not set LDAP_OPT_X_TLS to "
                             "LDAP_OPT_X_TLS_HARD";
            result->msg = ldap_err2string(result->rc);
        }   
    }
    else if (tls == APR_LDAP_STARTTLS) {
        result->rc = ldap_start_tls_s(ldap, NULL, NULL);
        if (result->rc != LDAP_SUCCESS) {
            result->reason = "LDAP: ldap_start_tls_s() failed";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (tls == APR_LDAP_STOPTLS) {
        result->reason = "LDAP: STOPTLS is not supported by the "
                         "OpenLDAP SDK";
        result->rc = -1;
    }
#else
    if (tls != APR_LDAP_NONE) {
        result->reason = "LDAP: SSL/TLS not yet supported by APR on this "
                         "version of the OpenLDAP toolkit";
        result->rc = -1;
    }
#endif
#endif

    /* Microsoft SDK */
#if APR_HAS_MICROSOFT_LDAPSDK
    if (tls == APR_LDAP_NONE) {
        result->rc = ldap_set_option(ldap, LDAP_OPT_SSL, LDAP_OPT_OFF);
        if (result->rc != LDAP_SUCCESS) {
            result->reason = "LDAP: an attempt to set LDAP_OPT_SSL off "
                             "failed.";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (tls == APR_LDAP_SSL) {
        result->rc = ldap_set_option(ldap, LDAP_OPT_SSL, LDAP_OPT_ON);
        if (result->rc != LDAP_SUCCESS) {
            result->reason = "LDAP: an attempt to set LDAP_OPT_SSL on "
                             "failed.";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (tls == APR_LDAP_STARTTLS) {
        result->rc = ldap_start_tls_s(ldap, NULL, NULL, NULL, NULL);
        if (result->rc != LDAP_SUCCESS) {
            result->reason = "LDAP: ldap_start_tls_s() failed";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (tls == APR_LDAP_STOPTLS) {
        result->rc = ldap_stop_tls_s(ldap);
        if (result->rc != LDAP_SUCCESS) {
            result->reason = "LDAP: ldap_stop_tls_s() failed";
            result->msg = ldap_err2string(result->rc);
        }
    }
#endif

#if APR_HAS_OTHER_LDAPSDK
    if (tls != APR_LDAP_NONE) {
        result->reason = "LDAP: SSL/TLS is currently not supported by "
                         "APR on this LDAP SDK";
        result->rc = -1;
    }
#endif

#endif /* APR_HAS_LDAP_SSL */

    return result->rc;
}

/**
 * Handle APR_LDAP_OPT_TLS_CACERTFILE
 *
 * This function sets the CA certificate for further SSL/TLS connections.
 *
 * The file provided are in different formats depending on the toolkit used:
 *
 * OpenLDAP: PEM (others supported?)
 * Microsoft: unknown
 */
static int option_set_cert(LDAP *ldap,
                           const void *invalue, apu_err_t *result)
{
#if APR_HAS_LDAP_SSL
#if APR_HAS_LDAPSSL_CLIENT_INIT || APR_HAS_OPENLDAP_LDAPSDK
    apr_array_header_t *certs = (apr_array_header_t *)invalue;
    struct apr_ldap_opt_tls_cert_t *ents = (struct apr_ldap_opt_tls_cert_t *)certs->elts;
    int i = 0;
#endif

    /* OpenLDAP SDK */
#if APR_HAS_OPENLDAP_LDAPSDK
#ifdef LDAP_OPT_X_TLS_CACERTFILE
    /* set one or more certificates */
    for (i = 0; i < certs->nelts; i++) {
        /* OpenLDAP SDK supports BASE64 files. */
        switch (ents[i].type) {
        case APR_LDAP_CA_TYPE_BASE64:
            result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_CACERTFILE,
                                         (void *)ents[i].path);
            result->msg = ldap_err2string(result->rc);
            break;
        case APR_LDAP_CERT_TYPE_BASE64:
            result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_CERTFILE,
                                         (void *)ents[i].path);
            result->msg = ldap_err2string(result->rc);
            break;
        case APR_LDAP_KEY_TYPE_BASE64:
            result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_KEYFILE,
                                         (void *)ents[i].path);
            result->msg = ldap_err2string(result->rc);
            break;
#ifdef LDAP_OPT_X_TLS_CACERTDIR
        case APR_LDAP_CA_TYPE_CACERTDIR_BASE64:
            result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_CACERTDIR,
                                         (void *)ents[i].path);
            result->msg = ldap_err2string(result->rc);
            break;
#endif
        default:
            result->rc = -1;
            result->reason = "LDAP: The OpenLDAP SDK only understands the "
                "PEM (BASE64) file type.";
            break;
        }
        if (result->rc != LDAP_SUCCESS) {
            break;
        }
    }
#else
    result->reason = "LDAP: LDAP_OPT_X_TLS_CACERTFILE not "
                     "defined by this OpenLDAP SDK. Certificate "
                     "authority file not set";
    result->rc = -1;
#endif
#endif

    /* Microsoft SDK */
#if APR_HAS_MICROSOFT_LDAPSDK
    /* Microsoft SDK use the registry certificate store - error out
     * here with a message explaining this. */
    result->reason = "LDAP: CA certificates cannot be set using this method, "
                     "as they are stored in the registry instead.";
    result->rc = -1;
#endif

    /* SDK not recognised */
#if APR_HAS_OTHER_LDAPSDK
    result->reason = "LDAP: LDAP_OPT_X_TLS_CACERTFILE not "
                     "defined by this LDAP SDK. Certificate "
                     "authority file not set";
    result->rc = -1;
#endif

#else  /* not compiled with SSL Support */
    result->reason = "LDAP: Attempt to set certificate(s) failed. "
                     "Not built with SSL support";
    result->rc = -1;
#endif /* APR_HAS_LDAP_SSL */

    return result->rc;
}

/**
 * APR LDAP get option function
 *
 * This function gets option values from a given LDAP session if
 * one was specified.
 *
 * If result_err is NULL, no error detail is returned. If *result_err is
 * NULL, an error detail will be created and returned. If *result_err is
 * not NULL, an error detail will be written to this location.
 */
APU_DECLARE_LDAP(apr_status_t) apr_ldap_option_get(apr_pool_t *pool, apr_ldap_t *ldap,
                                                   int option,
                                                   apr_ldap_opt_t *outvalue,
                                                   apu_err_t *result)
{
    int rc;

    memset(result, 0, sizeof(*result));

    switch (option) {
    case APR_LDAP_OPT_API_INFO: {
#if defined(LDAP_OPT_API_INFO)
        LDAPAPIInfo info = { 0 };

        info.ldapai_info_version = LDAP_API_INFO_VERSION;

        rc = ldap_get_option(NULL, LDAP_OPT_API_INFO, &info);

        outvalue->info.api_version = info.ldapai_api_version;
        outvalue->info.protocol_version = info.ldapai_protocol_version;
        outvalue->info.extensions = (const char **)info.ldapai_extensions;
        outvalue->info.vendor_name = info.ldapai_vendor_name;
        outvalue->info.vendor_version = info.ldapai_vendor_version;

        break;
#else
        result->reason = "LDAP: API info not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif
    }
    case APR_LDAP_OPT_API_FEATURE_INFO: {
#if defined(LDAP_OPT_API_FEATURE_INFO)
        LDAPAPIFeatureInfo ldfi = { 0 };

        ldfi.ldapaif_info_version = LDAP_FEATURE_INFO_VERSION;
        ldfi.ldapaif_name = (char *)outvalue->ldfi.name;

        rc = ldap_get_option(NULL, LDAP_OPT_API_FEATURE_INFO, &ldfi);

        outvalue->ldfi.version = ldfi.ldapaif_version;

        break;

#else
        result->reason = "LDAP: API feature info not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif
    }
    case APR_LDAP_OPT_PROTOCOL_VERSION: {

        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_PROTOCOL_VERSION, &outvalue->pv);

        break;
    }
    case APR_LDAP_OPT_HANDLE: {

        outvalue->handle = ldap ? ldap->ld : NULL;

        return APR_SUCCESS;
    }
    case APR_LDAP_OPT_DESC: {
#if defined(LDAP_OPT_DESC)

        apr_status_t status = APR_SUCCESS;

        if (!ldap->socket) {
            apr_os_sock_t sock;

            rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_DESC, &sock);

            if (rc == LDAP_SUCCESS) {
                status = apr_os_sock_put(&ldap->socket, &sock, ldap->pool);
            }
            else {
                status = apr_ldap_status(rc);
            }
        }
        outvalue->socket = ldap->socket;

        return status;
#else
        result->reason = "LDAP: LDAP_OPT_DESC not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif
    }
    case APR_LDAP_OPT_URI: {
#if APR_HAS_OPENLDAP_LDAPSDK
        rc = ldap_get_option(ldap ? ldap->ld : NULL, option, &outvalue->opt);
        break;
#else
        outvalue->uri = ldap->uri;
        return APR_SUCCESS;
#endif
    }
    case APR_LDAP_OPT_DEBUG_LEVEL: {

#if defined(LDAP_OPT_DEBUG_LEVEL)
        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_DEBUG_LEVEL, &outvalue->debug);

        break;
#else
        result->reason = "LDAP: Debug level not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif
    }
    case APR_LDAP_OPT_DEREF: {

        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_DEREF, &outvalue->deref);

        break;
    }
    case APR_LDAP_OPT_REFERRALS: {
        int refs;

        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_REFERRALS, &refs);

        if (rc == LDAP_SUCCESS) {
            outvalue->refs = refs ? APR_LDAP_OPT_ON : APR_LDAP_OPT_OFF;
        }

        break;
    }
    case APR_LDAP_OPT_REFHOPLIMIT: {
#if defined(LDAP_OPT_REFERRAL_HOP_LIMIT)
        /* Microsoft SDK defines LDAP_OPT_REFERRAL_HOP_LIMIT
         */
        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_REFERRAL_HOP_LIMIT, &outvalue->refhoplimit);
#elif defined(LDAP_OPT_REFHOPLIMIT)
        /* Setting this option is supported on TIVOLI_SDK.
         */
        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_REFHOPLIMIT, &outvalue->refhoplimit);
#else
        result->reason = "LDAP: Referral hop limit not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif

        break;
    }
    case APR_LDAP_OPT_RESULT_CODE: {

#ifdef LDAP_OPT_RESULT_CODE
        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_RESULT_CODE, &outvalue->result);
#else
#ifdef LDAP_OPT_ERROR_NUMBER
        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_ERROR_NUMBER, &outvalue->result);
#endif
#endif
        break;
    }
    case APR_LDAP_OPT_TLS_CERT: {

        result->reason = "LDAP: Could not get an option APR_LDAP_OPT_TLS_CERT: not implemented";

        return APR_ENOTIMPL;
    }
    case APR_LDAP_OPT_TLS: {

        result->reason = "LDAP: Could not get an option APR_LDAP_OPT_TLS: not implemented";

        return APR_ENOTIMPL;
    }
    case APR_LDAP_OPT_VERIFY_CERT: {

        result->reason = "LDAP: Could not get an option APR_LDAP_OPT_VERIFY_CERT: not implemented";

        return APR_ENOTIMPL;
    }
    case APR_LDAP_OPT_NETWORK_TIMEOUT: {
#if !defined(LDAP_OPT_NETWORK_TIMEOUT) && defined(LDAP_OPT_CONNECT_TIMEOUT)
#define LDAP_OPT_NETWORK_TIMEOUT LDAP_OPT_CONNECT_TIMEOUT
#endif   
#ifdef LDAP_OPT_NETWORK_TIMEOUT
        struct timeval networkTimeout = {0};

        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_NETWORK_TIMEOUT, &networkTimeout);

        outvalue->timeout = apr_time_make(networkTimeout.tv_sec, networkTimeout.tv_usec);

        break;
#else
        result->reason = "LDAP: Could not get an option APR_LDAP_OPT_NETWORK_TIMEOUT: not implemented";

        return APR_ENOTIMPL;
#endif
    }
    case APR_LDAP_OPT_TIMEOUT: {
#ifdef LDAP_OPT_TIMEOUT
        /*
         * LDAP_OPT_TIMEOUT is not portable, but it influences all synchronous ldap    
         * function calls and not just ldap_search_ext_s(), which accepts a timeout
         * parameter.
         * XXX: It would be possible to simulate LDAP_OPT_TIMEOUT by replacing all    
         * XXX: synchronous ldap function calls with asynchronous calls and using
         * XXX: ldap_result() with a timeout.
         */
        struct timeval timeout = {0};

        rc = ldap_get_option(ldap ? ldap->ld : NULL, LDAP_OPT_TIMEOUT, &timeout);

        outvalue->timeout = apr_time_make(timeout.tv_sec, timeout.tv_usec);

        break;
#else
        result->reason = "LDAP: Could not get an option APR_LDAP_OPT_TIMEOUT: not implemented";

        return APR_ENOTIMPL;
#endif
    }
    default:
        rc = ldap_get_option(ldap ? ldap->ld : NULL, option, &outvalue->opt);
    }

    if (rc != LDAP_SUCCESS) {

        result->rc = rc;
        result->msg = ldap_err2string(result->rc);
        result->reason = "LDAP: Could not get an option";

        return APR_EINVAL;
    }

    return APR_SUCCESS;
}

/**
 * APR LDAP set option function
 *
 * This function sets option values to a given LDAP session if
 * one was specified.
 *
 * Where an option is not supported by an LDAP toolkit, this function
 * will try and apply legacy functions to achieve the same effect,
 * depending on the platform.
 */
APU_DECLARE_LDAP(apr_status_t) apr_ldap_option_set(apr_pool_t *pool, apr_ldap_t *ldap,
                                                   int option,
                                                   const apr_ldap_opt_t *invalue,
                                                   apu_err_t *result)
{
    int rc;

    memset(result, 0, sizeof(*result));

    switch (option) {
    case APR_LDAP_OPT_DESC:

        /*
         * TODO:  we want the option to use our own socket. This option is normally
         * read only, however we could allow this to somehow call ldap_init_fd()
         * for us.
         *
         * This means we can asynchronously perform DNS lookups and SSL handshakes
         * without expecting the LDAP library to cooperate with that.
         */

        /* windows allows the socket to be set here */

        rc = LDAP_UNWILLING_TO_PERFORM;
        goto end;

    case APR_LDAP_OPT_URI:
        rc = option_set_uri(ldap, invalue->uri, result);
        goto end;

    default:
        break;
    }


    if (ldap && !ldap->ld) {
        result->reason = "LDAP: URI or descriptor needs to be set first";

        return APR_EINVAL;
    }


    switch (option) {
    case APR_LDAP_OPT_API_INFO:
        rc = LDAP_UNWILLING_TO_PERFORM;
        break;

    case APR_LDAP_OPT_API_FEATURE_INFO:
        rc = LDAP_UNWILLING_TO_PERFORM;
        break;

    case APR_LDAP_OPT_PROTOCOL_VERSION:
        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_PROTOCOL_VERSION, (void*)&invalue->pv);
        break;

    case APR_LDAP_OPT_HANDLE:
        rc = LDAP_UNWILLING_TO_PERFORM;
        break;

    case APR_LDAP_OPT_DEBUG_LEVEL:
#if defined(LDAP_OPT_DEBUG_LEVEL)
        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_DEBUG_LEVEL, &invalue->debug);
        break;
#else
        result->reason = "LDAP: Debug level not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif

    case APR_LDAP_OPT_DEREF:
        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_DEREF, (void*)&invalue->deref);
        break;

    case APR_LDAP_OPT_REFERRALS: {
        void *refs = invalue->refs ? (void *)LDAP_OPT_ON : (void *)LDAP_OPT_OFF;

        /* Setting this option is supported on at least TIVOLI_SDK and OpenLDAP.
         */
        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_REFERRALS, refs);
        break;

    }
    case APR_LDAP_OPT_REFHOPLIMIT:
#if defined(LDAP_OPT_REFERRAL_HOP_LIMIT)
        /* Microsoft SDK defines LDAP_OPT_REFERRAL_HOP_LIMIT
         */
        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_REFERRAL_HOP_LIMIT, &invalue->refhoplimit);
#elif defined(LDAP_OPT_REFHOPLIMIT)
        /* Setting this option is supported on TIVOLI_SDK.
         */
        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_REFHOPLIMIT, (void*)&invalue->refhoplimit);
#else
        /* If the LDAP_OPT_REFHOPLIMIT symbol is missing, assume that the
         * particular LDAP library has a reasonable default. So far certain
         * versions of the OpenLDAP SDK miss this symbol (but default to 5),
         * and the Microsoft SDK misses the symbol (the default is not known).
         */
        result->reason = "LDAP: Referral hop limit not yet supported by APR on this "
                         "LDAP SDK";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif
        break;

    case APR_LDAP_OPT_RESULT_CODE:
        rc = LDAP_UNWILLING_TO_PERFORM;
        break;

#if 0
    case APR_LDAP_OPT_REBIND_PROC:
        rc = option_set_rebind_proc(ldap->pool, ldap ? ldap->ld : NULL, invalue->rebind, result);
        break;
#endif

    case APR_LDAP_OPT_TLS_CERT:
        rc = option_set_cert(ldap ? ldap->ld : NULL, invalue->certs, result);
        break;

    case APR_LDAP_OPT_TLS:
        rc = option_set_tls(ldap ? ldap->ld : NULL, &invalue->tls, result);
        break;

    case APR_LDAP_OPT_VERIFY_CERT:
#ifdef LDAP_OPT_X_TLS
        /* This is not a per-connection setting so just pass NULL for the
           Ldap connection handle */
        if (invalue->verify) {
            int i = LDAP_OPT_X_TLS_DEMAND;
            result->rc = ldap_set_option(NULL, LDAP_OPT_X_TLS_REQUIRE_CERT, &i);
        }
        else {
            int i = LDAP_OPT_X_TLS_NEVER;
            result->rc = ldap_set_option(NULL, LDAP_OPT_X_TLS_REQUIRE_CERT, &i);
        }
#else
        result->reason = "LDAP: SSL/TLS not yet supported by APR on this "
                         "version of the OpenLDAP toolkit";
        result->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif

        /* handle the error case */
        if (result->rc != LDAP_SUCCESS) {
            result->msg = ldap_err2string(result->rc);
            result->reason = "LDAP: Could not set verify mode";
            return APR_EINVAL;
        }
        return APR_SUCCESS;

    case APR_LDAP_OPT_NETWORK_TIMEOUT: {
#if !defined(LDAP_OPT_NETWORK_TIMEOUT) && defined(LDAP_OPT_CONNECT_TIMEOUT)
#define LDAP_OPT_NETWORK_TIMEOUT LDAP_OPT_CONNECT_TIMEOUT
#endif
#ifdef LDAP_OPT_NETWORK_TIMEOUT
        struct timeval networkTimeout = {0};

        networkTimeout.tv_sec = apr_time_sec(invalue->timeout);
        networkTimeout.tv_usec = apr_time_usec(invalue->timeout);

        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_NETWORK_TIMEOUT, &networkTimeout);

        break;
#else
        result->reason = "LDAP: Could not set an option APR_LDAP_OPT_NETWORK_TIMEOUT: not implemented";

        return APR_ENOTIMPL;
#endif
    }
    case APR_LDAP_OPT_TIMEOUT: {
#ifdef LDAP_OPT_TIMEOUT
        /*
         * LDAP_OPT_TIMEOUT is not portable, but it influences all synchronous ldap
         * function calls and not just ldap_search_ext_s(), which accepts a timeout
         * parameter.
         * XXX: It would be possible to simulate LDAP_OPT_TIMEOUT by replacing all
         * XXX: synchronous ldap function calls with asynchronous calls and using
         * XXX: ldap_result() with a timeout.
         */
        struct timeval timeout = {0};

        timeout.tv_sec = apr_time_sec(invalue->timeout);
        timeout.tv_usec = apr_time_usec(invalue->timeout);

        rc = ldap_set_option(ldap ? ldap->ld : NULL, LDAP_OPT_TIMEOUT, &timeout);

        break;
#else
        result->reason = "LDAP: Could not set an option APR_LDAP_OPT_TIMEOUT: not implemented";

        return APR_ENOTIMPL;
#endif
    }
    default:
        rc = ldap_set_option(ldap ? ldap->ld : NULL, option, invalue->opt);
    }

end:

#if defined (LDAP_OPT_SUCCESS)
    if (rc != LDAP_OPT_SUCCESS) {
#else
    if (rc != LDAP_SUCCESS) {
#endif

        result->rc = rc;
        if (!result->msg) {
            result->msg = ldap_err2string(result->rc);
        }
        if (result->reason) {
            result->reason = "LDAP: Could not set an option";
        }

        return APR_EINVAL;
    }

    return APR_SUCCESS;
}





APU_DECLARE_LDAP(apr_status_t) apr_ldap_connect(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                apr_interval_time_t timeout,
                                                apu_err_t *err)
{
#if APR_HAVE_LDAP_CONNECT
    LDAP *ld = ldap->ld;

#if APR_HAS_MICROSOFT_LDAPSDK
    LDAP_TIMEVAL tv, *tvptr;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long) apr_time_sec(timeout);
        tv.tv_usec = (long) apr_time_usec(timeout);
        tvptr = &tv;
    }

    err->rc = ldap_connect(ld, tvptr);

#else

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

    err->rc = ldap_connect(ld);

#endif

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_connect() failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    return APR_SUCCESS;
#else
    return APR_ENOTIMPL;
#endif
}





APU_DECLARE_LDAP(apr_status_t) apr_ldap_prepare(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                apr_ldap_prepare_cb prepare_cb,
                                                void *prepare_ctx)
{

    apr_ldap_prepare_t *prepare = apr_array_push(ldap->prepares);

    if (!prepare) {
        return APR_ENOMEM;
    }

    prepare->pool = pool;
    prepare->cb = prepare_cb;
    prepare->ctx = prepare_ctx;

    apr_pool_cleanup_register(pool, prepare, prepare_cleanup,
                                      apr_pool_cleanup_null);

    return APR_SUCCESS;
}



static apr_status_t ldap_control_cleanup(void *dptr)
{
    if (dptr) {

        LDAPControl *ctl = dptr;

        ldap_control_free(ctl);
    }

    return APR_SUCCESS;
}

#if APR_HAS_OPENLDAP_LDAPSDK
static apr_status_t ldap_berval_cleanup(void *dptr)
{
    if (dptr) {

        struct berval *val = dptr;

        ber_bvfree(val);
    }

    return APR_SUCCESS;
}

static apr_status_t ldap_memfree_cleanup(void *dptr)
{
    if (dptr) {
        ldap_memfree(dptr);
    }

    return APR_SUCCESS;
}
#endif

static apr_status_t apr_ldap_control_parse(apr_pool_t *pool,
                                           apr_ldap_t *ldap,
                                           LDAPControl **ctls,
                                           apr_hash_t **controls,
                                           apu_err_t *err)
{
    apr_hash_t *cs;

    int i = 0;

    if (!ctls || !ctls[0]) {
        *controls = NULL;
        return APR_SUCCESS;
    }

    cs = apr_hash_make(pool);

    if (!cs) {
        return APR_ENOMEM;
    }

    for (i = 0; ctls[i]; i++) {

        LDAPControl *ctl = (LDAPControl *)ctls[i];

        apr_ldap_control_t *c = apr_pcalloc(pool, sizeof(apr_ldap_control_t));

        c->critical = ctl->ldctl_iscritical ? 1 : 0;

        /* what controls do we recognise? */

#if APR_HAS_OPENLDAP_LDAPSDK

        if (!strcmp(ctl->ldctl_oid, LDAP_CONTROL_SORTRESPONSE)) {

            ber_int_t result;
            char *attr;

            err->rc = ldap_parse_sortresponse_control(ldap->ld, ctl, &result, &attr);

            if (err->rc != LDAP_SUCCESS) {
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP: ldap_parse_sortresponse_control failed";
                return apr_ldap_status(err->rc);
            }

            c->type = APR_LDAP_CONTROL_SORT_RESPONSE;

            if (attr) {

                apr_pool_cleanup_register(pool, attr, ldap_memfree_cleanup,
                                          apr_pool_cleanup_null);

            }

            c->c.sortrs.attribute = (const char *)attr;
            c->c.sortrs.result = apr_ldap_status(result);

            apr_hash_set(cs, ctl->ldctl_oid, APR_HASH_KEY_STRING, c);

            continue;
        }

        if (!strcmp(ctl->ldctl_oid, LDAP_CONTROL_PAGEDRESULTS)) {

            ber_int_t count;
            struct berval cookie;

            err->rc = ldap_parse_pageresponse_control(ldap->ld, ctl, &count, &cookie);

            if (err->rc != LDAP_SUCCESS) {
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP: ldap_parse_pageresponse_control failed";
                return apr_ldap_status(err->rc);
            }

            c->type = APR_LDAP_CONTROL_PAGE_RESPONSE;

            if (cookie.bv_val) {
                apr_buffer_mem_set(&c->c.pagers.cookie, cookie.bv_val, cookie.bv_len);

                apr_pool_cleanup_register(pool, cookie.bv_val, ldap_memfree_cleanup,
                                          apr_pool_cleanup_null);

            }
            else {
                apr_buffer_mem_set(&c->c.pagers.cookie, NULL, 0);
            }

            c->c.pagers.count = (apr_size_t)count;

            apr_hash_set(cs, ctl->ldctl_oid, APR_HASH_KEY_STRING, c);

            continue;
        }

        if (!strcmp(ctl->ldctl_oid, LDAP_CONTROL_VLVRESPONSE)) {

            ber_int_t target_posp;
            ber_int_t list_countp;
            int result;

            struct berval *context = NULL;

            err->rc = ldap_parse_vlvresponse_control(ldap->ld, ctl, &target_posp, &list_countp, &context, &result);

            if (err->rc != LDAP_SUCCESS) {
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP: ldap_parse_vlvresponse_control failed";
                return apr_ldap_status(err->rc);
            }

            c->type = APR_LDAP_CONTROL_VLV_RESPONSE;

            c->c.vlvrs.offset = (apr_size_t)target_posp;
            c->c.vlvrs.count = (apr_size_t)list_countp;

            if (context) {
                apr_buffer_mem_set(&c->c.vlvrs.context, context->bv_val, context->bv_len);

                apr_pool_cleanup_register(pool, context, ldap_berval_cleanup,
                                          apr_pool_cleanup_null);

            }
            else {
                apr_buffer_mem_set(&c->c.vlvrs.context, NULL, 0);
            }

            c->c.vlvrs.result = apr_ldap_status(result);

            apr_hash_set(cs, ctl->ldctl_oid, APR_HASH_KEY_STRING, c);

            continue;
        }

#endif

        /* not recognised, return raw value */
        c->type = APR_LDAP_CONTROL_OID;

        c->oid.oid = (const char *)ctl->ldctl_oid;

        apr_buffer_mem_set(&c->oid.val, ctl->ldctl_value.bv_val,
                           ctl->ldctl_value.bv_len);

        apr_hash_set(cs, ctl->ldctl_oid, APR_HASH_KEY_STRING, c);

    }

    *controls = cs;

    return APR_SUCCESS;
}

static apr_status_t apr_ldap_control_create(apr_pool_t *pool,
                                            apr_ldap_t *ldap,
                                            LDAPControl ***ctrls,
                                            apr_array_header_t *controls,
                                            apu_err_t *err)
{
    LDAPControl **cs;

    int i, count;

    if (!controls || !(count = controls->nelts)) {
        *ctrls = NULL;
        return APR_SUCCESS;
    }

    cs = apr_pcalloc(pool, (count + 1) * sizeof(LDAPControl *));

    for (i = 0; i < count; ++i) {

        apr_ldap_control_t *control = &APR_ARRAY_IDX(controls, i, apr_ldap_control_t);

        /* what controls do we recognise? */
        switch (control->type) {

        /* page control */
        case APR_LDAP_CONTROL_PAGE_REQUEST: {
#if APR_HAS_OPENLDAP_LDAPSDK

            LDAPControl *c;

            ber_int_t pagesize = control->c.pagerq.size;
            struct berval cookie;

            cookie.bv_val = apr_buffer_str(&control->c.pagerq.cookie);
            cookie.bv_len = TO_BV_LEN(apr_buffer_len(&control->c.pagerq.cookie));

            err->rc = ldap_create_page_control(ldap->ld, pagesize, &cookie, control->critical ? 1 : 0, &c);

            if (err->rc != LDAP_SUCCESS) {
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP: ldap_create_page_control failed";
                return apr_ldap_status(err->rc);
            }

            apr_pool_cleanup_register(pool, c, ldap_control_cleanup,
                                      apr_pool_cleanup_null);

            cs[i] = c;

            break;
#else
            err->reason = "LDAP: page control not supported";
            return APR_ENOTIMPL;
#endif
        }

        /* sort control */
        case APR_LDAP_CONTROL_SORT_REQUEST: {
#if APR_HAS_OPENLDAP_LDAPSDK

            LDAPControl *c;
            LDAPSortKey **sks;

            apr_array_header_t *keys = control->c.sortrq.keys;

            int j;

            if (!keys || !keys->nelts) {
                err->reason = "LDAP: no sort control keys specified";
                return APR_EINVAL;
            }

            sks = apr_pcalloc(pool, (keys->nelts + 1) * sizeof(LDAPSortKey *));

            for (j = 0; j < keys->nelts; ++j) {

                apr_ldap_control_sortkey_t *sortkey = &APR_ARRAY_IDX(keys, j, apr_ldap_control_sortkey_t);

                LDAPSortKey *sk = apr_pcalloc(pool, sizeof(LDAPSortKey));

                sk->attributeType = (char *)sortkey->attribute;
                sk->orderingRule = (char *)sortkey->order;
                sk->reverseOrder = sortkey->direction == APR_LDAP_CONTROL_SORT_REVERSE ? 1 : 0;

                sks[j] = sk;
            }

            err->rc = ldap_create_sort_control(ldap->ld, sks, control->critical ? 1 : 0, &c);

            if (err->rc != LDAP_SUCCESS) {
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP: ldap_create_sort_control failed";
                return apr_ldap_status(err->rc);
            }

            apr_pool_cleanup_register(pool, c, ldap_control_cleanup,
                                      apr_pool_cleanup_null);

            cs[i] = c;

            break;
#else
            err->reason = "LDAP: sort control not supported";
            return APR_ENOTIMPL;
#endif
        }

        /* vlv control */
        case APR_LDAP_CONTROL_VLV_REQUEST: {
#if APR_HAS_OPENLDAP_LDAPSDK

            LDAPControl *c;
            LDAPVLVInfo vlvInfo;
            struct berval attrvalue;
            struct berval context;

            vlvInfo.ldvlv_before_count = control->c.vlvrq.before;
            vlvInfo.ldvlv_after_count = control->c.vlvrq.after;
            vlvInfo.ldvlv_offset = control->c.vlvrq.offset;
            vlvInfo.ldvlv_count = control->c.vlvrq.count;

            if (apr_buffer_is_null(&control->c.vlvrq.attrvalue)) {
                vlvInfo.ldvlv_attrvalue = NULL;
            }
            else {
                attrvalue.bv_val = (char *)apr_buffer_mem(&control->c.vlvrq.attrvalue, NULL);
                attrvalue.bv_len = TO_BV_LEN(apr_buffer_len(&control->c.vlvrq.attrvalue));
                vlvInfo.ldvlv_attrvalue = &attrvalue;
            }

            if (apr_buffer_is_null(&control->c.vlvrq.context)) {
                vlvInfo.ldvlv_context = NULL;
            }
            else {
                context.bv_val = (char *)apr_buffer_mem(&control->c.vlvrq.context, NULL);
                context.bv_len = TO_BV_LEN(apr_buffer_len(&control->c.vlvrq.context));
                vlvInfo.ldvlv_context = &context;
            }

            vlvInfo.ldvlv_extradata = NULL;
            vlvInfo.ldvlv_version = 1;

            err->rc = ldap_create_vlv_control(ldap->ld, &vlvInfo, &c);

            if (err->rc != LDAP_SUCCESS) {
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP: ldap_create_vlv_control failed";
                return apr_ldap_status(err->rc);
            }

            apr_pool_cleanup_register(pool, c, ldap_control_cleanup,
                                      apr_pool_cleanup_null);

            cs[i] = c;

            break;
#else
            err->reason = "LDAP: vlv control not supported";
            return APR_ENOTIMPL;
#endif
        }

        /* not recognised, return raw value */
        case APR_LDAP_CONTROL_OID: {

            apr_size_t size;

            LDAPControl *c = apr_pcalloc(pool, count * sizeof(LDAPControl));

            if (!control->oid.oid) {
                err->msg = NULL;
                err->reason = "LDAP: control oid missing";
                return APR_EINVAL;
            }

            c->ldctl_oid = (char *)control->oid.oid;
            c->ldctl_value.bv_val = apr_buffer_mem(&control->oid.val, &size);
            c->ldctl_value.bv_len = TO_BV_LEN(size);
            c->ldctl_iscritical = control->critical ? 1 : 0;

            cs[i] = c;

            break;
        }
        default:
            err->reason = "LDAP: control not recognised";
            return APR_EINVAL;
        }

    }

    *ctrls = (LDAPControl **)cs;

    return APR_SUCCESS;
}




/*
 * The results cleanup dance.
 *
 * We need to clean up each result when the pool belong to the request is
 * cleared. This involves removing the result from the skiplist, and freeing
 * linked data like LDAP messages.
 *
 * We need to clean up all the results when the pool belonging to the ldap
 * connection is cleared. This involves walking the results list, removing
 * the cleanups from the request pools, then freeing linked data like the
 * LDAP messages.
 */
static apr_status_t apr_ldap_result_clear(apr_ldap_result_t *res)
{
    res->rmech = NULL;
    if (res->message) {
        ldap_msgfree(res->message);
        res->message = NULL;
    }

    return APR_SUCCESS;
}

static void result_result_cleanup(void *dptr)
{   
    apr_ldap_result_clear(dptr);
}   

static apr_status_t result_cleanup(void *dptr)
{
    apr_ldap_result_t *res = dptr;

    apr_skiplist_remove(res->ld->results, dptr, result_result_cleanup);

    return APR_SUCCESS;
}

static void results_result_cleanup(void *dptr)
{
    apr_ldap_result_t *res = dptr;
    
    apr_pool_cleanup_kill(res->pool, res, result_cleanup);

    apr_ldap_result_clear(res);
}

static apr_status_t results_cleanup(void *dptr)
{   
    apr_skiplist *results = dptr;

    apr_skiplist_remove_all(results, results_result_cleanup);

    return APR_SUCCESS;
}


static void apr_ldap_result_add(apr_pool_t *pool,
                                apr_ldap_t *ldap,
                                apr_ldap_result_t *res,
                                MSGID_T msgid)
{
    res->pool = pool;
    res->ld = ldap;
    res->msgid = msgid;

    apr_pool_cleanup_register(pool, res, result_cleanup,
                              apr_pool_cleanup_null);

    apr_skiplist_add(ldap->results, res);
}

static void apr_ldap_result_remove(apr_ldap_t *ldap,
                                   apr_ldap_result_t *res)
{
    apr_pool_cleanup_run(res->pool, res, result_cleanup);
}


APU_DECLARE_LDAP(apr_status_t) apr_ldap_process(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                apr_interval_time_t timeout,
                                                apu_err_t *err)
{
    apr_skiplistnode *iter = NULL;

    apr_ldap_result_t *res;

    apr_status_t status = APR_SUCCESS;

    MSGID_T msgid = 0;

    /* do we have a prepare callback outstanding? */

    while (ldap->prepares->nelts) {

        apr_ldap_prepare_t *prepare = apr_array_pop(ldap->prepares);

        if (prepare->pool) {
            status = prepare->cb(ldap, status, prepare->ctx, err);

            apr_pool_cleanup_run(prepare->pool, prepare, prepare_cleanup);

            return status;
        }

    }

    /* any abandoned requests? handle them first */

    if (ldap->abandons->nelts) {

        MSGID_T *msgid = apr_array_pop(ldap->abandons);

#if APR_HAS_OPENLDAP_LDAPSDK
        err->rc = ldap_abandon_ext(ldap->ld, *msgid, NULL, NULL);
#else
        err->rc = ldap_abandon(ldap->ld, *msgid);
#endif  

        if (err->rc == LDAP_SUCCESS) {

            /* anyone needs a read? */
            if (apr_skiplist_size(ldap->results)) {
                return APR_WANT_READ;
            }

            /* otherwise we're done */
            return APR_SUCCESS;

        }
        else {
            err->reason = "LDAP: ldap_abandon_ext() failed";
            err->msg = ldap_err2string(err->rc);
            return apr_ldap_status(err->rc);
        }

    }

    /* iterate through skiplist, see if any response has outstanding work */

    for (iter = apr_skiplist_getlist(ldap->results);
         iter;
         apr_skiplist_next(ldap->results, &iter)) {

        res = apr_skiplist_element(iter);

        if (!res->message) {
            continue;
        }

        switch(res->msgtype) {
        case LDAP_RES_BIND: {

#if APR_HAS_OPENLDAP_LDAPSDK && APR_HAVE_LDAP_SASL_INTERACTIVE_BIND

            /* handle binding */

            LDAPControl *sctrls[] = { 0 };
            LDAPControl *cctrls[] = { 0 };

            unsigned int flags = LDAP_SASL_QUIET;

            err->rc = ldap_sasl_interactive_bind(ldap->ld, NULL, res->mech,
                                                 sctrls, cctrls, flags, NULL, NULL,
                                                 res->message, &res->rmech, &msgid);

            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP bind: ldap_sasl_interactive_bind()";

            if (err->rc == LDAP_SASL_BIND_IN_PROGRESS) {

                apr_skiplist_remove(ldap->results, res, NULL);

                res->msgid = msgid;
                ldap_msgfree(res->message);
                res->message = NULL;

                apr_skiplist_add(ldap->results, res);

                return APR_WANT_READ;
            }
            else {

                /* we got a response, send the news, good or bad */
                if (res->cb.bind) {
                    status = res->cb.bind(ldap, apr_ldap_status(err->rc),
                                 NULL, NULL, res->ctx, err);
                }
                else {
                    status = apr_ldap_status(err->rc);
                }

                apr_ldap_result_remove(ldap, res);
            }

            break;
#else

            /*
             * for platforms that do not support ldap_sasl_interactive_bind(), alternative
             * implementations using ldap_sasl_bind() go here.
             */

            err->reason = "LDAP: SASL bind not yet supported by APR on this "
                          "LDAP SDK";
            err->rc = LDAP_UNWILLING_TO_PERFORM;
            return APR_ENOTIMPL;
#endif
        }
        case LDAP_RES_COMPARE: {

            /* handle comparing */

            char *matcheddn = NULL;
            char *errmsg = NULL;
            LDAPControl **serverctrls = NULL;
            int rc;

            err->rc = ldap_parse_result(ldap->ld, res->message, &rc, &matcheddn, &errmsg,
                                        NULL, &serverctrls, 0);

            err->rc = rc != LDAP_SUCCESS ? rc : err->rc;
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP compare: ldap_parse_result()";

            if (res->cb.compare) {
                status = res->cb.compare(ldap, apr_ldap_status(err->rc),
                                         matcheddn, (apr_ldap_control_t **)serverctrls,
                                         res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }


            apr_ldap_result_remove(ldap, res);

            if (matcheddn) {
                ldap_memfree(matcheddn);
            }

            if (errmsg) {
                ldap_memfree(errmsg);
            }

            if (serverctrls) {
                ldap_controls_free(serverctrls);
            }

            break;
        }
        case LDAP_RES_SEARCH_RESULT: {

            /* handle search result */

            char *matcheddn = NULL;
            char *errmsg = NULL;
            LDAPControl **serverctrls = NULL;
            apr_hash_t *controls = NULL;
            int rc;

            err->rc = ldap_parse_result(ldap->ld, res->message, &rc, &matcheddn,
                                        &errmsg, NULL, &serverctrls, 0);

            status = apr_ldap_control_parse(res->pool, ldap, serverctrls, &controls, err);

            if (APR_SUCCESS == status) {
                err->rc = rc != LDAP_SUCCESS ? rc : err->rc;
                err->msg = ldap_err2string(err->rc);
                err->reason = "LDAP search: ldap_parse_result()";
                status = apr_ldap_status(err->rc);
            }

            if (res->cb.search) {
                status = res->cb.search(ldap, status, 0,
                                        matcheddn, controls,
                                        res->ctx, err);
            }

            apr_ldap_result_remove(ldap, res);

            if (matcheddn) {
                ldap_memfree(matcheddn);
            }

            if (errmsg) {
                ldap_memfree(errmsg);
            }

            if (serverctrls) {
                ldap_controls_free(serverctrls);
            }

            break;
        }
        case LDAP_RES_ADD: {

            /* handle adding */

            char *matcheddn = NULL;
            char *errmsg = NULL;
            LDAPControl **serverctrls = NULL;
            int rc;

            err->rc = ldap_parse_result(ldap->ld, res->message, &rc, &matcheddn, &errmsg,
                                        NULL, &serverctrls, 0);

            err->rc = rc != LDAP_SUCCESS ? rc : err->rc;
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP add: ldap_parse_result()";

            if (res->cb.add) {
                status = res->cb.add(ldap, apr_ldap_status(err->rc),
                                         matcheddn, (apr_ldap_control_t **)serverctrls,
                                         res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }


            apr_ldap_result_remove(ldap, res);

            if (matcheddn) {
                ldap_memfree(matcheddn);
            }

            if (errmsg) {
                ldap_memfree(errmsg);
            }

            if (serverctrls) {
                ldap_controls_free(serverctrls);
            }

            break;
        }
        case LDAP_RES_MODIFY: {

            /* handle modification */

            char *matcheddn = NULL;
            char *errmsg = NULL;
            LDAPControl **serverctrls = NULL;
            int rc;

            err->rc = ldap_parse_result(ldap->ld, res->message, &rc, &matcheddn, &errmsg,
                                        NULL, &serverctrls, 0);

            err->rc = rc != LDAP_SUCCESS ? rc : err->rc;
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP modify: ldap_parse_result()";

            if (res->cb.modify) {
                status = res->cb.modify(ldap, apr_ldap_status(err->rc),
                                        matcheddn, (apr_ldap_control_t **)serverctrls,
                                        res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }


            apr_ldap_result_remove(ldap, res);

            if (matcheddn) {
                ldap_memfree(matcheddn);
            }

            if (errmsg) {
                ldap_memfree(errmsg);
            }

            if (serverctrls) {
                ldap_controls_free(serverctrls);
            }

            break;
        }
        case LDAP_RES_RENAME: {

            /* handle rename */

            char *matcheddn = NULL;
            char *errmsg = NULL;
            LDAPControl **serverctrls = NULL;
            int rc;

            err->rc = ldap_parse_result(ldap->ld, res->message, &rc, &matcheddn, &errmsg,
                                        NULL, &serverctrls, 0);

            err->rc = rc != LDAP_SUCCESS ? rc : err->rc;
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP rename: ldap_parse_result()";

            if (res->cb.rename) {
                status = res->cb.rename(ldap, apr_ldap_status(err->rc),
                                        matcheddn, (apr_ldap_control_t **)serverctrls,
                                        res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }


            apr_ldap_result_remove(ldap, res);

            if (matcheddn) {
                ldap_memfree(matcheddn);
            }

            if (errmsg) {
                ldap_memfree(errmsg);
            }

            if (serverctrls) {
                ldap_controls_free(serverctrls);
            }

            break;
        }
        case LDAP_RES_DELETE: {

            /* handle delete */

            char *matcheddn = NULL;
            char *errmsg = NULL;
            LDAPControl **serverctrls = NULL;
            int rc;

            err->rc = ldap_parse_result(ldap->ld, res->message, &rc, &matcheddn, &errmsg,
                                        NULL, &serverctrls, 0);

            err->rc = rc != LDAP_SUCCESS ? rc : err->rc;
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP delete: ldap_parse_result()";

            if (res->cb.delete) {
                status = res->cb.delete(ldap, apr_ldap_status(err->rc),
                                        matcheddn, (apr_ldap_control_t **)serverctrls,
                                        res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }


            apr_ldap_result_remove(ldap, res);

            if (matcheddn) {
                ldap_memfree(matcheddn);
            }

            if (errmsg) {
                ldap_memfree(errmsg);
            }

            if (serverctrls) {
                ldap_controls_free(serverctrls);
            }

            break;
        }
        case LDAP_RES_EXTENDED: {

            /* handle extended operation */

            char *roid = NULL;
            struct berval *rd = NULL;
            apr_buffer_t rdata;

            err->rc = ldap_parse_extended_result(ldap->ld, res->message, &roid,
                                        &rd, 0);

            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP extended operation: ldap_parse_result()";

            if (rd) {
                apr_buffer_mem_set(&rdata, rd->bv_val, rd->bv_len);
            }

            if (res->cb.ext) {
                status = res->cb.ext(ldap, apr_ldap_status(err->rc),
                                     roid, &rdata,
                                     res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }


            apr_ldap_result_remove(ldap, res);

            break;
        }
        default:
            break;
        }

    }

    /* we're done with this task, is there more work outstanding? */
    if (APR_SUCCESS == status && apr_skiplist_size(ldap->results)) {
        return APR_WANT_READ;
    }

    /* otherwise we're done */
    return status;
}



APU_DECLARE_LDAP(apr_status_t) apr_ldap_result(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               apr_interval_time_t timeout,
                                               apu_err_t *err)
{
    apr_ldap_result_t *res;
    LDAPMessage *msg;
    apr_ldap_result_t find;

    LDAP *ld = ldap->ld;

    apr_status_t status = APR_SUCCESS;

#if APR_HAS_MICROSOFT_LDAPSDK
    LDAP_TIMEVAL tv, *tvptr;
#else
    struct timeval tv, *tvptr;
#endif

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long) apr_time_sec(timeout);
        tv.tv_usec = (long) apr_time_usec(timeout);
        tvptr = &tv;
    }

    err->rc = ldap_result(ld, LDAP_RES_ANY, LDAP_MSG_ONE, tvptr, &msg);
    if (err->rc == -1) {
        err->reason = "LDAP: ldap_result() retrieval failed";

// FIXME - trigger the skiplist now, we know this connection is toast

#ifdef LDAP_OPT_ERROR_NUMBER
        ldap_get_option(ld, LDAP_OPT_ERROR_NUMBER, &err->rc);
#endif 
#ifdef LDAP_OPT_RESULT_CODE
        ldap_get_option(ld, LDAP_OPT_RESULT_CODE, &err->rc);
#endif 
        err->msg = ldap_err2string(err->rc);

        return apr_ldap_status(err->rc);
    }
    else if (err->rc == 0) {
        err->reason = "LDAP: ldap_result() timed out";
        err->rc = LDAP_TIMEOUT;
        err->msg = ldap_err2string(err->rc);
        return APR_ETIMEDOUT;
    }


#if APR_HAS_MICROSOFT_LDAPSDK
    find.msgid = msg->lm_msgid;
#else
    find.msgid = ldap_msgid(msg);
#endif

    res = apr_skiplist_find(ldap->results, &find, NULL);

    if (res) {

        /* we expect this message, fire off the relevant callback */

        switch(err->rc) {
        case LDAP_RES_BIND: {

            /*
             * A subtle bug exists in the implementation of the ldap_sasl_bind() and
             * ldap_sasl_bind_interactive() functions: "are we done yet?" and "make
             * the next write in the conversation" are combined into the same
             * function.
             *
             * What this means for the async version of the API is that the "are
             * we done yet?" question can only be asked when we are next writable,
             * just in case the answer to the "are we done yet?" question is no
             * and a write subsequently occurs.
             *
             * We generally get away with this because the server doesn't typically
             * decide when the connection is closed, so we're almost always writable
             * so we don't see a problem.
             */

            if (res->message) {
                /* two unprocessed bind messages would be weird, but don't leak */
                ldap_msgfree(res->message);
            }

            res->message = msg;

            return APR_WANT_WRITE;
        }
        case LDAP_RES_SEARCH_ENTRY: {

            /*
             * Search entries are sent back immediately as we receive them. The
             * expectation is our caller will wait until the search result message
             * before trying to send any further LDAP requests.
             */

            LDAPMessage *entry;
            const char *dn;

            char *attr;
            BerElement *ber;

            apr_ldap_search_entry_t e = { 0 };

            entry = ldap_first_entry(ldap->ld, msg);

            dn = ldap_get_dn(ldap->ld, entry);

            for (attr = ldap_first_attribute(ldap->ld, entry, &ber);
                 attr != NULL;
                 attr = ldap_next_attribute(ldap->ld, entry, ber)) {
                e.nattrs++;
                ldap_memfree(attr);
            }
            if (ber) {
                ber_free(ber,0);
            }

            for (attr = ldap_first_attribute(ldap->ld, entry, &ber);
                 attr != NULL;
                 attr = ldap_next_attribute(ldap->ld, entry, ber)) {

                struct berval **vals = ldap_get_values_len(ldap->ld, entry, attr);

                e.attr = attr;

                if (vals) {

                    int binary = 0;

                    char *sc = attr;

                    /* support for RFC4522 binary encoding option */
                    while ((sc = strchr(sc, ';'))) {
                        if (!apr_cstr_casecmpn(sc, ";binary", 7) && (sc[7] == 0 || sc[7] == ';')) {
                            binary = 1;
                            break;
                        }
                    }

                    e.nvals = ldap_count_values_len(vals);

                    for (e.vidx = 0; e.vidx < e.nvals; e.vidx++) {

                        char *str = NULL;

                        if (binary) {
                            apr_buffer_mem_set(&e.val, vals[e.vidx]->bv_val, vals[e.vidx]->bv_len);
                        }
                        else {
                            str = malloc(vals[e.vidx]->bv_len + 1);
                            str[vals[e.vidx]->bv_len] = 0;
                            memcpy(str, vals[e.vidx]->bv_val, vals[e.vidx]->bv_len);
                            apr_buffer_str_set(&e.val, str, vals[e.vidx]->bv_len);
                        }

                        if (res->entry_cb.search) {
                            status = res->entry_cb.search(ldap, dn, res->nentries, &e,
                                                          res->ctx, err);
                        }
                        else {
                            status = apr_ldap_status(err->rc);
                        }

                        if (str) {
                            free(str);
                        }
                    }

                }
                else {
                    if (res->entry_cb.search) {
                        status = res->entry_cb.search(ldap, dn, res->nentries, &e,
                                                      res->ctx, err);
                    }
                    else {
                        status = apr_ldap_status(err->rc);
                    }      
                }

                ldap_value_free_len(vals);
                ldap_memfree(attr);

                if (APR_SUCCESS != status) {
                    break;
                }

                e.aidx++;
            }
            if (ber) {
                ber_free(ber,0);
            }

            res->nentries++;

            if (res->entry_cb.search) {
                status = res->entry_cb.search(ldap, dn, res->nentries, NULL, res->ctx, err);
            }
            else {
                status = apr_ldap_status(err->rc);
            }    

            ldap_memfree((void *)dn);

            break;
        }
        case LDAP_RES_SEARCH_REFERENCE: {

            break;
        }
        case LDAP_RES_SEARCH_RESULT:
        case LDAP_RES_COMPARE:
        case LDAP_RES_ADD:
        case LDAP_RES_MODIFY:
        case LDAP_RES_RENAME:
        case LDAP_RES_DELETE:
        case LDAP_RES_EXTENDED: {

            /*
             * Set the result aside for callbacks to be fired when our LDAP socket
             * is next writable. This means that we can safely write the next LDAP
             * request in the callback without messing about.
             */

            if (res->message) {
                /* two unprocessed messages would be weird, but don't leak */
                ldap_msgfree(res->message);
            }

            res->message = msg;

            return APR_WANT_WRITE;
        }
        default:

            /* we don't (yet) recognise this message */
            break;
        }

    }
    else {

        /* we are no longer interested in this message - a pool was cleaned up */

        MSGID_T *msgid = apr_array_push(ldap->abandons);
        *msgid = find.msgid;

        ldap_msgfree(msg);

        return APR_WANT_WRITE;
    }

    ldap_msgfree(msg);

    /* we're done with this task, is there more work outstanding? */
    if (APR_SUCCESS == status && apr_skiplist_size(ldap->results)) {
        return APR_WANT_READ;
    }

    /* otherwise we're done */
    return status;
}




typedef struct apr_ldap_connection_t {
    apr_pool_t *pool;
    apr_ldap_t *ldap;
    apr_pollcb_t *poll;
    apr_pollfd_t socket_read;
    apr_pollfd_t socket_write;
    apu_err_t *err;
} apr_ldap_connection_t;

static apr_status_t apr_ldap_connection_cb(void *baton, apr_pollfd_t *descriptor)
{
    apr_ldap_connection_t *conn = (apr_ldap_connection_t *) baton;

    apr_status_t status = APR_SUCCESS;

    /* remove our event */
    apr_pollcb_remove(conn->poll, descriptor);

    /* are we ready to write? */
    if (descriptor->rtnevents & APR_POLLOUT) {

        /* send oustanding request */
        status = apr_ldap_process(conn->pool, conn->ldap, apr_time_from_sec(0), conn->err);

    }

    /* are we ready to read? */
    else if (descriptor->rtnevents & APR_POLLIN) {

        /* read outstanding responses */
        status = apr_ldap_result(conn->pool, conn->ldap, -1, conn->err);

    }

    return status;
}


APU_DECLARE_LDAP(apr_status_t) apr_ldap_poll(apr_pool_t *pool,
                                             apr_ldap_t *ldap,
                                             apr_pollcb_t *poll,
                                             apr_interval_time_t timeout,
                                             apu_err_t *err)
{
    apr_ldap_connection_t conn;
    apr_ldap_opt_t opt;

    apr_status_t status;

    status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_DESC, &opt, err);
    if (APR_SUCCESS != status) {
        return status;
    }

    /* set up read descriptor */
    conn.socket_read.desc_type = APR_POLL_SOCKET;
    conn.socket_read.reqevents = APR_POLLIN;
    conn.socket_read.desc.s = opt.socket;
    conn.socket_read.client_data = opt.socket;

    /* set up write descriptor */
    conn.socket_write.desc_type = APR_POLL_SOCKET;
    conn.socket_write.reqevents = APR_POLLOUT;
    conn.socket_write.desc.s = opt.socket;
    conn.socket_write.client_data = opt.socket;

    conn.ldap = ldap;
    conn.poll = poll;
    conn.err = err;

    status = APR_WANT_WRITE;

    do {

        if (APR_WANT_READ == status) {

            /* wait for socket to be readable, then process another result */
            status = apr_pollcb_add(conn.poll, &conn.socket_read);
            if (APR_SUCCESS != status) {
                break;
            }

        }
        else if (APR_WANT_WRITE == status) {

            /* wait for socket to be writeable, then process result */
            status = apr_pollcb_add(conn.poll, &conn.socket_write);
            if (APR_SUCCESS != status) {
                break;
            }

        }
        else {
            break;
        }

        status = apr_pollcb_poll(conn.poll, timeout, apr_ldap_connection_cb, &conn);

    } while (1);

    return status;
}


typedef struct apr_ldap_bind_ctx_t {
    apr_ldap_t *ld;
    apr_ldap_bind_interact_cb *interact;
    void *ctx;
    apr_status_t status;
} apr_ldap_bind_ctx_t;

#if APR_HAS_OPENLDAP_LDAPSDK && APR_HAVE_LDAP_SASL_INTERACTIVE_BIND

#if !defined(HAVE_SASL_H) && !defined(HAVE_SASL_SASL_H)
#error OpenLDAP was built with SASL support, but the SASL headers are not installed as required.
#endif

static int bind_sasl_interact(LDAP *ld, unsigned flags, void *ctx, void *in)
{
    apr_ldap_bind_ctx_t *payload = ctx;
    sasl_interact_t *sasl_interact = in;

    if (!ld) {
        return LDAP_PARAM_ERROR;
    }

    while( sasl_interact->id != SASL_CB_LIST_END ) {

        apr_ldap_bind_interact_t interaction;

        apr_status_t status;

        if (!payload->interact) {
            return LDAP_PARAM_ERROR;
        }

        memset(&interaction, 0, sizeof(apr_ldap_bind_interact_t));

        interaction.id = sasl_interact->id;
        interaction.challenge = sasl_interact->challenge;
        interaction.prompt = sasl_interact->prompt;
        interaction.defresult = sasl_interact->defresult;

        status = payload->interact(payload->ld, flags, &interaction, payload->ctx);

        if (status != APR_SUCCESS) {
            payload->status = status;
            return LDAP_PARAM_ERROR;
        }

        sasl_interact->result = apr_buffer_mem(&interaction.result, NULL);
        sasl_interact->len = apr_buffer_len(&interaction.result);

        sasl_interact++;
    }

    return LDAP_SUCCESS;
}

#endif


/**
 * APR LDAP bind function
 *
 * This function binds a previously initialised LDAP connection
 * to the directory.
 *
 * Binds are attempted as SASL interactive, falling back to a
 * standard SASL bind, falling back to a simple bind, depending
 * on the capabilities of the platform.
 *
 * Binds are attempted asynchronously. If APR_EAGAIN is returned,
 * this function must be called again.
 */
APU_DECLARE_LDAP(apr_status_t) apr_ldap_bind(apr_pool_t *pool, apr_ldap_t *ldap,
                                             const char *mech,
                                             apr_ldap_bind_interact_cb *interact_cb,
                                             void *interact_ctx,
                                             apr_interval_time_t timeout,
                                             apr_ldap_bind_cb bind_cb, void *bind_ctx,
                                             apu_err_t *err)
{
    apr_ldap_result_t *res;

    LDAPControl *sctrls[] = { 0 };
    LDAPControl *cctrls[] = { 0 };

    apr_ldap_bind_ctx_t payload;

    payload.ld = ldap;
    payload.interact = interact_cb;
    payload.ctx = interact_ctx;
    payload.status = APR_SUCCESS;

    MSGID_T msgid = 0;

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

    if (!mech) {

        /* No mechanism means we want a simple bind */

        const char *dn;
        struct berval cred;

        apr_ldap_bind_interact_t interaction;

        memset(err, 0, sizeof(*err));

        memset(&interaction, 0, sizeof(apr_ldap_bind_interact_t));

        interaction.id = APR_LDAP_INTERACT_DN;
        interaction.prompt = "Distinguished Name";

        payload.status = interact_cb(ldap, 0, &interaction, interact_ctx);

        if (payload.status != APR_SUCCESS) {
            return payload.status;
        }

        /* avoid unnecessary duplication */
        if (!apr_buffer_is_null(&interaction.result)) {
            if (apr_buffer_is_str(&interaction.result)) {
                dn = apr_buffer_str(&interaction.result);
            }
            else {
                dn = apr_buffer_pstrdup(pool, &interaction.result);
            }
        } else {
            dn = "";
        }

        memset(&interaction, 0, sizeof(apr_ldap_bind_interact_t));

        interaction.id = APR_LDAP_INTERACT_PASS;
        interaction.prompt = "Password";

        payload.status = interact_cb(ldap, 0, &interaction, interact_ctx);

        if (payload.status != APR_SUCCESS) {
            return payload.status;
        }

        if (!apr_buffer_is_null(&interaction.result)) {
            cred.bv_val = (char *)apr_buffer_mem(&interaction.result, NULL);
            cred.bv_len = TO_BV_LEN(apr_buffer_len(&interaction.result));
        } else {
            cred.bv_val = "";
            cred.bv_len = 0;
        }

        /*
         * ldap_simple_bind() is deprecated, so use ldap_sasl_bind() instead. In this
         * mode mechanism is null, the username is passed in the dn, and the
         * password is passed as a buffer to cred.
         */

#if APR_HAS_MICROSOFT_LDAPSDK
        err->rc = ldap_sasl_bind(ldap->ld, (char *)dn, NULL, &cred,
                                 NULL, NULL, &msgid);
#else
        err->rc = ldap_sasl_bind(ldap->ld, dn, LDAP_SASL_SIMPLE, &cred,
                                 NULL, NULL, &msgid);
#endif

        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: ldap_sasl_bind(SIMPLE) failed";
            return apr_ldap_status(err->rc);
        }
        else {
            memset(err, 0, sizeof(*err));
        }

        res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

        if (!res) {
            return APR_ENOMEM;
        }

        res->msgtype = LDAP_RES_BIND;
        res->cb.bind = bind_cb;
        res->ctx = bind_ctx;

        apr_ldap_result_add(pool, ldap, res, msgid);

        return APR_WANT_READ;

    }
    else {

#if APR_HAS_OPENLDAP_LDAPSDK && APR_HAVE_LDAP_SASL_INTERACTIVE_BIND

        const char *rmech;

        unsigned int flags = LDAP_SASL_QUIET;

        /* No distinguished name is a SASL bind */

        memset(err, 0, sizeof(*err));

        err->rc = ldap_sasl_interactive_bind(ldap->ld, NULL, mech,
                                             sctrls, cctrls, flags, bind_sasl_interact, &payload,
                                             NULL, &rmech, &msgid);

        if (err->rc == LDAP_SASL_BIND_IN_PROGRESS) {

            res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

            if (!res) {
                return APR_ENOMEM;
            }

            res->msgid = msgid;
            res->msgtype = LDAP_RES_BIND;
            res->rmech = rmech;
            res->cb.bind = bind_cb;
            res->ctx = bind_ctx;

            apr_ldap_result_add(pool, ldap, res, msgid);
        }

        if (APR_SUCCESS != payload.status) {
            return payload.status;
        }
        else if (err->rc == LDAP_SUCCESS) {
            return APR_SUCCESS;
        }
        else if (err->rc == LDAP_SASL_BIND_IN_PROGRESS) {
            return APR_WANT_READ;
        }
        else {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: ldap_sasl_interactive_bind() failed";
            return apr_ldap_status(err->rc);
        }

#else

        /*
         * for platforms that do not support ldap_sasl_interactive_bind(), alternative
         * implementations using ldap_sasl_bind() go here.
         */

        err->reason = "LDAP: SASL bind not yet supported by APR on this "
                      "LDAP SDK";
        err->rc = LDAP_UNWILLING_TO_PERFORM;
        return APR_ENOTIMPL;
#endif

    }

}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_compare(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                const char *dn,
                                                const char *attr,
                                                const apr_buffer_t *val,
                                                apr_array_header_t *serverctrls,
                                                apr_array_header_t *clientctrls,
                                                apr_interval_time_t timeout,
                                                apr_ldap_compare_cb compare_cb, void *compare_ctx,
                                                apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_ldap_result_t *res;

    struct berval bval;
    apr_size_t size;

    MSGID_T msgid = 0;

    apr_status_t status;

    status = apr_ldap_control_create(pool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(pool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    bval.bv_val = apr_buffer_mem(val, &size);
    bval.bv_len = TO_BV_LEN(size);

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

#if APR_HAS_MICROSOFT_LDAPSDK
    err->rc = ldap_compare_ext(ldap->ld, (char *)dn, (char *)attr, NULL, &bval,
                               sctrls, cctrls, &msgid);
#else
    err->rc = ldap_compare_ext(ldap->ld, dn, attr, &bval,
                               sctrls, cctrls, &msgid);
#endif

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_compare failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_COMPARE;
    res->cb.compare = compare_cb;
    res->ctx = compare_ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}



APU_DECLARE_LDAP(apr_status_t) apr_ldap_search(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn,
                                               apr_ldap_search_scope_e scope,
                                               const char *filter,
                                               const char **attrs,
                                               apr_ldap_switch_e attrsonly,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ssize_t sizelimit,
                                               apr_ldap_search_result_cb search_result_cb,
                                               apr_ldap_search_entry_cb search_entry_cb,
                                               void *search_ctx,
                                               apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_ldap_result_t *res;

#if APR_HAS_MICROSOFT_LDAPSDK
    ULONG timelimit;
#else
    struct timeval tv, *tvptr;
#endif

    MSGID_T msgid = 0;

    apr_status_t status;

    status = apr_ldap_control_create(pool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(pool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

#if APR_HAS_MICROSOFT_LDAPSDK

    timelimit = (ULONG)apr_time_sec(timeout);

    err->rc = ldap_search_ext(ldap->ld, (char *)dn, scope, (char *)filter, (char **)attrs, attrsonly,
                              sctrls, cctrls, timelimit, (ULONG)sizelimit, &msgid);
#else

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long) apr_time_sec(timeout);
        tv.tv_usec = (long) apr_time_usec(timeout);
        tvptr = &tv;
    }

    err->rc = ldap_search_ext(ldap->ld, (char *)dn, scope, (char *)filter, (char **)attrs, attrsonly,
                              sctrls, cctrls, tvptr, sizelimit, &msgid);
#endif

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_search failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_SEARCH_RESULT;
    res->cb.search = search_result_cb;
    res->entry_cb.search = search_entry_cb;
    res->ctx = search_ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_add(apr_pool_t *pool,
                                            apr_ldap_t *ldap,
                                            const char *dn,
                                            apr_array_header_t *adds,
                                            apr_array_header_t *serverctrls,
                                            apr_array_header_t *clientctrls,
                                            apr_interval_time_t timeout,
                                            apr_ldap_add_cb add_cb, void *ctx,
                                            apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_pool_t *tpool;

    apr_ldap_result_t *res;

    LDAPMod **mps, *ms;

    MSGID_T msgid = 0;

    int i, j;

    apr_status_t status;

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

    /* sanity check - any binary inconsistency? */
    for (i = 0; i < adds->nelts; ++i) {
        apr_ldap_pair_t *pair = &APR_ARRAY_IDX(adds, i, apr_ldap_pair_t);

        int is_str = 0;

        /* no adding attributes with no values */
        if (!pair->vals->nelts) {
            return APR_EINVAL;
        }

        /* no adding mixed strings / binary */
        for (j = 0; j < pair->vals->nelts; ++j) {
            apr_buffer_t *buf = &APR_ARRAY_IDX(pair->vals, j, apr_buffer_t);

            if (0 == j) {
                is_str = apr_buffer_is_str(buf);
            }
            else if (apr_buffer_is_str(buf) != is_str) {
                return APR_EINVAL;
            }
        }
    }

    /* all sane, let's translate into the LDAP world */
    apr_pool_create(&tpool, pool);

    status = apr_ldap_control_create(tpool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(tpool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    mps = apr_pcalloc(tpool, (adds->nelts + 1) * sizeof(LDAPMod *));
    ms = apr_pcalloc(tpool, (adds->nelts) * sizeof(LDAPMod));

    /* walk our attributes */
    for (i = 0; i < adds->nelts; ++i) {
        apr_ldap_pair_t *pair = &APR_ARRAY_IDX(adds, i, apr_ldap_pair_t);

        ms->mod_op = 0; /* entry add, no operation on attributes */
        ms->mod_type = (char *)pair->attr;

        for (j = 0; j < pair->vals->nelts; ++j) {
            apr_buffer_t *buf = &APR_ARRAY_IDX(pair->vals, j, apr_buffer_t);

            if (apr_buffer_is_str(buf)) {
                if (0 == j) {
                    ms->mod_vals.modv_strvals = apr_pcalloc(tpool, (pair->vals->nelts + 1) * sizeof(void *));
                }
                ms->mod_vals.modv_strvals[j] = apr_buffer_str(buf);
            }
            else {
                if (0 == j) {
                    ms->mod_op |= LDAP_MOD_BVALUES;
                    ms->mod_vals.modv_bvals = apr_pcalloc(tpool, (pair->vals->nelts + 1) * sizeof(void *));
                }
                ms->mod_vals.modv_bvals[j]->bv_val = apr_buffer_mem(buf, NULL);
                ms->mod_vals.modv_bvals[j]->bv_len = TO_BV_LEN(apr_buffer_len(buf));
            }
        }

        mps[i] = ms++;
    }

#if APR_HAS_MICROSOFT_LDAPSDK
    err->rc = ldap_add_ext(ldap->ld, (char *)dn, mps,
                           sctrls, cctrls, &msgid);
#else
    err->rc = ldap_add_ext(ldap->ld, dn, mps,
                           sctrls, cctrls, &msgid);
#endif

    apr_pool_destroy(tpool);

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_add failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_ADD;
    res->cb.add = add_cb;
    res->ctx = ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_modify(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn,
                                               apr_array_header_t *mods,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ldap_modify_cb modify_cb, void *ctx,
                                               apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_pool_t *tpool;

    apr_ldap_result_t *res;

    LDAPMod **mps, *ms;

    MSGID_T msgid = 0;

    int i, j;

    apr_status_t status;

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

    /* sanity check - any binary inconsistency? */
    for (i = 0; i < mods->nelts; ++i) {
        apr_ldap_modify_t *mod = &APR_ARRAY_IDX(mods, i, apr_ldap_modify_t);

        int is_str = 0;

        /* non recognised operations */
        switch (mod->op) {
        case APR_LDAP_MOD_ADD:
        case APR_LDAP_MOD_DELETE:
        case APR_LDAP_MOD_REPLACE:
        case APR_LDAP_MOD_INCREMENT:
            break;
        default:
            return APR_EINVAL;
        }

        /* no attribute */
        if (!mod->pair.attr) {
            return APR_EINVAL;
        }

        /* no adding attributes with no values */
        if (mod->op == APR_LDAP_MOD_ADD && !mod->pair.vals->nelts) {
            return APR_EINVAL;
        }

        /* no adding mixed strings / binary */
        for (j = 0; j < mod->pair.vals->nelts; ++j) {
            apr_buffer_t *buf = &APR_ARRAY_IDX(mod->pair.vals, j, apr_buffer_t);

            if (0 == j) {
                is_str = apr_buffer_is_str(buf);
            }
            else if (apr_buffer_is_str(buf) != is_str) {
                return APR_EINVAL;
            }
        }
    }

    /* all sane, let's translate into the LDAP world */
    apr_pool_create(&tpool, pool);

    status = apr_ldap_control_create(tpool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(tpool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    mps = apr_pcalloc(tpool, (mods->nelts + 1) * sizeof(LDAPMod *));
    ms = apr_pcalloc(tpool, (mods->nelts) * sizeof(LDAPMod));

    /* walk our attributes */
    for (i = 0; i < mods->nelts; ++i) {
        apr_ldap_modify_t *mod = &APR_ARRAY_IDX(mods, i, apr_ldap_modify_t);

        switch (mod->op) {
        case APR_LDAP_MOD_ADD:
            ms->mod_op = LDAP_MOD_ADD;
            break;
        case APR_LDAP_MOD_DELETE:
            ms->mod_op = LDAP_MOD_DELETE;
            break;
        case APR_LDAP_MOD_REPLACE:
            ms->mod_op = LDAP_MOD_REPLACE;
            break;
        case APR_LDAP_MOD_INCREMENT:
            ms->mod_op = LDAP_MOD_INCREMENT;
            break;
        }

        ms->mod_type = (char *)mod->pair.attr;

        for (j = 0; j < mod->pair.vals->nelts; ++j) {
            apr_buffer_t *buf = &APR_ARRAY_IDX(mod->pair.vals, j, apr_buffer_t);

            if (apr_buffer_is_str(buf)) {
                if (0 == j) {
                    ms->mod_vals.modv_strvals = apr_pcalloc(tpool, (mod->pair.vals->nelts + 1) * sizeof(void *));
                }
                ms->mod_vals.modv_strvals[j] = apr_buffer_str(buf);
            }
            else {
                if (0 == j) {
                    ms->mod_op |= LDAP_MOD_BVALUES;
                    ms->mod_vals.modv_bvals = apr_pcalloc(tpool, (mod->pair.vals->nelts + 1) * sizeof(void *));
                }
                ms->mod_vals.modv_bvals[j]->bv_val = apr_buffer_mem(buf, NULL);
                ms->mod_vals.modv_bvals[j]->bv_len = TO_BV_LEN(apr_buffer_len(buf));
            }
        }

        mps[i] = ms++;
    }

#if APR_HAS_MICROSOFT_LDAPSDK
    err->rc = ldap_modify_ext(ldap->ld, (char *)dn, mps,
                              sctrls, cctrls, &msgid);
#else
    err->rc = ldap_modify_ext(ldap->ld, dn, mps,
                              sctrls, cctrls, &msgid);
#endif

    apr_pool_destroy(tpool);

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_modify failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_MODIFY;
    res->cb.modify = modify_cb;
    res->ctx = ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_rename(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn, const char *newrdn, const char *newparent,
                                               apr_ldap_rename_e flags,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ldap_rename_cb rename_cb, void *ctx,
                                               apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_ldap_result_t *res;

    MSGID_T msgid = 0;

    apr_status_t status;

    status = apr_ldap_control_create(pool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(pool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

#if APR_HAS_MICROSOFT_LDAPSDK
    err->rc = ldap_rename_ext(ldap->ld, (char *)dn, (char *)newrdn, (char *)newparent, flags,
                              sctrls, cctrls, &msgid);
#else
    err->rc = ldap_rename(ldap->ld, dn, newrdn, newparent, flags,
                          sctrls, cctrls, &msgid);
#endif

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_rename failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_RENAME;
    res->cb.rename = rename_cb;
    res->ctx = ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_delete(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ldap_delete_cb delete_cb, void *ctx,
                                               apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_ldap_result_t *res;

    MSGID_T msgid = 0;

    apr_status_t status;

    status = apr_ldap_control_create(pool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(pool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

#if APR_HAS_MICROSOFT_LDAPSDK
    err->rc = ldap_delete_ext(ldap->ld, (char *)dn,
                              sctrls, cctrls, &msgid);
#else
    err->rc = ldap_delete_ext(ldap->ld, dn,
                              sctrls, cctrls, &msgid);
#endif

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_delete failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_DELETE;
    res->cb.delete = delete_cb;
    res->ctx = ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_extended(apr_pool_t *pool,
                                                 apr_ldap_t *ldap,
                                                 const char *oid,
                                                 apr_buffer_t *data,
                                                 apr_array_header_t *serverctrls,
                                                 apr_array_header_t *clientctrls,
                                                 apr_interval_time_t timeout,
                                                 apr_ldap_extended_cb ext_cb, void *ctx,
                                                 apu_err_t *err)
{
    LDAPControl **sctrls = NULL;
    LDAPControl **cctrls = NULL;

    apr_ldap_result_t *res;

    struct berval reqdata;
    struct berval *rd;

    MSGID_T msgid = 0;

    apr_status_t status;

    status = apr_ldap_control_create(pool, ldap, &sctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(pool, ldap, &cctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    {
        struct timeval tv, *tvptr;

        if (timeout < 0) {
            tvptr = NULL;
        }
        else {
            tv.tv_sec = (long) apr_time_sec(timeout);
            tv.tv_usec = (long) apr_time_usec(timeout);
            tvptr = &tv;
        }

        err->rc = ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, tvptr);
        if (err->rc != LDAP_SUCCESS) {
            err->msg = ldap_err2string(err->rc);
            err->reason = "LDAP: Could not set network timeout";
            return APR_EINVAL;
        }
    }
#endif

    if (!data || apr_buffer_is_null(data)) {
        rd = NULL;
    }
    else {
        reqdata.bv_val = apr_buffer_mem(data, NULL);
        reqdata.bv_len = TO_BV_LEN(apr_buffer_len(data));
        rd = &reqdata;
    }

#if APR_HAS_MICROSOFT_LDAPSDK
    err->rc = ldap_extended_operation(ldap->ld, (char *)oid, rd,
                                      sctrls, cctrls, &msgid);
#else
    err->rc = ldap_extended_operation(ldap->ld, oid, rd,
                                      sctrls, cctrls, &msgid);
#endif

    if (err->rc != LDAP_SUCCESS) {
        err->msg = ldap_err2string(err->rc);
        err->reason = "LDAP: ldap_extended_operation failed";
        return apr_ldap_status(err->rc);
    }
    else {
        memset(err, 0, sizeof(*err));
    }

    res = apr_pcalloc(pool, sizeof(apr_ldap_result_t));

    if (!res) {
        return APR_ENOMEM;
    }

    res->msgtype = LDAP_RES_EXTENDED;
    res->cb.ext = ext_cb;
    res->ctx = ctx;

    apr_ldap_result_add(pool, ldap, res, msgid);

    return APR_WANT_READ;
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_unbind(apr_ldap_t *ldap,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apu_err_t *err)
{

    apr_status_t status;

    status = apr_ldap_control_create(ldap->pool, ldap, &ldap->serverctrls, serverctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_ldap_control_create(ldap->pool, ldap, &ldap->clientctrls, clientctrls, err);

    if (APR_SUCCESS != status) {
        return status;
    }

    apr_pool_cleanup_run(ldap->pool, ldap, apr_ldap_cleanup);

    memcpy(err, &ldap->err, sizeof(apu_err_t));

    return ldap->status;
}

#if APR_HAVE_MODULAR_DSO

/* For DSO builds, export the table of entry points into the apr_ldap DSO
 * See include/private/apu_internal.h for the corresponding declarations
 */
APU_MODULE_DECLARE_DATA struct apr__ldap_dso_fntable apr__ldap_fns = {
    apr_ldap_info,
    apr_ldap_initialise,
    apr_ldap_option_get,
    apr_ldap_option_set,
    apr_ldap_connect,
    apr_ldap_prepare,
    apr_ldap_process,
    apr_ldap_result,
    apr_ldap_poll,
    apr_ldap_bind,
    apr_ldap_compare,
    apr_ldap_search,
    apr_ldap_add,
    apr_ldap_modify,
    apr_ldap_rename,
    apr_ldap_delete,
    apr_ldap_extended,
    apr_ldap_unbind
};

#endif /* APR_HAVE_MODULAR_DSO */

#endif /* APR_HAS_LDAP */

