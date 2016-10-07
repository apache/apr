# Microsoft Developer Studio Project File - Name="libapr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libapr - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libapr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapr.mak" CFG="libapr - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapr - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libapr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libapr - x64 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libapr - x64 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libapr - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O2 /Oy- /I "./include" /I "./include/private" /I "./include/arch/win32" /I "./include/arch/unix" /I "../expat/lib" /D "NDEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "WINNT" /D "_WINDOWS" /Fo"$(INTDIR)\" /Fd"$(INTDIR)\libapr_src" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./include" /d "NDEBUG" /d "APR_VERSION_ONLY"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug /opt:ref
# ADD LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib libexpat.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug /libpath:"..\expat\win32\bin\Release" /out:"Release\libapr-2.dll" /pdb:"Release\libapr-2.pdb" /implib:"Release\libapr-2.lib" /MACHINE:X86 /opt:ref
# Begin Special Build Tool
TargetPath=Release\libapr-2.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
PostBuild_Cmds=if exist $(TargetPath).manifest mt.exe -manifest $(TargetPath).manifest -outputresource:$(TargetPath);2
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /EHsc /c
# ADD CPP /nologo /MDd /W3 /Zi /Od /I "./include" /I "./include/private" /I "./include/arch/win32" /I "./include/arch/unix" /I "../expat/lib" /D "_DEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "WINNT" /D "_WINDOWS" /Fo"$(INTDIR)\" /Fd"$(INTDIR)\libapr_src" /FD /EHsc /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "./include" /d "_DEBUG" /d "APR_VERSION_ONLY"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug
# ADD LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib libexpat.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug /libpath:"..\expat\win32\bin\Debug" /out:"Debug\libapr-2.dll" /pdb:"Debug\libapr-2.pdb" /implib:"Debug\libapr-2.lib" /MACHINE:X86
# Begin Special Build Tool
TargetPath=Debug\libapr-2.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
PostBuild_Cmds=if exist $(TargetPath).manifest mt.exe -manifest $(TargetPath).manifest -outputresource:$(TargetPath);2
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libapr - x64 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x64\Release"
# PROP BASE Intermediate_Dir "x64\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x64\Release"
# PROP Intermediate_Dir "x64\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O2 /Oy- /I "./include" /I "./include/private" /I "./include/arch/win32" /I "./include/arch/unix" /I "../expat/lib" /D "NDEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "WINNT" /D "_WINDOWS" /Fo"$(INTDIR)\" /Fd"$(INTDIR)\libapr_src" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./include" /d "NDEBUG" /d "APR_VERSION_ONLY"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug /opt:ref
# ADD LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib libexpat.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug /libpath:"..\expat\win32\bin\x64\Release" /out:"x64\Release\libapr-2.dll" /pdb:"x64\Release\libapr-2.pdb" /implib:"x64\Release\libapr-2.lib" /MACHINE:X64 /opt:ref
# Begin Special Build Tool
TargetPath=x64\Release\libapr-2.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
PostBuild_Cmds=if exist $(TargetPath).manifest mt.exe -manifest $(TargetPath).manifest -outputresource:$(TargetPath);2
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libapr - x64 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x64\Debug"
# PROP BASE Intermediate_Dir "x64\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x64\Debug"
# PROP Intermediate_Dir "x64\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /EHsc /c
# ADD CPP /nologo /MDd /W3 /Zi /Od /I "./include" /I "./include/private" /I "./include/arch/win32" /I "./include/arch/unix" /I "../expat/lib" /D "_DEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "WINNT" /D "_WINDOWS" /Fo"$(INTDIR)\" /Fd"$(INTDIR)\libapr_src" /FD /EHsc /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "./include" /d "_DEBUG" /d "APR_VERSION_ONLY"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug
# ADD LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib shell32.lib rpcrt4.lib libexpat.lib /nologo /base:"0x6EEC0000" /subsystem:windows /dll /incremental:no /debug /libpath:"..\expat\win32\bin\x64\Debug" /out:"x64\Debug\libapr-2.dll" /pdb:"x64\Debug\libapr-2.pdb" /implib:"x64\Debug\libapr-2.lib" /MACHINE:X64
# Begin Special Build Tool
TargetPath=x64\Debug\libapr-2.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
PostBuild_Cmds=if exist $(TargetPath).manifest mt.exe -manifest $(TargetPath).manifest -outputresource:$(TargetPath);2
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libapr - Win32 Release"
# Name "libapr - Win32 Debug"
# Name "libapr - x64 Release"
# Name "libapr - x64 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ".c"
# Begin Group "atomic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\atomic\win32\apr_atomic.c
# End Source File
# End Group
# Begin Group "buckets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\buckets\apr_brigade.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_alloc.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_eos.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_file.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_flush.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_heap.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_mmap.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_pipe.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_pool.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_refcount.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_simple.c
# End Source File
# Begin Source File

