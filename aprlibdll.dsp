# Microsoft Developer Studio Project File - Name="aprlibdll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
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
!MESSAGE "aprlibdll - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aprlibdll - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./include" /I "./inc" /I "./misc/win32" /I "./file_io/win32" /I "./time/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /machine:I386 /def:".\aprlib.def" /out:"Release/aprlib.dll" /libpath:"LibR" /base:@"..\..\os\win32\BaseAddr.ref",aprlib
# ADD LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /machine:I386 /def:".\aprlib.def" /out:"Release/aprlib.dll" /libpath:"LibR" /base:@"..\..\os\win32\BaseAddr.ref",aprlib

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "./include" /I "./inc" /I "./misc/win32" /I "./file_io/win32" /I "./time/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 aprlib.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /def:".\aprlib.def" /out:"Debug/aprlib.dll" /libpath:"LibD" /base:@"..\..\os\win32\BaseAddr.ref",aprlib
# ADD LINK32 apr.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /def:".\aprlib.def" /out:"Debug/aprlib.dll" /libpath:"LibD" /base:@"..\..\os\win32\BaseAddr.ref",aprlib

!ENDIF 

# Begin Target

# Name "aprlibdll - Win32 Release"
# Name "aprlibdll - Win32 Debug"
# Begin Source File

SOURCE=.\misc\win32\aprlib.c
# End Source File
# Begin Source File

SOURCE=.\aprlib.def
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
