# Microsoft Developer Studio Generated NMAKE File, Based on apr_app.dsp
!IF "$(CFG)" == ""
CFG=apr_app - Win32 Release
!MESSAGE No configuration specified. Defaulting to apr_app - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "apr_app - Win32 Release" && "$(CFG)" != "apr_app - Win32 Debug" && "$(CFG)" != "apr_app - Win32 ReleaseNT" && "$(CFG)" != "apr_app - Win32 DebugNT" && "$(CFG)" != "apr_app - x64 Release" && "$(CFG)" != "apr_app - x64 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "apr_app.mak" CFG="apr_app - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "apr_app - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "apr_app - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "apr_app - Win32 ReleaseNT" (based on "Win32 (x86) Static Library")
!MESSAGE "apr_app - Win32 DebugNT" (based on "Win32 (x86) Static Library")
!MESSAGE "apr_app - x64 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "apr_app - x64 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "apr_app - Win32 Release"

OUTDIR=.\LibR
INTDIR=.\LibR
# Begin Custom Macros
OutDir=.\LibR
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\apr_app-1.lib"

!ELSE 

ALL : "apr - Win32 Release" "$(OUTDIR)\apr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app-1.idb"
	-@erase "$(INTDIR)\apr_app-1.pdb"
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(OUTDIR)\apr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\apr_app-1" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\LibR\apr-1.lib"

"$(OUTDIR)\apr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "apr_app - Win32 Debug"

OUTDIR=.\LibD
INTDIR=.\LibD
# Begin Custom Macros
OutDir=.\LibD
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\apr_app-1.lib"

!ELSE 

ALL : "apr - Win32 Debug" "$(OUTDIR)\apr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app-1.idb"
	-@erase "$(INTDIR)\apr_app-1.pdb"
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(OUTDIR)\apr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\apr_app-1" /FD /EHsc /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\LibD\apr-1.lib"

"$(OUTDIR)\apr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "apr_app - Win32 ReleaseNT"

OUTDIR=.\NT\LibR
INTDIR=.\NT\LibR
# Begin Custom Macros
OutDir=.\NT\LibR
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\apr_app-1.lib"

!ELSE 

ALL : "apr - Win32 ReleaseNT" "$(OUTDIR)\apr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseNTCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app-1.idb"
	-@erase "$(INTDIR)\apr_app-1.pdb"
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(OUTDIR)\apr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\apr_app-1" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\NT\LibR\apr-1.lib"

"$(OUTDIR)\apr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "apr_app - Win32 DebugNT"

OUTDIR=.\NT\LibD
INTDIR=.\NT\LibD
# Begin Custom Macros
OutDir=.\NT\LibD
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\apr_app-1.lib"

!ELSE 

ALL : "apr - Win32 DebugNT" "$(OUTDIR)\apr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugNTCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app-1.idb"
	-@erase "$(INTDIR)\apr_app-1.pdb"
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(OUTDIR)\apr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\apr_app-1" /FD /EHsc /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\NT\LibD\apr-1.lib"

"$(OUTDIR)\apr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "apr_app - x64 Release"

OUTDIR=.\x64\LibR
INTDIR=.\x64\LibR
# Begin Custom Macros
OutDir=.\x64\LibR
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\apr_app-1.lib"

!ELSE 

ALL : "apr - x64 Release" "$(OUTDIR)\apr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - x64 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app-1.idb"
	-@erase "$(INTDIR)\apr_app-1.pdb"
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(OUTDIR)\apr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\apr_app-1" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\x64\LibR\apr-1.lib"

"$(OUTDIR)\apr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "apr_app - x64 Debug"

OUTDIR=.\x64\LibD
INTDIR=.\x64\LibD
# Begin Custom Macros
OutDir=.\x64\LibD
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\apr_app-1.lib"

!ELSE 

ALL : "apr - x64 Debug" "$(OUTDIR)\apr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - x64 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app-1.idb"
	-@erase "$(INTDIR)\apr_app-1.pdb"
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(OUTDIR)\apr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\apr_app-1" /FD /EHsc /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\x64\LibD\apr-1.lib"

"$(OUTDIR)\apr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("apr_app.dep")
!INCLUDE "apr_app.dep"
!ELSE 
!MESSAGE Warning: cannot find "apr_app.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "apr_app - Win32 Release" || "$(CFG)" == "apr_app - Win32 Debug" || "$(CFG)" == "apr_app - Win32 ReleaseNT" || "$(CFG)" == "apr_app - Win32 DebugNT" || "$(CFG)" == "apr_app - x64 Release" || "$(CFG)" == "apr_app - x64 Debug"

!IF  "$(CFG)" == "apr_app - Win32 Release"

"apr - Win32 Release" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" 
   cd ".\build"

"apr - Win32 ReleaseCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "apr_app - Win32 Debug"

"apr - Win32 Debug" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" 
   cd ".\build"

"apr - Win32 DebugCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "apr_app - Win32 ReleaseNT"

"apr - Win32 ReleaseNT" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 ReleaseNT" 
   cd ".\build"

"apr - Win32 ReleaseNTCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 ReleaseNT" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "apr_app - Win32 DebugNT"

"apr - Win32 DebugNT" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 DebugNT" 
   cd ".\build"

"apr - Win32 DebugNTCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 DebugNT" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "apr_app - x64 Release"

"apr - x64 Release" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - x64 Release" 
   cd ".\build"

"apr - x64 ReleaseCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - x64 Release" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "apr_app - x64 Debug"

"apr - x64 Debug" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - x64 Debug" 
   cd ".\build"

"apr - x64 DebugCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - x64 Debug" RECURSE=1 CLEAN 
   cd ".\build"

!ENDIF 

SOURCE=..\misc\win32\apr_app.c

"$(INTDIR)\apr_app.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

