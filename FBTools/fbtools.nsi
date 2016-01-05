; Default names
!ifndef NAME
!define NAME "FictionBook Tools"
!endif
!ifndef NSPNAME
!define NSPNAME "FictionBookTools"
!endif
!ifndef VENDOR
!define VENDOR "Haali"
!endif
!ifndef VERSION
!define VERSION "2.0"
!endif

Name "${NAME}"

SetCompressor lzma

; Use the new UI
!include "MUI.nsh"

; MUI settings
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\win-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\win-uninstall.ico"

; The file to write
OutFile "${NAME} v${VERSION} Setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\${VENDOR}\${NAME}"
InstallDirRegKey HKLM "SOFTWARE\${VENDOR}\${NSPNAME}" "InstallDir"

; Insert MUI
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Function CheckMSXMLVersion
  ReadRegStr $0 HKCR "Msxml2.SAXXMLReader.6.0\CLSID" ""
  StrCmp $0 "" noxml
  ReadRegStr $1 HKCR "CLSID\$0\ProgID" ""
  StrCmp $1 "Msxml2.SAXXMLReader.6.0" ok
noxml:
  MessageBox MB_OK|MB_ICONSTOP "This application requires Microsoft Core XML Services (MSXML) 6.0 or newer to run."
  Quit
ok:
FunctionEnd  

Function CheckIEVersion
  GetDllVersion "shdocvw.dll" $0 $1
  IntCmp $0 327730 ok 0 ok
  MessageBox MB_OK|MB_ICONSTOP "This application requires Internet Expolorer version 5.5 or newer to run."
  Quit
ok:
FunctionEnd

Function CheckFBERunning
  FindWindow $0 "FictionBookEditorFrame"
  IntCmp $0 0 ok1
  MessageBox MB_OK|MB_ICONSTOP "Please exit FictionBook Editor before running this installation."
  Quit
ok1:
  FindWindow $0 "" "FictionBook Validator"
  IntCmp $0 0 ok2
  MessageBox MB_OK|MB_ICONSTOP "Please exit FictionBook Validator before running this installation."
  Quit
ok2:
FunctionEnd
Function un.CheckFBERunning
  FindWindow $0 "FictionBookEditorFrame"
  IntCmp $0 0 ok1
  MessageBox MB_OK|MB_ICONSTOP "Please exit FictionBook Editor before running this installation."
  Quit
ok1:
  FindWindow $0 "" "FictionBook Validator"
  IntCmp $0 0 ok2
  MessageBox MB_OK|MB_ICONSTOP "Please exit FictionBook Validator before running this installation."
  Quit
ok2:
FunctionEnd

!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0

; first section, initialize
Section ""
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 nthere
  MessageBox MB_OK|MB_ICONSTOP "FictionBook Tools run only on Windows NT."
  Quit
nthere:

  Call CheckMSXMLVersion
  Call CheckIEVersion
  Call CheckFBERunning

  SetOutPath $INSTDIR

  ; create an FB2 progid
  WriteRegStr HKCR "FictionBook.2" "" "FictionBook"
  WriteRegStr HKCR "FictionBook.2\CurVer" "" "FictionBook.2"

  ; create an FB2 filetype
  WriteRegStr HKCR ".fb2" "" "FictionBook.2"
  WriteRegStr HKCR ".fb2" "PerceivedType" "Text"
  WriteRegStr HKCR ".fb2" "Content Type" "text/xml"

  ; install the shell extension, this might be in use
  IfFileExists 0 nodll
  ClearErrors
  Delete "$INSTDIR\FBShell.dll"
  IfErrors 0 nodll
  ; delete failed, assume destination was in use
  UnRegDll "$INSTDIR\FBShell.dll"
  MessageBox MB_OK|MB_ICONSTOP "FB shell intergation dll was in use and could not be overwritten.$\r$\nPlease logout and logon again, and restart this installation."
  Quit
