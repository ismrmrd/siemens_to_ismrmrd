#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_string.h"
#include "ace/Reactor.h"

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

#include "GadgetMessageInterface.h"
#include "GadgetMRIHeaders.h"
#include "siemensraw.h"
#include "GadgetronConnector.h"
#include "ImageWriter.h"
#include "hoNDArray.h"
#include "GadgetXml.h"
#include "XNode.h"
#include "FileInfo.h"

#include "siemens_hdf5_datatypes.h"
#include "HDF5ImageWriter.h"

#include "ismrmrd.h"
#include "ismrmrd_hdf5.h"

#include <H5Cpp.h>

#include <iomanip>

#define EXCLUDE_ISMRMRD_XSD
#include "GadgetIsmrmrdReadWrite.h"

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

using namespace Gadgetron;

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
    if (schema_doc == NULL) {
        /* the schema cannot be loaded or is not well-formed */
        return -1;
    }
    xmlSchemaParserCtxtPtr parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
    if (parser_ctxt == NULL) {
        /* unable to create a parser context for the schema */
        xmlFreeDoc(schema_doc);
        return -2;
    }
    xmlSchemaPtr schema = xmlSchemaParse(parser_ctxt);
    if (schema == NULL) {
        /* the schema itself is not valid */
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(schema_doc);
        return -3;
    }
    xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
    if (valid_ctxt == NULL) {
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
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -x <PARAMETER MAP STYLESHEET>  (default ./parammap.xsl)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -o <HDF5 dump file>            (default dump.h5)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -g <HDF5 dump group>           (/dataset)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -r <HDF5 result file>          (default result.h5)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -G <HDF5 result group>         (default date and time)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -c <GADGETRON CONFIG>          (default default.xml)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -w                             (write only flag, do not connect to Gadgetron)\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                  -X                             (Debug XML flag)\n") ));
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[] )
{
    char * gadgetron_home = ACE_OS::getenv("GADGETRON_HOME");

    if (std::string(gadgetron_home).size() == 0) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("GADGETRON_HOME variable not set.\n")),-1);
    }

    static const ACE_TCHAR options[] = ACE_TEXT(":p:h:f:d:o:c:m:x:g:r:G:wX");

    ACE_Get_Opt cmd_opts(argc, argv, options);

    ACE_TCHAR port_no[1024];
    ACE_OS_String::strncpy(port_no, "9002", 1024);

    ACE_TCHAR hostname[1024];
    ACE_OS_String::strncpy(hostname, "localhost", 1024);

    ACE_TCHAR filename[4096];
    ACE_OS_String::strncpy(filename, "./data.h5", 4096);

    ACE_TCHAR config_file[1024];
    ACE_OS_String::strncpy(config_file, "default.xml", 1024);

    ACE_TCHAR parammap_file[4096];
    ACE_OS::sprintf(parammap_file, "%s/schema/IsmrmrdParameterMap.xml", gadgetron_home);

    ACE_TCHAR parammap_xsl[4096];
    ACE_OS::sprintf(parammap_xsl, "%s/schema/IsmrmrdParameterMap.xsl", gadgetron_home);

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
    bool debug_xml = false;


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
            ACE_OS_String::strncpy(parammap_file, cmd_opts.opt_arg(), 4096);
            break;
        case 'x':
            ACE_OS_String::strncpy(parammap_xsl, cmd_opts.opt_arg(), 4096);
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
        case 'X':
            debug_xml = true;
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

    ACE_DEBUG(( LM_INFO, ACE_TEXT("Siemens MRI Client (for ISMRMRD format)\n") ));


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

    if (!FileInfo(std::string(parammap_xsl)).exists()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("Parameter map XSLT stylesheet %s does not exist.\n"), parammap_xsl));
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
    ACE_DEBUG((LM_INFO, ACE_TEXT("  -- parameter xsl :            %s\n"), parammap_xsl));


    boost::shared_ptr<ISMRMRD::IsmrmrdDataset>  ismrmrd_dataset;

    if (write_to_file) {
        ismrmrd_dataset = boost::shared_ptr<ISMRMRD::IsmrmrdDataset>(new ISMRMRD::IsmrmrdDataset(hdf5_file, hdf5_group));
    }


    //Get the HDF5 file opened.
    H5File hdf5file;

    MeasurementHeader mhead;
    {

        ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5

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
    std::vector<std::string> wip_long;
    std::vector<std::string> wip_double;
    long trajectory = 0;
    long dwell_time_0 = 0;
    long max_channels = 0;
    long radial_views = 0;
    std::string protocol_name = "";

    for (unsigned int b = 0; b < mhead.nr_buffers; b++) {
        if (std::string((char*)buffers[b].bufName_.p).compare("Meas") == 0) {
            std::string config_buffer((char*)buffers[b].buf_.p,buffers[b].buf_.len-2);//-2 because the last two character are ^@

            XProtocol::XNode n;

            if (debug_xml) {
                std::ofstream o("config_buffer.xprot");
                o.write(config_buffer.c_str(), config_buffer.size());
                o.close();
            }

            if (XProtocol::ParseXProtocol(const_cast<std::string&>(config_buffer),n) < 0) {
                ACE_DEBUG((LM_ERROR, ACE_TEXT("Failed to parse XProtocol")));
                return -1;
            }

            //Get some parameters - wip long
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sWipMemBlock.alFree"), n);
                if (n2) {
                    wip_long = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "Search path: MEAS.sWipMemBlock.alFree not found." << std::endl;
                }
                if (wip_long.size() == 0) {
                    std::cout << "Failed to find WIP long parameters" << std::endl;
                    return -1;
                }
            }

            //Get some parameters - wip long
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sWipMemBlock.adFree"), n);
                if (n2) {
                    wip_double = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "Search path: MEAS.sWipMemBlock.adFree not found." << std::endl;
                }
                if (wip_double.size() == 0) {
                    std::cout << "Failed to find WIP double parameters" << std::endl;
                    return -1;
                }
            }

            //Get some parameters - dwell times
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sRXSPEC.alDwellTime"), n);
                std::vector<std::string> temp;
                if (n2) {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "Search path: MEAS.sWipMemBlock.alFree not found." << std::endl;
                }
                if (temp.size() == 0) {
                    std::cout << "Failed to find dwell times" << std::endl;
                    return -1;
                } else {
                    dwell_time_0 = std::atoi(temp[0].c_str());
                }
            }


            //Get some parameters - trajectory
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.ucTrajectory"), n);
                std::vector<std::string> temp;
                if (n2) {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "Search path: MEAS.sKSpace.ucTrajectory not found." << std::endl;
                }
                if (temp.size() != 1) {
                    std::cout << "Failed to find appropriate trajectory array" << std::endl;
                    return -1;
                } else {
                    trajectory = std::atoi(temp[0].c_str());
                }
            }

            //Get some parameters - max channels
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("YAPS.iMaxNoOfRxChannels"), n);
                std::vector<std::string> temp;
                if (n2) {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "YAPS.iMaxNoOfRxChannels" << std::endl;
                }
                if (temp.size() != 1) {
                    std::cout << "Failed to find YAPS.iMaxNoOfRxChannels array" << std::endl;
                    return -1;
                } else {
                    max_channels = std::atoi(temp[0].c_str());
                }
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
                if (temp.size() != 1) {
                    std::cout << "Failed to find YAPS.MEAS.sKSpace.lRadialViews array" << std::endl;
                    return -1;
                } else {
                    radial_views = std::atoi(temp[0].c_str());
                }
            }

            //Get some parameters - protocol name
            {
                const XProtocol::XNode* n2 = boost::apply_visitor(XProtocol::getChildNodeByName("HEADER.tProtocolName"), n);
                std::vector<std::string> temp;
                if (n2) {
                    temp = boost::apply_visitor(XProtocol::getStringValueArray(), *n2);
                } else {
                    std::cout << "HEADER.tProtocolName not found" << std::endl;
                }
                if (temp.size() != 1) {
                    std::cout << "Failed to find HEADER.tProtocolName" << std::endl;
                    return -1;
                } else {
		    protocol_name = temp[0];
                }
            }


            xml_config = ProcessGadgetronParameterMap(n,parammap_file);
            break;
        }
    }

    std::cout << "Protocol Name: " << protocol_name << std::endl;

    if (debug_xml) {
        std::ofstream o("xml_raw.xml");
        o.write(xml_config.c_str(), xml_config.size());
        o.close();
    }


    //Get rid of dynamically allocated memory in header
    {
        ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
        ClearMeasurementHeader(&mhead);
    }


    ACE_TCHAR schema_file_name[4096];
    ACE_OS::sprintf(schema_file_name, "%s/schema/ismrmrd.xsd", gadgetron_home);

