/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#ifndef AP_BUF_H
#define AP_BUF_H

#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_private.h"
/* Currently we need this, but when the filtering is done, the iol's should
 * just go away all together, and so will this.  :-)  */
#include "../../../include/ap_iol.h"
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>	/* for struct iovec */
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif


/* The basic concept behind bucket_brigades.....
 *
 * A bucket brigade is simply a Queue of buckets, where we aren't limited
 * to inserting at the front and removing at the end.
 *
 * Buckets are just data stores.  They can be files, mmap areas, or just
 * pre-allocated memory.  The point of buckets is to store data.  Along with
 * that data, come some functions to access it.  The functions are relatively
 * simple, read, write, getlen, split, and free.
 *
 * read reads a string of data.  Currently, it assumes we read all of the 
 * data in the bucket.  This should be changed to only read the specified 
 * amount.
 *
 * getlen gets the number of bytes stored in the bucket.
 * 
 * write writes the specified data to the bucket.  Depending on the type of
 * bucket, this may append to the end of previous data, or wipe out the data
 * currently in the bucket.  rwmem buckets append currently, all others 
 * erase the current bucket.
 *
 * split just makes one bucket into two at the spefied location.  To implement
 * this correctly, we really need to implement reference counting.
 *
 * free just destroys the data associated with the bucket.
 *
 * We may add more functions later.  There has been talk of needing a stat,
 * which would probably replace the getlen.  And, we definately need a convert
 * function.  Convert would make one bucket type into another bucket type.
 *
 * To write a bucket brigade, they are first made into an iovec, so that we
 * don't write too little data at one time.  Currently we ignore compacting the
 * buckets into as few buckets as possible, but if we really want to be
 * performant, then we need to compact the buckets before we convert to an
 * iovec, or possibly while we are converting to an iovec.
 *
 * I'm not really sure what else to say about the buckets.  They are relatively
 * simple and straight forward IMO.  It is just a way to organize data in
 * memory that allows us to modify that data and move it around quickly and
 * easily.
 */

/* The types of bucket brigades the code knows about.  We really don't need
 * this enum.  All access to the bucket brigades can be done through function
 * pointers in the bucket type.  However, when we start to do conversion
 * routines, this enum will be a huge performance benefit, so we leave it
 * alone.  As of this moment, only rwmem, rmem, mmap, and eos buckets have
 * been implemented.  The rest will wait until the filtering design is
 * decided upon, or until somebody gets around to them. 
 */
typedef enum {
    AP_BUCKET_rwmem,
    AP_BUCKET_rmem,
    AP_BUCKET_file,
    AP_BUCKET_mmap,
    AP_BUCKET_filename,
    AP_BUCKET_cached_entity,
    AP_BUCKET_URI,
    AP_BUCKET_eos        /* End-of-stream bucket.  Special case to say this is
                          * the end of the bucket so all data should be sent
                          * immediately. */
} ap_bucket_color_e;

typedef struct ap_bucket ap_bucket;
/*
 * The basic bucket type.  This is an abstraction on top of all other bucket
 * types.  This contains the type of bucket, a pointer to the bucket, and
 * a couple of function pointers.  Doing it this way, lets us morph buckets
 * from one type to another relatively easily.  Just change the data pointer
 * to point to the new bucket, and replace all of the function pointers.
 *
 * This also allows for a very simple interface for all features of buckets
 * for all bucket types.   (does that make any sense at all?)
 *
 * The current functions are:
 * getlen   -- get the length of the data in the bucket 
 *                                        (likely to be replaced soon)
 * read     -- read the data in the bucket (not garaunteed to read it all)
 * write    -- insert data into the bucket
 * split    -- split one bucket into two buckets
 * free     -- destroy the bucket, freeing it's memory
 *
 * funtions to be added:
 * stat     -- get all of the metadata about the bucket (lifetime, type, etc.)
 * convert  -- change one bucket type into another bucket type.
 *
 * There are also pointer to the next and previus buckets in the list.
 */
struct ap_bucket {
    ap_bucket_color_e color;              /* what type of bucket is it */
    void *data;				  /* for use by free() */

    /* All of the function pointers that can act on a bucket. */
    void (*free)(void *e);                /* can be NULL */
    int (*getlen)(ap_bucket *e);          /* Get the length of the string */

    /* Read the data from the bucket. */
    const char *(*read)(ap_bucket *e);  /* Get the string */

    /* Write into a bucket.  The buf is a different type based on the
     * bucket type used.  For example, with AP_BUCKET_mmap it is an ap_mmap_t
     * for AP_BUCKET_file it is an ap_file_t, and for AP_BUCKET_rwmem it is
     * a char *.  The nbytes is the amount of actual data in buf.  This is
     * not the sizeof(buf), it is the actual number of bytes in the char *
     * that buf resolves to.  written is how much of that data was inserted
     * into the bucket.
     */ 
    int (*write)(ap_bucket *e, const void *buf, ap_size_t nbytes, ap_ssize_t *w);
   
