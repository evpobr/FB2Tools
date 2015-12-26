// FBEView.cpp : implementation of the CFBEView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include "FBEView.h"
#include "SearchReplace.h"
#include "Scintilla.h"

// normalization helpers
static void BubbleUp(MSHTML::IHTMLDOMNode *node,const wchar_t *name);
static void PackText(MSHTML::IHTMLElement2Ptr elem,MSHTML::IHTMLDocument2 *doc);
static void KillDivs(MSHTML::IHTMLElement2Ptr elem);
static void FixupParagraphs(MSHTML::IHTMLElement2Ptr elem);
static void RelocateParagraphs(MSHTML::IHTMLDOMNode *node);
static void KillStyles(MSHTML::IHTMLElement2Ptr elem);

_ATL_FUNC_INFO CFBEView::DocumentCompleteInfo=
  { CC_STDCALL, VT_EMPTY, 2, { VT_DISPATCH, (VT_BYREF | VT_VARIANT) } };
_ATL_FUNC_INFO CFBEView::BeforeNavigateInfo=
  { CC_STDCALL, VT_EMPTY, 7, {
      VT_DISPATCH,
      (VT_BYREF | VT_VARIANT),
      (VT_BYREF | VT_VARIANT),
      (VT_BYREF | VT_VARIANT),
      (VT_BYREF | VT_VARIANT),
      (VT_BYREF | VT_VARIANT),
      (VT_BYREF | VT_BOOL),
    }
  };
_ATL_FUNC_INFO CFBEView::VoidInfo=
  { CC_STDCALL, VT_EMPTY, 0 };
_ATL_FUNC_INFO CFBEView::EventInfo=
  { CC_STDCALL, VT_BOOL, 1, { VT_DISPATCH } };
_ATL_FUNC_INFO CFBEView::VoidEventInfo=
  { CC_STDCALL, VT_EMPTY, 1, { VT_DISPATCH } };

LRESULT CFBEView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (DefWindowProc(uMsg,wParam,lParam))
    return 1;
  if (!SUCCEEDED(QueryControl(&m_browser)))
    return 1;

  // register browser events handler
  BrowserEvents::DispEventAdvise(m_browser,&DIID_DWebBrowserEvents2);

  return 0;
}

CFBEView::~CFBEView() {
  if (HasDoc()) {
    DocumentEvents::DispEventUnadvise(Document(),&DIID_HTMLDocumentEvents2);
    TextEvents::DispEventUnadvise(Document()->body,&DIID_HTMLTextContainerEvents2);
    m_mkc->UnRegisterForDirtyRange(m_dirtyRangeCookie);
  }
  if (m_browser)
    BrowserEvents::DispEventUnadvise(m_browser,&DIID_DWebBrowserEvents2);
}

BOOL CFBEView::PreTranslateMessage(MSG* pMsg) {
  return SendMessage(WM_FORWARDMSG,0,(LPARAM)pMsg)!=0;
}

// editing commands
LRESULT CFBEView::ExecCommand(int cmd) {
  IOleCommandTargetPtr	  ct(m_browser);
  if (ct)
    ct->Exec(&CGID_MSHTML,cmd,0,NULL,NULL);
  return 0;
}

void	  CFBEView::QueryStatus(OLECMD *cmd,int ncmd) {
  IOleCommandTargetPtr	  ct(m_browser);
  if (ct)
    ct->QueryStatus(&CGID_MSHTML,ncmd,cmd,NULL);
}

CString	  CFBEView::QueryCmdText(int cmd) {
  IOleCommandTargetPtr	  ct(m_browser);
  if (ct) {
    OLECMD	oc={cmd};
    struct {
      OLECMDTEXT	oct;
      wchar_t		buffer[512];
    } oct={ { OLECMDTEXTF_NAME, 0, 512 } };
    if (SUCCEEDED(ct->QueryStatus(&CGID_MSHTML,1,&oc,&oct.oct)))
      return oct.oct.rgwz;
  }
  return CString();
}

LRESULT CFBEView::OnStyleLink(WORD, WORD, HWND, BOOL&) {
  try {
    if (Document()->execCommand(L"CreateLink",VARIANT_FALSE,_variant_t(L""))==VARIANT_TRUE)
    {
      ::SendMessage(m_frame,WM_COMMAND,MAKELONG(0,IDN_SEL_CHANGE),(LPARAM)m_hWnd);
      ::SendMessage(m_frame,WM_COMMAND,MAKELONG(IDC_HREF,IDN_WANTFOCUS),(LPARAM)m_hWnd);
    }
  }
  catch (_com_error&) { }
  return 0;
}

LRESULT CFBEView::OnStyleFootnote(WORD, WORD, HWND, BOOL&) {
  try {
    m_mk_srv->BeginUndoUnit((USHORT*)L"Create Footnote");
    if (Document()->execCommand(L"CreateLink",VARIANT_FALSE,_variant_t(L""))==VARIANT_TRUE) {
      MSHTML::IHTMLTxtRangePtr  r(Document()->selection->createRange());
      MSHTML::IHTMLElementPtr	pe(r->parentElement());
      if (U::scmp(pe->tagName,L"A")==0)
	pe->className=L"note";
    }
    m_mk_srv->EndUndoUnit();
    ::SendMessage(m_frame,WM_COMMAND,MAKELONG(0,IDN_SEL_CHANGE),(LPARAM)m_hWnd);
    ::SendMessage(m_frame,WM_COMMAND,MAKELONG(IDC_HREF,IDN_WANTFOCUS),(LPARAM)m_hWnd);
  }
  catch (_com_error&) { }
  return 0;
}

LRESULT	CFBEView::OnEditInsImage(WORD, WORD, HWND, BOOL&) {
  try {
    MSHTML::IHTMLDOMNodePtr   node(Call(L"InsImage"));
    if (node)
      BubbleUp(node,L"DIV");
  }
  catch (_com_error&) { }
  return 0;
}

bool	CFBEView::CheckCommand(WORD wID) {
  if (!m_normalize)
    return false;
  switch (wID) {
  case ID_EDIT_ADD_BODY:
    return true;
  case ID_EDIT_ADD_TITLE:
    return bCall(L"AddTitle",SelectionStructCon());
  case ID_EDIT_CLONE:
    return bCall(L"CloneContainer",SelectionStructCon());
  case ID_STYLE_NORMAL:
    return bCall(L"StyleNormal",SelectionStructCon());
  case ID_STYLE_SUBTITLE:
    return bCall(L"StyleSubtitle",SelectionStructCon());
  case ID_STYLE_TEXTAUTHOR:
    return bCall(L"StyleTextAuthor",SelectionStructCon());
  case ID_EDIT_INS_IMAGE:
    return bCall(L"InsImage");
  case ID_EDIT_ADD_IMAGE:
    return bCall(L"AddImage",SelectionStructCon());
  case ID_EDIT_ADD_EPIGRAPH:
    return bCall(L"AddEpigraph",SelectionStructCon());
  case ID_EDIT_ADD_ANN:
    return bCall(L"AddAnnotation",SelectionStructCon());
  case ID_EDIT_SPLIT:
    return SplitContainer(true);
  case ID_EDIT_INS_POEM:
    return InsertPoemOrCite(false,true);
  case ID_EDIT_INS_CITE:
    return InsertPoemOrCite(true,true);
  case ID_EDIT_ADD_TA:
    return bCall(L"AddTA",SelectionStructCon());
  case ID_EDIT_MERGE:
    return bCall(L"MergeContainers",SelectionStructCon());
  case ID_EDIT_REMOVE_OUTER_SECTION:
    return bCall(L"RemoveOuterContainer",SelectionStructCon());
  case ID_STYLE_LINK:
  case ID_STYLE_NOTE:
    try {
      return Document()->queryCommandEnabled(L"CreateLink")==VARIANT_TRUE;
    }
    catch (_com_error) { }
    break;
  }
  return false;
}

