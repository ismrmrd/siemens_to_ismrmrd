<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="/">
<ismrmrdHeader xsi:schemaLocation="http://www.ismrm.org/ISMRMRD ismrmrd.xsd"
        xmlns="http://www.ismrm.org/ISMRMRD"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xmlns:xs="http://www.w3.org/2001/XMLSchema">
  <encoding>
    <trajectory>
      <xsl:choose>
	<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">cartesian</xsl:when>
	<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 2">radial</xsl:when>
	<xsl:otherwise>
	  other
	</xsl:otherwise>
      </xsl:choose>
    </trajectory>
    <encodedSpace>
      <xsl:value-of select="siemens/MEAS/sKSpace/lBaseResolution"/>
    </encodedSpace>
  </encoding>
</ismrmrdHeader>
</xsl:template>

</xsl:stylesheet> 