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

#include "apr_strings.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"
#include <stdio.h>

#if APR_HAVE_STDLIB_H
#include <stdlib.h> /* for abort() */
#endif

#include "apu.h"
#include "apr_private.h"

#if APU_HAVE_LMDB

#include <lmdb.h>

#include "apr_dbm_private.h"

typedef struct {
    MDB_dbi dbi;
    MDB_cursor *cursor;
    MDB_txn *txn;
    MDB_env *env;
} real_file_t;


#define APR_DBM_LMDBMODE_RO       MDB_RDONLY
#define APR_DBM_LMDBMODE_RWCREATE MDB_CREATE
#define APR_DBM_LMDBMODE_RW       (MDB_RDONLY + MDB_CREATE + 1)
#define APR_DBM_LMDBMODE_RWTRUNC  (APR_DBM_LMDBMODE_RW + 1)

/* --------------------------------------------------------------------------
**
** UTILITY FUNCTIONS
*/

/* Map a DB error to an apr_status_t */
static apr_status_t db2s(int dberr)
{
    /* MDB_* error codes are negative, which are mapped to EGENERAL;
     * positive error codes are errno which maps directly to
     * apr_status_t. MDB_ codes could be mapped to some status code
     * region. */
    return dberr < 0 ? APR_EGENERAL : dberr;
}

/* Handle the return code of an mdb_* function (dberr), store the
 * error string for access via apr_dbm_geterror(), return translated
 * to an apr_status_t. */
static apr_status_t set_error(apr_dbm_t *dbm, int dberr)
{
    if ((dbm->errcode = dberr) == MDB_SUCCESS) {
        dbm->errmsg = NULL;
    }
    else {
        dbm->errmsg = mdb_strerror(dberr);
    }

    return db2s(dberr);
}

#if 0
static apr_status_t lmdb_retry(real_file_t *f, int dberr)
{
    if (f->cursor != 0)
//        slmdb_cursor_close(slmdb);

    if (dberr == MDB_MAP_FULL){
        
    } else if (dberr == MDB_MAP_RESIZED){
        
    }
}
#endif

/* --------------------------------------------------------------------------
**
** DEFINE THE VTABLE FUNCTIONS FOR LMDB
**
*/

#define DEFAULT_ENV_FLAGS (MDB_NOSUBDIR|MDB_NOSYNC)

static apr_status_t vt_lmdb_open(apr_dbm_t **pdb, const char *pathname,
                                 apr_int32_t mode, apr_fileperms_t perm,
                                 apr_pool_t *pool)
{
    real_file_t file;
    int dbi_open_flags = 0;
    int dbmode = 0;
    int truncate = 0;

    *pdb = NULL;
    switch (mode) {
    case APR_DBM_READONLY:
        dbmode = APR_DBM_LMDBMODE_RO;
        break;
    case APR_DBM_READWRITE:
        dbmode = APR_DBM_LMDBMODE_RW;
        break;
    case APR_DBM_RWCREATE:
        dbi_open_flags = APR_DBM_LMDBMODE_RWCREATE;
        break;
    case APR_DBM_RWTRUNC:
        truncate = APR_DBM_LMDBMODE_RWTRUNC;
        break;
    default:
        return APR_EINVAL;
    }

    {
        int dberr;
        file.txn = NULL;
        file.cursor = NULL;

        if ((dberr = mdb_env_create(&file.env)) == 0) {
            //XXX: properly set db size
            if ((dberr = mdb_env_set_mapsize(file.env, UINT32_MAX)) == 0){
                if ((dberr = mdb_env_open(file.env, pathname, dbmode | DEFAULT_ENV_FLAGS, apr_posix_perms2mode(perm))) == 0) {
                    if ((dberr = mdb_txn_begin(file.env, NULL, dbmode, &file.txn)) == 0){
                        if ((dberr = mdb_dbi_open(file.txn, NULL, dbi_open_flags, &file.dbi)) != 0){
                            /* close the env handler */
                            mdb_env_close(file.env);
                        }
                    }
                }
            }
        }

        if (truncate){
            if ((dberr = mdb_drop(file.txn, file.dbi, 0)) != 0){
                mdb_env_close(file.env);
            }
        }

        if (dberr != 0)
            return db2s(dberr);
    }

    /* we have an open database... return it */
    *pdb = apr_pcalloc(pool, sizeof(**pdb));
    (*pdb)->pool = pool;
    (*pdb)->type = &apr_dbm_type_lmdb;
    (*pdb)->file = apr_pmemdup(pool, &file, sizeof(file));

    /* ### register a cleanup to close the DBM? */

    return APR_SUCCESS;
}

static void vt_lmdb_close(apr_dbm_t *dbm)
{
    real_file_t *f = dbm->file;

    if (f->cursor){
        mdb_cursor_close(f->cursor);
        f->cursor = NULL;
    }

    if (f->txn){
       mdb_txn_commit(f->txn);
       f->txn = NULL;
    }

    mdb_dbi_close(f->env, f->dbi);
    mdb_env_close(f->env);

    f->env = NULL;
    f->dbi = 0;
}

