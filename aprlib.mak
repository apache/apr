# Microsoft Developer Studio Generated NMAKE File, Based on aprlib.dsp
!IF "$(CFG)" == ""
CFG=aprlib - Win32 Debug
!MESSAGE No configuration specified. Defaulting to aprlib - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "aprlib - Win32 Release" && "$(CFG)" != "aprlib - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aprlib.mak" CFG="aprlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aprlib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aprlib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aprlib - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\aprlib.dll"

!ELSE 

ALL : "$(OUTDIR)\aprlib.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\access.obj"
	-@erase "$(INTDIR)\apr_cpystrn.obj"
	-@erase "$(INTDIR)\apr_execve.obj"
	-@erase "$(INTDIR)\apr_fnmatch.obj"
	-@erase "$(INTDIR)\apr_getpass.obj"
	-@erase "$(INTDIR)\apr_md5.obj"
	-@erase "$(INTDIR)\apr_pools.obj"
	-@erase "$(INTDIR)\apr_slack.obj"
	-@erase "$(INTDIR)\apr_snprintf.obj"
	-@erase "$(INTDIR)\apr_tables.obj"
	-@erase "$(INTDIR)\dir.obj"
	-@erase "$(INTDIR)\fileacc.obj"
	-@erase "$(INTDIR)\filedup.obj"
	-@erase "$(INTDIR)\filestat.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\locks.obj"
	-@erase "$(INTDIR)\names.obj"
	-@erase "$(INTDIR)\open.obj"
	-@erase "$(INTDIR)\pipe.obj"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\proc.obj"
	-@erase "$(INTDIR)\readdir.obj"
	-@erase "$(INTDIR)\readwrite.obj"
	-@erase "$(INTDIR)\seek.obj"
	-@erase "$(INTDIR)\sendrecv.obj"
	-@erase "$(INTDIR)\signals.obj"
	-@erase "$(INTDIR)\sockaddr.obj"
	-@erase "$(INTDIR)\sockets.obj"
	-@erase "$(INTDIR)\sockopt.obj"
	-@erase "$(INTDIR)\start.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\threadcancel.obj"
	-@erase "$(INTDIR)\threadpriv.obj"
	-@erase "$(INTDIR)\time.obj"
	-@erase "$(INTDIR)\timestr.obj"
	-@erase "$(INTDIR)\timetest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\aprlib.dll"
	-@erase "$(OUTDIR)\aprlib.exp"
	-@erase "$(OUTDIR)\aprlib.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "./include" /I "./inc" /I "./misc/win32" /I\
 "./file_io/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\aprlib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aprlib.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib ws2_32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\aprlib.pdb" /machine:I386 /def:".\aprlib.def"\
 /out:"$(OUTDIR)\aprlib.dll" /implib:"$(OUTDIR)\aprlib.lib" 
DEF_FILE= \
	".\aprlib.def"
LINK32_OBJS= \
	"$(INTDIR)\access.obj" \
	"$(INTDIR)\apr_cpystrn.obj" \
	"$(INTDIR)\apr_execve.obj" \
	"$(INTDIR)\apr_fnmatch.obj" \
	"$(INTDIR)\apr_getpass.obj" \
	"$(INTDIR)\apr_md5.obj" \
	"$(INTDIR)\apr_pools.obj" \
	"$(INTDIR)\apr_slack.obj" \
	"$(INTDIR)\apr_snprintf.obj" \
	"$(INTDIR)\apr_tables.obj" \
	"$(INTDIR)\dir.obj" \
	"$(INTDIR)\fileacc.obj" \
	"$(INTDIR)\filedup.obj" \
	"$(INTDIR)\filestat.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\locks.obj" \
	"$(INTDIR)\names.obj" \
	"$(INTDIR)\open.obj" \
	"$(INTDIR)\pipe.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\proc.obj" \
	"$(INTDIR)\readdir.obj" \
	"$(INTDIR)\readwrite.obj" \
	"$(INTDIR)\seek.obj" \
	"$(INTDIR)\sendrecv.obj" \
	"$(INTDIR)\signals.obj" \
	"$(INTDIR)\sockaddr.obj" \
	"$(INTDIR)\sockets.obj" \
	"$(INTDIR)\sockopt.obj" \
	"$(INTDIR)\start.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\threadcancel.obj" \
	"$(INTDIR)\threadpriv.obj" \
	"$(INTDIR)\time.obj" \
	"$(INTDIR)\timestr.obj" \
	"$(INTDIR)\timetest.obj"