    /* Split one bucket into to at the specified position */
    ap_status_t (*split)(ap_bucket *e, ap_size_t nbytes);

    ap_bucket *next;                     /* The next node in the bucket list */
    ap_bucket *prev;                     /* The prev node in the bucket list */
};

typedef struct ap_bucket_brigade ap_bucket_brigade;
/*
 * This is the basic bucket brigade.  That means it is a list of buckets.
 * It has a pool out of which the buckets and the bucket brigade are allocated.
 * That may change though, because I am leaning towards make the buckets have
 * the same lifetime as the data they store in most cases.  It also has a
 * pointer to the head and tail of the bucket list.  This allows us to
 * easily remove data from the bucket list, and to easily append data at
 * the end.  By walking the list, it is also possible to insert in the middle
 * of the list.
 */
struct ap_bucket_brigade {
    ap_pool_t *p;                       /* The pool to associate this with.
                                           I do not allocate out of the pool,
                                           but this lets me register a cleanup
                                           to put a limit on the brigade's 
                                           lifetime. */
    ap_bucket *head;                    /* The start of the brigade */
    ap_bucket *tail;                    /* The end of the brigade */
};

/*    ******  Different bucket types   *****/

typedef struct ap_bucket_rmem ap_bucket_rmem;
/*
 * The Read only bucket type.  This is basically for memory allocated off the
 * stack or literal strings.  It cannot be modified, and the lifetime is
 * defined by when it was allocated.  Most likely these should be split into
 * two different types.  This contains a pointer to the front and end of the
 * string so that it is possible to remove characters at either end.
 */
struct ap_bucket_rmem {
    size_t  alloc_len;                  /* how much was allocated */
    const void    *start;               /* Where does the actual data start
                                           in the alloc'ed block */
    const void    *end;                 /* where does the data actually end? */
};

typedef struct ap_bucket_rwmem ap_bucket_rwmem;
/*
 * The read/write memory bucket type.  This is for data that has been 
 * allocated out of the heap.  This bucket actually starts by allocating
 * 4K of memory.  We do this so that the bucket has room to grow.  At the
 * bottom of the filter stack, we are likely to have to condense the buckets
 * to as few as possible.  By allocating a big space at the beginning, we 
 * don't have to make as many allocations at the bottom.  If the top level
 * handlers are written correctly, we won't have to do much copying either.
 * Of course, for legacy handlers, we will have to condense.
 *
 * This bucket type has a pointer to the start of the allocation.  This will
 * never be modified.  This is used a a reference for the free call.  It also
 * has the length of the amount allocated.  The length could probably go
 * away.
 *
 * Finally, we have a pointer to the start and end of the string currently
 * referenced by the bucket.  The end cannot be past the original allocation
 * pointer + the allocation length.  The start cannot be before the original
 * allocation pointer.  We keep a pointer to the start and end so that we can
 * easily add and remove characters at either end.  Oh, the start cannot be
 * after the end either.
 */
struct ap_bucket_rwmem {
    void    *alloc_addr;                /* Where does the data start */
    size_t  alloc_len;                  /* how much was allocated */
    void    *start;                     /* Where does the actual data start
                                           in the alloc'ed block */
    void    *end;                       /* where does the data actually end? */
};

typedef struct ap_bucket_mmap ap_bucket_mmap;

/* 
 * The mmap bucket type.  This is basically just an allocation address and a
 * length.  This needs to be changed to a pointer to an mmap structure that
 * has a reference count in it, and a pointer to the beginning and end of
 * the data the bucket is referencing.
 */
struct ap_bucket_mmap {
    void      *alloc_addr;   /* Where does the mmap start? */
    int       len;           /* The amount of data in the mmap that we are 
                              * referencing with this bucket.  This may be 
                              * smaller than the length in the data object, 
                              * but it may not be bigger.
                              */
};

/*   ******  Bucket Brigade Functions  *****  */

/* Create a new bucket brigade.  The bucket brigade is originally empty. */
APR_EXPORT(ap_bucket_brigade *) ap_brigade_create(ap_pool_t *p);

/* destroy an enitre bucket brigade.  This includes destroying all of the
 * buckets within the bucket brigade's bucket list. */
APR_EXPORT(ap_status_t) ap_brigade_destroy(void *b);

/* append bucket(s) to a bucket_brigade.  This is the correct way to add
 * buckets to the end of a bucket briagdes bucket list.  This will accept
 * a list of buckets of any length.
 */
APR_EXPORT(void) ap_brigade_append_buckets(ap_bucket_brigade *b,
                                                  ap_bucket *e);