static apr_status_t vt_lmdb_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t * pvalue)
{
    real_file_t *f = dbm->file;
    MDB_val ckey = { 0 };
    MDB_val rd = { 0 };
    int dberr;

    ckey.mv_data = key.dptr;
    ckey.mv_size = key.dsize;

    dberr = mdb_get(f->txn, f->dbi, &(ckey), &(rd));

    /* "not found" is not an error. return zero'd value. */
    if (dberr == MDB_NOTFOUND) {
        memset(&rd, 0, sizeof(rd));
        dberr = 0;
    }

    pvalue->dptr = rd.mv_data;
    pvalue->dsize = rd.mv_size;

    /* store the error info into DBM, and return a status code. Also, note
       that *pvalue should have been cleared on error. */
    return set_error(dbm, dberr);
}


//XXX: performance of store+del functions are very bad --> everything is one transaction
static apr_status_t vt_lmdb_store(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t value)
{
    real_file_t *f = dbm->file;
    int rv;
    MDB_val ckey = { 0 };
    MDB_val cvalue = { 0 };

    ckey.mv_data = key.dptr;
    ckey.mv_size = key.dsize;

    cvalue.mv_data = value.dptr;
    cvalue.mv_size = value.dsize;

    if ((rv = mdb_put(f->txn, f->dbi, &ckey, &cvalue, 0)) == 0) {
        /* commit transaction */
        if (((rv = mdb_txn_commit(f->txn)) == MDB_SUCCESS)
            && ((rv = mdb_txn_begin(f->env, NULL, 0, &f->txn)) == MDB_SUCCESS)) {
            f->cursor = NULL;
        }
    }

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static apr_status_t vt_lmdb_del(apr_dbm_t *dbm, apr_datum_t key)
{
    real_file_t *f = dbm->file;
    int rv;
    MDB_val ckey = { 0 };

    ckey.mv_data = key.dptr;
    ckey.mv_size = key.dsize;

    if ((rv = mdb_del(f->txn, f->dbi, &ckey, NULL)) == 0) {
        /* commit transaction */
        if (((rv = mdb_txn_commit(f->txn)) == MDB_SUCCESS)
            && ((rv = mdb_txn_begin(f->env, NULL, 0, &f->txn)) == MDB_SUCCESS)) {
            f->cursor = NULL;
        }
    }

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static int vt_lmdb_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    real_file_t *f = dbm->file;
    MDB_val ckey = { 0 };   /* converted key */
    MDB_val data = { 0 };
    int dberr;

    ckey.mv_data = key.dptr;
    ckey.mv_size = key.dsize;

    dberr = mdb_get(f->txn, f->dbi, &(ckey), &(data));

    /* note: the result data is "loaned" to us; we don't need to free it */

    /* DB returns DB_NOTFOUND if it doesn't exist. but we want to say
       that *any* error means it doesn't exist. */
    return dberr == 0;
}

static apr_status_t vt_lmdb_firstkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    real_file_t *f = dbm->file;
    MDB_val first, data;
    int dberr;

    if ((dberr = mdb_cursor_open(f->txn, f->dbi, &f->cursor)) == 0) {
        dberr = mdb_cursor_get(f->cursor, &first, &data, MDB_FIRST);
        if (dberr == MDB_NOTFOUND) {
            memset(&first, 0, sizeof(first));
            mdb_cursor_close(f->cursor);
            f->cursor = NULL;
            dberr = 0;
        }
    }

    pkey->dptr = first.mv_data;
    pkey->dsize = first.mv_size;

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, dberr);
}

static apr_status_t vt_lmdb_nextkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    real_file_t *f = dbm->file;
    MDB_val ckey, data;
    int dberr;

    ckey.mv_data = pkey->dptr;
    ckey.mv_size = pkey->dsize;

    if (f->cursor == NULL){
        return APR_EINVAL;
    }

    dberr = mdb_cursor_get(f->cursor, &ckey, &data, MDB_NEXT);
    if (dberr == MDB_NOTFOUND) {
        mdb_cursor_close(f->cursor);
        f->cursor = NULL;
        dberr = 0;
        ckey.mv_data = NULL;
        ckey.mv_size = 0;
    }

    pkey->dptr = ckey.mv_data;
    pkey->dsize = ckey.mv_size;

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, dberr);
}

static void vt_lmdb_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
    /* nothing to do */
}

static void vt_lmdb_usednames(apr_pool_t *pool, const char *pathname,
                              const char **used1, const char **used2)
{
    *used1 = apr_pstrdup(pool, pathname);
    *used2 = apr_pstrcat(pool, pathname, "-lock", NULL);
}


APR_MODULE_DECLARE_DATA const apr_dbm_driver_t apr_dbm_type_lmdb = {
    "lmdb",

    vt_lmdb_open,
    vt_lmdb_close,
    vt_lmdb_fetch,
    vt_lmdb_store,
    vt_lmdb_del,
    vt_lmdb_exists,
    vt_lmdb_firstkey,
    vt_lmdb_nextkey,
    vt_lmdb_freedatum,
    vt_lmdb_usednames
};

#endif /* APU_HAVE_LMDB */
