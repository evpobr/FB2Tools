function FBEScriptName() {
  return "FBE Demo Script";
}

function Run(dom) {
  Message("This is a demo script.\r\nIt inserts some junk text into your document.");
  dom.selectSingleNode("//fb:p").appendChild(dom.createTextNode(" blablabla"));
}