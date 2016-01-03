// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__AA787312_D02C_4332_A4DD_1B1AA8B9E8BF__INCLUDED_)
#define AFX_STDAFX_H__AA787312_D02C_4332_A4DD_1B1AA8B9E8BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// check for unicode
#ifndef UNICODE
#error This program requires unicode support to run
#endif

#define WINVER       0x0501     // W2K/98
#define _WIN32_WINNT 0x0501     // W2K
#define _WIN32_IE    0x0500     // IE 5+

// we are MT by default
#define _ATL_APARTMENT_THREADED

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>

#include <atlcom.h>

#include <comutil.h>
#include <comdef.h>

#import <msxml4.dll>

#include <shellapi.h>
#include <shlobj.h>
#include <shlguid.h>

#include <atlimage.h>

using namespace _com_util;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__AA787312_D02C_4332_A4DD_1B1AA8B9E8BF__INCLUDED_)

