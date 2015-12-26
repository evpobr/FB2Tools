#ifndef EXTERNALHELPER_H
#define EXTERNALHELPER_H

class ExternalHelper :
  public CComObjectRoot,
  public IDispatchImpl<IExternalHelper, &IID_IExternalHelper>
{
public:

  DECLARE_NO_REGISTRY()

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  BEGIN_COM_MAP(ExternalHelper)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IExternalHelper)
  END_COM_MAP()

  // IExternalHelper
  STDMETHOD(BeginUndoUnit)(IDispatch *obj,BSTR name) {
    MSHTML::IMarkupServices   *srv;
    HRESULT hr=obj->QueryInterface(&srv);
    if (FAILED(hr))
      return hr;
    hr=srv->raw_BeginUndoUnit(name);
    srv->Release();
    return hr;
  }
  STDMETHOD(EndUndoUnit)(IDispatch *obj) {
    MSHTML::IMarkupServices   *srv;
    HRESULT hr=obj->QueryInterface(&srv);
    if (FAILED(hr))
      return hr;
    hr=srv->raw_EndUndoUnit();
    srv->Release();
    return hr;
  }
  STDMETHOD(get_inflateBlock)(IDispatch *obj,BOOL *ifb) {
    MSHTML::IHTMLElement3 *elem;
    HRESULT hr=obj->QueryInterface(&elem);
    if (FAILED(hr))
      return hr;
    VARIANT_BOOL vb;
    hr=elem->get_inflateBlock(&vb);
    *ifb=SUCCEEDED(hr) && vb==VARIANT_TRUE ? TRUE : FALSE;
    elem->Release();
    return hr;
  }
  STDMETHOD(put_inflateBlock)(IDispatch *obj,BOOL ifb) {
    MSHTML::IHTMLElement3 *elem;
    HRESULT hr=obj->QueryInterface(&elem);
    if (FAILED(hr))
      return hr;
    hr=elem->put_inflateBlock(ifb ? VARIANT_TRUE : VARIANT_FALSE);
    elem->Release();
    return hr;
  }
  STDMETHOD(GenrePopup)(IDispatch *obj,LONG x,LONG y,BSTR *name);
};

#endif