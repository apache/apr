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
 
 /* Setup:
  *  - Set environment variables as below for each URL being
  *     tested, along with dns and credentials.
  *  - Copy the server certificates to the data/ directory.
  *     All DER type certificates must have the .der extention.
  *     All BASE64 or PEM certificates must have the .b64
  *     extension.  All certificate files copied to the /data
  *     directory will be added to the ldap certificate store.
  */
   
 /* This test covers the following three types of connections:
  *
  *  - Unsecure ldap://
  *  - Secure ldaps://
  *  - Secure ldap://+Start_TLS
  *  - Unix ldapi://
  *
  * Environment variables:
  *
  * One or more of the following, based on what you have available.
  *
  * - TESTLDAP=ldap://localhost:389
  * - TESTLDAP_TLS=ldap://starttls.server:389
  * - TESTLDAPS=ldaps://ssl.server:636
  * - TESTLDAPI=/tmp/slapd-server.socket
  *
  * Simple bind:
  *
  * - TESTLDAP_DN=cn=Directory Manager
  * - TESTLDAP_PASS=opensesame
  *
  * SASL PLAIN bind:
  *
  * - TESTLDAP_USER=sasluser
  * - TESTLDAP_AUTHNAME=sasluser
  * - TESTLDAP_PASS=opensesame
  *
  * SASL EXTERNAL bind is attempted where supported.
  */

 /*
  * For a working example of an asynchronous LDAP conversation involving a bind,
  * followed by a search, followed by a compare, start at test_ldap_connection()
  * below.
  */

#include "testutil.h"

#include "apr.h"
#include "apr_general.h"
#include "apr_ldap.h"
#include "apr_escape.h"
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_poll.h"
#include "apr_strings.h"
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include "apr_want.h"

#define DIRNAME "data"
#define FILENAME DIRNAME "/host.data"
#define CERTFILEDER DIRNAME "/*.der"
#define CERTFILEB64 DIRNAME "/*.b64"

#if APR_HAS_LDAP

static int add_ldap_certs(abts_case *tc)
{
    apr_status_t status;
    apr_dir_t *thedir;
    apr_finfo_t dirent;
    apu_err_t err;

    if ((status = apr_dir_open(&thedir, DIRNAME, p)) == APR_SUCCESS) {
        apr_ldap_opt_tls_cert_t *cert = (apr_ldap_opt_tls_cert_t *)apr_pcalloc(p, sizeof(apr_ldap_opt_tls_cert_t));

        do {
            status = apr_dir_read(&dirent, APR_FINFO_MIN | APR_FINFO_NAME, thedir);
            if (APR_STATUS_IS_INCOMPLETE(status)) {
                continue; /* ignore un-stat()able files */
            }
            else if (status != APR_SUCCESS) {
                break;
            }

            if (strstr(dirent.name, ".der")) {
                cert->type = APR_LDAP_CA_TYPE_DER;
                cert->path = apr_pstrcat (p, DIRNAME, "/", dirent.name, NULL);
                apr_ldap_option_set(p, NULL, APR_LDAP_OPT_TLS_CERT, (void *)cert, &err);
                ABTS_TRUE(tc, err.rc == APR_SUCCESS);
            }
            if (strstr(dirent.name, ".b64")) {
                cert->type = APR_LDAP_CA_TYPE_BASE64;
                cert->path = apr_pstrcat (p, DIRNAME, "/", dirent.name, NULL);
                apr_ldap_option_set(p, NULL, APR_LDAP_OPT_TLS_CERT, (void *)cert, &err);
                ABTS_TRUE(tc, err.rc == APR_SUCCESS);
            }

        } while (1);

        apr_dir_close(thedir);
    }
    return 0;
}

static apr_status_t bind_interact_simple(apr_ldap_t *ld, unsigned int flags, apr_ldap_bind_interact_t *interact, void *ctx)
{ 

    switch (interact->id) {
    case APR_LDAP_INTERACT_DN: {
        char *dn = getenv("TESTLDAP_DN");
        apr_buffer_str_set(&interact->result, dn ? dn : "", APR_BUFFER_STRING);
        break;
    }
    case APR_LDAP_INTERACT_PASS: {
        char *pass = getenv("TESTLDAP_PASS");
        apr_buffer_str_set(&interact->result, pass ? pass : "opensesame", APR_BUFFER_STRING);
        break;
    }
    default:
        break;
    }

    return APR_SUCCESS;
} 
  
static apr_status_t bind_interact_external(apr_ldap_t *ld, unsigned int flags, apr_ldap_bind_interact_t *interact, void *ctx)
{
    return APR_SUCCESS;
}

static apr_status_t bind_interact_plain(apr_ldap_t *ld, unsigned int flags, apr_ldap_bind_interact_t *interact, void *ctx)
{

    switch (interact->id) {
    case APR_LDAP_INTERACT_USER: {
        char *user = getenv("TESTLDAP_USER");
        apr_buffer_str_set(&interact->result, user ? user : "", APR_BUFFER_STRING);
        break;
    }   
    case APR_LDAP_INTERACT_AUTHNAME: {
        char *authname = getenv("TESTLDAP_AUTHNAME");
        apr_buffer_str_set(&interact->result, authname ? authname : "", APR_BUFFER_STRING);
        break;
    }
    case APR_LDAP_INTERACT_PASS: {
        char *pass = getenv("TESTLDAP_PASS");
        apr_buffer_str_set(&interact->result, pass ? pass : "opensesame", APR_BUFFER_STRING);
        break;
    }    
    default:
        break;
    }   

    return APR_SUCCESS;
}

