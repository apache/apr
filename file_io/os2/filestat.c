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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"
#include <sys/time.h>

#define INCL_DOS
#include <os2.h>


typedef struct {
  USHORT sec2:5;
  USHORT min:6;
  USHORT hour:5;
} DOSTIME;

typedef struct {
  USHORT day:5;
  USHORT month:4;
  USHORT year:7;
} DOSDATE;

long os2date2unix( FDATE os2date, FTIME os2time )
{
  struct tm tmpdate;

  memset(&tmpdate, 0, sizeof(tmpdate));
  tmpdate.tm_hour  = os2time.hours;
  tmpdate.tm_min   = os2time.minutes;
  tmpdate.tm_sec   = os2time.twosecs * 2;

  tmpdate.tm_mday  = os2date.day;
  tmpdate.tm_mon   = os2date.month - 1;
  tmpdate.tm_year  = os2date.year + 80;
  tmpdate.tm_isdst = -1;

  return mktime( &tmpdate );
}



ap_status_t ap_getfileinfo(struct file_t *thefile)
{
    ULONG rc; 
    
    if (thefile->isopen)
        rc = DosQueryFileInfo(thefile->filedes, FIL_STANDARD, &thefile->status, sizeof(thefile->status));
    else
        rc = DosQueryPathInfo(thefile->fname, FIL_STANDARD, &thefile->status, sizeof(thefile->status));
    
    if (rc == 0) {
        thefile->validstatus = TRUE;
        return APR_SUCCESS;
    }
    
    thefile->validstatus = FALSE;
    return os2errno(rc);
}