SOURCE=.\buckets\apr_buckets_socket.c
# End Source File
# End Group
# Begin Group "crypto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\crypto\apr_crypto.c
# End Source File
# Begin Source File

SOURCE=.\crypto\apr_md4.c
# End Source File
# Begin Source File

SOURCE=.\crypto\apr_md5.c
# End Source File
# Begin Source File

SOURCE=.\crypto\apr_passwd.c
# End Source File
# Begin Source File

SOURCE=.\crypto\apr_sha1.c
# End Source File
# Begin Source File

SOURCE=.\crypto\apr_siphash.c
# End Source File
# Begin Source File

SOURCE=.\crypto\crypt_blowfish.c
# End Source File
# Begin Source File

SOURCE=.\crypto\crypt_blowfish.h
# End Source File
# Begin Source File

SOURCE=.\crypto\getuuid.c
# End Source File
# Begin Source File

SOURCE=.\crypto\uuid.c
# End Source File
# End Group
# Begin Group "dbd"
# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dbd\apr_dbd.c
# End Source File
# Begin Source File

SOURCE=.\dbd\apr_dbd_mysql.c
# End Source File
# Begin Source File

SOURCE=.\dbd\apr_dbd_odbc.c
# End Source File
# Begin Source File

SOURCE=.\dbd\apr_dbd_oracle.c
# End Source File
# Begin Source File

SOURCE=.\dbd\apr_dbd_pgsql.c
# End Source File
# Begin Source File

SOURCE=.\dbd\apr_dbd_sqlite2.c
# End Source File
# Begin Source File

SOURCE=.\dbd\apr_dbd_sqlite3.c
# End Source File
# End Group
# Begin Group "dbm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dbm\apr_dbm.c
# End Source File
# Begin Source File

SOURCE=.\dbm\apr_dbm_berkeleydb.c
# End Source File
# Begin Source File

SOURCE=.\dbm\apr_dbm_gdbm.c
# End Source File
# Begin Source File

SOURCE=.\dbm\apr_dbm_sdbm.c
# End Source File
# End Group
# Begin Group "dso"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dso\win32\dso.c
# End Source File
# End Group
# Begin Group "encoding"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\encoding\apr_base64.c
# End Source File
# Begin Source File

SOURCE=.\encoding\apr_escape.c
# End Source File
# End Group
# Begin Group "file_io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\file_io\win32\buffer.c
# End Source File
# Begin Source File

SOURCE=.\file_io\unix\copy.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\dir.c
# End Source File
# Begin Source File

SOURCE=.\file_io\unix\fileacc.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\filedup.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\filepath.c
# End Source File
# Begin Source File

SOURCE=.\file_io\unix\filepath_util.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\filestat.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\filesys.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\flock.c
# End Source File
# Begin Source File

SOURCE=.\file_io\unix\fullrw.c
# End Source File
# Begin Source File

SOURCE=.\file_io\unix\mktemp.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\open.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\pipe.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\readwrite.c
# End Source File
# Begin Source File

SOURCE=.\file_io\win32\seek.c
# End Source File
# Begin Source File

