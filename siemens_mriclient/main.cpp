#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_string.h"
#include "ace/Reactor.h"

#include "GadgetMessageInterface.h"
#include "GadgetMRIHeaders.h"
#include "siemensraw.h"
#include "GadgetronConnector.h"
#include "GadgetSocketSender.h" //We need this for now for the GadgetAcquisitionWriter
#include "ImageWriter.h"
#include "hoNDArray.h"
#include "GadgetXml.h"
#include "XNode.h"
#include "FileInfo.h"

#include "hdf5_core.h"
#include "mri_hdf5_io.h"
#include "hoNDArray_hdf5_io.h"
#include "siemens_hdf5_datatypes.h"
#include "HDF5ImageWriter.h"

#include <H5Cpp.h>

#include <iomanip>

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

std::string get_date_time_string()
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );


	std::stringstream str;
	str << timeinfo->tm_year+1900 << "-"
			<< std::setw(2) << std::setfill('0') << timeinfo->tm_mon+1
			<< "-"
			<< std::setw(2) << std::setfill('0') << timeinfo->tm_mday
			<< " "
			<< std::setw(2) << std::setfill('0') << timeinfo->tm_hour
			<< ":"
			<< std::setw(2) << std::setfill('0') << timeinfo->tm_min
			<< ":"
			<< std::setw(2) << std::setfill('0') << timeinfo->tm_sec;

	std::string ret = str.str();

	return ret;
}


bool is_number(const std::string& s)
{
	bool ret = true;
	for (unsigned int i = 0; i < s.size(); i++) {
		if (!std::isdigit(s.c_str()[i])) {
			ret = false;
			break;
		}
	}
	return ret;
}

std::string ProcessGadgetronParameterMap(const XProtocol::XNode& node, std::string mapfilename)
{

	TiXmlDocument out_doc;

	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );
	out_doc.LinkEndChild( decl );


	GadgetXMLNode out_n(&out_doc);

	//Input Document
	TiXmlDocument doc(mapfilename.c_str());
	doc.LoadFile();
	TiXmlHandle docHandle(&doc);


	TiXmlElement* parameters = docHandle.FirstChildElement("gadgetron").FirstChildElement("parameters").ToElement();
	if (parameters) {
		TiXmlNode* p = 0;
		while( p = parameters->IterateChildren( "p",  p ) ) {
			TiXmlHandle ph(p);

			TiXmlText* s = ph.FirstChildElement("s").FirstChild().ToText();
			TiXmlText* d = ph.FirstChildElement("d").FirstChild().ToText();

			if (s && d) {
				std::string source      = s->Value();
				std::string destination = d->Value();

				std::vector<std::string> split_path;
				boost::split( split_path, source, boost::is_any_of("."), boost::token_compress_on );

				if (is_number(split_path[0])) {
					std::cout << "First element of path (" << source << ") cannot be numeric" << std::endl;
					continue;
				}

				std::string search_path = split_path[0];
				for (unsigned int i = 1; i < split_path.size()-1; i++) {
					/*
					if (is_number(split_path[i]) && (i != split_path.size())) {
						std::cout << "Numeric index not supported inside path for source = " << source << std::endl;
						continue;
					}*/

					search_path += std::string(".") + split_path[i];
				}

				int index = -1;
				if (is_number(split_path[split_path.size()-1])) {
					index = atoi(split_path[split_path.size()-1].c_str());
				} else {
					search_path += std::string(".") + split_path[split_path.size()-1];
				}

				//std::cout << "search_path: " << search_path << std::endl;

				const XProtocol::XNode* n = boost::apply_visitor(XProtocol::getChildNodeByName(search_path), node);

				std::vector<std::string> parameters;
				if (n) {
					parameters = boost::apply_visitor(XProtocol::getStringValueArray(), *n);
				} else {
					std::cout << "Search path: " << search_path << " not found." << std::endl;
				}

				if (index >= 0) {
					if (parameters.size() > index) {
						out_n.add(destination, parameters[index]);
					} else {
						std::cout << "Parameter index (" << index << ") not valid for search path " << search_path << std::endl;
						continue;
					}
				} else {
					out_n.add(destination, parameters);
				}
			} else {
				ACE_DEBUG(( LM_ERROR, ACE_TEXT("Malformed Gadgetron parameter map\n") ));
			}
		}

	} else {
		ACE_DEBUG(( LM_ERROR, ACE_TEXT("Malformed Gadgetron parameter map (parameters section not found)\n") ));
		return std::string("");
	}


	return XmlToString(out_doc);
}

