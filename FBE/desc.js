var fbNS="http://www.gribuser.ru/xml/fictionbook/2.0";
var xlNS="http://www.w3.org/1999/xlink";

function Indent(node,len) {
  var s="\r\n";
  while (len--)
    s=s+" ";
  node.appendChild(node.ownerDocument.createTextNode(s));
}

function SetAttr(node,name,val) {
  var an=node.ownerDocument.createNode(2,name,fbNS);
  an.appendChild(node.ownerDocument.createTextNode(val));
  node.setAttributeNode(an);
}
function MakeText(node,name,val,force,indent) {
  var ret=true;
  if (val.length==0) {
    if (!force)
      return false;
    ret=false;
  }
  var tn=node.ownerDocument.createNode(1,name,fbNS);
  tn.appendChild(node.ownerDocument.createTextNode(val));
  Indent(node,indent);
  node.appendChild(tn);
  return ret;
}

function MakeAuthor(node,name,dv,force,indent) {
  var au=node.ownerDocument.createNode(1,name,fbNS);
  var added=false;
  if (dv.all.first.value.length>0 || dv.all.middle.value.length>0 ||
      dv.all.last.value.length>0 || dv.all.nick.value.length==0)
  {
    added=MakeText(au,"first-name",dv.all.first.value,true,indent+1) || added;
    added=MakeText(au,"middle-name",dv.all.middle.value,false,indent+1) || added;
    added=MakeText(au,"last-name",dv.all.last.value,true,indent+1) || added;
  }
  added=MakeText(au,"nickname",dv.all.nick.value,false,indent+1) || added;
  added=MakeText(au,"home-page",dv.all.home.value,false,indent+1) || added;
  added=MakeText(au,"email",dv.all.email.value,false,indent+1) || added;

  if (added || force) {
    Indent(node,indent);
    node.appendChild(au);
    Indent(au,indent);
  }

  return added;
}

function MakeDate(node,d,v,indent) {
  var dt=node.ownerDocument.createNode(1,"date",fbNS);
  var added=false;
  if (v.length>0) {
    added=true;
    SetAttr(dt,"value",v);
  }
  if (d.length>0)
    added=true;
  dt.appendChild(node.ownerDocument.createTextNode(d));
  Indent(node,indent);
  node.appendChild(dt);
  return added;
}

function MakeSeq2(xn,hn,indent) {
  var added=false;
  var newxn=xn.ownerDocument.createNode(1,"sequence",fbNS);
  var name=hn.all("name",0).value;
  var num=hn.all("number",0).value;
  SetAttr(newxn,"name",name);
  if (num.length > 0)
    SetAttr(newxn,"number",num);
  for (var cn=hn.firstChild;cn;cn=cn.nextSibling)
    if (cn.nodeName=="DIV")
      added=MakeSeq2(newxn,cn,indent+1) || added;
  if (added || name.length>0 || num.length>0) {
    Indent(xn,indent);
    xn.appendChild(newxn);
    if (newxn.hasChildNodes())
      Indent(newxn,indent);
    added=true;
  }
  return added;
}

function MakeSeq(xn,hn,indent) {
  var added=false;
  for (var cn=hn.firstChild;cn;cn=cn.nextSibling)
    if (cn.nodeName=="DIV")
      added=MakeSeq2(xn,cn,indent) || added;
  return added;
}

function IsEmpty(ii) {
  if (!ii || !ii.hasChildNodes())
    return true;
  for (var v=ii.firstChild;v;v=v.nextSibling)
    if (v.nodeType==1 && v.baseName!="empty-line")
      return false;
  return true;
}