SOURCE=.\file_io\unix\tempdir.c
# End Source File
# End Group
# Begin Group "hooks"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hooks\apr_hooks.c
# End Source File
# End Group
# Begin Group "locks"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\locks\win32\proc_mutex.c
# End Source File
# Begin Source File

SOURCE=.\locks\win32\thread_cond.c
# End Source File
# Begin Source File

SOURCE=.\locks\win32\thread_mutex.c
# End Source File
# Begin Source File

SOURCE=.\locks\win32\thread_rwlock.c
# End Source File
# End Group
# Begin Group "memcache"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\memcache\apr_memcache.c
# End Source File
# End Group
# Begin Group "memory"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\memory\unix\apr_pools.c
# End Source File
# End Group
# Begin Group "misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\misc\win32\apr_app.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\misc\win32\charset.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\env.c
# End Source File
# Begin Source File

SOURCE=.\misc\unix\errorcodes.c
# End Source File
# Begin Source File

SOURCE=.\misc\unix\getopt.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\internal.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\misc.c
# End Source File
# Begin Source File

SOURCE=.\misc\unix\otherchild.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\rand.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\start.c
# End Source File
# Begin Source File

SOURCE=.\misc\win32\utf8.c
# End Source File
# Begin Source File

SOURCE=.\misc\unix\version.c
# End Source File
# End Group
# Begin Group "mmap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mmap\unix\common.c
# End Source File
# Begin Source File

SOURCE=.\mmap\win32\mmap.c
# End Source File
# End Group
# Begin Group "network_io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\network_io\unix\inet_ntop.c
# End Source File
# Begin Source File

SOURCE=.\network_io\unix\inet_pton.c
# End Source File
# Begin Source File

SOURCE=.\network_io\unix\multicast.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sendrecv.c
# End Source File
# Begin Source File

SOURCE=.\network_io\unix\sockaddr.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sockets.c
# End Source File
# Begin Source File

SOURCE=.\network_io\unix\socket_util.c
# End Source File
# Begin Source File

SOURCE=.\network_io\win32\sockopt.c
# End Source File
# End Group
# Begin Group "passwd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\passwd\apr_getpass.c
# End Source File
# End Group
# Begin Group "poll"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\poll\unix\pollcb.c
# End Source File
# Begin Source File

SOURCE=.\poll\unix\pollset.c
# End Source File
# Begin Source File

SOURCE=.\poll\unix\poll.c
# End Source File
# Begin Source File

SOURCE=.\poll\unix\select.c
# End Source File
# Begin Source File

SOURCE=.\poll\unix\wakeup.c
# End Source File
# End Group
# Begin Group "random"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\random\unix\apr_random.c
# End Source File
# Begin Source File

SOURCE=.\random\unix\sha2.c
# End Source File
# Begin Source File

SOURCE=.\random\unix\sha2_glue.c
# End Source File
# End Group
# Begin Group "sdbm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm.c
# End Source File
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm_hash.c
# End Source File
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm_lock.c
# End Source File
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm_pair.c
# End Source File
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm_pair.h
# End Source File
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm_private.h
# End Source File
# Begin Source File

SOURCE=.\dbm\sdbm\sdbm_tune.h
# End Source File
# End Group
# Begin Group "shmem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\shmem\win32\shm.c
# End Source File
# End Group
# Begin Group "strings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\strings\apr_cpystrn.c
# End Source File
# Begin Source File

SOURCE=.\strings\apr_fnmatch.c
# End Source File
# Begin Source File

SOURCE=.\strings\apr_snprintf.c
# End Source File
# Begin Source File

SOURCE=.\strings\apr_strings.c
# End Source File
# Begin Source File

SOURCE=.\strings\apr_strnatcmp.c
# End Source File
# Begin Source File

SOURCE=.\strings\apr_strtok.c
# End Source File
# End Group
# Begin Group "strmatch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\strmatch\apr_strmatch.c
# End Source File
# End Group
# Begin Group "tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tables\apr_hash.c
# Begin Source File

