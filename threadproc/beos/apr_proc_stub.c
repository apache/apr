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

#include <kernel/OS.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

struct pipefd {
	int in;
	int out;
	int err;
};

int main(int argc, char *argv[]) {
/* we expect the following...
 * 
 * argv[0] = this stub
 * argv[1] = directory to run in...
 * argv[2] = progname to execute
 * rest of arguments to be passed to program
 */
	char *progname = argv[2];
	char *directory = argv[1];
	struct pipefd *pfd;
	thread_id sender;
	void *buffer;
	char ** newargs;
	int i = 0;
	
	newargs = (char**)malloc(sizeof(char*) * (argc - 1));
  
	buffer = (void*)malloc(sizeof(struct pipefd));
	/* this will block until we get the data */
	receive_data(&sender, buffer, sizeof(struct pipefd));
	pfd = (struct pipefd*)buffer;
	
	if (pfd->in > STDERR_FILENO) {
		if (dup2(pfd->in, STDIN_FILENO) != STDIN_FILENO) return (-1);
	    close (pfd->in);
	}
	if (pfd->out > STDERR_FILENO) {
		if (dup2(pfd->out, STDOUT_FILENO) != STDOUT_FILENO) return (-1);
	    close (pfd->out);
	}
	if (pfd->err > STDERR_FILENO) {
		if (dup2(pfd->err, STDERR_FILENO) != STDERR_FILENO) return (-1);
	    close (pfd->err);
	}

	for	(i=3;i<=argc;i++){
	    newargs[i-3] = argv[i];
	}
	
	/* tell the caller we're OK to start */
	send_data(sender,1,NULL,0);

	if (directory != NULL)
		chdir(directory);
	execve (progname, newargs, environ);

	return (-1);
}
