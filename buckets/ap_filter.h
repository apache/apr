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

#ifndef AP_FILTER_H
#define AP_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APR_HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "httpd.h"
#include "apr.h"

/*
 * FILTER CHAIN
 *
 * Filters operate using a "chaining" mechanism. The filters are chained
 * together into a sequence. When output is generated, it is passed through
 * each of the filters on this chain, until it reaches the end (or "bottom")
 * and is placed onto the network.
 *
 * The top of the chain, the code generating the output, is typically called
 * a "content generator." The content generator's output is fed into the
 * filter chain using the standard Apache output mechanisms: ap_rputs(),
 * ap_rprintf(), ap_rwrite(), etc.
 *
 * Each filter is defined by a callback. This callback takes the output from
 * the previous filter (or the content generator if there is no previous
 * filter), operates on it, and passes the result to the next filter in the
 * chain. This pass-off is performed using the ap_fc_* functions, such as
 * ap_fc_puts(), ap_fc_printf(), ap_fc_write(), etc.
 *
 * When content generation is complete, the system will pass an "end of
 * stream" marker into the filter chain. The filters will use this to flush
 * out any internal state and to detect incomplete syntax (for example, an
 * unterminated SSI directive).
 */

/*
 * BUCKETS
 *
 * Filtering uses a "bucket" metaphor for holding content to be processed.
 * These buckets may contain arbitrary types of data. The selection of the
 * type is dependent upon how the "upstream" filter/generator places content
 * into the filter chain stream.
 *
 * For example, if a content generator uses ap_rwrite(), then the data will
 * be placed into an AP_BUCKET_PTRLEN bucket. This bucket type contains a
 * single pointer/length pair which will refer to the data.
 *
 * Buckets types are defined around the need to avoid copying the data if
 * at all possible. Whatever the "natural" input form is for a piece of
 * content, this is modelled within the bucket types. For example, when a
 * content generator uses ap_rprintf() or a filter uses ap_fc_printf(),
 * the format string and arguments are fed into/down the filter chain as
 * just theat: a format string and its arguments. The filter mechanism avoids
 * reducing the format/args to a final string for as long as possible. At
 * some point, a filter or the output of the chain will combine these to
 * produce actual bytes, but it is most optimal to defer this until it is
 * truly needed.
 *
 * See the ap_bucket_type enumeration for the different bucket types which
 * are currently defined.
 *
 * Buckets may also be linked into a list so that they may be passed as
 * entire groups of content. The filter may insert/remove/replace the buckets
 * within this list before passing the list to the next filter.
 */

/* forward declare some types */
typedef struct ap_filter_t ap_filter_t;
typedef struct ap_bucket_t ap_bucket_t;

/*
 * ap_filter_bucket_cb:
 *
 * This function type is used for filter callbacks. It will be passed a
 * pointer to "this" filter, and a "bucket" containing the content to be
 * filtered.
 *
 * In filter->ctx, the callback will find its context. This context is
 * provided here, so that a filter may be installed multiple times, each
 * receiving its own per-install context pointer.
 *
 * Callbacks are associated with a filter definition, which is specified
 * by name. See ap_register_filter() for setting the association between
 * a name for a filter and its associated callback (and other information).
 *
 * The *bucket structure (and all those referenced by ->next and ->prev)
 * should be considered "const". The filter is allowed to modify the
 * next/prev to insert/remove/replace elements in the bucket list, but
 * the types and values of the individual buckets should not be altered.
 */
typedef void (*ap_filter_bucket_cb)(ap_filter_t *filter,
                                    ap_bucket_t *bucket);

/*
 * ap_filter_type:
 *
 * Filters have different types/classifications. These are used to group
 * and sort the filters to properly sequence their operation.
 *
 * AP_FTYPE_CONTENT:
 *     These filters are used to alter the content that is passed through
 *     them. Examples are SSI or PHP.
 *
 * AP_FTYPE_CONNECTION:
 *     These filters will alter the content, but in ways that are more
 *     strongly associated with the output connection. Examples are
 *     compression, character recoding, or chunked transfer coding.
 *
 *     It is important to note that these types of filters are not allowed
 *     in a sub-request. A sub-requests output can certainly be filtered
 *     by AP_FTYPE_CONTENT filters, but all of the "final processing" is
 *     determined by the main request.
 *
 * The types have a particular sort order, which allows us to insert them
 * into the filter chain in a determistic order. Within a particular grouping,
 * the ordering is equivalent to the order of calls to ap_add_filter().
 */