function MakeTitleInfo(doc,desc,ann,indent) {
  var ti=doc.createNode(1,"title-info",fbNS);
  Indent(desc,indent);
  desc.appendChild(ti);

  // genres
  var list=document.all.tiGenre.getElementsByTagName("DIV");
  for (var i=0;i<list.length;++i) {
    var ge=doc.createNode(1,"genre",fbNS);
    var match=list.item(i).all.match.value;
    if (match.length>0 && match!="100")
      SetAttr(ge,"match",match);
    ge.appendChild(doc.createTextNode(list.item(i).all.genre.value));
    Indent(ti,indent+1);
    ti.appendChild(ge);
  }

  // authors
  var added=false;
  list=document.all.tiAuthor.getElementsByTagName("DIV");
  for (var i=0;i<list.length;++i)
    added=MakeAuthor(ti,"author",list.item(i),false,indent+1) || added;
  if (!added && list.length>0)
    MakeAuthor(ti,"author",list.item(0),true,indent+1);

  MakeText(ti,"book-title",document.all.tiTitle.value,true,indent+1);

  // annotation, will be filled by body.xsl
  if (!IsEmpty(ann)) {
    Indent(ti,indent+1);
    ti.appendChild(ann);
  }

  MakeText(ti,"keywords",document.all.tiKwd.value,false,indent+1);
  MakeDate(ti,document.all.tiDate.value,document.all.tiDateVal.value,indent+1);

  // coverpage images
  list=document.all.tiCover.getElementsByTagName("DIV");
  var cp=doc.createNode(1,"coverpage",fbNS);
  for (var i=0;i<list.length;++i)
    if (list.item(i).all.href.value.length>0) {
      var xn=doc.createNode(1,"image",fbNS);
      var an=doc.createNode(2,"l:href",xlNS);
      an.appendChild(doc.createTextNode(list.item(i).all.href.value));
      xn.setAttributeNode(an);
      Indent(cp,indent+2);
      cp.appendChild(xn);
    }
  if (cp.hasChildNodes) {
    Indent(ti,indent+1);
    ti.appendChild(cp);
  }

  MakeText(ti,"lang",document.all.tiLang.value,false,indent+1);
  MakeText(ti,"src-lang",document.all.tiSrcLang.value,false,indent+1);
  
  // translator
  list=document.all.tiTrans.getElementsByTagName("DIV");
  for (var i=0;i<list.length;++i)
    MakeAuthor(ti,"translator",list.item(i),false,indent+1);

  // sequence
  MakeSeq(ti,document.all.tiSeq,indent+1);
  Indent(ti,indent);
}

function MakeDocInfo(doc,desc,hist,indent) {
  var di=doc.createNode(1,"document-info",fbNS);
  var added=false;

  // authors
  var i;
  var list=document.all.diAuthor.getElementsByTagName("DIV");
  for (i=0;i<list.length;++i)
    added=MakeAuthor(di,"author",list.item(i),false,indent+1) || added;
  if (!added && list.length>0)
    MakeAuthor(di,"author",list.item(0),true,indent+1);

  added=MakeText(di,"program-used",document.all.diProgs.value,false,indent+1) || added;
  added=MakeDate(di,document.all.diDate.value,document.all.diDateVal.value,indent+1) || added;

  // src-url
  list=document.all.diURL.getElementsByTagName("INPUT");
  for (i=0;i<list.length;++i)
    added=MakeText(di,"src-url",list.item(i).value,false,indent+1) || added;

  added=MakeText(di,"src-ocr",document.all.diOCR.value,false,indent+1) || added;
  added=MakeText(di,"id",document.all.diID.value,true,indent+1) || added;
  added=MakeText(di,"version",document.all.diVersion.value,true,indent+1) || added;

  // history
  if (!IsEmpty(hist)) {
    Indent(di,indent+1);
    di.appendChild(hist);
    added=true;
  }
  
  // only append document info it is non-empty
  if (added) {
    Indent(desc,indent);
    desc.appendChild(di);
    Indent(di,indent);
  }
}

function MakePubInfo(doc,desc,indent) {
  var pi=doc.createNode(1,"publish-info",fbNS);
  var added=false;

  added=MakeText(pi,"book-name",document.all.piName.value,false,indent+1) || added;
  added=MakeText(pi,"publisher",document.all.piPub.value,false,indent+1) || added;
  added=MakeText(pi,"city",document.all.piCity.value,false,indent+1) || added;
  added=MakeText(pi,"year",document.all.piYear.value,false,indent+1) || added;
  added=MakeText(pi,"isbn",document.all.piISBN.value,false,indent+1) || added;

  // sequence
  added=MakeSeq(pi,document.all.piSeq,indent+1) || added;

  // only append publisher info it is non-empty
  if (added) {
    Indent(desc,indent);
    desc.appendChild(pi);
    Indent(pi,indent);
  }
}

function MakeCustInfo(doc,desc,indent) {
  var	list=document.all.ci.getElementsByTagName("DIV");
  for (var i=0;i<list.length;++i) {
    var t=list.item(i).all.type.value;
    var v=list.item(i).all.val.value;
    if (t.length>0 || v.length>0) {
      var ci=doc.createNode(1,"custom-info",fbNS);
      SetAttr(ci,"info-type",t);
      ci.appendChild(doc.createTextNode(v));
      Indent(desc,indent);
      desc.appendChild(ci);
    }
  }
}

function GetDesc(doc,ann,hist) {
  var desc=doc.createNode(1,"description",fbNS);
  Indent(doc.documentElement,1);
  doc.documentElement.appendChild(desc);
  MakeTitleInfo(doc,desc,ann,2);
  MakeDocInfo(doc,desc,hist,2);
  MakePubInfo(doc,desc,2);
  MakeCustInfo(doc,desc,2);
  Indent(desc,1);
}

