// Doc.cpp: implementation of the Doc class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include "FBEView.h"
#include "FBDoc.h"
#include "Scintilla.h"

namespace FB {

// namespaces
const _bstr_t	  FBNS(L"http://www.gribuser.ru/xml/fictionbook/2.0");
const _bstr_t	  XLINKNS(L"http://www.w3.org/1999/xlink");
const _bstr_t	  NEWLINE(L"\n");

// document list
CSimpleMap<Doc*,Doc*>	Doc::m_active_docs;

Doc   *Doc::LocateDocument(const wchar_t *id) {
  unsigned long	  *lv;
  if (swscanf(id,L"%lu",&lv)!=1)
    return NULL;
  return m_active_docs.Lookup((Doc*)lv);
}

// initialize a new Doc
Doc::Doc(HWND hWndFrame) :
	     m_filename(_T("Untitled.fb2")), m_namevalid(false),
	     m_desc(hWndFrame,false), m_body(hWndFrame,true),
	     m_frame(hWndFrame), m_desc_ver(-1), m_body_ver(-1),
	     m_desc_cp(-1), m_body_cp(-1),
	     m_encoding(_T("utf-8"))
{
  m_active_docs.Add(this,this);
}

// destroy a Doc
Doc::~Doc() {
  // destroy windows explicitly
  if (m_desc.IsWindow())
    m_desc.DestroyWindow();
  if (m_body.IsWindow())
    m_body.DestroyWindow();
  m_active_docs.Remove(this);
}

bool  Doc::GetBinary(const wchar_t *id,_variant_t& vt) {
  if (id && *id==L'#') {
    CComDispatchDriver	    desc(m_desc.Script());
    _variant_t	  vid(id+1);
    desc.Invoke1(L"GetBinary",&vid,&vt);
    return true;
  }
  return false;
}

struct ThreadArgs {
  MSXML2::IXSLProcessor	*proc;
  HANDLE		hWr;
};

static DWORD __stdcall XMLTransformThread(LPVOID varg) {
  ThreadArgs			*arg=(ThreadArgs*)varg;

  arg->proc->put_output(_variant_t(U::NewStream(arg->hWr)));
  VARIANT_BOOL	val;
  arg->proc->raw_transform(&val);
  arg->proc->Release();

  delete arg;

  return 0;
}

void Doc::TransformXML(MSXML2::IXSLTemplatePtr tp,MSXML2::IXMLDOMDocument2Ptr doc,
    CFBEView& dest)
{
  // create processor
  MSXML2::IXSLProcessorPtr	proc(tp->createProcessor());
  proc->input=_variant_t(doc.GetInterfacePtr());
  
  // add parameters
  CString	  fss(U::GetSettingS(_T("Font")));
  if (!fss.IsEmpty())
    proc->addParameter(L"font",(const wchar_t *)fss,_bstr_t());
  
  DWORD		  fs=U::GetSettingI(_T("FontSize"),1);
  if (fs>1) {
    fss.Format(_T("%d"),fs);
    proc->addParameter(L"fontSize",(const wchar_t *)fss,_bstr_t());
  }
  
  fs=U::GetSettingI(_T("ColorFG"),CLR_DEFAULT);
  if (fs!=CLR_DEFAULT) {
    fss.Format(_T("rgb(%d,%d,%d)"),GetRValue(fs),GetGValue(fs),GetBValue(fs));
    proc->addParameter(L"colorFG",(const wchar_t *)fss,_bstr_t());
  }

  fs=U::GetSettingI(_T("ColorBG"),CLR_DEFAULT);
  if (fs!=CLR_DEFAULT) {
    fss.Format(_T("rgb(%d,%d,%d)"),GetRValue(fs),GetGValue(fs),GetBValue(fs));
    proc->addParameter(L"colorBG",(const wchar_t *)fss,_bstr_t());
  }

  fs=::GetSysColor(COLOR_BTNFACE);
  fss.Format(_T("rgb(%d,%d,%d)"),GetRValue(fs),GetGValue(fs),GetBValue(fs));
  proc->addParameter(L"dlgBG",(const wchar_t *)fss,_bstr_t());

  fs=::GetSysColor(COLOR_BTNTEXT);
  fss.Format(_T("rgb(%d,%d,%d)"),GetRValue(fs),GetGValue(fs),GetBValue(fs));
  proc->addParameter(L"dlgFG",(const wchar_t *)fss,_bstr_t());

  proc->addParameter(L"dID",(const wchar_t *)MyID(),_bstr_t());

  // add jscript paths
  proc->addParameter(L"bodyscript",(const wchar_t *)U::UrlFromPath(U::GetProgDirFile(_T("body.js"))),_bstr_t());
  proc->addParameter(L"descscript",(const wchar_t *)U::UrlFromPath(U::GetProgDirFile(_T("desc.js"))),_bstr_t());

  ThreadArgs	*arg=new ThreadArgs;

  // pass the processor to worker thread, this is a dirty hack, but we know that
  // MSXML survives it, otherwise we'd have to jump through the hoops with
  // COM MTAs, because if we just marshal the pointer to worker thread, then
  // we'll deadlock in load. To do everything cleanly, we'd need to create the
  // XSL processor and XML documents in an MTA, and run the transforms there,
  // all this is rather awkward. Another alternative is to transform all XML
  // to memory and the load from it
  arg->proc=proc.Detach();

  // setup the streams
  HANDLE	  hRd;
  ::CreatePipe(&hRd,&arg->hWr,NULL,0);

  // start the processor
  ::CloseHandle(::CreateThread(NULL,0,XMLTransformThread,arg,0,NULL));

  // now stuff the data into mshtml
  IPersistStreamInitPtr	ips(dest.Browser()->Document);
  ips->InitNew();
  ips->Load(U::NewStream(hRd));
}

static MSXML2::IXSLTemplatePtr	LoadXSL(const CString& path) {
  MSXML2::IXMLDOMDocument2Ptr	xsl(U::CreateDocument(true));
  if (!U::LoadXml(xsl,U::GetProgDirFile(path)))
    throw _com_error(E_FAIL);
  MSXML2::IXSLTemplatePtr	tp(U::CreateTemplate());
  tp->stylesheet=xsl;
  return tp;
}

// loading
bool	Doc::LoadFromDOM(HWND hWndParent,MSXML2::IXMLDOMDocument2 *dom) {
  try {
    dom->setProperty(L"SelectionLanguage",L"XPath");
    CString   nsprop(L"xmlns:fb='");
    nsprop+=(const wchar_t *)FBNS;
    nsprop+=L"' xmlns:xlink='";
    nsprop+=(const wchar_t *)XLINKNS;
    nsprop+=L"'";
    dom->setProperty(L"SelectionNamespaces",(const TCHAR *)nsprop);

    // try to find out current encoding
    MSXML2::IXMLDOMProcessingInstructionPtr   pi(dom->firstChild);
    if (pi) {
      MSXML2::IXMLDOMNamedNodeMapPtr  attr(pi->attributes);
      if (attr) {
	MSXML2::IXMLDOMNodePtr	enc(attr->getNamedItem(L"encoding"));
	if (enc)
	  m_encoding=(const wchar_t *)enc->text;
      }
    }

    // create desc view
    m_desc.Create(hWndParent, CRect(0,0,500,500), _T("{8856F961-340A-11D0-A96B-00C04FD705A2}"));

    // navigate to blank page
    m_desc.Browser()->Navigate(L"about:blank");

    // run a message loop until it loads
    MSG	  msg;
    while (!m_desc.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    // transform to html
    TransformXML(LoadXSL(_T("description.xsl")),dom,m_desc);

    // wait until it loads
    while (!m_desc.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    // initialize view
    m_desc.Init();

    // store binaries
    CComDispatchDriver	  desc(m_desc.Script());
    _variant_t	    arg(dom);
    desc.Invoke1(L"PutBinaries",&arg);

    // create body view
    m_body.Create(hWndParent, CRect(0,0,500,500), _T("{8856F961-340A-11D0-A96B-00C04FD705A2}"));

    // navigate body browser
    m_body.Browser()->Navigate(L"about:blank");

    // wait until it loads
    while (!m_body.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    // transform to html
    TransformXML(LoadXSL(_T("body.xsl")),dom,m_body);

    // wait until it loads
    while (!m_body.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    // initialize view
    m_body.Init();

    // mark unchanged
    MarkSavePoint();

    // ok, now setup filename and return
    m_filename=_T("Untitled");
    m_namevalid=false;
  }
  catch (_com_error& e) {
    U::ReportError(e);
    return false;
  }

  return true;
}

bool	Doc::Load(HWND hWndParent,const CString& filename) {
  try {
    AU::CPersistentWaitCursor wc;

    // load document into DOM
    MSXML2::IXMLDOMDocument2Ptr	dom(U::CreateDocument(true));
    if (!U::LoadXml(dom,filename))
      return false;

    if (!LoadFromDOM(hWndParent,dom))
      return false;

    m_filename=filename;
    m_namevalid=true;
  }
  catch (_com_error& e) {
    U::ReportError(e);
    return false;
  }
  return true;
}

void  Doc::CreateBlank(HWND hWndParent) {
  try {
    // load document into DOM
    LoadFromDOM(hWndParent,U::CreateDocument(true));
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
}

// indent something
static void   Indent(MSXML2::IXMLDOMNode *node,MSXML2::IXMLDOMDocument2 *xml,
		     int len)
{
  // inefficient
  BSTR	s=SysAllocStringLen(NULL,len+2);
  if (s) {
    s[0]=L'\r';
    s[1]=L'\n';
    for (BSTR p=s+2,q=s+2+len;p<q;++p)
      *p=L' ';
    MSXML2::IXMLDOMTextPtr	text;
    if (SUCCEEDED(xml->raw_createTextNode(s,&text)))
      node->raw_appendChild(text,NULL);
    SysFreeString(s);
  }
}

// set an attribute on the element
static void   SetAttr(MSXML2::IXMLDOMElement *xe,const wchar_t *name,
		      const wchar_t *ns,const _bstr_t& val,
		      MSXML2::IXMLDOMDocument2 *doc)
{
  MSXML2::IXMLDOMAttributePtr  attr(doc->createNode(2L,name,ns));
  attr->appendChild(doc->createTextNode(val));
  xe->setAttributeNode(attr);
}

// setup an ID for the element
static void   SetID(MSHTML::IHTMLElement *he,MSXML2::IXMLDOMElement *xe,
		    MSXML2::IXMLDOMDocument2 *doc) {
  _bstr_t     id(he->id);
  if (id.length()>0)
    SetAttr(xe,L"id",FBNS,id,doc);
}

// copy text
static MSXML2::IXMLDOMTextPtr MkText(MSHTML::IHTMLDOMNode *hn,MSXML2::IXMLDOMDocument2 *xml)
{
  VARIANT   vt;
  VariantInit(&vt);
  CheckError(hn->get_nodeValue(&vt));
  if (V_VT(&vt)!=VT_BSTR) {
    VariantClear(&vt);
    return xml->createTextNode(_bstr_t());
  }
  MSXML2::IXMLDOMText	    *txt;
  HRESULT   hr=xml->raw_createTextNode(V_BSTR(&vt),&txt);
  VariantClear(&vt);
  CheckError(hr);
  return MSXML2::IXMLDOMTextPtr(txt,FALSE);
}

// set an href attribute
static void SetHref(MSXML2::IXMLDOMElementPtr xe,MSXML2::IXMLDOMDocument2 *xml,
		    const _bstr_t& href)
{
  SetAttr(xe,L"l:href",XLINKNS,href,xml);
}

// handle inline formatting
static MSXML2::IXMLDOMNodePtr	  ProcessInline(MSHTML::IHTMLDOMNode *inl,
						MSXML2::IXMLDOMDocument2 *doc)
{
  _bstr_t		      name(inl->nodeName);
  MSHTML::IHTMLElementPtr     einl(inl);
  _bstr_t		      cls(einl->className);

  const wchar_t		      *xname=NULL;
  bool			      fA=false;
  bool			      fStyle=false;

  if (U::scmp(name,L"STRONG")==0)
    xname=L"strong";
  else if (U::scmp(name,L"EM")==0)
    xname=L"emphasis";
  else if (U::scmp(name,L"A")==0) {
    xname=L"a"; fA=true;
  } else if (U::scmp(name,L"SPAN")==0) {
    xname=L"style"; fStyle=true;
  }

  MSXML2::IXMLDOMElementPtr   xinl(doc->createNode(1L,xname,FBNS));

  if (fA) {
    SetHref(xinl,doc,AU::GetAttrB(einl,L"href"));
    if (U::scmp(cls,L"note")==0)
      SetAttr(xinl,L"type",FBNS,cls,doc);
  }
  if (fStyle)
    SetAttr(xinl,L"name",FBNS,cls,doc);

  MSHTML::IHTMLDOMNodePtr     cn(inl->firstChild);

  while ((bool)cn) {
    if (cn->nodeType==3)
      xinl->appendChild(MkText(cn,doc));
    else if (cn->nodeType==1)
      xinl->appendChild(ProcessInline(cn,doc));
    cn=cn->nextSibling;
  }

  return xinl;
}

// handle a paragraph element with subelements
static MSXML2::IXMLDOMNodePtr	  ProcessP(MSHTML::IHTMLElement *p,
					   MSXML2::IXMLDOMDocument2 *doc,
					   const wchar_t *baseName)
{
  _bstr_t		    cls(p->className);

  if (U::scmp(cls,L"text-author")==0)
    baseName=L"text-author";
  else if (U::scmp(cls,L"subtitle")==0)
    baseName=L"subtitle";

  MSHTML::IHTMLDOMNodePtr   hp(p);

  // check if it is an empty-line
  if (hp->hasChildNodes()==VARIANT_FALSE ||
      (!(bool)hp->firstChild->nextSibling && hp->firstChild->nodeType==3 &&
	U::is_whitespace(hp->firstChild->nodeValue.bstrVal)))
  {
    if (MSHTML::IHTMLElement3Ptr(p)->inflateBlock==VARIANT_TRUE)
      return doc->createNode(1L,L"empty-line",FBNS);
    return MSXML2::IXMLDOMNodePtr();
  }

  MSXML2::IXMLDOMElementPtr xp(doc->createNode(1L,baseName,FBNS));

  SetID(p,xp,doc);

  _bstr_t	style(AU::GetAttrB(p,L"fbstyle"));
  if (style.length()>0)
    SetAttr(xp,L"style",FBNS,style,doc);

  hp=hp->firstChild;

  while ((bool)hp) {
    if (hp->nodeType==3) // text segment
      xp->appendChild(MkText(hp,doc));
    else if (hp->nodeType==1)
      xp->appendChild(ProcessInline(hp,doc));
    hp=hp->nextSibling;
  }

  return xp;
}

// handle a div element with subelements
static MSXML2::IXMLDOMNodePtr	  ProcessDiv(MSHTML::IHTMLElement *div,
					     MSXML2::IXMLDOMDocument2 *doc,
					     int indent)
{
  _bstr_t		    cls(div->className);
  MSXML2::IXMLDOMElementPtr xdiv(doc->createNode(1L,cls,FBNS));

  if (U::scmp(cls,L"image")==0) {
    SetID(div,xdiv,doc);
    SetHref(xdiv,doc,AU::GetAttrB(div,L"href"));
    return xdiv;
  }

  SetID(div,xdiv,doc);

  MSHTML::IHTMLDOMNodePtr   ndiv(div);
  MSHTML::IHTMLDOMNodePtr   fc(ndiv->firstChild);

  const wchar_t		    *bn=U::scmp(cls,L"stanza")==0 ? L"v" : L"p";

  while ((bool)fc) {
    _bstr_t	name(fc->nodeName);
    MSHTML::IHTMLElementPtr efc(fc);
    if (U::scmp(name,L"DIV")==0) {
      Indent(xdiv,doc,indent+1);
      xdiv->appendChild(ProcessDiv(efc,doc,indent+1));
    } else if (U::scmp(name,L"P")==0) {
      MSXML2::IXMLDOMNodePtr  np(ProcessP(efc,doc,bn));
      if (np) {
	Indent(xdiv,doc,indent+1);
	xdiv->appendChild(np);
      }
    }

    fc=fc->nextSibling;
  }

  Indent(xdiv,doc,indent);

  return xdiv;
}

// find a first named DIV
static MSXML2::IXMLDOMNodePtr  GetDiv(MSHTML::IHTMLElementPtr body,
				      MSXML2::IXMLDOMDocument2 *xml,
				      const wchar_t *name,
				      int indent)
{
  MSHTML::IHTMLElementCollectionPtr children(body->children);
  long				    c_len=children->length;

  for (long i=0;i<c_len;++i) {
    MSHTML::IHTMLElementPtr div(children->item(i));
    if (!(bool)div)
      continue;
    if (U::scmp(div->tagName,L"DIV")==0 && U::scmp(div->className,name)==0)
      return ProcessDiv(div,xml,indent);
  }

  return MSXML2::IXMLDOMNodePtr();
}

// fetch bodies
static void   GetBodies(MSHTML::IHTMLElementPtr	body,
			MSXML2::IXMLDOMDocument2 *doc)
{
  MSHTML::IHTMLElementCollectionPtr children(body->children);
  long			      c_len=children->length;

  for (long i=0;i<c_len;++i) {
    MSHTML::IHTMLElementPtr div(children->item(i));
    if (!(bool)div)
      continue;
    if (U::scmp(div->tagName,L"DIV")==0 && U::scmp(div->className,L"body")==0) {
      MSXML2::IXMLDOMElementPtr	xb(ProcessDiv(div,doc,1));
      _bstr_t	  bn(AU::GetAttrB(div,L"fbname"));
      if (bn.length()>0)
	SetAttr(xb,L"name",FBNS,bn,doc);
      Indent(doc->documentElement,doc,1);
      doc->documentElement->appendChild(xb);
    }
  }
}

// validator object
class SAXErrorHandler: public CComObjectRoot, public MSXML2::ISAXErrorHandler {
public:
  CString     m_msg;
  int	      m_line,m_col;

  SAXErrorHandler() : m_line(0),m_col(0) { }

  void	SetMsg(MSXML2::ISAXLocator *loc, const wchar_t *msg, HRESULT hr) {
    if (!m_msg.IsEmpty())
      return;
    m_msg=msg;
    CString   ns;
    ns.Format(_T("{%s}"),(const TCHAR *)FBNS);
    m_msg.Replace(ns,_T(""));
    ns.Format(_T("{%s}"),(const TCHAR *)XLINKNS);
    m_msg.Replace(ns,_T("xlink"));
    m_line=loc->getLineNumber();
    m_col=loc->getColumnNumber();
  }

  BEGIN_COM_MAP(SAXErrorHandler)
    COM_INTERFACE_ENTRY(MSXML2::ISAXErrorHandler)
  END_COM_MAP()

  STDMETHOD(raw_error)(MSXML2::ISAXLocator *loc, wchar_t *msg, HRESULT hr) {
    SetMsg(loc,msg,hr);
    return E_FAIL;
  }
  STDMETHOD(raw_fatalError)(MSXML2::ISAXLocator *loc, wchar_t *msg, HRESULT hr) {
    SetMsg(loc,msg,hr);
    return E_FAIL;
  }
  STDMETHOD(raw_ignorableWarning)(MSXML2::ISAXLocator *loc, wchar_t *msg, HRESULT hr) {
    SetMsg(loc,msg,hr);
    return E_FAIL;
  }
};

MSXML2::IXMLDOMDocument2Ptr Doc::CreateDOMImp(const CString& encoding) {
  // normalize body first
  m_body.Normalize(m_body.Document()->body);

  // create document
  MSXML2::IXMLDOMDocument2Ptr	ndoc(U::CreateDocument(false));
  ndoc->async=VARIANT_FALSE;

  // set encoding
  if (!encoding.IsEmpty())
    ndoc->appendChild(ndoc->createProcessingInstruction(L"xml",(const wchar_t *)(L"version=\"1.0\" encoding=\""+encoding+L"\"")));

  // create document element
  MSXML2::IXMLDOMElementPtr	root=
    ndoc->createNode(_variant_t(1L),L"FictionBook",FBNS);
  root->setAttribute(L"xmlns:l",XLINKNS);
  ndoc->documentElement=MSXML2::IXMLDOMElementPtr(root);

  // enable xpath queries
  ndoc->setProperty(L"SelectionLanguage",L"XPath");
  CString   nsprop(L"xmlns:fb='");
  nsprop+=(const wchar_t *)FBNS;
  nsprop+=L"' xmlns:xlink='";
  nsprop+=(const wchar_t *)XLINKNS;
  nsprop+=L"'";
  ndoc->setProperty(L"SelectionNamespaces",(const TCHAR *)nsprop);

  // fetch annotation
  MSXML2::IXMLDOMNodePtr  ann(GetDiv(m_body.Document()->body,ndoc,L"annotation",3));

  // fetch history
  MSXML2::IXMLDOMNodePtr  hist(GetDiv(m_body.Document()->body,ndoc,L"history",3));

  // fetch description
  CComDispatchDriver	    desc(m_desc.Script());
  _variant_t		    args[3];
  if (hist)
    args[0]=hist.GetInterfacePtr();
  if (ann)
    args[1]=ann.GetInterfacePtr();
  args[2]=ndoc.GetInterfacePtr();
  CheckError(desc.InvokeN(L"GetDesc",&args[0],3));

  // fetch body elements
  GetBodies(m_body.Document()->body,ndoc);

  // fetch binaries
  CheckError(desc.Invoke1(L"GetBinaries",&args[2]));

  Indent(root,ndoc,0);

  return ndoc;
}

MSXML2::IXMLDOMDocument2Ptr Doc::CreateDOM(const CString& encoding) {
  try {
    return CreateDOMImp(encoding);
  }
  catch (_com_error& e) {
    U::ReportError(e);
  }
  return NULL;
}

bool  Doc::SaveToFile(const CString& filename,bool fValidateOnly,
		      int *errline,int *errcol)
{
  try {
    // create a schema collection
    MSXML2::IXMLDOMSchemaCollection2Ptr	scol;
    CheckError(scol.CreateInstance(L"Msxml2.XMLSchemaCache.4.0"));

    // load fictionbook schema
    scol->add(FBNS,(const wchar_t *)U::GetProgDirFile(L"FictionBook.xsd"));

    // create a SAX reader
    MSXML2::ISAXXMLReaderPtr	  rdr;
    CheckError(rdr.CreateInstance(L"Msxml2.SAXXMLReader.4.0"));

    // attach a schema
    rdr->putFeature((USHORT*)L"schema-validation",VARIANT_TRUE);
    rdr->putProperty((USHORT*)L"schemas",scol.GetInterfacePtr());
    rdr->putFeature((USHORT*)L"exhaustive-errors",VARIANT_TRUE);

    // create an error handler
    CComObject<SAXErrorHandler>	  *ehp;
    CheckError(CComObject<SAXErrorHandler>::CreateInstance(&ehp));
    CComPtr<CComObject<SAXErrorHandler> > eh(ehp);
    rdr->putErrorHandler(eh);

    // construct the document
    MSXML2::IXMLDOMDocument2Ptr	ndoc(CreateDOMImp(m_encoding));

    // reparse the document
    IStreamPtr	    isp(ndoc);
    HRESULT hr=rdr->raw_parse(_variant_t((IUnknown *)isp));
    if (FAILED(hr)) {
      if (!eh->m_msg.IsEmpty()) {
	// record error position
	if (errline)
	  *errline=eh->m_line;
	if (errcol)
	  *errcol=eh->m_col;
	if (fValidateOnly)
	  ::MessageBeep(MB_ICONERROR);
	else
	  if (::MessageBox(m_frame,eh->m_msg +
			    _T("\r\n") 
			    _T("Do you still want to save the file?\r\n\r\n")
			    _T("WARNING: Invalid files can be improperly displayed and can cause errors in other applications."),
			    _T("Validation failed"),
			    MB_YESNO|MB_DEFBUTTON2|MB_ICONERROR)==IDYES)
	    goto forcesave;
	::SendMessage(m_frame,AU::WM_SETSTATUSTEXT,0,
	  (LPARAM)(const TCHAR *)eh->m_msg);
      } else
	U::ReportError(hr);
      return false;
    }

    if (fValidateOnly) {
      ::SendMessage(m_frame,AU::WM_SETSTATUSTEXT,0,
	(LPARAM)_T("No errors found."));
      ::MessageBeep(MB_OK);
      return true;
    }

forcesave:
    // now save it
    // create tmp filename
    CString	path(filename);
    int		cp=path.ReverseFind(_T('\\'));
    if (cp<0)
      path=_T(".\\");
    else
      path.Delete(cp,path.GetLength()-cp);
    CString	buf;
    TCHAR	*bp=buf.GetBuffer(MAX_PATH);
    UINT    uv=::GetTempFileName(path,_T("fbe"),0,bp);
    if (uv==0)
      throw _com_error(HRESULT_FROM_WIN32(::GetLastError()));
    buf.ReleaseBuffer();
    // try to save file
    hr=ndoc->raw_save(_variant_t((const wchar_t *)buf));
    if (FAILED(hr)) {
      ::DeleteFile(buf);
      _com_issue_errorex(hr,ndoc,__uuidof(ndoc));
    }
    // rename tmp file to original filename
    ::DeleteFile(filename);
    ::MoveFile(buf,filename);
  }
  catch (_com_error& e) {
    U::ReportError(e);
    return false;
  }

  return true;
}

bool  Doc::Save() {
  if (!m_namevalid)
    return false;
  AU::CPersistentWaitCursor wc;
  if (SaveToFile(m_filename)) {
    MarkSavePoint();
    return true;
  }
  return false;
}

bool  Doc::Save(const CString& filename) {
  AU::CPersistentWaitCursor wc;
  if (SaveToFile(filename)) {
    MarkSavePoint();
    m_filename=filename;
    m_namevalid=true;
    return true;
  }
  return false;
}

// IDs
static const wchar_t  *AddHash(CString& tmp,const _bstr_t& id) {
  wchar_t  *cp=tmp.GetBuffer(id.length()+1);
  *cp++=L'#';
  memcpy(cp,(const wchar_t *)id,id.length()*sizeof(wchar_t));
  tmp.ReleaseBuffer(id.length()+1);
  return tmp;
}

static void GrabIDs(CString& tmp,CComboBox& box,MSHTML::IHTMLDOMNode *node) {
  if (node->nodeType!=1)
    return;

  _bstr_t		  name(node->nodeName);
  if (U::scmp(name,L"P") && U::scmp(name,L"DIV") && U::scmp(name,L"BODY"))
    return;

  MSHTML::IHTMLElementPtr elem(node);
  _bstr_t		  id(elem->id);
  if (id.length()>0)
    box.AddString(AddHash(tmp,id));

  MSHTML::IHTMLDOMNodePtr cn(node->firstChild);
  while ((bool)cn) {
    GrabIDs(tmp,box,cn);
    cn=cn->nextSibling;
  }
}

void  Doc::ParaIDsToComboBox(CComboBox& box) {
  try {
    CString tmp;
    MSHTML::IHTMLDOMNodePtr body(m_body.Document()->body);
    GrabIDs(tmp,box,body);
  }
  catch (_com_error&) { }
}

void  Doc::BinIDsToComboBox(CComboBox& box) {
  try {
    IDispatchPtr	bo(m_desc.Document()->all->item(L"id"));
    if (!(bool)bo)
      return;
    CString	  tmp;
    MSHTML::IHTMLElementCollectionPtr sbo(bo);
    if ((bool)sbo) {
      long    l=sbo->length;
      for (long i=0;i<l;++i)
	box.AddString(AddHash(tmp,MSHTML::IHTMLInputTextElementPtr(sbo->item(i))->value));
    } else {
      MSHTML::IHTMLInputTextElementPtr ebo(bo);
      if ((bool)ebo)
	box.AddString(AddHash(tmp,ebo->value));
    }
  }
  catch (_com_error&) { }
}

// binaries
void  Doc::AddBinary(const CString& filename) {
  _variant_t	  args[3];
  HRESULT	  hr;

  if (FAILED(hr=U::LoadFile(filename,&args[0]))) {
    U::ReportError(hr);
    return;
  }

  // prepare a default id
  int	cp=filename.ReverseFind(_T('\\'));
  if (cp<0)
    cp=0;
  else
    ++cp;
  CString   newid;
  TCHAR	    *ncp=newid.GetBuffer(filename.GetLength()-cp);
  int	    newlen=0;
  while (cp<filename.GetLength()) {
    TCHAR   c=filename[cp];
    if ((c>=_T('0') && c<=_T('9')) ||
	(c>=_T('A') && c<=_T('Z')) ||
	(c>=_T('a') && c<=_T('z')) ||
	c==_T('_') || c==_T('.'))
      ncp[newlen++]=c;
    ++cp;
  }
  newid.ReleaseBuffer(newlen);
  if (!newid.IsEmpty() && !(
    (newid[0]>=_T('A') && newid[0]<=_T('Z')) ||
    (newid[0]>=_T('a') && newid[0]<=_T('z')) ||
    newid[0]==_T('_')))
    newid.Insert(0,_T('_'));
  V_BSTR(&args[2])=newid.AllocSysString();
  V_VT(&args[2])=VT_BSTR;

  // try to find out mime type
  V_BSTR(&args[1])=U::GetMimeType(filename).AllocSysString();
  V_VT(&args[1])=VT_BSTR;

  // stuff the thing into javascript
  CComDispatchDriver	    desc(m_desc.Script());
  hr=desc.InvokeN(L"AddBinary",args,3);
  if (FAILED(hr))
    U::ReportError(hr);
}

void  Doc::ApplyConfChanges() {
  try {
    MSHTML::IHTMLStylePtr	  hs(m_body.Document()->body->style);

    CString	  fss(U::GetSettingS(_T("Font")));
    if (!fss.IsEmpty())
      hs->fontFamily=(const wchar_t *)fss;
  
    DWORD		  fs=U::GetSettingI(_T("FontSize"),1);
    if (fs>1) {
      fss.Format(_T("%dpt"),fs);
      hs->fontSize=(const wchar_t *)fss;
    }
  
    fs=U::GetSettingI(_T("ColorFG"),CLR_DEFAULT);
    if (fs==CLR_DEFAULT)
      fs=::GetSysColor(COLOR_WINDOWTEXT);
    fss.Format(_T("rgb(%d,%d,%d)"),GetRValue(fs),GetGValue(fs),GetBValue(fs));
    hs->color=(const wchar_t *)fss;

    fs=U::GetSettingI(_T("ColorBG"),CLR_DEFAULT);
    if (fs==CLR_DEFAULT)
      fs=::GetSysColor(COLOR_WINDOW);
    fss.Format(_T("rgb(%d,%d,%d)"),GetRValue(fs),GetGValue(fs),GetBValue(fs));
    hs->backgroundColor=(const wchar_t *)fss;
  }
  catch (_com_error&) { }
}

static int  compare_nocase(const void *v1,const void *v2) {
  const wchar_t	*s1=*(const wchar_t **)v1;
  const wchar_t *s2=*(const wchar_t **)v2;
  int	cv=_wcsicmp(s1,s2);
  if (cv!=0)
    return cv;
  return wcscmp(s1,s2);
}

static int  compare_counts(const void *v1,const void *v2) {
  const Doc::Word *w1=(const Doc::Word *)v1;
  const Doc::Word *w2=(const Doc::Word *)v2;
  int	diff=w1->count - w2->count;
  return diff ? diff : w1->word.CompareNoCase(w2->word);
}

void  Doc::GetWordList(int flags,CSimpleArray<Word>& words) {
  CWaitCursor	hourglass;

  _bstr_t   bb(m_body.Document()->body->innerText);

  if (bb.length()==0)
    return;

  // construct a word list
  CSimpleValArray<wchar_t*>  wl;

  // iterate over bb using a primitive fsm
  wchar_t   *p=bb,*e=p+bb.length()+1; // include trailing 0!
  wchar_t   *wstart,*wend;

  enum {
    INITIAL, INWORD1, INWORD2, HYPH1, HYPH2
  } state=INITIAL;

  while (p<e) {
    int  letter=iswalpha(*p);
    switch (state) {
    case INITIAL: initial:
      if (letter) {
	wstart=p;
	state=INWORD1;
      }
      break;
    case INWORD1:
      if (!letter) {
	if (flags & GW_INCLUDE_HYPHENS) {
	  if (iswspace(*p)) {
	    wend=p; state=HYPH1;
	    break;
	  } else if (*p == L'-') {
	    wend=p; state=HYPH2;
	    break;
	  }
	}
	if (!(flags & GW_HYPHENS_ONLY)) {
	  *p=L'\0';
	  wl.Add(wstart);
	}
	state=INITIAL;
      }
      break;
    case INWORD2:
      if (!letter) {
	*p=L'\0';
	U::RemoveSpaces(wstart);
	wl.Add(wstart);
	state=INITIAL;
      }
      break;
    case HYPH1:
      if (*p==L'-')
	state=HYPH2;
      else if (!iswspace(*p)) {
	if (!(flags & GW_HYPHENS_ONLY)) {
	  *wend=L'\0';
	  wl.Add(wstart);
	}
	state=INITIAL;
	goto initial;
      }
      break;
    case HYPH2:
      if (letter)
	state=INWORD2;
      else if (!iswspace(*p)) {
	if (!(flags & GW_HYPHENS_ONLY)) {
	  *wend=L'\0';
	  wl.Add(wstart);
	}
	state=INITIAL;
	goto initial;
      }
      break;
    }
    ++p;
  }

  if (wl.GetSize()==0)
    return;

  // now sort the list
  qsort(wl.GetData(),wl.GetSize(),sizeof(wchar_t*),compare_nocase);

  // and count the words
  int	i,nw,start;
  Word	w;

  for (start=0,i=1,nw=wl.GetSize();i<nw;++i)
    if (_wcsicmp(wl[start],wl[i])!=0) {
      w.count=i-start;
      w.word=wl[start];
      words.Add(w);
      start=i;
    }
  // append last word
  w.word=wl[start];
  w.count=i-start;
  words.Add(w);

  // sort by count now
  if (flags & GW_SORT_BY_COUNT)
    qsort(words.GetData(),words.GetSize(),sizeof(Word),compare_counts);
}

bool  Doc::SetXML(MSXML2::IXMLDOMDocument2 *dom) {
  if (!dom)
    return false;

  try {
    // ok, it seems valid, put it into document then
    dom->setProperty(L"SelectionLanguage",L"XPath");
    CString   nsprop(L"xmlns:fb='");
    nsprop+=(const wchar_t *)FBNS;
    nsprop+=L"' xmlns:xlink='";
    nsprop+=(const wchar_t *)XLINKNS;
    nsprop+=L"'";
    dom->setProperty(L"SelectionNamespaces",(const TCHAR *)nsprop);

    // transform to html
    TransformXML(LoadXSL(_T("description.xsl")),dom,m_desc);

    // wait until it loads
    MSG msg;

    while (!m_desc.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    m_desc.Init();

    // store binaries
    CComDispatchDriver	  desc(m_desc.Script());
    _variant_t	    arg(dom);
    desc.Invoke1(L"PutBinaries",&arg);

    // transform to html
    TransformXML(LoadXSL(_T("body.xsl")),dom,m_body);

    // wait until it loads
    while (!m_body.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    m_body.Init();
  }
  catch (_com_error& e) {
    U::ReportError(e);
    return false;
  }

  return true;
}

// source editing
bool  Doc::SetXMLAndValidate(HWND sci,bool fValidateOnly,int& errline,int& errcol) {
  errline=errcol=0;

  // validate it first
  try {
    // create a schema collection
    MSXML2::IXMLDOMSchemaCollection2Ptr	scol;
    CheckError(scol.CreateInstance(L"Msxml2.XMLSchemaCache.4.0"));

    // load fictionbook schema
    scol->add(FBNS,(const wchar_t *)U::GetProgDirFile(L"FictionBook.xsd"));

    // create a SAX reader
    MSXML2::ISAXXMLReaderPtr	  rdr;
    CheckError(rdr.CreateInstance(L"Msxml2.SAXXMLReader.4.0"));

    // attach a schema
    rdr->putFeature((USHORT*)L"schema-validation",VARIANT_TRUE);
    rdr->putProperty((USHORT*)L"schemas",scol.GetInterfacePtr());
    rdr->putFeature((USHORT*)L"exhaustive-errors",VARIANT_TRUE);

    // create an error handler
    CComObject<SAXErrorHandler>	  *ehp;
    CheckError(CComObject<SAXErrorHandler>::CreateInstance(&ehp));
    CComPtr<CComObject<SAXErrorHandler> > eh(ehp);
    rdr->putErrorHandler(eh);

    // construct a document
    MSXML2::IXMLDOMDocument2Ptr	dom;
    
    if (!fValidateOnly) {
      dom=U::CreateDocument(true);

      // construct an xml writer
      MSXML2::IMXWriterPtr	wrt;
      CheckError(wrt.CreateInstance(L"Msxml2.MXXMLWriter.4.0"));

      // connect document to the writer
      wrt->output=dom.GetInterfacePtr();

      // connect the writer to the reader
      rdr->putContentHandler(MSXML2::ISAXContentHandlerPtr(wrt));
    }

    // now parse it!
    // oh well, let's waste more memory
    int	    textlen=::SendMessage(sci, SCI_GETLENGTH, 0, 0);
    char    *buffer=(char *)malloc(textlen+1);
    if (!buffer) {
nomem:
      ::MessageBox(::GetActiveWindow(),_T("Out of memory"),_T("FBE"),
		  MB_OK|MB_ICONERROR);
     return false;
    }
    ::SendMessage(sci, SCI_GETTEXT, textlen+1, (LPARAM)buffer);
    DWORD   ulen=::MultiByteToWideChar(CP_UTF8,0,buffer,textlen,NULL,0);
    BSTR    ustr=::SysAllocStringLen(NULL,ulen);
    if (!ustr) {
      free(buffer);
      goto nomem;
    }
    ::MultiByteToWideChar(CP_UTF8,0,buffer,textlen,ustr,ulen);
    free(buffer);

    VARIANT vt;
    V_VT(&vt)=VT_BSTR;
    V_BSTR(&vt)=ustr;
    HRESULT hr=rdr->raw_parse(vt);
    ::VariantClear(&vt);

    if (FAILED(hr)) {
      if (!eh->m_msg.IsEmpty()) {
	// record error position
	errline=eh->m_line;
	errcol=eh->m_col;
	::MessageBeep(MB_ICONERROR);
	::SendMessage(m_frame,AU::WM_SETSTATUSTEXT,0,
	  (LPARAM)(const TCHAR *)eh->m_msg);
      } else
	U::ReportError(hr);
      return false;
    }

    if (fValidateOnly) {
      ::SendMessage(m_frame,AU::WM_SETSTATUSTEXT,0,
	(LPARAM)_T("No errors found."));
      ::MessageBeep(MB_OK);
      return true;
    }

    // ok, it seems valid, put it into document then
    dom->setProperty(L"SelectionLanguage",L"XPath");
    CString   nsprop(L"xmlns:fb='");
    nsprop+=(const wchar_t *)FBNS;
    nsprop+=L"' xmlns:xlink='";
    nsprop+=(const wchar_t *)XLINKNS;
    nsprop+=L"'";
    dom->setProperty(L"SelectionNamespaces",(const TCHAR *)nsprop);

    // transform to html
    TransformXML(LoadXSL(_T("description.xsl")),dom,m_desc);

    // wait until it loads
    MSG msg;

    while (!m_desc.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    // initialize view
    m_desc.Init();

    // store binaries
    CComDispatchDriver	  desc(m_desc.Script());
    _variant_t	    arg(dom.GetInterfacePtr());
    desc.Invoke1(L"PutBinaries",&arg);

    // transform to html
    TransformXML(LoadXSL(_T("body.xsl")),dom,m_body);

    // wait until it loads
    while (!m_body.Loaded() && ::GetMessage(&msg,NULL,0,0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }

    // initialize view
    m_body.Init();

    // mark unchanged
    MarkSavePoint();
  }
  catch (_com_error& e) {
    U::ReportError(e);
    return false;
  }

  return true;
}

} // namespace