static apr_status_t bind_interact_abandon(apr_ldap_t *ld, unsigned int flags, apr_ldap_bind_interact_t *interact, void *ctx)
{

    switch (interact->id) {
    case APR_LDAP_INTERACT_USER: {
        apr_buffer_str_set(&interact->result, "cn=notexist", APR_BUFFER_STRING);
        break;
    }
    case APR_LDAP_INTERACT_AUTHNAME: {
        apr_buffer_str_set(&interact->result, "cn=notexist", APR_BUFFER_STRING);
        break;
    }
    case APR_LDAP_INTERACT_PASS: {
        apr_buffer_str_set(&interact->result, "wrongpassword", APR_BUFFER_STRING);
        break;
    }
    default:
        break;
    }

    return APR_SUCCESS;
}

typedef struct test_ldap_connection_t {
    abts_case *tc;
    apr_pool_t *pool;
    apr_ldap_t *ldap;
    apr_pool_t *bind_pool;
    const char *mech;
    apr_ldap_bind_interact_cb *interact;
    apr_pollcb_t *poll;
    apr_pollfd_t socket_read;
    apr_pollfd_t socket_write;
    apu_err_t err;
    int must_bind;
    const char *context;
    const char *dn;
    const char *rdn;
    const char *newdn;
} test_ldap_connection_t;

