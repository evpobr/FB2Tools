// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__149F645E_518F_4EA9_B603_63D633FFB194__INCLUDED_)
#define AFX_STDAFX_H__149F645E_518F_4EA9_B603_63D633FFB194__INCLUDED_

// check for unicode
#ifndef UNICODE
#error This program requires unicode support to run
#endif

// Change these values to use different versions
#define NTDDI_VERSION	0x05010000
#define WINVER		0x0501
#define _WIN32_WINNT	0x0501
#define _WIN32_IE	0x0501

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _ATL_APARTMENT_THREADED

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>
#include <atlstr.h>

#include <shellapi.h>

#include <atlapp.h>

#define _WTL_USE_VSSYM32
#include <atltheme.h>

extern CAppModule _Module;
extern CRegKey	  _Settings;

#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES
#include <atlmisc.h>
#include <atluser.h>

extern CString			  _SettingsPath;

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atlsplit.h>
#include <atlddx.h>

// C library
#include <ctype.h>
#include <time.h>

// msxml
#import <msxml4.dll>

// vb regexps
#import "vbscript3.tlb"

// mshtml additional includes
#include <exdispid.h>
#include <mshtmdid.h>
#include <mshtmcid.h>
#import <shdocvw.dll> rename_namespace("SHD")
#import <mshtml.tlb>

// use com utils
using namespace _com_util;

// extra defines
#ifndef I_IMAGENONE
#define	I_IMAGENONE -1
#endif
#ifndef BTNS_BUTTON
#define	BTNS_BUTTON TBSTYLE_BUTTON
#endif
#ifndef BTNS_AUTOSIZE
#define BTNS_AUTOSIZE TBSTYLE_AUTOSIZE
#endif
#ifndef ODS_HOTLIGHT
#define ODS_HOTLIGHT 0x0040
#endif
#ifndef SPI_GETDROPSHADOW
#define	SPI_GETDROPSHADOW 0x1024
#endif

// scripting support
#include <activscp.h>

int	StartScript(void);
void	StopScript(void);
HRESULT	ScriptLoad(const wchar_t *filename);
HRESULT	ScriptCall(const wchar_t *func,VARIANT *arg,VARIANT *ret);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#if defined _M_IX86

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#elif defined _M_IA64

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#elif defined _M_X64

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#else

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#endif

#endif // !defined(AFX_STDAFX_H__149F645E_518F_4EA9_B603_63D633FFB194__INCLUDED_)
