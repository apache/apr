# Microsoft Developer Studio Project File - Name="aprlibdll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=aprlibdll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aprlibdll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aprlibdll.mak" CFG="aprlibdll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aprlibdll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aprlibdll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aprlibdll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /I "dso/win32" /I "file_io/win32" /I "locks/win32" /I "misc/win32" /I "network_io/win32" /I "threadproc/win32" /I "time/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"Release/aprlib.dll" /libpath:"LibR" /base:@"..\..\os\win32\BaseAddr.ref",aprlib
# ADD LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"Release/aprlib.dll" /libpath:"LibR" /base:@"..\..\os\win32\BaseAddr.ref",aprlib

!ELSEIF  "$(CFG)" == "aprlibdll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "include" /I "dso/win32" /I "file_io/win32" /I "locks/win32" /I "misc/win32" /I "network_io/win32" /I "threadproc/win32" /I "time/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /out:"Debug/aprlib.dll" /libpath:"LibD" /base:@"..\..\os\win32\BaseAddr.ref",aprlib
# ADD LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/aprlib.dll" /libpath:"LibD" /base:@"..\..\os\win32\BaseAddr.ref",aprlib
# SUBTRACT LINK32 /incremental:no /map

!ENDIF 

# Begin Target

# Name "aprlibdll - Win32 Release"
# Name "aprlibdll - Win32 Debug"
# Begin Source File

SOURCE=.\misc\win32\aprlib.c
# End Source File
# Begin Source File

SOURCE=.\aprlib.def
# End Source File
# End Target
# End Project
