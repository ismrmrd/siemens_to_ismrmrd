/*
 * siemens_VD_to_HDF5.cpp
 *
 *  Created on: Feb 1, 2012
 *      Author: Michael S. Hansen
 *
 *  This application converts Siemens MRI VD line raw data files to
 *  convenient HDF5 file format. These HDF5 files can easily be
 *  read in Matlab, Python, C++, etc. without explicit knowledge
 *  of the Siemens raw data format.
 *
 */

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "siemensraw.h"
#include "siemens_hdf5_datatypes.h"

#include <H5Cpp.h>

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

void print_usage(const char* application)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "  - " << application << " <SIEMENS VD FILE NAME> " << "<HDF5 FILE NAME>" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        print_usage(argv[0]);
        return -1;
    }

    std::string infile(argv[1]);
    std::string outfile(argv[2]);

    std::cout << "Siemens MRI VD line converter:" << std::endl;
    std::cout << "  input file  :   " << infile << std::endl;
    std::cout << "  output file :   " << outfile << std::endl;


    //Get the HDF5 file opened. We will overwrite if the file exists
    H5File hdf5file(outfile, H5F_ACC_TRUNC);

    /* OK, let's open the file and start by checking
     * (as best as we can) that it is indeed a VD line file
     *
     */
    std::ifstream f(infile.c_str(), std::ios::in | std::ios::binary);

    MrParcRaidFileHeader ParcRaidHead;

    f.read(reinterpret_cast<char*>(&ParcRaidHead),sizeof(MrParcRaidFileHeader));

    bool VBFILE = false;

    if (ParcRaidHead.hdSize_ > 32)  {
        std::cout << "VB line file detected." << std::endl;
        VBFILE = true;
        f.seekg(0,std::ios::beg); //Rewind a bit, we have no raid file header.
        ParcRaidHead.hdSize_ = ParcRaidHead.count_;
        ParcRaidHead.count_ = 1;
    } else if (ParcRaidHead.hdSize_ != 0) { //This is a VB line data file
        std::cout << "MrParcRaidFileHeader.hdSize_ = " << ParcRaidHead.hdSize_ << std::endl;
        std::cout << "Only VD line files with MrParcRaidFileHeader.hdSize_ == 0 (MR_PARC_RAID_ALLDATA) supported." << std::endl;
        return -1;
    }

    std::cout << "This file contains " << ParcRaidHead.count_ << " measurement(s)." << std::endl;

    //Now lets head the Parc file entries for each of the included measurements.
    std::vector<MrParcRaidFileEntry> ParcFileEntries(64);

    unsigned int totalNumScan = 0;

    if (VBFILE)
    {
        //We are just going to fill these with zeros in this case. It doesn't exist.
        for (unsigned int i = 0; i < ParcFileEntries.size(); i++) {
            memset(&ParcFileEntries[i],0,sizeof(MrParcRaidFileEntry));
        }
        ParcFileEntries[0].off_ = 0;
        f.seekg(0,std::ios::end); //Rewind a bit, we have no raid file header.
        ParcFileEntries[0].len_ = f.tellg();
        f.seekg(0,std::ios::beg); //Rewind a bit, we have no raid file header.

        std::cout << " ParcFileEntries[0].len_ = " << ParcFileEntries[0].len_ << std::endl;

        // use a slow method to find file end
        uint32_t L_P;

        unsigned long long totalLen = 0;

        f.read(reinterpret_cast<char*>(&L_P),sizeof(uint32_t));
        f.seekg(L_P,std::ios::beg);
        f.seekg(7*4,std::ios::cur);

        short Samples;
        f.read(reinterpret_cast<char*>(&Samples),sizeof(uint16_t));
        f.seekg(128-30,std::ios::cur);
        f.seekg(8*Samples + 4,std::ios::cur);

        totalLen += L_P;
        totalLen += 128;
        totalLen += 8*Samples;

        unsigned long long totalScanNum = 0;
        unsigned int maxCha = Samples;
        try{
            while (!f.eof())
            {
                totalScanNum = totalScanNum + 1;
                f.seekg(7*4 - 4 ,std::ios::cur);
                f.read(reinterpret_cast<char*>(&Samples),sizeof(uint16_t));
                f.seekg(128-30,std::ios::cur);
                f.seekg(8*Samples + 4,std::ios::cur);
                totalLen += 128;
                totalLen += 8*Samples;
            }

            std::cout << "file end reached; total " << totalScanNum << " scans ... " << std::endl;
        }
        catch(...)
        {
            std::cout << "exception, file end reached; total " << totalScanNum << " scans ... " << std::endl;
        }

        f.close();
        f.open(infile.c_str(), std::ios::in | std::ios::binary);

        std::cout << "file end reached; total " << totalLen << " bytes ... " << std::endl;

        ParcFileEntries[0].len_ = totalLen;

        f.seekg(0,std::ios::beg);

    } else {
        for (unsigned int i = 0; i < ParcFileEntries.size(); i++) {
            f.read(reinterpret_cast<char*>(&ParcFileEntries[i]),sizeof(MrParcRaidFileEntry));
            if (i < ParcRaidHead.count_) {
                std::cout << "File id: " << ParcFileEntries[i].fileId_
                        << ", Meas id: " << ParcFileEntries[i].measId_
                        << ", Protocol name: " << ParcFileEntries[i].protName_ << std::endl;
            }
        }
    }

    //Let us write all the raid file headers to the root of the file
    try {
        std::vector<hsize_t> dims(1,1);
        DataSpace dspace( dims.size(), &dims[0], &dims[0]);

        boost::shared_ptr<DataType> headtype = getSiemensHDF5Type<MrParcRaidFileHeader>();
        DataSet dataset = hdf5file.createDataSet( "MrParcRaidFileHeader", *headtype, dspace);
        DataSpace mspace = dataset.getSpace();
        dataset.write(&ParcRaidHead, *headtype, mspace, dspace );

        headtype = getSiemensHDF5Type<MrParcRaidFileEntry>();
        dims[0] = 64; dims.push_back(1);
        dspace = DataSpace( dims.size(), &dims[0], &dims[0]);
        dataset = hdf5file.createDataSet( "MrParcRaidFileEntries", *headtype, dspace);
        mspace = dataset.getSpace();
        dataset.write(&ParcFileEntries[0], *headtype, mspace, dspace );

    } catch ( ... ) {
        std::cout << "Error writing headers to the root of the HDF5 file" << std::endl;
        return -1;
    }


    std::stringstream str;
    str << "/files";
    try {
        hdf5file.createGroup(str.str());
    } catch ( FileIException& e) {
        std::cout << "Error creating basic group in HDF5 file." << std::endl;
        return -1;
    }

    //Now we can loop over the measurements/files
    for (unsigned int i = 0; i < ParcRaidHead.count_; i++) {
        str.clear();
        str.str("");
        str << "/files/" << i;

        try {
            hdf5file.createGroup(str.str());
        } catch ( FileIException& e) {
            std::cout << "Error creating group for file " << i
                    << " in HDF5 file."
                    << "(" << str.str() << ")" << std::endl;
            return -1;
        }

        f.seekg(ParcFileEntries[i].off_, std::ios::beg);

        MeasurementHeader mhead;
        f.read(reinterpret_cast<char*>(&mhead.dma_length),sizeof(uint32_t));

        f.read(reinterpret_cast<char*>(&mhead.nr_buffers),sizeof(uint32_t));

        //Now allocate dynamic memory for the buffers
        mhead.buffers.len = mhead.nr_buffers;
        MeasurementHeaderBuffer* buffers = new MeasurementHeaderBuffer[mhead.nr_buffers];
        mhead.buffers.p = reinterpret_cast<void*>(buffers);

        std::cout << "Number of parameter buffers: " << mhead.nr_buffers << std::endl;


        char bufname_tmp[32];
        for (unsigned int b = 0; b < mhead.nr_buffers; b++) {
            f.getline(bufname_tmp,32,'\0');
            buffers[b].bufName_.len = f.gcount() + 1;
            bufname_tmp[f.gcount()] = '\0';
            buffers[b].bufName_.p = reinterpret_cast<void*>(new char[buffers[b].bufName_.len]);
            memcpy(buffers[b].bufName_.p, bufname_tmp, buffers[b].bufName_.len);

            //std::cout << "Buffer name: " << bufname_tmp << std::endl;

            f.read(reinterpret_cast<char*>(&buffers[b].bufLen_),sizeof(uint32_t));
            buffers[b].buf_.len = buffers[b].bufLen_;
            //std::cout << "buffers[b].buf_.len: " << buffers[b].buf_.len<< std::endl;
            buffers[b].buf_.p = reinterpret_cast<void*>(new char[buffers[b].buf_.len]);
            f.read(reinterpret_cast<char*>(buffers[b].buf_.p),buffers[b].buf_.len);
        }

        try {
            //Let's write the buffers to the HDF5 file.
            std::vector<hsize_t> dims(1,1);
            DataSpace dspace( dims.size(), &dims[0], &dims[0]); //There will only be one of these

            str << "/MeasurementHeader";
            boost::shared_ptr<DataType> headtype = getSiemensHDF5Type<MeasurementHeader>();
            DataSet dataset = hdf5file.createDataSet( str.str(), *headtype, dspace);
            DataSpace mspace = dataset.getSpace();

            dataset.write(&mhead, *headtype, mspace, dspace );
        } catch ( ... ) {
            std::cout << "Error caught while trying to write header buffers to HDF5 file" << std::endl;
            return -1;
        }

        ClearMeasurementHeader(&mhead);

        //We need to be on a 32 byte boundary after reading the buffers
        long long int position_in_meas = static_cast<long long int>(f.tellg())-ParcFileEntries[i].off_;
        if (position_in_meas % 32) {
            f.seekg(32-(position_in_meas % 32), std::ios::cur);
        }

        //Now we can read data from the Siemens file and append to the HDF5 file.
        str.clear();
        str.str("");
        str << "/files/" << i << "/data";

        std::vector<hsize_t> dims(2,1);
        std::vector<hsize_t> ddims(2,1);
        std::vector<hsize_t> max_dims(2,1);
        max_dims[0] = -1;

        //Create variable for the data
        DataSpace mspace1;
        DataSpace mspace2;
        DataSet dataset;
        boost::shared_ptr<DataType> datatype = getSiemensHDF5Type<sScanHeader_with_data>();

        try {
            mspace1 = DataSpace( dims.size(), &dims[0], &max_dims[0]);

            DSetCreatPropList cparms;
            cparms.setChunk( dims.size(), &dims[0] );


            dataset = hdf5file.createDataSet( str.str(), *datatype, mspace1, cparms);
            mspace1 = dataset.getSpace();

            mspace2 = DataSpace( dims.size(), &dims[0] );
        } catch ( ... ) {
            std::cout << "Error creating variable for measurment data in file" << std::endl;
            return -1;
        }

        std::vector<hsize_t> offset(dims.size(),0);

        //Preparations for syncdata variable. We will not create it now, because it may not be needed.
        DataSpace sync_mspace1;
        DataSpace sync_mspace2;
        DataSet sync_dataset;
        boost::shared_ptr<DataType> sync_datatype = getSiemensHDF5Type<sScanHeader_with_syncdata>();

        std::vector<hsize_t> sync_dims(2,1);
        std::vector<hsize_t> sync_ddims(2,1);
        std::vector<hsize_t> sync_max_dims(2,1);
        sync_max_dims[0] = -1;
        std::vector<hsize_t> sync_offset(sync_dims.size(),0);
        //End of variables for syncdata

        uint32_t last_mask = 0;
        unsigned long int acquisitions = 1;
        unsigned long int sync_data_packets = 0;
        sMDH mdh;//For VB line
        while (!(last_mask & 1) && //Last scan not encountered
                (((ParcFileEntries[i].off_+ ParcFileEntries[i].len_)-f.tellg()) > sizeof(sScanHeader)))  //not reached end of measurement without acqend
        {
            size_t position_in_meas = f.tellg();
            sScanHeader_with_data scanhead;
            f.read(reinterpret_cast<char*>(&scanhead.scanHeader.ulFlagsAndDMALength), sizeof(uint32_t));

            if ( mdh.ulScanCounter%1000 == 0 )
            {
                std::cout << " mdh.ulScanCounter = " <<  mdh.ulScanCounter << std::endl;
            }

            if (VBFILE) {
                f.read(reinterpret_cast<char*>(&mdh)+sizeof(uint32_t), sizeof(sMDH)-sizeof(uint32_t));
                scanhead.scanHeader.lMeasUID = mdh.lMeasUID;
                scanhead.scanHeader.ulScanCounter = mdh.ulScanCounter;
                scanhead.scanHeader.ulTimeStamp = mdh.ulTimeStamp;
                scanhead.scanHeader.ulPMUTimeStamp = mdh.ulPMUTimeStamp;
                scanhead.scanHeader.ushSystemType = 0;
                scanhead.scanHeader.ulPTABPosDelay = 0;
                scanhead.scanHeader.lPTABPosX = 0;
                scanhead.scanHeader.lPTABPosY = 0;
                scanhead.scanHeader.lPTABPosZ = mdh.ushPTABPosNeg;//TODO: Modify calculation
                scanhead.scanHeader.ulReserved1 = 0;
                scanhead.scanHeader.aulEvalInfoMask[0] = mdh.aulEvalInfoMask[0];
                scanhead.scanHeader.aulEvalInfoMask[1] = mdh.aulEvalInfoMask[1];
                scanhead.scanHeader.ushSamplesInScan = mdh.ushSamplesInScan;
                scanhead.scanHeader.ushUsedChannels = mdh.ushUsedChannels;
                scanhead.scanHeader.sLC = mdh.sLC;
                scanhead.scanHeader.sCutOff = mdh.sCutOff;
                scanhead.scanHeader.ushKSpaceCentreColumn = mdh.ushKSpaceCentreColumn;
                scanhead.scanHeader.ushCoilSelect = mdh.ushCoilSelect;
                scanhead.scanHeader.fReadOutOffcentre = mdh.fReadOutOffcentre;
                scanhead.scanHeader.ulTimeSinceLastRF = mdh.ulTimeSinceLastRF;
                scanhead.scanHeader.ushKSpaceCentreLineNo = mdh.ushKSpaceCentreLineNo;
                scanhead.scanHeader.ushKSpaceCentrePartitionNo = mdh.ushKSpaceCentrePartitionNo;
                scanhead.scanHeader.sSliceData = mdh.sSliceData;
                memset(scanhead.scanHeader.aushIceProgramPara,0,sizeof(uint16_t)*24);
                memcpy(scanhead.scanHeader.aushIceProgramPara,mdh.aushIceProgramPara,8*sizeof(uint16_t));
                memset(scanhead.scanHeader.aushReservedPara,0,sizeof(uint16_t)*4);
                scanhead.scanHeader.ushApplicationCounter = 0;
                scanhead.scanHeader.ushApplicationMask = 0;
                scanhead.scanHeader.ulCRC = 0;
            } else {
                f.read(reinterpret_cast<char*>(&scanhead.scanHeader) + sizeof(uint32_t),sizeof(sScanHeader)-sizeof(uint32_t));
            }

            uint32_t dma_length = scanhead.scanHeader.ulFlagsAndDMALength & MDH_DMA_LENGTH_MASK;
            uint32_t mdh_enable_flags = scanhead.scanHeader.ulFlagsAndDMALength & MDH_ENABLE_FLAGS_MASK;


            if (scanhead.scanHeader.aulEvalInfoMask[0] & ( 1 << 5)) { //Check is this is synch data, if so, it must be handled differently.
                //This is synch data.

                sScanHeader_with_syncdata synch_data;
                synch_data.scanHeader = scanhead.scanHeader;
                synch_data.last_scan_counter = acquisitions-1;

                if (VBFILE) {
                    synch_data.syncdata.len = dma_length-sizeof(sMDH);
                                } else {
                    synch_data.syncdata.len = dma_length-sizeof(sScanHeader);
                                 }
                std::vector<uint8_t> synchdatabytes(synch_data.syncdata.len);
                synch_data.syncdata.p = &synchdatabytes[0];
                f.read(reinterpret_cast<char*>(&synchdatabytes[0]), synch_data.syncdata.len);

                if (!sync_data_packets) { //This is the first, let's create the variable
                    try {
                        sync_mspace1 = DataSpace( sync_dims.size(), &sync_dims[0], &sync_max_dims[0]);

                        DSetCreatPropList sync_cparms;
                        sync_cparms.setChunk( sync_dims.size(), &sync_dims[0] );

                        str.clear();
                        str.str("");
                        str << "/files/" << i << "/syncdata";

                        sync_dataset = hdf5file.createDataSet( str.str(), *sync_datatype, sync_mspace1, sync_cparms);
                        sync_mspace1 = dataset.getSpace();

                        sync_mspace2 = DataSpace( sync_dims.size(), &sync_dims[0] );
                    } catch ( ... ) {
                        std::cout << "Error creating variable for sync data in file" << std::endl;
                        return -1;
                    }
                }
                sync_data_packets++;

                if (sync_ddims[0] != sync_data_packets) {
                    sync_ddims[0] = sync_data_packets;
                    sync_dataset.extend(&sync_ddims[0]);
                }
                sync_mspace1 = sync_dataset.getSpace();
                sync_offset[0] = sync_data_packets-1;

                try {
                    sync_mspace1.selectHyperslab(H5S_SELECT_SET, &sync_dims[0], &sync_offset[0]);
                    sync_dataset.write( &synch_data, *sync_datatype, sync_mspace2, sync_mspace1 );
                } catch ( ... ) {
                    std::cout << "Error appending syncdata to HDF5 file" << std::endl;
                    return -1;
                }

                continue;
            }

                //This check only makes sense in VD line files.
            if (!VBFILE && (scanhead.scanHeader.lMeasUID != ParcFileEntries[i].measId_)) {
                //Something must have gone terribly wrong. Bail out.
                std::cout << "Corrupted dataset detected (scanhead.scanHeader.lMeasUID != ParcFileEntries[i].measId_)" << std::endl;
                std::cout << "Bailing out" << std::endl;
                return -1;
            }

            //Allocate data for channels
            scanhead.data.len = scanhead.scanHeader.ushUsedChannels;
            sChannelHeader_with_data* chan = new sChannelHeader_with_data[scanhead.data.len];
            scanhead.data.p = reinterpret_cast<void*>(chan);

            for (unsigned int c = 0; c < scanhead.scanHeader.ushUsedChannels; c++) {
                if (VBFILE) {
                    if (c > 0) {
                        f.read(reinterpret_cast<char*>(&mdh),sizeof(sMDH));
                    }
                    chan[c].channelHeader.ulTypeAndChannelLength = 0;
                    chan[c].channelHeader.lMeasUID = mdh.lMeasUID;
                    chan[c].channelHeader.ulScanCounter = mdh.ulScanCounter;
                    chan[c].channelHeader.ulReserved1 = 0;
                    chan[c].channelHeader.ulSequenceTime = 0;
                    chan[c].channelHeader.ulUnused2 = 0;
                    chan[c].channelHeader.ulChannelId = mdh.ushChannelId;
                    chan[c].channelHeader.ulUnused3 = 0;
                    chan[c].channelHeader.ulCRC = 0;
                } else {
                    f.read(reinterpret_cast<char*>(&chan[c].channelHeader),sizeof(sChannelHeader));
                }
                chan[c].data.len = scanhead.scanHeader.ushSamplesInScan;
                chan[c].data.p = reinterpret_cast<void*>(new complex_t[chan[c].data.len]);
                f.read(reinterpret_cast<char*>(chan[c].data.p ),chan[c].data.len*sizeof(complex_t));
            }

            if (ddims[0] != acquisitions) {
                ddims[0] = acquisitions;
                dataset.extend(&ddims[0]);
            }
            mspace1 = dataset.getSpace();
            offset[0] = acquisitions-1;

            try {
                mspace1.selectHyperslab(H5S_SELECT_SET, &dims[0], &offset[0]);
                dataset.write( &scanhead, *datatype, mspace2, mspace1 );
            } catch ( ... ) {
                std::cout << "Error appending measurement data to HDF5 file" << std::endl;
                return -1;
            }
            ClearsScanHeader_with_data(&scanhead);
            acquisitions++;
            last_mask = scanhead.scanHeader.aulEvalInfoMask[0];
        }

        std::cout << " final mdh.ulScanCounter = " <<  mdh.ulScanCounter << " - last_mask : " << last_mask << std::endl;

        //Mystery bytes. There seems to be 160 mystery bytes at the end of the data.
        //We will store them in the HDF file in case we need them for creating a binary
        //identical dat file.
        unsigned int mystery_bytes = (ParcFileEntries[i].off_+ParcFileEntries[i].len_)-f.tellg();

        if (mystery_bytes > 0) {
            if (mystery_bytes != 160) {
                std::cout << "WARNING: An unexpected number of mystery bytes detected: " << mystery_bytes << std::endl;
                std::cout << "ParcFileEntries[i].off_ = " << ParcFileEntries[i].off_ << std::endl;
                std::cout << "ParcFileEntries[i].len_ = " << ParcFileEntries[i].len_ << std::endl;
                std::cout << "f.tellg() = " << f.tellg() << std::endl;
                return -1;
            }

            try {
                hsize_t mystery_dims[] = {1};

                DataSpace mysspace = DataSpace(1, mystery_dims, mystery_dims);

                hsize_t mystery_array_dims[1];
                mystery_array_dims[0] = mystery_bytes;
                DataType mysarraytype = ArrayType(PredType::NATIVE_CHAR, 1, mystery_array_dims);

                char* mystery_data = new char[mystery_bytes];
                f.read(mystery_data,mystery_bytes);

                str.clear();
                str.str("");
                str << "/files/" << i << "/mysteryBytesPost";

                DataSet mysdataset = hdf5file.createDataSet( str.str(), mysarraytype, mysspace);
                mysdataset.write( mystery_data, mysarraytype, mysspace);
                delete [] mystery_data;
            } catch (...) {
                std::cout << "Error caught while writing mystery bytes to HDF5 file." << std::endl;
                return -1;
            }


            //After this we have to be on a 512 byte boundary
            if (f.tellg() % 512) {
                f.seekg(512-(f.tellg() % 512), std::ios::cur);
            }
        }
    }

    size_t end_position = f.tellg();
    f.seekg(0,std::ios::end);
    size_t eof_position = f.tellg();
    if (end_position != eof_position) {
        size_t additional_bytes = eof_position-end_position;
        std::cout << "WARNING: End of file was not reached during conversion. There are "
                << additional_bytes << " additional bytes at the end of file." << std::endl;
    }

    f.close();
    hdf5file.close();

    std::cout << "Data conversion complete." << std::endl;
    return 0;

}



