/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#define INCL_DOSERRORS
#include "fileio.h"
#include "apr_file_io.h"
#include <errno.h>
#include <string.h>
#include <os2.h>


int errormap[][2] = {
    { NO_ERROR,                   APR_SUCCESS      },
    { ERROR_FILE_NOT_FOUND,       APR_ENOENT       },
    { ERROR_PATH_NOT_FOUND,       APR_ENOENT       },
    { ERROR_TOO_MANY_OPEN_FILES,  APR_EMFILE       },
    { ERROR_ACCESS_DENIED,        APR_EACCES       },
    { ERROR_SHARING_VIOLATION,    APR_EACCES       },
    { ERROR_INVALID_PARAMETER,    APR_EINVAL       },
    { ERROR_OPEN_FAILED,          APR_ENOENT       },
    { ERROR_DISK_FULL,            APR_ENOSPC       },
    { ERROR_FILENAME_EXCED_RANGE, APR_ENAMETOOLONG },
    { ERROR_INVALID_FUNCTION,     APR_EINVAL       },
    { ERROR_INVALID_HANDLE,       APR_EBADF        },
    { ERROR_NEGATIVE_SEEK,        APR_ESPIPE       }
};

#define MAPSIZE (sizeof(errormap)/sizeof(errormap[0]))

int os2errno( ULONG oserror )
{
    int rv = -1, index;
    
    for (index=0; index<MAPSIZE && errormap[index][0] != oserror; index++);
    
    if (index<MAPSIZE)
        rv = errormap[index][1];
        
    return rv;
}
