#include "stdafx.h"

#define	MAXARGS	32

#include <initguid.h>
DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0,
   0xaa, 0x0, 0x4a, 0x55, 0xe8);
DEFINE_GUID(CLSID_JScript, 0xf414c260, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00,
   0xaa, 0x00, 0xbb, 0xbb, 0x58);

#define	EMSG(x)	    MessageBox(GetActiveWindow(),x,_T("FBE Script"),MB_ICONERROR|MB_OK)

static void	strlcatW(wchar_t *d,const wchar_t *s,int dl) {
  int l;

  if (dl<=0)
    return;

  l = wcslen(d);
  if (l>=dl)
    return;

  --dl;

  while (l < dl && *s)
    d[l++] = *s++;

  d[l] = 0;
}

static inline void  *::operator new(size_t amount) {
  return ::HeapAlloc(::GetProcessHeap(),0,amount);
}

static inline void  ::operator delete(void *p) {
  ::HeapFree(::GetProcessHeap(),0,p);
}

static bool EQ(REFIID i1,REFIID i2) {
  char	*c1 = (char *)&i1;
  char	*c2 = (char *)&i2;
  for (int i=0;i<sizeof(i1);++i)
    if (*c1++ != *c2++)
      return false;
  return true;
}

// script callback functions
static HRESULT	sLoad(VARIANT *filename) {
  if (!filename || V_VT(filename) != VT_BSTR)
    return DISP_E_TYPEMISMATCH;

  return ScriptLoad(V_BSTR(filename));
}

static HRESULT	sMessage(VARIANT *msg) {
  VARIANT   vm;

  VariantInit(&vm);

  if (FAILED(VariantChangeType(&vm,msg,0,VT_BSTR)))
    return DISP_E_TYPEMISMATCH;

  MessageBoxW(GetActiveWindow(),V_BSTR(&vm),_T("FBE Script message"),MB_ICONINFORMATION|MB_OK);

  VariantClear(&vm);

  return S_OK;
}

static HRESULT	sGetClassName(VARIANT *w,VARIANT *result) {
  if (!result)
    return E_INVALIDARG;

  VARIANT   vi;
  VariantInit(&vi);

  if (FAILED(VariantChangeType(&vi,w,0,VT_UI4)))
    return DISP_E_TYPEMISMATCH;

  wchar_t name[1024];
  name[0] = 0;
  GetClassNameW((HWND)V_UI4(&vi),name,sizeof(name)/sizeof(name[0]));
  V_VT(result) = VT_BSTR;
  V_BSTR(result) = SysAllocString(name);

  return S_OK;
}

static HRESULT	sGetWindowText(VARIANT *w,VARIANT *result) {
  if (!result)
    return E_INVALIDARG;

  VARIANT   vi;
  VariantInit(&vi);

  if (FAILED(VariantChangeType(&vi,w,0,VT_UI4)))
    return DISP_E_TYPEMISMATCH;

  wchar_t name[1024];
  name[0] = 0;
  GetWindowTextW((HWND)V_UI4(&vi),name,sizeof(name)/sizeof(name[0]));
  V_VT(result) = VT_BSTR;
  V_BSTR(result) = SysAllocString(name);

  return S_OK;
}

static HRESULT	sRunProgram(VARIANT *path,VARIANT *args,VARIANT *result) {
  VARIANT vp,va;

  VariantInit(&vp);
  VariantInit(&va);
  if (FAILED(VariantChangeType(&vp,path,0,VT_BSTR)))
    return DISP_E_TYPEMISMATCH;

  if (args && FAILED(VariantChangeType(&va,args,0,VT_BSTR))) {
    VariantClear(&vp);
    return DISP_E_TYPEMISMATCH;
  }

  DWORD v = (DWORD)ShellExecuteW(GetActiveWindow(),NULL,V_BSTR(&vp),args ? V_BSTR(&va) : NULL,L"C:\\",SW_SHOWNORMAL);

  if (result) {
    V_VT(result) = VT_UI4;
    V_UI4(result) = v >= 32;
  }

  VariantClear(&vp);
  VariantClear(&va);

  return S_OK;
}

