# Microsoft Developer Studio Generated NMAKE File, Based on testappnt.dsp
!IF "$(CFG)" == ""
CFG=testappnt - Win32 Debug
!MESSAGE No configuration specified. Defaulting to testappnt - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "testappnt - Win32 Release" && "$(CFG)" != "testappnt - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "testappnt.mak" CFG="testappnt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testappnt - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testappnt - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "testappnt - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testappnt.exe"

!ELSE 

ALL : "apr_app - Win32 Release" "apr - Win32 Release" "$(OUTDIR)\testappnt.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseCLEAN" "apr_app - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(OUTDIR)\testappnt.exe"
	-@erase ".\testappnt.idb"
	-@erase ".\testappnt.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "../include" /D "NDEBUG" /D "WIN32" /D "WINNT" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"./testappnt" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testappnt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /entry:"wmainCRTStartup" /subsystem:console /incremental:no /pdb:"$(OUTDIR)\testappnt.pdb" /out:"$(OUTDIR)\testappnt.exe" 
LINK32_OBJS= \
	".\testappnt.obj" \
	"..\LibR\apr.lib" \
	"..\build\LibR\apr_app.lib"

"$(OUTDIR)\testappnt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "testappnt - Win32 Debug"

OUTDIR=.\.
INTDIR=.\.
# Begin Custom Macros
OutDir=.\.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testappnt.exe"

!ELSE 

ALL : "apr_app - Win32 Debug" "apr - Win32 Debug" "$(OUTDIR)\testappnt.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugCLEAN" "apr_app - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\testappnt.idb"
	-@erase "$(INTDIR)\testappnt.obj"
	-@erase "$(OUTDIR)\testappnt.exe"
	-@erase "$(OUTDIR)\testappnt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "WINNT" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\testappnt" /FD /EHsc /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testappnt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /entry:"wmainCRTStartup" /subsystem:console /incremental:no /pdb:"$(OUTDIR)\testappnt.pdb" /debug /out:"$(OUTDIR)\testappnt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\testappnt.obj" \
	"..\LibD\apr.lib" \
	"..\build\LibD\apr_app.lib"

"$(OUTDIR)\testappnt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("testappnt.dep")
!INCLUDE "testappnt.dep"
!ELSE 
!MESSAGE Warning: cannot find "testappnt.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "testappnt - Win32 Release" || "$(CFG)" == "testappnt - Win32 Debug"

!IF  "$(CFG)" == "testappnt - Win32 Release"

"apr - Win32 Release" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" 
   cd ".\test"

"apr - Win32 ReleaseCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" RECURSE=1 CLEAN 
   cd ".\test"

!ELSEIF  "$(CFG)" == "testappnt - Win32 Debug"

"apr - Win32 Debug" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" 
   cd ".\test"

"apr - Win32 DebugCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" RECURSE=1 CLEAN 
   cd ".\test"

!ENDIF 

!IF  "$(CFG)" == "testappnt - Win32 Release"

"apr_app - Win32 Release" : 
   cd ".\..\build"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr_app.mak" CFG="apr_app - Win32 Release" 
   cd "..\test"

"apr_app - Win32 ReleaseCLEAN" : 
   cd ".\..\build"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr_app.mak" CFG="apr_app - Win32 Release" RECURSE=1 CLEAN 
   cd "..\test"

!ELSEIF  "$(CFG)" == "testappnt - Win32 Debug"

"apr_app - Win32 Debug" : 
   cd ".\..\build"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr_app.mak" CFG="apr_app - Win32 Debug" 
   cd "..\test"

"apr_app - Win32 DebugCLEAN" : 
   cd ".\..\build"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr_app.mak" CFG="apr_app - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\test"

!ENDIF 

SOURCE=.\testapp.c

!IF  "$(CFG)" == "testappnt - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O2 /I "../include" /D "NDEBUG" /D "WIN32" /D "WINNT" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fo"testappnt" /Fd"./testappnt" /FD /c 

".\testappnt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "testappnt - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Zi /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "WINNT" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fo"$(INTDIR)\testappnt" /Fd"$(INTDIR)\testappnt" /FD /EHsc /c 

"$(INTDIR)\testappnt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

