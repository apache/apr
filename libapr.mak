# Microsoft Developer Studio Generated NMAKE File, Based on libapr.dsp
!IF "$(CFG)" == ""
CFG=libapr - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libapr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libapr - Win32 Release" && "$(CFG)" != "libapr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapr.mak" CFG="libapr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapr - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libapr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libapr - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr.dll"

!ELSE 

ALL : "apr - Win32 Release" "$(OUTDIR)\libapr.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\libapr.idb"
	-@erase "$(INTDIR)\libapr.obj"
	-@erase "$(OUTDIR)\libapr.dll"
	-@erase "$(OUTDIR)\libapr.exp"
	-@erase "$(OUTDIR)\libapr.lib"
	-@erase "$(OUTDIR)\libapr.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "./include" /I "./include/arch" /I\
 "./include/arch/win32" /I "./include/arch/unix" /D "NDEBUG" /D\
 "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\libapr" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo\
 /base:"0x6EE0000" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libapr.pdb" /map:"$(INTDIR)\libapr.map" /machine:I386\
 /def:".\libapr.def" /out:"$(OUTDIR)\libapr.dll" /implib:"$(OUTDIR)\libapr.lib" 
DEF_FILE= \
	".\libapr.def"
LINK32_OBJS= \
	"$(INTDIR)\libapr.obj" \
	".\LibR\apr.lib"

"$(OUTDIR)\libapr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libapr.dll"

!ELSE 

ALL : "apr - Win32 Debug" "$(OUTDIR)\libapr.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\libapr.idb"
	-@erase "$(INTDIR)\libapr.obj"
	-@erase "$(OUTDIR)\libapr.dll"
	-@erase "$(OUTDIR)\libapr.exp"
	-@erase "$(OUTDIR)\libapr.lib"
	-@erase "$(OUTDIR)\libapr.map"
	-@erase "$(OUTDIR)\libapr.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /I "./include" /I "./include/arch" /I\
 "./include/arch/win32" /I "./include/arch/unix" /D "_DEBUG" /D\
 "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\libapr" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo\
 /base:"0x6EE0000" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libapr.pdb" /map:"$(INTDIR)\libapr.map" /debug /machine:I386\
 /def:".\libapr.def" /out:"$(OUTDIR)\libapr.dll" /implib:"$(OUTDIR)\libapr.lib" 
DEF_FILE= \
	".\libapr.def"
LINK32_OBJS= \
	"$(INTDIR)\libapr.obj" \
	".\LibD\apr.lib"

"$(OUTDIR)\libapr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "libapr - Win32 Release" || "$(CFG)" == "libapr - Win32 Debug"

!IF  "$(CFG)" == "libapr - Win32 Release"

"apr - Win32 Release" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" 
   cd "."

"apr - Win32 ReleaseCLEAN" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\apr.mak" CFG="apr - Win32 Release"\
 RECURSE=1 
   cd "."

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

"apr - Win32 Debug" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" 
   cd "."

"apr - Win32 DebugCLEAN" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\apr.mak" CFG="apr - Win32 Debug" RECURSE=1\
 
   cd "."

!ENDIF 

SOURCE=.\misc\win32\libapr.c

"$(INTDIR)\libapr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

