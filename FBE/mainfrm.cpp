// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <dlgs.h>
#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include "FBEView.h"
#include "FBDoc.h"
#include "TreeView.h"
#include "OptDlg.h"
#include "ContainerWnd.h"
#include "Scintilla.h"
#include "MainFrm.h"
#include "FBE.h"
#include "Words.h"
#include "SearchReplace.h"

// utility methods
bool  CMainFrame::IsBandVisible(int id) {
  int nBandIndex = m_rebar.IdToIndex(id);
  REBARBANDINFO	rbi;
  rbi.cbSize=sizeof(rbi);
  rbi.fMask=RBBIM_STYLE;
  m_rebar.GetBandInfo(nBandIndex,&rbi);
  return (rbi.fStyle&RBBS_HIDDEN)==0;
}

void  CMainFrame::AttachDocument(FB::Doc *doc) {
  if (IsSourceActive()) {
    UIEnable(ID_VIEW_TREE, 1);
    UISetCheck(ID_VIEW_TREE, m_save_sp_mode);
    m_splitter.SetSinglePaneMode(m_save_sp_mode ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT);
  }
  m_view.AttachWnd(doc->m_body);
  m_view.AttachWnd(doc->m_desc);
  UISetCheck(ID_VIEW_BODY, 1);
  UISetCheck(ID_VIEW_DESC, 0);
  UISetCheck(ID_VIEW_SOURCE, 0);
  m_view.ActivateWnd(doc->m_body);
  m_view.SetFocus();
  m_cb_updated=false;
  m_need_title_update=m_sel_changed=true;
  m_tree.GetDocumentStructure(doc->m_body);
  m_tree.HighlightItemAtPos(doc->m_body.SelectionContainer());
}

CString	CMainFrame::GetOpenFileName() {
  CFileDialog	dlg(
    TRUE,
    _T("fb2"),
    NULL,
    OFN_HIDEREADONLY|OFN_PATHMUSTEXIST,
    _T("FictionBook files (*.fb2)\0*.fb2\0All files (*.*)\0*.*\0\0")
  );
  if (dlg.DoModal(*this)==IDOK)
    return dlg.m_szFileName;
  return CString();
}

class CCustomSaveDialog : public CFileDialogImpl<CCustomSaveDialog>
{
public:
  HWND	      m_hDlg;
  CString     m_encoding;

  CCustomSaveDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
    LPCTSTR lpszDefExt = NULL,
    LPCTSTR lpszFileName = NULL,
    DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
    LPCTSTR lpszFilter = NULL,
    HWND hWndParent = NULL)
    : CFileDialogImpl<CCustomSaveDialog>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent),
      m_hDlg(NULL)
  {
    m_ofn.lpTemplateName=MAKEINTRESOURCE(IDD_CUSTOMSAVEDLG);
  }

  BEGIN_MSG_MAP(CCustomSaveDialog)
    if (uMsg==WM_INITDIALOG)
      return OnInitDialog(hWnd,uMsg,wParam,lParam);

    MESSAGE_HANDLER(WM_SIZE, OnSize);

    CHAIN_MSG_MAP(CFileDialogImpl<CCustomSaveDialog>)
  END_MSG_MAP()

  LRESULT OnInitDialog(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    m_hDlg=hWnd;

    TCHAR   buf[1024];

    if (::LoadString(_Module.GetResourceInstance(),IDS_ENCODINGS,buf,sizeof(buf)/sizeof(buf[0]))==0)
      return TRUE;

    TCHAR   *cp=buf;
    while (*cp) {
      size_t len=_tcscspn(cp,_T(","));
      if (cp[len])
	cp[len++]=_T('\0');
      if (*cp)
	::SendDlgItemMessage(hWnd,IDC_ENCODING,CB_ADDSTRING,0,(LPARAM)cp);
      cp+=len;
    }

    m_encoding=U::GetSettingS(_T("DefaultSaveEncoding"),_T("utf-8"));
    ::SetDlgItemText(hWnd,IDC_ENCODING,m_encoding);

    return TRUE;
  }

  LRESULT OnSize(UINT uMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled) {
    // make combobox the same size as std controls
    RECT    rc_std,rc_my;
    HWND    hCB=::GetDlgItem(m_hDlg,IDC_ENCODING);
    ::GetWindowRect(hCB,&rc_my);
    ::GetWindowRect(GetFileDialogWindow().GetDlgItem(cmb1),&rc_std);
    POINT   pt={rc_my.left,rc_my.top};
    ::ScreenToClient(m_hDlg,&pt);
    ::MoveWindow(hCB,pt.x,pt.y,rc_std.right-rc_std.left,rc_my.bottom-rc_my.top,TRUE);

    return 0;
  }
  
  BOOL OnFileOK(LPOFNOTIFY on) {
    m_encoding=U::GetWindowText(::GetDlgItem(m_hDlg,IDC_ENCODING));
    _Settings.SetStringValue(_T("DefaultSaveEncoding"),m_encoding);
    return TRUE;
  }
};

CString	CMainFrame::GetSaveFileName(CString& encoding) {
  CCustomSaveDialog	dlg(
    FALSE,
    _T("fb2"),
    NULL,
    OFN_HIDEREADONLY|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT|
    OFN_ENABLETEMPLATE,
    _T("FictionBook files (*.fb2)\0*.fb2\0All files (*.*)\0*.*\0\0")
  );
  if (dlg.DoModal(*this)==IDOK) {
    encoding=dlg.m_encoding;
    return dlg.m_szFileName;
  }
  return CString();
}

bool	CMainFrame::DocChanged() {
  return m_doc && m_doc->DocChanged() || IsSourceActive() && m_source.SendMessage(SCI_GETMODIFY);
}

bool	CMainFrame::DiscardChanges() {
  if (DocChanged())
    switch (U::MessageBox(MB_YESNOCANCEL|MB_ICONEXCLAMATION,_T("FBE"),
	      _T("Save changes to %s?"),(const TCHAR *)m_doc->m_filename))
    {
    case IDYES:
      return SaveFile(false)==OK;
    case IDNO:
      return true;
    case IDCANCEL:
      return false;
    }
  return true;
}

void  CMainFrame::SetIsText() {
  if (m_is_fail)
    m_status.SetText(ID_DEFAULT_PANE,_T("Failing Incremental Search: ")+m_is_str);
  else
    m_status.SetText(ID_DEFAULT_PANE,_T("Incremental Search: ")+m_is_str);
}

void  CMainFrame::StopIncSearch(bool fCancel) {
  if (!m_incsearch)
    return;
  m_incsearch=0;
  m_sel_changed=true; // will cause status line update soon
  if (fCancel)
    m_doc->m_body.CancelIncSearch();
  else
    m_doc->m_body.StopIncSearch();
}

CMainFrame::FILE_OP_STATUS CMainFrame::SaveFile(bool askname) {
  ATLASSERT(m_doc!=NULL);

  // force consistent html view
  if (IsSourceActive() && !SourceToHTML())
    return FAIL;

  if (askname || !m_doc->m_namevalid) { // ask user about save file name
    CString encoding;
    CString filename(GetSaveFileName(encoding));
    if (filename.IsEmpty())
      return CANCELLED;
    m_doc->m_encoding=encoding;
    if (m_doc->Save(filename)) {
      m_doc->m_filename=filename;
      m_doc->m_namevalid=true;
      return OK;
    }
    return FAIL;
  }
  return m_doc->Save() ? OK : FAIL;
}

CMainFrame::FILE_OP_STATUS  CMainFrame::LoadFile(const wchar_t *initfilename) {
  if (!DiscardChanges())
    return CANCELLED;
  
  CString filename(initfilename);
  if (filename.IsEmpty())
    filename=GetOpenFileName();
  if (filename.IsEmpty())
    return CANCELLED;
  
  FB::Doc *doc=new FB::Doc(*this);

  EnableWindow(FALSE);
  m_status.SetPaneText(ID_DEFAULT_PANE,_T("Loading..."));
  bool fLoaded=doc->Load(m_view,filename);
  EnableWindow(TRUE);
  if (!fLoaded) {
    delete doc;
    return FAIL;
  }

  AttachDocument(doc);
  delete m_doc;
  m_doc=doc;
  return OK;
}

void  CMainFrame::GetDocumentStructure() {
  m_doc_changed=false;
  m_tree.GetDocumentStructure(m_doc->m_body);
}

void  CMainFrame::GoTo(MSHTML::IHTMLElement *e) {
  try {
    m_doc->m_body.GoTo(e);
    ShowView();
  }
  catch (_com_error&) {
  }
}

