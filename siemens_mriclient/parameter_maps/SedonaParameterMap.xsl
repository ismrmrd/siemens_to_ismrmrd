<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="xml" indent="yes"/>

<xsl:variable name="partialFourierPhase">
  <xsl:choose>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 1">0.5</xsl:when>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 2">0.75</xsl:when>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 2">0.875</xsl:when>
	<xsl:otherwise>1.0</xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:template match="/">
<ismrmrdHeader xsi:schemaLocation="http://www.ismrm.org/ISMRMRD ismrmrd.xsd"
        xmlns="http://www.ismrm.org/ISMRMRD"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xmlns:xs="http://www.w3.org/2001/XMLSchema">
        
  <experimentalConditions>
  	<H1resonanceFrequency_Hz><xsl:value-of select="siemens/DICOM/lFrequency"/></H1resonanceFrequency_Hz>
  </experimentalConditions>      
  <encoding>
    <trajectory>
      <xsl:choose>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">cartesian</xsl:when>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 2">radial</xsl:when>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 4">spiral</xsl:when>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 8">propellor</xsl:when>
		<xsl:otherwise>other</xsl:otherwise>
      </xsl:choose>
    </trajectory>
    <encodedSpace>
		<matrixSize>
			<x><xsl:value-of select="siemens/YAPS/iRoFTLength"/></x>		
			<y><xsl:value-of select="siemens/YAPS/iPEFTLength"/></y>		
			<z><xsl:value-of select="siemens/YAPS/i3DFTLength"/></z>
		</matrixSize>
		<fieldOfView_m>
			<x><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV * siemens/YAPS/flReadoutOSFactor"/></x>		
			<y><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dPhaseFOV * (1+siemens/IRIS/DERIVED/phaseOversampling)"/></y>		
			<z><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dThickness * (1+siemens/MEAS/sKSpace/dSliceOversamplingForDialog)"/></z>		
		</fieldOfView_m>
    </encodedSpace>
    <reconSpace>
		<matrixSize>
			<x><xsl:value-of select="siemens/IRIS/DERIVED/imageColumns"/></x>		
			<y><xsl:value-of select="siemens/IRIS/DERIVED/imageLines"/></y>		
			<z><xsl:value-of select="siemens/MEAS/sKSpace/lImagesPerSlab"/></z>
		</matrixSize>
		<fieldOfView_m>
			<x><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV"/></x>		
			<y><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dPhaseFOV"/></y>		
			<z><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dThickness"/></z>		
		</fieldOfView_m>
    </reconSpace>
    <encodingLimits>
		<kspace_encoding_step_1>
			<minimum>0</minimum>
			<maximum><xsl:value-of select="siemens/YAPS/iNoOfFourierLines - 1"/></maximum>
			<center><xsl:value-of select="floor((siemens/MEAS/sKSpace/lPhaseEncodingLines div 2)-(siemens/MEAS/sKSpace/lPhaseEncodingLines - siemens/YAPS/iNoOfFourierLines))"/></center>
		</kspace_encoding_step_1>    
		<kspace_encoding_step_2>
			<minimum>0</minimum>
			<maximum><xsl:value-of select="siemens/YAPS/iNoOfFourierPartitions - 1"/></maximum>
			<center><xsl:value-of select="floor((siemens/MEAS/sKSpace/lPartitions div 2)-(siemens/MEAS/sKSpace/lPartitions - siemens/YAPS/iNoOfFourierPartitions))"/></center>
		</kspace_encoding_step_2>    
    </encodingLimits>
  </encoding>
</ismrmrdHeader>
</xsl:template>

</xsl:stylesheet> 