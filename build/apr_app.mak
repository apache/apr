# Microsoft Developer Studio Generated NMAKE File, Based on apr_app.dsp
!IF "$(CFG)" == ""
CFG=apr_app - Win32 Debug
!MESSAGE No configuration specified. Defaulting to apr_app - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "apr_app - Win32 Release" && "$(CFG)" != "apr_app - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "apr_app.mak" CFG="apr_app - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "apr_app - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "apr_app - Win32 Debug" (based on "Win32 (x86) Static Library")
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

ALL : "$(OUTDIR)\apr_app.lib"

!ELSE 

ALL : "apr - Win32 Release" "$(OUTDIR)\apr_app.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\apr_app_src.idb"
	-@erase "$(INTDIR)\apr_app_src.pdb"
	-@erase "$(OUTDIR)\apr_app.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\apr_app_src" /FD /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\LibR\apr.lib"

"$(OUTDIR)\apr_app.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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

ALL : "$(OUTDIR)\apr_app.lib"

!ELSE 

ALL : "apr - Win32 Debug" "$(OUTDIR)\apr_app.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\apr_app_src.idb"
	-@erase "$(INTDIR)\apr_app_src.pdb"
	-@erase "$(OUTDIR)\apr_app.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /D "APR_DECLARE_STATIC" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\apr_app_src" /FD /EHsc /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apr_app.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"..\LibD\apr.lib"

"$(OUTDIR)\apr_app.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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


!IF "$(CFG)" == "apr_app - Win32 Release" || "$(CFG)" == "apr_app - Win32 Debug"

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

!ENDIF 

SOURCE=..\misc\win32\apr_app.c

"$(INTDIR)\apr_app.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