// message handlers
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
  // provide an easy way to enter unicode chars
  if (pMsg->message==WM_KEYDOWN && ::GetKeyState(VK_CONTROL)&0x8000) {
    wchar_t   c=0;
    switch (pMsg->wParam) {
    case VK_OEM_4: c=L'\x2018'; break;
    case VK_OEM_6: c=L'\x2019'; break;
    case VK_OEM_1: c=L'\x201C'; break;
    case VK_OEM_7: c=L'\x201D'; break;
    case VK_OEM_MINUS: c=L'\x2013'; break;
    case VK_OEM_PLUS: c=L'\x2014'; break;
    case VK_OEM_PERIOD: c=L'\x2026'; break;
    }
    if (c) {
      pMsg->message=WM_CHAR;
      pMsg->wParam=c;
    }
  }

  // well, if we are doing an incremental search, then swallow WM_CHARS
  if (m_incsearch && pMsg->hwnd!=*this) {
    BOOL tmp;
    if (pMsg->message==WM_CHAR) {
      OnChar(WM_CHAR,pMsg->wParam,0,tmp);
      return TRUE;
    }
    if ((pMsg->message==WM_KEYDOWN || pMsg->message==WM_KEYUP) &&
	(pMsg->wParam==VK_BACK || pMsg->wParam==VK_RETURN))
    {
      if (pMsg->message==WM_KEYDOWN)
	OnChar(WM_CHAR,pMsg->wParam,0,tmp);
      return TRUE;
    }
  }

  // let other windows do their translations
  if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
    return TRUE;

  // this is needed to pass certain keys to the web browser
  HWND	  hWndFocus=::GetFocus();
  if (m_doc) {
    if (::IsChild(m_doc->m_body,hWndFocus)) {
      if (m_doc->m_body.PreTranslateMessage(pMsg))
	return TRUE;
    } else if (::IsChild(m_doc->m_desc,hWndFocus)) {
      if (m_doc->m_desc.PreTranslateMessage(pMsg))
	return TRUE;
    }
  }

  return FALSE;
}

void  CMainFrame::UIUpdateViewCmd(CFBEView& view,WORD wID,OLECMD& oc,const wchar_t *hk)
{
  CString   fbuf;
  fbuf.Format(_T("%s\t%s"),(const TCHAR *)view.QueryCmdText(oc.cmdID),hk);
  UISetText(wID,fbuf);
  UIEnable(wID,(oc.cmdf & OLECMDF_ENABLED)!=0);
}

BOOL CMainFrame::OnIdle()
{
  if (IsSourceActive()) {
    static WORD	  disabled_commands[]={
      ID_EDIT_BOLD, ID_EDIT_ITALIC,
      ID_EDIT_CLONE, ID_EDIT_SPLIT, ID_EDIT_MERGE,
      ID_EDIT_REMOVE_OUTER_SECTION,
      ID_STYLE_NORMAL, ID_STYLE_TEXTAUTHOR, ID_STYLE_SUBTITLE,
      ID_STYLE_LINK, ID_STYLE_NOTE, ID_STYLE_NOLINK,
      ID_EDIT_ADD_BODY, ID_EDIT_ADD_TITLE, ID_EDIT_ADD_EPIGRAPH,
      ID_EDIT_ADD_IMAGE, ID_EDIT_ADD_ANN, ID_EDIT_ADD_TA,
      ID_EDIT_INS_IMAGE, ID_EDIT_INS_POEM, ID_EDIT_INS_CITE,ID_EDIT_ADDBINARY,
      ID_VIEW_TREE
    };
    for (int i=0;i<sizeof(disabled_commands)/sizeof(disabled_commands[0]);++i)
      UIEnable(disabled_commands[i],FALSE);
    m_id_box.EnableWindow(FALSE);
    m_href_box.EnableWindow(FALSE);
    // ...
    bool fCanCC=m_source.SendMessage(SCI_GETSELECTIONSTART)!=
		m_source.SendMessage(SCI_GETSELECTIONEND);
    UIEnable(ID_EDIT_COPY,fCanCC);
    UIEnable(ID_EDIT_CUT,fCanCC);
    UIEnable(ID_EDIT_PASTE,m_source.SendMessage(SCI_CANPASTE));
    if (m_source.SendMessage(SCI_CANUNDO)) {
      UISetText(ID_EDIT_UNDO,_T("&Undo"));
      UIEnable(ID_EDIT_UNDO, 1);
    } else {
      UISetText(ID_EDIT_UNDO,_T("Can't undo"));
      UIEnable(ID_EDIT_UNDO, 0);
    }
    if (m_source.SendMessage(SCI_CANREDO)) {
      UISetText(ID_EDIT_REDO,_T("&Redo"));
      UIEnable(ID_EDIT_REDO, 1);
    } else {
      UISetText(ID_EDIT_REDO,_T("Can't redo"));
      UIEnable(ID_EDIT_REDO, 0);
    }

    m_status.SetPaneText(ID_PANE_INS,
      m_source.SendMessage(SCI_GETOVERTYPE) ? _T("OVR") : _T("INS"));
  } else {
    // check if editing commands can be performed
    CString     fbuf;
    CFBEView&   view=ActiveView();

    static OLECMD	mshtml_commands[]={
      { IDM_REDO }, // 0
      { IDM_UNDO }, // 1
      { IDM_COPY }, // 2
      { IDM_CUT },  // 3
      { IDM_PASTE }, // 4
      { IDM_UNLINK }, // 5
      { IDM_BOLD }, // 6
      { IDM_ITALIC }, // 7
    };
    view.QueryStatus(mshtml_commands,sizeof(mshtml_commands)/sizeof(mshtml_commands[0]));

    static WORD	fbe_commands[]={
      ID_EDIT_REDO,
      ID_EDIT_UNDO,
      ID_EDIT_COPY,
      ID_EDIT_CUT,
      ID_EDIT_PASTE,
      ID_STYLE_NOLINK,
      ID_EDIT_BOLD,
      ID_EDIT_ITALIC
    };

    for (int jj=0;jj<sizeof(mshtml_commands)/sizeof(mshtml_commands[0]);++jj) {
      DWORD flags=mshtml_commands[jj].cmdf;
      WORD  cmd=fbe_commands[jj];
      UIEnable(cmd,(flags & OLECMDF_ENABLED)!=0);
      UISetCheck(cmd,(flags & OLECMDF_LATCHED)!=0);
    }
    UIUpdateViewCmd(view,ID_EDIT_REDO,mshtml_commands[0],_T("Ctrl+Y"));
    UIUpdateViewCmd(view,ID_EDIT_UNDO,mshtml_commands[1],_T("Ctrl+Z"));

    UIEnable(ID_EDIT_FINDNEXT,view.CanFindNext());

    UIUpdateViewCmd(view,ID_STYLE_LINK);
    UIUpdateViewCmd(view,ID_STYLE_NOTE);
    UIUpdateViewCmd(view,ID_STYLE_NORMAL);
    UIUpdateViewCmd(view,ID_STYLE_SUBTITLE);
    UIUpdateViewCmd(view,ID_STYLE_TEXTAUTHOR);
    UIUpdateViewCmd(view,ID_EDIT_ADD_TITLE);
    UIUpdateViewCmd(view,ID_EDIT_ADD_BODY);
    UIUpdateViewCmd(view,ID_EDIT_ADD_TA);
    UIUpdateViewCmd(view,ID_EDIT_CLONE);
    UIUpdateViewCmd(view,ID_EDIT_INS_IMAGE);
    UIUpdateViewCmd(view,ID_EDIT_ADD_IMAGE);
    UIUpdateViewCmd(view,ID_EDIT_ADD_EPIGRAPH);
    UIUpdateViewCmd(view,ID_EDIT_ADD_ANN);
    UIUpdateViewCmd(view,ID_EDIT_SPLIT);
    UIUpdateViewCmd(view,ID_EDIT_INS_POEM);
    UIUpdateViewCmd(view,ID_EDIT_INS_CITE);
    UIUpdateViewCmd(view,ID_EDIT_MERGE);
    UIUpdateViewCmd(view,ID_EDIT_REMOVE_OUTER_SECTION);

    if (m_sel_changed && GetCurView()!=DESC) {
      m_status.SetPaneText(ID_DEFAULT_PANE,m_doc->m_body.SelPath());
      // update hrefs & IDs
      try {
	MSHTML::IHTMLElementPtr an(m_doc->m_body.SelectionAnchor());
	_variant_t    href;
	if (an)
	  href.Attach(an->getAttribute(L"href",2));
	if ((bool)an && V_VT(&href)==VT_BSTR) {
	  m_href_box.EnableWindow();
	  m_ignore_cb_changes=true;
	  m_href.SetWindowText(V_BSTR(&href));
	  m_ignore_cb_changes=false;
	  bool img=U::scmp(an->tagName,L"DIV")==0;
	  if (img!=m_cb_last_images)
	    m_cb_updated=false;
	  m_cb_last_images=img;
	} else {
	  m_href_box.SetWindowText(_T(""));
	  m_href_box.EnableWindow(FALSE);
	}
	MSHTML::IHTMLElementPtr	    sc(m_doc->m_body.SelectionStructCon());
	if (sc) {
	  m_id_box.EnableWindow();
	  m_ignore_cb_changes=true;
	  m_id.SetWindowText(sc->id);
	  m_ignore_cb_changes=false;
	} else
	  m_id_box.EnableWindow(FALSE);
      }
      catch (_com_error&) { }
      // update current tree node
      if (!m_doc_changed)
	m_tree.HighlightItemAtPos(m_doc->m_body.SelectionContainer()); // locate an appropriate tree node
      m_sel_changed=false;
    }

    // insert/overwrite mode
    OLECMD  oc={IDM_OVERWRITE};
    view.QueryStatus(&oc,1);
    bool fOvr=(oc.cmdf & OLECMDF_LATCHED)!=0;
    if (fOvr!=m_last_ovr_state) {
      m_last_ovr_state=fOvr;
      m_status.SetPaneText(ID_PANE_INS,fOvr ? _T("OVR") : _T("INS"));
    }
  }

  // update ui
  UIUpdateToolBar();

  // update document tree
  if (m_doc_changed) {
    MSHTML::IHTMLDOMNodePtr   chp(m_doc->m_body.GetChangedNode());
    if ((bool)chp && m_tree_pane.IsWindowVisible()) {
      m_tree.UpdateDocumentStructure(m_doc->m_body,chp);
      m_tree.HighlightItemAtPos(m_doc->m_body.SelectionContainer());
    }
    m_doc_changed=false;
  }

  // focus some stupid control if requested
  BOOL tmp;
  switch (m_want_focus) {
  case IDC_ID:
    OnSelectCtl(0,ID_SELECT_ID,0,tmp);
    break;
  case IDC_HREF:
    OnSelectCtl(0,ID_SELECT_HREF,0,tmp);
    break;
  }
  m_want_focus=0;

  // install a posted status line message
  if (!m_status_msg.IsEmpty()) {
    m_status.SetPaneText(ID_DEFAULT_PANE,m_status_msg);
    m_status_msg.Empty();
  }

  // see if we need to update title
  if (m_need_title_update || m_change_state!=DocChanged()) {
    m_need_title_update=false;
    m_change_state=DocChanged();
    CString   tt(U::GetFileTitle(m_doc->m_filename));
    tt+=m_change_state ? _T(" +") : _T(" -");
    SetWindowText(tt + _T(" FB Editor"));
  }

  return FALSE;
}

