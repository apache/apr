!* ====================================================================
!* The Apache Software License, Version 1.1
!*
!* Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
!* reserved.
!*
!* Redistribution and use in source and binary forms, with or without
!* modification, are permitted provided that the following conditions
!* are met:
!*
!* 1. Redistributions of source code must retain the above copyright
!*    notice, this list of conditions and the following disclaimer.
!*
!* 2. Redistributions in binary form must reproduce the above copyright
!*    notice, this list of conditions and the following disclaimer in
!*    the documentation and/or other materials provided with the
!*    distribution.
!*
!* 3. The end-user documentation included with the redistribution,
!*    if any, must include the following acknowledgment:
!*       "This product includes software developed by the
!*        Apache Software Foundation (http://www.apache.org/)."
!*    Alternately, this acknowledgment may appear in the software itself,
!*    if and wherever such third-party acknowledgments normally appear.
!*
!* 4. The names "Apache" and "Apache Software Foundation" must
!*    not be used to endorse or promote products derived from this
!*    software without prior written permission. For written
!*    permission, please contact apache@apache.org.
!*
!* 5. Products derived from this software may not be called "Apache",
!*    nor may "Apache" appear in their name, without prior written
!*    permission of the Apache Software Foundation.
!*
!* THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
!* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
!* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
!* DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
!* ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
!* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
!* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
!* USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
!* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
!* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
!* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
!* SUCH DAMAGE.
!* ====================================================================
!*
!* This software consists of voluntary contributions made by many
!* individuals on behalf of the Apache Software Foundation.  For more
!* information on the Apache Software Foundation, please see
!* <http://www.apache.org/>.
!*

!*
!*
!*  This code is based on the UltraSPARC atomics library by Mike Bennett
!*
!*  The contents of this file are subject to the Mozilla Public License
!*  Version 1.0 (the "License"); you may not use this file except in
!*  compliance with the License. You may obtain a copy of the License at
!*  http://www.mozilla.org/MPL/
!*
!*  Software distributed under the License is distributed on an "AS IS"
!*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
!*  License for the specific language governing rights and limitations
!*  under the License.
!*
!*  The Original Code is UltraSPARC atomics library.
!*  The Initial Developer of the Original Code is Mike Bennett,
!*  mbennett@netcom.com, Copyright (C) 1999. All Rights Reserved.
!*
!*  This code is based on the sparc architecture Manual version 9
!*  section J.11 (page 333)
!
#include <sys/asm_linkage.h>
!     %o0  [input]   - the address of the value to increment
!     %o1  [input]   - the increment delta value
!     %o2  [local]   - work register (was %l0 in book)
!     %o3  [local]   - work register (was %l1 in book)
!     %o4  [local]   - work register
!     %o0  [output]  - contains return value 
!
!
!
        ENTRY(apr_atomic_add_sparc)

        ld 		[%o0], %o2                   ! set o2 to current value
_apr_atomic_add_sparc_loop:
        add 	%o2, %o1, %o3                ! o3 = o2 + o1 
        cas 	[%o0], %o2, %o3              ! if cur-val==o2 then cur-val=03
        cmp 	%o2, %o3                     ! see if the CAS worked
        bne,a 	_apr_atomic_add_sparc_loop   ! if not try again
        ld 		[%o0], %o2                   ! return the previous value
        retl
        mov 	%o3, %o0

        SET_SIZE(apr_atomic_add_sparc)
!
!
! 
        ENTRY(apr_atomic_sub_sparc)

        ld 		[%o0], %o2
_apr_atomic_sub_sparc_loop:
        sub 	%o2, %o1, %o3
        mov	%o3, %o4
        cas 	[%o0], %o2, %o3
        cmp 	%o2, %o3
        bne,a 	_apr_atomic_sub_sparc_loop
        nop
        retl
        mov 	%o4, %o0

        SET_SIZE(apr_atomic_sub_sparc)
!
!
!       
!     %o0  [input]   - the address of the value to compare
!     %o1  [input]   - the new value
!     %o2  [input]   - value to compare against
!     %o0  [output]  - the return value
!
        ENTRY(apr_atomic_cas_sparc)
        ENTRY(apr_atomic_casptr_sparc)

        cas 	[%o0], %o2, %o1
        retl
        mov      %o1, %o0
        
        SET_SIZE(apr_atomic_cas_sparc)

