#ifndef WIN32
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#endif //WIN32

#include "siemensraw.h"
#include "base64_decode.h"

#include "GadgetXml.h"
#include "XNode.h"

#include "siemens_hdf5_datatypes.h"

#include "ismrmrd.h"
#include "ismrmrd_hdf5.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <H5Cpp.h>
#include <iomanip>

#define EXCLUDE_ISMRMRD_XSD

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <utility>

#include <typeinfo>

extern std::string global_xml_VB_string;
extern std::string global_xml_VD_string;
extern std::string global_xsl_string;
extern std::string global_xsd_string;


struct MysteryData
{
	char mysteryData[160];
};


void calc_vds(double slewmax,double gradmax,double Tgsample,double Tdsample,int Ninterleaves,
    double* fov, int numfov,double krmax,
    int ngmax, double** xgrad,double** ygrad,int* numgrad);


void calc_traj(double* xgrad, double* ygrad, int ngrad, int Nints, double Tgsamp, double krmax,
    double** x_trajectory, double** y_trajectory,
    double** weights);


#ifndef WIN32
int xml_file_is_valid(std::string& xml, const char *schema_filename)
{

    xmlDocPtr doc;
    doc = xmlParseMemory(xml.c_str(),xml.size());

    xmlDocPtr schema_doc = xmlReadFile(schema_filename, NULL, XML_PARSE_NONET);
    if (schema_doc == NULL)
    {
        /* the schema cannot be loaded or is not well-formed */
        return -1;
    }
    xmlSchemaParserCtxtPtr parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
    if (parser_ctxt == NULL)
    {
        /* unable to create a parser context for the schema */
        xmlFreeDoc(schema_doc);
        return -2;
    }
    xmlSchemaPtr schema = xmlSchemaParse(parser_ctxt);
    if (schema == NULL)
    {
        /* the schema itself is not valid */
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(schema_doc);
        return -3;
    }
    xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
    if (valid_ctxt == NULL)
    {
        /* unable to create a validation context for the schema */
        xmlSchemaFree(schema);
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(schema_doc);
        xmlFreeDoc(doc);
        return -4;
    }
    int is_valid = (xmlSchemaValidateDoc(valid_ctxt, doc) == 0);
    xmlSchemaFreeValidCtxt(valid_ctxt);
    xmlSchemaFree(schema);
    xmlSchemaFreeParserCtxt(parser_ctxt);
    xmlFreeDoc(schema_doc);
    xmlFreeDoc(doc);

    /* force the return value to be non-negative on success */
    return is_valid ? 1 : 0;
}
#endif //WIN32


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
    for (unsigned int i = 0; i < s.size(); i++)
    {
        if (!std::isdigit(s.c_str()[i]))
        {
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
    if (parameters)
    {
        TiXmlNode* p = 0;
        while( p = parameters->IterateChildren( "p",  p ) )
        {
            TiXmlHandle ph(p);

            TiXmlText* s = ph.FirstChildElement("s").FirstChild().ToText();
            TiXmlText* d = ph.FirstChildElement("d").FirstChild().ToText();

            if (s && d)
            {
                std::string source      = s->Value();
                std::string destination = d->Value();

                std::vector<std::string> split_path;
                boost::split( split_path, source, boost::is_any_of("."), boost::token_compress_on );

                if (is_number(split_path[0]))
                {
                    std::cout << "First element of path (" << source << ") cannot be numeric" << std::endl;
                    continue;
                }

                std::string search_path = split_path[0];
                for (unsigned int i = 1; i < split_path.size()-1; i++)
                {
                    /*
                    if (is_number(split_path[i]) && (i != split_path.size())) {
                    std::cout << "Numeric index not supported inside path for source = " << source << std::endl;
                    continue;
                    }*/

                    search_path += std::string(".") + split_path[i];
                }

                int index = -1;
                if (is_number(split_path[split_path.size()-1]))
                {
                    index = atoi(split_path[split_path.size()-1].c_str());
                }
                else
                {
                    search_path += std::string(".") + split_path[split_path.size()-1];
                }

                const XProtocol::XNode* n = boost::apply_visitor(XProtocol::getChildNodeByName(search_path), node);

                std::vector<std::string> parameters;
                if (n)
                {
                    parameters = boost::apply_visitor(XProtocol::getStringValueArray(), *n);
                }
                else
                {
                    std::cout << "Search path: " << search_path << " not found." << std::endl;
                }

                if (index >= 0)
                {
                    if (parameters.size() > index)
                    {
                        out_n.add(destination, parameters[index]);
                    }
                    else
                    {
                        std::cout << "Parameter index (" << index << ") not valid for search path " << search_path << std::endl;
                        continue;
                    }
                }
                else
                {
                    out_n.add(destination, parameters);
                }
            }
            else
            {
                std::cout << "Malformed Gadgetron parameter map" << std::endl;
            }
        }
    }
    else
    {
        std::cout << "Malformed Gadgetron parameter map (parameters section not found)" << std::endl;
        return std::string("");
    }

    return XmlToString(out_doc);
}


void clear_all_temp_files()
{
    remove("tempfile.h5");
    remove("default_xml.xml");
    remove("default_xsl.xsl");
    remove("default_xsd.xsd");
}

void clear_temp_files(bool download_xml, bool download_xsl, bool download_xml_raw, bool debug_xml)
{
    remove("tempfile.h5");
    remove("default_xsd.xsd");
	if (!download_xml)
	{
		remove("default_xml.xml");
	}
	if (!download_xsl)
	{
		remove("default_xsl.xsl");
	}
	if (!download_xml_raw && !debug_xml)
	{
		remove("xml_raw.xml");
	}
}


/// compute noise dwell time in us for dependency and built-in noise in VD/VB lines
double compute_noise_sample_in_us(size_t num_of_noise_samples_this_acq, bool isAdjustCoilSens, bool isVB)
{
    if ( isAdjustCoilSens )
    {
        return 5.0;
    }
    else if ( isVB )
    {
        return (10e6/num_of_noise_samples_this_acq/130.0);
    }
    else
    {
        return ( ((long)(76800.0/num_of_noise_samples_this_acq)) / 10.0 );
    }

    return 5.0;
}


int main(int argc, char *argv[] )
{
	std::string filename;
	unsigned int hdf5_dataset_no;

	std::string parammap_file;
	std::string parammap_xsl;
	std::string schema_file_name;

	std::string hdf5_file;
	std::string hdf5_group;
	std::string hdf5_out_file;
	std::string hdf5_out_group;
	std::string out_format;
	std::string date_time = get_date_time_string();

	bool debug_xml = false;
	bool flash_pat_ref_scan = false;
    bool write_to_file = true;

    bool download_xml = false;
    bool download_xsl = false;
    bool download_xml_raw = false;

	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help, h", "produce help message")
	    (",f",                 			  po::value<std::string>(&filename)->default_value("./meas_MiniGadgetron_GRE.dat"), "<DAT FILE>")
	    (",d",   	   					  po::value<unsigned int>(&hdf5_dataset_no)->default_value(0), "<HDF5 DATASET NUMBER>")
	    (",m",       					  po::value<std::string>(&parammap_file)->default_value("default"), "<PARAMETER MAP FILE>")
	    (",x", 							  po::value<std::string>(&parammap_xsl)->default_value("default"), "<PARAMETER MAP STYLESHEET>")
	    (",c",         					  po::value<std::string>(&schema_file_name)->default_value("default"), "<SCHEMA FILE NAME>")

	    (",M",           				  po::value<bool>(&download_xml)->implicit_value(true), "<Download XML file>")
	    (",S",           				  po::value<bool>(&download_xsl)->implicit_value(true), "<Download XSL file>")
	    (",R",           				  po::value<bool>(&download_xml_raw)->implicit_value(true), "<Download intermediate XML file>")

	    (",o",           				  po::value<std::string>(&hdf5_file)->default_value("output.h5"), "<HDF5 output file>")
	    (",g",	          				  po::value<std::string>(&hdf5_group)->default_value("dataset"), "<HDF5 output group>")
	    (",r",         					  po::value<std::string>(&hdf5_out_file)->default_value("./result.h5"), "<HDF5 result file>")
	    (",G",        					  po::value<std::string>(&hdf5_out_group)->default_value(date_time.c_str()), "<HDF5 result group>")
	    (",X",           				  po::value<bool>(&debug_xml)->implicit_value(true), "<Debug XML flag>")
	    (",F",       					  po::value<bool>(&flash_pat_ref_scan)->implicit_value(false), "<FLASH PAT REF flag>")
	    (",U", 							  po::value<std::string>(&out_format)->default_value("h5"), "<OUT FILE FORMAT, 'h5 or hdf5' or 'hdf or analyze' or 'nii or nifti'>")

	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
	    std::cout << desc << "\n";
	    clear_all_temp_files();
	    return 1;
	}

	std::ifstream file_1(filename.c_str());
    if (!file_1)
    {
    	std::cout << "Data file: " << filename << " does not exist." << std::endl;
        std::cout << desc << "\n";
        clear_all_temp_files();
        return -1;
    }
    else
    {
    	std::cout << "Data file is:                          " << filename << std::endl;
    }
    file_1.close();

    std::string tempfile("tempfile.h5"); // tmpname

	if (parammap_xsl == "default")
	{
		std::string parammap_xsl_content = base64_decode(global_xsl_string);
		std::ofstream default_parammap_xsl("default_xsl.xsl", std::ofstream::out);
		default_parammap_xsl << parammap_xsl_content;
		default_parammap_xsl.close();
		parammap_xsl = "./default_xsl.xsl";
	}

    std::ifstream file_3(parammap_xsl.c_str());
    if (!file_3)
    {
    	std::cout << "Parameter XSL stylesheet: " << parammap_xsl << " does not exist." << std::endl;
    	std::cout << desc << "\n";
    	clear_all_temp_files();
        return -1;
    }
    else
    {
        std::cout << "Parameter XSL stylesheet is:           " << parammap_xsl << std::endl;
    }
    file_3.close();

	if (schema_file_name == "default")
	{
		std::string schema_file_name_content = base64_decode(global_xsd_string);
		std::ofstream default_schema_file_name("default_xsd.xsd", std::ofstream::out);
		default_schema_file_name << schema_file_name_content;
		default_schema_file_name.close();
		schema_file_name = "./default_xsd.xsd";
	}

    std::ifstream file_4(schema_file_name.c_str());
    if (!file_4)
    {
        std::cout << "Schema file name: " << schema_file_name << " does not exist." << std::endl;
        std::cout << desc << "\n";
        clear_all_temp_files();
        return -1;
    }
    else
    {
        std::cout << "Schema file name is:                   " << schema_file_name << std::endl;
    }
    file_4.close();

    boost::shared_ptr<ISMRMRD::IsmrmrdDataset>  ismrmrd_dataset;

    if (write_to_file)
    {
    	ismrmrd_dataset = boost::shared_ptr<ISMRMRD::IsmrmrdDataset>(new ISMRMRD::IsmrmrdDataset(hdf5_file.c_str(), hdf5_group.c_str()));
    }

	std::cout << " ----- " << std::endl;
	std::cout << "Processing file: " << filename << std::endl;

	std::ifstream f(filename.c_str(), std::ios::binary);

	MrParcRaidFileHeader ParcRaidHead;

	f.read((char*)(&ParcRaidHead), sizeof(MrParcRaidFileHeader));

	std::cout << "MrParcRaidFileHeader size: " << ParcRaidHead.hdSize_ << std::endl;
	std::cout << "MrParcRaidFileHeader count: " << ParcRaidHead.count_ << std::endl;

    bool VBFILE = false;

	if (ParcRaidHead.hdSize_ > 32)
	{
	    VBFILE = true;

        std::cout << "VB line file detected." << std::endl;
        std::cout << "We have no raid file header" << std::endl;
        std::cout << "ParcRaidHead.count_ is: " << ParcRaidHead.count_ << std::endl;

        //Rewind, we have no raid file header.
        f.seekg(0, std::ios::beg);

        ParcRaidHead.hdSize_ = ParcRaidHead.count_;
        ParcRaidHead.count_ = 1;

        std::cout << "NEW ParcRaidHead.hdSize_ is: " << ParcRaidHead.hdSize_ << std::endl;
        std::cout << "NEW ParcRaidHead.count_ is: " << ParcRaidHead.count_ << std::endl;
	}

	else if (ParcRaidHead.hdSize_ != 0)
	{
		//This is a VB line data file
		throw std::runtime_error("Only VD line files with MrParcRaidFileHeader.hdSize_ == 0 (MR_PARC_RAID_ALLDATA) supported.");
	}

    //if it is a VB scan
	if (parammap_file == "default" && VBFILE)
	{
		std::string parammap_file_content = base64_decode(global_xml_VB_string);
		std::ofstream default_parammap_file("default_xml.xml", std::ofstream::out);
		default_parammap_file << parammap_file_content;
		default_parammap_file.close();
		parammap_file = "./default_xml.xml";
		std::cout << "IT IS A VB SCAN" << std::endl;
	}

	//if it is a VD scan
	if (parammap_file == "default" && !VBFILE)
	{
		std::string parammap_file_content = base64_decode(global_xml_VD_string);
		std::ofstream default_parammap_file("default_xml.xml", std::ofstream::out);
		default_parammap_file << parammap_file_content;
		default_parammap_file.close();
		parammap_file = "./default_xml.xml";
		std::cout << "IT IS A VD SCAN" << std::endl;
	}

	std::ifstream file_2(parammap_file.c_str());
    if (!file_2)
    {
    	std::cout << "Parameter map file: " << parammap_file << " does not exist." << std::endl;
    	std::cout << desc << "\n";
    	clear_all_temp_files();
    	f.close();
        return -1;
    }
    else
    {
        std::cout << "Parameter map file is:                 " << parammap_file << std::endl;
    }
    file_2.close();

	std::cout << "This file contains " << ParcRaidHead.count_ << " measurement(s)." << std::endl;

	MrParcRaidFileEntry ParcFileEntries[64];

    if (VBFILE)
    {
    	//In case of VB file, we are just going to fill these with zeros. It doesn't exist.
		for (unsigned int i = 0; i < 64; i++)
		{
			memset(&ParcFileEntries[i], 0, sizeof(MrParcRaidFileEntry));
		}

        ParcFileEntries[0].off_ = 0;
        f.seekg(0,std::ios::end); //Rewind a bit, we have no raid file header.
        ParcFileEntries[0].len_ = f.tellg(); //This is the whole size of the dat file
        f.seekg(0,std::ios::beg); //Rewind a bit, we have no raid file header.

    	std::cout << "ParcFileEntries[0].off_ = " << ParcFileEntries[0].off_ << std::endl;
    	std::cout << "ParcFileEntries[0].len_ = " << ParcFileEntries[0].len_ << std::endl;
        std::cout << "File id: " << ParcFileEntries[0].fileId_ << std::endl; // 0
        std::cout << "Meas id: " << ParcFileEntries[0].measId_ << std::endl; // 0
        std::cout << "Protocol name: " << ParcFileEntries[0].protName_ << std::endl; // blank
    	std::cout << "Patient Name: " << ParcFileEntries[0].patName_ << std::endl; // blank

    }

    else
    {
    	for (unsigned int i = 0; i < 64; i++)
    	{
    		f.read((char*)(&ParcFileEntries[i]), sizeof(MrParcRaidFileEntry));

            if (i < ParcRaidHead.count_)
            {
                std::cout << "File id: " << ParcFileEntries[i].fileId_ << std::endl;
                std::cout << "Meas id: " << ParcFileEntries[i].measId_ << std::endl;
                std::cout << "Protocol name: " << ParcFileEntries[i].protName_ << std::endl;
            	std::cout << "Offset: " << ParcFileEntries[i].off_ << std::endl;
            	std::cout << "Length: " << ParcFileEntries[i].len_ << std::endl;
            	std::cout << "Patient Name: " << ParcFileEntries[i].patName_ << std::endl;
            }
    	}
    }

    MysteryData mystery_data;

	MeasurementHeader mhead;

	// find the beggining of the first measurement
	//TODO: Change this, so we find the measurement that user is asking for!
	f.seekg(ParcFileEntries[0].off_, std::ios::beg);

	//MeasurementHeader mhead;
	f.read((char*)(&mhead.dma_length), sizeof(uint32_t));
	f.read((char*)(&mhead.nr_buffers),sizeof(uint32_t));

	std::cout << "Measurement header DMA length: " << mhead.dma_length << std::endl;
	std::cout << "Measurement header number of buffers: " << mhead.nr_buffers << std::endl;

	//Now allocate dynamic memory for the buffers
	mhead.buffers.len = mhead.nr_buffers;
	std::cout << "Measurement header number of buffers (again): " << mhead.buffers.len << std::endl;

	MeasurementHeaderBuffer* buffers = new MeasurementHeaderBuffer[mhead.nr_buffers];
	mhead.buffers.p = (void*)(buffers);

	std::cout << "Number of parameter buffers: " << mhead.nr_buffers << std::endl;

	char bufname_tmp[32];
	for (int b = 0; b < mhead.nr_buffers; b++)
	{
		f.getline(bufname_tmp, 32, '\0');
		std::cout << "Buffer Name: " << bufname_tmp << std::endl;
		buffers[b].bufName_.len = f.gcount() + 1;
		bufname_tmp[f.gcount()] = '\0';
		buffers[b].bufName_.p = (void*)(new char[buffers[b].bufName_.len]);
		memcpy(buffers[b].bufName_.p, bufname_tmp, buffers[b].bufName_.len);

		f.read((char*)(&buffers[b].bufLen_), sizeof(uint32_t));
		buffers[b].buf_.len = buffers[b].bufLen_;
		buffers[b].buf_.p = (void*)(new char[buffers[b].buf_.len]);
		f.read((char*)(buffers[b].buf_.p), buffers[b].buf_.len);
	}

	//We need to be on a 32 byte boundary after reading the buffers
	//TODO:
	long long int position_in_meas = (long long int)(f.tellg()) - ParcFileEntries[0].off_;
	if (position_in_meas % 32)
	{
		f.seekg(32 - (position_in_meas % 32), std::ios::cur);
	}

	// Measurement header done!

    //Now we should have the measurement headers, so let's use the Meas header to create the gadgetron XML parameters

    std::string xml_config;
    std::vector<std::string> wip_long;
    std::vector<std::string> wip_double;
    long trajectory = 0;
    long dwell_time_0 = 0;
    long max_channels = 0;
    long radial_views = 0;
    long center_line = 0;
    long center_partition = 0;
    long lPhaseEncodingLines = 0;
    long iNoOfFourierLines = 0;
    long lPartitions = 0;
    long iNoOfFourierPartitions = 0;
    std::string seqString;
    std::string baseLineString;

    std::string protocol_name = "";

    for (unsigned int b = 0; b < mhead.nr_buffers; b++)
    {
        if (std::string((char*)buffers[b].bufName_.p).compare("Meas") == 0)
        {
            std::string config_buffer((char*)buffers[b].buf_.p, buffers[b].buf_.len-2);//-2 because the last two character are ^@

            XProtocol::XNode n;

            if (debug_xml)
            {
                std::ofstream o("config_buffer.xprot");
                o.write(config_buffer.c_str(), config_buffer.size());
                o.close();
            }

            if (XProtocol::ParseXProtocol(const_cast<std::string&>(config_buffer),n) < 0)
            {
            	std::cout << "Failed to parse XProtocol" << std::endl;
            	clear_all_temp_files();
                return -1;
            }

            //Get some parameters - wip long
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sWipMemBlock.alFree"), n);
                if (n2)
                {
                    wip_long = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "Search path: MEAS.sWipMemBlock.alFree not found." << std::endl;
                }
                if (wip_long.size() == 0)
                {
                    std::cout << "Failed to find WIP long parameters" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
            }

            //Get some parameters - wip long
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sWipMemBlock.adFree"), n);
                if (n2)
                {
                    wip_double = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "Search path: MEAS.sWipMemBlock.adFree not found." << std::endl;
                }
                if (wip_double.size() == 0)
                {
                    std::cout << "Failed to find WIP double parameters" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
            }

            //Get some parameters - dwell times
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sRXSPEC.alDwellTime"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "Search path: MEAS.sWipMemBlock.alFree not found." << std::endl;
                }
                if (temp.size() == 0)
                {
                    std::cout << "Failed to find dwell times" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    dwell_time_0 = std::atoi(temp[0].c_str());
                }
            }

            //Get some parameters - trajectory
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.ucTrajectory"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "Search path: MEAS.sKSpace.ucTrajectory not found." << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find appropriate trajectory array" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    trajectory = std::atoi(temp[0].c_str());
                    std::cout << "Trajectory is: " << trajectory << std::endl;
                }
            }

            //Get some parameters - max channels
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("YAPS.iMaxNoOfRxChannels"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "YAPS.iMaxNoOfRxChannels" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find YAPS.iMaxNoOfRxChannels array" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    max_channels = std::atoi(temp[0].c_str());
                }
            }

            //Get some parameters - cartesian encoding bits
            {
                // get the center line parameters
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.lPhaseEncodingLines"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "MEAS.sKSpace.lPhaseEncodingLines not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find MEAS.sKSpace.lPhaseEncodingLines array" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    lPhaseEncodingLines = std::atoi(temp[0].c_str());
                }

                n2 = boost::apply_visitor(XProtocol::getChildNodeByName("YAPS.iNoOfFourierLines"), n);
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "YAPS.iNoOfFourierLines not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find YAPS.iNoOfFourierLines array" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    iNoOfFourierLines = std::atoi(temp[0].c_str());
                }

                long lFirstFourierLine;
                bool has_FirstFourierLine = false;
                n2 = boost::apply_visitor(XProtocol::getChildNodeByName("YAPS.lFirstFourierLine"), n);
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "YAPS.lFirstFourierLine not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find YAPS.lFirstFourierLine array" << std::endl;
                    has_FirstFourierLine = false;
                }
                else
                {
                    lFirstFourierLine = std::atoi(temp[0].c_str());
                    has_FirstFourierLine = true;
                }

                // get the center partition parameters
                n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.lPartitions"), n);
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "MEAS.sKSpace.lPartitions not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find MEAS.sKSpace.lPartitions array" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    lPartitions = std::atoi(temp[0].c_str());
                }

                // Note: iNoOfFourierPartitions is sometimes absent for 2D sequences
                n2 = boost::apply_visitor(XProtocol::getChildNodeByName("YAPS.iNoOfFourierPartitions"), n);
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                    if (temp.size() != 1)
                    {
                        iNoOfFourierPartitions = 1;
                    }
                    else
                    {
                        iNoOfFourierPartitions = std::atoi(temp[0].c_str());
                    }
                }
                else
                {
                    iNoOfFourierPartitions = 1;
                }

                long lFirstFourierPartition;
                bool has_FirstFourierPartition = false;
                n2 = boost::apply_visitor(XProtocol::getChildNodeByName("YAPS.lFirstFourierPartition"), n);
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "YAPS.lFirstFourierPartition not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find YAPS.lFirstFourierPartition array" << std::endl;
                    has_FirstFourierPartition = false;
                }
                else
                {
                    lFirstFourierPartition = std::atoi(temp[0].c_str());
                    has_FirstFourierPartition = true;
                }

                // set the values
                if ( has_FirstFourierLine ) // bottom half for partial fourier
                {
                    center_line = lPhaseEncodingLines/2 - ( lPhaseEncodingLines - iNoOfFourierLines );
                }
                else
                {
                    center_line = lPhaseEncodingLines/2;
                }

                if (iNoOfFourierPartitions > 1) {
                    // 3D
                    if ( has_FirstFourierPartition ) // bottom half for partial fourier
                    {
                        center_partition = lPartitions/2 - ( lPartitions - iNoOfFourierPartitions );
                    }
                    else
                    {
                        center_partition = lPartitions/2;
                    }
                } else {
                    // 2D
                    center_partition = 0;
                }

                // for spiral sequences the center_line and center_partition are zero
                if (trajectory == 4) {
                    center_line = 0;
                    center_partition = 0;
                }

                std::cout << "center_line = " << center_line << std::endl;
                std::cout << "center_partition = " << center_partition << std::endl;
            }

            //Get some parameters - radial views
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.lRadialViews"), n);
                std::vector<std::string> temp;
                if (n2) {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "MEAS.sKSpace.lRadialViews not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find YAPS.MEAS.sKSpace.lRadialViews array" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    radial_views = std::atoi(temp[0].c_str());
                }
            }

            //Get some parameters - protocol name
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("HEADER.tProtocolName"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                else
                {
                    std::cout << "HEADER.tProtocolName not found" << std::endl;
                }
                if (temp.size() != 1)
                {
                    std::cout << "Failed to find HEADER.tProtocolName" << std::endl;
                    clear_all_temp_files();
                    return -1;
                }
                else
                {
                    protocol_name = temp[0];
                }
            }

            // Get some parameters - base line
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sProtConsistencyInfo.tBaselineString"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                if (temp.size() > 0)
                {
                    baseLineString = temp[0];
                }
            }

            if ( baseLineString.empty() )
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sProtConsistencyInfo.tMeasuredBaselineString"), n);
                std::vector<std::string> temp;
                if (n2)
                {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                }
                if (temp.size() > 0)
                {
                    baseLineString = temp[0];
                }
            }

            if ( baseLineString.empty() )
            {
                std::cout << "Failed to find MEAS.sProtConsistencyInfo.tBaselineString/tMeasuredBaselineString" << std::endl;
            }

            xml_config = ProcessGadgetronParameterMap(n,parammap_file);
            break;
        }
    }

    std::cout << "Protocol Name: " << protocol_name << std::endl;

    // whether this scan is a adjustment scan
    bool isAdjustCoilSens = false;
    if ( protocol_name == "AdjCoilSens" )
    {
        isAdjustCoilSens = true;
    }

    // whether this scan is from VB line
    bool isVB = false;
    if ( (baseLineString.find("VB17") != std::string::npos)
        || (baseLineString.find("VB15") != std::string::npos)
        || (baseLineString.find("VB13") != std::string::npos)
        || (baseLineString.find("VB11") != std::string::npos) )
    {
        isVB = true;
    }

    std::cout << "Baseline: " << baseLineString << std::endl;

    if (debug_xml || download_xml_raw)
    {
        std::ofstream o("xml_raw.xml");
        o.write(xml_config.c_str(), xml_config.size());
        o.close();
    }

    //Get rid of dynamically allocated memory in header
    {
        ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
        ClearMeasurementHeader(&mhead);
    }

