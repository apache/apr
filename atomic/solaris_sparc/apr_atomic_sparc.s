!*
!*
!*  This code is based on the UltraSPARC atomics library by Mike Bennett
!*  The Initial Developer of the Original Code is Mike Bennett, 
!*  mbennett@netcom.com, Copyright (C) 1999. All Rights Reserved.
!*  This code is based on the sparc architecture Manual version 9
!*  section J.11 (page 333)
!
#include <sys/asm_linkage.h>
!     %o0  [input]   - the address of the value to increment
!     %o1  [input]   - the increment delta value
!     %o2  [local]   - work register (was %l0 in book)
!     %o3  [local]   - work register (was %l1 in book)
!     %o0  [output]  - contains return value 
!
!
!
        ENTRY(apr_atomic_add_sparc)

        ld 		[%o0], %o2 
_apr_atomic_add_sparc_loop:
        add 	%o2, %o1, %o3
        cas 	[%o0], %o2, %o3
        cmp 	%o2, %o3
        bne,a 	_apr_atomic_add_sparc_loop
        ld 		[%o0], %o2
        retl
        mov 	%o3, %o0

        SET_SIZE(apr_atomic_add_sparc)
!
!
        ENTRY(apr_atomic_sub_sparc)

        ld 		[%o0], %o2
_apr_atomic_sub_sparc_loop:
        sub 	%o2, %o1, %o3
        cas 	[%o0], %o2, %o3
        cmp 	%o2, %o3
        bne,a 	_apr_atomic_sub_sparc_loop
        ld 		[%o0], %o2
        retl
        mov 	%o3, %o0

       SET_SIZE(apr_atomic_sub_sparc)