typedef enum {
    AP_FTYPE_CONTENT,
    AP_FTYPE_CONNECTION
} ap_filter_type;

/*
 * ap_filter_t:
 *
 * This is the request-time context structure for an installed filter (in
 * the output filter chain). It provides the callback to use for filtering,
 * the request this filter is associated with (which is important when
 * an output chain also includes sub-request filters), the context for this
 * installed filter, and the filter ordering/chaining fields.
 *
 * Filter callbacks are free to use ->ctx as they please, to store context
 * during the filter process. Generally, this is superior over associating
 * the state directly with the request. A callback should not change any of
 * the other fields.
 */
struct ap_filter_t {
    ap_filter_bucket_cb bucket_cb;
    request_rec *r;

    void *ctx;

    ap_filter_type ftype;
    ap_filter_t *next;
};

/*
 * ap_bucket_type:
 *
 * This enumeration is used to specify what type of bucket is present when
 * an ap_bucket_t is provided.
 *
 * AP_BUCKET_PTRLEN:
 *     This bucket type defines a simple pointer/length pair for the content.
 *     The content is NOT necessarily null-terminated.
 *
 *     This type occurs when ap_rwrite(), ap_fc_write(), ap_rputs(),
 *     ap_fc_puts(), ap_rputc(), or ap_fc_putc() is used by the upstream
 *     filter/generator.
 *
 * AP_BUCKET_STRINGS:
 *     This bucket type defines a set of null-terminated strings. The actual
 *     representation is through varargs' va_list type. A filter can sequence
 *     through the arguments using the va_arg() macro (and the "const char *"
 *     type). The filter should NOT use va_start() or va_end(). When va_arg()
 *     returns a NULL pointer, the list of strings is complete.
 *
 *     Note that you can only sequence through the strings once, due to the
 *     definition of va_list. Thus, the first filter to do this sequencing
 *     must pass the resulting content to the next filter in a new form (the
 *     bucket cannot simply be passed because ->va is useless).
 *
 *     This type occurs when ap_rvputs(), ap_fc_putstrs, or ap_fc_vputstrs()
 *     is used by the upstream filter/generator.
 *
 * AP_BUCKET_PRINTF:
 *     This bucket type defines a printf-style format and arguments. Similar
 *     to AP_BUCKET_STRINGS, this type also uses the ->va field to refer to
 *     the arguments. The format for the printf is stored in ->fmt.
 *
 *     Also similar to AP_BUCKET_STRINGS, the va_start/va_end macros should
 *     not be used, and ->va should be processed only once. The bucket may
 *     not be passed after this processing.
 *
 *     This type occurs when ap_rprintf(), ap_vrprintf(), ap_fc_printf(), or
 *     ap_fc_vprintf() is used by the upstream filter/generator.
 *
 * AP_BUCKET_FILE:
 *     This bucket type refers to an open file, from the current position
 *     and extending for ->flen bytes. Since there are some ap_file_t
 *     implementations/types that are not seekable, it may be impossible to
 *     "rewind" the file's current position after reading the contenxt.
 *     Therefore, it is important to note that once the content has been
 *     read, it must be passed to the next filter in a different form.
 *
 *     Note: if there is a way to determine whether a file is seekable, then
 *     it would be legal to fetch the current position, read the contents,
 *     rewind to the original position, and then pass this bucket/file down
 *     to the next filter in the output chain.
 *
 *     This type occurs when ap_send_fd(), ap_send_fd_length(), or
 *     ap_fc_sendfile() are used by the upstream filter/generator.
 *
 * AP_BUCKET_EOS:
 *     This bucket signals the end of the content stream. The filter should
 *     flush any internal state and issue errors if the state specifies that
 *     and end of stream cannot occur now (e.g. a command directive is
 *     incomplete).
 *
 *     This type occurs when Apache finalizes a (sub)request, or when an
 *     upstream filter passes this bucket along.
 */
typedef enum {
    AP_BUCKET_PTRLEN,
    AP_BUCKET_STRINGS,
    AP_BUCKET_PRINTF,
    AP_BUCKET_FILE,
    AP_BUCKET_EOS
} ap_bucket_type;

