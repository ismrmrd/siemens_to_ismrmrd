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

#include "mri_hdf5_io.h"
#include "hoNDArray_hdf5_io.h"

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
					if (is_number(split_path[i]) && (i != split_path.size())) {
						std::cout << "Numeric index not supported inside path for source = " << source << std::endl;
						continue;
					}
					search_path += std::string(".") + split_path[i];
				}

				int index = -1;
				if (is_number(split_path[split_path.size()-1])) {
					index = atoi(split_path[split_path.size()-1].c_str());
				} else {
					search_path += std::string(".") + split_path[split_path.size()-1];
				}

				const XProtocol::XNode* n = boost::apply_visitor(XProtocol::getChildNodeByName(search_path), node);

				std::vector<std::string> parameters = boost::apply_visitor(XProtocol::getStringValueArray(), *n);

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
  ACE_DEBUG((LM_INFO, ACE_TEXT("                  -f <DATA FILE>                 (default ./data.dat)\n") ));
  ACE_DEBUG((LM_INFO, ACE_TEXT("                  -m <PARAMETER MAP FILE>        (default ./parammap.xml)\n") ));
  ACE_DEBUG((LM_INFO, ACE_TEXT("                  -o <HDF5 output file>          (default out.h5)\n") ));
  ACE_DEBUG((LM_INFO, ACE_TEXT("                  -g <HDF5 output group>         (/dataset)\n") ));
  ACE_DEBUG((LM_INFO, ACE_TEXT("                  -c <GADGETRON CONFIG>          (default default.xml)\n") ));
  ACE_DEBUG((LM_INFO, ACE_TEXT("                  -w                             (write only flag, do not connect to Gadgetron)\n") ));
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[] )
{
	static const ACE_TCHAR options[] = ACE_TEXT(":p:h:f:o:c:m:g:w");

	ACE_Get_Opt cmd_opts(argc, argv, options);

	ACE_TCHAR port_no[1024];
	ACE_OS_String::strncpy(port_no, "9002", 1024);

	ACE_TCHAR hostname[1024];
	ACE_OS_String::strncpy(hostname, "localhost", 1024);

	ACE_TCHAR filename[4096];
	ACE_OS_String::strncpy(filename, "./data.dat", 4096);

	ACE_TCHAR config_file[1024];
	ACE_OS_String::strncpy(config_file, "default.xml", 1024);

	ACE_TCHAR parammap_file[1024];
	ACE_OS_String::strncpy(parammap_file, "parammap.xml", 1024);

	ACE_TCHAR hdf5_file[1024];
	ACE_OS_String::strncpy(hdf5_file, "out.h5", 1024);

	ACE_TCHAR hdf5_group[1024];
	ACE_OS_String::strncpy(hdf5_group, "dataset", 1024);

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
			boost::shared_ptr<H5File> f = OpenHF5File(hdf5_file);
			if (HDF5LinkExists(f.get(), hdf5_group)) {
				  ACE_DEBUG((LM_INFO, ACE_TEXT("HDF5 group \"%s\" already exists.\n"), hdf5_group));
				  print_usage();
				  return -1;
			}
		}
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- output file   :            %s\n"), hdf5_file));
		ACE_DEBUG((LM_INFO, ACE_TEXT("  -- output group  :            %s\n"), hdf5_group));
	}


	//We need some data to work with
	SiemensRawData sd;
	sd.ReadRawFile(filename);

	const std::string& config_buffer = sd.GetParameterBuffer("Meas");
	XProtocol::XNode n;

	if (XProtocol::ParseXProtocol(const_cast<std::string&>(config_buffer),n) < 0) {
		ACE_DEBUG((LM_ERROR, ACE_TEXT("Failed to parse XProtocol")));
		return -1;
	}

	std::string xml_config = ProcessGadgetronParameterMap(n,parammap_file);

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

		hdf5_append_array(&tmp,hdf5filename.c_str(),hdf5xmlvar.c_str());

	}


	GadgetronConnector con;
	if (!write_to_file_only) {
		con.register_writer(GADGET_MESSAGE_ACQUISITION, new GadgetAcquisitionMessageWriter());
		con.register_reader(GADGET_MESSAGE_IMAGE_REAL_USHORT, new ImageWriter<ACE_UINT16>());
		con.register_reader(GADGET_MESSAGE_IMAGE_REAL_FLOAT, new ImageWriter<float>());
		con.register_reader(GADGET_MESSAGE_IMAGE_CPLX_FLOAT, new ImageWriter< std::complex<float> >());

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


	//We need an array for collecting the data from all channels prior to transmission
	hoNDArray< std::complex<float> > buf;
	std::vector<unsigned int> buf_dim;
	buf_dim.push_back(sd.GetMaxValues()->ushSamplesInScan);
	buf_dim.push_back(sd.GetMaxValues()->ushUsedChannels);

	if (!buf.create(&buf_dim)) {
		ACE_DEBUG( (LM_ERROR,
				ACE_TEXT("Unable to allocate buffer for collecting channel data\n")) );
		return -1;
	}

	//Now loop through the data and send it all to the Gadgetron
	SiemensMdhNode* next = sd.GetFirstNode();
	while (next) {
		GadgetContainerMessage<GadgetMessageIdentifier>* m1 =
				new GadgetContainerMessage<GadgetMessageIdentifier>();

		m1->getObjectPtr()->id = GADGET_MESSAGE_ACQUISITION;

		GadgetContainerMessage<GadgetMessageAcquisition>* m2 =
				new GadgetContainerMessage<GadgetMessageAcquisition>();

		GadgetMessageAcquisition* acq_head = m2->getObjectPtr();
		acq_head->flags = 0;
		acq_head->idx.line                 = next->mdh.sLC.ushLine;
		acq_head->idx.acquisition          = next->mdh.sLC.ushAcquisition;
		acq_head->idx.slice                = next->mdh.sLC.ushSlice;
		acq_head->idx.partition            = next->mdh.sLC.ushPartition;
		acq_head->idx.echo                 = next->mdh.sLC.ushEcho;
		acq_head->idx.phase                = next->mdh.sLC.ushPhase;
		acq_head->idx.repetition           = next->mdh.sLC.ushRepetition;
		acq_head->idx.set                  = next->mdh.sLC.ushSet;
		acq_head->idx.segment              = next->mdh.sLC.ushSeg;
		acq_head->idx.channel              = next->mdh.ushChannelId;

		acq_head->flags |=
				(next->mdh.aulEvalInfoMask[0] & (1 << 25)) ?
						GADGET_FLAG_IS_NOISE_SCAN : 0;

		acq_head->flags |=
				(next->mdh.aulEvalInfoMask[0] & (1 << 29)) ?
						GADGET_FLAG_LAST_ACQ_IN_SLICE : 0;

		acq_head->flags |=
				(next->mdh.aulEvalInfoMask[0] & (1 << 28)) ?
						GADGET_FLAG_FIRST_ACQ_IN_SLICE : 0;

		acq_head->meas_uid                 = next->mdh.lMeasUID;
		acq_head->scan_counter             = next->mdh.ulScanCounter;
		acq_head->time_stamp               = next->mdh.ulTimeStamp;
		acq_head->samples                  = next->mdh.ushSamplesInScan;
		acq_head->channels                 = next->mdh.ushUsedChannels;
		acq_head->position[0]              = next->mdh.sSliceData.sSlicePosVec.flSag;
		acq_head->position[1]              = next->mdh.sSliceData.sSlicePosVec.flCor;
		acq_head->position[2]              = next->mdh.sSliceData.sSlicePosVec.flTra;

		memcpy(acq_head->quarternion,
				next->mdh.sSliceData.aflQuaternion,
				sizeof(float)*4);

		memcpy(buf.get_data_ptr()+next->mdh.ushChannelId*next->mdh.ushSamplesInScan,
				next->data,
				sizeof(float)*acq_head->samples*2);

		if (next->mdh.ushChannelId == (next->mdh.ushUsedChannels-1)) {

			std::vector<unsigned int> dimensions(2);
			dimensions[0] = m2->getObjectPtr()->samples;
			dimensions[1] = m2->getObjectPtr()->channels;

			GadgetContainerMessage< hoNDArray< std::complex<float> > >* m3 =
					new GadgetContainerMessage< hoNDArray< std::complex< float> > >();

			if (!m3->getObjectPtr()->create(&dimensions)) {
				ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to send Create storage for NDArray")));
				return -1;
			}

			memcpy(m3->getObjectPtr()->get_data_ptr(), buf.get_data_ptr(), buf.get_number_of_elements()*sizeof(float)*2);

			if (write_to_file) {
				std::string hdf5filename = std::string(hdf5_file);
				std::string hdf5datavar = std::string(hdf5_group) + std::string("/data");
				hdf5_append_struct_with_data(m2->getObjectPtr(), m3->getObjectPtr(), hdf5filename.c_str(), hdf5datavar.c_str());

//				ACE_OS::fwrite(acq_head, sizeof(GadgetMessageAcquisition), 1, outdatfile);
//				ACE_OS::fwrite(buf.get_data_ptr(), sizeof(float),buf.get_number_of_elements()*2, outdatfile);
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
		}

		next = (SiemensMdhNode*)next->next;
	}

	if (write_to_file) {
//		ACE_OS::fclose(outdatfile);
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