nodll:
  File "..\Release\FBShell.dll"

  ; m$' runtime lib
  ; File "C:\WINDOWS\System32\msvcr71.dll"

  RegDll "$INSTDIR\FBShell.dll"

  ; fictionbook validator
  File "..\Release\FBV.exe"
  File "..\Release\FictionBook.xsd"
  File "..\Release\FictionBookLinks.xsd"
  File "..\Release\FictionBookLang.xsd"
  File "..\Release\FictionBookGenres.xsd"

  ; editor
  File "..\Release\FBE.exe"
  File "..\Release\SciLexer.dll"
  File "..\Release\body.xsl"
  File "..\Release\body.js"
  File "..\Release\description.xsl"
  File "..\Release\desc.js"
  File "..\Release\genres.txt"
  File "..\Release\ExportHTML.dll"
  File "..\Release\html.xsl"
  File "..\Release\imgph.png"
  File "..\Release\demo.js"

  CreateDirectory "$SMPROGRAMS\${NAME}"
  CreateShortCut "$SMPROGRAMS\${NAME}\FictionBook Editor.lnk" "$INSTDIR\FBE.exe" "" "$INSTDIR\FBE.exe" 0

  ; register application
  WriteRegStr HKCR "Applications\FBE.exe" "FriendlyAppName" "FictionBook Editor"
  WriteRegStr HKCR "Applications\FBE.exe\SupportedTypes" ".fb2" ""
  WriteRegStr HKCR "Applications\FBE.exe\SupportedTypes" ".xml" ""
  WriteRegStr HKCR "Applications\FBE.exe\shell\Open\Command" "" '"$INSTDIR\FBE.exe" "%1"'
  WriteRegStr HKCR "Applications\FBE.exe\shell\Edit\Command" "" '"$INSTDIR\FBE.exe" "%1"'
  WriteRegStr HKLM "Software\Microsoft\IE Setup\DependentComponents" "FictionBook Editor" "5.5"
  RegDll "$INSTDIR\ExportHTML.dll"

  ; register verb
  WriteRegStr HKCR "FictionBook.2\shell\Edit\Command" "" '"$INSTDIR\FBE.exe" "%1"'

  ; refresh icons
  System::Call 'shell32.dll::SHChangeNotify(l, l, i, i) v (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'

  ; Uninstall shortcut
  CreateShortCut "$SMPROGRAMS\${NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\${VENDOR}\${NSPNAME}" "InstallDir" "$INSTDIR"
  ; Uninstall info
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${VENDOR}${NSPNAME}" "DisplayName" "${VENDOR} ${NAME} ${VERSION} (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${VENDOR}${NSPNAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  ; uninstall program
  WriteUninstaller "uninstall.exe"
SectionEnd

; Uninstall support
Section "Uninstall"
  Call un.CheckFBERunning

  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${VENDOR}${NSPNAME}"
  DeleteRegKey HKLM "SOFTWARE\${VENDOR}\${NSPNAME}"
  DeleteRegValue HKLM "Software\Microsoft\IE Setup\DependentComponents" "FictionBook Editor"

  ; remove shell extension
  UnRegDll "$INSTDIR\FBShell.dll"

  ; remove plugin
  UnRegDll "$INSTDIR\ExportHTML.dll"

  ; remove applications
  DeleteRegKey HKCR "Applications\FBE.exe"

  ; remove verbs; TODO: check if these really point to FBE
  DeleteRegKey HKCR "FictionBook.2\shell\Edit"

  ; remove files
  Delete "$INSTDIR\FBShell.dll"

  Delete "$INSTDIR\FBV.exe"

  Delete "$INSTDIR\ExportHTML.dll"
  Delete "$INSTDIR\html.xsl"

  Delete "$INSTDIR\FBE.exe"
  Delete "$INSTDIR\SciLexer.dll"
  Delete "$INSTDIR\genres.txt"
  Delete "$INSTDIR\body.xsl"
  Delete "$INSTDIR\body.js"
  Delete "$INSTDIR\description.xsl"
  Delete "$INSTDIR\desc.js"
  Delete "$INSTDIR\imgph.png"

  Delete "$INSTDIR\FictionBook.xsd"
  Delete "$INSTDIR\FictionBookLinks.xsd"

  ; MUST REMOVE UNINSTALLER, too
  Delete "$INSTDIR\uninstall.exe"

  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\${NAME}\*.*"

  ; remove directories used.
  RMDir "$SMPROGRAMS\${NAME}"
  RMDir "$INSTDIR"

  ; refresh icons
  System::Call 'shell32.dll::SHChangeNotify(l, l, i, i) v (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
SectionEnd

; eof