// changes tracking
MSHTML::IHTMLDOMNodePtr	  CFBEView::GetChangedNode() {
  MSHTML::IMarkupPointerPtr	  p1,p2;
  m_mk_srv->CreateMarkupPointer(&p1);
  m_mk_srv->CreateMarkupPointer(&p2);

  m_mkc->GetAndClearDirtyRange(m_dirtyRangeCookie,p1,p2);

  MSHTML::IHTMLElementPtr	  e1,e2;
  p1->CurrentScope(&e1);
  p2->CurrentScope(&e2);
  p1.Release();
  p2.Release();

  while ((bool)e1 && e1!=e2 && e1->contains(e2)!=VARIANT_TRUE)
    e1=e1->parentElement;

  return e1;
}

// splitting
bool  CFBEView::SplitContainer(bool fCheck) {
  try {
    MSHTML::IHTMLTxtRangePtr	rng(Document()->selection->createRange());
    if (!(bool)rng)
      return false;
    
    MSHTML::IHTMLElementPtr	pe(rng->parentElement());
    while ((bool)pe && U::scmp(pe->tagName,L"DIV"))
      pe=pe->parentElement;
    
    if (!(bool)pe || (U::scmp(pe->className,L"section") && U::scmp(pe->className,L"stanza")))
      return false;
    
    MSHTML::IHTMLTxtRangePtr	r2(rng->duplicate());
    r2->moveToElementText(pe);

    if (rng->compareEndPoints(L"StartToStart",r2)==0)
      return false;

    if (fCheck)
      return true;

    // at this point we are ready to split

    // * create & position markup pointers
    MSHTML::IMarkupPointerPtr	selstart,selend,elemend;
    m_mk_srv->CreateMarkupPointer(&selstart);
    m_mk_srv->CreateMarkupPointer(&selend);
    m_mk_srv->CreateMarkupPointer(&elemend);
    m_mk_srv->MovePointersToRange(rng,selstart,selend);
    elemend->MoveAdjacentToElement(pe,MSHTML::ELEM_ADJ_BeforeEnd);

    // * check if title needs to be created
    bool  fTitle=rng->compareEndPoints(L"StartToEnd",rng)!=0;
    bool  fContent=rng->compareEndPoints(L"EndToEnd",r2)!=0;

    // * create an undo unit
    CString   name(L"split ");
    name+=(const wchar_t *)pe->className;
    m_mk_srv->BeginUndoUnit((USHORT*)(const TCHAR *)name);

    // * create a new element
    MSHTML::IHTMLElementPtr   ne(Document()->createElement(L"DIV"));
    ne->className=pe->className;

    // * insert it after pe
    MSHTML::IHTMLElement2Ptr(pe)->insertAdjacentElement(L"afterEnd",ne);

    // * move content or create new
    if (fContent) {
      // * create&position destination markup pointer
      MSHTML::IMarkupPointerPtr	dest;
      m_mk_srv->CreateMarkupPointer(&dest);
      dest->MoveAdjacentToElement(ne,MSHTML::ELEM_ADJ_AfterBegin);
      m_mk_srv->move(selend,elemend,dest);
    } else {
      MSHTML::IHTMLElementPtr	para(Document()->createElement(L"P"));
      MSHTML::IHTMLElement3Ptr(para)->inflateBlock=VARIANT_TRUE;
      MSHTML::IHTMLElement2Ptr(ne)->insertAdjacentElement(L"beforeEnd",para);
    }

    // * create and move title if needed
    if (fTitle) {
      MSHTML::IHTMLElementPtr title(Document()->createElement(L"DIV"));
      title->className=L"title";
      MSHTML::IHTMLElement2Ptr(ne)->insertAdjacentElement(L"afterBegin",title);

      // * create&position destination markup pointer
      MSHTML::IMarkupPointerPtr	dest;
      m_mk_srv->CreateMarkupPointer(&dest);
      dest->MoveAdjacentToElement(title,MSHTML::ELEM_ADJ_AfterBegin);
      m_mk_srv->move(selstart,selend,dest);

      // * delete all containers from title
      KillDivs(title);
      KillStyles(title);
    }

    // * ensure we have good html
    PackText(ne,Document());

    // * close undo unit
    m_mk_srv->EndUndoUnit();

    // * move cursor to newly created item
    GoTo(ne,false);
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
  return false;
}

// find parent DIV
static MSHTML::IHTMLElementPtr	GetHP(MSHTML::IHTMLElementPtr hp) {
  while ((bool)hp && U::scmp(hp->tagName,L"DIV"))
    hp=hp->parentElement;
  return hp;
}

// conversion to poems or citations
bool  CFBEView::InsertPoemOrCite(bool fCite,bool fCheck) {
  try {
    // * create selection range
    MSHTML::IHTMLTxtRangePtr	rng(Document()->selection->createRange());
    if (!(bool)rng)
      return false;

    // * get its parent element
    MSHTML::IHTMLElementPtr	pe(GetHP(rng->parentElement()));
    if (!(bool)pe)
      return false;

    // * get parents for start and end ranges and ensure they are the same as pe
    MSHTML::IHTMLTxtRangePtr	tr(rng->duplicate());
    tr->collapse(VARIANT_TRUE);
    if (GetHP(tr->parentElement())!=pe)
      return false;
#if 0
    tr=rng->duplicate();
    tr->collapse(VARIANT_FALSE);
    if (GetHP(tr->parentElement())!=pe)
      return false;
#endif

    // * check if it possible to insert a poem there
    _bstr_t   cls(pe->className);
    if (U::scmp(cls,L"section") && U::scmp(cls,L"epigraph") &&
	  U::scmp(cls,L"annotation") && U::scmp(cls,L"history") && (fCite || U::scmp(cls,L"cite")))
      return false;

    // * ok, all checks passed
    if (fCheck)
      return true;

    // at this point we are ready to create a poem

    // * create an undo unit
    m_mk_srv->BeginUndoUnit(fCite ? (USHORT*)L"insert cite" : (USHORT*)L"insert poem");

    MSHTML::IHTMLElementPtr	  ne(Document()->createElement(L"DIV"));
    if (!fCite) {
      // * create stanza
      MSHTML::IHTMLElementPtr	  se(Document()->createElement(L"DIV"));
      se->className=L"stanza";

      // * copy content
      if (rng->compareEndPoints(L"StartToEnd",rng)!=0)
	se->innerHTML=rng->htmlText;
      else
	se->innerHTML=L"<P></P>";

      // * cleanup content
      KillDivs(se);
      KillStyles(se);
      PackText(se,Document());

      // * create poem
      ne->className=L"poem";
      MSHTML::IHTMLElement2Ptr(ne)->insertAdjacentElement(L"afterBegin",se);
    } else {
      // * create cite
      ne->className=L"cite";

      // * copy content
      if (rng->compareEndPoints(L"StartToEnd",rng)!=0)
	ne->innerHTML=rng->htmlText;
      else
	ne->innerHTML=L"<P></P>";

      // * cleanup content
      KillDivs(ne);
      KillStyles(ne);
      PackText(ne,Document());
    }

    // * paste the results back
    rng->pasteHTML(ne->outerHTML);

    // * ensure we have good html
    RelocateParagraphs(MSHTML::IHTMLDOMNodePtr(pe));
    FixupParagraphs(pe);

    // * close undo unit
    m_mk_srv->EndUndoUnit();
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
  return false;
}

// searching
void  CFBEView::StartIncSearch() {
  try {
    m_is_start=Document()->selection->createRange();
    m_is_start->collapse(VARIANT_TRUE);
  }
  catch (_com_error&) {
  }
}

void  CFBEView::CancelIncSearch() {
  if (m_is_start) {
    m_is_start->raw_select();
    m_is_start.Release();
  }
}

// script calls
void	      CFBEView::ImgSetURL(IDispatch *elem,const CString& url) {
  try {
    CComDispatchDriver	dd(Script());
    _variant_t	  ve(elem);
    _variant_t	  vu((const TCHAR *)url);
    dd.Invoke2(L"ImgSetURL",&ve,&vu);
  }
  catch (_com_error&) { }
}

IDispatchPtr  CFBEView::Call(const wchar_t *name) {
  try {
    CComDispatchDriver  dd(Script());
    _variant_t  ret;
    _variant_t  vt2(false);
    dd.Invoke1(name,&vt2,&ret);
    if (V_VT(&ret)==VT_DISPATCH)
      return V_DISPATCH(&ret);
  }
  catch (_com_error&) { }
  return IDispatchPtr();
}
IDispatchPtr  CFBEView::Call(const wchar_t *name,IDispatch *pDisp) {
  try {
    CComDispatchDriver  dd(Script());
    _variant_t  vt;
    if (pDisp)
      vt=pDisp;
    _variant_t  vt2(false);
    _variant_t  ret;
    dd.Invoke2(name,&vt,&vt2,&ret);
    if (V_VT(&ret)==VT_DISPATCH)
      return V_DISPATCH(&ret);
  }
  catch (_com_error&) { }
  return IDispatchPtr();
}
static bool vt2bool(const _variant_t& vt) {
  if (V_VT(&vt)==VT_DISPATCH)
    return V_DISPATCH(&vt)!=0;
  if (V_VT(&vt)==VT_BOOL)
    return V_BOOL(&vt)==VARIANT_TRUE;
  if (V_VT(&vt)==VT_I4)
    return V_I4(&vt)!=0;
  if (V_VT(&vt)==VT_UI4)
    return V_UI4(&vt)!=0;
  return false;
}
bool  CFBEView::bCall(const wchar_t *name,IDispatch *pDisp) {
  try {
    CComDispatchDriver  dd(Script());
    _variant_t  vt;
    if (pDisp)
      vt=pDisp;
    _variant_t  vt2(true);
    _variant_t  ret;
    dd.Invoke2(name,&vt,&vt2,&ret);
    return vt2bool(ret);
  }
  catch (_com_error&) { }
  return false;
}
bool  CFBEView::bCall(const wchar_t *name) {
  try {
    CComDispatchDriver  dd(Script());
    _variant_t  vt2(true);
    _variant_t  ret;
    dd.Invoke1(name,&vt2,&ret);
    return vt2bool(ret);
  }
  catch (_com_error&) { }
  return false;
}

// utilities
static CString	GetPath(MSHTML::IHTMLElementPtr elem) {
  try {
    if (!(bool)elem)
      return CString();
    CString		      path;
    while (elem) {
      CString	  cur((const wchar_t *)elem->tagName);
      if (cur==_T("BODY"))
        return path;
      _bstr_t	  cls(elem->className);
      if (cls.length()>0)
	cur=(const wchar_t *)cls;
      _bstr_t	  id(elem->id);
      if (id.length()>0) {
	cur+=_T(':');
	cur+=(const wchar_t *)id;
      }
      if (!path.IsEmpty())
	path=_T('/')+path;
      path=cur+path;
      elem=elem->parentElement;
    }
    return path;
  }
  catch (_com_error&) { }
  return CString();
}

CString	CFBEView::SelPath() {
  return GetPath(SelectionContainer());
}

void  CFBEView::GoTo(MSHTML::IHTMLElement *e,bool fScroll) {
  if (!e)
    return;

  if (fScroll)
    e->scrollIntoView(VARIANT_TRUE);

  MSHTML::IHTMLTxtRangePtr	r(MSHTML::IHTMLBodyElementPtr(Document()->body)->createTextRange());
  r->moveToElementText(e);
  r->collapse(VARIANT_TRUE);
  // all m$ editors like to position the pointer at the end of the preceding element,
  // which sucks. This workaround seems to work most of the time.
  if (e!=r->parentElement() && r->move(L"character",1)==1)
    r->move(L"character",-1);

  r->select();
}

MSHTML::IHTMLElementPtr	  CFBEView::SelectionContainerImp() {
  try {
    IDispatchPtr	      selrange(Document()->selection->createRange());
    MSHTML::IHTMLTxtRangePtr  range(selrange);
    if (range)
      return range->parentElement();
    MSHTML::IHTMLControlRangePtr  coll(selrange);
    if ((bool)coll)
      return coll->commonParentElement();
  }
  catch (_com_error&) {
  }
  return MSHTML::IHTMLElementPtr();
}

MSHTML::IHTMLElementPtr CFBEView::SelectionAnchor() {
  try {
    MSHTML::IHTMLElementPtr   cur(SelectionContainer());
    while (cur) {
      _bstr_t	tn(cur->tagName);
      if (U::scmp(tn,L"A")==0 || (U::scmp(tn,L"DIV")==0 && U::scmp(cur->className,L"image")==0))
	return cur;
      cur=cur->parentElement;
    }
  }
  catch (_com_error&) { }
  return MSHTML::IHTMLElementPtr();
}

MSHTML::IHTMLElementPtr	  CFBEView::SelectionStructCon() {
  try {
    MSHTML::IHTMLElementPtr   cur(SelectionContainer());
    while (cur) {
      if (U::scmp(cur->tagName,L"P")==0 || U::scmp(cur->tagName,L"DIV")==0)
	return cur;
      cur=cur->parentElement;
    }
  }
  catch (_com_error&) {
  }
  return MSHTML::IHTMLElementPtr();
}

// cleaning up html
static void KillDivs(MSHTML::IHTMLElement2Ptr elem) {
  MSHTML::IHTMLElementCollectionPtr	  divs(elem->getElementsByTagName(L"DIV"));
  while (divs->length>0)
    MSHTML::IHTMLDOMNodePtr(divs->item(0L))->removeNode(VARIANT_FALSE);
}

static void KillStyles(MSHTML::IHTMLElement2Ptr elem) {
  MSHTML::IHTMLElementCollectionPtr	  ps(elem->getElementsByTagName(L"P"));
  for (long l=0;l<ps->length;++l)
    CheckError(MSHTML::IHTMLElementPtr(ps->item(l))->put_className(NULL));
}

static bool   RemoveUnk(MSHTML::IHTMLDOMNode *node,MSHTML::IHTMLDocument2 *doc) {
  if (node->nodeType!=1)
    return false;

  bool	fRet=false;

restart:
  MSHTML::IHTMLDOMNodePtr   cur(node->firstChild);
  while (cur) {
    MSHTML::IHTMLDOMNodePtr next(cur->nextSibling);

    if (RemoveUnk(cur,doc))
      goto restart;

    _bstr_t			name(cur->nodeName);
    MSHTML::IHTMLElementPtr	curelem(cur);
    
    if (U::scmp(name,L"B")==0 || U::scmp(name,L"I")==0) {
      const wchar_t		*newname=U::scmp(name,L"B")==0 ? L"STRONG" : L"EM";
      MSHTML::IHTMLElementPtr	newelem(doc->createElement(newname));
      MSHTML::IHTMLDOMNodePtr	newnode(newelem);
      newelem->innerHTML=curelem->innerHTML;
      cur->replaceNode(newnode);
      cur=newnode;
      fRet=true;
      goto restart;
    }

    if (U::scmp(name,L"P") && U::scmp(name,L"STRONG") &&
	U::scmp(name,L"EM") && U::scmp(name,L"A") &&
	(U::scmp(name,L"SPAN") || curelem->className.length()==0) &&
	U::scmp(name,L"#text") && U::scmp(name,L"BR") && U::scmp(name,L"IMG"))
    {
      if (U::scmp(name,L"DIV")==0) {
	_bstr_t	  cls(curelem->className);
	if (!(U::scmp(cls,L"body") && U::scmp(cls,L"section") &&
	  U::scmp(cls,L"annotation") && U::scmp(cls,L"title") && U::scmp(cls,L"epigraph") &&
	  U::scmp(cls,L"poem") && U::scmp(cls,L"stanza") && U::scmp(cls,L"cite") &&
	  U::scmp(cls,L"history") && U::scmp(cls,L"image")))
	  goto ok;
      }
      MSHTML::IHTMLDOMNodePtr ce(cur->previousSibling);
      cur->removeNode(VARIANT_FALSE);
      if (ce)
	next=ce->nextSibling;
      else
	next=node->firstChild;
    }
ok:

    cur=next;
  }
  return fRet;
}

// move the paragraph up one level
void MoveUp(bool fCopyFmt,MSHTML::IHTMLDOMNodePtr& node) {
  MSHTML::IHTMLDOMNodePtr   parent(node->parentNode);
  MSHTML::IHTMLElement2Ptr  elem(parent);

  // clone parent (it can be A/EM/STRONG/SPAN)
  if (fCopyFmt) {
    MSHTML::IHTMLDOMNodePtr   clone(parent->cloneNode(VARIANT_FALSE));
    while ((bool)node->firstChild)
      clone->appendChild(node->firstChild);
    node->appendChild(clone);
  }

  // clone parent once more and move siblings after node to it
  if ((bool)node->nextSibling) {
    MSHTML::IHTMLDOMNodePtr   clone(parent->cloneNode(VARIANT_FALSE));
    while ((bool)node->nextSibling)
      clone->appendChild(node->nextSibling);
    elem->insertAdjacentElement(L"afterEnd",MSHTML::IHTMLElementPtr(clone));
    if (U::scmp(parent->nodeName,L"P")==0)
      MSHTML::IHTMLElement3Ptr(clone)->inflateBlock=VARIANT_TRUE;
  }

  // now move node to parent level, the tree may be in some weird state
  node->removeNode(VARIANT_TRUE); // delete from tree
  node=elem->insertAdjacentElement(L"afterEnd",MSHTML::IHTMLElementPtr(node));
}

static void BubbleUp(MSHTML::IHTMLDOMNode *node,const wchar_t *name) {
  MSHTML::IHTMLElement2Ptr	    elem(node);
  MSHTML::IHTMLElementCollectionPtr elements(elem->getElementsByTagName(name));
  long				    len=elements->length;
  for (long i=0;i<len;++i) {
    MSHTML::IHTMLDOMNodePtr	  ce(elements->item(i));
    if (!(bool)ce)
      break;
    for (int ll=0;ce->parentNode!=node && ll<30;++ll)
      MoveUp(true,ce);
    MoveUp(false,ce);
  }
}

// split paragraphs containing BR elements
static void   SplitBRs(MSHTML::IHTMLElement2Ptr elem) {
  MSHTML::IHTMLElementCollectionPtr BRs(elem->getElementsByTagName(L"BR"));
  while (BRs->length>0) {
    MSHTML::IHTMLDOMNodePtr	  ce(BRs->item(0L));
    if (!(bool)ce)
      break;
    for (;;) {
      MSHTML::IHTMLDOMNodePtr	parent(ce->parentNode);
      if (!(bool)parent) // no parent? huh?
	goto blowit;
      _bstr_t	  name(parent->nodeName);
      if (U::scmp(name,L"P")==0 || U::scmp(name,L"DIV")==0)
	break;
      if (U::scmp(name,L"BODY")==0)
	goto blowit;
      MoveUp(false,ce);
    }
    MoveUp(false,ce);
blowit:
    ce->removeNode(VARIANT_TRUE);
  }
}

// this sub should locate any nested paragraphs and bubble them up
static void RelocateParagraphs(MSHTML::IHTMLDOMNode *node) {
  if (node->nodeType!=1)
    return;

  MSHTML::IHTMLDOMNodePtr   cur(node->firstChild);
  while (cur) {
    if (cur->nodeType==1) {
      if (!U::scmp(cur->nodeName,L"P")) {
	BubbleUp(cur,L"P");
	BubbleUp(cur,L"DIV");
      } else
	RelocateParagraphs(cur);
    }
    cur=cur->nextSibling;
  }
}

static bool IsEmptyNode(MSHTML::IHTMLDOMNode *node) {
  if (node->nodeType!=1)
    return false;

  _bstr_t   name(node->nodeName);

  if (U::scmp(name,L"BR")==0)
    return false;

  if (U::scmp(name,L"P")==0) // the editor uses empty Ps to represent empty lines
    return false;

  // images are always empty
  if (U::scmp(name,L"DIV")==0 && U::scmp(MSHTML::IHTMLElementPtr(node)->className,L"image")==0)
    return false;
  if (U::scmp(name,L"IMG")==0)
    return false;

  if (node->hasChildNodes()==VARIANT_FALSE)
    return true;

  if (U::scmp(name,L"A")==0) // links can be meaningful even if the contain only ws
    return false;

  if ((bool)node->firstChild->nextSibling)
    return false;

  if (node->firstChild->nodeType!=3)
    return false;

  if (U::is_whitespace(node->firstChild->nodeValue.bstrVal))
    return true;

  return false;
}

// remove empty leaf nodes
static void RemoveEmptyNodes(MSHTML::IHTMLDOMNode *node) {
  if (node->nodeType!=1)
    return;

  MSHTML::IHTMLDOMNodePtr cur(node->firstChild);
  while (cur) {
    MSHTML::IHTMLDOMNodePtr next(cur->nextSibling);
    RemoveEmptyNodes(cur);
    if (IsEmptyNode(cur))
      cur->removeNode(VARIANT_TRUE);
    cur=next;
  }
}

static bool IsStanza(MSHTML::IHTMLDOMNode *node) {
  MSHTML::IHTMLElementPtr   elem(node);
  return U::scmp(elem->className,L"stanza")==0;
}

// move text content in DIV items to P elements, so DIVs can
// contain P and DIV _only_
static void PackText(MSHTML::IHTMLElement2Ptr elem,MSHTML::IHTMLDocument2 *doc) {
  MSHTML::IHTMLElementCollectionPtr elements(elem->getElementsByTagName(L"DIV"));
  long				    len=elements->length;
  for (long i=0;i<len;++i) {
    MSHTML::IHTMLDOMNodePtr	div(elements->item(i));
    if (U::scmp(MSHTML::IHTMLElementPtr(div)->className,L"image")==0)
      continue;
    MSHTML::IHTMLDOMNodePtr	cur(div->firstChild);
    while ((bool)cur) {
      _bstr_t	  cur_name(cur->nodeName);
      if (U::scmp(cur_name,L"P") && U::scmp(cur_name,L"DIV")) {
	// create a paragraph from a run of !P && !DIV
	MSHTML::IHTMLElementPtr	newp(doc->createElement(L"P"));
	MSHTML::IHTMLDOMNodePtr	newn(newp);
	cur->replaceNode(newn);
	newn->appendChild(cur);
	while ((bool)newn->nextSibling) {
	  cur_name=newn->nextSibling->nodeName;
	  if (U::scmp(cur_name,L"P")==0 || U::scmp(cur_name,L"DIV")==0)
	    break;
	  newn->appendChild(newn->nextSibling);
	}
	cur=newn->nextSibling;
      } else
	cur=cur->nextSibling;
    }
  }
}

static void FixupLinks(MSHTML::IHTMLDOMNode *dom) {
  MSHTML::IHTMLElement2Ptr  elem(dom);

  if (!(bool)elem)
    return;
  
  MSHTML::IHTMLElementCollectionPtr coll(elem->getElementsByTagName(L"A"));
  if (!(bool)coll)
    return;

  for (long l=0;l<coll->length;++l) {
    MSHTML::IHTMLElementPtr a(coll->item(l));
    if (!(bool)a)
      continue;

    _variant_t	  href(a->getAttribute(L"href",2));
    if (V_VT(&href)==VT_BSTR && V_BSTR(&href) &&
	::SysStringLen(V_BSTR(&href))>11 &&
	memcmp(V_BSTR(&href),L"about:blank",11*sizeof(wchar_t))==0)
    {
      a->setAttribute(L"href",V_BSTR(&href)+11,0);
    }
  }
}

void  CFBEView::Normalize(MSHTML::IHTMLDOMNodePtr dom) {
  try {
    // wrap in an undo unit
    m_mk_srv->BeginUndoUnit((USHORT*)L"Normalize");

    // remove unsupported elements
    RemoveUnk(dom,Document());
    // get rid of nested DIVs and Ps
    RelocateParagraphs(dom);
    // delete empty nodes
    RemoveEmptyNodes(dom);
    // make sure text appears under Ps only
    PackText(dom,Document());
    // get rid of nested Ps once more
    RelocateParagraphs(dom);
    // convert BRs to separate paragraphs
    SplitBRs(dom);
    // delete empty nodes again
    RemoveEmptyNodes(dom);
    // fixup links
    FixupLinks(dom);

    m_mk_srv->EndUndoUnit();
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
}

static void FixupParagraphs(MSHTML::IHTMLElement2Ptr elem) {
  MSHTML::IHTMLElementCollectionPtr   pp(elem->getElementsByTagName(L"P"));
  for (long l=0;l<pp->length;++l)
    MSHTML::IHTMLElement3Ptr(pp->item(l))->inflateBlock=VARIANT_TRUE;
}

LRESULT CFBEView::OnPaste(WORD, WORD, HWND, BOOL&) {
  try {
    m_mk_srv->BeginUndoUnit((USHORT*)L"Paste");
    ++m_enable_paste;
    IOleCommandTargetPtr(m_browser)->Exec(&CGID_MSHTML, IDM_PASTE, 0, NULL, NULL);
    --m_enable_paste;
    if (m_normalize)
      Normalize(Document()->body);
    m_mk_srv->EndUndoUnit();
  }
  catch (_com_error&) { }
  return 0;
}

// searching
bool  CFBEView::DoSearch(bool fMore) {
  if (m_fo.match)
    m_fo.match.Release();
  if (m_fo.pattern.IsEmpty()) {
    if (m_is_start)
      m_is_start->raw_select();
    return true;
  }
  return m_fo.fRegexp ? DoSearchRegexp(fMore) : DoSearchStd(fMore);
}

void CFBEView::SelMatch(MSHTML::IHTMLTxtRange *tr,AU::ReMatch rm) {
  tr->collapse(VARIANT_TRUE);
  tr->move(L"character",rm->FirstIndex);
  if (tr->moveStart(L"character",1)==1)
    tr->move(L"character",-1);
  tr->moveEnd(L"character",rm->Length);
  tr->select();
  m_fo.match=rm;
}

bool  CFBEView::DoSearchRegexp(bool fMore) {
  try {
    // well, try to compile it first
    AU::RegExp	    re;
    CheckError(re.CreateInstance(L"VBScript.RegExp"));
    re->IgnoreCase=m_fo.flags&4 ? VARIANT_FALSE : VARIANT_TRUE;
    re->Global=VARIANT_TRUE;
    re->Pattern=(const wchar_t *)m_fo.pattern;

    // locate starting paragraph
    MSHTML::IHTMLTxtRangePtr  sel(Document()->selection->createRange());
    if (!fMore && (bool)m_is_start)
      sel=m_is_start->duplicate();
    if (!(bool)sel)
      return false;

    MSHTML::IHTMLElementPtr   sc(SelectionStructCon());
    long		      s_idx=0;
    long		      s_off1=0;
    long		      s_off2=0;
    if ((bool)sc) {
      s_idx=sc->sourceIndex;
      if (U::scmp(sc->tagName,L"P")==0 && (bool)sel) {
	s_off2=sel->text.length();
	MSHTML::IHTMLTxtRangePtr  pr(sel->duplicate());
	pr->moveToElementText(sc);
	pr->setEndPoint(L"EndToStart",sel);
	s_off1=pr->text.length();
	s_off2+=s_off1;
      }
    }
    // walk the all collection now, looking for the next P
    MSHTML::IHTMLElementCollectionPtr all(Document()->all);
    long			      all_len=all->length;
    long			      incr=m_fo.flags&1 ? -1 : 1;
    bool			      fWrapped=false;

    // * search in starting element
    if ((bool)sc && U::scmp(sc->tagName,L"P")==0) {
      sel->moveToElementText(sc);
      AU::ReMatches  rm(re->Execute(sel->text));
      if (rm->Count > 0) {
	if (incr>0) {
	  for (long l=0;l<rm->Count;++l) {
	    AU::ReMatch	crm(rm->Item[l]);
	    if (crm->FirstIndex >= s_off2) {
	      SelMatch(sel,crm);
	      return true;
	    }
	  }
	} else {
	  for (long l=rm->Count-1;l>=0;--l) {
	    AU::ReMatch	crm(rm->Item[l]);
	    if (crm->FirstIndex < s_off1) {
	      SelMatch(sel,crm);
	      return true;
	    }
	  }
	}
      }
    }

    // search all others
    for (long cur=s_idx+incr;;cur+=incr) {
      // adjust out of bounds indices
      if (cur<0) {
	cur=all_len-1;
	fWrapped=true;
      } else if (cur>=all_len) {
	cur=0;
	fWrapped=true;
      }
      // check for wraparound
      if (cur==s_idx)
	break;
      // check current element type
      MSHTML::IHTMLElementPtr	  elem(all->item(cur));
      if (!(bool)elem || U::scmp(elem->tagName,L"P"))
	continue;
      // search inside current element
      sel->moveToElementText(elem);
      AU::ReMatches  rm(re->Execute(sel->text));
      if (rm->Count <= 0)
	continue;
      if (incr>0)
	SelMatch(sel,rm->Item[0]);
      else
	SelMatch(sel,rm->Item[rm->Count-1]);
      if (fWrapped)
	::MessageBeep(MB_ICONASTERISK);
      return true;
    }
    // search again in starting element
    if ((bool)sc && U::scmp(sc->tagName,L"P")==0) {
      sel->moveToElementText(sc);
      AU::ReMatches  rm(re->Execute(sel->text));
      if (rm->Count > 0) {
	if (incr>0) {
	  for (long l=0;l<rm->Count;++l) {
	    AU::ReMatch	crm(rm->Item[l]);
	    if (crm->FirstIndex < s_off1) {
	      SelMatch(sel,crm);
	      if (fWrapped)
		::MessageBeep(MB_ICONASTERISK);
	      return true;
	    }
	  }
	} else {
	  for (long l=rm->Count-1;l>=0;--l) {
	    AU::ReMatch	crm(rm->Item[l]);
	    if (crm->FirstIndex >= s_off2) {
	      SelMatch(sel,crm);
	      if (fWrapped)
		::MessageBeep(MB_ICONASTERISK);
	      return true;
	    }
	  }
	}
      }
    }
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
  return false;
}

bool  CFBEView::DoSearchStd(bool fMore) {
  try {
    // fetch selection
    MSHTML::IHTMLTxtRangePtr  sel(Document()->selection->createRange());
    if (!fMore && (bool)m_is_start)
      sel=m_is_start->duplicate();
    if (!(bool)sel)
      return false;
    MSHTML::IHTMLTxtRangePtr  org(sel->duplicate());
    // check if it is collapsed
    if (sel->compareEndPoints(L"StartToEnd",sel)!=0) {
      // collapse and advance
      if (m_fo.flags&FRF_REVERSE)
	sel->collapse(VARIANT_TRUE);
      else
	sel->collapse(VARIANT_FALSE);
    }
    // search for text
    if (sel->findText((const wchar_t *)m_fo.pattern,1073741824,m_fo.flags)==VARIANT_TRUE) {
      // ok, found
      sel->select();
      return true;
    }
    // not found, try searching from start to sel
    sel=MSHTML::IHTMLBodyElementPtr(Document()->body)->createTextRange();
    sel->collapse(m_fo.flags&1 ? VARIANT_FALSE : VARIANT_TRUE);
    if (sel->findText((const wchar_t *)m_fo.pattern,1073741824,m_fo.flags)==VARIANT_TRUE &&
      org->compareEndPoints("StartToStart",sel)*(m_fo.flags&1 ? -1 : 1)>0)
    { // found
      sel->select();
      MessageBeep(MB_ICONASTERISK);
      return true;
    }
  }
  catch (_com_error&) { }
  return false;
}

static CString	GetSM(VBScript_RegExp_55::ISubMatches *sm,int idx) {
  if (!sm)
    return CString();
  if (idx<0 || idx>=sm->Count)
    return CString();
  _variant_t  vt(sm->Item[idx]);
  if (V_VT(&vt)==VT_BSTR)
    return V_BSTR(&vt);
  return CString();
}

struct RR {
  enum {
    STRONG=1,
    EMPHASIS=2,
    UPPER=4,
    LOWER=8,
    TITLE=16
  };
  int	    flags;
  int	    start;
  int	    len;
};
typedef CSimpleValArray<RR>  RRList;

static CString GetReplStr(const CString& rstr,VBScript_RegExp_55::IMatch2 *rm,RRList& rl)
{
  CString	  rep;
  rep.GetBuffer(rstr.GetLength());
  rep.ReleaseBuffer(0);

  AU::ReSubMatches rs(rm->SubMatches);

  RR		  cr;
  memset(&cr,0,sizeof(cr));
  int		  flags=0;

  CString  rv;

  for (int i=0;i<rstr.GetLength();++i) {
    if (rstr[i]==_T('$') && i<rstr.GetLength()-1) {
      switch (rstr[++i]) {
      case _T('&'): // whole match
	rv=(const wchar_t *)rm->Value;
	break;

      case _T('+'): // last submatch
	rv=GetSM(rs,rs->Count-1);
	break;

      case _T('1'): case _T('2'): case _T('3'): case _T('4'):
      case _T('5'): case _T('6'): case _T('7'): case _T('8'): case _T('9'):
	rv=GetSM(rs,rstr[i]-_T('0')-1);
	break;

      case _T('T'): // title case
	flags|=RR::TITLE;
	continue;

      case _T('U'): // uppercase
	flags|=RR::UPPER;
	continue;

      case _T('L'): // lowercase
	flags|=RR::LOWER;
	continue;

      case _T('S'): // strong
	flags|=RR::STRONG;
	continue;

      case _T('E'): // emphasis
	flags|=RR::EMPHASIS;
	continue;

      case _T('Q'): // turn off flags
	flags=0;
	continue;

      default: // ignore
	continue;
      }
    }
    if (cr.flags!=flags && cr.flags && cr.start<rep.GetLength()) {
      cr.len=rep.GetLength()-cr.start;
      rl.Add(cr);
      cr.flags=0;
    }
    if (flags) {
      cr.flags=flags;
      cr.start=rep.GetLength();
    }
    if (!rv.IsEmpty()) {
      rep+=rv;
      rv.Empty();
    } else
      rep+=rstr[i];
  }
  if (cr.flags && cr.start<rep.GetLength()) {
    cr.len=rep.GetLength()-cr.start;
    rl.Add(cr);
  }
  // process case conversions here
  int	  tl=rep.GetLength();
  TCHAR	  *cp=rep.GetBuffer(tl);
  for (int j=0;j<rl.GetSize();) {
    RR	  rr=rl[j];
    if (rr.flags&RR::UPPER)
      LCMapString(CP_ACP,LCMAP_UPPERCASE,cp+rr.start,rr.len,cp+rr.start,rr.len);
    else if (rr.flags&RR::LOWER)
      LCMapString(CP_ACP,LCMAP_LOWERCASE,cp+rr.start,rr.len,cp+rr.start,rr.len);
    else if (rr.flags&RR::TITLE && rr.len>0) {
      LCMapString(CP_ACP,LCMAP_UPPERCASE,cp+rr.start,1,cp+rr.start,1);
      LCMapString(CP_ACP,LCMAP_LOWERCASE,cp+rr.start+1,rr.len-1,cp+rr.start+1,rr.len-1);
    }
    if ((rr.flags&~(RR::UPPER|RR::LOWER|RR::TITLE))==0)
      rl.RemoveAt(j);
    else
      ++j;
  }
  rep.ReleaseBuffer(tl);
  return rep;
}

void  CFBEView::DoReplace() {
  try {
    MSHTML::IHTMLTxtRangePtr  sel(Document()->selection->createRange());
    if (!(bool)sel)
      return;
    MSHTML::IHTMLTxtRangePtr  x2(sel->duplicate());
    int			      adv=0;

    if (m_fo.match) { // use regexp match
      RRList	rl;
      CString rep(GetReplStr(m_fo.replacement,m_fo.match,rl));
      sel->text=(const wchar_t *)rep;
      // change bold/italic where needed
      for (int i=0;i<rl.GetSize();++i) {
	RR  rr=rl[i];
	x2=sel->duplicate();
	x2->move(L"character",rr.start-rep.GetLength());
	x2->moveEnd(L"character",rr.len);
	if (rr.flags&RR::STRONG)
	  x2->execCommand(L"Bold",VARIANT_FALSE);
	if (rr.flags&RR::EMPHASIS)
	  x2->execCommand(L"Italic",VARIANT_FALSE);
      }
      adv=rep.GetLength();
    } else { // plain text
      sel->text=(const wchar_t *)m_fo.replacement;
      adv=m_fo.replacement.GetLength();
    }
    sel->moveStart(L"character",-adv);
    sel->select();
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
}

int  CFBEView::GlobalReplace() {
  if (m_fo.pattern.IsEmpty())
    return 0;
  try {
    MSHTML::IHTMLTxtRangePtr  sel(MSHTML::IHTMLBodyElementPtr(Document()->body)->createTextRange());
    if (!(bool)sel)
      return 0;

    AU::RegExp	    re;
    CheckError(re.CreateInstance(L"VBScript.RegExp"));
    re->IgnoreCase=m_fo.flags&4 ? VARIANT_FALSE : VARIANT_TRUE;
    re->Global=VARIANT_TRUE;
    re->Pattern=(const wchar_t *)m_fo.pattern;

    m_mk_srv->BeginUndoUnit((USHORT*)L"replace");

    sel->collapse(VARIANT_TRUE);

    int	  nRepl=0;

    if (m_fo.fRegexp) {
      MSHTML::IHTMLTxtRangePtr  s3;
      MSHTML::IHTMLElementCollectionPtr all(Document()->all);
      _bstr_t	charstr(L"character");
      RRList	rl;
      CString	repl;

      for (long l=0;l<all->length;++l) {
	MSHTML::IHTMLElementPtr	  elem(all->item(l));
	if (!(bool)elem || U::scmp(elem->tagName,L"P"))
	  continue;
	sel->moveToElementText(elem);
	AU::ReMatches  rm(re->Execute(sel->text));
	if (rm->Count <= 0)
	  continue;
	// replace
	sel->collapse(VARIANT_TRUE);
	long	  last=0;
	for (long i=0;i<rm->Count;++i) {
	  AU::ReMatch  cur(rm->Item[i]);
	  long	      delta=cur->FirstIndex - last;
	  if (delta) {
	    sel->move(charstr,delta);
	    last+=delta;
	  }
	  if (sel->moveStart(charstr,1)==1)
	    sel->move(charstr,-1);
	  delta=cur->Length;
	  last+=cur->Length;
	  sel->moveEnd(charstr,delta);
	  rl.RemoveAll();
	  repl=GetReplStr(m_fo.replacement,cur,rl);
	  sel->text=(const wchar_t *)repl;
	  for (int k=0;k<rl.GetSize();++k) {
	    RR  rr=rl[k];
	    s3=sel->duplicate();
	    s3->move(L"character",rr.start-repl.GetLength());
	    s3->moveEnd(L"character",rr.len);
	    if (rr.flags&RR::STRONG)
	      s3->execCommand(L"Bold",VARIANT_FALSE);
	    if (rr.flags&RR::EMPHASIS)
	      s3->execCommand(L"Italic",VARIANT_FALSE);
	  }
	  ++nRepl;
	}
      }
    } else {
      DWORD   flags=m_fo.flags&~FRF_REVERSE;
      _bstr_t pattern((const wchar_t *)m_fo.pattern);
      _bstr_t repl((const wchar_t *)m_fo.replacement);
      while (sel->findText(pattern,1073741824,flags)==VARIANT_TRUE) {
	sel->text=repl;
	++nRepl;
      }
    }

    m_mk_srv->EndUndoUnit();
    return nRepl;
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
  return 0;
}

class CViewFindDlg : public CFindDlgBase {
public:
  CViewFindDlg(CFBEView *view) : CFindDlgBase(view) { }

  virtual void	DoFind() {
    GetData();
    if (!m_view->DoSearch())
      U::MessageBox(MB_OK|MB_ICONEXCLAMATION,_T("FBE"),
	_T("Cannot find the string '%s'."),m_view->m_fo.pattern);
    else {
      SaveString();
      SaveHistory();
      EndDialog(IDOK);
    }
  }
};

class CViewReplaceDlg : public CReplaceDlgBase {
public:
  CViewReplaceDlg(CFBEView *view) : CReplaceDlgBase(view) { }

  virtual void DoFind() {
    if (!m_view->DoSearch())
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
      m_view->DoReplace();
      m_selvalid=false;
    }
    DoFind();
  }
  virtual void DoReplaceAll() {
    int nRepl=m_view->GlobalReplace();
    if (nRepl>0) {
      SaveString();
      SaveHistory();
      U::MessageBox(MB_OK,_T("Replace All"),_T("%d replacement(s) done."),nRepl);
      MakeClose();
      m_selvalid=false;
    } else
      U::MessageBox(MB_OK|MB_ICONEXCLAMATION,_T("FBE"),
	_T("Finished searching for '%s'."),m_view->m_fo.pattern);
  }
};

LRESULT  CFBEView::OnFind(WORD, WORD, HWND, BOOL&) {
  m_fo.pattern=(const wchar_t *)Selection();
  CViewFindDlg    dlg(this);
  dlg.DoModal();
  return 0;
}

LRESULT  CFBEView::OnReplace(WORD, WORD, HWND, BOOL&) {
  m_fo.pattern=(const wchar_t *)Selection();
  CViewReplaceDlg    dlg(this);
  dlg.DoModal();
  return 0;
}

LRESULT  CFBEView::OnFindNext(WORD, WORD, HWND, BOOL&) {
  if (!DoSearch())
    U::MessageBox(MB_OK|MB_ICONEXCLAMATION,_T("FBE"),
      _T("Cannot find the string '%s'."),m_fo.pattern);
  return 0;
}

// binary objects
_variant_t	CFBEView::GetBinary(const wchar_t *id) {
  try {
    CComDispatchDriver    dd(Script());
    _variant_t    ret;
    _variant_t    arg(id);
    if (SUCCEEDED(dd.Invoke1(L"GetBinary",&arg,&ret)))
      return ret;
  }
  catch (_com_error&) { }
  return _variant_t();
}

// change notifications
void	CFBEView::EditorChanged(int id) {
  switch (id) {
  case FWD_SINK:
    break;
  case BACK_SINK:
    break;
  case RANGE_SINK:
    if (!m_ignore_changes)
      ::SendMessage(m_frame,WM_COMMAND,MAKELONG(0,IDN_ED_CHANGED),(LPARAM)m_hWnd);
    break;
  }
}

// DWebBrowserEvents2
void  CFBEView::OnDocumentComplete(IDispatch *pDisp,VARIANT *vtUrl) {
  m_complete=true;
}

void  CFBEView::Init() {
  // save document pointer
  m_hdoc=m_browser->Document;

  m_mk_srv=m_hdoc;
  m_mkc=m_hdoc;

  // attach document events handler
  DocumentEvents::DispEventAdvise(Document(),&DIID_HTMLDocumentEvents2);
  TextEvents::DispEventAdvise(Document()->body,&DIID_HTMLTextContainerEvents2);

  // attach editing changed handlers
  m_mkc->RegisterForDirtyRange((RangeSink*)this,&m_dirtyRangeCookie);

  // attach external helper
  SetExternalDispatch(CreateHelper());

  // fixup all P elements
  FixupParagraphs(Document()->body);

  if (m_normalize)
    Normalize(Document()->body);

  if (!m_normalize) {
    // check ID and version fields
    MSHTML::IHTMLInputElementPtr	    ii(Document()->all->item(L"diID"));
    if ((bool)ii && ii->value.length()==0) { // generate new ID
      UUID	      uuid;
      unsigned char *str;
      if (UuidCreate(&uuid)==RPC_S_OK && UuidToStringA(&uuid,&str)==RPC_S_OK) {
	CString     us(str);
	RpcStringFreeA(&str);
	us.MakeUpper();
	ii->value=(const wchar_t *)us;
      }
    }
    ii=Document()->all->item(L"diVersion");
    if ((bool)ii && ii->value.length()==0)
      ii->value=L"1.0";
    ii=Document()->all->item(L"diDate");
    MSHTML::IHTMLInputElementPtr jj(Document()->all->item(L"diDateVal"));
    if ((bool)ii && (bool)jj && ii->value.length()==0 && jj->value.length()==0) {
      time_t  tt;
      time(&tt);
      char    buffer[128];
      strftime(buffer,sizeof(buffer),"%Y-%m-%d",localtime(&tt));
      ii->value=buffer;
      jj->value=buffer;
    }
    ii=Document()->all->item(L"diProgs");
    if ((bool)ii && ii->value.length()==0)
      ii->value=L"FB Tools";
  }

  // turn off browser's d&d
  m_browser->RegisterAsDropTarget=VARIANT_FALSE;

  m_initialized=true;
}

void  CFBEView::OnBeforeNavigate(IDispatch *pDisp,VARIANT *vtUrl,VARIANT *vtFlags,
				 VARIANT *vtTargetFrame,VARIANT *vtPostData,
				 VARIANT *vtHeaders,VARIANT_BOOL *fCancel)
{
  if (!m_initialized)
    return;

  if (vtUrl && V_VT(vtUrl)==VT_BSTR) {
    m_nav_url=V_BSTR(vtUrl);

    if (m_nav_url.Left(13)==_T("fbe-internal:"))
      return;

    ::SendMessage(m_frame,WM_COMMAND,MAKELONG(0,IDN_NAVIGATE),(LPARAM)m_hWnd);
  }

  // disable navigating away
  *fCancel=VARIANT_TRUE;
}

// HTMLDocumentEvents
void  CFBEView::OnSelChange(IDispatch *evt) {
  if (!m_ignore_changes)
    ::SendMessage(m_frame,WM_COMMAND,MAKELONG(0,IDN_SEL_CHANGE),(LPARAM)m_hWnd);
  if (m_cur_sel)
    m_cur_sel.Release();
}

VARIANT_BOOL  CFBEView::OnContextMenu(IDispatch *evt) {
  MSHTML::IHTMLEventObjPtr    oe(evt);
  oe->cancelBubble=VARIANT_TRUE;
  oe->returnValue=VARIANT_FALSE;
  if (!m_normalize) {
    MSHTML::IHTMLElementPtr   elem(oe->srcElement);
    if (!(bool)elem)
      return VARIANT_TRUE;
    if (U::scmp(elem->tagName,L"INPUT") && U::scmp(elem->tagName,L"TEXTAREA"))
      return VARIANT_TRUE;
  }
  // display custom context menu here
  CMenu	  menu;
  menu.CreatePopupMenu();
  menu.AppendMenu(MF_STRING,ID_EDIT_UNDO,_T("&Undo"));
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING,ID_EDIT_CUT,_T("Cu&t"));
  menu.AppendMenu(MF_STRING,ID_EDIT_COPY,_T("&Copy"));
  menu.AppendMenu(MF_STRING,ID_EDIT_PASTE,_T("&Paste"));
  if (m_normalize) {
    menu.AppendMenu(MF_SEPARATOR);
    MSHTML::IHTMLElementPtr   cur(SelectionContainer());
    int			      cmd=ID_SEL_BASE;
    while ((bool)cur && U::scmp(cur->tagName,L"BODY")) {
      menu.AppendMenu(MF_STRING,cmd,_T("Select ")+GetPath(cur));
      cur=cur->parentElement;
      ++cmd;
    }
  }

  AU::TRACKPARAMS   tp;
  tp.hMenu=menu;
  tp.uFlags=TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON;
  tp.x=oe->screenX;
  tp.y=oe->screenY;
  ::SendMessage(m_frame,AU::WM_TRACKPOPUPMENU,0,(LPARAM)&tp);

  return VARIANT_TRUE;
}

LRESULT CFBEView::OnSelectElement(WORD, WORD wID, HWND, BOOL&) {
  int	steps=wID-ID_SEL_BASE;
  try {
    MSHTML::IHTMLElementPtr	  cur(SelectionContainer());

    while ((bool)cur && steps-->0)
      cur=cur->parentElement;

    MSHTML::IHTMLTxtRangePtr	  r(MSHTML::IHTMLBodyElementPtr(Document()->body)->createTextRange());

    r->moveToElementText(cur);

    ++m_ignore_changes;
    r->select();
    --m_ignore_changes;

    m_cur_sel=cur;
    ::SendMessage(m_frame,WM_COMMAND,MAKELONG(0,IDN_SEL_CHANGE),(LPARAM)m_hWnd);
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }

  return 0;
}

VARIANT_BOOL  CFBEView::OnClick(IDispatch *evt) {
  MSHTML::IHTMLEventObjPtr    oe(evt);
  MSHTML::IHTMLElementPtr     elem(oe->srcElement);

  if (!(bool)elem)
    return VARIANT_FALSE;
  if (U::scmp(elem->tagName,L"A"))
    return VARIANT_FALSE;
  if (oe->altKey!=VARIANT_TRUE || oe->shiftKey==VARIANT_TRUE || oe->ctrlKey==VARIANT_TRUE)
    return VARIANT_FALSE;

  CString	    sref(AU::GetAttrCS(elem,L"href"));
  if (sref.IsEmpty() || sref[0]!=_T('#'))
    return VARIANT_FALSE;

  sref.Delete(0);

  MSHTML::IHTMLElementPtr     targ(Document()->all->item((const wchar_t *)sref));

  if (!(bool)targ)
    return VARIANT_FALSE;

  GoTo(targ);

  oe->cancelBubble=VARIANT_TRUE;
  oe->returnValue=VARIANT_FALSE;

  return VARIANT_TRUE;
}

VARIANT_BOOL  CFBEView::OnRealPaste(IDispatch *evt) {
  MSHTML::IHTMLEventObjPtr    oe(evt);
  oe->cancelBubble=VARIANT_TRUE;
  if (!m_enable_paste) {
    oe->returnValue=VARIANT_FALSE;
    PostMessage(WM_COMMAND,MAKELONG(ID_EDIT_PASTE,0),0);
  } else
    oe->returnValue=VARIANT_TRUE;
  return VARIANT_TRUE;
}

bool  CFBEView::IsFormChanged() {
  if (!m_form_changed && (bool)m_cur_input)
    m_form_changed=m_form_changed || m_cur_input->value != m_cur_val;
  return m_form_changed;
}

bool  CFBEView::IsFormCP() {
  if (!m_form_cp && (bool)m_cur_input)
    m_form_cp=m_form_cp || m_cur_input->value != m_cur_val;
  return m_form_cp;
}

void  CFBEView::ResetFormChanged() {
  m_form_changed=false;
  if (m_cur_input)
    m_cur_val=m_cur_input->value;
}

void  CFBEView::ResetFormCP() {
  m_form_cp=false;
  if (m_cur_input)
    m_cur_val=m_cur_input->value;
}

void  CFBEView::OnFocusIn(IDispatch *evt) {
  // check previous value
  if (m_cur_input) {
    bool cv=m_cur_input->value != m_cur_val;
    m_form_changed=m_form_changed || cv;
    m_form_cp=m_form_cp || cv;
    m_cur_input.Release();
  }

  MSHTML::IHTMLEventObjPtr  oe(evt);
  if (!(bool)oe)
    return;

  MSHTML::IHTMLElementPtr   te(oe->srcElement);
  if (!(bool)te || U::scmp(te->tagName,L"INPUT"))
    return;

  m_cur_input=te;
  if (!(bool)m_cur_input)
    return;

  if (U::scmp(m_cur_input->type,L"text")) {
    m_cur_input.Release();
    return;
  }

  m_cur_val=m_cur_input->value;
}

// find/replace support for scintilla
bool CFBEView::SciFindNext(HWND src,bool fFwdOnly,bool fBarf) {
  if (m_fo.pattern.IsEmpty())
    return true;

  int	    flags=0;
  if (m_fo.flags & FRF_WHOLE)
    flags|=SCFIND_WHOLEWORD;
  if (m_fo.flags & FRF_CASE)
    flags|=SCFIND_MATCHCASE;
  if (m_fo.fRegexp)
    flags|=SCFIND_REGEXP|SCFIND_POSIX;
  int rev=m_fo.flags & FRF_REVERSE && !fFwdOnly;
  DWORD   len=::WideCharToMultiByte(CP_UTF8,0,
		  m_fo.pattern,m_fo.pattern.GetLength(),
		  NULL,0,NULL,NULL);
  char    *tmp=(char *)malloc(len+1);
  if (tmp) {
    ::WideCharToMultiByte(CP_UTF8,0,
		  m_fo.pattern,m_fo.pattern.GetLength(),
		  tmp,len,NULL,NULL);
    tmp[len]='\0';
    int p1=::SendMessage(src,SCI_GETSELECTIONSTART,0,0);
    int p2=::SendMessage(src,SCI_GETSELECTIONEND,0,0);
    if (p1!=p2 && !rev)
      ++p1;
    if (rev)
      --p1;
    if (p1<0)
      p1=0;
    p2=rev ? 0 : ::SendMessage(src,SCI_GETLENGTH,0,0);
    int p3=p2==0 ? ::SendMessage(src,SCI_GETLENGTH,0,0) : 0;
    ::SendMessage(src,SCI_SETTARGETSTART,p1,0);
    ::SendMessage(src,SCI_SETTARGETEND,p2,0);
    ::SendMessage(src,SCI_SETSEARCHFLAGS,flags,0);
    // this sometimes hangs in reverse search :)
    int ret=::SendMessage(src,SCI_SEARCHINTARGET,len,(LPARAM)tmp);
    if (ret==-1) { // try wrap
      if (p1!=p3) {
	::SendMessage(src,SCI_SETTARGETSTART,p3,0);
	::SendMessage(src,SCI_SETTARGETEND,p1,0);
	::SendMessage(src,SCI_SETSEARCHFLAGS,flags,0);
	ret=::SendMessage(src,SCI_SEARCHINTARGET,len,(LPARAM)tmp);
      }
      if (ret==-1) {
	free(tmp);
	if (fBarf)
	  U::MessageBox(MB_OK|MB_ICONEXCLAMATION,_T("FBE"),
	    _T("Cannot find the string '%s'."),m_fo.pattern);
	return false;
      }
      ::MessageBeep(MB_ICONASTERISK);
    }
    free(tmp);
    p1=::SendMessage(src,SCI_GETTARGETSTART,0,0);
    p2=::SendMessage(src,SCI_GETTARGETEND,0,0);
    ::SendMessage(src,SCI_SETSELECTIONSTART,p1,0);
    ::SendMessage(src,SCI_SETSELECTIONEND,p2,0);
    ::SendMessage(src,SCI_SCROLLCARET,0,0);
    return true;
  } else
    ::MessageBox(::GetActiveWindow(),_T("Out of memory"),_T("FBE"),
		  MB_OK|MB_ICONERROR);

  return false;
}

_bstr_t	  CFBEView::Selection() {
  try {
    MSHTML::IHTMLTxtRangePtr	rng(Document()->selection->createRange());
    if (!(bool)rng)
      return _bstr_t();

    MSHTML::IHTMLTxtRangePtr	dup(rng->duplicate());
    dup->collapse(VARIANT_TRUE);

    MSHTML::IHTMLElementPtr	elem(dup->parentElement());
    while ((bool)elem && U::scmp(elem->tagName,L"P") && U::scmp(elem->tagName,L"DIV"))
      elem=elem->parentElement;

    if (elem) {
      dup->moveToElementText(elem);
      if (rng->compareEndPoints(L"EndToEnd",dup)>0)
	rng->setEndPoint(L"EndToEnd",dup);
    }

    return rng->text;
  }
  catch (_com_error) {
  }

  return _bstr_t();
}