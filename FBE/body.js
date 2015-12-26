var imgcode="<DIV onresizestart='return false' contentEditable='false' class='image' href='' imgW='0' imgH='0'><IMG onload='ImgLoaded(this)' src='fbe-internal:{$dID}:'></DIV>";

function ImgLoaded(img) {
  var div=img.parentElement;
  img.imgW=img.clientWidth;
  img.imgH=img.clientHeight;
  img.style.setExpression("width","imgW <= parentElement.clientWidth ? (imgW+'px') : (parentElement.clientWidth+'px')");
  div.style.setExpression("height","firstChild.clientHeight+'px'");
}

function ImgSetURL(div,url) {
  div.href=url;
  div.firstChild.style.width=null;
  div.firstChild.src=document.urlprefix+url;
}

function GoTo(elem) {
  if (!elem)
    return;
  var r=document.selection.createRange();
  if (!r || !("compareEndPoints" in r))
    return;
  r.moveToElementText(elem);
  r.collapse(true);
  if (r.parentElement!==elem && r.move("character",1)==1)
    r.move("character",-1);

  r.select();
}

function SkipOver(np,n1,n2,n3) {
  while (np) {
    if (!(np.tagName=="P" && !np.firstChild && !window.external.inflateBlock(np)) &&  // not an empty P
	(!n1 || (np.tagName!=n1 && np.className!=n1)) && // and not n1
	(!n2 || (np.tagName!=n2 && np.className!=n2)) && // and not n2
	(!n3 || (np.tagName!=n3 && np.className!=n3)))   // and not n3
      break;
    np=np.nextSibling;
  }
  return np;
}

function StyleCheck(cp,st) {
  if (!cp || cp.tagName!="P")
    return false;
  var pp=cp.parentElement;
  if (!pp || pp.tagName!="DIV")
    return false;
  switch (st) {
    case "":
      if (pp.className!="section" && pp.className!="title" && pp.className!="epigraph" &&
	  pp.className!="stanza" && pp.className!="cite" && pp.className!="annotation" &&
	  pp.className!="history")
	return false;
      break;
    case "subtitle":
      if (pp.className!="section" && pp.className!="stanza")
	return false;
      break;
    case "text-author":
      if (pp.className!="cite" && pp.className!="epigraph" && pp.className!="poem")
	return false;
      if (cp.nextSibling)
	return false;
      break;
  }
  return true;
}
function SetStyle(cp,check,name) {
  if (!StyleCheck(cp,name))
    return;
  if (check)
    return true;
  if (name.length==0)
    name="normal";
  window.external.BeginUndoUnit(document,name+" style");
  cp.className=name;
  window.external.EndUndoUnit(document);
}
function StyleNormal(cp,check) {
  return SetStyle(cp,check,"");
}
function StyleTextAuthor(cp,check) {
  return SetStyle(cp,check,"text-author");
}
function StyleSubtitle(cp,check) {
  return SetStyle(cp,check,"subtitle");
}

function InflateIt(elem) {
  if (!elem || elem.nodeType!=1)
    return;
  if (elem.tagName=="P") {
    window.external.inflateBlock(elem)=true;
    return;
  }
  elem=elem.firstChild;
  while (elem) {
    InflateIt(elem);
    elem=elem.nextSibling;
  }
}

function AddTitle(cp,check) {
  if (!cp)
    return;
  if (cp.tagName=="P")
    cp=cp.parentElement;
  if (cp.tagName!="DIV")
    return;
  if (cp.className!="body" && cp.className!="section" &&
      cp.className!="stanza" && cp.className!="poem")
    return;
  var np=cp.firstChild;
  if (np) {
    if (cp.className=="body" && np.tagName=="DIV" && np.className=="image")
      np=np.nextSibling;
    if (np.tagName=="DIV" && np.className=="title")
      return;
  }
  if (check)
    return true;
  window.external.BeginUndoUnit(document,"add title");
  var div=document.createElement("DIV");
  div.className="title";
  var pp=document.createElement("P");
  window.external.inflateBlock(pp)=true;
  div.appendChild(pp);
  InsBefore(cp,np,div);
  window.external.EndUndoUnit(document);
  GoTo(div);
}

function AddBody(check) {
  if (check)
    return true;
  window.external.BeginUndoUnit(document,"add body");
  var newbody=document.createElement("DIV");
  newbody.className="body";
  newbody.innerHTML='<DIV class="title"><P></P></DIV><DIV class="section"><P></P></DIV>';
  document.body.appendChild(newbody);
  InflateIt(newbody);
  window.external.EndUndoUnit(document);
  GoTo(newbody);
}

function GetCP(cp) {
  if (!cp)
    return;
  if (cp.tagName=="P")
    cp=cp.parentElement;
  if (cp.tagName=="DIV" && cp.className=="title")
    cp=cp.parentElement;
  if (cp.tagName!="DIV")
    return;
  return cp;
}

