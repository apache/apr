# Microsoft Developer Studio Generated NMAKE File, Based on libapr.dsp
!IF "$(CFG)" == ""
CFG=libapr - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libapr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libapr - Win32 Release" && "$(CFG)" != "libapr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapr.mak" CFG="libapr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapr - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libapr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libapr - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr.dll"

!ELSE 

ALL : "$(OUTDIR)\libapr.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\access.obj"
	-@erase "$(INTDIR)\apr.idb"
	-@erase "$(INTDIR)\apr_cpystrn.obj"
	-@erase "$(INTDIR)\apr_fnmatch.obj"
	-@erase "$(INTDIR)\apr_getpass.obj"
	-@erase "$(INTDIR)\apr_hash.obj"
	-@erase "$(INTDIR)\apr_md5.obj"
	-@erase "$(INTDIR)\apr_pools.obj"
	-@erase "$(INTDIR)\apr_sms.obj"
	-@erase "$(INTDIR)\apr_sms_blocks.obj"
	-@erase "$(INTDIR)\apr_sms_std.obj"
	-@erase "$(INTDIR)\apr_sms_tracking.obj"
	-@erase "$(INTDIR)\apr_sms_trivial.obj"
	-@erase "$(INTDIR)\apr_snprintf.obj"
	-@erase "$(INTDIR)\apr_strings.obj"
	-@erase "$(INTDIR)\apr_strnatcmp.obj"
	-@erase "$(INTDIR)\apr_strtok.obj"
	-@erase "$(INTDIR)\apr_tables.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\dir.obj"
	-@erase "$(INTDIR)\dso.obj"
	-@erase "$(INTDIR)\errorcodes.obj"
	-@erase "$(INTDIR)\fileacc.obj"
	-@erase "$(INTDIR)\filedup.obj"
	-@erase "$(INTDIR)\filepath.obj"
	-@erase "$(INTDIR)\filestat.obj"
	-@erase "$(INTDIR)\flock.obj"
	-@erase "$(INTDIR)\fullrw.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getuuid.obj"
	-@erase "$(INTDIR)\groupinfo.obj"
	-@erase "$(INTDIR)\inet_ntop.obj"
	-@erase "$(INTDIR)\inet_pton.obj"
	-@erase "$(INTDIR)\locks.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\mmap.obj"
	-@erase "$(INTDIR)\names.obj"
	-@erase "$(INTDIR)\open.obj"
	-@erase "$(INTDIR)\otherchild.obj"
	-@erase "$(INTDIR)\pipe.obj"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\proc.obj"
	-@erase "$(INTDIR)\rand.obj"
	-@erase "$(INTDIR)\readwrite.obj"
	-@erase "$(INTDIR)\seek.obj"
	-@erase "$(INTDIR)\sendrecv.obj"
	-@erase "$(INTDIR)\signals.obj"
	-@erase "$(INTDIR)\sockaddr.obj"
	-@erase "$(INTDIR)\sockets.obj"
	-@erase "$(INTDIR)\sockopt.obj"
	-@erase "$(INTDIR)\start.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\threadpriv.obj"
	-@erase "$(INTDIR)\time.obj"
	-@erase "$(INTDIR)\timestr.obj"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\utf8_ucs2.obj"
	-@erase "$(INTDIR)\uuid.obj"
	-@erase "$(OUTDIR)\libapr.dll"
	-@erase "$(OUTDIR)\libapr.exp"
	-@erase "$(OUTDIR)\libapr.lib"
	-@erase "$(OUTDIR)\libapr.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "./include" /I "./include/arch" /I\
 "./include/arch/win32" /I "./include/arch/unix" /D "NDEBUG" /D\
 "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\apr" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo\
 /base:"0x6EE00000" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libapr.pdb" /map:"$(INTDIR)\libapr.map" /machine:I386\
 /out:"$(OUTDIR)\libapr.dll" /implib:"$(OUTDIR)\libapr.lib" /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\access.obj" \
	"$(INTDIR)\apr_cpystrn.obj" \
	"$(INTDIR)\apr_fnmatch.obj" \
	"$(INTDIR)\apr_getpass.obj" \
	"$(INTDIR)\apr_hash.obj" \
	"$(INTDIR)\apr_md5.obj" \
	"$(INTDIR)\apr_pools.obj" \
	"$(INTDIR)\apr_sms.obj" \
	"$(INTDIR)\apr_sms_blocks.obj" \
	"$(INTDIR)\apr_sms_std.obj" \
	"$(INTDIR)\apr_sms_tracking.obj" \
	"$(INTDIR)\apr_sms_trivial.obj" \
	"$(INTDIR)\apr_snprintf.obj" \
	"$(INTDIR)\apr_strings.obj" \
	"$(INTDIR)\apr_strnatcmp.obj" \
	"$(INTDIR)\apr_strtok.obj" \
	"$(INTDIR)\apr_tables.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\dir.obj" \
	"$(INTDIR)\dso.obj" \
	"$(INTDIR)\errorcodes.obj" \
	"$(INTDIR)\fileacc.obj" \
	"$(INTDIR)\filedup.obj" \
	"$(INTDIR)\filepath.obj" \
	"$(INTDIR)\filestat.obj" \
	"$(INTDIR)\flock.obj" \
	"$(INTDIR)\fullrw.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getuuid.obj" \
	"$(INTDIR)\groupinfo.obj" \
	"$(INTDIR)\inet_ntop.obj" \
	"$(INTDIR)\inet_pton.obj" \
	"$(INTDIR)\locks.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\mmap.obj" \
	"$(INTDIR)\names.obj" \
	"$(INTDIR)\open.obj" \
	"$(INTDIR)\otherchild.obj" \
	"$(INTDIR)\pipe.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\proc.obj" \
	"$(INTDIR)\rand.obj" \
	"$(INTDIR)\readwrite.obj" \
	"$(INTDIR)\seek.obj" \
	"$(INTDIR)\sendrecv.obj" \
	"$(INTDIR)\signals.obj" \
	"$(INTDIR)\sockaddr.obj" \
	"$(INTDIR)\sockets.obj" \
	"$(INTDIR)\sockopt.obj" \
	"$(INTDIR)\start.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\threadpriv.obj" \
	"$(INTDIR)\time.obj" \
	"$(INTDIR)\timestr.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\utf8_ucs2.obj" \
	"$(INTDIR)\uuid.obj"

