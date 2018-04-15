<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="xml" indent="yes"/>

<xsl:variable name="phaseOversampling">
  <xsl:choose>
    <xsl:when test="not(siemens/IRIS/DERIVED/phaseOversampling)">0</xsl:when>
    <xsl:otherwise><xsl:value-of select="siemens/IRIS/DERIVED/phaseOversampling"/></xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:variable name="sliceOversampling">
  <xsl:choose>
    <xsl:when test="not(siemens/MEAS/sKSpace/dSliceOversamplingForDialog)">0</xsl:when>
    <xsl:otherwise><xsl:value-of select="siemens/MEAS/sKSpace/dSliceOversamplingForDialog"/></xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:variable name="partialFourierPhase">
  <xsl:choose>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 1">0.5</xsl:when>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 2">0.625</xsl:when>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 4">0.75</xsl:when>
	<xsl:when test="siemens/MEAS/sKSpace/ucPhasePartialFourier = 8">0.875</xsl:when>
	<xsl:otherwise>1.0</xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:variable name="numberOfContrasts">
	<xsl:value-of select="siemens/MEAS/lContrasts"/>
</xsl:variable>


<xsl:template match="/">
<ismrmrdHeader xsi:schemaLocation="http://www.ismrm.org/ISMRMRD ismrmrd.xsd"
        xmlns="http://www.ismrm.org/ISMRMRD"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xmlns:xs="http://www.w3.org/2001/XMLSchema">
        
    <!--
    <subjectInformation>
        <patientName>
            <xsl:value-of select="siemens/DICOM/tPatientName"/>
        </patientName>
        <xsl:if test="siemens/YAPS/flUsedPatientWeight > 0">
            <patientWeight_kg>
                <xsl:value-of select="siemens/YAPS/flUsedPatientWeight"/>
            </patientWeight_kg>
        </xsl:if>
        <patientID>
            <xsl:value-of select="siemens/IRIS/RECOMPOSE/PatientID"/>
        </patientID>
        <patientGender>
            <xsl:choose>
                <xsl:when test="siemens/DICOM/lPatientSex = 1">F</xsl:when>
                <xsl:when test="siemens/DICOM/lPatientSex = 2">M</xsl:when>
                <xsl:otherwise>O</xsl:otherwise>
            </xsl:choose>
        </patientGender>
    </subjectInformation>

    <studyInformation>
        <studyInstanceUID>
            <xsl:value-of select="$studyID" />
        </studyInstanceUID>
    </studyInformation>
    -->

  <acquisitionSystemInformation>
  	<systemVendor><xsl:value-of select="siemens/DICOM/Manufacturer"/></systemVendor>
  	<systemModel><xsl:value-of select="siemens/DICOM/ManufacturersModelName"/></systemModel>  	
  	<systemFieldStrength_T><xsl:value-of select="siemens/YAPS/flMagneticFieldStrength"/></systemFieldStrength_T>
  	<relativeReceiverNoiseBandwidth>0.79</relativeReceiverNoiseBandwidth> 	
  	<receiverChannels><xsl:value-of select="siemens/YAPS/iMaxNoOfRxChannels" /></receiverChannels> 
	
	<!-- Coil Labels -->
	<xsl:choose>
          <!-- VD line with dual density -->
                    <xsl:when test="siemens/MEAS/asCoilSelectMeas/ADC/lADCChannelConnected">
                        <xsl:variable name="NumberOfSelectedCoils">
                            <xsl:value-of select="count(siemens/MEAS/asCoilSelectMeas/Select/lElementSelected[text() = '1'])" />
                        </xsl:variable>
                        <xsl:for-each select="siemens/MEAS/asCoilSelectMeas/ADC/lADCChannelConnected[position() >= 1  and not(position() > $NumberOfSelectedCoils)]">
                            <xsl:sort data-type="number"
                                      select="." />
                            <xsl:variable name="CurADC"
                                          select="."/>
                            <xsl:variable name="CurADCIndex"
                                          select="position()" />
                            <xsl:for-each select="../lADCChannelConnected[position() >= 1  and not(position() > $NumberOfSelectedCoils)]">
                                <xsl:if test="$CurADC = .">
                                    <xsl:variable name="CurCoil" select="position()"/>
                                    <xsl:variable name="CurCoilID" select="../../ID/tCoilID[$CurCoil]"/>
                                    <xsl:variable name="CurCoilElement" select="../../Elem/tElement[$CurCoil]"/>
                                    <xsl:variable name="CurCoilCopyID" select="../../Coil/lCoilCopy[$CurCoil]"/>
                                    <coilLabel>
                                        <coilNumber>
                                            <xsl:value-of select="number(../lADCChannelConnected[$CurADCIndex])"/>
                                        </coilNumber>
                                        <coilName>
                                            <xsl:value-of select="$CurCoilID"/>:<xsl:value-of select="string($CurCoilCopyID)"/>:<xsl:value-of select="$CurCoilElement"/>
                                        </coilName>
                                    </coilLabel>
                                </xsl:if>
                            </xsl:for-each>
                        </xsl:for-each>
                    </xsl:when>
          <xsl:otherwise> <!-- This is probably VB -->
            <xsl:for-each select="siemens/MEAS/asCoilSelectMeas/ID/tCoilID">
              <xsl:variable name="CurCoil" select="position()"/>
	      <coilLabel>
		<coilNumber><xsl:value-of select="$CurCoil -1"/></coilNumber>
		<coilName><xsl:value-of select="."/>:<xsl:value-of select="../../Elem/tElement[$CurCoil]"/></coilName>
	      </coilLabel>
            </xsl:for-each>
          </xsl:otherwise>
        </xsl:choose>
  </acquisitionSystemInformation> 
        
  <experimentalConditions>
  	<H1resonanceFrequency_Hz><xsl:value-of select="siemens/DICOM/lFrequency"/></H1resonanceFrequency_Hz>
  </experimentalConditions>      
  <encoding>
    <trajectory>
      <xsl:choose>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">cartesian</xsl:when>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 2">radial</xsl:when>
		<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 4">spiral</xsl:when>
		<!--<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 8">propellor</xsl:when>--> <!-- "propellor is not valid for ismrmrd -->
		<xsl:otherwise>other</xsl:otherwise>
      </xsl:choose>
    </trajectory>
    
    <xsl:if test="siemens/MEAS/sKSpace/ucTrajectory = 4">
	    <trajectoryDescription>
	    	<identifier>HargreavesVDS2000</identifier>
	    	<userParameterLong>
	    		<name>interleaves</name><value><xsl:value-of select="siemens/MEAS/sKSpace/lRadialViews" /></value>
	    	</userParameterLong>
	    	<userParameterLong>
	    		<name>fov_coefficients</name><value>1</value>
	    	</userParameterLong>
	    	<userParameterLong>
	    		<name>SamplingTime_ns</name><value><xsl:value-of select="siemens/MEAS/sWipMemBlock/alFree[57]" /></value>
	    	</userParameterLong>
	    	<userParameterDouble>
	    		<name>MaxGradient_G_per_cm</name><value><xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[7]" /></value>
	    	</userParameterDouble>
	    	<userParameterDouble>
	    		<name>MaxSlewRate_G_per_cm_per_s</name><value><xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[8]" /></value>
	    	</userParameterDouble>
	    	<userParameterDouble>
	    		<name>FOVCoeff_1_cm</name><value><xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[10]" /></value>
	    	</userParameterDouble>
	    	<userParameterDouble>
	    		<name>krmax_per_cm</name><value><xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[9]" /></value>
	    	</userParameterDouble>
	    	<comment>Using spiral design by Brian Hargreaves (http://mrsrl.stanford.edu/~brian/vdspiral/)</comment>
	    </trajectoryDescription>    
    </xsl:if>
    <encodedSpace>
		<matrixSize>
			<xsl:choose>
				<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">	
					<x><xsl:value-of select="siemens/YAPS/iRoFTLength"/></x>		
				</xsl:when>	
				<xsl:otherwise>
					<x><xsl:value-of select="siemens/IRIS/DERIVED/imageColumns"/></x>
				</xsl:otherwise>
			</xsl:choose>		
		
			<y><xsl:value-of select="siemens/YAPS/iPEFTLength"/></y>		
			<z><xsl:value-of select="siemens/YAPS/i3DFTLength"/></z>
		</matrixSize>
		<fieldOfView_mm>
		  	<xsl:choose>
				<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">	
					<x><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV * siemens/YAPS/flReadoutOSFactor"/></x>
				</xsl:when>	
				<xsl:otherwise>
					<x><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV"/></x>
				</xsl:otherwise>
			</xsl:choose>		
			<y><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dPhaseFOV * (1+$phaseOversampling)"/></y>		
			<z><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dThickness * (1+$sliceOversampling)"/></z>		
		</fieldOfView_mm>
    </encodedSpace>
    <reconSpace>
		<matrixSize>
			<x><xsl:value-of select="siemens/IRIS/DERIVED/imageColumns"/></x>		
			<y><xsl:value-of select="siemens/IRIS/DERIVED/imageLines"/></y>
			<xsl:choose>
				<xsl:when test="siemens/YAPS/i3DFTLength = 1">
					<z>1</z>
				</xsl:when>	
				<xsl:otherwise>
					<z><xsl:value-of select="siemens/MEAS/sKSpace/lImagesPerSlab"/></z>		
				</xsl:otherwise>
			</xsl:choose>	
		</matrixSize>
		<fieldOfView_mm>
			<x><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV"/></x>		
			<y><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dPhaseFOV"/></y>		
			<z><xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dThickness"/></z>		
		</fieldOfView_mm>
    </reconSpace>
    <encodingLimits>
		<kspace_encoding_step_1>
			<minimum>0</minimum>
			<maximum><xsl:value-of select="siemens/YAPS/iNoOfFourierLines - 1"/></maximum>
			<xsl:choose>
				<xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">	
					<center><xsl:value-of select="floor((siemens/MEAS/sKSpace/lPhaseEncodingLines div 2)-(siemens/MEAS/sKSpace/lPhaseEncodingLines - siemens/YAPS/iNoOfFourierLines))"/></center>
				</xsl:when>
				<xsl:otherwise>
					<center>0</center>
				</xsl:otherwise>
			</xsl:choose>
		</kspace_encoding_step_1>    
		<kspace_encoding_step_2>
			<minimum>0</minimum>
			<xsl:choose>
				<xsl:when test="not(siemens/YAPS/iNoOfFourierPartitions) or (siemens/YAPS/i3DFTLength = 1)">
					<maximum>0</maximum>
					<center>0</center>				
				</xsl:when>
				<xsl:otherwise>
					<maximum><xsl:value-of select="siemens/YAPS/iNoOfFourierPartitions - 1"/></maximum>
					<center><xsl:value-of select="floor((siemens/MEAS/sKSpace/lPartitions div 2)-(siemens/MEAS/sKSpace/lPartitions - siemens/YAPS/iNoOfFourierPartitions))"/></center>
				</xsl:otherwise>
			</xsl:choose>
		</kspace_encoding_step_2>
		<slice>
			<minimum>0</minimum>
			<maximum><xsl:value-of select="siemens/MEAS/sSliceArray/lSize - 1"/></maximum>
			<center>0</center>
		</slice>    		    
		<set>
			<minimum>0</minimum>
			<maximum>
			<xsl:choose>
				<xsl:when test="siemens/MEAS/sAngio/sFlowArray/lSize">
					<xsl:value-of select="siemens/MEAS/sAngio/sFlowArray/lSize"/>
				</xsl:when>
				<xsl:otherwise>0</xsl:otherwise>
			</xsl:choose>
			</maximum>
			<center>0</center>
		</set>    		    
    </encodingLimits>
    <xsl:if test="not(siemens/MEAS/sPat/ucPATMode = 1)">  
      <parallelImaging>
  	<accelerationFactor>
	  <kspace_encoding_step_1>
	    <xsl:choose>
	      <xsl:when test="not(siemens/MEAS/sPat/lAccelFactPE)">1</xsl:when>
	      <xsl:otherwise><xsl:value-of select="(siemens/MEAS/sPat/lAccelFactPE)"/></xsl:otherwise>
	    </xsl:choose>
	  </kspace_encoding_step_1>
	  <kspace_encoding_step_2>
	    <xsl:choose>
	      <xsl:when test="not(siemens/MEAS/sPat/lAccelFact3D)">1</xsl:when>
	      <xsl:otherwise><xsl:value-of select="(siemens/MEAS/sPat/lAccelFact3D)"/></xsl:otherwise>
	    </xsl:choose>
	  </kspace_encoding_step_2>
  	</accelerationFactor>
  	<calibrationMode>
	  <xsl:choose>
	    <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 2">embedded</xsl:when>
	    <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 4">separate</xsl:when>
	    <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 8">separate</xsl:when>
	    <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 16">interleaved</xsl:when>
	    <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 32">interleaved</xsl:when>
	    <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 64">interleaved</xsl:when>
	    <xsl:otherwise>other</xsl:otherwise>
	  </xsl:choose>
  	</calibrationMode>
  	<xsl:if test="(siemens/MEAS/sPat/ucRefScanMode = 16) or (siemens/MEAS/sPat/ucRefScanMode = 32) or (siemens/MEAS/sPat/ucRefScanMode = 64)">
	  <interleavingDimension>
	    <xsl:choose>
	      <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 16">average</xsl:when>
	      <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 32">repetition</xsl:when>
	      <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 64">phase</xsl:when>
	      <xsl:otherwise>other</xsl:otherwise>
	    </xsl:choose>
	  </interleavingDimension>
  	</xsl:if>
      </parallelImaging>
    </xsl:if>
  </encoding>
  <sequenceParameters>
   <xsl:for-each select="siemens/MEAS/alTR">
	<xsl:if test=". &gt; 0"><TR><xsl:value-of select=". div 1000.0" /></TR></xsl:if>
   </xsl:for-each>
   <xsl:for-each select="siemens/MEAS/alTE">
	<xsl:if test=". &gt; 0">
		<xsl:if test="position() &lt; ($numberOfContrasts + 1)">
		<TE><xsl:value-of select=". div 1000.0" /></TE>
		</xsl:if>
	</xsl:if>
   </xsl:for-each>
   <xsl:for-each select="siemens/MEAS/alTI">
	<xsl:if test=". &gt; 0"><TI><xsl:value-of select=". div 1000.0" /></TI></xsl:if>
   </xsl:for-each>
   <xsl:for-each select="siemens/DICOM/adFlipAngleDegree">
    <xsl:if test=". &gt; 0">
            <flipAngle_deg>
                <xsl:value-of select="." />
            </flipAngle_deg>
    </xsl:if>
   </xsl:for-each>
   <xsl:if test="siemens/MEAS/ucSequenceType">
        <sequence_type>
            <xsl:choose>
                <xsl:when test="siemens/MEAS/ucSequenceType = 1">Flash</xsl:when>
                <xsl:when test="siemens/MEAS/ucSequenceType = 2">SSFP</xsl:when>
                <xsl:when test="siemens/MEAS/ucSequenceType = 4">EPI</xsl:when>
                <xsl:when test="siemens/MEAS/ucSequenceType = 8">TurboSpinEcho</xsl:when>
                <xsl:when test="siemens/MEAS/ucSequenceType = 16">ChemicalShiftImaging</xsl:when>
                <xsl:when test="siemens/MEAS/ucSequenceType = 32">FID</xsl:when>
                <xsl:otherwise>Unknown</xsl:otherwise>
            </xsl:choose>
        </sequence_type>
   </xsl:if>
   <xsl:if test="siemens/YAPS/lEchoSpacing">
        <echo_spacing>
            <xsl:value-of select="siemens/YAPS/lEchoSpacing div 1000.0" />
        </echo_spacing>
   </xsl:if>
  </sequenceParameters>
</ismrmrdHeader>
</xsl:template>

</xsl:stylesheet> 
