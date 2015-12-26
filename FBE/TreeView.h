#ifndef LTREEVIEW_H
#define	LTREEVIEW_H

typedef CWinTraits<WS_CHILD|WS_VISIBLE|
		   TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS,0>
		  CLPTVWinTraits;

class CTreeView : public CWindowImpl<CTreeView, CTreeViewCtrlEx, CLPTVWinTraits>
{
protected:
  WTL::CImageList	  m_ImageList;
  HTREEITEM		  m_last_lookup_item;

public:
  DECLARE_WND_SUPERCLASS(_T("Tree"), CTreeViewCtrlEx::GetWndClassName())

  CTreeView() : m_last_lookup_item(0) { }
    
  BOOL PreTranslateMessage(MSG* pMsg);
  
  BEGIN_MSG_MAP(CTreeView)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnClick)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnDblClick)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_CHAR, OnChar)

    REFLECTED_NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnDeleteItem)
    REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
  END_MSG_MAP()
    
  LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    NMTREEVIEW	  *tvn=(NMTREEVIEW*)pnmh;
    if (tvn->itemOld.lParam)
      ((MSHTML::IHTMLElement*)tvn->itemOld.lParam)->Release();
    return 0;
  }
  LRESULT OnRClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    // TODO Display context menu
    return 0;
  }

  // get document structure from view
  void GetDocumentStructure(CFBEView& v);
  void UpdateDocumentStructure(CFBEView& v,MSHTML::IHTMLDOMNodePtr node);
  void HighlightItemAtPos(MSHTML::IHTMLElement *p);
protected:
  CTreeItem LocatePosition(MSHTML::IHTMLElement *p);
};

#endif