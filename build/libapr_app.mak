# Microsoft Developer Studio Generated NMAKE File, Based on libapr_app.dsp
!IF "$(CFG)" == ""
CFG=libapr_app - Win32 Release
!MESSAGE No configuration specified. Defaulting to libapr_app - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "libapr_app - Win32 Release" && "$(CFG)" != "libapr_app - Win32 Debug" && "$(CFG)" != "libapr_app - Win32 ReleaseNT" && "$(CFG)" != "libapr_app - Win32 DebugNT" && "$(CFG)" != "libapr_app - x64 Release" && "$(CFG)" != "libapr_app - x64 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapr_app.mak" CFG="libapr_app - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapr_app - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libapr_app - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libapr_app - Win32 ReleaseNT" (based on "Win32 (x86) Static Library")
!MESSAGE "libapr_app - Win32 DebugNT" (based on "Win32 (x86) Static Library")
!MESSAGE "libapr_app - x64 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libapr_app - x64 Debug" (based on "Win32 (x86) Static Library")
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

ALL : "$(OUTDIR)\libapr_app-1.lib"

!ELSE 

ALL : "libapr - Win32 Release" "$(OUTDIR)\libapr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app-1.idb"
	-@erase "$(INTDIR)\libapr_app-1.pdb"
	-@erase "$(OUTDIR)\libapr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\libapr_app-1" /FD /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\Release\libapr-1.lib"

"$(OUTDIR)\libapr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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

ALL : "$(OUTDIR)\libapr_app-1.lib"

!ELSE 

ALL : "libapr - Win32 Debug" "$(OUTDIR)\libapr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app-1.idb"
	-@erase "$(INTDIR)\libapr_app-1.pdb"
	-@erase "$(OUTDIR)\libapr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\libapr_app-1" /FD /EHsc /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\Debug\libapr-1.lib"

"$(OUTDIR)\libapr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr_app - Win32 ReleaseNT"

OUTDIR=.\NT\Release
INTDIR=.\NT\Release
# Begin Custom Macros
OutDir=.\NT\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr_app-1.lib"

!ELSE 

ALL : "libapr - Win32 ReleaseNT" "$(OUTDIR)\libapr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 ReleaseNTCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app-1.idb"
	-@erase "$(INTDIR)\libapr_app-1.pdb"
	-@erase "$(OUTDIR)\libapr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\libapr_app-1" /FD /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\NT\Release\libapr-1.lib"

"$(OUTDIR)\libapr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr_app - Win32 DebugNT"

OUTDIR=.\NT\Debug
INTDIR=.\NT\Debug
# Begin Custom Macros
OutDir=.\NT\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr_app-1.lib"

!ELSE 

ALL : "libapr - Win32 DebugNT" "$(OUTDIR)\libapr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 DebugNTCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app-1.idb"
	-@erase "$(INTDIR)\libapr_app-1.pdb"
	-@erase "$(OUTDIR)\libapr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\libapr_app-1" /FD /EHsc /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\NT\Debug\libapr-1.lib"

"$(OUTDIR)\libapr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr_app - x64 Release"

OUTDIR=.\x64\Release
INTDIR=.\x64\Release
# Begin Custom Macros
OutDir=.\x64\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr_app-1.lib"

!ELSE 

ALL : "libapr - x64 Release" "$(OUTDIR)\libapr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - x64 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app-1.idb"
	-@erase "$(INTDIR)\libapr_app-1.pdb"
	-@erase "$(OUTDIR)\libapr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "NDEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\libapr_app-1" /FD /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\x64\Release\libapr-1.lib"

"$(OUTDIR)\libapr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr_app - x64 Debug"

OUTDIR=.\x64\Debug
INTDIR=.\x64\Debug
# Begin Custom Macros
OutDir=.\x64\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr_app-1.lib"

!ELSE 

ALL : "libapr - x64 Debug" "$(OUTDIR)\libapr_app-1.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - x64 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_app.obj"
	-@erase "$(INTDIR)\internal.obj"
	-@erase "$(INTDIR)\libapr_app-1.idb"
	-@erase "$(INTDIR)\libapr_app-1.pdb"
	-@erase "$(OUTDIR)\libapr_app-1.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "../include" /I "../include/arch" /I "../include/arch/win32" /I "../include/arch/unix" /D "_DEBUG" /D "WINNT" /D "WIN32" /D "_WINDOWS" /D "APR_APP" /Fo"$(INTDIR)\\" /Fd"$(OUTDIR)\libapr_app-1" /FD /EHsc /c 

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
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapr_app-1.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_app.obj" \
	"$(INTDIR)\internal.obj" \
	"..\x64\Debug\libapr-1.lib"

"$(OUTDIR)\libapr_app-1.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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


!IF "$(CFG)" == "libapr_app - Win32 Release" || "$(CFG)" == "libapr_app - Win32 Debug" || "$(CFG)" == "libapr_app - Win32 ReleaseNT" || "$(CFG)" == "libapr_app - Win32 DebugNT" || "$(CFG)" == "libapr_app - x64 Release" || "$(CFG)" == "libapr_app - x64 Debug"

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

!ELSEIF  "$(CFG)" == "libapr_app - Win32 ReleaseNT"

"libapr - Win32 ReleaseNT" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 ReleaseNT" 
   cd ".\build"

"libapr - Win32 ReleaseNTCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 ReleaseNT" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "libapr_app - Win32 DebugNT"

"libapr - Win32 DebugNT" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 DebugNT" 
   cd ".\build"

"libapr - Win32 DebugNTCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 DebugNT" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "libapr_app - x64 Release"

"libapr - x64 Release" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - x64 Release" 
   cd ".\build"

"libapr - x64 ReleaseCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - x64 Release" RECURSE=1 CLEAN 
   cd ".\build"

!ELSEIF  "$(CFG)" == "libapr_app - x64 Debug"

"libapr - x64 Debug" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - x64 Debug" 
   cd ".\build"

"libapr - x64 DebugCLEAN" : 
   cd ".\.."
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - x64 Debug" RECURSE=1 CLEAN 
   cd ".\build"

!ENDIF 

SOURCE=..\misc\win32\apr_app.c

"$(INTDIR)\apr_app.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\misc\win32\internal.c

"$(INTDIR)\internal.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

