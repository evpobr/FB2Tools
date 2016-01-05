#pragma once

#include "resource.h"
#include "ExportHTML_h.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

class ATL_NO_VTABLE CExportHTMLPlugin :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CExportHTMLPlugin, &CLSID_ExportHTMLPlugin>,
	public IFBEExportPlugin
{
public:
	CExportHTMLPlugin()
	{
	}

  DECLARE_REGISTRY_RESOURCEID(IDR_EXPORTHTML)

  DECLARE_NOT_AGGREGATABLE(CExportHTMLPlugin)

  BEGIN_COM_MAP(CExportHTMLPlugin)
	  COM_INTERFACE_ENTRY(IFBEExportPlugin)
  END_COM_MAP()

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct()
  {
	  return S_OK;
  }

  void FinalRelease()
  {
  }

  // IFBEExportPlugin
  STDMETHOD(Export)(long hWnd,BSTR filename,IDispatch *doc);
};

OBJECT_ENTRY_AUTO(CLSID_ExportHTMLPlugin, CExportHTMLPlugin)