static HRESULT	sGetWindowPID(VARIANT *w,VARIANT *result) {
  VARIANT vp;

  VariantInit(&vp);
  if (FAILED(VariantChangeType(&vp,w,0,VT_UI4)))
    return DISP_E_TYPEMISMATCH;

  DWORD	  pid = 0;

  GetWindowThreadProcessId((HWND)V_UI4(&vp),&pid);

  if (result) {
    V_VT(result) = VT_UI4;
    V_UI4(result) = pid;
  }

  VariantClear(&vp);

  return S_OK;
}

typedef HRESULT  (*GenFunc)(...);
static struct {
  const wchar_t	*name;
  unsigned	minargs,maxargs;
  GenFunc	func;
} g_funcs[] = {
  { L"Win32Call", 0,  0,  0			},
  { L"Load",	  1,  1,  (GenFunc)sLoad	},
  { L"Message",	  1,  1,  (GenFunc)sMessage	},
  { L"GetClassName",1,1,  (GenFunc)sGetClassName},
  { L"GetWindowText",1,1, (GenFunc)sGetWindowText},
  { L"RunProgram",1,  2,  (GenFunc)sRunProgram	},
  { L"GetWindowPID",1,1,  (GenFunc)sGetWindowPID},
};

#define	NUMFUNCS  (sizeof(g_funcs)/sizeof(g_funcs[0]))

static HRESULT	__declspec(naked) VCall(GenFunc fp,unsigned argc,unsigned null,
					VARIANT *argv,VARIANT *result)
{
  __asm {
    push    ebx
    push    esi
    push    ebp
    mov	    ebp,esp

    mov	    eax,[ebp+16]
    mov	    ebx,[ebp+20]
    mov	    ecx,[ebp+28]
    mov	    edx,[ebp+32]
    mov	    esi,[ebp+24]

    push    edx

lv:
    cmp	    esi,0
    je	    li
    push    0
    dec	    esi
    jmp	    lv
li:
    cmp	    ebx,0
    je	    lo
    dec	    ebx
    push    ecx
    add	    ecx,16
    jmp	    li
lo:

    call    eax

    mov	    esp,ebp
    pop	    ebp
    pop	    esi
    pop	    ebx
    ret
  };
}

static DWORD __declspec(naked)	Win32CallImpl(FARPROC proc,unsigned argc,DWORD *argv)
{
  __asm {
    mov	    eax,[esp+4]
    mov	    ecx,[esp+8]
    mov	    edx,[esp+12]
li:
    cmp	    ecx,0
    je	    lo
    dec	    ecx
    push    [edx]
    add	    edx,4
    jmp	    li
lo:
    call    eax

    ret
  };
}

static HRESULT	Win32Call(unsigned argc,VARIANT *argv,VARIANT *result) {
  if (argc<2 || argc>MAXARGS+2)
    return DISP_E_BADPARAMCOUNT;

  if (V_VT(&argv[argc-1]) != VT_BSTR || V_VT(&argv[argc-2]) != VT_BSTR)
    return DISP_E_TYPEMISMATCH;

  if (!V_BSTR(&argv[argc-1]) || !V_BSTR(&argv[argc-2]))
    return E_INVALIDARG;

  HMODULE   hDLL = GetModuleHandleW(V_BSTR(&argv[argc-1]));
  if (hDLL == NULL) {
    hDLL = LoadLibraryW(V_BSTR(&argv[argc-1]));
    if (hDLL == NULL)
      return HRESULT_FROM_WIN32(GetLastError());
  }

  char	      procname[256];
  WideCharToMultiByte(CP_ACP,0,V_BSTR(&argv[argc-2]),-1,procname,sizeof(procname)-1,NULL,NULL);

  FARPROC    proc = GetProcAddress(hDLL,procname);
  if (proc == NULL) {
    lstrcatA(procname,"W");
    proc = GetProcAddress(hDLL,procname);
    if (proc == NULL)
      return HRESULT_FROM_WIN32(GetLastError());
  }

  DWORD	      wargs[MAXARGS];
  unsigned    numargs=0;

  argc -= 2;

  VARIANT vi;

  while (argc--) {
    switch (V_VT(argv)) {
      case VT_BSTR:
	wargs[numargs] = (DWORD)V_BSTR(argv);
	break;
      default:
	VariantInit(&vi);
	if (FAILED(VariantChangeType(&vi,argv,0,VT_UI4)))
	  return DISP_E_TYPEMISMATCH;
  	wargs[numargs] = V_UI4(&vi);
	VariantClear(&vi);
	break;
    }
    ++argv;
    ++numargs;
  }

  DWORD	r = Win32CallImpl(proc,numargs,wargs);

  if (result) {
    VariantInit(result);
    V_VT(result) = VT_UI4;
    V_UI4(result) = r;
  }

  return S_OK;
}

