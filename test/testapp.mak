# Microsoft Developer Studio Generated NMAKE File, Based on testapp.dsp
!IF "$(CFG)" == ""
CFG=testapp - Win32 Debug
!MESSAGE No configuration specified. Defaulting to testapp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "testapp - Win32 Release" && "$(CFG)" != "testapp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "testapp.mak" CFG="testapp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testapp - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testapp - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "testapp - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testapp.exe"

!ELSE 

ALL : "apr_app - Win32 Release" "apr - Win32 Release" "$(OUTDIR)\testapp.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseCLEAN" "apr_app - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\testapp.obj"
	-@erase "$(OUTDIR)\testapp.exe"
	-@erase ".\testapp.idb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "../include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"./testapp" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testapp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\testapp.pdb" /out:"$(OUTDIR)\testapp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\testapp.obj" \
	"..\LibR\apr.lib" \
	"..\build\LibR\apr_app.lib"

"$(OUTDIR)\testapp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "testapp - Win32 Debug"

OUTDIR=.\.
INTDIR=.\.
# Begin Custom Macros
OutDir=.\.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testapp.exe"

!ELSE 

ALL : "apr_app - Win32 Debug" "apr - Win32 Debug" "$(OUTDIR)\testapp.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugCLEAN" "apr_app - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\testapp.idb"
	-@erase "$(INTDIR)\testapp.obj"
	-@erase "$(OUTDIR)\testapp.exe"
	-@erase "$(OUTDIR)\testapp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\testapp" /FD /EHsc /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testapp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\testapp.pdb" /debug /out:"$(OUTDIR)\testapp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\testapp.obj" \
	"..\LibD\apr.lib" \
	"..\build\LibD\apr_app.lib"

"$(OUTDIR)\testapp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("testapp.dep")
!INCLUDE "testapp.dep"
!ELSE 
!MESSAGE Warning: cannot find "testapp.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "testapp - Win32 Release" || "$(CFG)" == "testapp - Win32 Debug"

!IF  "$(CFG)" == "testapp - Win32 Release"

"apr - Win32 Release" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" 
   cd ".\test"

"apr - Win32 ReleaseCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" RECURSE=1 CLEAN 
   cd ".\test"

!ELSEIF  "$(CFG)" == "testapp - Win32 Debug"

"apr - Win32 Debug" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" 
   cd ".\test"

"apr - Win32 DebugCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" RECURSE=1 CLEAN 
   cd ".\test"

!ENDIF 

!IF  "$(CFG)" == "testapp - Win32 Release"

"apr_app - Win32 Release" : 
   cd ".\..\build"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr_app.mak" CFG="apr_app - Win32 Release" 
   cd "..\test"

"apr_app - Win32 ReleaseCLEAN" : 
   cd ".\..\build"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr_app.mak" CFG="apr_app - Win32 Release" RECURSE=1 CLEAN 
   cd "..\test"

!ELSEIF  "$(CFG)" == "testapp - Win32 Debug"

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

"$(INTDIR)\testapp.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

