<?xml version="1.0" encoding="utf-8"?>

<!-- this stylesheet handles conversion of <body> fictionbook elements
  to editable html.

  Available elements:
    structure: section epigraph poem stanza cite title annotation
    text: p v empty-line text-author subtitle
    inline: strong emphasis a
-->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:f="http://www.gribuser.ru/xml/fictionbook/2.0"
  xmlns:l="http://www.w3.org/1999/xlink">

  <xsl:output
    method="html"
    indent="no"
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd"
  />

  <!-- colors and fonts -->
  <xsl:param name="font" select="'Trebuchet MS'"/>
  <xsl:param name="fontSize" select="'12'"/>
  <xsl:param name="colorFG"/>
  <xsl:param name="colorBG"/>

  <!-- document ID -->
  <xsl:param name="dID"/>
  <xsl:param name="bodyscript"/>

  <xsl:template match="/">
    <html>
      <head>
	<style type="text/css">
	  body { 
	    font-family: "<xsl:value-of select="$font"/>", sans-serif;
	    font-size: <xsl:value-of select="$fontSize"/>pt;
	    <xsl:if test="$colorBG">
	      background-color: <xsl:value-of select="$colorBG"/>;
	    </xsl:if>
	    <xsl:if test="$colorFG">
	      color: <xsl:value-of select="$colorFG"/>;
	    </xsl:if>
	  }

	  div {
	    padding: 0em 0em 0em 0.5em;
	    margin: 0.5em 0em 0.5em 0em;
	    border-left: solid 2px;
	    clear: both;
	  }
	  div.body { padding: 0px; margin: 0px; border: none; }
	  div.section { border-color: green; }
	  div.cite {
	    padding-left: 1em;
	    padding-right: 1em;
	    color: rgb(228,175,0);
	    border-color: rgb(228,175,0);
	  }
	  div.epigraph { font-size: 75%; border-color: rgb(165,37,207); }
	  div.epigraph p { text-indent: 2em; }
	  div.annotation { border-color: rgb(72,150,172); }
	  div.history { border-color: rgb(255,0,128); }
	  div.poem { border-color: black; }
	  div.stanza { border-color: rgb(208,36,36); }
	  div.stanza p { text-align: left; text-indent: 0em; margin-bottom: 0em; }
	  div.title { font-size: 150%; border-color: green; background: green; color: black; }
	  div.title p { text-align: center; text-indent: 0em; }  
	  p { text-indent: 3em; margin-top: 0em; margin-bottom: 0.2em; text-align: justify; }
	  p.subtitle { text-align: center; font-size: 120%; text-indent: 0em; }
	  p.text-author { margin-left: 3em; margin-bottom: 1em; color: rgb(192,64,64); }

	  a.note { vertical-align: super; color: blue; font-size: 75%; }

	  div.image {
	    margin: 0px;
	    padding: 0px;
	    border: none;
	    overflow: hidden;
	    width: 100%;
	    height: 100px;
	    text-align: center;
	  }
	  img { border: none; margin: 0px; padding: 0px; position: relative; z-index: -1; }
	</style>
	<script type="text/javascript">
	  document.urlprefix="fbe-internal:<xsl:value-of select="$dID"/>:";
	</script>
	<script type="text/javascript" src="{$bodyscript}"/>
      </head>
      <body contentEditable="true" oncontextmenu="return false" onload="oncontextmenu=undefined">
	<!-- book annotation -->
	  <div class="annotation">
	    <xsl:apply-templates select="/f:FictionBook/f:description/f:title-info/f:annotation/*"/>
	    <xsl:if test="not(/f:FictionBook/f:description/f:title-info/f:annotation/*)">
	      <p/>
	    </xsl:if>
	  </div>
	<!-- book history -->
	  <div class="history">
	    <xsl:apply-templates select="/f:FictionBook/f:description/f:document-info/f:history/*"/>
	    <xsl:if test="not(/f:FictionBook/f:description/f:document-info/f:history/*)">
	      <p/>
	    </xsl:if>
	  </div>
	<!-- bodies -->
	<xsl:apply-templates select="/f:FictionBook/f:body"/>
	<xsl:if test="not(/f:FictionBook/f:body)">
	  <div class="body">
	    <div class="title">
	      <p/>
	    </div>
	    <div class="section">
	      <p/>
	    </div>
	  </div>
	</xsl:if>
      </body>
    </html>
  </xsl:template>

  <!-- structure -->
  <xsl:template match="f:body">
    <div class="body" fbname="{@name}"><!-- container -->
      <xsl:apply-templates/>
    </div>
  </xsl:template>

  <xsl:template match="f:section | f:annotation | f:epigraph | f:cite | f:poem | f:stanza | f:title">
    <div class="{local-name()}"><!-- container -->
      <xsl:call-template name="id"/>
      <!-- element content -->
      <xsl:apply-templates/>
    </div>
  </xsl:template>

  <!-- images -->
  <xsl:template match="f:image">
    <div onresizestart="return false" class='image' contentEditable='false' href="{@l:href}" imgW="0" imgH="0">
      <img onload='ImgLoaded(this)' src="fbe-internal:{$dID}:{@l:href}"/>
    </div>
  </xsl:template>

  <!-- text -->
  <xsl:template match="f:p | f:v">
    <p><xsl:call-template name="id"/><xsl:call-template name="style"/><xsl:apply-templates/></p>
  </xsl:template>
  <xsl:template match="f:empty-line">
    <p><xsl:call-template name="id"/></p>
  </xsl:template>
  <xsl:template match="f:text-author | f:subtitle">
    <p class="{local-name()}">
      <xsl:call-template name="id"/>
      <xsl:apply-templates/>
    </p>
  </xsl:template>

  <!-- inline -->
  <xsl:template match="f:emphasis">
    <em><xsl:apply-templates/></em>
  </xsl:template>
  <xsl:template match="f:strong">
    <strong><xsl:apply-templates/></strong>
  </xsl:template>
  <xsl:template match="f:style">
    <span class="{@name}"><xsl:apply-templates/></span>
  </xsl:template>
  <xsl:template match="f:a">
    <a class="{@type}" href="{@l:href}"><xsl:apply-templates/></a>
  </xsl:template>

  <!-- IDs -->
  <xsl:template name="id">
    <xsl:if test="@id">
      <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
    </xsl:if>
  </xsl:template>

  <!-- paragraph styles -->
  <xsl:template name="style">
    <xsl:if test="@style">
      <xsl:attribute name="fbstyle"><xsl:value-of select="@style"/></xsl:attribute>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