function InsBefore(parent,ref,item) {
  if (ref)
    ref.insertAdjacentElement("beforeBegin",item);
  else
    parent.insertAdjacentElement("beforeEnd",item);
}

function InsBeforeHTML(parent,ref,ht) {
  if (ref)
    ref.insertAdjacentHTML("beforeBegin",ht);
  else
    parent.insertAdjacentHTML("beforeEnd",ht);
}

function CloneContainer(cp,check) {
  cp=GetCP(cp);
  if (!cp)
    return;

  switch (cp.className) {
    case "section": case "poem": case "stanza": case "cite": case "epigraph":
      break;
    default:
      return;
  }
  if (check)
    return true;
  window.external.BeginUndoUnit(document,"clone "+cp.className);
  var ncp=cp.cloneNode(false);
  switch (cp.className) {
    case "section":
      ncp.innerHTML='<DIV class="title"><P></P></DIV><P></P>';
      break;
    case "poem":
      ncp.innerHTML='<DIV class="title"><P></P></DIV><DIV class="stanza"><P></P></DIV>';
      break;
    case "stanza":
    case "cite":
    case "epigraph":
      ncp.innerHTML='<P></P>';
      break;
  }
  InflateIt(ncp);
  cp.insertAdjacentElement("afterEnd",ncp);
  window.external.EndUndoUnit(document);
  GoTo(ncp);
}

function InsImage(check) {
  var rng=document.selection.createRange();
  if (!rng || !("compareEndPoints" in rng))
    return;
  if (rng.compareEndPoints("StartToEnd",rng)!=0) {
    rng.collapse(true);
    if (rng.move("character",1)==1)
      rng.move("character",-1);
  }
  var	pe=rng.parentElement();
  while (pe) {
    if (pe.tagName=="DIV")
      break;
    pe=pe.parentElement;
  }
  if (!pe || pe.className!="section")
    return;
  if (check)
    return true;
  window.external.BeginUndoUnit(document,"insert image");
  rng.pasteHTML(imgcode);
  window.external.EndUndoUnit(document);
  return rng.parentElement;
}

function AddImage(cp,check) {
  cp=GetCP(cp);
  if (!cp)
    return;

  if (cp.className!="body" && cp.className!="section")
    return;

  var np=cp.firstChild;
  if (cp.className=="body")
    np=SkipOver(np,null,null,null);
  else
    np=SkipOver(np,"title","epigraph",null);
  if (np && np.tagName=="DIV" && np.className=="image")
    return;

  if (check)
    return true;

  window.external.BeginUndoUnit(document,"add image");
  InsBeforeHTML(cp,np,imgcode);
  window.external.EndUndoUnit(document);
}

function AddEpigraph(cp,check) {
  cp=GetCP(cp);
  if (!cp)
    return;

  if (cp.className!="body" && cp.className!="section" && cp.className!="poem")
    return;

  var	pp=cp.firstChild;
  if (cp.className=="body") // different order
    pp=SkipOver(pp,"title","image",null);
  else
    pp=SkipOver(pp,"title",null,null);

  if (check)
    return true;

  window.external.BeginUndoUnit(document,"add epigraph");
  var ep=document.createElement("DIV");
  ep.className="epigraph";
  ep.appendChild(document.createElement("P"));
  InsBefore(cp,pp,ep);
  InflateIt(ep);
  window.external.EndUndoUnit(document);
  GoTo(ep);
}

function AddAnnotation(cp,check) {
  cp=GetCP(cp);
  if (!cp)
    return;

  if (cp.className!="section")
    return;

  var	pp=SkipOver(cp.firstChild,"title","epigraph","image");

  if (pp && pp.tagName=="DIV" && pp.className=="annotation")
    return;

  if (check)
    return true;

  window.external.BeginUndoUnit(document,"add annotation");
  var ep=document.createElement("DIV");
  ep.className="annotation";
  ep.appendChild(document.createElement("P"));
  InsBefore(cp,pp,ep);
  InflateIt(ep);
  window.external.EndUndoUnit(document);
  GoTo(ep);
}

function AddTA(cp,check) {
  cp=GetCP(cp);

  while (cp) {
    if (cp.tagName=="DIV" && (cp.className=="poem" || cp.className=="epigraph" || cp.className=="cite"))
      break;
    cp=cp.parentElement;
  }

  if (!cp)
    return;

  var lc=cp.lastChild;
  if (lc && lc.tagName=="P" && lc.className=="text-author")
    return;

  if (check)
    return true;

  window.external.BeginUndoUnit(document,"add text author");
  var np=document.createElement("P");
  np.className="text-author";
  window.external.inflateBlock(np)=true;
  cp.appendChild(np);
  window.external.EndUndoUnit(document);
  GoTo(np);
}

