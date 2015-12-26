#include "stdafx.h"
#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include "FBEView.h"
#include "FBDoc.h"
#include "TreeView.h"

// redrawing the tree is _very_ ugly visually, so we first build a copy and compare them
struct TreeNode {
  TreeNode    *parent,*next,*last,*child;
  CString     text;
  int	      img;
  MSHTML::IHTMLElement	*pos;
  TreeNode() : parent(0), next(0), child(0), last(0), img(0), pos(0) { }
  TreeNode(TreeNode *nn,const CString& s,int ii,MSHTML::IHTMLElement *pp) : parent(nn), next(0), last(0), child(0),
    text(s), img(ii), pos(pp)
  {
    if (pos)
      pos->AddRef();
  }
  ~TreeNode() {
    if (pos)
      pos->Release();
    TreeNode *q;
    for (TreeNode *n=child;n;n=q) {
      q=n->next;
      delete n;
    }
  }
  TreeNode *Append(const CString& s,int ii,MSHTML::IHTMLElement *p) {
    TreeNode *n=new TreeNode(this,s,ii,p);
    if (last) {
      last->next=n;
      last=n;
    } else
      last=child=n;
    return n;
  }
};

static bool  SearchUnder(CTreeItem& ret,CTreeItem ii,MSHTML::IHTMLElement *p) {
  CTreeItem   jj(ii.GetChild());
  while (!jj.IsNull()) {
    MSHTML::IHTMLElement    *n=(MSHTML::IHTMLElement *)jj.GetData();
    if (n && n->contains(p)) {
      ret=jj;
      if (jj.HasChildren())
	SearchUnder(ret,jj,p);
      return true;
    }
    jj=jj.GetNextSibling();
  }
  return false;
}

CTreeItem CTreeView::LocatePosition(MSHTML::IHTMLElement *p) {
  CTreeItem   ret(TVI_ROOT,this);
  if (GetCount()==0 || !p)
    return ret; // no items at all

  SearchUnder(ret,ret,p);

  return ret;
}

void  CTreeView::HighlightItemAtPos(MSHTML::IHTMLElement *p) {
  CTreeItem ii(LocatePosition(p));
  if (ii==m_last_lookup_item)
    return;
  m_last_lookup_item=ii;
  if (ii!=TVI_ROOT) {
    SelectItem(ii);
    EnsureVisible(ii);
  } else
    SelectItem(0);
}

static MSHTML::IHTMLElementPtr	FindTitleNode(MSHTML::IHTMLDOMNodePtr elem) {
  MSHTML::IHTMLDOMNodePtr node(elem->firstChild);

  if ((bool)node && node->nodeType==1 && U::scmp(node->nodeName,L"DIV")==0) {
    _bstr_t   cls(MSHTML::IHTMLElementPtr(node)->className);
    if (U::scmp(cls,L"image")==0) {
      node=node->nextSibling;
      if (node->nodeType!=1 || U::scmp(node->nodeName,L"DIV"))
	return NULL;
      cls=MSHTML::IHTMLElementPtr(node)->className;
    }
    if (U::scmp(cls,L"title")==0)
      return MSHTML::IHTMLElementPtr(node);
  }
  return NULL;
}

static CString	FindTitle(MSHTML::IHTMLDOMNodePtr elem) {
  MSHTML::IHTMLElementPtr tn(FindTitleNode(elem));
  if (tn)
    return (const wchar_t *)tn->innerText;
  return CString();
}