/* consume nbytes from beginning of b -- call ap_bucket_destroy as
    appropriate, and/or modify start on last element */
APR_EXPORT(void) ap_brigade_consume(ap_bucket_brigade *, int nbytes);

/* create an iovec of the elements in a bucket_brigade... return number 
 * of elements used.  This is useful for writing to a file or to the
 * network efficiently.
 */
APR_EXPORT(int) ap_brigade_to_iovec(ap_bucket_brigade *, 
                                           struct iovec *vec, int nvec);

/* catenate bucket_brigade b onto bucket_brigade a, bucket_brigade b is 
 * empty after this.  Neither bucket brigade can be NULL, but either one of
 * them can be emtpy when calling this function.
 */
APR_EXPORT(void) ap_brigade_catenate(ap_bucket_brigade *a, 
                                            ap_bucket_brigade *b);

/* Destroy the first nvec buckets.  This is very much like ap_brigade_consume
 * except instead of specifying the number of bytes to consume, it consumes
 * a specified number of buckets.  The original purpose for this function
 * was in ap_brigade_to_iovec.  After converting the first 16 buckets to
 * vectors, we would destroy those 16 buckets.  My gut is that this is the
 * wrong approach.  I plan to change this soon-ish.
 */
APR_EXPORT(void) ap_consume_buckets(ap_bucket_brigade *b, int nvec);

/* save the buf out to the specified iol.  This can be used to flush the
 * data to the disk, or to send it out to the network.  This is a poor 
 * function.  It never should have been implemented.  Unfortunately, it is
 * also required.  Once filters have been finished, the whole concept of
 * iol's can just go away, and this function can go away with it.  The
 * correct solution, is to have the functions that are currently calling 
 * this just call either ap_sendv or ap_writev directly.
 */
APR_EXPORT(ap_status_t) ap_brigade_to_iol(ap_ssize_t *total_bytes,
                                                 ap_bucket_brigade *a, 
                                                 ap_iol *iol);

/*
 * This function writes a bunch of strings into a bucket brigade.  How this
 * works is a bit strange.  If there is already a rwmem bucket at the end of
 * the list, we just add the next string to the end.  This requires a memcpy,
 * but it is assumed that we will have to condense buckets at the bottom of
 * the stack anyway, so we would have to do the memcpy anyway.  If there is no
 * rwmem bucket, then we just allocate a new rmem bucket for each string.
 * this avoids the memory allocation, and we hope that one of the intervening
 * filters will be removing some of the data.  This may be a dubios
 * optimization, I just don't know.
 */
APR_EXPORT(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va);

/*
 * Both of these functions evaluate the printf and put the resulting string
 * into a bucket at the end of the bucket brigade.  The only reason there are
 * two of them, is that the ap_r* functions needed both.  I would love to be
 * able to remove one, but I don't think it's feasible.
 */
APR_EXPORT(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...);
APR_EXPORT(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va);

/*   ******  Bucket Functions  *****  */

/* destroy a bucket, and remove it's memory.  This does not necessarily
 * free the actual data.  For example, an mmap may have multiple buckets
 * referenceing it (not currently implemented).  Those would only get freed
 * when the bucket with the last reference is destroyed.  Rwmem buckets
 * always have their data destroyed currently.
 */
APR_EXPORT(ap_status_t) ap_bucket_destroy(ap_bucket *e);

/* get the length of the data in the bucket that is currently being
 * referenced.  The bucket may contain more data, but if the start or end
 * has been moved, we really don't care about it.
 */
APR_EXPORT(int) ap_get_bucket_len(ap_bucket *b);

/****** Functions to Create Buckets of varying type ******/

/*
 * All of these functions are responsibly for creating a bucket and filling
 * it out with an initial value.  Some buckets can be over-written, others
 * can't.  What should happen, is that buckets that can't be over-written,
 * will have NULL write functions.  That is currently broken, although it is
 * easy to fix.  The creation routines may not allocate the space for the
 * buckets, because we may be using a free list.  Regardless, creation
 * routines are responsible for getting space for a bucket from someplace
 * and inserting the initial data.
 */

/* Create a read/write memory bucket */
APR_EXPORT(ap_bucket *) ap_bucket_rwmem_create(const void *buf,
                                ap_size_t nbyte, ap_ssize_t *w);


/* Create a mmap memory bucket */
APR_EXPORT(ap_bucket *) ap_bucket_mmap_create(const void *buf,
                                      ap_size_t nbytes, ap_ssize_t *w);

/* Create a read only memory bucket. */
APR_EXPORT(ap_bucket *) ap_bucket_rmem_create(const void *buf,
                               ap_size_t nbyte, ap_ssize_t *w);

/* Create an End of Stream bucket */
APR_EXPORT(ap_bucket *) ap_bucket_eos_create(void);

#endif

