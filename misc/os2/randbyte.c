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

/* The high resolution timer API provides access to the hardware timer 
 * running at around 1.1MHz. The amount this changes in a time slice is
 * varies randomly due to system events, hardware interrupts etc
 */
static UCHAR randbyte_hrtimer()
{
    QWORD t1, t2;
    UCHAR byte;

    DosTmrQueryTime(&t1);
    DosSleep(5);
    DosTmrQueryTime(&t2);

    byte = (t2.ulLo - t1.ulLo) & 0xFF;
    byte ^= (t2.ulLo - t1.ulLo) >> 8;
    return byte;
}



/* A bunch of system information like memory & process stats.
 * Not highly random but every bit helps....
 */
static UCHAR randbyte_sysinfo()
{
    UCHAR byte = 0;
    UCHAR SysVars[100];
    int b;

    DosQuerySysInfo(1, QSV_FOREGROUND_PROCESS, SysVars, sizeof(SysVars));

    for (b = 0; b < 100; b++) {
        byte ^= SysVars[b];
    }

    return byte;
}



/* Similar in concept to randbyte_hrtimer() but accesses the CPU's internal
 * counters which run at the CPU's MHz speed. We get separate 
 * idle / busy / interrupt cycle counts which should provide very good 
 * randomness due to interference of hardware events.
 * This only works on newer CPUs (at least PPro or K6) and newer OS/2 versions
 * which is why it's run-time linked.
 */

static APIRET APIENTRY(*DosPerfSysCall) (ULONG ulCommand, ULONG ulParm1,
                                         ULONG ulParm2, ULONG ulParm3) = NULL;
static HMODULE hDoscalls = 0;
#define   CMD_KI_RDCNT    (0x63)

typedef struct _CPUUTIL {
    ULONG ulTimeLow;            /* Low 32 bits of time stamp      */
    ULONG ulTimeHigh;           /* High 32 bits of time stamp     */
    ULONG ulIdleLow;            /* Low 32 bits of idle time       */
    ULONG ulIdleHigh;           /* High 32 bits of idle time      */
    ULONG ulBusyLow;            /* Low 32 bits of busy time       */
    ULONG ulBusyHigh;           /* High 32 bits of busy time      */
    ULONG ulIntrLow;            /* Low 32 bits of interrupt time  */
    ULONG ulIntrHigh;           /* High 32 bits of interrupt time */
} CPUUTIL;


static UCHAR randbyte_perf()
{
    UCHAR byte = 0;
    CPUUTIL util;
    int c;

    if (hDoscalls == 0) {
        char failed_module[20];
        ULONG rc;

        rc = DosLoadModule(failed_module, sizeof(failed_module), "DOSCALLS",
                           &hDoscalls);

        if (rc == 0) {
            rc = DosQueryProcAddr(hDoscalls, 976, NULL, (PFN *)&DosPerfSysCall);

            if (rc) {
                DosPerfSysCall = NULL;
            }
        }
    }

    if (DosPerfSysCall) {
        if (DosPerfSysCall(CMD_KI_RDCNT, (ULONG)&util, 0, 0) == 0) {
            for (c = 0; c < sizeof(util); c++) {
                byte ^= ((UCHAR *)&util)[c];
            }
        }
        else {
            DosPerfSysCall = NULL;
        }
    }

    return byte;
}



static UCHAR randbyte()
{
    return randbyte_hrtimer() ^ randbyte_sysinfo() ^ randbyte_perf();
}