static void MakeNode(TreeNode *parent,MSHTML::IHTMLDOMNodePtr elem) {
  if (elem->nodeType!=1)
    return;
  MSHTML::IHTMLElementPtr   he(elem);
  _bstr_t		    nn(he->tagName);
  _bstr_t		    cn(he->className);
  if (U::scmp(nn,L"DIV")==0) {
    // at this point we are interested only in sections/subtitles/poems/stanzas
    int img=0;
    CString txt;
    if (U::scmp(cn,L"poem")==0 || U::scmp(cn,L"stanza")==0) {
      txt=FindTitle(elem);
      img=1;
    } else if (U::scmp(cn,L"body")==0) {
      txt=AU::GetAttrCS(he,L"fbname");
      U::NormalizeInplace(txt);
      if (txt.IsEmpty())
	txt=FindTitle(elem);
      img=0;
    } else if (U::scmp(cn,L"section")==0) {
      txt=FindTitle(elem);
      img=0;
    } else if (U::scmp(cn,L"epigraph")==0 || U::scmp(cn,L"annotation")==0 ||
	       U::scmp(cn,L"history")==0 || U::scmp(cn,L"cite")==0)
    {
      img=0;
    } else
      return;
    U::NormalizeInplace(txt);
    if (txt.IsEmpty())
      txt.Format(_T("<%s>"),(const TCHAR *)cn);
    parent=parent->Append(txt,img,he);
  } else if (U::scmp(nn,L"P")==0 && U::scmp(cn,L"subtitle")==0) {
    CString txt((const TCHAR *)he->innerText);
    U::NormalizeInplace(txt);
    if (txt.IsEmpty())
      txt=_T("<subtitle>");
    parent=parent->Append(txt,2,he);
  }
  elem=elem->firstChild;
  while (elem) {
    MakeNode(parent,elem);
    elem=elem->nextSibling;
  }
  return;
}

static TreeNode  *GetDocTree(CFBEView& view) {
  TreeNode	*root=new TreeNode();
  try {
    MakeNode(root,view.Document()->body);
  }
  catch (_com_error&) {
  }
  if (!root->child) {
    delete root;
    return NULL;
  }
  return root;
}

static void  CompareTreesAndSet(TreeNode *n,CTreeItem ii,bool& fDisableRedraw) {
  bool	fH1=ii==TVI_ROOT ? ii.m_pTreeView->GetCount()>0 : ii.HasChildren()!=0;
  // walk them one by one and check
  CTreeItem   nc=fH1 ? ii.GetChild() : CTreeItem(NULL,ii.m_pTreeView);
  TreeNode    *ic=n->child;
  CString     text;
  while (ic && nc) {
    int	  img1,img2;
    nc.GetImage(img1,img2);
    nc.GetText(text);
    if (text!=ic->text || img1!=ic->img) { // differ
      // copy the item here
      if (!fDisableRedraw) {
	ii.m_pTreeView->SetRedraw(FALSE);
	fDisableRedraw=true;
      }
      nc.SetImage(ic->img,ic->img);
      nc.SetText(ic->text);
    }
    MSHTML::IHTMLElement    *od=(MSHTML::IHTMLElement *)nc.GetData();
    if (od)
      od->Release();
    if (ic->pos)
      ic->pos->AddRef();
    nc.SetData((LPARAM)ic->pos);
    CompareTreesAndSet(ic,nc,fDisableRedraw);
    ic=ic->next;
    nc=nc.GetNextSibling();
  }
  CTreeItem next;
  if ((nc || ic) && !fDisableRedraw) {
    ii.m_pTreeView->SetRedraw(FALSE);
    fDisableRedraw=true;
  }
  while (nc) { // remove extra children, staring with ic
    next=nc.GetNextSibling();
    nc.Delete();
    nc=next;
  }
  while (ic) { // append children to ii
    nc=ii.AddTail(ic->text,ic->img);
    if (ic->pos)
      ic->pos->AddRef();
    nc.SetData((LPARAM)ic->pos);
    if (ic->child)
      CompareTreesAndSet(ic,nc,fDisableRedraw);
    ic=ic->next;
  }
}

void  CTreeView::GetDocumentStructure(CFBEView& view) {
  m_last_lookup_item=0;

  TreeNode  *root=GetDocTree(view);
  if (!root) {
    SetRedraw(FALSE);
    DeleteAllItems();
    SetRedraw(TRUE);
    return;
  }
  bool	fDisableRedraw=false;
  CompareTreesAndSet(root,CTreeItem(TVI_ROOT,this),fDisableRedraw);
  if (fDisableRedraw)
    SetRedraw(TRUE);
  delete root;
}

