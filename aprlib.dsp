# Microsoft Developer Studio Project File - Name="aprlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=aprlib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aprlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aprlib.mak" CFG="aprlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aprlib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "aprlib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "aprlib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "LibR"
# PROP BASE Intermediate_Dir "LibR"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "LibR"
# PROP Intermediate_Dir "LibR"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./include" /I "./inc" /I "./misc/win32" /I "./file_io/win32" /I "./time/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\LibR\apr.lib"
# ADD LIB32 /nologo /out:".\LibR\apr.lib"

!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "LibD"
# PROP BASE Intermediate_Dir "LibD"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "LibD"
# PROP Intermediate_Dir "LibD"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "./include" /I "./inc" /I "./misc/win32" /I "./file_io/win32" /I "./time/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\LibD\apr.lib"
# ADD LIB32 /nologo /out:".\LibD\apr.lib"

!ENDIF 

# Begin Target

# Name "aprlib - Win32 Release"
# Name "aprlib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=.\time\win32\access.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_cpystrn.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_execve.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_fnmatch.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_getpass.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_md5.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_pools.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_snprintf.c
# End Source File
# Begin Source File

SOURCE=.\lib\apr_tables.c
# End Source File
# Begin Source File

SOURCE=.\aprlib.def
# End Source File
# Begin Source File

SOURCE=.\misc\win32\canonerr.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\dir.c
# End Source File
# Begin Source File

SOURCE=.\dso\win32\dso.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\errorcodes.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\fileacc.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\filedup.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\fileio.h
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\filestat.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\getopt.c
# End Source File
# Begin Source File

SOURCE=.\locks\win32\locks.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\names.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\open.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\pipe.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\poll.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\proc.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\rand.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\readwrite.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\seek.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sendrecv.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\signals.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sockaddr.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sockets.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sockopt.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\start.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\thread.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\threadpriv.c
# End Source File
# Begin Source File

SOURCE=.\time\win32\time.c
# End Source File
# Begin Source File

SOURCE=.\time\win32\timestr.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\include\apr_errno.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_file_io.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_fnmatch.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_general.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_lib.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_lock.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_md5.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_network_io.h
# End Source File
# Begin Source File

SOURCE=.\inc\apr_pools.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_portable.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_shmem.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_signal.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_thread_proc.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_time.h
# End Source File
# Begin Source File

SOURCE=.\time\win32\atime.h
# End Source File
# Begin Source File

SOURCE=.\dso\win32\dso.h
# End Source File
# Begin Source File

SOURCE=.\locks\win32\locks.h
# End Source File
# Begin Source File

SOURCE=.\misc\win32\misc.h
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\networkio.h
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\threadproc.h
# End Source File
# End Group
# Begin Group "Generated Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\apr.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_private.h
# End Source File
# End Group
# Begin Group "Internal Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\apr_private.hw

!IF  "$(CFG)" == "aprlib - Win32 Release"

# Begin Custom Build
InputPath=.\include\apr_private.hw

".\include\apr_private.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apr_private.hw .\include\apr_private.h > nul 
	echo Created apr_private.h from apr_private.hw 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

# Begin Custom Build
InputPath=.\include\apr_private.hw

".\include\apr_private.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apr_private.hw .\include\apr_private.h > nul 
	echo Created apr_private.h from apr_private.hw 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "External Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\apr.hw

!IF  "$(CFG)" == "aprlib - Win32 Release"

# Begin Custom Build
InputPath=.\include\apr.hw

".\include\apr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apr.hw .\include\apr.h > nul 
	echo Created apr.h from apr.hw 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "aprlib - Win32 Debug"

# Begin Custom Build
InputPath=.\include\apr.hw

".\include\apr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apr.hw .\include\apr.h > nul 
	echo Created apr.h from apr.hw 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