"$(OUTDIR)\libapr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr.dll"

!ELSE 

ALL : "$(OUTDIR)\libapr.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\access.obj"
	-@erase "$(INTDIR)\apr.idb"
	-@erase "$(INTDIR)\apr.pdb"
	-@erase "$(INTDIR)\apr_cpystrn.obj"
	-@erase "$(INTDIR)\apr_fnmatch.obj"
	-@erase "$(INTDIR)\apr_getpass.obj"
	-@erase "$(INTDIR)\apr_hash.obj"
	-@erase "$(INTDIR)\apr_md5.obj"
	-@erase "$(INTDIR)\apr_pools.obj"
	-@erase "$(INTDIR)\apr_sms.obj"
	-@erase "$(INTDIR)\apr_sms_blocks.obj"
	-@erase "$(INTDIR)\apr_sms_std.obj"
	-@erase "$(INTDIR)\apr_sms_tracking.obj"
	-@erase "$(INTDIR)\apr_sms_trivial.obj"
	-@erase "$(INTDIR)\apr_snprintf.obj"
	-@erase "$(INTDIR)\apr_strings.obj"
	-@erase "$(INTDIR)\apr_strnatcmp.obj"
	-@erase "$(INTDIR)\apr_strtok.obj"
	-@erase "$(INTDIR)\apr_tables.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\dir.obj"
	-@erase "$(INTDIR)\dso.obj"
	-@erase "$(INTDIR)\errorcodes.obj"
	-@erase "$(INTDIR)\fileacc.obj"
	-@erase "$(INTDIR)\filedup.obj"
	-@erase "$(INTDIR)\filepath.obj"
	-@erase "$(INTDIR)\filestat.obj"
	-@erase "$(INTDIR)\flock.obj"
	-@erase "$(INTDIR)\fullrw.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getuuid.obj"
	-@erase "$(INTDIR)\groupinfo.obj"
	-@erase "$(INTDIR)\inet_ntop.obj"
	-@erase "$(INTDIR)\inet_pton.obj"
	-@erase "$(INTDIR)\locks.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\mmap.obj"
	-@erase "$(INTDIR)\names.obj"
	-@erase "$(INTDIR)\open.obj"
	-@erase "$(INTDIR)\otherchild.obj"
	-@erase "$(INTDIR)\pipe.obj"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\proc.obj"
	-@erase "$(INTDIR)\rand.obj"
	-@erase "$(INTDIR)\readwrite.obj"
	-@erase "$(INTDIR)\seek.obj"
	-@erase "$(INTDIR)\sendrecv.obj"
	-@erase "$(INTDIR)\signals.obj"
	-@erase "$(INTDIR)\sockaddr.obj"
	-@erase "$(INTDIR)\sockets.obj"
	-@erase "$(INTDIR)\sockopt.obj"
	-@erase "$(INTDIR)\start.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\threadpriv.obj"
	-@erase "$(INTDIR)\time.obj"
	-@erase "$(INTDIR)\timestr.obj"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\utf8_ucs2.obj"
	-@erase "$(INTDIR)\uuid.obj"
	-@erase "$(OUTDIR)\libapr.dll"
	-@erase "$(OUTDIR)\libapr.exp"
	-@erase "$(OUTDIR)\libapr.lib"
	-@erase "$(OUTDIR)\libapr.map"
	-@erase "$(OUTDIR)\libapr.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /I "./include" /I "./include/arch" /I\
 "./include/arch/win32" /I "./include/arch/unix" /D "_DEBUG" /D\
 "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\apr" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo\
 /base:"0x6EE00000" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libapr.pdb" /map:"$(INTDIR)\libapr.map" /debug /machine:I386\
 /out:"$(OUTDIR)\libapr.dll" /implib:"$(OUTDIR)\libapr.lib" /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\access.obj" \
	"$(INTDIR)\apr_cpystrn.obj" \
	"$(INTDIR)\apr_fnmatch.obj" \
	"$(INTDIR)\apr_getpass.obj" \
	"$(INTDIR)\apr_hash.obj" \
	"$(INTDIR)\apr_md5.obj" \
	"$(INTDIR)\apr_pools.obj" \
	"$(INTDIR)\apr_sms.obj" \
	"$(INTDIR)\apr_sms_blocks.obj" \
	"$(INTDIR)\apr_sms_std.obj" \
	"$(INTDIR)\apr_sms_tracking.obj" \
	"$(INTDIR)\apr_sms_trivial.obj" \
	"$(INTDIR)\apr_snprintf.obj" \
	"$(INTDIR)\apr_strings.obj" \
	"$(INTDIR)\apr_strnatcmp.obj" \
	"$(INTDIR)\apr_strtok.obj" \
	"$(INTDIR)\apr_tables.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\dir.obj" \
	"$(INTDIR)\dso.obj" \
	"$(INTDIR)\errorcodes.obj" \
	"$(INTDIR)\fileacc.obj" \
	"$(INTDIR)\filedup.obj" \
	"$(INTDIR)\filepath.obj" \
	"$(INTDIR)\filestat.obj" \
	"$(INTDIR)\flock.obj" \
	"$(INTDIR)\fullrw.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getuuid.obj" \
	"$(INTDIR)\groupinfo.obj" \
	"$(INTDIR)\inet_ntop.obj" \
	"$(INTDIR)\inet_pton.obj" \
	"$(INTDIR)\locks.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\mmap.obj" \
	"$(INTDIR)\names.obj" \
	"$(INTDIR)\open.obj" \
	"$(INTDIR)\otherchild.obj" \
	"$(INTDIR)\pipe.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\proc.obj" \
	"$(INTDIR)\rand.obj" \
	"$(INTDIR)\readwrite.obj" \
	"$(INTDIR)\seek.obj" \
	"$(INTDIR)\sendrecv.obj" \
	"$(INTDIR)\signals.obj" \
	"$(INTDIR)\sockaddr.obj" \
	"$(INTDIR)\sockets.obj" \
	"$(INTDIR)\sockopt.obj" \
	"$(INTDIR)\start.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\threadpriv.obj" \
	"$(INTDIR)\time.obj" \
	"$(INTDIR)\timestr.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\utf8_ucs2.obj" \
	"$(INTDIR)\uuid.obj"