void print_usage() 
{
	ACE_DEBUG((LM_INFO, ACE_TEXT("Usage: \n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("siemens_mriclient -p <PORT>                      (default 9002)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -h <HOST>                      (default localhost)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -f <HDF5 DATA FILE>            (default ./data.h5)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -d <HDF5 DATASET NUMBER>       (default 0)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -m <PARAMETER MAP FILE>        (default ./parammap.xml)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -o <HDF5 dump file>            (default dump.h5)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -g <HDF5 dump group>           (/dataset)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -r <HDF5 result file>          (default result.h5)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -G <HDF5 result group>         (default date and time)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -c <GADGETRON CONFIG>          (default default.xml)\n") ));
	ACE_DEBUG((LM_INFO, ACE_TEXT("                  -w                             (write only flag, do not connect to Gadgetron)\n") ));
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[] )
{
	static const ACE_TCHAR options[] = ACE_TEXT(":p:h:f:d:o:c:m:g:r:G:w");

	ACE_Get_Opt cmd_opts(argc, argv, options);

	ACE_TCHAR port_no[1024];
	ACE_OS_String::strncpy(port_no, "9002", 1024);

	ACE_TCHAR hostname[1024];
	ACE_OS_String::strncpy(hostname, "localhost", 1024);

	ACE_TCHAR filename[4096];
	ACE_OS_String::strncpy(filename, "./data.h5", 4096);

	ACE_TCHAR config_file[1024];
	ACE_OS_String::strncpy(config_file, "default.xml", 1024);

	ACE_TCHAR parammap_file[1024];
	ACE_OS_String::strncpy(parammap_file, "parammap.xml", 1024);

	ACE_TCHAR hdf5_file[1024];
	ACE_OS_String::strncpy(hdf5_file, "dump.h5", 1024);

	ACE_TCHAR hdf5_group[1024];
	ACE_OS_String::strncpy(hdf5_group, "dataset", 1024);

	ACE_TCHAR hdf5_out_file[1024];
	ACE_OS_String::strncpy(hdf5_out_file, "./result.h5", 1024);

	ACE_TCHAR hdf5_out_group[1024];

	std::string date_time = get_date_time_string();

	ACE_OS_String::strncpy(hdf5_out_group, date_time.c_str(), 1024);

	unsigned int hdf5_dataset_no = 0;

	bool write_to_file = false;
	bool write_to_file_only = false;

	int option;
	while ((option = cmd_opts()) != EOF) {
		switch (option) {
		case 'p':
			ACE_OS_String::strncpy(port_no, cmd_opts.opt_arg(), 1024);
			break;
		case 'h':
			ACE_OS_String::strncpy(hostname, cmd_opts.opt_arg(), 1024);
			break;
		case 'f':
			ACE_OS_String::strncpy(filename, cmd_opts.opt_arg(), 4096);
			break;
		case 'd':
			hdf5_dataset_no = atoi(cmd_opts.opt_arg());;
			break;
		case 'm':
			ACE_OS_String::strncpy(parammap_file, cmd_opts.opt_arg(), 1024);
			break;
		case 'c':
			ACE_OS_String::strncpy(config_file, cmd_opts.opt_arg(), 1024);
			break;
		case 'o':
			ACE_OS_String::strncpy(hdf5_file, cmd_opts.opt_arg(), 1024);
			write_to_file = true;
			break;
		case 'g':
			ACE_OS_String::strncpy(hdf5_group, cmd_opts.opt_arg(), 1024);
			write_to_file = true;
			break;
		case 'w':
			write_to_file_only = true;
			break;
		case 'r':
			ACE_OS_String::strncpy(hdf5_out_file, cmd_opts.opt_arg(), 1024);
			break;
		case 'G':
			ACE_OS_String::strncpy(hdf5_out_group, cmd_opts.opt_arg(), 1024);
			break;
		case ':':
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("-%c requires an argument.\n"), cmd_opts.opt_opt()),-1);
			break;
		default:
			ACE_ERROR_RETURN( (LM_ERROR, ACE_TEXT("Command line parse error\n")), -1);
			break;
		}
	}

	ACE_DEBUG(( LM_INFO, ACE_TEXT("Siemens MRI Client\n") ));

	if (!FileInfo(std::string(filename)).exists()) {
		ACE_DEBUG((LM_INFO, ACE_TEXT("Data file %s does not exist.\n"), filename));
		print_usage();
		return -1;
	}

	if (!FileInfo(std::string(parammap_file)).exists()) {
		ACE_DEBUG((LM_INFO, ACE_TEXT("Parameter map file %s does not exist.\n"), parammap_file));
		print_usage();
		return -1;
	}

	if (!write_to_file_only) {
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- host          :            %s\n"), hostname));
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- port          :            %s\n"), port_no));
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- config        :            %s\n"), config_file));
	}
	ACE_DEBUG((LM_INFO, ACE_TEXT("  -- data          :            %s\n"), filename));
	ACE_DEBUG((LM_INFO, ACE_TEXT("  -- parameter map :            %s\n"), parammap_file));

	if (write_to_file) {
		if (FileInfo(std::string(filename)).exists()) {
			boost::shared_ptr<H5File> f = OpenHDF5File(hdf5_file);
			if (HDF5LinkExists(f.get(), hdf5_group)) {
				ACE_DEBUG((LM_INFO, ACE_TEXT("HDF5 group \"%s\" already exists.\n"), hdf5_group));
				print_usage();
				return -1;
			}
		}
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- output file   :            %s\n"), hdf5_file));
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- output group  :            %s\n"), hdf5_group));
	}


	//Get the HDF5 file opened.
	H5File hdf5file;
	MeasurementHeader mhead;
	{

		HDF5Exclusive lock; //This will ensure threadsafe access to HDF5

		try {
			hdf5file = H5File(filename, H5F_ACC_RDONLY);

			std::stringstream str;
			str << "/files/" << hdf5_dataset_no << "/MeasurementHeader";

			DataSet headds = hdf5file.openDataSet(str.str());
			DataType dtype = headds.getDataType();
			DataSpace headspace = headds.getSpace();

			boost::shared_ptr<DataType> headtype = getSiemensHDF5Type<MeasurementHeader>();
			if (!(dtype == *headtype)) {
				std::cout << "Wrong datatype for MeasurementHeader detected." << std::endl;
				return -1;
			}


			headds.read(&mhead, *headtype, headspace, headspace);

			std::cout << "mhead.nr_buffers = " << mhead.nr_buffers << std::endl;
		} catch (...) {
			std::cout << "Error opening HDF5 file and reading dataset header." << std::endl;
			return -1;
	}

	}
	//Now we should have the measurement headers, so let's use the Meas header to create the gadgetron XML parameters
	MeasurementHeaderBuffer* buffers = reinterpret_cast<MeasurementHeaderBuffer*>(mhead.buffers.p);

	std::string xml_config;
	for (unsigned int b = 0; b < mhead.nr_buffers; b++) {
		if (std::string((char*)buffers[b].bufName_.p).compare("Meas") == 0) {
			std::string config_buffer((char*)buffers[b].buf_.p,buffers[b].buf_.len-2);//-2 because the last two character are ^@

			XProtocol::XNode n;

			if (XProtocol::ParseXProtocol(const_cast<std::string&>(config_buffer),n) < 0) {
				ACE_DEBUG((LM_ERROR, ACE_TEXT("Failed to parse XProtocol")));
				return -1;
			}

			xml_config = ProcessGadgetronParameterMap(n,parammap_file);
			break;
		}
	}

	//Get rid of dynamically allocated memory in header
	{
		HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
		ClearMeasurementHeader(&mhead);
	}

	//std::cout << xml_config << std::endl;


	GadgetMessageAcquisition acq_head_base;
	memset(&acq_head_base, 0, sizeof(GadgetMessageAcquisition) );

	if (write_to_file) {
		std::string hdf5filename(hdf5_file);
		std::string hdf5xmlvar = std::string(hdf5_group) + std::string("/xml");
		std::vector<unsigned int> xmldims(1,xml_config.length()+1); //+1 because of null termination
		hoNDArray<char> tmp;
		tmp.create(&xmldims);
		memcpy(tmp.get_data_ptr(),xml_config.c_str(),tmp.get_number_of_elements());

		{
			HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
			hdf5_append_array(&tmp,hdf5filename.c_str(),hdf5xmlvar.c_str());
		}
	}


	GadgetronConnector con;
	if (!write_to_file_only) {

		con.register_writer(GADGET_MESSAGE_ACQUISITION, new GadgetAcquisitionMessageWriter());
		con.register_reader(GADGET_MESSAGE_IMAGE_REAL_USHORT, new HDF5ImageWriter<ACE_UINT16>(std::string(hdf5_out_file), std::string(hdf5_out_group)));
		con.register_reader(GADGET_MESSAGE_IMAGE_REAL_FLOAT, new HDF5ImageWriter<float>(std::string(hdf5_out_file), std::string(hdf5_out_group)));
		con.register_reader(GADGET_MESSAGE_IMAGE_CPLX_FLOAT, new HDF5ImageWriter< std::complex<float> >(std::string(hdf5_out_file), std::string(hdf5_out_group)));

		//Open a connection with the gadgetron
		if (con.open(std::string(hostname),std::string(port_no)) != 0) {
			ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to connect to the Gadgetron host")));
			return -1;
		}

		//Tell Gadgetron which XML configuration to run.
		if (con.send_gadgetron_configuration_file(std::string(config_file)) != 0) {
			ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to send XML configuration to the Gadgetron host")));
			return -1;
		}

		if (con.send_gadgetron_parameters(xml_config) != 0) {
			ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to send XML parameters to the Gadgetron host")));
			return -1;
		}
	}


	//Let's figure out how many acquisitions we have
	DataSet rawdataset;
	DataSpace rawspace;
	std::vector<hsize_t> raw_dimensions;
	std::vector<hsize_t> single_scan_dims;
	std::vector<hsize_t> offset;
	boost::shared_ptr<DataType> rawdatatype = getSiemensHDF5Type<sScanHeader_with_data>();
	unsigned long int acquisitions = 0;
	try {
		std::stringstream str;
		str << "/files/" << hdf5_dataset_no << "/data";

		HDF5Exclusive lock; //This will ensure threadsafe access to HDF5

		rawdataset = hdf5file.openDataSet(str.str());

		DataType dtype = rawdataset.getDataType();
		if (!(dtype == *rawdatatype)) {
			std::cout << "Wrong datatype detected in HDF5 file" << std::endl;
			return -1;
		}

		rawspace = rawdataset.getSpace();

		int rank = rawspace.getSimpleExtentNdims();
		raw_dimensions.resize(rank);
		single_scan_dims.resize(rank,1);
		offset.resize(rank,0);

		rawspace.getSimpleExtentDims(&raw_dimensions[0]);

		if (rank != 2) {
			std::cout << "Wrong number of dimensions (" << rank << ") detected in dataset" << std::endl;
			return -1;
		}

		acquisitions = raw_dimensions[0];


	} catch ( ... ) {
		std::cout << "Error accessing data variable for raw dataset" << std::endl;
		return -1;
	}

	for (unsigned int a = 0; a < acquisitions; a++) {
		sScanHeader_with_data scanhead;

		offset[0] = a;

		{
			HDF5Exclusive lock; //This will ensure threadsafe access to HDF5

			DataSpace space = rawdataset.getSpace();
			space.selectHyperslab(H5S_SELECT_SET, &single_scan_dims[0], &offset[0]);

			DataSpace memspace(2,&single_scan_dims[0]);

			rawdataset.read( &scanhead, *rawdatatype, memspace, space);
		}

		if (scanhead.scanHeader.aulEvalInfoMask[0] & 1) {
			std::cout << "Last scan reached..." << std::endl;
			HDF5Exclusive lock;
			ClearsScanHeader_with_data(&scanhead);
			break;
		}

		GadgetContainerMessage<GadgetMessageIdentifier>* m1 =
				new GadgetContainerMessage<GadgetMessageIdentifier>();

		m1->getObjectPtr()->id = GADGET_MESSAGE_ACQUISITION;

		GadgetContainerMessage<GadgetMessageAcquisition>* m2 =
				new GadgetContainerMessage<GadgetMessageAcquisition>();

		GadgetMessageAcquisition* acq_head = m2->getObjectPtr();
		acq_head->flags = 0;
		acq_head->idx.line                 = scanhead.scanHeader.sLC.ushLine;
		acq_head->idx.acquisition          = scanhead.scanHeader.ulScanCounter;
		acq_head->idx.slice                = scanhead.scanHeader.sLC.ushSlice;
		acq_head->idx.partition            = scanhead.scanHeader.sLC.ushPartition;
		acq_head->idx.echo                 = scanhead.scanHeader.sLC.ushEcho;
		acq_head->idx.phase                = scanhead.scanHeader.sLC.ushPhase;
		acq_head->idx.repetition           = scanhead.scanHeader.sLC.ushRepetition;
		acq_head->idx.set                  = scanhead.scanHeader.sLC.ushSet;
		acq_head->idx.segment              = scanhead.scanHeader.sLC.ushSeg;
		acq_head->idx.channel              = 0;//scanhead.scanHeader.ushChannelId;

		acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 25)) ?
						GADGET_FLAG_IS_NOISE_SCAN : 0;

		acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 29)) ?
						GADGET_FLAG_LAST_ACQ_IN_SLICE : 0;

		acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 28)) ?
						GADGET_FLAG_FIRST_ACQ_IN_SLICE : 0;

        acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 22)) ?
						GADGET_FLAG_IS_PATREF_SCAN : 0;

        acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 23)) ?
						GADGET_FLAG_IS_PATREFANDIMA_SCAN : 0;

        acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 8)) ?
						GADGET_FLAG_LAST_ACQ_IN_CONCAT : 0;

        acq_head->flags |=
				(scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 11)) ?
						GADGET_FLAG_LAST_ACQ_IN_MEAS : 0;

		acq_head->meas_uid                 = scanhead.scanHeader.lMeasUID;
		acq_head->scan_counter             = scanhead.scanHeader.ulScanCounter;
		acq_head->time_stamp               = scanhead.scanHeader.ulTimeStamp;
        acq_head->pmu_time_stamp           = scanhead.scanHeader.ulPMUTimeStamp;
		acq_head->samples                  = scanhead.scanHeader.ushSamplesInScan;
		acq_head->channels                 = scanhead.scanHeader.ushUsedChannels;
	    acq_head->centre_column            = scanhead.scanHeader.ushKSpaceCentreColumn;
		acq_head->position[0]              = scanhead.scanHeader.sSliceData.sSlicePosVec.flSag;
		acq_head->position[1]              = scanhead.scanHeader.sSliceData.sSlicePosVec.flCor;
		acq_head->position[2]              = scanhead.scanHeader.sSliceData.sSlicePosVec.flTra;

		memcpy(acq_head->quaternion,
				scanhead.scanHeader.sSliceData.aflQuaternion,
				sizeof(float)*4);

		std::vector<unsigned int> dimensions(2);
		dimensions[0] = m2->getObjectPtr()->samples;
		dimensions[1] = m2->getObjectPtr()->channels;

		GadgetContainerMessage< hoNDArray< std::complex<float> > >* m3 =
				new GadgetContainerMessage< hoNDArray< std::complex< float> > >();

		if (!m3->getObjectPtr()->create(&dimensions)) {
			ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to send Create storage for NDArray")));
			return -1;
		}

		if (scanhead.data.len != m2->getObjectPtr()->channels) {
			std::cout << "Wrong number of channels detected in dataset" << std::endl;
			return -1;
		}

		sChannelHeader_with_data* channel_header = reinterpret_cast<sChannelHeader_with_data*>(scanhead.data.p);
		for (unsigned int c = 0; c < m2->getObjectPtr()->channels; c++) {
			std::complex<float>* dptr = reinterpret_cast< std::complex<float>* >(channel_header[c].data.p);

			memcpy(m3->getObjectPtr()->get_data_ptr()+ c*m2->getObjectPtr()->samples,
					dptr, m2->getObjectPtr()->samples*sizeof(float)*2);
		}

		if (write_to_file) {
			HDF5Exclusive lock;
			std::string hdf5filename = std::string(hdf5_file);
			std::string hdf5datavar = std::string(hdf5_group) + std::string("/data");
			hdf5_append_struct_with_data(m2->getObjectPtr(), m3->getObjectPtr(), hdf5filename.c_str(), hdf5datavar.c_str());
		}

		//Chain the message block together.
		m1->cont(m2);
		m2->cont(m3);
		if (!write_to_file_only) {
			if (con.putq(m1) == -1) {
				ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to put data package on queue")));
				return -1;
			}
		} else {
			m1->release();
		}

		{
			HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
			ClearsScanHeader_with_data(&scanhead);
		}
	}



	if (!write_to_file_only) {
		GadgetContainerMessage<GadgetMessageIdentifier>* m1 =
				new GadgetContainerMessage<GadgetMessageIdentifier>();

		m1->getObjectPtr()->id = GADGET_MESSAGE_CLOSE;

		if (con.putq(m1) == -1) {
			ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to put CLOSE package on queue")));
			return -1;
		}

		con.wait(); //Wait for recon to finish
	}

	return 0;
}