SOURCE=.\tables\apr_tables.c
# End Source File
# Begin Source File

SOURCE=.\tables\apr_skiplist.c
# End Source File
# End Group
# Begin Group "threadproc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\threadproc\win32\proc.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\signals.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\thread.c
# End Source File
# Begin Source File

SOURCE=.\threadproc\win32\threadpriv.c
# End Source File
# End Group
# Begin Group "time"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\time\win32\time.c
# End Source File
# Begin Source File

SOURCE=.\time\win32\timestr.c
# End Source File
# End Group
# Begin Group "uri"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\uri\apr_uri.c
# End Source File
# End Group
# Begin Group "user"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\user\win32\groupinfo.c
# End Source File
# Begin Source File

SOURCE=.\user\win32\userinfo.c
# End Source File
# End Group
# Begin Group "util-misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\util-misc\apr_date.c
# End Source File
# Begin Source File

SOURCE=.\util-misc\apu_dso.c
# End Source File
# Begin Source File

SOURCE=.\util-misc\apr_queue.c
# End Source File
# Begin Source File

SOURCE=.\util-misc\apr_reslist.c
# End Source File
# Begin Source File

SOURCE=.\util-misc\apr_rmm.c
# End Source File
# Begin Source File

SOURCE=.\util-misc\apr_thread_pool.c
# End Source File
# End Group
# Begin Group "xlate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\xlate\xlate.c
# End Source File
# End Group
# Begin Group "xml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\xml\apr_xml.c
# End Source File
# Begin Source File

SOURCE=.\xml\apr_xml_expat.c
# End Source File
# End Group
# End Group
# Begin Group "Private Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_atime.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_dso.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_file_io.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_inherit.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_misc.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_networkio.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_thread_mutex.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_thread_rwlock.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_threadproc.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_arch_utf8.h
# End Source File
# Begin Source File

SOURCE=.\include\arch\win32\apr_private.h
# End Source File
# End Group
# Begin Group "Public Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\apr.h.in
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\include\apr.hnw
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\include\apr.hw

!IF  "$(CFG)" == "libapr - Win32 Release"

# Begin Custom Build - Creating apr.h from apr.hw, apr_escape_test_char.h from gen_test_char.exe
InputPath=.\include\apr.hw

".\include\apr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apr.hw > .\include\apr.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

# Begin Custom Build - Creating apr.h from apr.hw, apr_escape_test_char.h from gen_test_char.exe
InputPath=.\include\apr.hw

".\include\apr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apr.hw > .\include\apr.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Release"

# Begin Custom Build - Creating apr.h from apr.hw, apr_escape_test_char.h from gen_test_char.exe
InputPath=.\include\apr.hw

".\include\apr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apr.hw > .\include\apr.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Debug"

# Begin Custom Build - Creating apr.h from apr.hw, apr_escape_test_char.h from gen_test_char.exe
InputPath=.\include\apr.hw

".\include\apr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apr.hw > .\include\apr.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\include\apr_allocator.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_atomic.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_dso.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_env.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_errno.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_escape.h

!IF  "$(CFG)" == "libapr - Win32 Release"

# Begin Custom Build - Creating gen_test_char.exe and apr_escape_test_char.h
InputPath=.\include\apr_escape.h

".\include\apr_escape_test_char.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl.exe /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /I ".\include" /Fo.\Release\gen_test_char /Fe.\Release\gen_test_char.exe .\tools\gen_test_char.c 
	.\Release\gen_test_char.exe > .\include\apr_escape_test_char.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

# Begin Custom Build - Creating gen_test_char.exe and apr_escape_test_char.h
InputPath=.\include\apr_escape.h

".\include\apr_escape_test_char.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl.exe /nologo /W3 /EHsc /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /I ".\include" /Fo.\Debug\gen_test_char /Fe.\Debug\gen_test_char.exe .\tools\gen_test_char.c  
	.\Debug\gen_test_char.exe > .\include\apr_escape_test_char.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Release"

