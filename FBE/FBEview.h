// FBEView.h : interface of the CFBEView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FBEVIEW_H__E0C71279_419D_4273_93E3_57F6A57C7CFE__INCLUDED_)
#define AFX_FBEVIEW_H__E0C71279_419D_4273_93E3_57F6A57C7CFE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

template<class T,int chgID>
class ATL_NO_VTABLE CHTMLChangeSink: public MSHTML::IHTMLChangeSink {
protected:
public:
  // IUnknown
  STDMETHOD(QueryInterface)(REFIID iid,void **ppvObject) {
    if (iid==IID_IUnknown || iid==IID_IHTMLChangeSink) {
      *ppvObject=this;
      return S_OK;
    }
    return E_NOINTERFACE;
  }
  STDMETHOD_(ULONG,AddRef)() { return 1; }
  STDMETHOD_(ULONG,Release)() { return 1; }
  // IHTMLChangeSink
  STDMETHOD(raw_Notify)() {
    T	*pT=static_cast<T*>(this);
    pT->EditorChanged(chgID);
    return S_OK;
  }
};

typedef CWinTraits<WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0>
		  CFBEViewWinTraits;

enum { FWD_SINK, BACK_SINK, RANGE_SINK };

class CFBEView : public CWindowImpl<CFBEView, CAxWindow, CFBEViewWinTraits>,
		 public IDispEventSimpleImpl<0, CFBEView, &DIID_DWebBrowserEvents2>,
		 public IDispEventSimpleImpl<0, CFBEView, &DIID_HTMLDocumentEvents2>,
		 public IDispEventSimpleImpl<0, CFBEView, &DIID_HTMLTextContainerEvents2>,
		 public CHTMLChangeSink<CFBEView,RANGE_SINK>
{
protected:
  typedef IDispEventSimpleImpl<0, CFBEView, &DIID_DWebBrowserEvents2> BrowserEvents;
  typedef IDispEventSimpleImpl<0, CFBEView, &DIID_HTMLDocumentEvents2> DocumentEvents;
  typedef IDispEventSimpleImpl<0, CFBEView, &DIID_HTMLTextContainerEvents2> TextEvents;
  typedef CHTMLChangeSink<CFBEView,FWD_SINK>	  ForwardSink;
  typedef CHTMLChangeSink<CFBEView,BACK_SINK>	  BackwardSink;
  typedef CHTMLChangeSink<CFBEView,RANGE_SINK>	  RangeSink;

  HWND			    m_frame;

  IWebBrowser2Ptr	    m_browser;
  MSHTML::IHTMLDocument2Ptr m_hdoc;
  MSHTML::IMarkupServices2Ptr  m_mk_srv;
  MSHTML::IMarkupContainer2Ptr m_mkc;
  DWORD			    m_dirtyRangeCookie;

  int			    m_ignore_changes;
  int			    m_enable_paste;

  bool			    m_normalize:1;
  bool			    m_complete:1;
  bool			    m_initialized:1;

  MSHTML::IHTMLElementPtr   m_cur_sel;
  MSHTML::IHTMLInputTextElementPtr m_cur_input;
  _bstr_t		    m_cur_val;
  bool			    m_form_changed;
  bool			    m_form_cp;

  CString		    m_nav_url;

  static _ATL_FUNC_INFO DocumentCompleteInfo;
  static _ATL_FUNC_INFO BeforeNavigateInfo;
  static _ATL_FUNC_INFO	EventInfo;
  static _ATL_FUNC_INFO	VoidEventInfo;
  static _ATL_FUNC_INFO VoidInfo;

  enum {
    FRF_REVERSE	= 1,
    FRF_WHOLE = 2,
    FRF_CASE = 4,
    FRF_REGEX = 8
  };
  struct FindReplaceOptions {
    CString	pattern;
    CString	replacement;
    AU::ReMatch	match;
    int		flags; // IHTMLTxtRange::findText() flags
    bool	fRegexp;

    FindReplaceOptions() : fRegexp(false), flags(0) { }
  };
  FindReplaceOptions	    m_fo;
  MSHTML::IHTMLTxtRangePtr  m_is_start;

  friend class CFindDlgBase;
  friend class CViewFindDlg;
  friend class CReplaceDlgBase;
  friend class CViewReplaceDlg;
  friend class CSciFindDlg;
  friend class CSciReplaceDlg;
  friend class FRBase;

  void			    SelMatch(MSHTML::IHTMLTxtRange *tr,AU::ReMatch rm);
  MSHTML::IHTMLElementPtr   SelectionContainerImp();

public:

  IWebBrowser2Ptr	    Browser() { return m_browser; }
  MSHTML::IHTMLDocument2Ptr Document() { return m_hdoc; }
  bool			    HasDoc() { return m_hdoc; }
  IDispatchPtr		    Script(){ return MSHTML::IHTMLDocumentPtr(m_hdoc)->Script; }
  CString		    NavURL() { return m_nav_url; }

  bool			    Loaded() { bool cmp=m_complete; m_complete=false; return cmp; }
  void			    Init();

  long			    GetVersionNumber() { return m_mkc ? m_mkc->GetVersionNumber() : -1; }

  void			    BeginUndoUnit(const wchar_t *name) { m_mk_srv->BeginUndoUnit((USHORT*)name); }
  void			    EndUndoUnit() { m_mk_srv->EndUndoUnit(); }

  DECLARE_WND_SUPERCLASS(NULL, CAxWindow::GetWndClassName())

  CFBEView(HWND frame,bool fNorm) : m_frame(frame), m_ignore_changes(0), m_enable_paste(0),
    m_normalize(fNorm), m_complete(false), m_initialized(false),
    m_form_changed(false), m_form_cp(false) { }
  ~CFBEView();

  BOOL PreTranslateMessage(MSG* pMsg);

  BEGIN_MSG_MAP(CFBEView)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)

    // editing commands
    COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnUndo)
    COMMAND_ID_HANDLER(ID_EDIT_REDO, OnRedo)
    COMMAND_ID_HANDLER(ID_EDIT_CUT, OnCut)
    COMMAND_ID_HANDLER(ID_EDIT_COPY, OnCopy)
    COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnPaste)
    COMMAND_ID_HANDLER(ID_EDIT_BOLD, OnBold)
    COMMAND_ID_HANDLER(ID_EDIT_ITALIC, OnItalic)
    COMMAND_ID_HANDLER(ID_EDIT_FIND, OnFind)
    COMMAND_ID_HANDLER(ID_EDIT_FINDNEXT, OnFindNext)
    COMMAND_ID_HANDLER(ID_EDIT_REPLACE, OnReplace)

    COMMAND_ID_HANDLER(ID_STYLE_LINK, OnStyleLink)
    COMMAND_ID_HANDLER(ID_STYLE_NOTE, OnStyleFootnote)
    COMMAND_ID_HANDLER(ID_STYLE_NOLINK, OnStyleNolink)

    COMMAND_ID_HANDLER(ID_STYLE_NORMAL, OnStyleNormal)
    COMMAND_ID_HANDLER(ID_STYLE_TEXTAUTHOR, OnStyleTextAuthor)
    COMMAND_ID_HANDLER(ID_STYLE_SUBTITLE, OnStyleSubtitle)

    COMMAND_ID_HANDLER(ID_EDIT_ADD_TITLE, OnEditAddTitle)
    COMMAND_ID_HANDLER(ID_EDIT_ADD_BODY, OnEditAddBody)
    COMMAND_ID_HANDLER(ID_EDIT_ADD_EPIGRAPH, OnEditAddEpigraph)
    COMMAND_ID_HANDLER(ID_EDIT_ADD_TA, OnEditAddTA)
    COMMAND_ID_HANDLER(ID_EDIT_CLONE, OnEditClone)
    COMMAND_ID_HANDLER(ID_EDIT_INS_IMAGE, OnEditInsImage)
    COMMAND_ID_HANDLER(ID_EDIT_ADD_IMAGE, OnEditAddImage)
    COMMAND_ID_HANDLER(ID_EDIT_ADD_ANN,OnEditAddAnn)

    COMMAND_ID_HANDLER(ID_EDIT_SPLIT, OnEditSplit)
    COMMAND_ID_HANDLER(ID_EDIT_MERGE, OnEditMerge)
    COMMAND_ID_HANDLER(ID_EDIT_REMOVE_OUTER_SECTION, OnEditRemoveOuter)

    COMMAND_ID_HANDLER(ID_EDIT_INS_POEM, OnEditInsPoem)
    COMMAND_ID_HANDLER(ID_EDIT_INS_CITE, OnEditInsCite)

    COMMAND_ID_HANDLER(ID_VIEW_HTML, OnViewHTML)

    COMMAND_RANGE_HANDLER(ID_SEL_BASE,ID_SEL_BASE+99,OnSelectElement)
  END_MSG_MAP()

  BEGIN_SINK_MAP(CFBEView)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, OnDocumentComplete, &DocumentCompleteInfo)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, OnBeforeNavigate, &BeforeNavigateInfo)
    SINK_ENTRY_INFO(0, DIID_HTMLDocumentEvents2, DISPID_HTMLDOCUMENTEVENTS2_ONSELECTIONCHANGE, OnSelChange, &VoidEventInfo)
    SINK_ENTRY_INFO(0, DIID_HTMLDocumentEvents2, DISPID_HTMLDOCUMENTEVENTS2_ONCONTEXTMENU, OnContextMenu, &EventInfo)
    SINK_ENTRY_INFO(0, DIID_HTMLDocumentEvents2, DISPID_HTMLDOCUMENTEVENTS2_ONCLICK, OnClick, &EventInfo)
    SINK_ENTRY_INFO(0, DIID_HTMLDocumentEvents2, DISPID_HTMLDOCUMENTEVENTS2_ONFOCUSIN, OnFocusIn, &VoidEventInfo)
    SINK_ENTRY_INFO(0, DIID_HTMLTextContainerEvents2, DISPID_HTMLELEMENTEVENTS2_ONPASTE, OnRealPaste, &EventInfo)
    SINK_ENTRY_INFO(0, DIID_HTMLTextContainerEvents2, DISPID_HTMLELEMENTEVENTS2_ONDRAGEND, OnDrop, &VoidEventInfo)
  END_SINK_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    // pass to document
    if (HasDoc())
      MSHTML::IHTMLDocument4Ptr(Document())->focus();
    return 0;
  }

  // editing commands
  LRESULT ExecCommand(int cmd);
  void	  QueryStatus(OLECMD *cmd,int ncmd);
  CString QueryCmdText(int cmd);
  LRESULT OnUndo(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_UNDO); }
  LRESULT OnRedo(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_REDO); }
  LRESULT OnCut(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_CUT); }
  LRESULT OnCopy(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_COPY); }
  LRESULT OnPaste(WORD, WORD, HWND, BOOL&);
  LRESULT OnBold(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_BOLD); }
  LRESULT OnItalic(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_ITALIC); }
  LRESULT OnFind(WORD, WORD, HWND, BOOL&);
  LRESULT OnFindNext(WORD, WORD, HWND, BOOL&);
  LRESULT OnReplace(WORD, WORD, HWND, BOOL&);
  LRESULT OnStyleLink(WORD, WORD, HWND, BOOL&);
  LRESULT OnStyleFootnote(WORD, WORD, HWND, BOOL&);
  LRESULT OnStyleNolink(WORD, WORD, HWND, BOOL&) { return ExecCommand(IDM_UNLINK); }
  LRESULT OnStyleNormal(WORD, WORD, HWND, BOOL&) { Call(L"StyleNormal",SelectionStructCon()); return 0; }
  LRESULT OnStyleTextAuthor(WORD, WORD, HWND, BOOL&) { Call(L"StyleTextAuthor",SelectionStructCon()); return 0; }
  LRESULT OnStyleSubtitle(WORD, WORD, HWND, BOOL&) { Call(L"StyleSubtitle",SelectionStructCon()); return 0; }
  LRESULT OnViewHTML(WORD, WORD, HWND, BOOL&) {
    IOleCommandTargetPtr  ct(m_browser);
    if (ct)
      ct->Exec(&CGID_MSHTML, IDM_VIEWSOURCE, 0, NULL, NULL);
    return 0;
  }
  LRESULT OnSelectElement(WORD, WORD, HWND, BOOL&);
  LRESULT OnEditAddTitle(WORD, WORD, HWND, BOOL&) { Call(L"AddTitle",SelectionStructCon()); return 0; }
  LRESULT OnEditAddEpigraph(WORD, WORD, HWND, BOOL&) { Call(L"AddEpigraph",SelectionStructCon()); return 0; }
  LRESULT OnEditAddBody(WORD, WORD, HWND, BOOL&) { Call(L"AddBody"); return 0; }
  LRESULT OnEditAddTA(WORD, WORD, HWND, BOOL&) { Call(L"AddTA",SelectionStructCon()); return 0; }
  LRESULT OnEditClone(WORD, WORD, HWND, BOOL&) { Call(L"CloneContainer",SelectionStructCon()); return 0; }
  LRESULT OnEditInsImage(WORD, WORD, HWND, BOOL&);
  LRESULT OnEditAddImage(WORD, WORD, HWND, BOOL&) { Call(L"AddImage",SelectionStructCon()); return 0; }
  LRESULT OnEditAddAnn(WORD, WORD, HWND, BOOL&) { Call(L"AddAnnotation",SelectionStructCon()); return 0; }
  LRESULT OnEditMerge(WORD, WORD, HWND, BOOL&) { Call(L"MergeContainers",SelectionStructCon()); return 0; }
  LRESULT OnEditSplit(WORD, WORD, HWND, BOOL&) { SplitContainer(false); return 0; }
  LRESULT OnEditInsPoem(WORD, WORD, HWND, BOOL&) { InsertPoemOrCite(false,false); return 0; }
  LRESULT OnEditInsCite(WORD, WORD, HWND, BOOL&) { InsertPoemOrCite(true,false); return 0; }
  LRESULT OnEditRemoveOuter(WORD, WORD, HWND, BOOL&) { Call(L"RemoveOuterContainer",SelectionStructCon()); return 0; }

  bool	CheckCommand(WORD wID);

  // searching
  bool	  CanFindNext() { return !m_fo.pattern.IsEmpty(); }
  void	  CancelIncSearch();
  void	  StartIncSearch();
  void	  StopIncSearch() { if (m_is_start) m_is_start.Release(); }
  bool	  DoIncSearch(const CString& str,bool fMore) {
    ++m_ignore_changes;
    m_fo.pattern=str;
    bool ret=DoSearch(fMore);
    --m_ignore_changes;
    return ret;
  }
  bool	  DoSearch(bool fMore=true);
  bool	  DoSearchStd(bool fMore=true);
  bool	  DoSearchRegexp(bool fMore=true);
  void	  DoReplace();
  int	  GlobalReplace();
  CString LastSearchPattern() { return m_fo.pattern; }
  int	  ReplaceAllRe(const CString& re,const CString& str) {
    m_fo.pattern=re;
    m_fo.replacement=str;
    m_fo.fRegexp=true;
    m_fo.flags=0;
    return GlobalReplace();
  }

  // searching in scintilla
  bool SciFindNext(HWND src,bool fFwdOnly,bool fBarf);

  // utilities
  CString		    SelPath();
  void			    GoTo(MSHTML::IHTMLElement *e,bool fScroll=true);
  MSHTML::IHTMLElementPtr   SelectionContainer() {
    if (m_cur_sel)
      return m_cur_sel;
    return SelectionContainerImp();
  }
  MSHTML::IHTMLElementPtr   SelectionAnchor();
  MSHTML::IHTMLElementPtr   SelectionStructCon();
  void			    Normalize(MSHTML::IHTMLDOMNodePtr dom);
  MSHTML::IHTMLDOMNodePtr   GetChangedNode();
  void			    ImgSetURL(IDispatch *elem,const CString& url);

  bool			    SplitContainer(bool fCheck);
  bool			    InsertPoemOrCite(bool fCite,bool fCheck);

  // script calls
  IDispatchPtr	Call(const wchar_t *name);
  bool		bCall(const wchar_t *name);
  IDispatchPtr	Call(const wchar_t *name,IDispatch *pDisp);
  bool		bCall(const wchar_t *name,IDispatch *pDisp);

  // binary objects
  _variant_t	GetBinary(const wchar_t *id);

  // change notifications
  void	EditorChanged(int id);

  // external helper
  static IDispatchPtr	CreateHelper();

  // DWebBrowserEvents2
  void __stdcall  OnDocumentComplete(IDispatch *pDisp,VARIANT *vtUrl);
  void __stdcall  OnBeforeNavigate(IDispatch *pDisp,VARIANT *vtUrl,VARIANT *vtFlags,
				   VARIANT *vtTargetFrame,VARIANT *vtPostData,
				   VARIANT *vtHeaders,VARIANT_BOOL *fCancel);

  // HTMLDocumentEvents2
  void __stdcall	  OnSelChange(IDispatch *evt);
  VARIANT_BOOL __stdcall  OnContextMenu(IDispatch *evt);
  VARIANT_BOOL __stdcall  OnClick(IDispatch *evt);
  void __stdcall	  OnFocusIn(IDispatch *evt);

  // HTMLTextContainerEvents2
  VARIANT_BOOL __stdcall  OnRealPaste(IDispatch *evt);
  void __stdcall  OnDrop(IDispatch *evt) {
    if (m_normalize)
      Normalize(Document()->body);
  }

  // form changes
  bool	    IsFormChanged();
  void	    ResetFormChanged();
  bool	    IsFormCP();
  void	    ResetFormCP();

  // extract currently selected text
  _bstr_t   Selection();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FBEVIEW_H__E0C71279_419D_4273_93E3_57F6A57C7CFE__INCLUDED_)