#ifndef WIN32
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;

    const char *params[16 + 1];
    int nbparams = 0;
    params[nbparams] = NULL;
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;

    cur = xsltParseStylesheetFile((const xmlChar*)parammap_xsl.c_str());
    doc = xmlParseMemory(xml_config.c_str(),xml_config.size());
    res = xsltApplyStylesheet(cur, doc, params);

    xmlChar* out_ptr = NULL;
    int xslt_length = 0;
    int xslt_result = xsltSaveResultToString(&out_ptr,
        &xslt_length,
        res,
        cur);

    if (xslt_result < 0)
    {
        std::cout << "Failed to save converted doc to string" << std::endl;
        clear_all_temp_files();
        return -1;
    }

    xml_config = std::string((char*)out_ptr,xslt_length);

    if (debug_xml)
    {
        std::ofstream o("processed.xml");
        o.write(xml_config.c_str(), xml_config.size());
        o.close();
    }

    if (xml_file_is_valid(xml_config,schema_file_name.c_str()) <= 0)
    {
        std::cout << "Generated XML is not valid according to the ISMRMRD schema" << schema_file_name << std::endl;
        clear_all_temp_files();
        return -1;
    }

    xsltFreeStylesheet(cur);
    xmlFreeDoc(res);
    xmlFreeDoc(doc);

    xsltCleanupGlobals();
    xmlCleanupParser();