"$(OUTDIR)\libapr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "libapr - Win32 Release" || "$(CFG)" == "libapr - Win32 Debug"
SOURCE=.\time\win32\access.c
DEP_CPP_ACCES=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_time.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\atime.h"\
	

"$(INTDIR)\access.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\time\win32\time.c
DEP_CPP_TIME_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\atime.h"\
	

"$(INTDIR)\time.obj" : $(SOURCE) $(DEP_CPP_TIME_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\time\win32\timestr.c
DEP_CPP_TIMES=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\atime.h"\
	

"$(INTDIR)\timestr.obj" : $(SOURCE) $(DEP_CPP_TIMES) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\strings\apr_cpystrn.c
DEP_CPP_APR_C=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_cpystrn.obj" : $(SOURCE) $(DEP_CPP_APR_C) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\strings\apr_fnmatch.c
DEP_CPP_APR_F=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_fnmatch.h"\
	".\include\apr_lib.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_fnmatch.obj" : $(SOURCE) $(DEP_CPP_APR_F) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\strings\apr_snprintf.c
DEP_CPP_APR_S=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_snprintf.obj" : $(SOURCE) $(DEP_CPP_APR_S) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\strings\apr_strings.c
DEP_CPP_APR_ST=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_strings.obj" : $(SOURCE) $(DEP_CPP_APR_ST) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\strings\apr_strnatcmp.c
DEP_CPP_APR_STR=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	

"$(INTDIR)\apr_strnatcmp.obj" : $(SOURCE) $(DEP_CPP_APR_STR) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\strings\apr_strtok.c
DEP_CPP_APR_STRT=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_want.h"\
	

"$(INTDIR)\apr_strtok.obj" : $(SOURCE) $(DEP_CPP_APR_STRT) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\passwd\apr_getpass.c
DEP_CPP_APR_G=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_getpass.obj" : $(SOURCE) $(DEP_CPP_APR_G) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\passwd\apr_md5.c
DEP_CPP_APR_M=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_lib.h"\
	".\include\apr_md5.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_xlate.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_md5.obj" : $(SOURCE) $(DEP_CPP_APR_M) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\tables\apr_hash.c
DEP_CPP_APR_H=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_general.h"\
	".\include\apr_hash.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_hash.obj" : $(SOURCE) $(DEP_CPP_APR_H) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\tables\apr_tables.c
DEP_CPP_APR_T=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_tables.obj" : $(SOURCE) $(DEP_CPP_APR_T) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\unix\errorcodes.c
DEP_CPP_ERROR=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\errorcodes.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\unix\getopt.c
DEP_CPP_GETOP=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\getopt.obj" : $(SOURCE) $(DEP_CPP_GETOP) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\win32\getuuid.c
DEP_CPP_GETUU=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_uuid.h"\
	

"$(INTDIR)\getuuid.obj" : $(SOURCE) $(DEP_CPP_GETUU) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\win32\misc.c
DEP_CPP_MISC_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\win32\names.c
DEP_CPP_NAMES=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\names.obj" : $(SOURCE) $(DEP_CPP_NAMES) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\unix\otherchild.c
DEP_CPP_OTHER=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	".\include\arch\win32\threadproc.h"\
	

"$(INTDIR)\otherchild.obj" : $(SOURCE) $(DEP_CPP_OTHER) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\win32\rand.c
DEP_CPP_RAND_=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_general.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\rand.obj" : $(SOURCE) $(DEP_CPP_RAND_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\unix\start.c
DEP_CPP_START=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_signal.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\locks.h"\
	

"$(INTDIR)\start.obj" : $(SOURCE) $(DEP_CPP_START) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc\unix\uuid.c
DEP_CPP_UUID_=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_lib.h"\
	".\include\apr_uuid.h"\
	

"$(INTDIR)\uuid.obj" : $(SOURCE) $(DEP_CPP_UUID_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\dir.c
DEP_CPP_DIR_C=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\atime.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\dir.obj" : $(SOURCE) $(DEP_CPP_DIR_C) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\unix\fileacc.c
DEP_CPP_FILEA=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\fileacc.obj" : $(SOURCE) $(DEP_CPP_FILEA) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\filedup.c
DEP_CPP_FILED=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\inherit.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\filedup.obj" : $(SOURCE) $(DEP_CPP_FILED) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\filepath.c
DEP_CPP_FILEP=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\filepath.obj" : $(SOURCE) $(DEP_CPP_FILEP) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\filestat.c
DEP_CPP_FILES=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\atime.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\filestat.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\flock.c
DEP_CPP_FLOCK=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\flock.obj" : $(SOURCE) $(DEP_CPP_FLOCK) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\unix\fullrw.c
DEP_CPP_FULLR=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_inherit.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	

"$(INTDIR)\fullrw.obj" : $(SOURCE) $(DEP_CPP_FULLR) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\open.c
DEP_CPP_OPEN_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\open.obj" : $(SOURCE) $(DEP_CPP_OPEN_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\pipe.c
DEP_CPP_PIPE_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\pipe.obj" : $(SOURCE) $(DEP_CPP_PIPE_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\readwrite.c
DEP_CPP_READW=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\atime.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\readwrite.obj" : $(SOURCE) $(DEP_CPP_READW) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\file_io\win32\seek.c
DEP_CPP_SEEK_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\seek.obj" : $(SOURCE) $(DEP_CPP_SEEK_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\locks\win32\locks.c
DEP_CPP_LOCKS=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\locks.h"\
	

"$(INTDIR)\locks.obj" : $(SOURCE) $(DEP_CPP_LOCKS) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\unix\inet_ntop.c
DEP_CPP_INET_=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\networkio.h"\
	

"$(INTDIR)\inet_ntop.obj" : $(SOURCE) $(DEP_CPP_INET_) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\unix\inet_pton.c
DEP_CPP_INET_P=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\networkio.h"\
	

"$(INTDIR)\inet_pton.obj" : $(SOURCE) $(DEP_CPP_INET_P) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\win32\poll.c
DEP_CPP_POLL_=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\networkio.h"\
	

"$(INTDIR)\poll.obj" : $(SOURCE) $(DEP_CPP_POLL_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\unix\sa_common.c
SOURCE=.\network_io\win32\sendrecv.c
DEP_CPP_SENDR=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	".\include\arch\win32\networkio.h"\
	

"$(INTDIR)\sendrecv.obj" : $(SOURCE) $(DEP_CPP_SENDR) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\win32\sockaddr.c
DEP_CPP_SOCKA=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\networkio.h"\
	".\network_io\unix\sa_common.c"\
	

"$(INTDIR)\sockaddr.obj" : $(SOURCE) $(DEP_CPP_SOCKA) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\win32\sockets.c
DEP_CPP_SOCKE=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\networkio.h"\
	

"$(INTDIR)\sockets.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\network_io\win32\sockopt.c
DEP_CPP_SOCKO=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\networkio.h"\
	

"$(INTDIR)\sockopt.obj" : $(SOURCE) $(DEP_CPP_SOCKO) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\threadproc\win32\proc.c
DEP_CPP_PROC_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	".\include\arch\win32\threadproc.h"\
	

"$(INTDIR)\proc.obj" : $(SOURCE) $(DEP_CPP_PROC_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\threadproc\win32\signals.c
DEP_CPP_SIGNA=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	".\include\arch\win32\threadproc.h"\
	

"$(INTDIR)\signals.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\threadproc\win32\thread.c
DEP_CPP_THREA=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\threadproc.h"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\threadproc\win32\threadpriv.c
DEP_CPP_THREAD=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\threadproc.h"\
	

"$(INTDIR)\threadpriv.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dso\win32\dso.c
DEP_CPP_DSO_C=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\dso.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\dso.obj" : $(SOURCE) $(DEP_CPP_DSO_C) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\i18n\unix\utf8_ucs2.c
DEP_CPP_UTF8_=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	

"$(INTDIR)\utf8_ucs2.obj" : $(SOURCE) $(DEP_CPP_UTF8_) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\i18n\unix\xlate.c
SOURCE=.\shmem\win32\shmem.c
SOURCE=.\mmap\unix\common.c
DEP_CPP_COMMO=\
	".\include\apr.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_inherit.h"\
	".\include\apr_mmap.h"\
	".\include\apr_pools.h"\
	".\include\apr_sms.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\common.obj" : $(SOURCE) $(DEP_CPP_COMMO) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mmap\win32\mmap.c
DEP_CPP_MMAP_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_mmap.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\mmap.obj" : $(SOURCE) $(DEP_CPP_MMAP_) "$(INTDIR)" ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\user\win32\groupinfo.c
DEP_CPP_GROUP=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\groupinfo.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\user\win32\userinfo.c
DEP_CPP_USERI=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_tables.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\apr_xlate.h"\
	".\include\arch\unix\i18n.h"\
	".\include\arch\unix\misc.h"\
	".\include\arch\win32\apr_private.h"\
	".\include\arch\win32\fileio.h"\
	

"$(INTDIR)\userinfo.obj" : $(SOURCE) $(DEP_CPP_USERI) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\memory\unix\apr_pools.c
DEP_CPP_APR_P=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_hash.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	

"$(INTDIR)\apr_pools.obj" : $(SOURCE) $(DEP_CPP_APR_P) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\memory\unix\apr_sms.c
DEP_CPP_APR_SM=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_hash.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_strings.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\memory\unix\sms_private.h"\
	

"$(INTDIR)\apr_sms.obj" : $(SOURCE) $(DEP_CPP_APR_SM) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\memory\unix\apr_sms_blocks.c
DEP_CPP_APR_SMS=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_sms_blocks.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\memory\unix\sms_private.h"\
	

"$(INTDIR)\apr_sms_blocks.obj" : $(SOURCE) $(DEP_CPP_APR_SMS) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\memory\unix\apr_sms_std.c
DEP_CPP_APR_SMS_=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\memory\unix\sms_private.h"\
	

"$(INTDIR)\apr_sms_std.obj" : $(SOURCE) $(DEP_CPP_APR_SMS_) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\memory\unix\apr_sms_tracking.c
DEP_CPP_APR_SMS_T=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_sms_tracking.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\memory\unix\sms_private.h"\
	

"$(INTDIR)\apr_sms_tracking.obj" : $(SOURCE) $(DEP_CPP_APR_SMS_T) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\memory\unix\apr_sms_trivial.c
DEP_CPP_APR_SMS_TR=\
	".\include\apr.h"\
	".\include\apr_dso.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_info.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_inherit.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_sms.h"\
	".\include\apr_sms_trivial.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_user.h"\
	".\include\apr_want.h"\
	".\include\arch\win32\apr_private.h"\
	".\memory\unix\sms_private.h"\
	

"$(INTDIR)\apr_sms_trivial.obj" : $(SOURCE) $(DEP_CPP_APR_SMS_TR) "$(INTDIR)"\
 ".\include\apr.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\include\apr.hw

!IF  "$(CFG)" == "libapr - Win32 Release"

InputPath=.\include\apr.hw

".\include\apr.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apr.hw .\include\apr.h > nul 
	echo Created apr.h from apr.hw 
	

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

InputPath=.\include\apr.hw

".\include\apr.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apr.hw .\include\apr.h > nul 
	echo Created apr.h from apr.hw 
	

!ENDIF 


!ENDIF 