static void AddTbButton(HWND hWnd,const TCHAR *text) {
  TBBUTTON    sep;
  memset(&sep,0,sizeof(sep));
  sep.iBitmap=I_IMAGENONE;
  sep.fsStyle=BTNS_BUTTON|BTNS_AUTOSIZE;
  sep.iString=(int)text;
  ::SendMessage(hWnd,TB_ADDBUTTONS,1,(LPARAM)&sep);
}

void	CMainFrame::InitPluginsType(HMENU hMenu,const TCHAR *type,
				    UINT cmdbase,CSimpleArray<CLSID>& plist)
{
  CRegKey   rk;

  if (rk.Open(HKEY_LOCAL_MACHINE,_SettingsPath+_T("\\Plugins"))!=ERROR_SUCCESS)
    return;
  int	ncmd=0;
  for (int i=0;ncmd<20;++i) {
    CString   name;
    DWORD     size=128; // enough for guids
    TCHAR     *cp=name.GetBuffer(size);
    FILETIME  ft;
    if (::RegEnumKeyEx(rk,i,cp,&size,0,0,0,&ft)!=ERROR_SUCCESS)
      break;
    name.ReleaseBuffer(size);
    CRegKey   pk;
    if (pk.Open(rk,name)!=ERROR_SUCCESS)
      continue;
    CString   pt(U::QuerySV(pk,_T("Type")));
    CString   ms(U::QuerySV(pk,_T("Menu")));
    if (pt.IsEmpty() || ms.IsEmpty() || pt!=type)
      continue;
    CLSID     clsid;
    if (::CLSIDFromString((TCHAR*)(const TCHAR *)name,&clsid)!=NOERROR)
      continue;
    // all checks pass, add to menu and remember clsid
    plist.Add(clsid);
    ::AppendMenu(hMenu,MF_STRING,cmdbase+ncmd,ms);
    // check if an icon is avaliable
    CString   icon(U::QuerySV(pk,_T("Icon")));
    if (!icon.IsEmpty()) {
      int   cp=icon.ReverseFind(_T(','));
      int   iconID;
      if (cp>0 && _stscanf((const TCHAR *)icon+cp,_T(",%d"),&iconID)==1)
	icon.Delete(cp,icon.GetLength()-cp);
      else
	iconID=0;

      // try load from file first
      HICON hIcon;
      if (::ExtractIconEx(icon,iconID,NULL,&hIcon,1)>0 && hIcon) {
	m_CmdBar.AddIcon(hIcon,cmdbase+ncmd);
	::DestroyIcon(hIcon);
      }
    }
    ++ncmd;
  }
  if (ncmd>0) // delete placeholder from menu
    ::RemoveMenu(hMenu,0,MF_BYPOSITION);
}

void	CMainFrame::InitPlugins() {
  HMENU file=::GetSubMenu(m_CmdBar.GetMenu(),0);
  for (int i=0;i<::GetMenuItemCount(file);++i) {
    HMENU sub=::GetSubMenu(file,i);
    if (sub) {
      CString	name;
      DWORD	size=128;
      name.ReleaseBuffer(::GetMenuString(file,i,name.GetBuffer(size),size,MF_BYPOSITION));
      if (name==_T("&Import"))
	InitPluginsType(sub,_T("Import"),ID_IMPORT_BASE,m_import_plugins);
      else if (name==_T("&Export"))
	InitPluginsType(sub,_T("Export"),ID_EXPORT_BASE,m_export_plugins);
      else if (name==_T("&Recent Files")) {
	m_mru.SetMenuHandle(sub);
	m_mru.ReadFromRegistry(_SettingsPath);
	m_mru.SetMaxEntries(m_mru.m_nMaxEntries_Max-1);
      }
    }
  }

  // scripts
  HMENU		  scripts = ::GetSubMenu(::GetSubMenu(m_CmdBar.GetMenu(),3),2);
  while (::GetMenuItemCount(scripts)>0)
    ::RemoveMenu(scripts,0,MF_BYPOSITION);
  m_scripts.RemoveAll();

  CString	  dir(U::GetProgDir());
  WIN32_FIND_DATA fd;
  HANDLE	  hFind = FindFirstFile(dir + _T("\\*.js"),&fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    StopScript();
    do {
      if (StartScript() < 0)
	continue;
      CString   sfn(dir + _T("\\") + fd.cFileName);
      if (SUCCEEDED(ScriptLoad(sfn))) {
	CComVariant vt;
	if (SUCCEEDED(ScriptCall(L"FBEScriptName",NULL,&vt)) && V_VT(&vt) == VT_BSTR && V_BSTR(&vt)) {
	  AppendMenu(scripts,MF_STRING,ID_SCRIPT_BASE + m_scripts.GetSize(),V_BSTR(&vt));
	  m_scripts.Add(sfn);
	}
      }
      StopScript();
    } while (FindNextFile(hFind,&fd));
    FindClose(hFind);
  }

  if (m_scripts.GetSize() == 0)
    AppendMenu(scripts,MF_STRING | MF_DISABLED | MF_GRAYED, IDCANCEL, _T("No scripts found"));
}