class ScriptCB : public IDispatch {
  ULONG	m_refs;
public:
  ScriptCB() : m_refs(1) { }
  ~ScriptCB() { }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid,void **ppv) {
    if (EQ(riid,IID_IUnknown) || EQ(riid,IID_IDispatch)) {
      *ppv = this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHOD_(ULONG,AddRef)() {
    return ++m_refs;
  }

  STDMETHOD_(ULONG,Release)() {
    ULONG refs = --m_refs;
    if (refs == 0)
      delete this;
    return refs;
  }

  // IDispatch
  STDMETHOD(GetTypeInfoCount)(unsigned *count) {
    *count = 0;
    return S_OK;
  }
  STDMETHOD(GetTypeInfo)(unsigned,LCID,ITypeInfo **) {
    return E_NOTIMPL;
  }
  STDMETHOD(GetIDsOfNames)(REFIID,OLECHAR **names,unsigned cNames,LCID,DISPID *IDs) {
    if (cNames == 0)
      return S_OK;

    IDs[0] = DISPID_UNKNOWN;
    for (unsigned i=0;i<NUMFUNCS;++i)
      if (lstrcmpW(names[0],g_funcs[i].name)==0) {
	IDs[0] = i + 1;
	break;
      }

    if (cNames > 1 || IDs[0] == DISPID_UNKNOWN) {
      for (unsigned i=1;i<cNames;++i)
	IDs[i] = DISPID_UNKNOWN;
      return DISP_E_UNKNOWNNAME;
    }

    return S_OK;
  }
  STDMETHOD(Invoke)(DISPID id,REFIID,LCID,WORD flags,DISPPARAMS *params,
		    VARIANT *result,EXCEPINFO*,unsigned *argerr)
  {
    if (id<1 || id>NUMFUNCS || !(flags & DISPATCH_METHOD))
      return DISP_E_MEMBERNOTFOUND;

    if (params->cNamedArgs != 0)
      return DISP_E_NONAMEDARGS;

    if (id == 1) // Win32Call is special
      return Win32Call(params->cArgs,params->rgvarg,result);

    if (params->cArgs < g_funcs[id-1].minargs || params->cArgs > g_funcs[id-1].maxargs)
      return DISP_E_BADPARAMCOUNT;

    if (result)
      VariantInit(result);

    return VCall(g_funcs[id-1].func,params->cArgs,g_funcs[id-1].maxargs-params->cArgs,params->rgvarg,result);
  }
};

static ScriptCB	  *g_CB;

class ScriptSite : public IActiveScriptSite {
  ULONG	m_refs;
public:
  ScriptSite() : m_refs(1) { }
  ~ScriptSite() { }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid,void **ppv) {
    if (EQ(riid,IID_IUnknown) || EQ(riid,IID_IActiveScriptSite)) {
      *ppv = this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHOD_(ULONG,AddRef)() {
    return ++m_refs;
  }

  STDMETHOD_(ULONG,Release)() {
    ULONG refs = --m_refs;
    if (refs == 0)
      delete this;
    return refs;
  }

  // IActiveScriptSite
  STDMETHOD(GetLCID)(LCID *) { return E_NOTIMPL; }
  STDMETHOD(GetItemInfo)(LPCOLESTR name, DWORD mask,IUnknown **ppUnk,ITypeInfo **ppTI) {
    if (ppTI)
      *ppTI = NULL; // we don't support this
    if (ppUnk) {
      *ppUnk = NULL;
      if (mask & SCRIPTINFO_IUNKNOWN) {
	if (lstrcmpW(name,L"IR")==0 && g_CB!=NULL) {
	  *ppUnk = g_CB;
	  g_CB->AddRef();
	  return S_OK;
	}
      }
    }
    return TYPE_E_ELEMENTNOTFOUND;
  }
  STDMETHOD(GetDocVersionString)(BSTR *) { return E_NOTIMPL; }
  STDMETHOD(OnScriptTerminate)(const VARIANT *,const EXCEPINFO *) { return S_OK; }
  STDMETHOD(OnStateChange)(SCRIPTSTATE) { return S_OK; }
  STDMETHOD(OnScriptError)(IActiveScriptError *err) {
    EXCEPINFO ei;
    if (SUCCEEDED(err->GetExceptionInfo(&ei))) {
      DWORD ctx;
      ULONG line = 0;
      LONG  column = 0;
      wchar_t *buf = NULL;

      err->GetSourcePosition(&ctx,&line,&column);

      buf = new wchar_t[lstrlenW(ei.bstrDescription) + 256];
      if (buf) {
	if (ei.bstrDescription)
	  wsprintfW(buf,L"Script error: %s at line %d, column %d",ei.bstrDescription,line+1,column+1);
	else
	  wsprintfW(buf,L"Script error: %08X at line %d, column %d",ei.scode,line+1,column+1);
	EMSG(buf);
	delete[] buf;
      }
      SysFreeString(ei.bstrSource);
      SysFreeString(ei.bstrDescription);
      SysFreeString(ei.bstrHelpFile);
    } else
      EMSG(_T("Unknown error in script."));
    return S_OK;
  }
  STDMETHOD(OnEnterScript)() { return S_OK; }
  STDMETHOD(OnLeaveScript)() { return S_OK; }
};

static ScriptSite     *g_site;
static IActiveScript  *g_script;

void	StopScript(void) {
  if (g_script) {
    g_script->Close();
    g_script->Release();
    g_script = NULL;
  }
  if (g_site) {
    g_site->Release();
    g_site = NULL;
  }
  if (g_CB) {
    g_CB->Release();
    g_CB = NULL;
  }
}

int	StartScript(void) {
  HRESULT hr;

  g_site = new ScriptSite();
  if (g_site == NULL)
    return -1;

  g_CB = new ScriptCB();
  if (g_CB == NULL) {
    StopScript();
    return -1;
  }

  if (FAILED(CoCreateInstance(CLSID_JScript,NULL,CLSCTX_INPROC_SERVER,IID_IActiveScript,(void**)&g_script))) {
    StopScript();
    return -1;
  }

  hr = g_script->SetScriptSite(g_site);
  hr = g_script->AddNamedItem(L"IR",SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISPERSISTENT|SCRIPTITEM_NOCODE|SCRIPTITEM_GLOBALMEMBERS);

  IActiveScriptParse	*pF;
  if (FAILED(g_script->QueryInterface(IID_IActiveScriptParse,(void**)&pF))) {
    StopScript();
    return -1;
  }

  hr = pF->InitNew();
  pF->Release();

  hr = g_script->SetScriptState(SCRIPTSTATE_STARTED);

  return 0;
}

HRESULT	ScriptCall(const wchar_t *func,VARIANT *arg,VARIANT *ret) {
  HRESULT hr;

  if (g_script == NULL)
    return E_FAIL;

  IDispatch *pDisp;
  if (FAILED(hr = g_script->GetScriptDispatch(NULL,&pDisp)))
    return hr;

  DISPID    id;
  if (FAILED(hr = pDisp->GetIDsOfNames(IID_NULL,(wchar_t **)&func,1,LANG_USER_DEFAULT,&id))) {
    pDisp->Release();
    return hr;
  }

  DISPPARAMS  params;
  params.cNamedArgs = 0;
  params.rgdispidNamedArgs = NULL;
  params.cArgs = 0;
  params.rgvarg = NULL;

  if (arg) {
    params.cArgs = 1;
    params.rgvarg = arg;
  }

  unsigned  argerr;
  hr = pDisp->Invoke(id,IID_NULL,LANG_USER_DEFAULT,DISPATCH_METHOD,&params,ret,NULL,&argerr);

  pDisp->Release();

  return hr;
}

static HANDLE TryOpen(bool pfx,const wchar_t *mid,const wchar_t *last) {
  wchar_t   xfilename[MAX_PATH];

  xfilename[0] = 0;

  if (pfx) {
    wchar_t *cp;
    GetModuleFileNameW(NULL,xfilename,sizeof(xfilename)/sizeof(xfilename[0]));
    for (cp = xfilename + lstrlenW(xfilename);cp>xfilename;--cp)
      if (cp[-1] == L'/' || cp[-1] == L'\\')
	break;
    *cp = 0;
  }

  if (mid)
    strlcatW(xfilename,mid,sizeof(xfilename)/sizeof(xfilename[0]));

  int len = lstrlenW(xfilename);
  if (len > 0 && (xfilename[len-1]==_T('/') || xfilename[len-1]==_T('\\')) &&
      (last && (*last==_T('/') || *last==_T('\\'))))
    ++last;

  strlcatW(xfilename,last,sizeof(xfilename)/sizeof(xfilename[0]));

  return CreateFile(xfilename,GENERIC_READ,0,NULL,OPEN_EXISTING,0,0);
}

HRESULT	ScriptLoad(const wchar_t *filename) {
  if (!g_script)
    return E_FAIL;

  HRESULT   hr;

  // open file and try to load it in memory
  HANDLE    hFile;
  if ((hFile = TryOpen(false,NULL,filename)) == INVALID_HANDLE_VALUE &&
      (hFile = TryOpen(true,NULL,filename)) == INVALID_HANDLE_VALUE &&
      (hFile = TryOpen(true,L"..\\",filename)) == INVALID_HANDLE_VALUE)
  {
    DWORD   code = GetLastError();
    wchar_t em[256];
    wchar_t *buf = new wchar_t[lstrlenW(filename)+512];
    if (buf) {
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,code,0,em,sizeof(em)/sizeof(em[0]),0);
      wsprintfW(buf,L"Error loading %s: %s",filename,em);
      EMSG(buf);
      delete[] buf;
    }
    return HRESULT_FROM_WIN32(GetLastError());
  }

  DWORD length = SetFilePointer(hFile,0,NULL,FILE_END);
  SetFilePointer(hFile,0,NULL,FILE_BEGIN);

  char		  *tmp = new char[length];
  if (tmp == NULL) {
    CloseHandle(hFile);
    return E_OUTOFMEMORY;
  }

  DWORD	nrd;
  BOOL	ok = ReadFile(hFile,tmp,length,&nrd,NULL);
  CloseHandle(hFile);
  if (!ok) {
    delete[] tmp;
    return HRESULT_FROM_WIN32(GetLastError());
  }
  if (nrd!=length) {
    delete[] tmp;
    return E_FAIL;
  }

  wchar_t	  *buffer = new wchar_t[length+1];
  if (buffer == NULL) {
    delete[] tmp;
    return E_OUTOFMEMORY;
  }

  DWORD	cvt = MultiByteToWideChar(CP_ACP,0,tmp,length,buffer,length);

  delete[] tmp;

  buffer[cvt] = 0;

  IActiveScriptParse	*pF;
  if (FAILED(hr = g_script->QueryInterface(IID_IActiveScriptParse,(void**)&pF))) {
    delete[] buffer;
    return hr;
  }

  EXCEPINFO ei;
  ZeroMemory(&ei,sizeof(ei));
  hr = pF->ParseScriptText(buffer,
			   NULL,NULL,NULL,0,0,SCRIPTTEXT_ISVISIBLE|SCRIPTTEXT_ISPERSISTENT,
			   NULL,&ei);

  SysFreeString(ei.bstrSource);
  SysFreeString(ei.bstrDescription);
  SysFreeString(ei.bstrHelpFile);

  pF->Release();
  delete[] buffer;

  return hr;
}