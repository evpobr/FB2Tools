# Microsoft Developer Studio Project File - Name="FBShell" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=FBShell - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FBShell.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FBShell.mak" CFG="FBShell - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FBShell - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FBShell - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FBShell - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FBSHELL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /I "..\FBE\wtl" /I "..\..\HR Stable\libjpeg" /I "..\..\HR Stable\libpng" /I "..\..\HR Stable\zlib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_USRDLL" /D "STANDALONE" /YX"stdafx.h" /FD /Oxs /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib gdiplus.lib /nologo /dll /machine:I386 /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "FBShell - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FBSHELL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\FBE\wtl" /I "..\..\HR Stable\libjpeg" /I "..\..\HR Stable\libpng" /I "..\..\HR Stable\zlib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_USRDLL" /D "STANDALONE" /YX"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib gdiplus.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "FBShell - Win32 Release"
# Name "FBShell - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ColumnProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\ContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\FBShell.cpp
# End Source File
# Begin Source File

SOURCE=.\FBShell.def
# End Source File
# Begin Source File

SOURCE=.\IconExtractor.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\FBShell.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ColumnProvider.rgs
# End Source File
# Begin Source File

SOURCE=.\ContextMenu.rgs
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\res\FB.ico"
# End Source File
# Begin Source File

SOURCE=.\FBSHell.rc
# End Source File
# Begin Source File

SOURCE=.\IconExtractor.rgs
# End Source File
# Begin Source File

SOURCE=.\NSFolder.rgs
# End Source File
# End Group
# Begin Group "libjpeg sources"

# PROP Default_Filter "c"
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jcomapi.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdapimin.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdapistd.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdcoefct.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdcolor.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jddctmgr.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdhuff.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdinput.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdmainct.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdmarker.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdmaster.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdmerge.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdphuff.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdpostct.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdsample.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdtrans.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jerror.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jidctfst.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jidctint.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jidctred.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jmemmgr.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jmemnobs.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jutils.c"
# End Source File
# End Group
# Begin Group "libpng sources"

# PROP Default_Filter "c"
# Begin Source File

SOURCE="..\..\HR Stable\libpng\png.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngerror.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngget.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngmem.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngpread.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngread.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngrio.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngrtran.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngrutil.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngset.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngtrans.c"
# End Source File
# End Group
# Begin Group "zlib sources"

# PROP Default_Filter "c"
# Begin Source File

SOURCE="..\..\HR Stable\zlib\adler32.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\crc32.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\infblock.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\infcodes.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\inffast.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\inflate.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\inftrees.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\infutil.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\uncompr.c"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\zutil.c"
# End Source File
# End Group
# Begin Group "ImageLib sources"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE="..\..\HR Stable\Image.cpp"
# End Source File
# End Group
# Begin Group "libjpeg headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jconfig.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdct.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jdhuff.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jerror.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jinclude.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jmemsys.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jmorecfg.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jpegint.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jpeglib.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libjpeg\jversion.h"
# End Source File
# End Group
# Begin Group "libpng headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE="..\..\HR Stable\libpng\png.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\libpng\pngconf.h"
# End Source File
# End Group
# Begin Group "zlib headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE="..\..\HR Stable\zlib\infblock.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\infcodes.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\inffast.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\inffixed.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\inftrees.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\infutil.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\zconf.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\zlib.h"
# End Source File
# Begin Source File

SOURCE="..\..\HR Stable\zlib\zutil.h"
# End Source File
# End Group
# Begin Group "ImageLib headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE="..\..\HR Stable\Image.h"
# End Source File
# End Group
# End Target
# End Project
