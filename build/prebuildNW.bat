# As part of the pre-build process, the utility GenURI.NLM
#  (Gen URI Delims) must be built, copied to a NetWare server 
#  and run using the following command:
#
# genuri >uri_delims.h
#
#  The file "sys:\uri_delims.h" must then be copied to
#  "apr-util\uri\uri_delims.h" on the build machine.

# Fix up the APR headers
copy ..\include\apr.hnw ..\include\apr.h

# Fix up the APR-Util headers
copy ..\..\apr-util\include\apu.h.in ..\..\apr-util\include\apu.h
copy ..\..\apr-util\include\private\apu_config.hw ..\..\apr-util\include\private\apu_config.h
copy ..\..\apr-util\xml\expat\lib\expat.h.in ..\..\apr-util\xml\expat\lib\expat.h
copy ..\..\apr-util\include\private\apu_select_dbm.hw ..\..\apr-util\include\private\apu_select_dbm.h

# Fix up the pcre headers
copy ..\..\pcre\config.hw ..\..\pcre\config.h
copy ..\..\pcre\pcre.hw ..\..\pcre\pcre.h

# Generate the import list
awk95 -f make_nw_export.awk ..\include\*.h |sort > ..\aprlib.imp
awk95 -f make_nw_export.awk ..\..\apr-util\include\*.h |sort > ..\aprutil.imp