function GetBinaries(doc) {
  var bo=document.all.binobj.getElementsByTagName("DIV");
  for (var i=0;i<bo.length;++i) {
    var newb=doc.createNode(1,"binary",fbNS);
    newb.dataType="bin.base64";
    newb.nodeTypedValue=bo.item(i).base64data;
    SetAttr(newb,"id",bo.item(i).all.id.value);
    SetAttr(newb,"content-type",bo.item(i).all.type.value);
    newb.dataType=undefined;
    Indent(doc.documentElement,1);
    doc.documentElement.appendChild(newb);
  }
}

// load a list of binary objects from document
function PutBinaries(doc) {
  var bl=doc.selectNodes("/fb:FictionBook/fb:binary");
  var nerr=0;
  for (var i=0;i<bl.length;++i) {
    bl.item(i).dataType="bin.base64";
    var id=bl.item(i).getAttribute("id");
    var dt;
    try {
      dt=bl.item(i).nodeTypedValue;
    }
    catch (e) {
      if (nerr++<3)
	alert("Invalid base64 data for "+id);
      continue;
    }
    AddBinary(id,bl.item(i).getAttribute("content-type"),dt);
  }
  if (nerr>3) {
    nerr-=3;
    alert(nerr+" more invalid images ignored");
  }
}

function AddBinary(id,type,data) {
  // check if this id already exists
  var	bo=document.all.binobj.getElementsByTagName("DIV");
  var	idx=0;
  var	curid=id;
  for (;;) {
    var found=false;
    for (var i=0;i<bo.length;++i)
      if (bo.item(i).all.id.value==curid) {
	found=true;
	break;
      }
    if (!found)
      break;
    curid=id+"_"+idx;
    ++idx;
  }
  var div=document.createElement("DIV");
  div.innerHTML='<button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button><label unselectable="on">ID:</label><input type="text" maxlength="256" id="id" class="b"><label unselectable="on">Type:</label><input type="text" maxlength="256" id="type" class="b" value="">';
  div.all.id.value=curid;
  div.all.type.value=type;
  div.base64data=data;
  document.all.binobj.appendChild(div);
  CheckCounts(document.all.binobj);
}

function GetBinary(id) {
  var bo=document.all.binobj.getElementsByTagName("DIV");
  for (var i=0;i<bo.length;++i)
    if (bo.item(i).all.id.value==id)
      return bo.item(i).base64data;
}
// editing helpers
function dClone(obj) {
  var qn=obj.cloneNode(true);
  var cn=qn.firstChild;
  while (cn) {
    var nn=cn.nextSibling;
    if (cn.nodeName=="DIV")
      cn.removeNode(true);
    cn=nn;
  }
  return qn;
}
function Remove(obj) {
  var pn=obj.parentNode;
  obj.removeNode(true);
  CheckCounts(pn);
}
function Clone(obj) {
  obj.insertAdjacentElement("afterEnd",dClone(obj));
  CheckCounts(obj.parentNode);
}
function ChildClone(obj) {
  obj.appendChild(dClone(obj));
  CheckCounts(obj);
}
function CheckCounts(obj) {
  var l=obj.getElementsByTagName("div");
  if (obj.id=="binobj") {
    if (l.length==0) {
      var sp=document.createElement("SPAN");
      sp.className="x";
      sp.id="spacer";
      obj.appendChild(sp);
    } else {
      var sp=obj.all.item("spacer");
      if (sp)
	sp.removeNode(true);
    }
    return;
  }
  if (l.length==0)
    return;
  var s=0;
  if (obj.nodeName!="DIV") {
    l.item(0).style.marginTop="0";
    l.item(0).style.paddingTop="0";
    l.item(0).style.borderTop="none";
    l.item(0).all.del.disabled=l.length <= 1;
    s=1;
  }
  for (var i=s;i<l.length;++i) {
    l.item(i).all.del.disabled=false;
    l.item(i).style.marginTop="0.2em";
    l.item(i).style.paddingTop="0.2em";
    l.item(i).style.borderTop="solid black 1px";
  }
}
function Init() {
  var fss=document.body.getElementsByTagName("fieldset");
  for (var n=0;n<fss.length;++n)
    CheckCounts(fss.item(n));
}

function GetGenre(d) {
  var v=window.external.GenrePopup(d,window.event.screenX,window.event.screenY);
  if (v.length>0)
    d.parentNode.all.genre.value=v;
}
