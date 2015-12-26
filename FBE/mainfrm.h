// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__38D356D4_C28B_47B0_A7AD_8C6B70F7F283__INCLUDED_)
#define AFX_MAINFRM_H__38D356D4_C28B_47B0_A7AD_8C6B70F7F283__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef CWinTraits<WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_LEFT,WS_EX_CLIENTEDGE>
		  CCustomEditWinTraits;

class CCustomEdit : public CWindowImpl<CCustomEdit,CEdit,CCustomEditWinTraits>,
		    public CEditCommands<CCustomEdit>
{
public:
  DECLARE_WND_SUPERCLASS(NULL, CEdit::GetWndClassName())

  CCustomEdit() { }

  BEGIN_MSG_MAP(CCustomEdit)
    MESSAGE_HANDLER(WM_CHAR, OnChar)
    CHAIN_MSG_MAP_ALT(CEditCommands<CCustomEdit>,1)
  END_MSG_MAP()

  LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
  {
    if (wParam==VK_RETURN)
      ::PostMessage(::GetParent(GetParent()),WM_COMMAND,MAKELONG(GetDlgCtrlID(),IDN_ED_RETURN),(LPARAM)m_hWnd);
    bHandled=FALSE;
    return 0;
  }
};

class CMainFrame : public CFrameWindowImpl<CMainFrame>,
		   public CUpdateUI<CMainFrame>,
		   public CMessageFilter,
		   public CIdleHandler
{
public:
  enum FILE_OP_STATUS {
    FAIL,
    OK,
    CANCELLED
  };
  DECLARE_FRAME_WND_CLASS(_T("FictionBookEditorFrame"), IDR_MAINFRAME)
  
  // child windows
  CSplitterWindow	  m_splitter; // doc tree and views
  CContainerWnd		  m_view; // document, description and source
  CPaneContainer	  m_tree_pane; // left pane with a tree
  CSplitterWindow	  m_dummy_pane; // frame around the tree
  CTreeView		  m_tree; // treeview itself
  CMultiPaneStatusBarCtrl m_status; // status bar
  CCommandBarCtrl	  m_CmdBar; // menu bar
  CReBarCtrl		  m_rebar; // toolbars
  CComboBox		  m_id_box;
  CComboBox		  m_href_box;
  CCustomEdit		  m_id; // paragraph ID
  CCustomEdit		  m_href; // link's href
  CWindow		  m_source; // source editor
  bool			  m_save_sp_mode;

  CRecentDocumentList	  m_mru; // MRU list

  FB::Doc		  *m_doc; // currently open document
  DWORD			  m_last_tree_update;
  bool			  m_last_ovr_state:1;
  bool			  m_doc_changed:1;
  bool			  m_sel_changed:1;
  bool			  m_change_state:1;
  bool			  m_need_title_update:1;

  // IDs in combobox
  bool			  m_cb_updated:1;
  bool			  m_cb_last_images:1; // images or plain ids?
  bool			  m_ignore_cb_changes:1;

  int			  m_want_focus; // focus this control when idle

  CString		  m_status_msg; // message to be posted to frame's status line

  
  // incremental search helpers
  CString		  m_is_str;
  CString		  m_is_prev;
  int			  m_incsearch;
  bool			  m_is_fail;

  // scripts
  CSimpleArray<CString>	  m_scripts;

  // contruction/destruction
  CMainFrame() : m_doc(0), m_cb_updated(false),
    m_doc_changed(false), m_sel_changed(false), m_want_focus(0),
    m_ignore_cb_changes(false), m_incsearch(0), m_cb_last_images(false),
    m_last_ovr_state(true) { }
  ~CMainFrame() { delete m_doc; }

  // toolbars
  bool	  IsBandVisible(int id);

  // browser controls
  void	  AttachDocument(FB::Doc *doc);
  CFBEView& ActiveView() {
    return m_doc->m_desc==m_view.GetActiveWnd() ?
	      m_doc->m_desc : m_doc->m_body;
  }
  bool	  IsSourceActive() { return m_source==m_view.GetActiveWnd(); }

  // document structure
  void	  GetDocumentStructure();
  void	  GoTo(MSHTML::IHTMLElement *elem);

  // loading/saving support
  CString GetOpenFileName();
  CString GetSaveFileName(CString& encoding);
  bool	  SaveToFile(const CString& filename);
  bool	  DiscardChanges();

  FILE_OP_STATUS	  SaveFile(bool askname);
  FILE_OP_STATUS	  LoadFile(const wchar_t *initfilename=NULL);

  // show a specific view
  enum VIEW_TYPE { BODY, DESC, SOURCE, NEXT };
  void	  ShowView(VIEW_TYPE vt=BODY);
  VIEW_TYPE GetCurView();

  // plugins support
  CSimpleArray<CLSID>	m_import_plugins;
  CSimpleArray<CLSID>	m_export_plugins;
  void	  InitPlugins();
  void	  InitPluginsType(HMENU hMenu,const TCHAR *type,UINT cmdbase,CSimpleArray<CLSID>& plist);

  // ui updating
  void	  UIUpdateViewCmd(CFBEView& view,WORD wID,OLECMD& oc,const TCHAR *hk);
  void	  UIUpdateViewCmd(CFBEView& view,WORD wID) { UIEnable(wID,view.CheckCommand(wID)); }

  void	  StopIncSearch(bool fCancel);
  void	  SetIsText();

  // source editor
  void	  DefineMarker(int marker, int markerType, COLORREF fore,COLORREF back);
  void	  SetupSci();

  // source folding
  void	  FoldAll();
  void	  ExpandFold(int &line, bool doExpand, bool force = false,
		     int visLevels = 0, int level = -1);

  // source editor styles
  void	  SetSciStyles();

  // source<->html exchange
  bool	  SourceToHTML();

  // track changes depending on current view
  bool	  DocChanged();

  // message handlers
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  virtual BOOL OnIdle();
  
  BEGIN_UPDATE_UI_MAP(CMainFrame)
    // ui windows
    UPDATE_ELEMENT(ATL_IDW_BAND_FIRST, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ATL_IDW_BAND_FIRST+1, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ATL_IDW_BAND_FIRST+2, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ATL_IDW_BAND_FIRST+3, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ATL_IDW_BAND_FIRST+4, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ID_VIEW_TREE, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ID_VIEW_DESC, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ID_VIEW_BODY, UPDUI_MENUPOPUP)
    UPDATE_ELEMENT(ID_VIEW_SOURCE, UPDUI_MENUPOPUP)

    // editing commands
    UPDATE_ELEMENT(ID_EDIT_UNDO, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_REDO, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_CUT, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_COPY, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_PASTE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_BOLD, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ITALIC, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_FINDNEXT, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_STYLE_NORMAL, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_STYLE_SUBTITLE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_STYLE_TEXTAUTHOR, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_STYLE_LINK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_STYLE_NOTE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_STYLE_NOLINK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ADD_TITLE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ADD_BODY, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ADD_TA, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_CLONE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_INS_IMAGE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ADD_IMAGE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ADD_EPIGRAPH, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_ADD_ANN, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_SPLIT, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_INS_POEM, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_INS_CITE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_MERGE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_EDIT_REMOVE_OUTER_SECTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
  END_UPDATE_UI_MAP()
    
  BEGIN_MSG_MAP(CMainFrame)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(AU::WM_POSTCREATE, OnPostCreate)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
#if _WIN32_WINNT>=0x0501
    MESSAGE_HANDLER(WM_THEMECHANGED, OnSettingChange)
#endif
    MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
    MESSAGE_HANDLER(AU::WM_SETSTATUSTEXT, OnSetStatusText)
    MESSAGE_HANDLER(AU::WM_TRACKPOPUPMENU, OnTrackPopupMenu)

    // incremental search support
    MESSAGE_HANDLER(WM_CHAR, OnChar)
    MESSAGE_HANDLER(WM_COMMAND, OnPreCommand)

    // tree view notifications
    COMMAND_CODE_HANDLER(IDN_TREE_CLICK, OnTreeClick)
    COMMAND_CODE_HANDLER(IDN_TREE_RETURN, OnTreeReturn)

    // file menu
    COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
    COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
    COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
    COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
    COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnFileSaveAs)
    COMMAND_ID_HANDLER(ID_FILE_VALIDATE, OnFileValidate)
    COMMAND_RANGE_HANDLER(ID_EXPORT_BASE,ID_EXPORT_BASE+19, OnToolsExport)
    COMMAND_RANGE_HANDLER(ID_IMPORT_BASE,ID_IMPORT_BASE+19, OnToolsImport)
    COMMAND_RANGE_HANDLER(ID_FILE_MRU_FIRST,ID_FILE_MRU_LAST,OnFileOpenMRU)

    // edit menu
    COMMAND_ID_HANDLER(ID_EDIT_INCSEARCH, OnEditIncSearch)
    COMMAND_ID_HANDLER(ID_EDIT_ADDBINARY, OnEditAddBinary)
    COMMAND_ID_HANDLER(ID_EDIT_FIND, OnEditFind)
    COMMAND_ID_HANDLER(ID_EDIT_FINDNEXT, OnEditFind)
    COMMAND_ID_HANDLER(ID_EDIT_REPLACE, OnEditFind)

    // view menu
    COMMAND_ID_HANDLER(ATL_IDW_BAND_FIRST, OnViewToolBar)
    COMMAND_ID_HANDLER(ATL_IDW_BAND_FIRST+1, OnViewToolBar)
    COMMAND_ID_HANDLER(ATL_IDW_BAND_FIRST+2, OnViewToolBar)
    COMMAND_ID_HANDLER(ATL_IDW_BAND_FIRST+3, OnViewToolBar)
    COMMAND_ID_HANDLER(ATL_IDW_BAND_FIRST+4, OnViewToolBar)
    COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
    COMMAND_ID_HANDLER(ID_VIEW_TREE, OnViewTree)
    COMMAND_ID_HANDLER(ID_VIEW_DESC, OnViewDesc)
    COMMAND_ID_HANDLER(ID_VIEW_BODY, OnViewBody)
    COMMAND_ID_HANDLER(ID_VIEW_SOURCE, OnViewSource)
    COMMAND_ID_HANDLER(ID_VIEW_OPTIONS, OnViewOptions)

    // tools menu
    COMMAND_ID_HANDLER(ID_TOOLS_WORDS, OnToolsWords)
    COMMAND_ID_HANDLER(ID_TOOLS_OPTIONS, OnToolsOptions)
    COMMAND_RANGE_HANDLER(ID_SCRIPT_BASE, ID_SCRIPT_BASE + 999, OnToolsScript)

    // help menu
    COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)

    // navigation commands
    COMMAND_ID_HANDLER(ID_SELECT_TREE, OnSelectCtl)
    COMMAND_ID_HANDLER(ID_SELECT_ID, OnSelectCtl)
    COMMAND_ID_HANDLER(ID_SELECT_HREF, OnSelectCtl)
    COMMAND_ID_HANDLER(ID_SELECT_TEXT, OnSelectCtl)
    COMMAND_ID_HANDLER(ID_NEXT_ITEM, OnNextItem)

    // editor notifications
    COMMAND_CODE_HANDLER(IDN_SEL_CHANGE, OnEdSelChange)
    COMMAND_CODE_HANDLER(IDN_ED_CHANGED, OnEdChange)
    COMMAND_CODE_HANDLER(IDN_ED_TEXT, OnEdStatusText)
    COMMAND_CODE_HANDLER(IDN_WANTFOCUS, OnEdWantFocus)
    COMMAND_CODE_HANDLER(IDN_ED_RETURN, OnEdReturn)
    COMMAND_CODE_HANDLER(IDN_NAVIGATE, OnNavigate)
    COMMAND_CODE_HANDLER(EN_KILLFOCUS, OnEdKillFocus)
    COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnCbEdChange)
    COMMAND_CODE_HANDLER(CBN_SELENDOK, OnCbSelEndOk)
    COMMAND_HANDLER(IDC_HREF,CBN_SETFOCUS, OnCbSetFocus)

    // source code editor notifications
    NOTIFY_CODE_HANDLER(SCN_MODIFIED, OnSciModified)
    NOTIFY_CODE_HANDLER(SCN_MARGINCLICK, OnSciMarginClick)

    // tree pane
    COMMAND_ID_HANDLER(ID_PANE_CLOSE, OnViewTree)

    // chain commands to active view
    MESSAGE_HANDLER(WM_COMMAND, OnUnhandledCommand)

    CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
    CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
  END_MSG_MAP()
    
  LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnPostCreate(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnSettingChange(UINT, WPARAM, LPARAM, BOOL&) {
    if (m_doc)
      m_doc->ApplyConfChanges();
    return 0;
  }
  LRESULT OnUnhandledCommand(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnSetFocus(UINT, WPARAM, LPARAM, BOOL&) {
    m_view.SetFocus();
    return 0;
  }
  LRESULT OnDropFiles(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnSetStatusText(UINT, WPARAM, LPARAM lParam, BOOL&) {
    m_status_msg=(const TCHAR *)lParam;
    return 0;
  }
  LRESULT OnTrackPopupMenu(UINT, WPARAM, LPARAM lParam, BOOL&) {
    AU::TRACKPARAMS   *tp=(AU::TRACKPARAMS *)lParam;
    m_CmdBar.TrackPopupMenu(tp->hMenu,tp->uFlags,tp->x,tp->y);
    return 0;
  }

  LRESULT OnChar(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnPreCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled=FALSE;
    if ((HIWORD(wParam)==0 || HIWORD(wParam)==1) && LOWORD(wParam)!=ID_EDIT_INCSEARCH)
      StopIncSearch(true);
    return 0;
  }

  LRESULT OnFileExit(WORD, WORD, HWND, BOOL&) {
    PostMessage(WM_CLOSE);
    return 0;
  }
  LRESULT OnFileNew(WORD, WORD, HWND, BOOL&);
  LRESULT OnFileOpen(WORD, WORD, HWND, BOOL&);
  LRESULT OnFileOpenMRU(WORD, WORD, HWND, BOOL&);
  LRESULT OnFileSave(WORD, WORD, HWND, BOOL&);
  LRESULT OnFileSaveAs(WORD, WORD, HWND, BOOL&);
  LRESULT OnFileValidate(WORD, WORD, HWND, BOOL&);
  LRESULT OnToolsImport(WORD, WORD, HWND, BOOL&);
  LRESULT OnToolsExport(WORD, WORD, HWND, BOOL&);

  LRESULT OnEditIncSearch(WORD, WORD, HWND, BOOL&);
  LRESULT OnEditAddBinary(WORD, WORD, HWND, BOOL&);
  LRESULT OnEditFind(WORD, WORD, HWND, BOOL& bHandled) {
    if (!IsSourceActive())
      ShowView(BODY);
    bHandled=FALSE;
    return 0;
  }

  LRESULT OnViewToolBar(WORD, WORD, HWND, BOOL&);
  LRESULT OnViewStatusBar(WORD, WORD, HWND, BOOL&);
  LRESULT OnViewTree(WORD, WORD, HWND, BOOL&);
  LRESULT OnViewDesc(WORD, WORD, HWND, BOOL&) {
    ShowView(DESC);
    return 0;
  }
  LRESULT OnViewBody(WORD, WORD, HWND, BOOL&) {
    ShowView(BODY);
    return 0;
  }
  LRESULT OnViewSource(WORD, WORD, HWND, BOOL&) {
    ShowView(SOURCE);
    return 0;
  }
  LRESULT OnViewOptions(WORD, WORD, HWND, BOOL&);

  LRESULT OnToolsWords(WORD, WORD, HWND, BOOL&);
  LRESULT OnToolsOptions(WORD, WORD, HWND, BOOL&);
  LRESULT OnToolsScript(WORD, WORD, HWND, BOOL&);

  LRESULT OnAppAbout(WORD, WORD, HWND, BOOL&);

  LRESULT OnSelectCtl(WORD, WORD, HWND, BOOL&);
  LRESULT OnNextItem(WORD, WORD, HWND, BOOL&);

  LRESULT OnEdSelChange(WORD, WORD, HWND hWndCtl, BOOL&) {
    m_sel_changed=true;
    StopIncSearch(true);
    return 0;
  }
  LRESULT OnEdStatusText(WORD, WORD, HWND hWndCtl, BOOL&) {
    StopIncSearch(true);
    m_status.SetText(ID_DEFAULT_PANE,(const TCHAR *)hWndCtl);
    return 0;
  }
  LRESULT OnEdWantFocus(WORD, WORD wID, HWND, BOOL&) {
    m_want_focus=wID;
    return 0;
  }
  LRESULT OnEdReturn(WORD, WORD, HWND, BOOL&) {
    m_view.SetFocus();
    return 0;
  }
  LRESULT OnNavigate(WORD, WORD, HWND, BOOL&);
  LRESULT OnCbSetFocus(WORD, WORD, HWND, BOOL&) {
    if (!m_cb_updated) {
      m_ignore_cb_changes=true;
      CString   str(U::GetWindowText(m_href));
      m_href_box.ResetContent();
      m_href.SetWindowText(str);
      m_href.SetSel(0,str.GetLength()+1);
      m_ignore_cb_changes=false;
      if (m_cb_last_images)
	m_doc->BinIDsToComboBox(m_href_box);
      else
	m_doc->ParaIDsToComboBox(m_href_box); 
      m_cb_updated=true;
    }
    return 0;
  }

  LRESULT OnEdChange(WORD, WORD, HWND, BOOL&) {
    StopIncSearch(true);
    m_doc_changed=true;
    m_cb_updated=false;
    return 0;
  }
  LRESULT OnCbEdChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnCbSelEndOk(WORD code, WORD wID, HWND hWnd, BOOL&) {
    PostMessage(WM_COMMAND,MAKELONG(wID,CBN_EDITCHANGE),(LPARAM)hWnd);
    return 0;
  }

  LRESULT OnEdKillFocus(WORD, WORD, HWND, BOOL&) {
    StopIncSearch(true);
    return 0;
  }

  LRESULT OnTreeReturn(WORD, WORD, HWND, BOOL&);
  LRESULT OnTreeClick(WORD, WORD, HWND, BOOL&);

  LRESULT OnSciModified(int id,NMHDR *hdr,BOOL& bHandled) {
    if (hdr->hwndFrom!=m_source) {
      bHandled=FALSE;
      return 0;
    }
    SciModified(*(SCNotification*)hdr);
    return 0;
  }

  LRESULT OnSciMarginClick(int id,NMHDR *hdr,BOOL& bHandled) {
    if (hdr->hwndFrom!=m_source) {
      bHandled=FALSE;
      return 0;
    }
    SciMarginClicked(*(SCNotification*)hdr);
    return 0;
  }

  void	SciModified(const SCNotification& scn);
  void	SciMarginClicked(const SCNotification& scn);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__38D356D4_C28B_47B0_A7AD_8C6B70F7F283__INCLUDED_)