LRESULT CMainFrame::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
{
  // create command bar window
  m_CmdBar.SetAlphaImages(true);
  HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);

  // attach menu
  m_CmdBar.AttachMenu(GetMenu());

  // remove old menu
  SetMenu(NULL);
  
  // init plugins&MRU list
  InitPlugins();

  // load command bar images
  m_CmdBar.LoadImages(IDR_MAINFRAME);
  m_CmdBar.LoadImages(IDR_EXTRAICONS);

  HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
  UIAddToolBar(hWndToolBar);
 
  HWND hWndLinksBar = ::CreateWindowEx(0,TOOLBARCLASSNAME,NULL,
    ATL_SIMPLE_TOOLBAR_PANE_STYLE|TBSTYLE_LIST,0,0,100,100,m_hWnd,NULL,
    _Module.GetModuleInstance(),NULL);
  ::SendMessage(hWndLinksBar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
  AddTbButton(hWndLinksBar,_T("ID:"));
  AddTbButton(hWndLinksBar,_T("123456789012345678901234567890"));
  AddTbButton(hWndLinksBar,_T("Href:"));
  AddTbButton(hWndLinksBar,_T("123456789012345678901234567890"));

  CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
  AddSimpleReBarBand(hWndCmdBar,NULL,FALSE,0,TRUE);
  AddSimpleReBarBand(hWndToolBar,NULL,FALSE,0,TRUE);
  AddSimpleReBarBand(hWndLinksBar,NULL,FALSE,0,TRUE);
  m_rebar=m_hWndToolBar;

  // add editor controls
  HFONT	  hFont=(HFONT)::SendMessage(hWndLinksBar,WM_GETFONT,0,0);
  RECT	  rc;
  ::SendMessage(hWndLinksBar,TB_GETITEMRECT,1,(LPARAM)&rc);
  //rc.top++;
  rc.bottom--;
  m_id_box.Create(hWndLinksBar,rc,NULL,WS_CHILD|WS_VISIBLE|CBS_AUTOHSCROLL,WS_EX_CLIENTEDGE,IDC_ID);
  m_id_box.SetFont(hFont);
  m_id.SubclassWindow(m_id_box.ChildWindowFromPoint(CPoint(3,3)));

  ::SendMessage(hWndLinksBar,TB_GETITEMRECT,3,(LPARAM)&rc);
  //rc.top++;
  rc.bottom--;
  m_href_box.Create(hWndLinksBar,rc,NULL,WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWN|CBS_AUTOHSCROLL,WS_EX_CLIENTEDGE,IDC_HREF);
  m_href_box.SetFont(hFont);
  m_href.SubclassWindow(m_href_box.ChildWindowFromPoint(CPoint(3,3)));
  
  // create status bar
  CreateSimpleStatusBar();
  m_status.SubclassWindow(m_hWndStatusBar);
  int panes[]={ID_DEFAULT_PANE,ID_PANE_INS};
  m_status.SetPanes(panes,sizeof(panes)/sizeof(panes[0]));

  // create splitter
  m_hWndClient = m_splitter.Create(m_hWnd,rcDefault,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
  m_splitter.SetSplitterExtendedStyle(0);

  // create splitter contents
  m_tree_pane.Create(m_splitter);
  m_tree_pane.SetTitle(L"Document Tree");
  m_view.Create(m_splitter,rcDefault,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);

  // create a tree
  m_dummy_pane.Create(m_tree_pane,rcDefault,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,WS_EX_CLIENTEDGE);
  m_tree_pane.SetClient(m_dummy_pane);
  m_tree.Create(m_dummy_pane, rcDefault);
  m_tree.SetBkColor(::GetSysColor(COLOR_WINDOW));
  m_dummy_pane.SetSplitterPane(0,m_tree);
  m_dummy_pane.SetSinglePaneMode(SPLIT_PANE_LEFT);

  // create a source view
  m_source.Create(_T("Scintilla"),m_view,rcDefault,NULL,WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,0);
  m_view.AttachWnd(m_source);
  SetupSci();
  SetSciStyles();

  // initialize a new blank document
  m_doc=new FB::Doc(*this);
  if (_ARGV.GetSize()>0 && !_ARGV[0].IsEmpty()) { // load a command line arg if it was provided
    if (m_doc->Load(m_view,_ARGV[0]))
      m_mru.AddToList(_ARGV[0]);
    else
      return -1; // abort
  } else
    m_doc->CreateBlank(m_view);
  AttachDocument(m_doc);
  UISetCheck(ID_VIEW_BODY,1);

  if (AU::_ARGS.start_in_desc_mode)
    ShowView(DESC);

  // setup splitter
  m_splitter.SetSplitterPanes(m_tree_pane, m_view);

  // hide elements
  if (U::GetSettingI(_T("HideStatusBar"))) {
    m_status.ShowWindow(SW_HIDE);
    UISetCheck(ID_VIEW_STATUS_BAR, FALSE);
  } else
    UISetCheck(ID_VIEW_STATUS_BAR, 1);

  if (U::GetSettingI(_T("HideDocTree"))) {
    m_tree_pane.ShowWindow(SW_HIDE);
    UISetCheck(ID_VIEW_TREE, FALSE);
    m_splitter.SetSinglePaneMode(SPLIT_PANE_RIGHT);
  } else
    UISetCheck(ID_VIEW_TREE, 1);

  // load toolbar settings
  for (int j=ATL_IDW_BAND_FIRST;j<ATL_IDW_BAND_FIRST+5;++j)
    UISetCheck(j,TRUE);
  REBARBANDINFO   rbi;
  memset(&rbi,0,sizeof(rbi));
  rbi.cbSize=sizeof(rbi);
  rbi.fMask=RBBIM_SIZE|RBBIM_STYLE;
  CString     tbs(U::GetSettingS(_T("Toolbars")));
  const TCHAR *cp=tbs;
  for (int bn=0;;++bn) {
    const TCHAR	  *ce=_tcschr(cp,_T(';'));
    if (!ce)
      break;
    int	      id,style,cx;
    if (_stscanf(cp,_T("%d,%d,%d;"),&id,&style,&cx)!=3)
      break;
    cp=ce+1;
    int	      idx=m_rebar.IdToIndex(id);
    m_rebar.GetBandInfo(idx,&rbi);
    rbi.fStyle &= ~(RBBS_BREAK|RBBS_HIDDEN);
    style &= RBBS_BREAK|RBBS_HIDDEN;
    rbi.fStyle |= style;
    rbi.cx=cx;
    m_rebar.SetBandInfo(idx,&rbi);
    if (idx!=bn)
      m_rebar.MoveBand(idx,bn);
    UISetCheck(id,style & RBBS_HIDDEN ? FALSE : TRUE);
  }

  // delay resizing
  PostMessage(AU::WM_POSTCREATE);

  // register object for message filtering and idle updates
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  ATLASSERT(pLoop != NULL);
  pLoop->AddMessageFilter(this);
  pLoop->AddIdleHandler(this);

  // accept dropped files
  ::DragAcceptFiles(*this,TRUE);

  return 0;
}

LRESULT CMainFrame::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  bHandled=FALSE;
  return 0;
}

LRESULT CMainFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (DiscardChanges()) {
    _Settings.SetDWORDValue(_T("HideStatusBar"),m_status.IsWindowVisible()==0);
    _Settings.SetDWORDValue(_T("HideDocTree"),IsSourceActive() ? !m_save_sp_mode : m_tree_pane.IsWindowVisible()==0);
    _Settings.SetDWORDValue(_T("SplitterPos"),m_splitter.GetSplitterPos());
    WINDOWPLACEMENT wpl;
    wpl.length=sizeof(wpl);
    GetWindowPlacement(&wpl);
    ::RegSetValueEx(_Settings,_T("WindowPosition"),0,REG_BINARY,(BYTE*)&wpl,sizeof(wpl));
    m_mru.WriteToRegistry(_SettingsPath);
    // save toolbars state
    CString tbs;
    REBARBANDINFO  rbi;
    memset(&rbi,0,sizeof(rbi));
    rbi.cbSize=sizeof(rbi);
    rbi.fMask=RBBIM_ID|RBBIM_SIZE|RBBIM_STYLE;
    int	  num_bands=m_rebar.GetBandCount();
    for (int i=0;i<num_bands;++i) {
      m_rebar.GetBandInfo(i,&rbi);
      CString	bi;
      bi.Format(_T("%d,%d,%d;"),rbi.wID,rbi.fStyle,rbi.cx);
      tbs+=bi;
    }
    _Settings.SetStringValue(_T("Toolbars"),tbs);
    DefWindowProc(WM_CLOSE,0,0);
  }
  return 0;
}

LRESULT CMainFrame::OnPostCreate(UINT, WPARAM, LPARAM, BOOL&)
{
  //SetSplitterPos works best after the default WM_CREATE has been handled
  m_splitter.SetSplitterPos(U::GetSettingI(_T("SplitterPos"),200));
  return 0;
}

// search&replace in scintilla
CString	  SciSelection(CWindow source) {
  int	  start=source.SendMessage(SCI_GETSELECTIONSTART);
  int	  end=source.SendMessage(SCI_GETSELECTIONEND);

  if (start>=end)
    return CString();

  char	  *buffer=(char*)malloc(end-start+1);
  if (buffer==NULL)
    return CString();
  source.SendMessage(SCI_GETSELTEXT,0,(LPARAM)buffer);

  char	  *p=buffer;
  while (*p && *p!='\r' && *p!='\n')
    ++p;

  int	  wlen=::MultiByteToWideChar(CP_UTF8,0,buffer,p-buffer,NULL,0);
  CString ret;
  wchar_t *wp=ret.GetBuffer(wlen);
  if (wp)
    ::MultiByteToWideChar(CP_UTF8,0,buffer,p-buffer,wp,wlen);
  free(buffer);
  if (wp)
    ret.ReleaseBuffer(wlen);
  return ret;
}

class CSciFindDlg : public CFindDlgBase {
public:
  CWindow	m_source;

  CSciFindDlg(CFBEView *view,HWND src) :
    CFindDlgBase(view), m_source(src)
  {
    m_view->m_fo.pattern=SciSelection(src);
  }

  virtual void	DoFind() {
    GetData();
    if (m_view->SciFindNext(m_source,false,true)) {
      SaveString();
      SaveHistory();
      EndDialog(IDOK);
    }
  }
};

