@echo off
@echo # As part of the pre-build process, the utility GenURI.NLM
@echo #  (Gen URI Delims) must be built, copied to a NetWare server 
@echo #  and run using the following command:
@echo #
@echo # "sys:\genuri >sys:\uri_delims.h"
@echo #
@echo #  The file "sys:\uri_delims.h" must then be copied to
@echo #  "apr-util\uri\uri_delims.h" on the build machine.

@echo Fixing up the APR headers
copy ..\include\apr.hnw ..\include\apr.h

@echo Fixing up the APR-Util headers
copy ..\..\apr-util\include\apu.hnw ..\..\apr-util\include\apu.h
copy ..\..\apr-util\include\private\apu_config.hw ..\..\apr-util\include\private\apu_config.h
copy ..\..\apr-util\xml\expat\lib\expat.h.in ..\..\apr-util\xml\expat\lib\expat.h
copy ..\..\apr-util\xml\expat\lib\config.hnw ..\..\apr-util\xml\expat\lib\config.h
copy ..\..\apr-util\include\private\apu_select_dbm.hw ..\..\apr-util\include\private\apu_select_dbm.h

@echo Fixing up the pcre headers
copy ..\..\pcre\config.hw ..\..\pcre\config.h
copy ..\..\pcre\pcre.hw ..\..\pcre\pcre.h

@echo Generating the import list...
awk -f make_nw_export.awk ..\include\*.h |sort > ..\aprlib.imp
awk -f make_nw_export.awk ..\..\apr-util\include\*.h |sort >> ..\aprlib.imp