static apr_status_t test_ldap_vlv_result_cb(apr_ldap_t *ldap,
                                            apr_status_t status,
                                            apr_size_t nentries,
                                            const char *matcheddn,
                                            apr_hash_t *serverctrls,
                                            void *ctx, apu_err_t *err)
{
    char errbuf[128];

    apr_ldap_control_t *sort = apr_hash_get(serverctrls, APR_LDAP_CONTROL_SORT_RESPONSE_OID, APR_HASH_KEY_STRING);
    apr_ldap_control_t *vlv = apr_hash_get(serverctrls, APR_LDAP_CONTROL_VLV_RESPONSE_OID, APR_HASH_KEY_STRING);

    /*
     * Step 30: vlv search result callback triggered, result is complete.
     */

    abts_log_message("vlv search matcheddn: \n", matcheddn);

    if (sort) {
        abts_log_message("apr_ldap_search() sort control response: %s\n", apr_strerror(sort->c.sortrs.result, errbuf, sizeof(errbuf)));
    }

    if (vlv) {
        abts_log_message("apr_ldap_search() vlv control response: %s\n", apr_strerror(sort->c.vlvrs.result, errbuf, sizeof(errbuf)));
    }

    if (APR_SUCCESS == status) {
        /* success */
    }
    if (APR_STATUS_IS_OPERATIONS_ERROR(status)) {
        abts_log_message("apr_ldap_search() failed, most likely because the LDAP server does not have a VLV index configured: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }
    else {
        abts_log_message("apr_ldap_search() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }

    return status;
}

static apr_status_t test_ldap_vlv_entry_cb(apr_ldap_t *ldap,
                                           const char *dn,
                                           apr_size_t eidx,
                                           apr_ldap_search_entry_t *entry,
                                           void *ctx, apu_err_t *err)
{
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 29: search vlv entry callback triggered, start processing results.
     */

    if (entry && !entry->aidx && !entry->vidx) {
        /* first attribute and first value and attr present? output dn */
        abts_log_message("dn: %s", dn);
    }

    if (entry) {
        abts_log_message("%s: %s", entry->attr, apr_buffer_pstrdup(test->pool, &entry->val));
    }

    return APR_SUCCESS;
}

static apr_status_t test_ldap_delete_cb(apr_ldap_t *ldap,
                                        apr_status_t status,
                                        const char *matcheddn,
                                        apr_ldap_control_t **serverctrls,
                                        void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 27: delete result callback triggered, next is vlv search.
     */

    abts_log_message("delete matcheddn: \n", matcheddn);

    if (APR_SUCCESS == status) {

        /*
         * Step 28: we're writable, trigger a vlv search, then wait for readable.
         */
        apr_array_header_t *scontrols = apr_array_make(test->pool, 2, sizeof(apr_ldap_control_t));
        apr_ldap_control_t *sort = apr_array_push(scontrols);
        apr_ldap_control_t *vlv = apr_array_push(scontrols);
        apr_ldap_control_sortkey_t *sortkey;

        /* make a sort control */
        sort->type = APR_LDAP_CONTROL_SORT_REQUEST;
        sort->critical = 1;
        sort->c.sortrq.keys = apr_array_make(test->pool, 1, sizeof(apr_ldap_control_sortkey_t));

        /* sort by ou */
        sortkey = apr_array_push(sort->c.sortrq.keys);
        sortkey->attribute = "ou";
        sortkey->order = "caseIgnoreOrderingMatch";
        sortkey->direction = APR_LDAP_CONTROL_SORT_FORWARD;

        /* make a vlv control */
        vlv->type = APR_LDAP_CONTROL_VLV_REQUEST;
        vlv->c.vlvrq.before = 1; /* we want the one before the second entry */
        vlv->c.vlvrq.after = 1; /* we want one after the second entry */
        vlv->c.vlvrq.offset = 2; /* we want the second entry */

        status = apr_ldap_search(test->pool, test->ldap, test->context /* base dn */,
                                 APR_LDAP_SCOPE_ONELEVEL, "(objectclass=*)" /* filter */,
                                 NULL /* attrs */, APR_LDAP_OPT_ON /* attrsonly */,
                                 scontrols /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timelimit */, 0 /* sizelimit */,
                                 test_ldap_vlv_result_cb, test_ldap_vlv_entry_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else {
        abts_log_message("apr_ldap_delete() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
        ABTS_INT_EQUAL(test->tc, APR_SUCCESS, status);
    }

    return APR_SUCCESS;
}

static apr_status_t test_ldap_rename_cb(apr_ldap_t *ldap,
                                        apr_status_t status,
                                        const char *matcheddn,
                                        apr_ldap_control_t **serverctrls,
                                        void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 24: rename result callback triggered.
     */

    abts_log_message("rename matcheddn: \n", matcheddn);

    if (APR_SUCCESS == status || APR_ALREADY_EXISTS == status) {

        /*
         * Step 25: success or object already exists, it is time to trigger the delete.
         */

        status = apr_ldap_delete(test->pool, test->ldap, test->newdn /* dn to delete */,
                                 NULL /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timeout */,
                                 test_ldap_delete_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_delete: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else {
        abts_log_message("apr_ldap_rename() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
        ABTS_INT_EQUAL(test->tc, APR_SUCCESS, status);
    }

    return APR_SUCCESS;
}

static apr_status_t test_ldap_modify_cb(apr_ldap_t *ldap,
                                        apr_status_t status,
                                        const char *matcheddn,
                                        apr_ldap_control_t **serverctrls,
                                        void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 21: modify result callback triggered, result is complete.
     */

    abts_log_message("modify matcheddn: \n", matcheddn);

    if (APR_SUCCESS == status) {

        /*
         * Step 22: modify succeeded, it is time to trigger the rename.
         */

        test->rdn = "ou=Apache Portable Runtime Test Organizational Unit 2";
        test->newdn = apr_pstrcat(test->pool, test->rdn , ",", test->context, NULL);

        status = apr_ldap_rename(test->pool, test->ldap, test->dn /* dn to rename */,
                                 test->rdn /* new rdn */, NULL, APR_LDAP_RENAME_DELETEOLDRDN,
                                 NULL /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timeout */,
                                 test_ldap_rename_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_rename: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else {
        abts_log_message("apr_ldap_modify() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
        ABTS_INT_EQUAL(test->tc, APR_SUCCESS, status);
    }

    return APR_SUCCESS;
}

static apr_status_t test_ldap_add_cb(apr_ldap_t *ldap,
                                     apr_status_t status,
                                     const char *matcheddn,
                                     apr_ldap_control_t **serverctrls,
                                     void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 18: add result callback triggered, next is modify.
     */

    abts_log_message("add matcheddn: \n", matcheddn);

    if (APR_SUCCESS == status) {

        apr_array_header_t *mods = apr_array_make(test->pool, 10, sizeof(apr_ldap_modify_t));
        apr_ldap_modify_t *mod;
        apr_buffer_t *val;

        /* modify our newly added object */

        /*
         * Step 19: add successful, it is time to trigger the modify.
         */

        /* create objectclass */
        mod = apr_array_push(mods);
        mod->op = APR_LDAP_MOD_ADD;
        mod->pair.attr = "objectclass";
        mod->pair.vals = apr_array_make(test->pool, 1, sizeof(apr_buffer_t));

        /* objectclass: extensibleObject */
        val = apr_array_push(mod->pair.vals);
        apr_buffer_str_set(val, "extensibleObject", APR_BUFFER_STRING);



        status = apr_ldap_modify(test->pool, test->ldap, test->dn /* dn to modify */,
                                 mods /* modifications */,
                                 NULL /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timeout */,
                                 test_ldap_modify_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_modify: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else if (APR_ALREADY_EXISTS == status) {

        /* previous run has left objects lying around, clean up */

        /*
         * Step 25: add said object already exists, it is time to trigger the delete.
         */

        status = apr_ldap_delete(test->pool, test->ldap, test->dn /* dn to delete */,
                                 NULL /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timeout */,
                                 test_ldap_delete_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_delete: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else if (APR_INSUFFICIENT_ACCESS == status) {
        abts_log_message("apr_ldap_add() was denied, skipping: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }
    else {
        abts_log_message("apr_ldap_add() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
        ABTS_INT_EQUAL(test->tc, APR_SUCCESS, status);
    }

    return APR_SUCCESS;
}

static apr_status_t test_ldap_compare_cb(apr_ldap_t *ldap,
                                         apr_status_t status,
                                         const char *matcheddn,
                                         apr_ldap_control_t **serverctrls,
                                         void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 15: compare result callback triggered, next is add.
     */

    abts_log_message("comparison matcheddn: \n", matcheddn);

    ABTS_INT_EQUAL(test->tc, APR_COMPARE_TRUE, status);

    abts_log_message("\n");

    if (APR_COMPARE_TRUE != status) {
        abts_log_message("apr_ldap_compare() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }
    else if (!test->context) {
        abts_log_message("no namingContext in root DSE, we're done.\n");
    }
    else {

        apr_array_header_t *adds = apr_array_make(test->pool, 10, sizeof(apr_ldap_pair_t));
        apr_ldap_pair_t *pair;
        apr_buffer_t *val;

        /*
         * Step 16: compare successful, it is time to trigger the add.
         */

        test->dn = apr_pstrcat(test->pool, "ou=Apache Portable Runtime Test Organizational Unit,", test->context, NULL);

        /* create objectclass */
        pair = apr_array_push(adds);
        pair->attr = "objectclass";
        pair->vals = apr_array_make(test->pool, 2, sizeof(apr_buffer_t));

        /* objectclass: top */
        val = apr_array_push(pair->vals);
        apr_buffer_str_set(val, "top", APR_BUFFER_STRING);

        /* objectclass: organizationalUnit */
        val = apr_array_push(pair->vals);
        apr_buffer_str_set(val, "organizationalUnit", APR_BUFFER_STRING);

        /* create cn */
        pair = apr_array_push(adds);
        pair->attr = "ou";
        pair->vals = apr_array_make(test->pool, 1, sizeof(apr_buffer_t));

        /* cn: Apache Portable Runtime Test Organizational Unit */
        val = apr_array_push(pair->vals);
        apr_buffer_str_set(val, "Apache Portable Runtime Test Organizational Unit", APR_BUFFER_STRING);

        /* create description */
        pair = apr_array_push(adds);
        pair->attr = "description";
        pair->vals = apr_array_make(test->pool, 1, sizeof(apr_buffer_t));

        /* description: If found, please delete. */
        val = apr_array_push(pair->vals);
        apr_buffer_str_set(val, "This object was created by a test case. If found, please delete.", APR_BUFFER_STRING);

        status = apr_ldap_add(test->pool, test->ldap, test->dn /* dn to add */,
                              adds /* attribute-value pairs */,
                              NULL /* serverctls */, NULL /* clientctls */,
                              apr_time_from_sec(5) /* timeout */,
                              test_ldap_add_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_add: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }


    return APR_SUCCESS;
}      

static apr_status_t test_ldap_search_entry_cb(apr_ldap_t *ldap,
                                              const char *dn,
                                              apr_size_t eidx,
                                              apr_ldap_search_entry_t *entry,
                                              void *ctx, apu_err_t *err)
{
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 11: search entry callback triggered, start processing results.
     */

    if (entry && !entry->aidx && !entry->vidx) {
        /* first attribute and first value and attr present? output dn */
        abts_log_message("dn: %s", dn);
    }

    if (entry) {
        abts_log_message("%s: %s", entry->attr, apr_buffer_pstrdup(test->pool, &entry->val));

        if (!strcmp(entry->attr, "namingContexts")) {
            test->context = apr_buffer_pstrdup(test->pool, &entry->val);
        }
    }

    return APR_SUCCESS;
}

static apr_status_t test_ldap_search_result_cb(apr_ldap_t *ldap, 
                                               apr_status_t status,
                                               apr_size_t nentries,
                                               const char *matcheddn,
                                               apr_hash_t *serverctrls,
                                               void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    /*
     * Step 12: search result callback triggered, next is compare.
     */

    abts_log_message("\n");

    ABTS_INT_EQUAL(test->tc, APR_SUCCESS, status);

    if (APR_SUCCESS == status) {

        /*
         * Step 13: search successful, it is time to trigger the compare.
         */

        apr_buffer_t val;
        apr_buffer_str_set(&val, "top", APR_BUFFER_STRING);

        /*
         * Step 11: we're writable, trigger a compare, then wait for readable.
         */

        status = apr_ldap_compare(test->pool, test->ldap, "" /* root dn */,
                                 "objectclass" /* attribute */, &val /* value */,
                                 NULL /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timeout */,
                                 test_ldap_compare_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else {
        abts_log_message("apr_ldap_search() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }

    return status;
}

static apr_status_t test_ldap_whoami_cb(apr_ldap_t *ldap, apr_status_t status,
                                        const char *roid,
                                        apr_buffer_t *rdata,
                                        void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    if (APR_SUCCESS == status) {

        abts_log_message("whoami: %s\n", apr_buffer_pstrdup(test->pool, rdata));

        /*
         * Step 8: extended operation successful, it is time to trigger the search.
         */
        const char *attrs[2];

        apr_array_header_t *scontrols = apr_array_make(test->pool, 1, sizeof(apr_ldap_control_t));
        apr_ldap_control_t *page = apr_array_push(scontrols);

        /* make a page control */
        page->type = APR_LDAP_CONTROL_PAGE_REQUEST;
        page->critical = 0;
        page->c.pagerq.size = 1; /* one entry per page */

        attrs[0] = "+";
        attrs[1] = NULL;

        /*
         * Step 9: we're writable, trigger a search, then wait for readable.
         */

        status = apr_ldap_search(test->pool, test->ldap, "" /* root dn */,
                                 APR_LDAP_SCOPE_BASE, "(objectclass=*)" /* filter */,
                                 attrs /* attrs */, APR_LDAP_OPT_OFF /* attrsonly */,
                                 scontrols /* serverctls */, NULL /* clientctls */,
                                 apr_time_from_sec(5) /* timelimit */, 0 /* sizelimit */,
                                 test_ldap_search_result_cb, test_ldap_search_entry_cb, test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else {
        abts_log_message("apr_ldap_extended() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }

    return status;
}

static apr_status_t test_ldap_bind_cb(apr_ldap_t *ldap, apr_status_t status,
                                         const char *matcheddn,
                                         apr_ldap_control_t **serverctrls,
                                         void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    if (APR_SUCCESS == status) {

        /*
         * Step 5: bind successful, it is time to trigger the extended operation.
         */

        const char *whoami = "1.3.6.1.4.1.4203.1.11.3";

        /*
         * Step 6: we're writable, trigger an extended operation, then wait for readable.
         */

        status = apr_ldap_extended(test->pool, test->ldap, whoami, NULL,
                                   NULL /* serverctls */, NULL /* clientctls */,
                                   apr_time_from_sec(5), test_ldap_whoami_cb,
                                   test, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }
    else {
        abts_log_message("apr_ldap_bind() failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }

    return status;
}

static apr_status_t test_ldap_initialise_cb(apr_ldap_t *ldap, apr_status_t status,
                                            void *ctx, apu_err_t *err)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *)ctx;

    if (APR_SUCCESS == status) {

        /*
         * Step 2: we're writable, trigger the initial bind, then wait for readable.
         */

        status = apr_ldap_bind(test->pool, test->ldap, test->mech, test->interact, NULL,
                               apr_time_from_sec(0), test_ldap_bind_cb, test, &test->err);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            return status;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
            return status;
        }

    }
    else {
        abts_log_message("initialise failed: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
    }

    return status;
}

static apr_status_t test_ldap_connection_cb(void *baton, apr_pollfd_t *descriptor)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *) baton;

    apr_status_t status = APR_SUCCESS;

    /* remove our event */
    apr_pollcb_remove(test->poll, descriptor);

    /* are we ready to write? */
    if (descriptor->rtnevents & APR_POLLOUT) {

        /*
         * Step 4: bind callback triggered, continue the bind processing.
         */

        /* handle oustanding tasks */
        status = apr_ldap_process(test->pool, test->ldap, apr_time_from_sec(0), &test->err);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }

    /* are we ready to read? */
    else if (descriptor->rtnevents & APR_POLLIN) {

        /*
         * Step 3: we're readable, fetch the bind result, then request a write to be scheduled.
         */

        /*
         * Step 7: we're readable, fetch the extended operation result, then trigger extended callback.
         */

        /*
         * Step 10: we're readable, fetch the search result, then trigger search callback.
         */

        /*
         * Step 14: we're readable, fetch the compare result, then trigger compare callback.
         */

        /*
         * Step 17: we're readable, fetch the add result, then trigger add callback.
         */

        /*
         * Step 20: we're readable, fetch the modify result, then trigger modify callback.
         */

        /*
         * Step 23: we're readable, fetch the rename result, then trigger rename callback.
         */

        /*
         * Step 26: we're readable, fetch the delete result, then trigger delete callback.
         */

        status = apr_ldap_result(test->pool, test->ldap, -1, &test->err);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            break;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
        }

    }

    return status;
}

/*
 * Asynchronous test of an LDAP connection.
 *
 * Like all async code, we jump around a lot. In this example we make
 * an attempt to bind, and that that was successful, we make an
 * attempt to search for the root DSE, then if that was successful,
 * we attempt a compare.
 *
 * Start at "Step 1" and follow each step through the callbacks.
 */

static void test_ldap_connection(abts_case *tc, apr_pool_t *pool, apr_ldap_t *ldap,
                                 const char *mech, apr_ldap_bind_interact_cb *interact)
{
    char errbuf[128];
    apu_err_t err;
    test_ldap_connection_t test;
    apr_ldap_opt_t opt;
    apr_status_t status, want;

    memset(&test, 0, sizeof(test_ldap_connection_t));
 
    test.tc = tc;
    test.pool = pool;
    test.ldap = ldap;
    test.mech = mech;
    test.interact = interact;

    /* always default to LDAP V3 */
    opt.pv = APR_LDAP_VERSION3;
    apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_PROTOCOL_VERSION, &opt, &err);


    /*
     * Step 1: call us back when we're writable.
     */

    want = APR_WANT_WRITE;


    /* set the initialise callback */
    apr_ldap_prepare(pool, ldap, test_ldap_initialise_cb, &test);


    apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_HANDLE, &opt, &err);
    ABTS_TRUE(tc, opt.handle != NULL);

    status = apr_ldap_connect(pool, ldap, apr_time_from_sec(5), &err);

    if (APR_ENOTIMPL == status) {

        /* macos is a pain - as of this writing their ancient openldap
         * does not support ldap_connect. To work around this,
         * synchronously fire off the first callback, which will force
         * a connect, which will in turn avoid a bad descriptor on the
         * socket below.
         */

        status = apr_ldap_process(pool, ldap, apr_time_from_sec(0), &err);

        if (APR_WANT_READ == status) {
            status = APR_SUCCESS;
            want = APR_WANT_READ;
        }
    }

    if (APR_SUCCESS != status) {
        abts_log_message("%s - %s (%d)\n", err.reason, err.msg, err.rc);
        return;
    }


    status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_DESC, &opt, &err);
    ABTS_TRUE(tc, status == APR_SUCCESS);
    ABTS_ASSERT(tc, "failed to get descriptor", opt.socket != NULL);

    status = apr_pollcb_create(&test.poll, 1, pool, APR_POLLSET_DEFAULT);
    if (APR_ENOTIMPL == status) {
        abts_log_message("apr_pollcb_create() not implemented on this platform, skipping test\n");
        return;
    }
    ABTS_TRUE(tc, status == APR_SUCCESS);
    if (APR_SUCCESS != status) {
        return;
    }

    /* set up read descriptor */
    test.socket_read.desc_type = APR_POLL_SOCKET;
    test.socket_read.reqevents = APR_POLLIN;
    test.socket_read.desc.s = opt.socket;
    test.socket_read.client_data = opt.socket;

    /* set up write descriptor */
    test.socket_write.desc_type = APR_POLL_SOCKET;
    test.socket_write.reqevents = APR_POLLOUT;
    test.socket_write.desc.s = opt.socket;
    test.socket_write.client_data = opt.socket;

    do {

        if (APR_SUCCESS == want) {
            abts_log_message("apr_pollcb_poll() complete\n");
            break;
        }
        else if (APR_WANT_READ == want) {

            /* wait for socket to be readable, then process another result */
            status = apr_pollcb_add(test.poll, &test.socket_read);
            ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
            if (APR_SUCCESS != status) {
                break;
            }

        }
        else if (APR_WANT_WRITE == want) {

            /* wait for socket to be writeable, then process result */
            status = apr_pollcb_add(test.poll, &test.socket_write);
            ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
            if (APR_SUCCESS != status) {
                break;
            }

        }
        else {
            abts_log_message("apr_ldap_process/apr_ldap_result: %s [%s]\n", test.err.reason,
                              apr_strerror(want, errbuf, sizeof(errbuf)));
            break;
        }

        want = apr_pollcb_poll(test.poll, -1, test_ldap_connection_cb, &test);

    } while (1);


    /*
     * Step 31: bind, extended, search, compare, add, modify, rename, delete, search.
     *
     * result is complete, event loop unwound.
     */

    return;
}

/*
 * Synchronous test of an LDAP connection.
 *
 * The code below is identical to the above code, but uses the event loop inside
 * apr_ldap_poll().
 *
 * The initial callback test_ldap_initialise_cb is set, all queries are bootstrapped
 * from this callback.
 */

#if 0
static void test_ldap_connection(abts_case *tc, apr_pool_t *pool, apr_ldap_t *ldap,
                                 const char *mech, apr_ldap_bind_interact_cb *interact)
{
    char errbuf[128];
    apu_err_t err;
    test_ldap_connection_t test;
    apr_ldap_opt_t opt;
    apr_status_t status;

    memset(&test, 0, sizeof(test_ldap_connection_t));

    test.tc = tc;
    test.pool = pool;
    test.ldap = ldap;
    test.mech = mech;
    test.interact = interact;


    /* always default to LDAP V3 */
    opt.pv = APR_LDAP_VERSION3;
    apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_PROTOCOL_VERSION, &opt, &err);


    /* set the initialise callback */
    apr_ldap_prepare(pool, ldap, test_ldap_initialise_cb, &test);


    /* initial connect */
    status = apr_ldap_connect(pool, ldap, apr_time_from_sec(5), &err);

    if (APR_SUCCESS != status) {
        abts_log_message("%s - %s (%d)\n", err.reason, err.msg, err.rc);
        return;
    }


    /* run the callbacks */
    status = apr_ldap_poll(pool, ldap, apr_time_from_sec(5), &err);

    if (APR_SUCCESS == status) {
        abts_log_message("apr_pollcb_poll() complete\n");
    }
    else {
        abts_log_message("apr_ldap_process/apr_ldap_result: %s [%s]\n", test.err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
    }


    return;
}
#endif


static apr_status_t test_ldap_abandon_cb(void *baton, apr_pollfd_t *descriptor)
{
    char errbuf[128];
    test_ldap_connection_t *test = (test_ldap_connection_t *) baton;

    apr_status_t status = APR_SUCCESS;

    /* remove our event */
    apr_pollcb_remove(test->poll, descriptor);

    /* are we ready to write? */
    if (descriptor->rtnevents & APR_POLLOUT) {

        if (test->must_bind) {

            /*
             * Step B: we're writable, trigger the initial bind from the bind_pool, then wait for readable.
             */

            status = apr_ldap_bind(test->bind_pool, test->ldap, test->mech, test->interact, NULL, apr_time_from_sec(0), test_ldap_bind_cb, test, &test->err);

            test->must_bind = 0;

            ABTS_INT_EQUAL(test->tc, APR_WANT_READ, status);

            if (APR_SUCCESS == status) {
                /* should not have happened */
                return status;
            }
            else if (APR_WANT_READ == status) {

                /*
                 * Step C: spanner in the works - lets abandon the bind.
                 */

                apr_pool_destroy(test->bind_pool);

                test->bind_pool = NULL;

                abts_log_message("bind abandon triggered\n");

                /* and then continue as if nothing had happened */

                return status;

            }
            else {
                return status;
            }

        }

        /*
         * Step E: write callback triggered, send the abandon.
         */

        /* handle oustanding tasks */
        status = apr_ldap_process(test->pool, test->ldap, apr_time_from_sec(0), &test->err);

        ABTS_INT_EQUAL(test->tc, APR_SUCCESS, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            return status;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
            return status;
        }

    }

    /* are we ready to read? */
    if (descriptor->rtnevents & APR_POLLIN) {

        /*
         * Step D: we're readable, but our bind has been abandoned.
         */

        status = apr_ldap_result(test->pool, test->ldap, -1, &test->err);

        ABTS_INT_EQUAL(test->tc, APR_WANT_WRITE, status);

        switch (status) {
        case APR_SUCCESS:
        case APR_WANT_READ:
        case APR_WANT_WRITE:
            return status;
        default:
            abts_log_message("apr_ldap_result: %s [%s]\n", test->err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
            return status;
        }

    }

    return status;
}

static void test_ldap_abandon(abts_case *tc, apr_pool_t *pool, apr_ldap_t *ldap)
{
    char errbuf[128];
    apu_err_t err;
    test_ldap_connection_t test;
    apr_ldap_opt_t opt;
    apr_status_t status;

    memset(&test, 0, sizeof(test_ldap_connection_t));

    test.tc = tc;
    test.pool = pool;
    test.ldap = ldap;
    test.mech = "PLAIN";
    test.interact = bind_interact_abandon;

    test.must_bind = 1;

    apr_pool_create(&test.bind_pool, test.pool);


    /* always default to LDAP V3 */
    opt.pv = APR_LDAP_VERSION3;
    apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_PROTOCOL_VERSION, &opt, &err);

    apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_HANDLE, &opt, &err);
    ABTS_TRUE(tc, opt.handle != NULL);

    status = apr_ldap_connect(pool, ldap, apr_time_from_sec(5), &err);

    if (APR_SUCCESS != status) {
        abts_log_message("%s - %s (%d)\n", err.reason, err.msg, err.rc);
        return;
    }

    status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_DESC, &opt, &err);
    ABTS_TRUE(tc, status == APR_SUCCESS);
    ABTS_ASSERT(tc, "failed to get descriptor", opt.socket != NULL);

    status = apr_pollcb_create(&test.poll, 1, pool, APR_POLLSET_DEFAULT);
    if (APR_ENOTIMPL == status) {
        abts_log_message("apr_pollcb_create() not implemented on this platform, skipping test\n");
        return;
    }
    ABTS_TRUE(tc, status == APR_SUCCESS);
    if (APR_SUCCESS != status) {
        return;
    }

    /* set up read descriptor */
    test.socket_read.desc_type = APR_POLL_SOCKET;
    test.socket_read.reqevents = APR_POLLIN;
    test.socket_read.desc.s = opt.socket;
    test.socket_read.client_data = opt.socket;

    /* set up write descriptor */
    test.socket_write.desc_type = APR_POLL_SOCKET;
    test.socket_write.reqevents = APR_POLLOUT;
    test.socket_write.desc.s = opt.socket;
    test.socket_write.client_data = opt.socket;


    /*
     * Step A: call us back when we're writable.
     */

    status = APR_WANT_WRITE;

    do {

        if (APR_SUCCESS == status) {
            abts_log_message("apr_pollcb_poll() complete\n");
            break;
        }
        else if (APR_WANT_READ == status) {

            /* wait for socket to be readable, then process another result */
            status = apr_pollcb_add(test.poll, &test.socket_read);
            ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
            if (APR_SUCCESS != status) {
                break;
            }

        }
        else if (APR_WANT_WRITE == status) {

            /* wait for socket to be writeable, then process result */
            status = apr_pollcb_add(test.poll, &test.socket_write);
            ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
            if (APR_SUCCESS != status) {
                break;
            }

        }
        else {
            abts_log_message("apr_ldap_result: %s [%s]\n", test.err.reason,
                              apr_strerror(status, errbuf, sizeof(errbuf)));
            break;
        }

        status = apr_pollcb_poll(test.poll, -1, test_ldap_abandon_cb, &test);

    } while (1);

    /*
     * Step F: bind, result, abandon is complete, event loop unwound.
     */

    return;
}


static void test_ldap_global_opts(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    apu_err_t err;
    apr_ldap_opt_t opt;
    apr_status_t status;

    char errbuf[128];

    apr_pool_create(&pool, p);

    status = apr_ldap_option_get(pool, NULL, APR_LDAP_OPT_API_INFO, &opt, &err);

    if (status != APR_SUCCESS) {
        abts_log_message("apr_ldap_option_get: %s [%s]\n", err.reason,
                          apr_strerror(status, errbuf, sizeof(errbuf)));
    }

    ABTS_TRUE(tc, status == APR_SUCCESS);
    ABTS_TRUE(tc, err.rc == 0);
    ABTS_TRUE(tc, opt.info.vendor_name != NULL);

    opt.ldfi.name = "THREAD_SAFE";
    status = apr_ldap_option_get(pool, NULL, APR_LDAP_OPT_API_FEATURE_INFO, &opt, &err);
    /* feature may exist, or may not */
    ABTS_TRUE(tc, status == APR_SUCCESS || status == APR_EINVAL);

    status = apr_ldap_option_get(pool, NULL, APR_LDAP_OPT_DEBUG_LEVEL, &opt, &err);
    if (APR_ENOTIMPL == status) {
        abts_log_message("apr_ldap_option_get(APR_LDAP_OPT_DEBUG_LEVEL) not implemented on this platform, skipping test\n");
        return;
    }       
    else {
        ABTS_TRUE(tc, status == APR_SUCCESS);
    }

    apr_pool_destroy(pool);
}

static void test_ldap_opts(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    apr_ldap_t *ldap = NULL;
    apu_err_t err;
    const char *url;

    apr_status_t status;

    apr_pool_create(&pool, p);

    url = apr_psprintf(pool, "ldap://%s:%d", "localhost", APR_LDAP_PORT);

    status = apr_ldap_initialise(pool, &ldap, &(err));

    if (APR_SUCCESS != status) {
        char errbuf[128];
        abts_log_message("apr_ldap_initialise() skipped: %s\n", apr_strerror(status, errbuf, sizeof(errbuf)));
        return;
    }

    ABTS_TRUE(tc, ldap != NULL);

    if (ldap) {

        apr_ldap_opt_t opt;

        opt.uri = url;
        status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_URI, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS); 
        status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_URI, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        ABTS_ASSERT(tc, "failed to set uri", !strcmp(opt.uri, url));

        opt.pv = APR_LDAP_VERSION3;
        status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_PROTOCOL_VERSION, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_PROTOCOL_VERSION, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        ABTS_ASSERT(tc, "failed to set protocol version", opt.pv == APR_LDAP_VERSION3);

        opt.deref = APR_LDAP_DEREF_ALWAYS;
        status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_DEREF, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_DEREF, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        ABTS_ASSERT(tc, "failed to set deref", opt.deref == APR_LDAP_DEREF_ALWAYS);

        opt.refs = APR_LDAP_OPT_ON;
        status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_REFERRALS, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_REFERRALS, &opt, &err);
        ABTS_TRUE(tc, status == APR_SUCCESS);
        ABTS_ASSERT(tc, "failed to set referrals", opt.refs == APR_LDAP_OPT_ON);

#if 0
        opt.refhoplimit = 5;
        status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_REFHOPLIMIT, &opt, &err);
        if (APR_ENOTIMPL != status) {
            ABTS_TRUE(tc, status == APR_SUCCESS); 
            status = apr_ldap_option_get(pool, ldap, APR_LDAP_OPT_REFHOPLIMIT, &opt, &err);
            ABTS_TRUE(tc, status == APR_SUCCESS); 
            ABTS_ASSERT(tc, "failed to set refhoplimit", opt.refhoplimit == 5);
        }
#endif

    }

    apr_pool_destroy(pool);
}

static void test_ldap(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    apr_ldap_t *ldap;
    apu_err_t err;
    const char *url;
    apr_ldap_opt_t opt;
    apr_status_t status;

    const char *testldap = getenv("TESTLDAP");

    if (!testldap) {
        ABTS_NOT_IMPL(tc, "Environment TESTLDAP unset - skipping");
        return;
    }

    apr_pool_create(&pool, p);

    url = apr_psprintf(pool, "ldap://%s:%d", testldap, APR_LDAP_PORT);
    
    apr_ldap_initialise(pool, &ldap, &(err));

    ABTS_TRUE(tc, ldap != NULL);

    opt.uri = url;
    status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_URI, &opt, &err);

    if (status == APR_SUCCESS) {
        test_ldap_connection(tc, pool, ldap, NULL, bind_interact_simple);
    }

    apr_pool_destroy(pool);
}

static void test_ldaps(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    apr_ldap_t *ldap;
    apu_err_t err;
    const char *url; 
    apr_ldap_opt_t opt;
    apr_status_t status;

    const char *testldaps = getenv("TESTLDAPS");

    if (!testldaps) {
        ABTS_NOT_IMPL(tc, "Environment TESTLDAPS unset - skipping");
        return;
    }

    apr_pool_create(&pool, p);

    url = apr_psprintf(pool, "ldaps://%s:%d", testldaps, APR_LDAPS_PORT);

    apr_ldap_initialise(pool, &ldap, &(err));

    ABTS_TRUE(tc, ldap != NULL);

    opt.uri = url;
    status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_URI, &opt, &err);

    if (status == APR_SUCCESS) {
        add_ldap_certs(tc);

        test_ldap_connection(tc, pool, ldap, "EXTERNAL", bind_interact_external);
        test_ldap_connection(tc, pool, ldap, "PLAIN", bind_interact_plain);
        test_ldap_connection(tc, pool, ldap, NULL, bind_interact_simple);
    }

    apr_pool_destroy(pool);
}

static void test_ldap_tls(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    apr_ldap_t *ldap;
    apu_err_t err;
    const char *url;
    apr_ldap_opt_t opt;
    apr_status_t status;

    const char *testldap_tls = getenv("TESTLDAP_TLS");

    if (!testldap_tls) {
        ABTS_NOT_IMPL(tc, "Environment TESTLDAP_TLS unset - skipping");
        return;
    }

    apr_pool_create(&pool, p);

    opt.tls = APR_LDAP_TLS_STARTTLS;

    url = apr_psprintf(pool, "ldap://%s:%d", testldap_tls, APR_LDAP_PORT);

    apr_ldap_initialise(pool, &ldap, &(err));

    if (err.rc == APR_SUCCESS) {
        apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_TLS, &opt, &(err));
    }

    ABTS_TRUE(tc, ldap != NULL);

    opt.uri = url;
    status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_URI, &opt, &err);

    if (status == APR_SUCCESS) {
        add_ldap_certs(tc);

        test_ldap_connection(tc, pool, ldap, "EXTERNAL", bind_interact_external);
        test_ldap_connection(tc, pool, ldap, "PLAIN", bind_interact_plain);
        test_ldap_connection(tc, pool, ldap, NULL, bind_interact_simple);
    }

    apr_pool_destroy(pool);
}

static void test_ldapi(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    apr_ldap_t *ldap;
    apu_err_t err;
    const char *url;
    apr_ldap_opt_t opt;
    apr_status_t status;

    const char *testldapi = getenv("TESTLDAPI");

    if (!testldapi) {
        ABTS_NOT_IMPL(tc, "Environment TESTLDAPI unset - skipping");
        return;
    }

    apr_pool_create(&pool, p);

    url = apr_psprintf(pool, "ldapi://%s", apr_pescape_urlencoded(pool, testldapi));

    apr_ldap_initialise(pool, &ldap, &(err));

    ABTS_TRUE(tc, ldap != NULL);

    opt.uri = url;
    status = apr_ldap_option_set(pool, ldap, APR_LDAP_OPT_URI, &opt, &err);
    ABTS_TRUE(tc, status == APR_SUCCESS);

    abts_log_message("Initialiased LDAP url [%s]: %s - %s (%d)\n", url, err.reason, err.msg, err.rc);

    if (status == APR_SUCCESS) {
        test_ldap_connection(tc, pool, ldap, "EXTERNAL", bind_interact_external);
        test_ldap_connection(tc, pool, ldap, "PLAIN", bind_interact_plain);
        test_ldap_connection(tc, pool, ldap, NULL, bind_interact_simple);

        test_ldap_abandon(tc, pool, ldap);
    }

    status = apr_ldap_unbind(ldap, NULL, NULL, &err);
    ABTS_TRUE(tc, status == APR_SUCCESS);

    apr_pool_destroy(pool);
}

#endif /* APR_HAS_LDAP */

abts_suite *testldap(abts_suite *suite)
{
#if APR_HAS_LDAP
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_ldap_global_opts, NULL);
    abts_run_test(suite, test_ldap_opts, NULL);

    abts_run_test(suite, test_ldap, NULL);
    abts_run_test(suite, test_ldaps, NULL);
    abts_run_test(suite, test_ldap_tls, NULL);
    abts_run_test(suite, test_ldapi, NULL);

#endif /* APR_HAS_LDAP */

    return suite;
}

