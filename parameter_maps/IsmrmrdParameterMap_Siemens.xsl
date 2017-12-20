<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:output method="xml" indent="yes"/>

    <xsl:variable name="phaseOversampling">
        <xsl:choose>
            <xsl:when test="not(siemens/IRIS/DERIVED/phaseOversampling)">0</xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="siemens/IRIS/DERIVED/phaseOversampling"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>

    <xsl:variable name="sliceOversampling">
        <xsl:choose>
            <xsl:when test="not(siemens/MEAS/sKSpace/dSliceOversamplingForDialog)">0</xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="siemens/MEAS/sKSpace/dSliceOversamplingForDialog"/>
            </xsl:otherwise>
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

    <xsl:variable name="studyID">
        <xsl:value-of select="substring(siemens/IRIS/RECOMPOSE/StudyLOID, 6)"/>
    </xsl:variable>

    <xsl:variable name="patientID">
        <xsl:value-of select="substring(siemens/IRIS/RECOMPOSE/PatientLOID, 6)"/>
    </xsl:variable>

    <xsl:variable name="strSeperator">_</xsl:variable>

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

            <measurementInformation>
                <measurementID>
                    <xsl:value-of select="concat(string(siemens/DICOM/DeviceSerialNumber), $strSeperator, $patientID, $strSeperator, $studyID, $strSeperator, string(siemens/HEADER/MeasUID))"/>
                </measurementID>
                <patientPosition>
                    <xsl:value-of select="siemens/YAPS/tPatientPosition"/>
                </patientPosition>
                <protocolName>
                    <xsl:value-of select="siemens/MEAS/tProtocolName"/>
                </protocolName>

                <xsl:if test="siemens/YAPS/ReconMeasDependencies/RFMap > 0">
                    <measurementDependency>
                        <dependencyType>RFMap</dependencyType>
                        <measurementID>
                            <xsl:value-of select="concat(string(siemens/DICOM/DeviceSerialNumber), $strSeperator, $patientID, $strSeperator, $studyID, $strSeperator, string(siemens/YAPS/ReconMeasDependencies/RFMap))"/>
                        </measurementID>
                    </measurementDependency>
                </xsl:if>

                <xsl:if test="siemens/YAPS/ReconMeasDependencies/SenMap > 0">
                    <measurementDependency>
                        <dependencyType>SenMap</dependencyType>
                        <measurementID>
                            <xsl:value-of select="concat(string(siemens/DICOM/DeviceSerialNumber), $strSeperator, $patientID, $strSeperator, $studyID, $strSeperator, string(siemens/YAPS/ReconMeasDependencies/SenMap))"/>
                        </measurementID>
                    </measurementDependency>
                </xsl:if>

                <xsl:if test="siemens/YAPS/ReconMeasDependencies/Noise > 0">
                    <measurementDependency>
                        <dependencyType>Noise</dependencyType>
                        <measurementID>
                            <xsl:value-of select="concat(string(siemens/DICOM/DeviceSerialNumber), $strSeperator, $patientID, $strSeperator, $studyID, $strSeperator, string(siemens/YAPS/ReconMeasDependencies/Noise))"/>
                        </measurementID>
                    </measurementDependency>
                </xsl:if>

                <frameOfReferenceUID>
                    <xsl:value-of select="siemens/YAPS/tFrameOfReference" />
                </frameOfReferenceUID>

            </measurementInformation>

            <acquisitionSystemInformation>
                <systemVendor>
                    <xsl:value-of select="siemens/DICOM/Manufacturer"/>
                </systemVendor>
                <systemModel>
                    <xsl:value-of select="siemens/DICOM/ManufacturersModelName"/>
                </systemModel>
                <systemFieldStrength_T>
                    <xsl:value-of select="siemens/YAPS/flMagneticFieldStrength"/>
                </systemFieldStrength_T>
                <relativeReceiverNoiseBandwidth>0.793</relativeReceiverNoiseBandwidth>
                <receiverChannels>
                    <xsl:value-of select="siemens/YAPS/iMaxNoOfRxChannels" />
                </receiverChannels>
		
		<!-- Coil Labels -->
		<xsl:choose>
                  <!-- VD line with dual density -->
                  <xsl:when test="siemens/MEAS/asCoilSelectMeas/ADC/lADCChannelConnected">
                    <xsl:variable name="NumberOfSelectedCoils">
                        <xsl:value-of select="count(siemens/MEAS/asCoilSelectMeas/Select/lElementSelected[text() = '1'])" />
                    </xsl:variable>
                    <xsl:for-each select="siemens/MEAS/asCoilSelectMeas/ADC/lADCChannelConnected[position() >= 1  and not(position() > $NumberOfSelectedCoils)]">
                      <xsl:sort data-type="number" select="." />
                      <xsl:variable name="CurADC" select="."/>
                      <xsl:variable name="CurADCIndex" select="position()" />
                      <xsl:for-each select="../lADCChannelConnected[position() >= 1  and not(position() > $NumberOfSelectedCoils)]">
                        <xsl:if test="$CurADC = .">
                            <xsl:variable name="CurCoil" select="position()"/>
			    <coilLabel>
			      <coilNumber><xsl:value-of select="$CurADCIndex -1"/></coilNumber>
			      <coilName><xsl:value-of select="../../ID/tCoilID[$CurCoil]"/>:<xsl:value-of select="../../Elem/tElement[$CurCoil]"/></coilName>
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

                <institutionName>
                    <xsl:value-of select="siemens/DICOM/InstitutionName" />
                </institutionName>
            </acquisitionSystemInformation>

            <experimentalConditions>
                <H1resonanceFrequency_Hz>
                    <xsl:value-of select="siemens/DICOM/lFrequency"/>
                </H1resonanceFrequency_Hz>
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

                <xsl:if test="siemens/MEAS/sKSpace/ucTrajectory = 4">
                    <trajectoryDescription>
                        <identifier>HargreavesVDS2000</identifier>
                        <userParameterLong>
                            <name>interleaves</name>
                            <value>
                                <xsl:value-of select="siemens/MEAS/sKSpace/lRadialViews" />
                            </value>
                        </userParameterLong>
                        <userParameterLong>
                            <name>fov_coefficients</name>
                            <value>1</value>
                        </userParameterLong>
                        <userParameterLong>
                            <name>SamplingTime_ns</name>
                            <value>
                                <xsl:value-of select="siemens/MEAS/sWipMemBlock/alFree[57]" />
                            </value>
                        </userParameterLong>
                        <userParameterDouble>
                            <name>MaxGradient_G_per_cm</name>
                            <value>
                                <xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[7]" />
                            </value>
                        </userParameterDouble>
                        <userParameterDouble>
                            <name>MaxSlewRate_G_per_cm_per_s</name>
                            <value>
                                <xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[8]" />
                            </value>
                        </userParameterDouble>
                        <userParameterDouble>
                            <name>FOVCoeff_1_cm</name>
                            <value>
                                <xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[10]" />
                            </value>
                        </userParameterDouble>
                        <userParameterDouble>
                            <name>krmax_per_cm</name>
                            <value>
                                <xsl:value-of select="siemens/MEAS/sWipMemBlock/adFree[9]" />
                            </value>
                        </userParameterDouble>
                        <comment>Using spiral design by Brian Hargreaves (http://mrsrl.stanford.edu/~brian/vdspiral/)</comment>
                    </trajectoryDescription>
                </xsl:if>

                <xsl:if test="siemens/YAPS/alRegridRampupTime > 0">
                    <xsl:if test="siemens/YAPS/alRegridRampdownTime > 0">
                        <trajectoryDescription>
                            <identifier>ConventionalEPI</identifier>
                            <userParameterLong>
                                <name>etl</name>
                                <value>
                                    <xsl:value-of select="siemens/MEAS/sFastImaging/lEPIFactor"/>
                                </value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>numberOfNavigators</name>
                                <value>3</value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>rampUpTime</name>
                                <value>
                                    <xsl:value-of select="siemens/YAPS/alRegridRampupTime"/>
                                </value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>rampDownTime</name>
                                <value>
                                    <xsl:value-of select="siemens/YAPS/alRegridRampdownTime"/>
                                </value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>flatTopTime</name>
                                <value>
                                    <xsl:value-of select="siemens/YAPS/alRegridFlattopTime"/>
                                </value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>echoSpacing</name>
                                <value>
                                    <xsl:value-of select="siemens/YAPS/lEchoSpacing"/>
                                </value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>acqDelayTime</name>
                                <value>
                                    <xsl:value-of select="siemens/YAPS/alRegridDelaySamplesTime"/>
                                </value>
                            </userParameterLong>
                            <userParameterLong>
                                <name>numSamples</name>
                                <value>
                                    <xsl:value-of select="siemens/YAPS/alRegridDestSamples"/>
                                </value>
                            </userParameterLong>
                            <userParameterDouble>
                                <name>dwellTime</name>
                                <value>
                                    <xsl:value-of select="siemens/MEAS/sRXSPEC/alDwellTime div 1000.0"/>
                                </value>
                            </userParameterDouble>
                            <comment>Conventional 2D EPI sequence</comment>
                        </trajectoryDescription>
                    </xsl:if>
                </xsl:if>

                <encodedSpace>
                    <matrixSize>

                        <xsl:choose>
                            <xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">
                                <x>
                                    <xsl:value-of select="siemens/YAPS/iNoOfFourierColumns"/>
                                </x>
                            </xsl:when>
                            <xsl:otherwise>
                                <x>
                                    <xsl:value-of select="siemens/IRIS/DERIVED/imageColumns"/>
                                </x>
                            </xsl:otherwise>
                        </xsl:choose>

                        <xsl:choose>
                            <xsl:when test="siemens/MEAS/sKSpace/uc2DInterpolation" >
                                <xsl:choose>
                                    <xsl:when test="siemens/MEAS/sKSpace/uc2DInterpolation = 1">
                                        <y>
                                            <xsl:value-of select="floor(siemens/YAPS/iPEFTLength div 2)"/>
                                        </y>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <y>
                                            <xsl:value-of select="siemens/YAPS/iPEFTLength"/>
                                        </y>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </xsl:when>
                            <xsl:otherwise>
                                <y>
                                    <xsl:value-of select="siemens/YAPS/iPEFTLength"/>
                                </y>
                            </xsl:otherwise>
                        </xsl:choose>

                        <xsl:choose>
                            <xsl:when test="not(siemens/YAPS/iNoOfFourierPartitions) or (siemens/YAPS/i3DFTLength = 1)">
                                <z>1</z>
                            </xsl:when>
                            <xsl:otherwise>
                                <z>
                                    <xsl:value-of select="siemens/YAPS/i3DFTLength"/>
                                </z>
                            </xsl:otherwise>
                        </xsl:choose>
                    </matrixSize>

                    <fieldOfView_mm>
                        <xsl:choose>
                            <xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">
                                <x>
                                    <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV * siemens/YAPS/flReadoutOSFactor"/>
                                </x>
                            </xsl:when>
                            <xsl:otherwise>
                                <x>
                                    <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV"/>
                                </x>
                            </xsl:otherwise>
                        </xsl:choose>
                        <y>
                            <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dPhaseFOV * (1+$phaseOversampling)"/>
                        </y>
                        <z>
                            <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dThickness * (1+$sliceOversampling)"/>
                        </z>
                    </fieldOfView_mm>
                </encodedSpace>
                <reconSpace>
                    <matrixSize>
                        <x>
                            <xsl:value-of select="siemens/IRIS/DERIVED/imageColumns"/>
                        </x>
                        <y>
                            <xsl:value-of select="siemens/IRIS/DERIVED/imageLines"/>
                        </y>
                        <xsl:choose>
                            <xsl:when test="siemens/YAPS/i3DFTLength = 1">
                                <z>1</z>
                            </xsl:when>
                            <xsl:otherwise>
                                <z>
                                    <xsl:value-of select="siemens/MEAS/sKSpace/lImagesPerSlab"/>
                                </z>
                            </xsl:otherwise>
                        </xsl:choose>
                    </matrixSize>
                    <fieldOfView_mm>
                        <x>
                            <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dReadoutFOV"/>
                        </x>
                        <y>
                            <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dPhaseFOV"/>
                        </y>
                        <z>
                            <xsl:value-of select="siemens/MEAS/sSliceArray/asSlice/s0/dThickness"/>
                        </z>
                    </fieldOfView_mm>
                </reconSpace>
                <encodingLimits>
                    <kspace_encoding_step_1>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:value-of select="siemens/YAPS/iNoOfFourierLines - 1"/>
                        </maximum>
                        <center>
                            <xsl:value-of select="floor(siemens/MEAS/sKSpace/lPhaseEncodingLines div 2)"/>
                        </center>
                    </kspace_encoding_step_1>
                    <kspace_encoding_step_2>
                        <minimum>0</minimum>
                        <xsl:choose>
                            <xsl:when test="not(siemens/YAPS/iNoOfFourierPartitions) or (siemens/YAPS/i3DFTLength = 1)">
                                <maximum>0</maximum>
                                <center>0</center>
                            </xsl:when>
                            <xsl:otherwise>
                                <maximum>
                                    <xsl:value-of select="siemens/YAPS/iNoOfFourierPartitions - 1"/>
                                </maximum>
                                <xsl:choose>
                                    <xsl:when test="siemens/MEAS/sKSpace/ucTrajectory = 1">
                                        <xsl:choose>
                                            <xsl:when test="siemens/MEAS/sPat/lAccelFact3D">
                                                <xsl:choose>
                                                    <xsl:when test="not(siemens/MEAS/sPat/lAccelFact3D) > 1">
                                                        <center>
                                                            <xsl:value-of select="floor(siemens/MEAS/sKSpace/lPartitions div 2) - (siemens/YAPS/lPartitions - siemens/YAPS/iNoOfFourierPartitions)"/>
                                                        </center>
                                                    </xsl:when>
                                                    <xsl:otherwise>
                                                        <xsl:choose>
                                                            <xsl:when test="(siemens/MEAS/sKSpace/lPartitions - siemens/YAPS/iNoOfFourierPartitions) > siemens/MEAS/sPat/lAccelFact3D">
                                                                <center>
                                                                    <xsl:value-of select="floor(siemens/MEAS/sKSpace/lPartitions div 2) - (siemens/MEAS/sKSpace/lPartitions - siemens/YAPS/iNoOfFourierPartitions)"/>
                                                                </center>
                                                            </xsl:when>
                                                            <xsl:otherwise>
                                                                <center>
                                                                    <xsl:value-of select="floor(siemens/MEAS/sKSpace/lPartitions div 2)"/>
                                                                </center>
                                                            </xsl:otherwise>
                                                        </xsl:choose>
                                                    </xsl:otherwise>
                                                </xsl:choose>
                                            </xsl:when>
                                            <xsl:otherwise>
                                                <center>
                                                    <xsl:value-of select="floor(siemens/MEAS/sKSpace/lPartitions div 2) - (siemens/MEAS/sKSpace/lPartitions - siemens/YAPS/iNoOfFourierPartitions)"/>
                                                </center>
                                            </xsl:otherwise>
                                        </xsl:choose>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <center>0</center>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </xsl:otherwise>
                        </xsl:choose>
                    </kspace_encoding_step_2>
                    <slice>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:value-of select="siemens/MEAS/sSliceArray/lSize - 1"/>
                        </maximum>
                        <center>0</center>
                    </slice>
                    <set>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:choose>
                                <xsl:when test="siemens/YAPS/iNSet">
                                    <xsl:value-of select="siemens/YAPS/iNSet - 1"/>
                                </xsl:when>
                                <xsl:otherwise>0</xsl:otherwise>
                            </xsl:choose>
                        </maximum>
                        <center>0</center>
                    </set>
                    <phase>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:choose>
                                <xsl:when test="siemens/MEAS/sPhysioImaging/lPhases">
                                    <xsl:value-of select="siemens/MEAS/sPhysioImaging/lPhases - 1"/>
                                </xsl:when>
                                <xsl:otherwise>0</xsl:otherwise>
                            </xsl:choose>
                        </maximum>
                        <center>0</center>
                    </phase>
                    <repetition>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:choose>
                                <xsl:when test="siemens/MEAS/lRepetitions">
                                    <xsl:value-of select="siemens/MEAS/lRepetitions"/>
                                </xsl:when>
                                <xsl:otherwise>0</xsl:otherwise>
                            </xsl:choose>
                        </maximum>
                        <center>0</center>
                    </repetition>
                    <segment>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:choose>
                                <xsl:when test="siemens/MEAS/sFastImaging/ucSegmentationMode" >
                                    <xsl:choose>
                                        <xsl:when test="siemens/MEAS/sFastImaging/ucSegmentationMode = 2">
                                            <xsl:choose>
                                                <xsl:when test="siemens/MEAS/sFastImaging/lShots">
                                                    <xsl:value-of select="siemens/MEAS/sFastImaging/lShots - 1"/>
                                                </xsl:when>
                                                <xsl:otherwise>0</xsl:otherwise>
                                            </xsl:choose>
                                        </xsl:when>
                                        <xsl:when test="siemens/MEAS/sFastImaging/ucSegmentationMode = 1">
                                            <xsl:choose>
                                                <xsl:when test="siemens/MEAS/sFastImaging/lSegments &gt; 1">
                                                    <xsl:value-of select="ceiling((siemens/YAPS/iNoOfFourierPartitions * siemens/YAPS/iNoOfFourierLines) div siemens/MEAS/sFastImaging/lSegments)"/>
                                                </xsl:when>
                                                <xsl:otherwise>0</xsl:otherwise>
                                            </xsl:choose>
                                        </xsl:when>
                                        <xsl:otherwise>0</xsl:otherwise>
                                    </xsl:choose>
                                </xsl:when>
                                <xsl:otherwise>0</xsl:otherwise>
                            </xsl:choose>
                        </maximum>
                        <center>0</center>
                    </segment>
                    <contrast>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:choose>
                                <xsl:when test="siemens/MEAS/lContrasts">
                                    <xsl:value-of select="siemens/MEAS/lContrasts - 1"/>
                                </xsl:when>
                                <xsl:otherwise>0</xsl:otherwise>
                            </xsl:choose>
                        </maximum>
                        <center>0</center>
                    </contrast>
                    <average>
                        <minimum>0</minimum>
                        <maximum>
                            <xsl:choose>
                                <xsl:when test="siemens/MEAS/lAverages">
                                    <xsl:value-of select="siemens/MEAS/lAverages - 1"/>
                                </xsl:when>
                                <xsl:otherwise>0</xsl:otherwise>
                            </xsl:choose>
                        </maximum>
                        <center>0</center>
                    </average>
                </encodingLimits>
        <parallelImaging>
          <accelerationFactor>
                    <kspace_encoding_step_1>
              <xsl:choose>
            <xsl:when test="not(siemens/MEAS/sPat/lAccelFactPE)">1</xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="(siemens/MEAS/sPat/lAccelFactPE)"/>
            </xsl:otherwise>
              </xsl:choose>
                    </kspace_encoding_step_1>
                    <kspace_encoding_step_2>
              <xsl:choose>
            <xsl:when test="not(siemens/MEAS/sPat/lAccelFact3D)">1</xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="(siemens/MEAS/sPat/lAccelFact3D)"/>
            </xsl:otherwise>
              </xsl:choose>
                    </kspace_encoding_step_2>
          </accelerationFactor>
          <calibrationMode>
                    <xsl:choose>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 1">other</xsl:when>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 2">embedded</xsl:when>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 4">separate</xsl:when>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 8">separate</xsl:when>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 16">interleaved</xsl:when>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 32">interleaved</xsl:when>
              <xsl:when test="siemens/MEAS/sPat/ucRefScanMode = 64">interleaved</xsl:when>
              <xsl:otherwise>other</xsl:otherwise>
                    </xsl:choose>
          </calibrationMode>
          <xsl:if test="(siemens/MEAS/sPat/ucRefScanMode = 1) or (siemens/MEAS/sPat/ucRefScanMode = 16) or (siemens/MEAS/sPat/ucRefScanMode = 32) or (siemens/MEAS/sPat/ucRefScanMode = 64)">
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
            </encoding>

            <sequenceParameters>
                <xsl:for-each select="siemens/MEAS/alTR">
                     <xsl:if test="position() = 1">
                            <TR>
                                <xsl:value-of select=". div 1000.0" />
                            </TR>
                    </xsl:if>
                    <xsl:if test="(position() &gt; 1) and (. &gt; 0)">
                        <TR>
                            <xsl:value-of select=". div 1000.0" />
                        </TR>
                    </xsl:if>
                </xsl:for-each>
                <xsl:for-each select="siemens/MEAS/alTE">
                     <xsl:if test="position() = 1">
                            <TE>
                                <xsl:value-of select=". div 1000.0" />
                            </TE>
                    </xsl:if>
                    <xsl:if test="(position() &gt; 1) and (. &gt; 0)">
                        <xsl:if test="position() &lt; ($numberOfContrasts + 1)">
                            <TE>
                                <xsl:value-of select=". div 1000.0" />
                            </TE>
                        </xsl:if>
                    </xsl:if>
                </xsl:for-each>
                <xsl:for-each select="siemens/MEAS/alTI">
                    <xsl:if test=". &gt; 0">
                        <TI>
                            <xsl:value-of select=". div 1000.0" />
                        </TI>
                    </xsl:if>
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

            <userParameters>
                <xsl:if test="siemens/MEAS/sAngio/sFlowArray/lSize">
                    <userParameterLong>
                        <name>VENC_0</name>
                        <value>
                            <xsl:value-of select="siemens/MEAS/sAngio/sFlowArray/asElm/s0/nVelocity" />
                        </value>
                    </userParameterLong>
                </xsl:if>

                <xsl:if test="not(siemens/MEAS/sPhysioImaging/lSignal1 = 1) and not(siemens/MEAS/sPhysioImaging/lSignal1 = 16) and (siemens/MEAS/sPhysioImaging/lMethod1 = 8)">
                    <xsl:if test="siemens/MEAS/sFastImaging/lShots > 1">
                        <xsl:if test="siemens/MEAS/sPhysioImaging/lPhases > 1">
                            <xsl:if test="siemens/MEAS/sPhysioImaging/lRetroGatedImages > 0">
                                <userParameterLong>
                                    <name>RetroGatedImages</name>
                                    <value>
                                        <xsl:value-of select="siemens/MEAS/sPhysioImaging/lRetroGatedImages" />
                                    </value>
                                </userParameterLong>

                                <userParameterLong>
                                    <name>RetroGatedSegmentSize</name>
                                    <value>
                                        <xsl:choose>
                                            <xsl:when test="siemens/MEAS/sFastImaging/lSegments">
                                                <xsl:value-of select="siemens/MEAS/sFastImaging/lSegments"/>
                                            </xsl:when>
                                            <xsl:otherwise>0</xsl:otherwise>
                                        </xsl:choose>
                                    </value>
                                </userParameterLong>
                            </xsl:if>
                        </xsl:if>
                    </xsl:if>
                </xsl:if>

                <xsl:if test="(siemens/MEAS/ucOneSeriesForAllMeas = 2) or (siemens/MEAS/ucOneSeriesForAllMeas = 8)">
                    <userParameterLong>
                        <name>MultiSeriesForSlices</name>
                        <value>
                            <xsl:value-of select="siemens/MEAS/ucOneSeriesForAllMeas" />
                        </value>
                    </userParameterLong>
                </xsl:if>

                <xsl:if test="siemens/MEAS/sPat/lRefLinesPE">
                    <userParameterLong>
                        <name>EmbeddedRefLinesE1</name>
                        <value>
                            <xsl:value-of select="siemens/MEAS/sPat/lRefLinesPE" />
                        </value>
                    </userParameterLong>
                </xsl:if>

                <xsl:if test="siemens/MEAS/sPat/lRefLines3D">
                    <userParameterLong>
                        <name>EmbeddedRefLinesE2</name>
                        <value>
                            <xsl:value-of select="siemens/MEAS/sPat/lRefLines3D" />
                        </value>
                    </userParameterLong>
                </xsl:if>

                <xsl:if test="siemens/MEAS/lProtonDensMap">
                    <userParameterLong>
                        <name>NumOfProtonDensityImages</name>
                        <value>
                            <xsl:value-of select="siemens/MEAS/lProtonDensMap" />
                        </value>
                    </userParameterLong>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[1]">
                  <userParameterDouble>
                      <name>MaxwellCoefficient_0</name>
                      <value>
                          <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[1]" />
                      </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[3]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_1</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[2]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[3]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_2</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[3]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[4]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_3</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[4]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[5]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_4</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[5]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[6]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_5</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[6]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[7]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_6</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[7]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[8]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_7</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[8]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[9]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_8</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[9]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[10]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_9</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[10]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[11]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_10</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[11]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[12]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_11</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[12]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[13]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_12</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[13]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[14]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_13</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[14]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[15]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_14</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[15]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

                <xsl:if test="siemens/YAPS/aflMaxwellCoefficients[16]">
                  <userParameterDouble>
                    <name>MaxwellCoefficient_15</name>
                    <value>
                        <xsl:value-of select="siemens/YAPS/aflMaxwellCoefficients[16]" />
                    </value>
                  </userParameterDouble>
                </xsl:if>

            </userParameters>

        </ismrmrdHeader>
    </xsl:template>

</xsl:stylesheet>