/*
 * ap_bucket_t:
 *
 * The actual bucket definition. The type is determined by the ->type field.
 * Which fields are valid/useful in the bucket is determined by the type,
 * as noted below and in the comments above for each type.
 *
 * Buckets are arranged in a doubly-linked list so that a filter may insert,
 * remove, or replace elements in a list of buckets. Generally, a filter
 * should not change any bucket values other than these link pointers.
 */
struct ap_bucket_t {
    ap_bucket_type type;

    const char *buf;            /* AP_BUCKET_PTRLEN */
    ap_size_t len;              /* AP_BUCKET_PTRLEN */

    const char *fmt;            /* AP_BUCKET_PRINTF */
    va_list va;                 /* AP_BUCKET_STRINGS, _PRINTF */

    ap_file_t *file;            /* AP_BUCKET_FILE */
    ap_ssize_t flen;            /* AP_BUCKET_FILE */

    ap_bucket_t *next;          /* next bucket in list */
    ap_bucket_t *prev;          /* previous bucket in list */
};

/*
 * FILTER CHAIN OUTPUT FUNCTIONS
 *
 * These functions are used to deliver output/content down to the next
 * filter in the chain.
 *
 * ap_fc_write(): write a block of bytes
 * ap_fc_putc(): write a single character
 * ap_fc_puts(): write a null-terminated string
 * ap_fc_putstrs(): write a set of null-termianted strings; the end is
 *                  signaled by a NULL parameter
 * ap_fc_vputstrs(): same as ap_fc_putstrs(), but where the set of strings
 *                   is defined by a va_list
 * ap_fc_printf(): use printf-like semantics for writing a string
 * ap_fc_vprintf(): use printf-like semantics, but with a va_list for the args
 * ap_fc_sendfile(): send the file contents, from the current file position,
 *                   and extending for "len" bytes; AP_FC_SENDFILE_ALL is
 *                   used to send from current-position to the end-of-file.
 * ap_fc_putbucket(): write a bucket into the filter chain
 */
API_EXPORT(void) ap_fc_write(ap_filter_t *filter, const char *buf,
                             ap_size_t len);
API_EXPORT(void) ap_fc_putc(ap_filter_t *filter, int c);
API_EXPORT(void) ap_fc_puts(ap_filter_t *filter, const char *str);

API_EXPORT_NONSTD(void) ap_fc_putstrs(ap_filter_t *filter, ...);
API_EXPORT(void) ap_fc_vputstrs(ap_filter_t *filter, va_list va);

API_EXPORT_NONSTD(void) ap_fc_printf(ap_filter_t *filter,
                                     const char *fmt, ...);
API_EXPORT(void) ap_fc_vprintf(ap_filter_t *filter,
                               const char *fmt, va_list va);

API_EXPORT(void) ap_fc_sendfile(ap_filter_t *filter, ap_file_t *file,
                                ap_ssize_t flen);
#define AP_FC_SENDFILE_ALL ((ap_ssize_t) -1)

/* note: bucket->next and ->prev may be changed upon return from this */
API_EXPORT(void) ap_fc_putbucket(ap_filter_t *filter, ap_bucket_t *bucket);


/*
 * ap_register_filter():
 *
 * This function is used to register a filter with the system. After this
 * registration is performed, then a filter may be added into the filter
 * chain by using ap_add_filter() and simply specifying the name.
 *
 * The filter's callback and type should be passed.
 */
API_EXPORT(void) ap_register_filter(const char *name,
                                    ap_filter_bucket_cb bucket_cb,
                                    ap_filter_type ftype);

/*
 * ap_add_filter():
 *
 * Adds a named filter into the filter chain on the specified request record.
 * The filter will be installed with the specified context pointer.
 *
 * Filters added in this way will always be placed at the end of the filters
 * that have the same type (thus, the filters have the same order as the
 * calls to ap_add_filter). If the current filter chain contains filters
 * from another request, then this filter will be added before those other
 * filters.
 */
API_EXPORT(void) ap_add_filter(const char *name, void *ctx, request_rec *r);


#ifdef __cplusplus
}
#endif

#endif	/* !AP_FILTER_H */
