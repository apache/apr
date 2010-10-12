# Microsoft Developer Studio Generated NMAKE File, Based on libapr_app.dsp
!IF "$(CFG)" == ""
CFG=libapr_app - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libapr_app - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libapr_app - Win32 Release" && "$(CFG)" != "libapr_app - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapr_app.mak" CFG="libapr_app - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapr_app - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libapr_app - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libapr_app - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr_app.lib"

!ELSE 

ALL : "libapr - Win32 Release" "$(OUTDIR)\libapr_app.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app_src.idb"
	-@erase "$(INTDIR)\libapr_app_src.pdb"
	-@erase "$(OUTDIR)\libapr_app.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\libapr_app_src" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\Release\libapr.lib"

"$(OUTDIR)\libapr_app.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr_app - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr_app.lib"

!ELSE 

ALL : "libapr - Win32 Debug" "$(OUTDIR)\libapr_app.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app_src.idb"
	-@erase "$(INTDIR)\libapr_app_src.pdb"
	-@erase "$(OUTDIR)\libapr_app.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\libapr_app_src" /FD /EHsc /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapr_app.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\Debug\libapr.lib"

"$(OUTDIR)\libapr_app.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libapr_app.dep")
!INCLUDE "libapr_app.dep"
!ELSE 
!MESSAGE Warning: cannot find "libapr_app.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libapr_app - Win32 Release" || "$(CFG)" == "libapr_app - Win32 Debug"

!IF  "$(CFG)" == "libapr_app - Win32 Release"

"libapr - Win32 Release" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Release" 
   cd ".\build"

"libapr - Win32 ReleaseCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Release" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "libapr_app - Win32 Debug"

"libapr - Win32 Debug" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Debug" 
   cd ".\build"

"libapr - Win32 DebugCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Debug" RECURSE=1 CLEAN 
   cd ".\build"

!ENDIF 

SOURCE=..\misc\win32\apr_app.c

"$(INTDIR)\apr_app.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\misc\win32\internal.c

"$(INTDIR)\internal.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