#else
    std::string syscmd;
    int xsltproc_res(0);

    std::string xml_post("xml_post.xml"), xml_pre("xml_pre.xml");
    syscmd = std::string("xsltproc --output xml_post.xml \"") + std::string(parammap_xsl) + std::string("\" xml_pre.xml");

    //if ( !gadgetron_home_from_env )
    //{
    //    xml_post = gadgetron_home + std::string("/xml_post.xml");
    //    xml_pre = gadgetron_home + std::string("/xml_pre.xml");

    //    syscmd = gadgetron_home + std::string("/xsltproc.exe --output ") + xml_post + std::string(" \"") + std::string(parammap_xsl) + std::string("\" ") + xml_pre;
    //}

    std::ofstream o(xml_pre);
    o.write(xml_config.c_str(), xml_config.size());
    o.close();

    xsltproc_res = system(syscmd.c_str());

    std::ifstream t(xml_post.c_str());
    xml_config = std::string((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());

    if ( xsltproc_res != 0 )
    {
        std::cerr << "Failed to call up xsltproc : \t" << syscmd << std::endl;

        // try again
        //if ( !gadgetron_home_from_env )
        //{
        //    xml_post = std::string("/xml_post.xml");
        //    xml_pre = std::string("/xml_pre.xml");

        //    syscmd = std::string("xsltproc.exe --output ") + xml_post + std::string(" \"") + std::string(parammap_xsl) + std::string("\" ") + xml_pre;
        //}

        std::ofstream o(xml_pre);
        o.write(xml_config.c_str(), xml_config.size());
        o.close();

        xsltproc_res = system(syscmd.c_str());

        if ( xsltproc_res != 0 )
        {
            std::cerr << "Failed to generate XML header" << std::endl;
            clear_all_temp_files();
            return -1;
        }

        std::ifstream t(xml_post.c_str());
        xml_config = std::string((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());
    }
#endif //WIN32

    ISMRMRD::AcquisitionHeader acq_head_base;
    memset(&acq_head_base, 0, sizeof(ISMRMRD::AcquisitionHeader) );


    if (write_to_file)
    {
		ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
		if (ismrmrd_dataset->writeHeader(xml_config) < 0 )
		{
			std::cerr << "Failed to write XML header to HDF file" << std::endl;
			clear_all_temp_files();
			return -1;
		}

		// a test
		if (ismrmrd_dataset->writeHeader(xml_config) < 0 )
		{
			std::cerr << "Failed to write XML header to HDF file" << std::endl;
			clear_all_temp_files();
			return -1;
		}
    }

    //If this is a spiral acquisition, we will calculate the trajectory and add it to the individual profiles
	 ISMRMRD::NDArrayContainer<float> traj;
	 if (trajectory == 4)
	 {
		 int     nfov   = 1;         /*  number of fov coefficients.             */
		 int     ngmax  = (int)1e5;  /*  maximum number of gradient samples      */
		 double  *xgrad;             /*  x-component of gradient.                */
		 double  *ygrad;             /*  y-component of gradient.                */
		 double  *x_trajectory;
		 double  *y_trajectory;
		 double  *weighting;
		 int     ngrad;

		 double sample_time = (1.0*dwell_time_0) * 1e-9;
		 double smax = std::atof(wip_double[7].c_str());
		 double gmax = std::atof(wip_double[6].c_str());
		 double fov = std::atof(wip_double[9].c_str());
		 double krmax = std::atof(wip_double[8].c_str());
		 long interleaves = radial_views;

		 /*    call c-function here to calculate gradients */
		 calc_vds(smax, gmax, sample_time, sample_time, interleaves, &fov, nfov, krmax, ngmax, &xgrad, &ygrad, &ngrad);

		 /*
		 std::cout << "Calculated trajectory for spiral: " << std::endl
		 << "sample_time: " << sample_time << std::endl
		 << "smax: " << smax << std::endl
		 << "gmax: " << gmax << std::endl
		 << "fov: " << fov << std::endl
		 << "krmax: " << krmax << std::endl
		 << "interleaves: " << interleaves << std::endl
		 << "ngrad: " << ngrad << std::endl;
		 */

		 /* Calcualte the trajectory and weights*/
		 calc_traj(xgrad, ygrad, ngrad, interleaves, sample_time, krmax, &x_trajectory, &y_trajectory, &weighting);

		 std::vector<unsigned int> trajectory_dimensions;
		 trajectory_dimensions.push_back(ngrad);
		 trajectory_dimensions.push_back(interleaves);

		 traj = ISMRMRD::NDArrayContainer<float>(trajectory_dimensions);

		 // 2 * number of points for each X and Y
		 traj.resize(2 * ngrad * interleaves);

		 for (int i = 0; i < (ngrad*interleaves); i++)
		 {
			 traj[i * 2]     = (float)(-x_trajectory[i]/2);
			 traj[i * 2 + 1] = (float)(-y_trajectory[i]/2);
		 }

		 delete [] xgrad;
		 delete [] ygrad;
		 delete [] x_trajectory;
		 delete [] y_trajectory;
		 delete [] weighting;
	 }

     uint32_t last_mask = 0;
     unsigned long int acquisitions = 1;
     unsigned long int sync_data_packets = 0;
     sMDH mdh;//For VB line
     bool first_call = true;

     while (!(last_mask & 1) && //Last scan not encountered
    		 //TODO:
             (((ParcFileEntries[0].off_+ ParcFileEntries[0].len_)-f.tellg()) > sizeof(sScanHeader)))  //not reached end of measurement without acqend
     {
    	 size_t position_in_meas = f.tellg();
         sScanHeader_with_data scanhead;
         f.read(reinterpret_cast<char*>(&scanhead.scanHeader.ulFlagsAndDMALength), sizeof(uint32_t));

         //std::cout << " mdh.ulScanCounter === " <<  mdh.ulScanCounter << std::endl;

         if ( mdh.ulScanCounter%1000 == 0 )
         {
             std::cout << " mdh.ulScanCounter = " <<  mdh.ulScanCounter << std::endl;
         }

         if (VBFILE)
         {
             f.read(reinterpret_cast<char*>(&mdh) + sizeof(uint32_t), sizeof(sMDH) - sizeof(uint32_t));
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
         }
         else
         {
             f.read(reinterpret_cast<char*>(&scanhead.scanHeader) + sizeof(uint32_t), sizeof(sScanHeader)-sizeof(uint32_t));
         }

         uint32_t dma_length = scanhead.scanHeader.ulFlagsAndDMALength & MDH_DMA_LENGTH_MASK;
         uint32_t mdh_enable_flags = scanhead.scanHeader.ulFlagsAndDMALength & MDH_ENABLE_FLAGS_MASK;


         if (scanhead.scanHeader.aulEvalInfoMask[0] & ( 1 << 5))
         { //Check if this is synch data, if so, it must be handled differently.
             //This is synch data.

        	 std::cout << "THIS IS SYNCH DATA!!!!" << std::endl;

             sScanHeader_with_syncdata synch_data;
             synch_data.scanHeader = scanhead.scanHeader;
             synch_data.last_scan_counter = acquisitions-1;

             if (VBFILE)
             {
                 synch_data.syncdata.len = dma_length-sizeof(sMDH);
             }
             else
             {
                 synch_data.syncdata.len = dma_length-sizeof(sScanHeader);
             }
             std::vector<uint8_t> synchdatabytes(synch_data.syncdata.len);
             synch_data.syncdata.p = &synchdatabytes[0];
             f.read(reinterpret_cast<char*>(&synchdatabytes[0]), synch_data.syncdata.len);

             sync_data_packets++;
             continue;
         }

         //std::cout << "scanhead.scanHeader.lMeasUID: " << scanhead.scanHeader.lMeasUID << std::endl;
         //std::cout << "ParcFileEntries[i].measId_: " << ParcFileEntries[0].measId_ << std::endl;

         //This check only makes sense in VD line files.
         //TODO:
         if (!VBFILE && (scanhead.scanHeader.lMeasUID != ParcFileEntries[0].measId_))
         {
             //Something must have gone terribly wrong. Bail out.
             if ( first_call )
             {
                 std::cout << "Corrupted or retro-recon dataset detected (scanhead.scanHeader.lMeasUID != ParcFileEntries[0].measId_)" << std::endl;
                 std::cout << "Fix the scanhead.scanHeader.lMeasUID ... " << std::endl;
                 first_call = false;
             }

             scanhead.scanHeader.lMeasUID = ParcFileEntries[0].measId_;
         }

         //Allocate data for channels
         scanhead.data.len = scanhead.scanHeader.ushUsedChannels;
         sChannelHeader_with_data* chan = new sChannelHeader_with_data[scanhead.data.len];
         scanhead.data.p = reinterpret_cast<void*>(chan);

         for (unsigned int c = 0; c < scanhead.scanHeader.ushUsedChannels; c++)
         {
             if (VBFILE)
             {
                 if (c > 0)
                 {
                     f.read(reinterpret_cast<char*>(&mdh), sizeof(sMDH));
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
             }
             else
             {
                 f.read(reinterpret_cast<char*>(&chan[c].channelHeader), sizeof(sChannelHeader));
             }
             chan[c].data.len = scanhead.scanHeader.ushSamplesInScan;
             chan[c].data.p = reinterpret_cast<void*>(new complex_t[chan[c].data.len]);
             f.read(reinterpret_cast<char*>(chan[c].data.p), chan[c].data.len*sizeof(complex_t));
         }

         acquisitions++;
         last_mask = scanhead.scanHeader.aulEvalInfoMask[0];

         if (scanhead.scanHeader.aulEvalInfoMask[0] & 1)
         {
             std::cout << "Last scan reached..." << std::endl;
             ISMRMRD::HDF5Exclusive lock;
             ClearsScanHeader_with_data(&scanhead);
             break;
         }

         ISMRMRD::Acquisition* ismrmrd_acq = new ISMRMRD::Acquisition;

         ISMRMRD::AcquisitionHeader ismrmrd_acq_head;

         memset(&ismrmrd_acq_head,0,sizeof(ismrmrd_acq_head));

         ismrmrd_acq_head.measurement_uid                 = scanhead.scanHeader.lMeasUID;
         ismrmrd_acq_head.scan_counter                    = scanhead.scanHeader.ulScanCounter;
         ismrmrd_acq_head.acquisition_time_stamp          = scanhead.scanHeader.ulTimeStamp;
         ismrmrd_acq_head.physiology_time_stamp[0]        = scanhead.scanHeader.ulPMUTimeStamp;
         ismrmrd_acq_head.number_of_samples               = scanhead.scanHeader.ushSamplesInScan;
         ismrmrd_acq_head.available_channels              = (uint16_t)max_channels;
         ismrmrd_acq_head.active_channels                 = scanhead.scanHeader.ushUsedChannels;
         // uint64_t channel_mask[16];                       //Mask to indicate which channels are active. Support for 1024 channels
         ismrmrd_acq_head.discard_pre                     = scanhead.scanHeader.sCutOff.ushPre;
         ismrmrd_acq_head.discard_post                    = scanhead.scanHeader.sCutOff.ushPost;
         ismrmrd_acq_head.center_sample                   = scanhead.scanHeader.ushKSpaceCentreColumn;
         ismrmrd_acq_head.encoding_space_ref              = 0;
         ismrmrd_acq_head.trajectory_dimensions           = 0;

         if ( scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 25) )
         { //This is noise
             ismrmrd_acq_head.sample_time_us                =  compute_noise_sample_in_us(ismrmrd_acq_head.number_of_samples, isAdjustCoilSens, isVB);
         }
         else
         {
             ismrmrd_acq_head.sample_time_us                = dwell_time_0 / 1000.0f;
         }

         ismrmrd_acq_head.position[0]                        = scanhead.scanHeader.sSliceData.sSlicePosVec.flSag;
         ismrmrd_acq_head.position[1]                        = scanhead.scanHeader.sSliceData.sSlicePosVec.flCor;
         ismrmrd_acq_head.position[2]                        = scanhead.scanHeader.sSliceData.sSlicePosVec.flTra;

         // Convert Siemens quaternions to direction cosines.
         // In the Siemens convention the quaternion corresponds to a rotation matrix with columns P R S
         // Siemens stores the quaternion as (W,X,Y,Z)
         float quat[4];
         quat[0] = scanhead.scanHeader.sSliceData.aflQuaternion[1]; // X
         quat[1] = scanhead.scanHeader.sSliceData.aflQuaternion[2]; // Y
         quat[2] = scanhead.scanHeader.sSliceData.aflQuaternion[3]; // Z
         quat[3] = scanhead.scanHeader.sSliceData.aflQuaternion[0]; // W
         ISMRMRD::quaternion_to_directions(	quat,
                                         	ismrmrd_acq_head.phase_dir,
                                         	ismrmrd_acq_head.read_dir,
                                         	ismrmrd_acq_head.slice_dir);

         ismrmrd_acq_head.patient_table_position[0]  = (float)scanhead.scanHeader.lPTABPosX;
         ismrmrd_acq_head.patient_table_position[1]  = (float)scanhead.scanHeader.lPTABPosY;
         ismrmrd_acq_head.patient_table_position[2]  = (float)scanhead.scanHeader.lPTABPosZ;

         bool fixedE1E2 = true;
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 25)))   fixedE1E2 = false; // noise
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 1)))    fixedE1E2 = false; // navigator, rt feedback
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 2)))    fixedE1E2 = false; // hp feedback
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 51)))   fixedE1E2 = false; // dummy
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 5)))    fixedE1E2 = false; // synch data

         ismrmrd_acq_head.idx.average                = scanhead.scanHeader.sLC.ushAcquisition;
         ismrmrd_acq_head.idx.contrast               = scanhead.scanHeader.sLC.ushEcho;

         ismrmrd_acq_head.idx.kspace_encode_step_1   = scanhead.scanHeader.sLC.ushLine;
         ismrmrd_acq_head.idx.kspace_encode_step_2   = scanhead.scanHeader.sLC.ushPartition;

         ismrmrd_acq_head.idx.phase                  = scanhead.scanHeader.sLC.ushPhase;
         ismrmrd_acq_head.idx.repetition             = scanhead.scanHeader.sLC.ushRepetition;
         ismrmrd_acq_head.idx.segment                = scanhead.scanHeader.sLC.ushSeg;
         ismrmrd_acq_head.idx.set                    = scanhead.scanHeader.sLC.ushSet;
         ismrmrd_acq_head.idx.slice                  = scanhead.scanHeader.sLC.ushSlice;
         ismrmrd_acq_head.idx.user[0]                = scanhead.scanHeader.sLC.ushIda;
         ismrmrd_acq_head.idx.user[1]                = scanhead.scanHeader.sLC.ushIdb;
         ismrmrd_acq_head.idx.user[2]                = scanhead.scanHeader.sLC.ushIdc;
         ismrmrd_acq_head.idx.user[3]                = scanhead.scanHeader.sLC.ushIdd;
         ismrmrd_acq_head.idx.user[4]                = scanhead.scanHeader.sLC.ushIde;
         // TODO: remove this once the GTPlus can properly autodetect partial fourier
         ismrmrd_acq_head.idx.user[5]                = scanhead.scanHeader.ushKSpaceCentreLineNo;
         ismrmrd_acq_head.idx.user[6]                = scanhead.scanHeader.ushKSpaceCentrePartitionNo;

         /*****************************************************************************/
         /* the user_int[0] and user_int[1] are used to store user defined parameters */
         /*****************************************************************************/
         ismrmrd_acq_head.user_int[0]   = scanhead.scanHeader.aushIceProgramPara[0];
         ismrmrd_acq_head.user_int[1]   = scanhead.scanHeader.aushIceProgramPara[1];
         ismrmrd_acq_head.user_int[2]   = scanhead.scanHeader.aushIceProgramPara[2];
         ismrmrd_acq_head.user_int[3]   = scanhead.scanHeader.aushIceProgramPara[3];
         ismrmrd_acq_head.user_int[4]   = scanhead.scanHeader.aushIceProgramPara[4];
         ismrmrd_acq_head.user_int[5]   = scanhead.scanHeader.aushIceProgramPara[5];
         ismrmrd_acq_head.user_int[6]   = scanhead.scanHeader.aushIceProgramPara[6];
         ismrmrd_acq_head.user_int[7]   = scanhead.scanHeader.aushIceProgramPara[7];

         ismrmrd_acq_head.user_float[0] = scanhead.scanHeader.aushIceProgramPara[8];
         ismrmrd_acq_head.user_float[1] = scanhead.scanHeader.aushIceProgramPara[9];
         ismrmrd_acq_head.user_float[2] = scanhead.scanHeader.aushIceProgramPara[10];
         ismrmrd_acq_head.user_float[3] = scanhead.scanHeader.aushIceProgramPara[11];
         ismrmrd_acq_head.user_float[4] = scanhead.scanHeader.aushIceProgramPara[12];
         ismrmrd_acq_head.user_float[5] = scanhead.scanHeader.aushIceProgramPara[13];
         ismrmrd_acq_head.user_float[6] = scanhead.scanHeader.aushIceProgramPara[14];
         ismrmrd_acq_head.user_float[7] = scanhead.scanHeader.aushIceProgramPara[15];

         ismrmrd_acq->setHead(ismrmrd_acq_head);

         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 25)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_NOISE_MEASUREMENT));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 28)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_FIRST_IN_SLICE));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 29)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_SLICE));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 11)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_REPETITION));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 22)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PARALLEL_CALIBRATION));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 23)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PARALLEL_CALIBRATION_AND_IMAGING));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 24)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_REVERSE));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 11)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_MEASUREMENT));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 21)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PHASECORR_DATA));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 1)))    ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_NAVIGATION_DATA));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 1)))    ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_RTFEEDBACK_DATA));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 2)))    ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_HPFEEDBACK_DATA));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 51)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_DUMMYSCAN_DATA));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 10)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_SURFACECOILCORRECTIONSCAN_DATA));
         if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 5)))    ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_DUMMYSCAN_DATA));
         // if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1ULL << 1))) ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_REPETITION));

         //This memory will be deleted by the ISMRMRD::Acquisition object
         //ismrmrd_acq->data_ = new float[ismrmrd_acq->head_.number_of_samples*ismrmrd_acq->head_.active_channels*2];

         if ((flash_pat_ref_scan) & (ismrmrd_acq->isFlagSet(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PARALLEL_CALIBRATION))))
         {
             // For some sequences the PAT Reference data is collected using a different encoding space
             // e.g. EPI scans with FLASH PAT Reference
             // enabled by command line option
             // TODO: it is likely that the dwell time is not set properly for this type of acquisition
        	 ismrmrd_acq->setEncodingSpaceRef(1);
         }

         if (trajectory == 4)
         { //Spiral, we will add the trajectory to the data
      	    std::valarray<float> traj_data = traj.data_;
 	        std::vector<unsigned int> traj_dims = traj.dimensions_;

 			if (!(ismrmrd_acq->isFlagSet(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_NOISE_MEASUREMENT))))
 			{ //Only when this is not noise
 				 unsigned long traj_samples_to_copy = ismrmrd_acq->getNumberOfSamples();
 				 if (traj_dims[0] < traj_samples_to_copy)
 				 {
 					 traj_samples_to_copy = (unsigned long)traj_dims[0];
 					 ismrmrd_acq->setDiscardPost((uint16_t)(ismrmrd_acq->getNumberOfSamples()-traj_samples_to_copy) );
 				 }
 				 ismrmrd_acq->setTrajectoryDimensions(2);
 				 float* t_ptr = reinterpret_cast<float*>(&traj_data[0] + (ismrmrd_acq->getIdx().kspace_encode_step_1 * traj_dims[0]));
 				 memcpy(const_cast<float*>(&ismrmrd_acq->getTraj()[0]), t_ptr, sizeof(float) * traj_samples_to_copy * ismrmrd_acq->getTrajectoryDimensions());
             }
         }

         sChannelHeader_with_data* channel_header = reinterpret_cast<sChannelHeader_with_data*>(scanhead.data.p);
         for (unsigned int c = 0; c < ismrmrd_acq->getActiveChannels(); c++)
         {
             std::complex<float>* dptr = reinterpret_cast< std::complex<float>* >(channel_header[c].data.p);
             memcpy(const_cast<float*>(&ismrmrd_acq->getData()[c*ismrmrd_acq->getNumberOfSamples()*2]), dptr, ismrmrd_acq->getNumberOfSamples()*sizeof(float)*2);
         }

         if (write_to_file)
         {
        	ISMRMRD::HDF5Exclusive lock;
 			if (ismrmrd_dataset->appendAcquisition(ismrmrd_acq) < 0)
 			{
 				std::cerr << "Error appending ISMRMRD Dataset" << std::endl;
 				clear_all_temp_files();
 				return -1;
 			}
         }

         if ( scanhead.scanHeader.ulScanCounter % 1000 == 0 ) {
             std::cout << "sending scan : " << scanhead.scanHeader.ulScanCounter << std::endl;
         }

         {
             ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
             ClearsScanHeader_with_data(&scanhead);
         }

     }//End of the while loop


     //Mystery bytes. There seems to be 160 mystery bytes at the end of the data.
     //We will store them in the HDF file in case we need them for creating a binary
     //identical dat file.
     //TODO:
     unsigned int mystery_bytes = (ParcFileEntries[0].off_+ParcFileEntries[0].len_)-f.tellg();

     std::cout << "MYSTERY BITES: " << mystery_bytes << std::endl;

     if (mystery_bytes > 0)
     {
         if (mystery_bytes != 160)
         {
             std::cout << "WARNING: An unexpected number of mystery bytes detected: " << mystery_bytes << std::endl;
             std::cout << "ParcFileEntries[i].off_ = " << ParcFileEntries[0].off_ << std::endl;
             std::cout << "ParcFileEntries[i].len_ = " << ParcFileEntries[0].len_ << std::endl;
             std::cout << "f.tellg() = " << f.tellg() << std::endl;
             throw std::runtime_error("Error caught while writing mystery bytes to HDF5 file.");
         }

         f.read(reinterpret_cast<char*>(&mystery_data), mystery_bytes);

         //After this we have to be on a 512 byte boundary
         if (f.tellg() % 512)
         {
             f.seekg(512-(f.tellg() % 512), std::ios::cur);
         }
     }

     size_t end_position = f.tellg();
     f.seekg(0,std::ios::end);
     size_t eof_position = f.tellg();
     if (end_position != eof_position)
     {
         size_t additional_bytes = eof_position-end_position;
         std::cout << "WARNING: End of file was not reached during conversion. There are "
                 << additional_bytes << " additional bytes at the end of file." << std::endl;
     }

     f.close();

     //remove(tempfile.c_str());
     clear_temp_files(download_xml, download_xsl, download_xml_raw, debug_xml);
     return 0;
}