class CSciReplaceDlg : public CReplaceDlgBase {
public:
  CWindow	m_source;

  CSciReplaceDlg(CFBEView *view,HWND src) : 
    CReplaceDlgBase(view), m_source(src)
  {
    m_view->m_fo.pattern=SciSelection(src);
  }

  virtual void DoFind() {
    if (!m_view->SciFindNext(m_source,true,false))
      U::MessageBox(MB_OK|MB_ICONEXCLAMATION,_T("FBE"),
	_T("Finished searching for '%s'."),m_view->m_fo.pattern);
    else {
      SaveString();
      SaveHistory();
      m_selvalid=true;
      MakeClose();
    }
  }
  virtual void DoReplace() {
    if (m_selvalid) { // replace
      m_source.SendMessage(SCI_TARGETFROMSELECTION);
      DWORD   len=::WideCharToMultiByte(CP_UTF8,0,
		m_view->m_fo.replacement,m_view->m_fo.replacement.GetLength(),
		NULL,0,NULL,NULL);
      char  *tmp=(char *)malloc(len+1);
      if (tmp) {
	::WideCharToMultiByte(CP_UTF8,0,
		      m_view->m_fo.replacement,m_view->m_fo.replacement.GetLength(),
		      tmp,len,NULL,NULL);
	tmp[len]='\0';
	if (m_view->m_fo.fRegexp)
	  m_source.SendMessage(SCI_REPLACETARGETRE,len,(LPARAM)tmp);
	else
	  m_source.SendMessage(SCI_REPLACETARGET,len,(LPARAM)tmp);
	free(tmp);
      }
      m_selvalid=false;
    }
    DoFind();
  }
  virtual void DoReplaceAll() {
    if (m_view->m_fo.pattern.IsEmpty())
      return;

    // setup search flags
    int	    flags=0;
    if (m_view->m_fo.flags & CFBEView::FRF_WHOLE)
      flags|=SCFIND_WHOLEWORD;
    if (m_view->m_fo.flags & CFBEView::FRF_CASE)
      flags|=SCFIND_MATCHCASE;
    if (m_view->m_fo.fRegexp)
      flags|=SCFIND_REGEXP|SCFIND_POSIX;
    m_source.SendMessage(SCI_SETSEARCHFLAGS,flags,0);

    // setup target range
    int	  end=m_source.SendMessage(SCI_GETLENGTH);
    m_source.SendMessage(SCI_SETTARGETSTART,0);
    m_source.SendMessage(SCI_SETTARGETEND,end);

    // convert search pattern and replacement to utf8
    int	  patlen;
    char  *pattern=AU::ToUtf8(m_view->m_fo.pattern,patlen);
    if (pattern==NULL)
      return;
    int	  replen;
    char  *replacement=AU::ToUtf8(m_view->m_fo.replacement,replen);
    if (replacement==NULL) {
      free(pattern);
      return;
    }

    // find first match
    int pos=m_source.SendMessage(SCI_SEARCHINTARGET,patlen,(LPARAM)pattern);

    int   num_repl=0;

    if (pos!=-1 && pos<=end) {
      int   last_match=pos;

      m_source.SendMessage(SCI_BEGINUNDOACTION);
      while (pos!=-1) {
	int matchlen=m_source.SendMessage(SCI_GETTARGETEND)-m_source.SendMessage(SCI_GETTARGETSTART);
	int mvp=0;
	if (matchlen<=0) {
	  char	ch=(char)m_source.SendMessage(SCI_GETCHARAT,m_source.SendMessage(SCI_GETTARGETEND));
	  if (ch=='\r' || ch=='\n')
	    mvp=1;
	}
	int rlen=matchlen;
	if (m_view->m_fo.fRegexp)
	  rlen=m_source.SendMessage(SCI_REPLACETARGETRE,replen,(LPARAM)replacement);
	else
	  m_source.SendMessage(SCI_REPLACETARGET,replen,(LPARAM)replacement);
	end += rlen-matchlen;
	last_match=pos+rlen+mvp;
	if (last_match>=end)
	  pos=-1;
	else {
	  m_source.SendMessage(SCI_SETTARGETSTART,last_match);
	  m_source.SendMessage(SCI_SETTARGETEND,end);
	  pos=m_source.SendMessage(SCI_SEARCHINTARGET,patlen,(LPARAM)pattern);
	}
	++num_repl;
      }
      m_source.SendMessage(SCI_ENDUNDOACTION);
    }

    free(pattern);
    free(replacement);

    if (num_repl>0) {
      SaveString();
      SaveHistory();
      U::MessageBox(MB_OK,_T("Replace All"),_T("%d replacement(s) done."),num_repl);
      MakeClose();
      m_selvalid=false;
    } else
      U::MessageBox(MB_OK|MB_ICONEXCLAMATION,_T("FBE"),
	_T("Finished searching for '%s'."),m_view->m_fo.pattern);
  }
};

LRESULT CMainFrame::OnUnhandledCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HWND	  hFocus=::GetFocus();
  UINT	  idCtl=HIWORD(wParam);
  // only pass messages to the editors
  if (idCtl==0 || idCtl==1) {
    if (hFocus==m_id || hFocus==m_href || ::IsChild(m_id,hFocus) || ::IsChild(m_href,hFocus))
      return ::SendMessage(hFocus,WM_COMMAND,wParam,lParam);
    // we need to check that the focused window is a web browser indeed
    if (hFocus==m_view.GetActiveWnd() || ::IsChild(m_view.GetActiveWnd(),hFocus)) {
      if (IsSourceActive()) {
	switch (LOWORD(wParam)) {
	case ID_EDIT_UNDO:
	  m_source.SendMessage(SCI_UNDO);
	  break;
	case ID_EDIT_REDO:
	  m_source.SendMessage(SCI_REDO);
	  break;
	case ID_EDIT_CUT:
	  m_source.SendMessage(SCI_CUT);
	  break;
	case ID_EDIT_COPY:
	  m_source.SendMessage(SCI_COPY);
	  break;
	case ID_EDIT_PASTE:
	  m_source.SendMessage(SCI_PASTE);
	  break;
	case ID_EDIT_FIND: {
	  CSciFindDlg	dlg(&m_doc->m_body,m_source);
	  dlg.DoModal();
	  break; }
	case ID_EDIT_FINDNEXT:
	  m_doc->m_body.SciFindNext(m_source,false,true);
	  break;
	case ID_EDIT_REPLACE: {
	  CSciReplaceDlg dlg(&m_doc->m_body,m_source);
	  dlg.DoModal();
	  break; }
	}
      } else
	return ActiveView().SendMessage(WM_COMMAND,wParam,0);
    }
  }
  return 0;
}

LRESULT CMainFrame::OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HDROP	  hDrop=(HDROP)wParam;
  UINT	  nf=::DragQueryFile(hDrop,0xFFFFFFFF,NULL,0);
  CString buf;
  if (nf>0) {
    UINT    len=::DragQueryFile(hDrop,0,NULL,0);
    TCHAR   *cp=buf.GetBuffer(len+1);
    len=::DragQueryFile(hDrop,0,cp,len+1);
    buf.ReleaseBuffer(len);
  }
  ::DragFinish(hDrop);
  if (!buf.IsEmpty() && LoadFile(buf)==OK)
    m_mru.AddToList(m_doc->m_filename);
  return 0;
}

LRESULT CMainFrame::OnNavigate(WORD, WORD, HWND, BOOL&) {
  CString   url(m_doc->m_body.NavURL());
  if (!url.IsEmpty() && LoadFile(url)==OK)
    m_mru.AddToList(m_doc->m_filename);
  return 0;
}

// commands
LRESULT CMainFrame::OnFileNew(WORD, WORD, HWND, BOOL&)
{
  if (!DiscardChanges())
    return 0;

  FB::Doc *doc=new FB::Doc(*this);
  doc->CreateBlank(m_view);
  AttachDocument(doc);
  delete m_doc;
  m_doc=doc;

  return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD, WORD, HWND, BOOL& bHandled)
{
  if (LoadFile()==OK)
    m_mru.AddToList(m_doc->m_filename);
  return 0;
}

LRESULT CMainFrame::OnFileOpenMRU(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  CString   filename;
  TCHAR	    *buf=filename.GetBuffer(MAX_PATH);
  m_mru.GetFromList(wID,buf);
  filename.ReleaseBuffer();

  switch (LoadFile(filename)) {
  case OK:
    m_mru.MoveToTop(wID);
    break;
  case FAIL:
    m_mru.RemoveFromList(wID);
    break;
  case CANCELLED:
    break;
  }

  return 0;
}

LRESULT CMainFrame::OnFileSave(WORD, WORD, HWND, BOOL&)
{
  SaveFile(false);
  return 0;
}