#ifndef WIN32
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;

    const char *params[16 + 1];
    int nbparams = 0;
    params[nbparams] = NULL;
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;

    cur = xsltParseStylesheetFile((const xmlChar*)parammap_xsl);
    doc = xmlParseMemory(xml_config.c_str(),xml_config.size());
    res = xsltApplyStylesheet(cur, doc, params);

    xmlChar* out_ptr = NULL;
    int xslt_length = 0;
    int xslt_result = xsltSaveResultToString(&out_ptr,
                         &xslt_length,
                         res,
                         cur);

    if (xslt_result < 0) {
        std::cout << "Failed to save converted doc to string" << std::endl;
        return -1;
    }

    xml_config = std::string((char*)out_ptr,xslt_length);


    if (!FileInfo(std::string(schema_file_name)).exists()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("ISMRMRD schema file %s does not exist.\n"), schema_file_name));
        return -1;
    }

    if (debug_xml) {
        std::ofstream o("processed.xml");
        o.write(xml_config.c_str(), xml_config.size());
        o.close();
    }

    if (xml_file_is_valid(xml_config,schema_file_name) <= 0) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("Generated XML is not valid according to the ISMRMRD schema %s.\n"), schema_file_name));
        return -1;
    }


    xsltFreeStylesheet(cur);
    xmlFreeDoc(res);
    xmlFreeDoc(doc);

    xsltCleanupGlobals();
    xmlCleanupParser();
