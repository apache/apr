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

#define INCL_DOSERRORS
#include "apr_arch_file_io.h"
#include "apr_file_io.h"
#include <errno.h>
#include <string.h>
#include "apr_errno.h"

static int errormap[][2] = {
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
    { ERROR_NEGATIVE_SEEK,        APR_ESPIPE       },
    { ERROR_NO_SIGNAL_SENT,       ESRCH            },
    { ERROR_NO_DATA,              APR_EAGAIN       },
    { SOCEINTR,                 EINTR           },
    { SOCEWOULDBLOCK,           EWOULDBLOCK     },
    { SOCEINPROGRESS,           EINPROGRESS     },
    { SOCEALREADY,              EALREADY        },
    { SOCENOTSOCK,              ENOTSOCK        },
    { SOCEDESTADDRREQ,          EDESTADDRREQ    },
    { SOCEMSGSIZE,              EMSGSIZE        },
    { SOCEPROTOTYPE,            EPROTOTYPE      },
    { SOCENOPROTOOPT,           ENOPROTOOPT     },
    { SOCEPROTONOSUPPORT,       EPROTONOSUPPORT },
    { SOCESOCKTNOSUPPORT,       ESOCKTNOSUPPORT },
    { SOCEOPNOTSUPP,            EOPNOTSUPP      },
    { SOCEPFNOSUPPORT,          EPFNOSUPPORT    },
    { SOCEAFNOSUPPORT,          EAFNOSUPPORT    },
    { SOCEADDRINUSE,            EADDRINUSE      },
    { SOCEADDRNOTAVAIL,         EADDRNOTAVAIL   },
    { SOCENETDOWN,              ENETDOWN        },
    { SOCENETUNREACH,           ENETUNREACH     },
    { SOCENETRESET,             ENETRESET       },
    { SOCECONNABORTED,          ECONNABORTED    },
    { SOCECONNRESET,            ECONNRESET      },
    { SOCENOBUFS,               ENOBUFS         },
    { SOCEISCONN,               EISCONN         },
    { SOCENOTCONN,              ENOTCONN        },
    { SOCESHUTDOWN,             ESHUTDOWN       },
    { SOCETOOMANYREFS,          ETOOMANYREFS    },
    { SOCETIMEDOUT,             ETIMEDOUT       },
    { SOCECONNREFUSED,          ECONNREFUSED    },
    { SOCELOOP,                 ELOOP           },
    { SOCENAMETOOLONG,          ENAMETOOLONG    },
    { SOCEHOSTDOWN,             EHOSTDOWN       },
    { SOCEHOSTUNREACH,          EHOSTUNREACH    },
    { SOCENOTEMPTY,             ENOTEMPTY       },
    { SOCEPIPE,                 EPIPE           }
};

#define MAPSIZE (sizeof(errormap)/sizeof(errormap[0]))

int apr_canonical_error(apr_status_t err)
{
    int rv = -1, index;

    if (err < APR_OS_START_SYSERR)
        return err;

    err -= APR_OS_START_SYSERR;

    for (index=0; index<MAPSIZE && errormap[index][0] != err; index++);
    
    if (index<MAPSIZE)
        rv = errormap[index][1];
    else
        fprintf(stderr, "apr_canonical_error: Unknown OS/2 error code %d\n", err );
        
    return rv;
}