"$(OUTDIR)\aprlib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\aprlib.dll"

!ELSE 

ALL : "$(OUTDIR)\aprlib.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\access.obj"
	-@erase "$(INTDIR)\apr_cpystrn.obj"
	-@erase "$(INTDIR)\apr_execve.obj"
	-@erase "$(INTDIR)\apr_fnmatch.obj"
	-@erase "$(INTDIR)\apr_getpass.obj"
	-@erase "$(INTDIR)\apr_md5.obj"
	-@erase "$(INTDIR)\apr_pools.obj"
	-@erase "$(INTDIR)\apr_slack.obj"
	-@erase "$(INTDIR)\apr_snprintf.obj"
	-@erase "$(INTDIR)\apr_tables.obj"
	-@erase "$(INTDIR)\dir.obj"
	-@erase "$(INTDIR)\fileacc.obj"
	-@erase "$(INTDIR)\filedup.obj"
	-@erase "$(INTDIR)\filestat.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\locks.obj"
	-@erase "$(INTDIR)\names.obj"
	-@erase "$(INTDIR)\open.obj"
	-@erase "$(INTDIR)\pipe.obj"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\proc.obj"
	-@erase "$(INTDIR)\readdir.obj"
	-@erase "$(INTDIR)\readwrite.obj"
	-@erase "$(INTDIR)\seek.obj"
	-@erase "$(INTDIR)\sendrecv.obj"
	-@erase "$(INTDIR)\signals.obj"
	-@erase "$(INTDIR)\sockaddr.obj"
	-@erase "$(INTDIR)\sockets.obj"
	-@erase "$(INTDIR)\sockopt.obj"
	-@erase "$(INTDIR)\start.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\threadcancel.obj"
	-@erase "$(INTDIR)\threadpriv.obj"
	-@erase "$(INTDIR)\time.obj"
	-@erase "$(INTDIR)\timestr.obj"
	-@erase "$(INTDIR)\timetest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\aprlib.dll"
	-@erase "$(OUTDIR)\aprlib.exp"
	-@erase "$(OUTDIR)\aprlib.ilk"
	-@erase "$(OUTDIR)\aprlib.lib"
	-@erase "$(OUTDIR)\aprlib.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "./include" /I "./inc" /I\
 "./misc/win32" /I "./file_io/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\aprlib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aprlib.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib ws2_32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\aprlib.pdb" /debug /machine:I386 /def:".\aprlib.def"\
 /out:"$(OUTDIR)\aprlib.dll" /implib:"$(OUTDIR)\aprlib.lib" /pdbtype:sept 
DEF_FILE= \
	".\aprlib.def"
LINK32_OBJS= \
	"$(INTDIR)\access.obj" \
	"$(INTDIR)\apr_cpystrn.obj" \
	"$(INTDIR)\apr_execve.obj" \
	"$(INTDIR)\apr_fnmatch.obj" \
	"$(INTDIR)\apr_getpass.obj" \
	"$(INTDIR)\apr_md5.obj" \
	"$(INTDIR)\apr_pools.obj" \
	"$(INTDIR)\apr_slack.obj" \
	"$(INTDIR)\apr_snprintf.obj" \
	"$(INTDIR)\apr_tables.obj" \
	"$(INTDIR)\dir.obj" \
	"$(INTDIR)\fileacc.obj" \
	"$(INTDIR)\filedup.obj" \
	"$(INTDIR)\filestat.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\locks.obj" \
	"$(INTDIR)\names.obj" \
	"$(INTDIR)\open.obj" \
	"$(INTDIR)\pipe.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\proc.obj" \
	"$(INTDIR)\readdir.obj" \
	"$(INTDIR)\readwrite.obj" \
	"$(INTDIR)\seek.obj" \
	"$(INTDIR)\sendrecv.obj" \
	"$(INTDIR)\signals.obj" \
	"$(INTDIR)\sockaddr.obj" \
	"$(INTDIR)\sockets.obj" \
	"$(INTDIR)\sockopt.obj" \
	"$(INTDIR)\start.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\threadcancel.obj" \
	"$(INTDIR)\threadpriv.obj" \
	"$(INTDIR)\time.obj" \
	"$(INTDIR)\timestr.obj" \
	"$(INTDIR)\timetest.obj"

