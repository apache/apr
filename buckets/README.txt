
This directory contains several prototype implementations of
layered IO with filtering.  None of these will be distributed
as part of the server until we agree on a solution.

Design Rationale
----------------

   doc_SFmtg.txt
      -- notes from the 1998 design meeting in SF

   doc_stacked_io.txt
      -- Ed and Alexei's vision of stacked-io with layer caching

   doc_page_io.txt
      -- Dean's comments on performance considerations

   doc_dean_iol.txt
      -- Rationale behind the IOL stuff that is now in APR

   doc_bucket_brigades.txt
      -- Roy's ramblings about the bucket brigades design

   doc_wishes.txt
      -- Everyone's requirements for layered-IO and filters

   doc_greg_filters.txt
      -- Greg's initial filter design rationale

Bachelor #1
-----------

   apr_buf.h
   ap_buf.c
   ap_mmap_buf.c
   ap_rwmem_buf.c
   ap_rmem_buf.c
   ap_eos_buf.c
   ryan.patch

Bachelor #2
-----------

   ap_filter.h
   filters.c
   greg_patch.txt

Bachelor #3  --  The combination of #1 and #2 (hopefully)
-----------

   util_filter.h
   util_filter.c

