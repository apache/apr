# Microsoft Developer Studio Project File - Name="aprtest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=aprtest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aprtest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aprtest.mak" CFG="aprtest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aprtest - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "aprtest - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "aprtest - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "aprtest.exe"
# PROP BASE Bsc_Name "aprtest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f aprtest.win /a"
# PROP Rebuild_Opt ""
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "aprtest - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f aprtest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "aprtest.exe"
# PROP BASE Bsc_Name "aprtest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "NMAKE /f aprtest.win /a"
# PROP Rebuild_Opt ""
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "aprtest - Win32 Release"
# Name "aprtest - Win32 Debug"

!IF  "$(CFG)" == "aprtest - Win32 Release"

!ELSEIF  "$(CFG)" == "aprtest - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\aprtest.win

!IF  "$(CFG)" == "aprtest - Win32 Release"

!ELSEIF  "$(CFG)" == "aprtest - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Makefile.in
# End Source File
# Begin Source File

SOURCE=.\MakeWin32Make.pl
# End Source File
# End Target
# End Project