"$(OUTDIR)\aprlib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "aprlib - Win32 Release" || "$(CFG)" == "aprlib - Win32 Debug"
SOURCE=.\time\win32\access.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_ACCES=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\time\win32\atime.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_ACCES=\
	".\include\apr.h"\
	

"$(INTDIR)\access.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_ACCES=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\time\win32\atime.h"\
	

"$(INTDIR)\access.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_cpystrn.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_C=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_C=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_cpystrn.obj" : $(SOURCE) $(DEP_CPP_APR_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_C=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\apr_cpystrn.obj" : $(SOURCE) $(DEP_CPP_APR_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_execve.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

NODEP_CPP_APR_E=\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_execve.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"


"$(INTDIR)\apr_execve.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_fnmatch.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_F=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_fnmatch.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_F=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_fnmatch.obj" : $(SOURCE) $(DEP_CPP_APR_F) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_F=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_fnmatch.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	

"$(INTDIR)\apr_fnmatch.obj" : $(SOURCE) $(DEP_CPP_APR_F) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_getpass.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_G=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_G=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_getpass.obj" : $(SOURCE) $(DEP_CPP_APR_G) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_G=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\apr_getpass.obj" : $(SOURCE) $(DEP_CPP_APR_G) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_md5.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_M=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_md5.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_M=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_md5.obj" : $(SOURCE) $(DEP_CPP_APR_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_M=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_md5.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	

"$(INTDIR)\apr_md5.obj" : $(SOURCE) $(DEP_CPP_APR_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_pools.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_P=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\misc\win32\misc.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_P=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_pools.obj" : $(SOURCE) $(DEP_CPP_APR_P) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_P=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\misc\win32\misc.h"\
	

"$(INTDIR)\apr_pools.obj" : $(SOURCE) $(DEP_CPP_APR_P) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_slack.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_S=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_S=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	".\lib\http_log.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I "./include" /I "./inc" /I\
 "./misc/win32" /I "./file_io/win32" /I "./misc/win32/" /I "./file_io/win32/" /D\
 "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\aprlib.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\apr_slack.obj" : $(SOURCE) $(DEP_CPP_APR_S) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_S=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "./include" /I "./inc" /I\
 "./misc/win32" /I "./file_io/win32" /I "./misc/win32/" /I "./file_io/win32/" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\aprlib.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\apr_slack.obj" : $(SOURCE) $(DEP_CPP_APR_S) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\lib\apr_snprintf.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_SN=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_SN=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_snprintf.obj" : $(SOURCE) $(DEP_CPP_APR_SN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_SN=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\apr_snprintf.obj" : $(SOURCE) $(DEP_CPP_APR_SN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lib\apr_tables.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_APR_T=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\misc\win32\misc.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_APR_T=\
	".\include\apr.h"\
	".\lib\apr_config.h"\
	

"$(INTDIR)\apr_tables.obj" : $(SOURCE) $(DEP_CPP_APR_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_APR_T=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\misc\win32\misc.h"\
	

"$(INTDIR)\apr_tables.obj" : $(SOURCE) $(DEP_CPP_APR_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\dir.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_DIR_C=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_DIR_C=\
	".\include\apr.h"\
	

"$(INTDIR)\dir.obj" : $(SOURCE) $(DEP_CPP_DIR_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_DIR_C=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\dir.obj" : $(SOURCE) $(DEP_CPP_DIR_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\fileacc.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_FILEA=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_FILEA=\
	".\include\apr.h"\
	

"$(INTDIR)\fileacc.obj" : $(SOURCE) $(DEP_CPP_FILEA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_FILEA=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\fileacc.obj" : $(SOURCE) $(DEP_CPP_FILEA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\filedup.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_FILED=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_FILED=\
	".\include\apr.h"\
	

"$(INTDIR)\filedup.obj" : $(SOURCE) $(DEP_CPP_FILED) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_FILED=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\filedup.obj" : $(SOURCE) $(DEP_CPP_FILED) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\filestat.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_FILES=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_FILES=\
	".\include\apr.h"\
	

"$(INTDIR)\filestat.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_FILES=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\filestat.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\misc\win32\getopt.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_GETOP=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\misc\win32\misc.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_GETOP=\
	".\include\apr.h"\
	

"$(INTDIR)\getopt.obj" : $(SOURCE) $(DEP_CPP_GETOP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_GETOP=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\misc\win32\misc.h"\
	

"$(INTDIR)\getopt.obj" : $(SOURCE) $(DEP_CPP_GETOP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\locks\win32\locks.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_LOCKS=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\locks\win32\locks.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_LOCKS=\
	".\include\apr.h"\
	

"$(INTDIR)\locks.obj" : $(SOURCE) $(DEP_CPP_LOCKS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_LOCKS=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\locks\win32\locks.h"\
	

"$(INTDIR)\locks.obj" : $(SOURCE) $(DEP_CPP_LOCKS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\misc\win32\names.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_NAMES=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NAMES=\
	".\include\apr.h"\
	

"$(INTDIR)\names.obj" : $(SOURCE) $(DEP_CPP_NAMES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_NAMES=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\names.obj" : $(SOURCE) $(DEP_CPP_NAMES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\open.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_OPEN_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_OPEN_=\
	".\include\apr.h"\
	

"$(INTDIR)\open.obj" : $(SOURCE) $(DEP_CPP_OPEN_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_OPEN_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\open.obj" : $(SOURCE) $(DEP_CPP_OPEN_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\pipe.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_PIPE_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_PIPE_=\
	".\include\apr.h"\
	

"$(INTDIR)\pipe.obj" : $(SOURCE) $(DEP_CPP_PIPE_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_PIPE_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\pipe.obj" : $(SOURCE) $(DEP_CPP_PIPE_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\network_io\win32\poll.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_POLL_=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_POLL_=\
	".\include\apr.h"\
	

"$(INTDIR)\poll.obj" : $(SOURCE) $(DEP_CPP_POLL_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_POLL_=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	

"$(INTDIR)\poll.obj" : $(SOURCE) $(DEP_CPP_POLL_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\threadproc\win32\proc.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_PROC_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\threadproc\win32\threadproc.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_PROC_=\
	".\include\apr.h"\
	

"$(INTDIR)\proc.obj" : $(SOURCE) $(DEP_CPP_PROC_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_PROC_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_pools.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\threadproc\win32\threadproc.h"\
	

"$(INTDIR)\proc.obj" : $(SOURCE) $(DEP_CPP_PROC_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\readdir.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_READD=\
	".\file_io\win32\readdir.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\readdir.obj" : $(SOURCE) $(DEP_CPP_READD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_READD=\
	".\file_io\win32\readdir.h"\
	

"$(INTDIR)\readdir.obj" : $(SOURCE) $(DEP_CPP_READD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\readwrite.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_READW=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_READW=\
	".\include\apr.h"\
	

"$(INTDIR)\readwrite.obj" : $(SOURCE) $(DEP_CPP_READW) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_READW=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\readwrite.obj" : $(SOURCE) $(DEP_CPP_READW) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\file_io\win32\seek.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_SEEK_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_SEEK_=\
	".\include\apr.h"\
	

"$(INTDIR)\seek.obj" : $(SOURCE) $(DEP_CPP_SEEK_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_SEEK_=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	

"$(INTDIR)\seek.obj" : $(SOURCE) $(DEP_CPP_SEEK_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\network_io\win32\sendrecv.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_SENDR=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_SENDR=\
	".\include\apr.h"\
	

"$(INTDIR)\sendrecv.obj" : $(SOURCE) $(DEP_CPP_SENDR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_SENDR=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	

"$(INTDIR)\sendrecv.obj" : $(SOURCE) $(DEP_CPP_SENDR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\threadproc\win32\signals.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_SIGNA=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\threadproc\win32\threadproc.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_SIGNA=\
	".\include\apr.h"\
	

"$(INTDIR)\signals.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_SIGNA=\
	".\file_io\win32\fileio.h"\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\threadproc\win32\threadproc.h"\
	

"$(INTDIR)\signals.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\network_io\win32\sockaddr.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_SOCKA=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_SOCKA=\
	".\include\apr.h"\
	

"$(INTDIR)\sockaddr.obj" : $(SOURCE) $(DEP_CPP_SOCKA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_SOCKA=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	

"$(INTDIR)\sockaddr.obj" : $(SOURCE) $(DEP_CPP_SOCKA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\network_io\win32\sockets.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_SOCKE=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_SOCKE=\
	".\include\apr.h"\
	

"$(INTDIR)\sockets.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_SOCKE=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	

"$(INTDIR)\sockets.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\network_io\win32\sockopt.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_SOCKO=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_SOCKO=\
	".\include\apr.h"\
	

"$(INTDIR)\sockopt.obj" : $(SOURCE) $(DEP_CPP_SOCKO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_SOCKO=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_network_io.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\network_io\win32\networkio.h"\
	

"$(INTDIR)\sockopt.obj" : $(SOURCE) $(DEP_CPP_SOCKO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\misc\win32\start.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_START=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\misc\win32\misc.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_START=\
	".\include\apr.h"\
	

"$(INTDIR)\start.obj" : $(SOURCE) $(DEP_CPP_START) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_START=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_getopt.h"\
	".\include\apr_lib.h"\
	".\include\apr_pools.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\misc\win32\misc.h"\
	

"$(INTDIR)\start.obj" : $(SOURCE) $(DEP_CPP_START) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\threadproc\win32\thread.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_THREA=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\threadproc\win32\threadproc.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_THREA=\
	".\include\apr.h"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_THREA=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\threadproc\win32\threadproc.h"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\threadproc\win32\threadcancel.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_THREAD=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\threadproc\win32\threadproc.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_THREAD=\
	".\include\apr.h"\
	

"$(INTDIR)\threadcancel.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_THREAD=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\threadproc\win32\threadproc.h"\
	

"$(INTDIR)\threadcancel.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\threadproc\win32\threadpriv.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_THREADP=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\threadproc\win32\threadproc.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_THREADP=\
	".\include\apr.h"\
	

"$(INTDIR)\threadpriv.obj" : $(SOURCE) $(DEP_CPP_THREADP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_THREADP=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\threadproc\win32\threadproc.h"\
	

"$(INTDIR)\threadpriv.obj" : $(SOURCE) $(DEP_CPP_THREADP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\time\win32\time.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_TIME_=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\time\win32\atime.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_TIME_=\
	".\include\apr.h"\
	

"$(INTDIR)\time.obj" : $(SOURCE) $(DEP_CPP_TIME_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_TIME_=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lib.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\time\win32\atime.h"\
	

"$(INTDIR)\time.obj" : $(SOURCE) $(DEP_CPP_TIME_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\time\win32\timestr.c

!IF  "$(CFG)" == "aprlib - Win32 Release"

DEP_CPP_TIMES=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\time\win32\atime.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_TIMES=\
	".\include\apr.h"\
	

"$(INTDIR)\timestr.obj" : $(SOURCE) $(DEP_CPP_TIMES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

DEP_CPP_TIMES=\
	".\include\apr_errno.h"\
	".\include\apr_file_io.h"\
	".\include\apr_general.h"\
	".\include\apr_lock.h"\
	".\include\apr_network_io.h"\
	".\include\apr_portable.h"\
	".\include\apr_thread_proc.h"\
	".\include\apr_time.h"\
	".\include\apr_win.h"\
	".\include\apr_winconfig.h"\
	".\time\win32\atime.h"\
	

"$(INTDIR)\timestr.obj" : $(SOURCE) $(DEP_CPP_TIMES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\misc\win32\timetest.c

"$(INTDIR)\timetest.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

