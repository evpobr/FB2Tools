<?xml version="1.0" encoding="us-ascii"?>

<X:stylesheet version="1.0" xmlns:X="http://www.w3.org/1999/XSL/Transform">

  <X:output method="text" encoding="utf-8"/>

  <X:template match="/">
    <X:for-each select="//genre">
      <X:value-of select="normalize-space(root-descr[@lang='en']/@genre-title)"/>
      <X:text>&#10;</X:text>
      <X:for-each select="subgenres/subgenre">
        <X:text> </X:text>
        <X:value-of select="normalize-space(@path)"/>
        <X:text> </X:text>
        <X:value-of select="normalize-space(genre-descr[@lang='en']/@title)"/>
        <X:text>&#10;</X:text>
      </X:for-each>
    </X:for-each>
  </X:template>
</X:stylesheet>