void  CTreeView::UpdateDocumentStructure(CFBEView& v,MSHTML::IHTMLDOMNodePtr node) {
  MSHTML::IHTMLElementPtr     ce(node);

  CTreeItem   ii(LocatePosition(ce));

  if (ii==TVI_ROOT) { // huh?
    GetDocumentStructure(v);
    return;
  }

  MSHTML::IHTMLElementPtr   jj((MSHTML::IHTMLElement *)ii.GetData());

  // shortcut for the most common situation
  // all changes confined to a P, which is not in the title
  if (U::scmp(jj->tagName,L"DIV")==0 && U::scmp(node->nodeName,L"P")==0) {
    MSHTML::IHTMLElementPtr   tn(FindTitleNode(jj));
    if (!(bool)tn || (tn!=ce && tn->contains(ce)!=VARIANT_TRUE))
      return;
  }

  TreeNode		    *nn=new TreeNode;

  MakeNode(nn,jj);

  bool	fDisableRedraw=false;
  // check the item itself
  CString text;
  int	  img1,img2;
  ii.GetImage(img1,img2);
  ii.GetText(text);
  if (text!=nn->child->text || img1!=nn->child->img) { // differ
    // copy the item here
    SetRedraw(FALSE);
    fDisableRedraw=true;
    ii.SetImage(nn->child->img,nn->child->img);
    ii.SetText(nn->child->text);
  }

  CompareTreesAndSet(nn->child,ii,fDisableRedraw);
  if (fDisableRedraw)
    SetRedraw(TRUE);

  delete nn;
}

BOOL CTreeView::PreTranslateMessage(MSG* pMsg)
{
  pMsg;
  return FALSE;
}

LRESULT CTreeView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  // "CTreeViewCtrl::OnCreate()"
  LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
  
  // "OnInitialUpdate"
  m_ImageList.CreateFromImage(IDB_STRUCTURE,16,32,RGB(255,0,255),IMAGE_BITMAP);
  SetImageList(m_ImageList,TVSIL_NORMAL);

  SetScrollTime(1);

  bHandled = TRUE;
  
  return lRet;
}

LRESULT CTreeView::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetImageList(NULL,TVSIL_NORMAL);
  m_ImageList.Destroy();
  
  // Say that we didn't handle it so that the treeview and anyone else
  //  interested gets to handle the message
  bHandled = FALSE;
  return 0;
}

LRESULT CTreeView::OnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  // check if we are going to hit an item
  UINT	  flags=0;
  CTreeItem ii(HitTest(CPoint(LOWORD(lParam),HIWORD(lParam)),&flags));
  if (flags&TVHT_ONITEM && !ii.IsNull()) { // try to select and expand it
    CTreeItem	sel(GetSelectedItem());
    if (sel!=ii)
      ii.Select();
    if (!(ii.GetState(TVIS_EXPANDED)&TVIS_EXPANDED))
      ii.Expand();
    SetFocus();
  } else
    bHandled=FALSE;
  return 0;
}

LRESULT CTreeView::OnDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  // check if we double-clicked an already selected item item
  UINT	  flags=0;
  CTreeItem ii(HitTest(CPoint(LOWORD(lParam),HIWORD(lParam)),&flags));
  if (flags&TVHT_ONITEM && !ii.IsNull() && ii==GetSelectedItem())
    ::SendMessage(GetParent(),WM_COMMAND,MAKELONG(0,IDN_TREE_CLICK),(LPARAM)m_hWnd);
  return 0;
}

static void RecursiveExpand(CTreeItem n,bool *fEnable) {
  for (CTreeItem   ch(n.GetChild());!ch.IsNull();ch=ch.GetNextSibling()) {
    if (!ch.HasChildren())
      continue;
    if (!(ch.GetState(TVIS_EXPANDED)&TVIS_EXPANDED)) {
      if (!*fEnable) {
	*fEnable=true;
	ch.GetTreeView()->SetRedraw(FALSE);
      }
      ch.Expand();
    }
    RecursiveExpand(ch,fEnable);
  }
}

LRESULT CTreeView::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  switch (wParam) {
  case VK_RETURN: // swallow
    break;
  case '*': // expand the entire tree
    if (GetCount()>0) {
      bool  fEnable=false;
      RecursiveExpand(CTreeItem(TVI_ROOT,this),&fEnable);
      if (fEnable)
	SetRedraw(TRUE);
    }
    break;
  default: // pass to control
    bHandled=FALSE;
  }
  return 0;
}

LRESULT CTreeView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (wParam==VK_RETURN)
    ::PostMessage(GetParent(),WM_COMMAND,MAKELONG(0,IDN_TREE_RETURN),(LPARAM)m_hWnd);
  else
    bHandled=FALSE;
  return 0;
}
