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

#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_portable.h"
#include "apr_lib.h"
#include <sys/time.h>
#include <stdlib.h>
#define INCL_DOS
#include <os2.h>

/*  OS/2 doesn't have a poll function, implement using OS/2 style select */
 
ap_status_t ap_setup_poll(struct pollfd_t **new, ap_int32_t num, ap_context_t *cont)
{
    *new = (struct pollfd_t *)ap_palloc(cont, sizeof(struct pollfd_t));

    if (*new == NULL) {
        return APR_ENOMEM;
    }

    (*new)->socket_list = ap_palloc(cont, sizeof(int) * num);
    
    if ((*new)->socket_list == NULL) {
        return APR_ENOMEM;
    }
    
    (*new)->r_socket_list = ap_palloc(cont, sizeof(int) * num);
    
    if ((*new)->r_socket_list == NULL) {
        return APR_ENOMEM;
    }
    
    (*new)->cntxt = cont;
    (*new)->num_total = 0;
    (*new)->num_read = 0;
    (*new)->num_write = 0;
    (*new)->num_except = 0;
    
    return APR_SUCCESS;
}



ap_status_t ap_add_poll_socket(struct pollfd_t *aprset, 
			       struct socket_t *sock, ap_int16_t events)
{
    int i;
    
    if (events & APR_POLLIN) {
        for (i=aprset->num_total; i>aprset->num_read; i--)
            aprset->socket_list[i] = aprset->socket_list[i-1];
        aprset->socket_list[i] = sock->socketdes;
        aprset->num_read++;
        aprset->num_total++;
    }
            
    if (events & APR_POLLOUT) {
        for (i=aprset->num_total; i>aprset->num_read + aprset->num_write; i--)
            aprset->socket_list[i] = aprset->socket_list[i-1];
        aprset->socket_list[i] = sock->socketdes;
        aprset->num_write++;
        aprset->num_total++;
    }            
        
    if (events &APR_POLLPRI) {
        aprset->socket_list[aprset->num_total] = sock->socketdes;
        aprset->num_except++;
        aprset->num_total++;
    }
    return APR_SUCCESS;
}



ap_status_t ap_poll(struct pollfd_t *pollfdset, ap_int32_t *nsds, ap_int32_t timeout)
{
    int i;
    int rv = 0;
    time_t starttime;
    struct timeval tv;
    
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    time(&starttime);

    do {
        for (i=0; i<pollfdset->num_total; i++) {
            pollfdset->r_socket_list[i] = pollfdset->socket_list[i];
        }
        
        rv = select(pollfdset->r_socket_list, 
                    pollfdset->num_read, 
                    pollfdset->num_write, 
                    pollfdset->num_except, 
                    timeout >= 0 ? timeout * 1000 : -1);

        if (rv < 0 && sock_errno() == SOCEINTR && timeout >= 0 ) {
            time_t elapsed = time(NULL) - starttime;

            if (timeout <= elapsed)
                break;

            timeout -= elapsed;
        }
    } while ( rv < 0 && sock_errno() == SOCEINTR );

    (*nsds) = rv;
    return rv < 0 ? os2errno(sock_errno()) : APR_SUCCESS;
}



ap_status_t ap_get_revents(ap_int16_t *event, struct socket_t *sock, struct pollfd_t *aprset)
{
    int i;
    
    *event = 0;
    
    for (i=0; i < aprset->num_total; i++) {
        if (aprset->socket_list[i] == sock->socketdes && aprset->r_socket_list[i] > 0) {
            if (i < aprset->num_read)
                *event |= APR_POLLIN;
            else if (i < aprset->num_read + aprset->num_write)
                *event |= APR_POLLOUT;
            else
                *event |= APR_POLLPRI;
        }
    }

    return APR_SUCCESS;
}



ap_status_t ap_remove_poll_socket(struct pollfd_t *aprset, 
                                  struct socket_t *sock, ap_int16_t events)
{
    int start, *count, pos;

    while (events) {
        if (events & APR_POLLIN) {
            start = 0;
            count = &aprset->num_read;
            events -= APR_POLLIN;
        } else if (events & APR_POLLOUT) {
            start = aprset->num_read;
            count = &aprset->num_write;
            events -= APR_POLLOUT;
        } else if (events & APR_POLLPRI) {
            start = aprset->num_read + aprset->num_write;
            count = &aprset->num_except;
            events -= APR_POLLPRI;
        } else
            break;

        for (pos=start; pos < start+(*count) && aprset->socket_list[pos] != sock->socketdes; pos++);

        if (pos < start+(*count)) {
            aprset->num_total--;
            (*count)--;

            for (;pos<aprset->num_total; pos++) {
                aprset->socket_list[pos] = aprset->socket_list[pos+1];
            }
        } else {
            return APR_NOTFOUND;
        }
    }

    return APR_SUCCESS;
}