function IsCtSection(s) {
  for (s=s.firstChild;s;s=s.nextSibling)
    if (s.nodeName=="P")
      return false;
  return true;
}

function FindSE(cp,name) {
  for (cp=cp.firstChild;cp;cp=cp.nextSibling) {
    if (cp.nodeName!="DIV")
      return;
    if (cp.className==name)
      return cp;
    if (cp.className=="section")
      return;
  }
}

function MergeContainers(cp,check) {
  cp=GetCP(cp);

  if (!cp)
    return;

  if (cp.className!="section" && cp.className!="stanza")
    return;

  var nx=cp.nextSibling;

  if (!nx || nx.tagName!="DIV" || nx.className!=cp.className)
    return;

  if (check)
    return true;

  window.external.BeginUndoUnit(document,"merge "+cp.className+"s");

  if (!IsCtSection(cp)) {
    // delete all nested sections
    var pi,ii=nx.firstChild;

    while (ii) {
      if (ii.tagName=="DIV") {
	if (ii.className=="title") {
	  var pp=ii.getElementsByTagName("P");
	  for (var l=0;l<pp.length;++l)
	    pp.item(l).className="subtitle";
	}
	if (ii.className=="title" || ii.className=="epigraph" || ii.className=="section" || ii.className=="annotation") {
	  ii.removeNode(false);
	  ii=pi ? pi.nextSibling : nx.firstChild;
	  continue;
	}
      }
      pi=ii;
      ii=ii.nextSibling;
    }
  } else if (!IsCtSection(nx)) {
    // simply move nx under cp
    cp.appendChild(nx);
  } else {
    // move nx's content under cp
    
    // check if there are any header items to save
    var needSave;
    for (var ii=nx.firstChild;ii;ii=ii.nextSibling)
      if (ii.nodeName=="DIV" && (ii.className=="image" || ii.className=="epigraph" ||
				 ii.className=="annotation" || ii.className=="title"))
      {
	needSave=true;
	break;
      }

    if (needSave) {
      // find a destination section for section header items
      var dst=nx.firstChild;
      while (dst) {
	if (dst.nodeName=="DIV" && dst.className=="section")
	  break;
	dst=dst.nextSibling;
      }

      // create one
      if (!dst) {
	dst=document.createElement("DIV");
	dst.className="section";
	var pp=document.createElement("P");
	window.external.inflateBlock(pp)=true;
	dst.appendChild(pp);
	cp.appendChild(dst);
      }

      // move items
      var jj=dst.firstChild;
      for (;;) {
	var ii=nx.firstChild;
	if (!ii)
	  break;
	if (ii.nodeName!="DIV")
	  break;
	var stop;
	switch (ii.className) {
	  case "title":
	    if (jj && jj.nodeName=="DIV" && jj.className=="title") {
	      jj.insertAdjacentElement("afterBegin",ii);
	      ii.removeNode(false);
	    } else
	      InsBefore(dst,jj,ii);
	    break;
	  case "epigraph":
	    while (jj && jj.nodeName=="DIV" && (jj.className=="title" || jj.className=="epigraph"))
	      jj=jj.nextSibling;
	    InsBefore(dst,jj,ii);
	    break;
	  case "image":
	    while (jj && jj.nodeName=="DIV" && (jj.className=="title" || jj.className=="epigraph"))
	      jj=jj.nextSibling;
	    InsBefore(dst,jj,ii);
	    break;
	  case "annotation":
	    while (jj && jj.nodeName=="DIV" && (jj.className=="title" || jj.className=="epigraph" || jj.className=="image"))
	      jj=jj.nextSibling;
	    if (jj && jj.nodeName=="DIV" && jj.className=="annotation") {
	      jj.insertAdjacentElement("afterBegin",ii);
	      ii.removeNode(false);
	    } else
	      InsBefore(dst,jj,ii);
	    break;
	  default:
	    stop=true;
	    break;
	}
	if (stop)
	  break;
      }
    }

    // finally merge
    cp.appendChild(nx);
    nx.removeNode(false);
  }

  cp.insertAdjacentElement("beforeEnd",nx);
  nx.removeNode(false);

  window.external.EndUndoUnit(document);
}

function RemoveOuterContainer(cp,check) {
  cp=GetCP(cp);

  if (!cp)
    return;

  if ((cp.className!="section" && cp.className!="body") || !IsCtSection(cp))
    return;

  if (check)
    return true;

  window.external.BeginUndoUnit(document,"Remove Outer Section");

  // ok, move all child sections to upper level
  while (cp.firstChild) {
    var cc=cp.firstChild;
    cc.removeNode(true);
    if (cc.nodeName=="DIV" && cc.className=="section")
      cp.insertAdjacentElement("beforeBegin",cc);
  }

  // delete the section itself
  cp.removeNode(true);

  window.external.EndUndoUnit(document);
}
