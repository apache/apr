
This directory contains several prototype implementations of
layered IO with filtering.  None of these will be distributed
as part of the server until we agree on a solution.

Design Rationale
----------------

   [Roy is collecting these from his mail archives]
   layered-IO-1998.txt
   bucket_brigades.txt

Bachelor #1
-----------

   apr_buf.h
   ap_buf.c
   ap_mmap_buf.c
   ap_rwmem_buf.c
   ap_rmem_buf.c
   ap_eos_buf.c
   util_filter.h
   util_filter.c
   ryan.patch

Bachelor #2
-----------

   ap_filter.h
   filters.c
   greg_patch.txt

