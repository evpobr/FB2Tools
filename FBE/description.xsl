<?xml version="1.0" encoding="utf-8"?>

<!-- this stylesheet handles conversion of <description> fictionbook element
  to editable html. -->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:f="http://www.gribuser.ru/xml/fictionbook/2.0"
  xmlns:l="http://www.w3.org/1999/xlink">

  <xsl:output
    method="html"
    indent="no"
  />

  <xsl:param name="dlgFG"/>
  <xsl:param name="dlgBG"/>
  <xsl:param name="descscript"/>

  <xsl:template match="/">
    <html>
      <head>
	<meta http-equiv="MSThemeCompatible" content="yes"/>
	<title>Description block</title>
	<style type="text/css">
	  body {
	    font-family: "Tahoma", sans-serif;
	    font-size: 8pt;
	    background-color: <xsl:value-of select="$dlgBG"/>;
	  }
	  
	  legend.s { font-size: 12pt; color: <xsl:value-of select="$dlgFG"/>; }

	  input { font-family: "Tahoma", sans-serif; font-size: 8pt; }
	  input.u { width: 50em; }
	  input.c { width: 44em; }
	  input.f { width: 30em; }
	  input.b { width: 20em; }
	  input.d { width: 15em; }
	  input.g { width: 15em; }
	  input.m { width: 3em; }
	  textarea { width: 44em; font-family: "Tahoma", sans-serif; font-size: 8pt; }

	  label { padding-left: 0.5em; vertical-align: 35%; color: <xsl:value-of select="$dlgFG"/>; }

	  span.top { vertical-align: top; }

	  fieldset.s label { width: 9em; }
	  fieldset.a label { width: 6em; }
	  fieldset.b label { width: 5em; }

	  button { font-family: "Webdings"; font-size: 8pt; }
	  button.r { float: right; }

	  fieldset { margin: 0.2em; padding: 0.2em 0.3em 0.2em 0.2em; }
	  fieldset.s { width: 59.5em; }
	  fieldset.a { width: 100%; }
	  fieldset.b { width: 59.5em; }

	  div { white-space: nowrap; vertical-align: middle; clear: both; }
	  div.s { margin-left: 2em; }

	  span.x { width: 1px; height: 1px; }
	</style>
	<script language="JavaScript" src="{$descscript}"/>
      </head>
      <body onLoad="Init()">
	<!-- title-info -->
	<xsl:call-template name="ti">
	  <xsl:with-param name="cur" select="/f:FictionBook/f:description/f:title-info"/>
	</xsl:call-template>
	<!-- document-info -->
	<xsl:call-template name="di">
	  <xsl:with-param name="cur" select="/f:FictionBook/f:description/f:document-info"/>
	</xsl:call-template>
	<!-- publish-info -->
	<xsl:call-template name="pi">
	  <xsl:with-param name="cur" select="/f:FictionBook/f:description/f:publish-info"/>
	</xsl:call-template>
	<!-- custom-info -->
	<xsl:call-template name="custom-info">
	  <xsl:with-param name="items" select="/f:FictionBook/f:description/f:custom-info"/>
	</xsl:call-template>
	<!-- binary objects -->
	<fieldset class="b" id="binobj" unselectable="on">
	  <legend class="s" unselectable="on">Binary objects</legend>
	</fieldset>
      </body>
    </html>
  </xsl:template>

  <!-- title-info -->
  <xsl:template name="ti">
    <xsl:param name="cur"/>

    <fieldset class="s" unselectable="on">
      <legend class="s" unselectable="on">Title Info</legend>
      <fieldset class="a" id="tiGenre" unselectable="on">
	<legend unselectable="on">Genres</legend>
	<xsl:for-each select="$cur/f:genre">
	  <div unselectable="on">
	    <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	    <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	    <label unselectable="on">Genre:</label><input type="text" maxlength="25" id="genre" class="g" value="{.}"/>
	    <button onclick="GetGenre(this)" unselectable="on">&#x34;</button>
	    <label unselectable="on">Match:</label><input type="text" maxlength="3" id="match" class="m" value="{@match}"/>
	  </div>
	</xsl:for-each>
	<xsl:if test="not($cur/f:genre)">
	  <div unselectable="on">
	    <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	    <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	    <label unselectable="on">Genre:</label><input type="text" maxlength="25" id="genre" class="g" value=""/>
	    <button onclick="GetGenre(this)" unselectable="on">&#x34;</button>
	    <label unselectable="on">Match:</label><input type="text" maxlength="3" id="match" class="m" value=""/>
	  </div>
	</xsl:if>
      </fieldset>
      <fieldset class="a" id="tiAuthor" unselectable="on">
	<legend unselectable="on">Authors</legend>
	<xsl:call-template name="authors">
	  <xsl:with-param name="items" select="$cur/f:author"/>
	</xsl:call-template>
      </fieldset>
      <label unselectable="on">Book title:</label>
      <input type="text" maxlength="256" id="tiTitle" class="f" value="{$cur/f:book-title}"/><br/>
      <label unselectable="on">Keywords:</label>
      <input type="text" maxlength="256" id="tiKwd" class="f" value="{$cur/f:keywords}"/><br/>
      <label unselectable="on">Date:</label>
      <input type="text" maxlength="256" id="tiDate" class="d" value="{$cur/f:date}"/>
      <label unselectable="on">Value:</label>
      <input type="text" maxlength="256" id="tiDateVal" class="d" value="{$cur/f:date/@value}"/><br/>
      <fieldset class="a" id="tiCover" unselectable="on">
	<legend unselectable="on">Coverpage</legend>
	<xsl:for-each select="$cur/f:coverpage/f:image">
	  <div unselectable="on">
	    <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	    <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	    <label unselectable="on">Image:</label>
	    <input type="text" maxlength="256" id="href" class="f" value="{@l:href}"/>
	  </div>
	</xsl:for-each>
	<xsl:if test="not($cur/f:coverpage/f:image)">
	  <div unselectable="on">
	    <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	    <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	    <label unselectable="on">Image:</label>
	    <input type="text" maxlength="256" id="href" class="f" value=""/>
	  </div>
	</xsl:if>
      </fieldset>
      <label unselectable="on">Language:</label>
      <input type="text" maxlength="256" id="tiLang" class="f" value="{$cur/f:lang}"/><br/>
      <label unselectable="on">Source language:</label>
      <input type="text" maxlength="256" id="tiSrcLang" class="f" value="{$cur/f:src-lang}"/><br/>
      <fieldset class="a" id="tiTrans" unselectable="on">
	<legend unselectable="on">Translators</legend>
	<xsl:call-template name="authors">
	  <xsl:with-param name="items" select="$cur/f:translator"/>
	</xsl:call-template>
      </fieldset>
      <fieldset class="a" id="tiSeq" unselectable="on">
	<legend unselectable="on">Sequence</legend>
	<xsl:for-each select="$cur/f:sequence">
	  <xsl:call-template name="seq">
	    <xsl:with-param name="item" select="."/>
	  </xsl:call-template>
	</xsl:for-each>
	<xsl:if test="not($cur/f:sequence)">
	  <xsl:call-template name="seq">
	    <xsl:with-param name="item" select="."/>
	  </xsl:call-template>
	</xsl:if>
      </fieldset>
    </fieldset>
  </xsl:template>

  <!-- document-info -->
  <xsl:template name="di">
    <xsl:param name="cur"/>

    <fieldset class="s" unselectable="on">
      <legend class="s" unselectable="on">Document Info</legend>
      <fieldset class="a" id="diAuthor" unselectable="on">
	<legend unselectable="on">Authors</legend>
	<xsl:call-template name="authors">
	  <xsl:with-param name="items" select="$cur/f:author"/>
	</xsl:call-template>
      </fieldset>
      <label unselectable="on">Programs used:</label>
      <input type="text" maxlength="256" id="diProgs" class="f" value="{$cur/f:program-used}"/><br/>
      <label unselectable="on">Date:</label>
      <input type="text" maxlength="256" id="diDate" class="d" value="{$cur/f:date}"/>
      <label unselectable="on">Value:</label>
      <input type="text" maxlength="256" id="diDateVal" class="d" value="{$cur/f:date/@value}"/><br/>
      <fieldset class="u" id="diURL" unselectable="on">
	<legend unselectable="on">Source URLs</legend>
	<xsl:for-each select="$cur/f:src-url">
	  <div unselectable="on">
	    <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	    <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	    <input type="text" maxlength="256" class="u" value="{.}"/>
	  </div>
	</xsl:for-each>
	<xsl:if test="not($cur/f:src-url)">
	  <div unselectable="on">
	    <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	    <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	    <input type="text" maxlength="256" class="u" value=""/>
	  </div>
	</xsl:if>
      </fieldset>
      <label unselectable="on">Source OCR:</label>
      <input type="text" maxlength="256" id="diOCR" class="f" value="{$cur/f:src-ocr}"/><br/>
      <label unselectable="on">ID:</label>
      <input type="text" maxlength="256" id="diID" class="f" value="{$cur/f:id}"/><br/>
      <label unselectable="on">Version:</label>
      <input type="text" maxlength="256" id="diVersion" class="f" value="{$cur/f:version}"/><br/>
    </fieldset>
  </xsl:template>

  <!-- publish-info -->
  <xsl:template name="pi">
    <xsl:param name="cur"/>

    <fieldset class="s" unselectable="on">
      <legend class="s" unselectable="on">Publisher Info</legend>
      <label unselectable="on">Book name:</label>
      <input type="text" maxlength="256" id="piName" class="f" value="{$cur/f:book-name}"/><br/>
      <label unselectable="on">Publisher:</label>
      <input type="text" maxlength="256" id="piPub" class="f" value="{$cur/f:publisher}"/><br/>
      <label unselectable="on">City:</label>
      <input type="text" maxlength="256" id="piCity" class="f" value="{$cur/f:city}"/><br/>
      <label unselectable="on">Year:</label>
      <input type="text" maxlength="256" id="piYear" class="f" value="{$cur/f:year}"/><br/>
      <label unselectable="on">ISBN:</label>
      <input type="text" maxlength="256" id="piISBN" class="f" value="{$cur/f:isbn}"/><br/>
      <fieldset class="a" id="piSeq" unselectable="on">
	<legend unselectable="on">Sequence</legend>
	<xsl:for-each select="$cur/f:sequence">
	  <xsl:call-template name="seq">
	    <xsl:with-param name="item" select="."/>
	  </xsl:call-template>
	</xsl:for-each>
	<xsl:if test="not($cur/f:sequence)">
	  <xsl:call-template name="seq">
	    <xsl:with-param name="item" select="."/>
	  </xsl:call-template>
	</xsl:if>
      </fieldset>
    </fieldset>
  </xsl:template>

  <!-- custom-info -->
  <xsl:template name="custom-info">
    <xsl:param name="items"/>

    <fieldset class="s" id="ci" unselectable="on">
      <legend class="s" unselectable="on">Custom Info</legend>
      <xsl:for-each select="$items">
	<div unselectable="on">
	  <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	  <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	  <label unselectable="on">Type:</label>
	  <input type="text" maxlength="256" id="type" class="c" value="{@info-type}"/><br/>
	  <span class="top" unselectable="on"><label unselectable="on">Value:</label></span>
	  <textarea rows="8" id="val"><xsl:value-of select="."/></textarea>
	</div>
      </xsl:for-each>
      <xsl:if test="not($items)">
	<div unselectable="on">
	  <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
	  <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
	  <label unselectable="on">Type:</label>
	  <input type="text" maxlength="256" id="type" class="c" value=""/><br/>
	  <span class="top" unselectable="on"><label unselectable="on">Value:</label></span>
	  <textarea rows="8" id="val"/>
	</div>
      </xsl:if>
    </fieldset>
  </xsl:template>

  <!-- authors -->
  <xsl:template name="authors">
    <xsl:param name="items"/>

    <xsl:for-each select="$items">
      <xsl:call-template name="author">
	<xsl:with-param name="item" select="."/>
      </xsl:call-template>
    </xsl:for-each>
    <xsl:if test="not($items)">
      <xsl:call-template name="author">
	<xsl:with-param name="item" select="."/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>
  
  <!-- author -->
  <xsl:template name="author">
    <xsl:param name="item"/>

    <div unselectable="on">
      <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
      <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
      <label unselectable="on">First:</label>
      <input type="text" maxlength="256" id="first" class="aname" value="{$item/f:first-name}"/>
      <label unselectable="on">Middle:</label>
      <input type="text" maxlength="256" id="middle" class="aname" value="{$item/f:middle-name}"/>
      <label unselectable="on">Last:</label>
      <input type="text" maxlength="256" id="last" class="aname" value="{$item/f:last-name}"/>
      <br/>
      <label unselectable="on">Nickname:</label>
      <input type="text" maxlength="256" id="nick" class="aname" value="{$item/f:nickname}"/>
      <label unselectable="on">Email:</label>
      <input type="text" maxlength="256" id="email" class="aname" value="{$item/f:email}"/>
      <label unselectable="on">Homepage:</label>
      <input type="text" maxlength="256" id="home" class="aname" value="{$item/f:home-page}"/>
    </div>
  </xsl:template>

  <!-- sequence -->
  <xsl:template name="seq">
    <xsl:param name="item"/>

    <div class="s" unselectable="on">
      <button class="r" id="del" onclick="Remove(this.parentNode)" unselectable="on">&#x72;</button>
      <button class="r" onclick="Clone(this.parentNode)" unselectable="on">&#x32;</button>
      <button class="r" onclick="ChildClone(this.parentNode)" unselectable="on">&#x34;</button>
      <label unselectable="on">Name:</label>
      <input type="text" maxlength="256" id="name" class="f" value="{$item/@name}"/>
      <label unselectable="on">Number:</label>
      <input type="text" maxlength="256" id="number" class="m" value="{$item/@number}"/>
      <xsl:for-each select="$item/f:sequence">
	<xsl:call-template name="seq">
	  <xsl:with-param name="item" select="."/>
	</xsl:call-template>
      </xsl:for-each>
    </div>
  </xsl:template>

</xsl:stylesheet>