LRESULT CMainFrame::OnFileSaveAs(WORD, WORD, HWND, BOOL&)
{
  SaveFile(true);
  return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD, WORD wID, HWND, BOOL&)
{
  int nBandIndex = m_rebar.IdToIndex(wID);
  BOOL bVisible = !IsBandVisible(wID);
  m_rebar.ShowBand(nBandIndex, bVisible);
  UISetCheck(wID, bVisible);
  UpdateLayout();
  return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD, WORD, HWND, BOOL&)
{
  BOOL bVisible = !m_status.IsWindowVisible();
  ::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
  UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
  UpdateLayout();
  return 0;
}

LRESULT CMainFrame::OnViewTree(WORD, WORD, HWND, BOOL&)
{
  if (IsSourceActive())
    return 0;

  BOOL bVisible = !m_tree_pane.IsWindowVisible();
  m_tree_pane.ShowWindow(bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
  UISetCheck(ID_VIEW_TREE, bVisible);
  m_splitter.SetSinglePaneMode(bVisible ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT);
  return 0;
}

LRESULT CMainFrame::OnViewOptions(WORD, WORD, HWND, BOOL&)
{
  if (ShowOptionsDialog()) {
    m_doc->ApplyConfChanges();
    SetSciStyles();
  }
  return 0;
}

LRESULT CMainFrame::OnToolsImport(WORD, WORD wID, HWND, BOOL&) {
  wID-=ID_IMPORT_BASE;
  if (wID<m_import_plugins.GetSize()) {
    try {
      IUnknownPtr			    unk;
      CheckError(unk.CreateInstance(m_import_plugins[wID]));

      CComQIPtr<IFBEImportPlugin>	    ipl(unk);

      IDispatchPtr  obj;
      _bstr_t	    filename;
      if (ipl) {
	BSTR	bs=NULL;
	HRESULT hr=ipl->Import((long)m_hWnd,&bs,&obj);
	CheckError(hr);
	filename.Assign(bs);
	if (hr!=S_OK)
	  return 0;
      } else {
	  U::MessageBox(MB_OK|MB_ICONERROR,_T("Import Error"),
	    _T("Plugin does not support the required interface"));
	  return 0;
      }

      MSXML2::IXMLDOMDocument2Ptr dom(obj);
      if (!(bool)dom)
	U::MessageBox(MB_OK|MB_ICONERROR,_T("Error"),
	  _T("Imported object does not support IXMLDOMDocument2 interface"));
      else if (DiscardChanges()) {
	FB::Doc *doc=new FB::Doc(*this);

	if (doc->LoadFromDOM(m_view,dom)) {
	  if (filename.length()>0) {
	    doc->m_filename=(const TCHAR *)filename;
	    if (doc->m_filename.GetLength()<4 || doc->m_filename.Right(4).CompareNoCase(_T(".fb2"))!=0)
	      doc->m_filename+=_T(".fb2");
	    doc->m_namevalid=true;
	  }
	  AttachDocument(doc);
	  delete m_doc;
	  m_doc=doc;
	  m_doc->ResetSavePoint();
	} else
	  delete doc;
      }
    }
    catch (_com_error& e) {
      U::ReportError(e);
    }
  }
  return 0;
}

LRESULT CMainFrame::OnToolsExport(WORD, WORD wID, HWND, BOOL&) {
  wID-=ID_EXPORT_BASE;
  if (wID<m_export_plugins.GetSize()) {
    try {
      IUnknownPtr			  unk;
      CheckError(unk.CreateInstance(m_export_plugins[wID]));

      CComQIPtr<IFBEExportPlugin>	  epl(unk);

      if (epl) {
	MSXML2::IXMLDOMDocument2Ptr	  dom(m_doc->CreateDOM(m_doc->m_encoding));
	_bstr_t				  filename;
	if (m_doc->m_namevalid) {
	  CString tmp(m_doc->m_filename);
	  if (tmp.GetLength()>=4 && tmp.Right(4).CompareNoCase(_T(".fb2"))==0)
	    tmp.Delete(tmp.GetLength()-4,4);
	  filename=(const TCHAR *)tmp;
	}
	if (dom)
	  CheckError(epl->Export((long)m_hWnd,filename,dom));
      } else {
	U::MessageBox(MB_OK|MB_ICONERROR,_T("Export Error"),
	  _T("Plugin does not support the required interface"));
	return 0;
      }
    }
    catch (_com_error& e) {
      U::ReportError(e);
    }
  }
  return 0;
}

extern "C" {
  extern const char *build_timestamp;
};

class CAboutDlg : public CDialogImpl<CAboutDlg> {
public:
  enum { IDD = IDD_ABOUTBOX };

  BEGIN_MSG_MAP(CAboutDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    CString stamp(build_timestamp);
    ::SetWindowText(GetDlgItem(IDC_BUILDSTAMP),stamp);
    return 0;
  }
  LRESULT OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
    EndDialog(wID);
    return 0;
  }
};

LRESULT CMainFrame::OnToolsWords(WORD, WORD, HWND, BOOL&) {
  ShowWordsDialog(*m_doc);
  return 0;
}

LRESULT CMainFrame::OnToolsOptions(WORD, WORD, HWND, BOOL&) {
  if (ShowToolsOptionsDialog()) {
    SetupSci();
    SetSciStyles();
  }
  return 0;
}

LRESULT CMainFrame::OnToolsScript(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  wID -= ID_SCRIPT_BASE;

  if (wID < m_scripts.GetSize()) {
    if (StartScript() >= 0) {
      if (SUCCEEDED(ScriptLoad(m_scripts[wID]))) {
	MSXML2::IXMLDOMDocument2Ptr dom(m_doc->CreateDOM(m_doc->m_encoding));
	if (dom) {
	  CComVariant arg;
	  V_VT(&arg) = VT_DISPATCH;
	  V_DISPATCH(&arg) = dom;
	  dom.AddRef();
	  if (SUCCEEDED(ScriptCall(L"Run",&arg,NULL))) {
	    m_doc->SetXML(dom);
	  }
	}
      }
      StopScript();
    }
  }

  return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD, WORD, HWND, BOOL&)
{
  CAboutDlg dlg;
  dlg.DoModal();
  return 0;
}

// navigation
LRESULT CMainFrame::OnSelectCtl(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  switch (wID) {
  case ID_SELECT_TREE:
    if (!m_tree_pane.IsWindowVisible())
      OnViewTree(0,0,0,bHandled);
    m_tree.SetFocus();
    break;
  case ID_SELECT_ID:
    if (!IsBandVisible(ATL_IDW_BAND_FIRST+2))
      OnViewToolBar(0,ATL_IDW_BAND_FIRST+2,NULL,bHandled);
    m_id.SetFocus();
    break;
  case ID_SELECT_HREF:
    if (!IsBandVisible(ATL_IDW_BAND_FIRST+2))
      OnViewToolBar(0,ATL_IDW_BAND_FIRST+2,NULL,bHandled);
    m_href.SetFocus();
    break;
  case ID_SELECT_TEXT:
    m_view.SetFocus();
    break;
  }
  return 0;
}

LRESULT CMainFrame::OnNextItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  ShowView(NEXT);
  return 1;
}

// editor notifications
LRESULT CMainFrame::OnCbEdChange(WORD, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (m_ignore_cb_changes)
    return 0;

  try {
    if (wID==IDC_HREF) {
      MSHTML::IHTMLElementPtr an(m_doc->m_body.SelectionAnchor());
      _variant_t    href;
      if (an)
	href=an->getAttribute(L"href",2);
      if ((bool)an && V_VT(&href)==VT_BSTR) {
	CString	    newhref(U::GetWindowText(m_href));

	if (U::scmp(an->tagName,L"DIV")==0) // must be an image
	  m_doc->m_body.ImgSetURL(an,newhref);
	else
	  an->setAttribute(L"href",_variant_t((const wchar_t *)newhref),0);
      } else {
        m_href_box.SetWindowText(_T(""));
	m_href_box.EnableWindow(FALSE);
      }
    }
    if (wID==IDC_ID) {
      MSHTML::IHTMLElementPtr		sc(m_doc->m_body.SelectionStructCon());
      if (sc)
	sc->id=(const wchar_t *)U::GetWindowText(m_id);
      else
	m_id_box.EnableWindow(FALSE);
    }
  }
  catch (_com_error&) { }

  return 0;
}

// tree view notifications
LRESULT CMainFrame::OnTreeReturn(WORD, WORD, HWND, BOOL&)
{
  CTreeItem ii(m_tree.GetSelectedItem());
  if (!ii.IsNull() && ii.GetData())
    GoTo((MSHTML::IHTMLElement *)ii.GetData());
  return 0;
}

LRESULT CMainFrame::OnTreeClick(WORD, WORD, HWND hWndCtl, BOOL&)
{
  CTreeItem ii(m_tree.GetSelectedItem());
  if (ii.GetData())
    GoTo((MSHTML::IHTMLElement*) ii.GetData());
  return 0;
}

