// FBE.cpp : main source file for FBE.exe
//

#include "stdafx.h"
#include <locale.h>

#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include "FBEView.h"
#include "FBDoc.h"
#include "TreeView.h"
#include "ContainerWnd.h"
#include "Scintilla.h"
#include "MainFrm.h"
#include "MemProtocol.h"

// typelib interfaces
#include "FBE.h"

// implementation
#include "ExternalHelper.h"

// typelib guids
#include "fbe_i.c"

#define DEFINE_CLSID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const CLSID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

// {7301FF90-9029-4819-B778-19D9999DB419}
DEFINE_CLSID(CLSID_MemProtocol, 
0x7301ff90, 0x9029, 0x4819, 0xb7, 0x78, 0x19, 0xd9, 0x99, 0x9d, 0xb4, 0x19);


CAppModule	      _Module;
BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_MemProtocol, CMemProtocol)
END_OBJECT_MAP()

CRegKey		      _Settings;
CString		      _SettingsPath;
CSimpleArray<CString> _ARGV;

// external helpers
IDispatchPtr  CFBEView::CreateHelper() {
  CComObject<ExternalHelper>	  *obj;
  if (FAILED(CComObject<ExternalHelper>::CreateInstance(&obj)))
    obj=NULL;
  return obj;
}

// command line parser
static void	ParseCommandLine(LPTSTR cmd,CSimpleArray<CString>& args) {
  TCHAR	  *p=cmd;
  int	  len=_tcslen(p);
  TCHAR	  *e=p+len;

  for (;;) {
    // skip ws
    while (p<e && (unsigned)*p<=32)
      ++p;
    if (p>=e)
      break;

    // process argument
    CString   arg;
    TCHAR     *buf=arg.GetBuffer(e-p);
    TCHAR     *q=buf;
    bool      fQuote=false;
    while (p<e) {
      if (fQuote) {
	if (*p==_T('"')) { // possible end of arg
	  if (p+1<e && p[1]==_T('"')) {// literal quote
	    *q++=_T('"');
	    ++p;
	  } else
	    fQuote=false;
	} else
	  *q++=*p; // normal char
      } else {
	if (*p<=32) // end of arg
	  break;
	if (*p==_T('"')) // quoted part
	  fQuote=true;
	else // normal text
	  *q++=*p;
      }
      ++p;
    }
    arg.ReleaseBuffer(q-buf);
    args.Add(arg);
  }
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
  CMessageLoop theLoop;
  _Module.AddMessageLoop(&theLoop);
  
  CMainFrame wndMain;
  
  if(wndMain.CreateEx() == NULL)
  {
    ATLTRACE(_T("Main window creation failed!\n"));
    return 0;
  }
 
  WINDOWPLACEMENT   wpl;
  DWORD		    sz=sizeof(wpl);
  DWORD		    type=REG_BINARY;
  if (::RegQueryValueEx(_Settings,_T("WindowPosition"),0,&type,(BYTE*)&wpl,&sz)==ERROR_SUCCESS &&
      type==REG_BINARY && sz==sizeof(wpl) && wpl.length==sizeof(wpl))
    wndMain.SetWindowPlacement(&wpl);
  else
    wndMain.ShowWindow(nCmdShow);

  int nRet = theLoop.Run();
  
  _Module.RemoveMessageLoop();
  return nRet;
}

bool  LoadEditor() {
  HINSTANCE hLib=::LoadLibrary(_T("SciLexer.dll"));
  if (hLib==NULL)
    return false;
#if 0
  bool	(*scireg)(void *hinst)=(bool (*)(void*))::GetProcAddress(hLib,"Scintilla_RegisterClasses");
  if (scireg==NULL || !scireg(_Module.GetModuleInstance())) {
    ::FreeLibrary(hLib);
    return false;
  }
#endif
  return true;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
  int nRet=1;

#if 1
#ifdef _DEBUG
  _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
  _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
  _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
#endif
#endif

  // initialize RNG
  srand(time(NULL));

  // switch to user's locale
  setlocale(LC_CTYPE,"");
  setlocale(LC_COLLATE,"");

  // initialize COM/OLE
  HRESULT hRes = ::OleInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));
  
  // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
  ::DefWindowProc(NULL, 0, 0, 0L);

  AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

  // init module
  hRes = _Module.Init(ObjectMap, hInstance, &LIBID_FBELib);
  ATLASSERT(SUCCEEDED(hRes));

  // register typelib
  hRes = _Module.RegisterTypeLib();
  ATLASSERT(SUCCEEDED(hRes));

  // enable web browser hosting
  AtlAxWinInit();

  // initialize registry settings
  U::InitSettings();

  // parse command line
  ParseCommandLine(lpstrCmdLine,_ARGV);
  if (!AU::ParseCmdLineArgs())
    goto out;
  
  // load xml source editor
  if (!LoadEditor()) {
    ::MessageBox(NULL,_T("Can't load Scintilla."),_T("Error"),MB_OK|MB_ICONERROR);
    goto out;
  }

  // register our protocol handler
  IInternetSession    *isess;
  if (SUCCEEDED(::CoInternetGetSession(0,&isess,0))) {
    IClassFactory *cf;
    if (SUCCEEDED(_Module.GetClassObject(CLSID_MemProtocol,IID_IClassFactory,(void**)&cf))) {
      HRESULT hr=isess->RegisterNameSpace(cf,CLSID_MemProtocol,L"fbe-internal",0,NULL,0);
      if (FAILED(hr))
	ATLTRACE("Failed to register protocol handler: %x\n",hr);
      cf->Release();
    }
    isess->Release();
  }

  // run the main loop
  nRet = Run(lpstrCmdLine, nCmdShow);

out:
  _Module.Term();

  ::OleUninitialize();
  
  return nRet;
}