# Begin Custom Build - Creating gen_test_char.exe and apr_escape_test_char.h
InputPath=.\include\apr_escape.h

".\include\apr_escape_test_char.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl.exe /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /I ".\include" /Fo.\x64\Release\gen_test_char /Fe.\x64\Release\gen_test_char.exe .\tools\gen_test_char.c 
	.\x64\Release\gen_test_char.exe > .\include\apr_escape_test_char.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Debug"

# Begin Custom Build - Creating gen_test_char.exe and apr_escape_test_char.h
InputPath=.\include\apr_escape.h

".\include\apr_escape_test_char.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl.exe /nologo /W3 /EHsc /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /I ".\include" /Fo.\x64\Debug\gen_test_char /Fe.\x64\Debug\gen_test_char.exe .\tools\gen_test_char.c 
	.\x64\Debug\gen_test_char.exe > .\include\apr_escape_test_char.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\include\apr_file_info.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_file_io.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_fnmatch.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_general.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_getopt.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_global_mutex.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_hash.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_inherit.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_lib.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_mmap.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_network_io.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_poll.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_pools.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_portable.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_proc_mutex.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_random.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_ring.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_shm.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_signal.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_skiplist.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_strings.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_support.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_tables.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_thread_cond.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_thread_mutex.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_thread_proc.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_thread_rwlock.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_time.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_user.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_version.h
# End Source File
# Begin Source File

SOURCE=.\include\apr_want.h
# End Source File
# Begin Source File

SOURCE=.\include\apu.h
# End Source File
# Begin Source File

SOURCE=.\include\private\apu_select_dbm.h.in
# End Source File
# Begin Source File

SOURCE=.\include\private\apu_select_dbm.hw

!IF  "$(CFG)" == "libapr - Win32 Release"

# Begin Custom Build - Creating apu_select_dbm.h from apu_select_dbm.hw
InputPath=.\include\private\apu_select_dbm.hw

".\include\private\apu_select_dbm.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\private\apu_select_dbm.hw > .\include\private\apu_select_dbm.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

# Begin Custom Build - Creating apu_select_dbm.h from apu_select_dbm.hw
InputPath=.\include\private\apu_select_dbm.hw

".\include\private\apu_select_dbm.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\private\apu_select_dbm.hw > .\include\private\apu_select_dbm.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Release"

# Begin Custom Build - Creating apu_select_dbm.h from apu_select_dbm.hw
InputPath=.\include\private\apu_select_dbm.hw

".\include\private\apu_select_dbm.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\private\apu_select_dbm.hw > .\include\private\apu_select_dbm.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Debug"

# Begin Custom Build - Creating apu_select_dbm.h from apu_select_dbm.hw
InputPath=.\include\private\apu_select_dbm.hw

".\include\private\apu_select_dbm.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\private\apu_select_dbm.hw > .\include\private\apu_select_dbm.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\include\apu_want.h.in
# End Source File
# Begin Source File

SOURCE=.\include\apu_want.hnw
# End Source File
# Begin Source File

SOURCE=.\include\apu_want.hw

!IF  "$(CFG)" == "libapr - Win32 Release"

# Begin Custom Build - Creating apu_want.h from apu_want.hw
InputPath=.\include\apu_want.hw

".\include\apu_want.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apu_want.hw > .\include\apu_want.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - Win32 Debug"

# Begin Custom Build - Creating apu_want.h from apu_want.hw
InputPath=.\include\apu_want.hw

".\include\apu_want.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apu_want.hw > .\include\apu_want.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Release"

# Begin Custom Build - Creating apu_want.h from apu_want.hw
InputPath=.\include\apu_want.hw

".\include\apu_want.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apu_want.hw > .\include\apu_want.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libapr - x64 Debug"

# Begin Custom Build - Creating apu_want.h from apu_want.hw
InputPath=.\include\apu_want.hw

".\include\apu_want.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	type .\include\apu_want.hw > .\include\apu_want.h

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\libapr.rc
# End Source File
# End Target
# End Project