// binary objects
LRESULT CMainFrame::OnEditAddBinary(WORD, WORD, HWND, BOOL&) {
  if (!m_doc)
    return 0;

  CFileDialog	dlg(
    TRUE,
    NULL,
    NULL,
    OFN_HIDEREADONLY|OFN_PATHMUSTEXIST,
    _T("All files (*.*)\0*.*\0\0")
  );

  if (dlg.DoModal(*this)==IDOK)
    m_doc->AddBinary(dlg.m_szFileName);

  return 0;
}

// incremental search
LRESULT CMainFrame::OnEditIncSearch(WORD, WORD, HWND, BOOL&) {
  if (IsSourceActive())
    return 0;

  if (m_incsearch==0) {
    ShowView();
    m_doc->m_body.StartIncSearch();
    m_is_str.Empty();
    m_is_prev=m_doc->m_body.LastSearchPattern();
    m_incsearch=1;
    m_is_fail=false;
    SetIsText();
  } else if (m_incsearch==1 && m_is_str.IsEmpty() && !m_is_prev.IsEmpty()) {
    m_incsearch=2;
    m_is_str.Empty();
    for (int i=0;i<m_is_prev.GetLength();++i)
      PostMessage(WM_CHAR,m_is_prev[i],0x20000000);
  } else if (!m_is_fail)
    m_doc->m_body.DoIncSearch(m_is_str,true);
  return 0;
}

LRESULT CMainFrame::OnChar(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
  if (!m_incsearch)
    return 0;
  // only a few keys are supported
  if (wParam==8) { // backspace
    if (!m_is_str.IsEmpty())
      m_is_str.Delete(m_is_str.GetLength()-1);
    if (!m_doc->m_body.DoIncSearch(m_is_str,false)) {
      m_is_fail=true;
      ::MessageBeep(MB_ICONEXCLAMATION);
    } else
      m_is_fail=false;
  } else if (wParam==13) { // enter
    StopIncSearch(false);
    return 0;
  } else if (wParam>=32 && wParam!=127) { // printable char
    if (m_is_fail) {
      ::MessageBeep(MB_ICONEXCLAMATION);
      if (!(lParam&0x20000000))
	return 0;
    }
    m_is_str+=(TCHAR)wParam;
    if (!m_doc->m_body.DoIncSearch(m_is_str,false)) {
      if (!m_is_fail)
	::MessageBeep(MB_ICONEXCLAMATION);
      m_is_fail=true;
    } else
      m_is_fail=false;
  }
  SetIsText();
  return 0;
}

bool  CMainFrame::SourceToHTML() {
  // don't copy text back if it wasnt changed
  if (!m_source.SendMessage(SCI_GETMODIFY))
    return true;

  int	  line,col;

  if (!m_doc->SetXMLAndValidate(m_source,false,line,col)) {
    int	pos=m_source.SendMessage(SCI_POSITIONFROMLINE,line-1);
    while (col--)
      pos=m_source.SendMessage(SCI_POSITIONAFTER,pos);
    m_source.SendMessage(SCI_SETSELECTIONSTART,pos);
    m_source.SendMessage(SCI_SETSELECTIONEND,pos);
    m_source.SendMessage(SCI_SCROLLCARET);
    return false;
  }
  m_doc->ResetSavePoint();
  m_doc->MarkDocCP(); // document is in sync with source
  m_source.SendMessage(SCI_SETSAVEPOINT);
  m_tree.GetDocumentStructure(m_doc->m_body);
  m_tree.HighlightItemAtPos(m_doc->m_body.SelectionContainer());
  return true;
}

void  CMainFrame::ShowView(VIEW_TYPE vt) {
  VIEW_TYPE prev=GetCurView();

  if (vt==NEXT)
    // we intentionally skip source view here, it is
    // accessible only via menu
    switch (prev) {
    case BODY: vt=DESC; break;
    case DESC: vt=BODY; break;
    case SOURCE: vt=BODY; break;
    }

  if (prev!=vt && prev==SOURCE) {
    if (!SourceToHTML())
      return;
  }

  if (prev!=vt && vt==SOURCE) {
    if (m_doc->DocRelChanged()) {
      // copy doc source to editor
      // quite ineffective and wastes memory
      m_source.SendMessage(SCI_CLEARALL);
      MSXML2::IXMLDOMDocument2Ptr dom(m_doc->CreateDOM(_T("")));
      if (!(bool)dom)
	return;

      _bstr_t   src(dom->xml);
      dom.Release();
      DWORD nch=::WideCharToMultiByte(CP_UTF8,0,src,src.length(),
				      NULL,0,NULL,NULL);
      char      *buffer=(char*)malloc(nch);
      if (buffer!=NULL) {
	::WideCharToMultiByte(CP_UTF8,0,src,src.length(),
				      buffer,nch,NULL,NULL);
	m_source.SendMessage(SCI_APPENDTEXT,nch,(LPARAM)buffer);
	free(buffer);
      }
      m_source.SendMessage(SCI_EMPTYUNDOBUFFER);
      m_doc->MarkDocCP();
    }

    // turn off doctree
    m_save_sp_mode=m_tree_pane.IsWindowVisible()!=0;
    UISetCheck(ID_VIEW_TREE,0);
  }

  if (prev!=vt && vt!=SOURCE) {
    UIEnable(ID_VIEW_TREE,1);
    UISetCheck(ID_VIEW_TREE, m_save_sp_mode);
    m_splitter.SetSinglePaneMode(m_save_sp_mode ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT);
  }

  UISetCheck(ID_VIEW_BODY, 0);
  UISetCheck(ID_VIEW_DESC, 0);
  UISetCheck(ID_VIEW_SOURCE, 0);

  switch (vt) {
  case BODY:
    UISetCheck(ID_VIEW_BODY, 1);
    m_view.ActivateWnd(m_doc->m_body);
    m_sel_changed=true;
    break;
  case DESC:
    UISetCheck(ID_VIEW_DESC, 1);
    m_view.ActivateWnd(m_doc->m_desc);
    m_href_box.SetWindowText(_T(""));
    m_href_box.EnableWindow(FALSE);
    m_id_box.SetWindowText(_T(""));
    m_id_box.EnableWindow(FALSE);
    m_status.SetPaneText(ID_DEFAULT_PANE,_T(""));
    break;
  case SOURCE:
    UISetCheck(ID_VIEW_SOURCE, 1);
    m_view.HideActiveWnd();
    m_splitter.SetSinglePaneMode(SPLIT_PANE_RIGHT);
    m_view.ActivateWnd(m_source);
    break;
  }
  m_view.SetFocus();
}

CMainFrame::VIEW_TYPE CMainFrame::GetCurView() {
  HWND	hWnd=m_view.GetActiveWnd();
  if (hWnd==m_doc->m_body)
    return BODY;
  if (hWnd==m_doc->m_desc)
    return DESC;
  return SOURCE;
}

void  CMainFrame::SetSciStyles() {
  m_source.SendMessage(SCI_STYLERESETDEFAULT);

  m_source.SendMessage(SCI_STYLESETFONT,STYLE_DEFAULT,(LPARAM)"Lucida Console");
  m_source.SendMessage(SCI_STYLESETSIZE,STYLE_DEFAULT,U::GetSettingI(_T("FontSize"),12));

  DWORD fs=U::GetSettingI(_T("ColorFG"),CLR_DEFAULT);
  if (fs!=CLR_DEFAULT)
    m_source.SendMessage(SCI_STYLESETFORE,STYLE_DEFAULT,fs);

  fs=U::GetSettingI(_T("ColorBG"),CLR_DEFAULT);
  if (fs!=CLR_DEFAULT)
    m_source.SendMessage(SCI_STYLESETBACK,STYLE_DEFAULT,fs);

  m_source.SendMessage(SCI_STYLECLEARALL);

  // set XML specific styles
  static struct {
    char    style;
    int	    color;
  }   styles[]={
    { 0, RGB(0,0,0) },	    // default text
    { 1, RGB(128,0,0) },    // tags
    { 2, RGB(128,0,0) },    // unknown tags
    { 3, RGB(128,128,0) },  // attributes
    { 4, RGB(128,128,0) },  // unknown attributes
    { 5, RGB(0,128,96) },   // numbers
    { 6, RGB(0,128,0) },    // double quoted strings
    { 7, RGB(0,128,0) },    // single quoted strings
    { 8, RGB(128,0,128) },  // other inside tag
    { 9, RGB(0,128,128) },  // comments
    { 10, RGB(128,0,128) }, // entities
    { 11, RGB(128,0,0) },   // tag ends
    { 12, RGB(128,0,128) }, // xml decl start
    { 13, RGB(128,0,128) }, // xml decl end
    { 17, RGB(128,0,0) },   // cdata
    { 18, RGB(128,0,0) },   // question
    { 19, RGB(96,128,96) }, // unquoted value
  };
  if (U::GetSettingI(_T("XMLSrcSyntaxHL"),TRUE))
    for (int i=0;i<sizeof(styles)/sizeof(styles[0]);++i)
      m_source.SendMessage(SCI_STYLESETFORE,styles[i].style,styles[i].color);
}

