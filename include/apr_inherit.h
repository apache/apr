/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#ifndef APR_INHERIT_H
#define APR_INHERIT_H

/**
 * @file apr_inherit.h 
 * @brief APR File Handle Inheritance Helpers
 * @remark This internal header includes internal declaration helpers 
 * for other headers to declare apr_foo_inherit_[un]set functions.
 */

/**
 * Prototype for type-specific declarations of apr_foo_inherit_set 
 * functions.  
 * @remark Doxygen unwraps this macro (via doxygen.conf) to provide 
 * actual help for each specific occurance of apr_foo_inherit_set.
 * @remark the linkage is specified for APR. It would be possible to expand
 *       the macros to support other linkages.
 */
#define APR_DECLARE_INHERIT_SET(type) \
    APR_DECLARE(apr_status_t) apr_##type##_inherit_set( \
                                          apr_##type##_t *the##type)

/**
 * Prototype for type-specific declarations of apr_foo_inherit_unset 
 * functions.  
 * @remark Doxygen unwraps this macro (via doxygen.conf) to provide 
 * actual help for each specific occurance of apr_foo_inherit_unset.
 * @remark the linkage is specified for APR. It would be possible to expand
 *       the macros to support other linkages.
 */
#define APR_DECLARE_INHERIT_UNSET(type) \
    APR_DECLARE(apr_status_t) apr_##type##_inherit_unset( \
                                          apr_##type##_t *the##type)

#endif	/* ! APR_INHERIT_H */