#else
    std::ofstream o("xml_pre.xml");
    o.write(xml_config.c_str(), xml_config.size());
    o.close();

    std::string syscmd = std::string("xsltproc --output xml_post.xml \"") + std::string(parammap_xsl) + std::string("\" xml_pre.xml");
    system(syscmd.c_str());

    std::ifstream t("xml_post.xml");
    xml_config = std::string((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
#endif //WIN32

    ISMRMRD::AcquisitionHeader acq_head_base;
    memset(&acq_head_base, 0, sizeof(ISMRMRD::AcquisitionHeader) );

    if (write_to_file) {
        ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
        if (ismrmrd_dataset->writeHeader(xml_config) < 0 ) {
            std::cerr << "Failed to write XML header to HDF file" << std::endl;
            return -1;
        }
    }


    GadgetronConnector con;
    if (!write_to_file_only) {

        //con.register_writer(GADGET_MESSAGE_ACQUISITION, new GadgetAcquisitionMessageWriter());
        con.register_writer(GADGET_MESSAGE_ISMRMRD_ACQUISITION, new GadgetIsmrmrdAcquisitionMessageWriter());
        con.register_reader(GADGET_MESSAGE_ISMRMRD_IMAGE_REAL_USHORT, new HDF5ImageWriter<ACE_UINT16>(std::string(hdf5_out_file), std::string(hdf5_out_group)));
        con.register_reader(GADGET_MESSAGE_ISMRMRD_IMAGE_REAL_FLOAT, new HDF5ImageWriter<float>(std::string(hdf5_out_file), std::string(hdf5_out_group)));
        con.register_reader(GADGET_MESSAGE_ISMRMRD_IMAGE_CPLX_FLOAT, new HDF5ImageWriter< std::complex<float> >(std::string(hdf5_out_file), std::string(hdf5_out_group)));

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

        ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5

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


    //If this is a spiral acquisition, we will calculate the trajectory and add it to the individual profiles
    boost::shared_ptr< hoNDArray<floatd2> > traj;
    if (trajectory == 4) {
        int     nfov   = 1;         /*  number of fov coefficients.             */
        int     ngmax  = 1e5;       /*  maximum number of gradient samples      */
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
        calc_vds(smax,gmax,sample_time,sample_time,interleaves,&fov,nfov,krmax,ngmax,&xgrad,&ygrad,&ngrad);

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

        traj = boost::shared_ptr< hoNDArray<floatd2> >(new hoNDArray<floatd2>);

        std::vector<unsigned int> trajectory_dimensions;
        trajectory_dimensions.push_back(ngrad);
        trajectory_dimensions.push_back(interleaves);

        traj->create(&trajectory_dimensions);

        float* co_ptr = reinterpret_cast<float*>(traj->get_data_ptr());

        for (int i = 0; i < (ngrad*interleaves); i++) {
            co_ptr[i*2]   = -x_trajectory[i]/2;
            co_ptr[i*2+1] = -y_trajectory[i]/2;
        }

        delete [] xgrad;
        delete [] ygrad;
        delete [] x_trajectory;
        delete [] y_trajectory;
        delete [] weighting;
    }

    for (unsigned int a = 0; a < acquisitions; a++) {
        sScanHeader_with_data scanhead;


        offset[0] = a;

        {
            ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5

            DataSpace space = rawdataset.getSpace();
            space.selectHyperslab(H5S_SELECT_SET, &single_scan_dims[0], &offset[0]);

            DataSpace memspace(2,&single_scan_dims[0]);

            rawdataset.read( &scanhead, *rawdatatype, memspace, space);
        }

        if (scanhead.scanHeader.aulEvalInfoMask[0] & 1) {
            std::cout << "Last scan reached..." << std::endl;
            ISMRMRD::HDF5Exclusive lock;
            ClearsScanHeader_with_data(&scanhead);
            break;
        }



        GadgetContainerMessage<GadgetMessageIdentifier>* m1 =
                new GadgetContainerMessage<GadgetMessageIdentifier>();

        m1->getObjectPtr()->id = GADGET_MESSAGE_ISMRMRD_ACQUISITION;

        GadgetContainerMessage<ISMRMRD::Acquisition>* m2 =
                new GadgetContainerMessage<ISMRMRD::Acquisition>();

        ISMRMRD::Acquisition* ismrmrd_acq = m2->getObjectPtr();
		ISMRMRD::AcquisitionHeader ismrmrd_acq_head;		

		memset(&ismrmrd_acq_head,0,sizeof(ismrmrd_acq_head));

		ismrmrd_acq_head.measurement_uid 				= scanhead.scanHeader.lMeasUID;
		ismrmrd_acq_head.scan_counter					= scanhead.scanHeader.ulScanCounter;
		ismrmrd_acq_head.acquisition_time_stamp		    = scanhead.scanHeader.ulTimeStamp;
		ismrmrd_acq_head.physiology_time_stamp[0]		= scanhead.scanHeader.ulPMUTimeStamp;
		ismrmrd_acq_head.number_of_samples 			= scanhead.scanHeader.ushSamplesInScan;
		ismrmrd_acq_head.available_channels			= max_channels;
		ismrmrd_acq_head.active_channels 				= scanhead.scanHeader.ushUsedChannels;
           uint64_t           channel_mask[16];               //Mask to indicate which channels are active. Support for 1024 channels
		ismrmrd_acq_head.discard_pre					= scanhead.scanHeader.sCutOff.ushPre;
		ismrmrd_acq_head.discard_post					= scanhead.scanHeader.sCutOff.ushPost;
		ismrmrd_acq_head.center_sample					= scanhead.scanHeader.ushKSpaceCentreColumn;
		ismrmrd_acq_head.encoding_space_ref            = 0;
		ismrmrd_acq_head.trajectory_dimensions         = 0;
		if (scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 25) && (protocol_name.compare("AdjCoilSens") != 0)) { //This is noise
			ismrmrd_acq_head.sample_time_us                =  7680.0f/ismrmrd_acq_head.number_of_samples;
        } else {
			ismrmrd_acq_head.sample_time_us                = dwell_time_0 / 1000.0;

        }
		ismrmrd_acq_head.position[0]              		= scanhead.scanHeader.sSliceData.sSlicePosVec.flSag;
		ismrmrd_acq_head.position[1]              		= scanhead.scanHeader.sSliceData.sSlicePosVec.flCor;
		ismrmrd_acq_head.position[2]              		= scanhead.scanHeader.sSliceData.sSlicePosVec.flTra;

		// Convert Siemens quaternions to direction cosines.
		// In the Siemens convention the quaternion corresponds to a rotation matrix with columns P R S
                // Siemens stores the quaternion as (W,X,Y,Z)
                float quat[4];
                quat[0] = scanhead.scanHeader.sSliceData.aflQuaternion[1]; // X
                quat[1] = scanhead.scanHeader.sSliceData.aflQuaternion[2]; // Y
                quat[2] = scanhead.scanHeader.sSliceData.aflQuaternion[3]; // Z
                quat[3] = scanhead.scanHeader.sSliceData.aflQuaternion[0]; // W
		ISMRMRD::quaternion_to_directions( quat,
					  ismrmrd_acq_head.phase_dir,
					  ismrmrd_acq_head.read_dir,
					  ismrmrd_acq_head.slice_dir);

		ismrmrd_acq_head.patient_table_position[0]   		= scanhead.scanHeader.lPTABPosX;
		ismrmrd_acq_head.patient_table_position[1]   		= scanhead.scanHeader.lPTABPosY;
		ismrmrd_acq_head.patient_table_position[2]   		= scanhead.scanHeader.lPTABPosZ;

		ismrmrd_acq_head.idx.average						= scanhead.scanHeader.sLC.ushAcquisition;
		ismrmrd_acq_head.idx.contrast						= scanhead.scanHeader.sLC.ushEcho;
		ismrmrd_acq_head.idx.kspace_encode_step_1          = scanhead.scanHeader.sLC.ushLine;
		ismrmrd_acq_head.idx.kspace_encode_step_2			= scanhead.scanHeader.sLC.ushPartition;
		ismrmrd_acq_head.idx.phase							= scanhead.scanHeader.sLC.ushPhase;
		ismrmrd_acq_head.idx.repetition					= scanhead.scanHeader.sLC.ushRepetition;
		ismrmrd_acq_head.idx.segment						= scanhead.scanHeader.sLC.ushSeg;
		ismrmrd_acq_head.idx.set							= scanhead.scanHeader.sLC.ushSet;
		ismrmrd_acq_head.idx.slice							= scanhead.scanHeader.sLC.ushSlice;
		ismrmrd_acq_head.idx.user[0]						= scanhead.scanHeader.sLC.ushIda;
		ismrmrd_acq_head.idx.user[1]						= scanhead.scanHeader.sLC.ushIdb;
		ismrmrd_acq_head.idx.user[2]						= scanhead.scanHeader.sLC.ushIdc;
		ismrmrd_acq_head.idx.user[3]						= scanhead.scanHeader.sLC.ushIdd;
		ismrmrd_acq_head.idx.user[4]						= scanhead.scanHeader.sLC.ushIde;
        ismrmrd_acq_head.idx.user[5]						= scanhead.scanHeader.ushKSpaceCentreLineNo;
        ismrmrd_acq_head.idx.user[6]						= scanhead.scanHeader.ushKSpaceCentrePartitionNo;
        //   int32_t            user_int[8];                    //Free user parameters
        //   float              user_float[8];                  //Free user parameters

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

        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 25)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_NOISE_MEASUREMENT));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 28)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_FIRST_IN_SLICE));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 29)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_SLICE));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 11)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_REPETITION));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 22)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PARALLEL_CALIBRATION));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 23)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PARALLEL_CALIBRATION_AND_IMAGING));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 24)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_REVERSE));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 11)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_LAST_IN_MEASUREMENT));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 21)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_PHASECORR_DATA));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 1)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_NAVIGATION_DATA));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 1)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_RTFEEDBACK_DATA));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 2)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_HPFEEDBACK_DATA));
        if ((scanhead.scanHeader.aulEvalInfoMask[0] & (1 << 51)))   ismrmrd_acq->setFlag(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_DUMMYSCAN_DATA));

        //This memory will be deleted by the ISMRMRD::Acquisition object
		//ismrmrd_acq->data_ = new float[ismrmrd_acq->head_.number_of_samples*ismrmrd_acq->head_.active_channels*2];

        if (trajectory == 4) { //Spiral, we will add the trajectory to the data

            if (!(ismrmrd_acq->isFlagSet(ISMRMRD::FlagBit(ISMRMRD::ACQ_IS_NOISE_MEASUREMENT)))) { //Only when this is not noise
			  unsigned long traj_samples_to_copy = ismrmrd_acq->getNumberOfSamples();//head_.number_of_samples;
                if (traj->get_size(0) < traj_samples_to_copy) {
                    traj_samples_to_copy = traj->get_size(0);
					ismrmrd_acq->setDiscardPost(ismrmrd_acq->getNumberOfSamples()-traj_samples_to_copy);
                }
				ismrmrd_acq->setTrajectoryDimensions(2);
				//ismrmrd_acq->traj_ = new float[ismrmrd_acq->head_.number_of_samples*ismrmrd_acq->head_.trajectory_dimensions];
				float* t_ptr = reinterpret_cast<float*>(traj->get_data_ptr() + (ismrmrd_acq->getIdx().kspace_encode_step_1 * traj->get_size(0)));
				//memset(ismrmrd_acq->traj_,0,sizeof(float)*ismrmrd_acq->head_.number_of_samples*ismrmrd_acq->head_.trajectory_dimensions);
				memcpy(const_cast<float*>(&ismrmrd_acq->getTraj()[0]),t_ptr, sizeof(float)*traj_samples_to_copy*ismrmrd_acq->getTrajectoryDimensions());
            }
        }

        sChannelHeader_with_data* channel_header = reinterpret_cast<sChannelHeader_with_data*>(scanhead.data.p);
		for (unsigned int c = 0; c < m2->getObjectPtr()->getActiveChannels(); c++) {
            std::complex<float>* dptr = reinterpret_cast< std::complex<float>* >(channel_header[c].data.p);
			memcpy(const_cast<float*>(&ismrmrd_acq->getData()[c*ismrmrd_acq->getNumberOfSamples()*2]),
			       dptr, ismrmrd_acq->getNumberOfSamples()*sizeof(float)*2);
        }


        if (write_to_file) {
            ISMRMRD::HDF5Exclusive lock;
            if (ismrmrd_dataset->appendAcquisition(ismrmrd_acq) < 0) {
                std::cerr << "Error appending ISMRMRD Dataset" << std::endl;
                return -1;
            }
        }

        if ( scanhead.scanHeader.ulScanCounter % 1000 == 0 )
        {
            std::cout << "sending scan : " << scanhead.scanHeader.ulScanCounter << std::endl;
        }

        //Chain the message block together.
        m1->cont(m2);
        if (!write_to_file_only) {
            if (con.putq(m1) == -1) {
                ACE_DEBUG((LM_ERROR, ACE_TEXT("Unable to put data package on queue")));
                return -1;
            }
        } else {
            m1->release();
        }

        {
            ISMRMRD::HDF5Exclusive lock; //This will ensure threadsafe access to HDF5
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