LRESULT CMainFrame::OnFileValidate(WORD, WORD, HWND, BOOL&) {
  int col,line;
  bool fv;
  if (IsSourceActive())
    fv=m_doc->SetXMLAndValidate(m_source,true,line,col);
  else
    fv=m_doc->Validate(line,col);
  if (!fv) {
    ShowView(SOURCE);
    // have to jump through the hoops to move to required column
    int	pos=m_source.SendMessage(SCI_POSITIONFROMLINE,line-1);
    while (col--)
      pos=m_source.SendMessage(SCI_POSITIONAFTER,pos);
    m_source.SendMessage(SCI_SETSELECTIONSTART,pos);
    m_source.SendMessage(SCI_SETSELECTIONEND,pos);
    m_source.SendMessage(SCI_SCROLLCARET);
  }
  return 0;
}

void  CMainFrame::FoldAll() {
  m_source.SendMessage(SCI_COLOURISE, 0, -1);
  int maxLine = m_source.SendMessage(SCI_GETLINECOUNT);
  bool expanding = true;
  for (int lineSeek = 0; lineSeek < maxLine; lineSeek++) {
    if (m_source.SendMessage(SCI_GETFOLDLEVEL, lineSeek) & SC_FOLDLEVELHEADERFLAG) {
      expanding = !m_source.SendMessage(SCI_GETFOLDEXPANDED, lineSeek);
      break;
    }
  }
  for (int line = 0; line < maxLine; line++) {
    int level = m_source.SendMessage(SCI_GETFOLDLEVEL, line);
    if ((level & SC_FOLDLEVELHEADERFLAG) &&
      (SC_FOLDLEVELBASE == (level & SC_FOLDLEVELNUMBERMASK))) {
      if (expanding) {
	m_source.SendMessage(SCI_SETFOLDEXPANDED, line, 1);
	ExpandFold(line, true, false, 0, level);
	line--;
      } else {
	int lineMaxSubord = m_source.SendMessage(SCI_GETLASTCHILD, line, -1);
	m_source.SendMessage(SCI_SETFOLDEXPANDED, line, 0);
	if (lineMaxSubord > line)
	  m_source.SendMessage(SCI_HIDELINES, line + 1, lineMaxSubord);
      }
    }
  }
}

void CMainFrame::ExpandFold(int &line, bool doExpand, bool force, int visLevels, int level) {
  int lineMaxSubord = m_source.SendMessage(SCI_GETLASTCHILD, line, level & SC_FOLDLEVELNUMBERMASK);
  line++;
  while (line <= lineMaxSubord) {
    if (force) {
      if (visLevels > 0)
	m_source.SendMessage(SCI_SHOWLINES, line, line);
      else
	m_source.SendMessage(SCI_HIDELINES, line, line);
    } else {
      if (doExpand)
	m_source.SendMessage(SCI_SHOWLINES, line, line);
    }
    int levelLine = level;
    if (levelLine == -1)
      levelLine = m_source.SendMessage(SCI_GETFOLDLEVEL, line);
    if (levelLine & SC_FOLDLEVELHEADERFLAG) {
      if (force) {
	if (visLevels > 1)
	  m_source.SendMessage(SCI_SETFOLDEXPANDED, line, 1);
	else
	  m_source.SendMessage(SCI_SETFOLDEXPANDED, line, 0);
	ExpandFold(line, doExpand, force, visLevels - 1);
      } else {
	if (doExpand) {
	  if (!m_source.SendMessage(SCI_GETFOLDEXPANDED, line))
	    m_source.SendMessage(SCI_SETFOLDEXPANDED, line, 1);
	  ExpandFold(line, true, force, visLevels - 1);
	} else {
	  ExpandFold(line, false, force, visLevels - 1);
	}
      }
    } else {
      line++;
    }
  }
}

void  CMainFrame::DefineMarker(int marker, int markerType, COLORREF fore,COLORREF back) {
  m_source.SendMessage(SCI_MARKERDEFINE, marker, markerType);
  m_source.SendMessage(SCI_MARKERSETFORE, marker, fore);
  m_source.SendMessage(SCI_MARKERSETBACK, marker, back);
}

void  CMainFrame::SetupSci() {
  m_source.SendMessage(SCI_SETCODEPAGE,SC_CP_UTF8);
  m_source.SendMessage(SCI_SETEOLMODE,SC_EOL_CRLF);
  m_source.SendMessage(SCI_SETVIEWEOL,U::GetSettingI(_T("XMLSrcShowEOL"),FALSE));
  m_source.SendMessage(SCI_SETWRAPMODE,
    U::GetSettingI(_T("XMLSrcWrap"),TRUE) ? SC_WRAP_WORD : SC_WRAP_NONE);
  m_source.SendMessage(SCI_SETXCARETPOLICY,CARET_SLOP|CARET_EVEN,50);
  m_source.SendMessage(SCI_SETYCARETPOLICY,CARET_SLOP|CARET_EVEN,50);
  m_source.SendMessage(SCI_SETMARGINWIDTHN,1,0);
  m_source.SendMessage(SCI_SETFOLDFLAGS, 16);
  m_source.SendMessage(SCI_SETPROPERTY,(WPARAM)"fold",(WPARAM)"1");
  m_source.SendMessage(SCI_SETPROPERTY,(WPARAM)"fold.html",(WPARAM)"1");
  m_source.SendMessage(SCI_SETPROPERTY,(WPARAM)"fold.compact",(WPARAM)"1");
  m_source.SendMessage(SCI_SETPROPERTY,(WPARAM)"fold.flags",(WPARAM)"16");
  m_source.SendMessage(SCI_SETSTYLEBITS,7);
  if (U::GetSettingI(_T("XMLSrcSyntaxHL"),TRUE)) {
    m_source.SendMessage(SCI_SETLEXER,5); // xml
    m_source.SendMessage(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
    m_source.SendMessage(SCI_SETMARGINWIDTHN, 2, 16);
    m_source.SendMessage(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
    m_source.SendMessage(SCI_SETMARGINSENSITIVEN, 2, 1);
    DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_PLUS, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, RGB(0xff, 0xff, 0xff), RGB(0, 0, 0));
    m_source.SendMessage(SCI_COLOURISE,0,-1);
  } else {
    m_source.SendMessage(SCI_SETLEXER,1); // null lexer
    m_source.SendMessage(SCI_SETMARGINWIDTHN, 2, 0);
  }
}

void  CMainFrame::SciModified(const SCNotification& scn) {
  if (scn.modificationType & SC_MOD_CHANGEFOLD) {
    if (scn.foldLevelNow & SC_FOLDLEVELHEADERFLAG) {
      if (!(scn.foldLevelPrev & SC_FOLDLEVELHEADERFLAG))
	m_source.SendMessage(SCI_SETFOLDEXPANDED, scn.line, 1);
    } else if (scn.foldLevelPrev & SC_FOLDLEVELHEADERFLAG) {
      if (!m_source.SendMessage(SCI_GETFOLDEXPANDED, scn.line)) {
	// Removing the fold from one that has been contracted so should expand
	// otherwise lines are left invisible with no way to make them visible
	int tmpline=scn.line;
	ExpandFold(tmpline, true, false, 0, scn.foldLevelPrev);
      }
    }
  }
}

void  CMainFrame::SciMarginClicked(const SCNotification& scn) {
  int lineClick = m_source.SendMessage(SCI_LINEFROMPOSITION, scn.position);
  if ((scn.modifiers & SCMOD_SHIFT) && (scn.modifiers & SCMOD_CTRL)) {
    FoldAll();
  } else {
    int levelClick = m_source.SendMessage(SCI_GETFOLDLEVEL, lineClick);
    if (levelClick & SC_FOLDLEVELHEADERFLAG) {
      if (scn.modifiers & SCMOD_SHIFT) {
	// Ensure all children visible
	m_source.SendMessage(SCI_SETFOLDEXPANDED, lineClick, 1);
	ExpandFold(lineClick, true, true, 100, levelClick);
      } else if (scn.modifiers & SCMOD_CTRL) {
	if (m_source.SendMessage(SCI_GETFOLDEXPANDED, lineClick)) {
	  // Contract this line and all children
	  m_source.SendMessage(SCI_SETFOLDEXPANDED, lineClick, 0);
	  ExpandFold(lineClick, false, true, 0, levelClick);
	} else {
	  // Expand this line and all children
	  m_source.SendMessage(SCI_SETFOLDEXPANDED, lineClick, 1);
	  ExpandFold(lineClick, true, true, 100, levelClick);
	}
      } else {
	// Toggle this line
	m_source.SendMessage(SCI_TOGGLEFOLD, lineClick);
      }
    }
  }
}
