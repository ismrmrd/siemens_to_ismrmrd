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
#include <boost/make_shared.hpp>

#include "siemensraw.h"
#include "base64.h"
#include "XNode.h"
#include "ConverterXml.h"

#include "ismrmrd/ismrmrd.h"
#include "ismrmrd/dataset.h"
#include "ismrmrd/version.h"
#include "ismrmrd/xml.h"
#include "converter_version.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/filesystem.hpp>
#include <boost/locale/encoding_utf.hpp>
using boost::locale::conv::utf_to_utf;

#include <iomanip>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <utility>
#include <typeinfo>

const size_t MYSTERY_BYTES_EXPECTED = 160;

// defined in generated defaults.cpp
extern void initializeEmbeddedFiles(void);
extern std::map<std::string, std::string> global_embedded_files;


struct ChannelHeaderAndData
{
    sChannelHeader header;
    std::vector<complex_float_t> data;
};

struct MeasurementHeaderBuffer
{
    std::string name;
    std::string buf;
};

void calc_vds(double slewmax,double gradmax,double Tgsample,double Tdsample,int Ninterleaves,
              double* fov, int numfov,double krmax,
              int ngmax, double** xgrad,double** ygrad,int* numgrad);


void calc_traj(double* xgrad, double* ygrad, int ngrad, int Nints, double Tgsamp, double krmax,
               double** x_trajectory, double** y_trajectory,
               double** weights);


std::vector<ISMRMRD::Waveform> readSyncdata(std::ifstream &siemens_dat, bool VBFILE, unsigned long acquisitions,
                                            uint32_t dma_length, sScanHeader scanheader, ISMRMRD::IsmrmrdHeader &header,
                                            long scan_counter, bool skip_syncdata);

std::string select_file(const std::string &, const std::string &, bool, unsigned int);
std::string get_file_content(const std::string &file);


std::vector<MrParcRaidFileEntry>
readParcFileEntries(std::ifstream &siemens_dat, const MrParcRaidFileHeader &ParcRaidHead, bool VBFILE);

std::vector<MeasurementHeaderBuffer> readMeasurementHeaderBuffers(std::ifstream &siemens_dat, uint32_t num_buffers);

std::string readXmlConfig(bool debug_xml, const std::string &parammap_file_content, uint32_t num_buffers,
                          std::vector<MeasurementHeaderBuffer> &buffers, std::vector<std::string> &wip_double,
                          Trajectory &trajectory, long &dwell_time_0, long &max_channels, long &radial_views, long* global_table_pos,
                          std::string &baseLine_string, std::string &protocol_name, std::string& software_version);

std::string parseXML(bool debug_xml, const std::string &parammap_xsl_content, std::string &schema_file_name_content,
                     const std::string xml_config);

ISMRMRD::NDArray<float>
getTrajectory(const std::vector<std::string> &wip_double, const Trajectory &trajectory, long dwell_time_0,
              long radial_views);


ISMRMRD::Acquisition
getAcquisition(bool flash_pat_ref_scan, const Trajectory &trajectory, long dwell_time_0, long* global_table_pos, long max_channels,
               bool isAdjustCoilSens, bool isAdjQuietCoilSens, bool isVB, bool isNX, ISMRMRD::NDArray<float> &traj,
               const sScanHeader &scanhead, const std::vector<ChannelHeaderAndData> &channels);

void readScanHeader(std::ifstream &siemens_dat, bool VBFILE, sMDH &mdh, sScanHeader &scanhead);

std::vector<ChannelHeaderAndData>
readChannelHeaders(std::ifstream &siemens_dat, bool VBFILE, const sScanHeader &scanhead);

int xml_file_is_valid(std::string &xml, std::string &schema_file) {
    xmlDocPtr doc;
    //parse an XML in-memory block and build a tree.
    doc = xmlParseMemory(xml.c_str(), xml.size());

    xmlDocPtr schema_doc;
    //parse an XML in-memory block and build a tree.
    schema_doc = xmlParseMemory(schema_file.c_str(), schema_file.size());

    //Create an XML Schemas parse context for that document. NB. The document may be modified during the parsing process.
    xmlSchemaParserCtxtPtr parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
    if (parser_ctxt == NULL) {
        /* unable to create a parser context for the schema */
        xmlFreeDoc(schema_doc);
        return -2;
    }

    //parse a schema definition resource and build an internal XML Shema structure which can be used to validate instances.
    xmlSchemaPtr schema = xmlSchemaParse(parser_ctxt);
    if (schema == NULL) {
        /* the schema itself is not valid */
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(schema_doc);
        return -3;
    }

    //Create an XML Schemas validation context based on the given schema.
    xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
    if (valid_ctxt == NULL) {
        /* unable to create a validation context for the schema */
        xmlSchemaFree(schema);
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(schema_doc);
        xmlFreeDoc(doc);
        return -4;
    }

    //Validate a document tree in memory. Takes a schema validation context and a parsed document tree
    int is_valid = (xmlSchemaValidateDoc(valid_ctxt, doc) == 0);
    xmlSchemaFreeValidCtxt(valid_ctxt);
    xmlSchemaFree(schema);
    xmlSchemaFreeParserCtxt(parser_ctxt);
    xmlFreeDoc(schema_doc);
    xmlFreeDoc(doc);

    /* force the return value to be non-negative on success */
    return is_valid ? 1 : 0;
}


std::string get_date_time_string() {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    std::stringstream str;
    str << timeinfo->tm_year + 1900 << "-"
        << std::setw(2) << std::setfill('0') << timeinfo->tm_mon + 1
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


bool is_number(const std::string &s) {
    bool ret = true;
    for (unsigned int i = 0; i < s.size(); i++) {
        if (!std::isdigit(s.c_str()[i])) {
            ret = false;
            break;
        }
    }
    return ret;
}

std::string get_time_string(size_t hours, size_t mins, size_t secs) {
    std::stringstream str;
    str << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << mins << ":"
        << std::setw(2) << std::setfill('0') << secs;

    std::string ret = str.str();

    return ret;
}

bool fill_ismrmrd_header(ISMRMRD::IsmrmrdHeader &h, const std::string &study_date, const std::string &study_time) {
    try {

        // ---------------------------------
        // fill more info into the ismrmrd header
        // ---------------------------------
        // study
        bool study_date_needed = false;
        bool study_time_needed = false;

        if (h.studyInformation) {
            if (!h.studyInformation->studyDate) {
                study_date_needed = true;
            }

            if (!h.studyInformation->studyTime) {
                study_time_needed = true;
            }
        } else {
            study_date_needed = true;
            study_time_needed = true;
        }

        if (study_date_needed || study_time_needed) {
            ISMRMRD::StudyInformation study;

            if(h.studyInformation)
                study = *h.studyInformation;

            if(study_date_needed && !study_date.empty())
            {
                study.studyDate.set(study_date);
                std::cout << "Study date: " << study_date << std::endl;
            }

            if (study_time_needed && !study_time.empty()) {
                study.studyTime.set(study_time);
                std::cout << "Study time: " << study_time << std::endl;
            }

            h.studyInformation.set(study);
        }

        // ---------------------------------
        // go back to string
        // ---------------------------------


    }
    catch (...) {
        return false;
    }

    return true;
}

void append_buffers_to_xml_header(std::vector<MeasurementHeaderBuffer> &buffers, size_t num_buffers,
                                  ISMRMRD::IsmrmrdHeader &header) {

    for (unsigned int b = 0; b < num_buffers; b++) {
        ISMRMRD::UserParameterString p;
        p.value = base64_encode(reinterpret_cast<const unsigned char *>(buffers[b].buf.c_str()), buffers[b].buf.size());
        p.name = std::string("SiemensBuffer_") + buffers[b].name;
        if (!header.userParameters.is_present()) {
            ISMRMRD::UserParameters up;
            header.userParameters = up;
        }
        header.userParameters().userParameterBase64.push_back(p);
    }

}

std::string ProcessParameterMap(const XProtocol::XNode &node, const char *mapfile) {
    TiXmlDocument out_doc;

    TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
    out_doc.LinkEndChild(decl);

    ConverterXMLNode out_n(&out_doc);

    //Input document
    TiXmlDocument doc;
    doc.Parse(mapfile);
    TiXmlHandle docHandle(&doc);

    TiXmlElement *parameters = docHandle.FirstChildElement("siemens").FirstChildElement("parameters").ToElement();
    if (parameters) {
        TiXmlNode *p = 0;
        while ((p = parameters->IterateChildren("p", p))) {
            TiXmlHandle ph(p);

            TiXmlText *s = ph.FirstChildElement("s").FirstChild().ToText();
            TiXmlText *d = ph.FirstChildElement("d").FirstChild().ToText();

            if (s && d) {
                std::string source = s->Value();
                std::string destination = d->Value();

                std::vector<std::string> split_path;
                boost::split(split_path, source, boost::is_any_of("."), boost::token_compress_on);

                if (is_number(split_path[0])) {
                    std::cout << "First element of path (" << source << ") cannot be numeric" << std::endl;
                    continue;
                }

                std::string search_path = split_path[0];
                for (unsigned int i = 1; i < split_path.size() - 1; i++) {
                    /*
                    if (is_number(split_path[i]) && (i != split_path.size())) {
                    std::cout << "Numeric index not supported inside path for source = " << source << std::endl;
                    continue;
                    }*/

                    search_path += std::string(".") + split_path[i];
                }

                int index = -1;
                if (is_number(split_path[split_path.size() - 1])) {
                    index = atoi(split_path[split_path.size() - 1].c_str());
                } else {
                    search_path += std::string(".") + split_path[split_path.size() - 1];
                }

                const XProtocol::XNode *n = boost::apply_visitor(XProtocol::getChildNodeByName(search_path), node);

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
                        std::cout << "Parameter index (" << index << ") not valid for search path " << search_path
                                  << std::endl;
                        continue;
                    }
                } else {
                    out_n.add(destination, parameters);
                }
            } else {
                std::cout << "Malformed parameter map" << std::endl;
            }
        }
    } else {
        std::cout << "Malformed parameter map (parameters section not found)" << std::endl;
        return std::string("");
    }
    return XmlToString(out_doc);
}


/// compute noise dwell time in us for dependency and built-in noise in VD/VB lines
double compute_noise_sample_in_us(size_t num_of_noise_samples_this_acq, bool isAdjustCoilSens, bool isAdjQuietCoilSens,
                                  bool isVB, bool isNX)
{
    if(isNX)
    {
        return 5.0;
    }
    else if (isAdjustCoilSens)
    {
        return 5.0;
    }
    else if (isAdjQuietCoilSens)
    {
        return 4.0;
    }
    else if (isVB)
    {
        return (1e6 / num_of_noise_samples_this_acq / 130.0);
    }
    else
    {
        return (((long) (76800.0 / num_of_noise_samples_this_acq)) / 10.0);
    }

    return 5.0;
}

std::string load_embedded(std::string name) {
    std::string contents;
    std::map<std::string, std::string>::iterator it = global_embedded_files.find(name);
    if (it != global_embedded_files.end()) {
        std::string encoded = it->second;
        contents = base64_decode(encoded);
    } else {
        std::stringstream sstream;
        sstream << "ERROR: File " << name << " is not embedded!";
        throw std::runtime_error(sstream.str());
    }
    return contents;
}

std::string load_file(std::string file_name) {
    // Read in file contents
    std::string contents;

    std::ifstream f(file_name.c_str());
    if (!f) {
        std::stringstream sstream;
        sstream << "file: " << file_name << " does not exist.";
        throw std::runtime_error(sstream.str());
    }
    std::string str_f((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    contents = str_f;

    return contents;
}


std::string ws2s(const std::wstring &wstr) {
    std::string ret(wstr.size(), '0');
    for (size_t i = 0; i < wstr.size(); i++) {
        wchar_t c = wstr[i];
        if (((uint32_t) c) > 127) {
            ret[i] = 'X';
        } else {
            ret[i] = static_cast<char>(c);
        }
    }
    return ret;
}

int main(int argc, char* argv[]) {
    std::string siemens_dat_filename;
    int measurement_number;

    std::string parammap_file;
    std::string parammap_xsl;

    std::string usermap_file;
    std::string usermap_xsl;

    std::string schema_file_name;

    std::string ismrmrd_group;
    std::string date_time = get_date_time_string();

    std::string study_date_user_supplied;

    bool debug_xml = false;
    bool flash_pat_ref_scan = false;
    bool header_only = false;
    bool append_buffers = false;
    bool all_measurements = false;
    bool multi_meas_file = false;
    bool skip_syncdata = false;

    bool list = false;
    std::string to_extract;

    std::string xslt_home;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce HELP message")
        ("version,v", "Prints converter version and ISMRMRD version")
        ("file,f", po::value<std::string>(&siemens_dat_filename), "<SIEMENS dat file>")
        ("measNum,z", po::value<int>(&measurement_number)->default_value(1), "<Measurement number (with negative indexing)>")
        ("allMeas,Z", po::value<bool>(&all_measurements)->implicit_value(true), "<All measurements flag>")
        ("multiMeasFile,M", po::value<bool>(&multi_meas_file)->implicit_value(true), "<Multiple measurements in single output file flag>")
        ("skipSyncData", po::value<bool>(&skip_syncdata)->implicit_value(true), "<Skip syncdata (PMU) conversion>")
        ("pMap,m", po::value<std::string>(&parammap_file), "<Parameter map XML file>")
        ("pMapStyle,x", po::value<std::string>(&parammap_xsl), "<Parameter stylesheet XSL file>")
        ("user-map", po::value<std::string>(&usermap_file), "<Provide a parameter map XML file>")
        ("user-stylesheet", po::value<std::string>(&usermap_xsl), "<Provide a parameter stylesheet XSL file>")
        ("output,o", po::value<std::string>(), "<ISMRMRD output file (defaults to the input file name, with .mrd extension)>")
        ("outputGroup,g", po::value<std::string>(&ismrmrd_group)->default_value("dataset"),
            "<ISMRMRD output group>")
            ("list,l", po::value<bool>(&list)->implicit_value(true), "<List embedded files>")
        ("extract,e", po::value<std::string>(&to_extract), "<Extract embedded file>")
        ("debug,X", po::value<bool>(&debug_xml)->implicit_value(true), "<Debug XML flag>")
        ("flashPatRef,F", po::value<bool>(&flash_pat_ref_scan)->implicit_value(true), "<FLASH PAT REF flag>")
        ("headerOnly,H", po::value<bool>(&header_only)->implicit_value(true),
            "<HEADER ONLY flag (create xml header only)>")
            ("bufferAppend,B", po::value<bool>(&append_buffers)->implicit_value(true),
                "<Append Siemens protocol buffers (bas64) to user parameters>")
                ("studyDate", po::value<std::string>(&study_date_user_supplied),
                    "<User can supply study date, in the format of yyyy-mm-dd>");

    po::options_description display_options("Allowed options");
    display_options.add_options()
        ("help,h", "Produce HELP message")
        ("version,v", "Prints converter version and ISMRMRD version")
        ("file,f", "<SIEMENS dat file>")
        ("measNum,z", "<Measurement number>")
        ("allMeas,Z", "<All measurements flag>")
        ("multiMeasFile,M", "<Multiple measurements in single file flag>")
        ("skipSyncData", "<Skip syncdata (PMU) conversion>")
        ("pMap,m", "<Parameter map XML>")
        ("pMapStyle,x", "<Parameter stylesheet XSL>")
        ("output,o", "<ISMRMRD output file>")
        ("outputGroup,g", "<ISMRMRD output group>")
        ("list,l", "<List embedded files>")
        ("extract,e", "<Extract embedded file>")
        ("debug,X", "<Debug XML flag>")
        ("flashPatRef,F", "<FLASH PAT REF flag>")
        ("headerOnly,H", "<HEADER ONLY flag (create xml header only)>")
        ("bufferAppend,B", "<Append protocol buffers>")
        ("studyDate", "<User can supply study date, in the format of yyyy-mm-dd>");

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << display_options << "\n";
            return 0;
        }

        if (vm.count("version")) {
            std::cout << "Converter version is: " << SIEMENS_TO_ISMRMRD_VERSION_MAJOR << "."
                << SIEMENS_TO_ISMRMRD_VERSION_MINOR << "." << SIEMENS_TO_ISMRMRD_VERSION_PATCH << "\n";
            std::cout << "Built against ISMRMRD version: " << ISMRMRD_VERSION_MAJOR << "." << ISMRMRD_VERSION_MINOR
                << "." << ISMRMRD_VERSION_PATCH << "\n";
            return 0;
        }
    }

    catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << display_options << std::endl;
        return -1;
    }

    if (!usermap_file.empty()) {
        if (!parammap_file.empty()) throw std::runtime_error("Specifying both --user-map and -m is not allowed.");

        std::cout << "WARNING: Specifying --user-map is deprecated; use -m instead." << std::endl;
        parammap_file = usermap_file;
    }

    if (!usermap_xsl.empty()) {
        if (!parammap_xsl.empty()) throw std::runtime_error("Specifying both --user-stylesheet and -x is not allowed.");

        std::cout << "WARNING: Specifying --user-stylesheet is deprecated; use -x instead." << std::endl;
        parammap_xsl = usermap_xsl;
    }

    // Add all embedded files to the global_embedded_files map
    initializeEmbeddedFiles();

    // List embedded parameter maps if requested
    if (list) {
        std::map<std::string, std::string>::iterator iter;
        std::cout << "Embedded Files: " << std::endl;
        for (iter = global_embedded_files.begin(); iter != global_embedded_files.end(); ++iter) {
            if (iter->first != "ismrmrd.xsd") {
                std::cout << "    " << iter->first << std::endl;
            }
        }
        return 0;
    }

    // Extract specified parameter map if requested
    if (to_extract.length() > 0) {
        std::string contents = load_embedded(to_extract);
        std::ofstream outfile(to_extract.c_str());
        outfile.write(contents.c_str(), contents.size());
        std::cout << to_extract << " successfully extracted. " << std::endl;
        return 0;
    }

    if (measurement_number == 0) {
        std::cerr << "The measurement number must not be zero (count starts at 1)" << std::endl;
        std::cerr << display_options << "\n";
        return -1;
    }

    // Siemens file must be specified
    if (siemens_dat_filename.length() == 0) {
        std::cerr << "Missing Siemens DAT filename" << std::endl;
        std::cerr << display_options << "\n";
        return -1;
    }

    // Check if Siemens file is valid
    std::ifstream infile(siemens_dat_filename.c_str());
    if (!infile) {
        std::cerr << "Provided Siemens file can not be open or does not exist." << std::endl;
        std::cerr << display_options << "\n";
        return -1;
    }
    std::cout << "Siemens file is: " << siemens_dat_filename << std::endl;

    std::string ismrmrd_file;
    if (!vm.count("output"))
    {
        boost::filesystem::path siemens_dat_path(siemens_dat_filename);
        ismrmrd_file = siemens_dat_path.replace_extension(".mrd").string();
        std::cout << "Output file not specified -- using " << ismrmrd_file << std::endl;
    } else {
        ismrmrd_file = vm["output"].as<std::string>();
    }

    std::string schema_file_name_content = load_embedded("ismrmrd.xsd");

    std::ifstream siemens_dat(siemens_dat_filename.c_str(), std::ios::binary);

    MrParcRaidFileHeader ParcRaidHead;

    siemens_dat.read((char*)(&ParcRaidHead), sizeof(MrParcRaidFileHeader));

    bool VBFILE = false;

    if (ParcRaidHead.hdSize_ > 32) {
        VBFILE = true;

        //Rewind, we have no raid file header.
        siemens_dat.seekg(0, std::ios::beg);

        ParcRaidHead.hdSize_ = ParcRaidHead.count_;
        ParcRaidHead.count_ = 1;
    }
    else if (ParcRaidHead.hdSize_ != 0) {
        //This is a VB line data file
        std::cerr << "Only VD line files with MrParcRaidFileHeader.hdSize_ == 0 (MR_PARC_RAID_ALLDATA) supported."
            << std::endl;
        return -1;
    }

    if (measurement_number < 0) {
        // negative indexing support ('-1' returns the last measurement)
        if (-measurement_number > ParcRaidHead.count_)
        {
            std::cout << "The file you are trying to convert has only " << ParcRaidHead.count_ << " measurements."
                << std::endl;
            std::cout << "Using negative indexing, you are trying to convert measurement number: " << measurement_number
                << std::endl;
            return -1;
        }

        measurement_number = ParcRaidHead.count_ + measurement_number + 1;
    }

    // Loop through all measurements in multi-raid
    std::string ismrmrd_file_orig = ismrmrd_file;
    std::string ismrmrd_group_orig = ismrmrd_group;
    unsigned int firstMeas, lastMeas;

    if (all_measurements)
    {
        firstMeas = 1;
        lastMeas  = ParcRaidHead.count_;
    } 
    else 
    {
        firstMeas = measurement_number;
        lastMeas  = measurement_number;
    }

    for (unsigned int currentMeas = firstMeas; currentMeas <= lastMeas; currentMeas++) {
        measurement_number = currentMeas;

        if (all_measurements)
        {
            if (multi_meas_file)
            {
                // Add the measurement number as a suffix to the group name
                ismrmrd_group = ismrmrd_group_orig;
                ismrmrd_group.append("_");
                ismrmrd_group.append(std::to_string(currentMeas));
            }
            else
            {
                // Add the measurement number as a suffix to the filename, excluding the file extension
                std::vector<std::string> v;
                boost::algorithm::split(v, ismrmrd_file_orig, boost::is_any_of("."));

                if (v.size() > 1)
                {
                    std::stringstream ss;
                    ss << v.at(v.size()-2) << "_" << currentMeas;
                    v.at(v.size()-2) = ss.str();
                    ismrmrd_file = boost::algorithm::join(v, ".");
                }
                else
                {
                    // No file extension found
                    std::stringstream ss;
                    ss << ismrmrd_file_orig << "_" << currentMeas;
                    ismrmrd_file = ss.str();
                }
            }

            // Reset file position
            if (!VBFILE)
            {
                siemens_dat.seekg(sizeof(MrParcRaidFileHeader), std::ios::beg);
            }
            else
            {
                siemens_dat.seekg(0, std::ios::beg);
            }
        }

        std::cout << "-----------------------------------------------------------------" << std::endl;
        if (all_measurements)
        {
            std::cout << "Converting measurement " << currentMeas << "/" << lastMeas << " into file " << ismrmrd_file << " in group " << ismrmrd_group << std::endl;
        }
        else
        {
            std::cout << "Converting measurement " << currentMeas << " into file " << ismrmrd_file << " in group " << ismrmrd_group << std::endl;
        }
        std::cout << "-----------------------------------------------------------------" << std::endl;

        if (!VBFILE && measurement_number > ParcRaidHead.count_) {
            std::cout << "The file you are trying to convert has only " << ParcRaidHead.count_ << " measurements."
                << std::endl;
            std::cout << "You are trying to convert measurement number: " << measurement_number << std::endl;
            return -1;
        }

        //if it is a VB scan
        if (VBFILE && measurement_number != 1) {
            std::cout << "The file you are trying to convert is a VB file and it has only one measurement." << std::endl;
            std::cout << "You tried to convert measurement number: " << measurement_number << std::endl;
            return -1;
        }

        // Parameter map
        std::string default_parammap;
        if (VBFILE) {
            default_parammap = "IsmrmrdParameterMap_Siemens_VB17.xml";
        } else {
            default_parammap = "IsmrmrdParameterMap_Siemens.xml";
        }
        std::string parammap_actual_file = select_file(parammap_file, default_parammap, all_measurements, currentMeas);
        std::string parammap_file_content = get_file_content(parammap_actual_file);
        std::cout << "Using parameter map: " << parammap_actual_file << std::endl;

        std::cout << "This file contains " << ParcRaidHead.count_ << " measurement(s)." << std::endl;

        std::vector<MrParcRaidFileEntry> ParcFileEntries = readParcFileEntries(siemens_dat, ParcRaidHead, VBFILE);

        // find the beginning of the desired measurement
        siemens_dat.seekg(ParcFileEntries[measurement_number - 1].off_, std::ios::beg);

        uint32_t dma_length = 0, num_buffers = 0;

        siemens_dat.read((char*)(&dma_length), sizeof(uint32_t));
        siemens_dat.read((char*)(&num_buffers), sizeof(uint32_t));

        //std::cout << "Measurement header DMA length: " << mhead.dma_length << std::endl;

        auto buffers = readMeasurementHeaderBuffers(siemens_dat, num_buffers);

        //We need to be on a 32 byte boundary after reading the buffers
        long long int position_in_meas =
            (long long int) (siemens_dat.tellg()) - ParcFileEntries[measurement_number - 1].off_;
        if (position_in_meas % 32 != 0) {
            siemens_dat.seekg(32 - (position_in_meas % 32), std::ios::cur);
        }

        // Measurement header done!
        //Now we should have the measurement headers, so let's use the Meas header to create the XML parametersstd::string xml_config;
        std::vector<std::string> wip_double;
        Trajectory trajectory;
        long dwell_time_0;
        long max_channels;
        long radial_views;
        long* global_table_pos = new long[3];
        std::string baseLineString;
        std::string protocol_name;
        std::string software_version;
        std::string xml_config = readXmlConfig(debug_xml, parammap_file_content, num_buffers, buffers, wip_double,
            trajectory, dwell_time_0,
            max_channels, radial_views, global_table_pos, baseLineString, protocol_name, software_version);

        // whether this scan is a adjustment scan
        bool isAdjustCoilSens = false;
        if (protocol_name == "AdjCoilSens") {
            isAdjustCoilSens = true;
        }

        bool isAdjQuietCoilSens = false;
        if (protocol_name == "AdjQuietCoilSens") {
            isAdjQuietCoilSens = true;
        }

        // whether this scan is from VB line
        bool isVB = false;
        if ((baseLineString.find("VB17") != std::string::npos)
            || (baseLineString.find("VB15") != std::string::npos)
            || (baseLineString.find("VB13") != std::string::npos)
            || (baseLineString.find("VB11") != std::string::npos)) {
            isVB = true;
        }

        std::cout << "Baseline: " << baseLineString << std::endl;
        std::cout << "Software version: " << software_version << std::endl;
        std::cout << "Protocol name: " << protocol_name << std::endl;

        bool isNX = false;
        if ((baseLineString.find("NXVA") != std::string::npos) || (software_version.find("XA11") != std::string::npos) )
        {
            isNX = true;
        }

        std::cout << "Dwell time: " << dwell_time_0 << std::endl;

        if (debug_xml) {
            std::ofstream o("xml_raw.xml");
            o.write(xml_config.c_str(), xml_config.size());
        }

        // Parameter style-sheet
        std::string default_parammap_xsl;
        if (isNX) {
            default_parammap_xsl = "IsmrmrdParameterMap_Siemens_NX.xsl";
        } else {
            default_parammap_xsl = "IsmrmrdParameterMap_Siemens.xsl";
        }
        std::string parammap_xsl_actual_file = select_file(parammap_xsl, default_parammap_xsl, all_measurements, currentMeas);
        std::string parammap_xsl_content = get_file_content(parammap_xsl_actual_file);
        std::cout << "Using parameter XSL: " << parammap_xsl_actual_file << std::endl;


        ISMRMRD::IsmrmrdHeader header;
        {
            std::string config = parseXML(debug_xml, parammap_xsl_content, schema_file_name_content, xml_config);
            ISMRMRD::deserialize(config.c_str(), header);
        }
        //Append buffers to xml_config if requested
        if (append_buffers) {
            append_buffers_to_xml_header(buffers, num_buffers, header);
        }

        // Free memory used for MeasurementHeaderBuffers


        auto ismrmrd_dataset = boost::make_shared<ISMRMRD::Dataset>(ismrmrd_file.c_str(), ismrmrd_group.c_str(), true);
        //If this is a spiral acquisition, we will calculate the trajectory and add it to the individual profilesISMRMRD::NDArray<float> traj;
        auto traj = getTrajectory(wip_double, trajectory, dwell_time_0, radial_views);

        uint32_t last_mask = 0;
        unsigned long int acquisitions = 1;
        unsigned long int sync_data_packets = 0;
        sMDH mdh;//For VB line
        bool first_call = true;

        while (!(last_mask & 1) && //Last scan not encountered
            (((ParcFileEntries[measurement_number - 1].off_ + ParcFileEntries[measurement_number - 1].len_) -
                siemens_dat.tellg()) > sizeof(sScanHeader)))  //not reached end of measurement without acqend
        {
            size_t position_in_meas = siemens_dat.tellg();
            sScanHeader scanhead;
            readScanHeader(siemens_dat, VBFILE, mdh, scanhead);

            if (!siemens_dat) {
                std::cerr << "Error reading header at acquisition " << acquisitions << "." << std::endl;
                break;
            }

            uint32_t dma_length = scanhead.ulFlagsAndDMALength & MDH_DMA_LENGTH_MASK;
            uint32_t mdh_enable_flags = scanhead.ulFlagsAndDMALength & MDH_ENABLE_FLAGS_MASK;

            //Check if this is synch data, if so, it must be handled differently.
            if (scanhead.aulEvalInfoMask[0] & (1 << 5)) {
                uint32_t last_scan_counter = acquisitions - 1;

                auto waveforms = readSyncdata(siemens_dat, VBFILE, acquisitions, dma_length, scanhead, header,
                                            last_scan_counter, skip_syncdata);
                for (auto &w : waveforms)
                    ismrmrd_dataset->appendWaveform(w);
                sync_data_packets++;
                continue;
            }

            if (first_call) {
                uint32_t time_stamp = scanhead.ulTimeStamp;

                // convert to acqusition date and time
                double timeInSeconds = time_stamp * 2.5 / 1e3;

                size_t hours = (size_t) (timeInSeconds / 3600);
                size_t mins = (size_t) ((timeInSeconds - hours * 3600) / 60);
                size_t secs = (size_t) (timeInSeconds - hours * 3600 - mins * 60);

                hours = hours % 24;
                mins  = mins  % 60;

                std::string study_time = get_time_string(hours, mins, secs);

                // if some of the ismrmrd header fields are not filled, here is a place to take some further actions
                if (!fill_ismrmrd_header(header, study_date_user_supplied, study_time)) {
                    std::cerr << "Failed to further fill XML header" << std::endl;
                }

                std::stringstream sstream;
                ISMRMRD::serialize(header, sstream);
                xml_config = sstream.str();

                if (xml_file_is_valid(xml_config, schema_file_name_content) <= 0) {
                    std::cerr << "Generated XML is not valid according to the ISMRMRD schema" << std::endl;
                    return -1;
                }

                if (debug_xml) {
                    std::ofstream o("processed.xml");
                    o.write(xml_config.c_str(), xml_config.size());
                }

                //This means we should only create XML header and exit
                if (header_only) {
                    std::ofstream header_out_file(ismrmrd_file.c_str());
                    header_out_file << xml_config;
                    return -1;
                }

                // Create an ISMRMRD dataset
            }

            //This check only makes sense in VD line files.
            if (!VBFILE && (scanhead.lMeasUID != ParcFileEntries[measurement_number - 1].measId_)) {
                //Something must have gone terribly wrong. Bail out.
                if (first_call) {
                    std::cerr << "Corrupted or retro-recon dataset detected (scanhead.lMeasUID != ParcFileEntries["
                            << measurement_number - 1 << "].measId_)" << std::endl;
                    std::cerr << "Fix the scanhead.lMeasUID ... " << std::endl;
                }
                scanhead.lMeasUID = ParcFileEntries[measurement_number - 1].measId_;
            }

            if (first_call) first_call = false;

            //Allocate data for channels
            std::vector<ChannelHeaderAndData> channels = readChannelHeaders(siemens_dat, VBFILE, scanhead);

            if (!siemens_dat) {
                std::cerr << "Error reading data at acquisition " << acquisitions << "." << std::endl;
                break;
            }

            acquisitions++;
            last_mask = scanhead.aulEvalInfoMask[0];

            if (scanhead.aulEvalInfoMask[0] & 1) {
                std::cout << "Last scan reached..." << std::endl;
                break;
            }

            ismrmrd_dataset->appendAcquisition(
                    getAcquisition(flash_pat_ref_scan, trajectory, dwell_time_0, global_table_pos, max_channels, isAdjustCoilSens,
                                isAdjQuietCoilSens, isVB, isNX, traj, scanhead, channels));

        }//End of the while loop
        delete [] global_table_pos;

        if (!siemens_dat) {
            std::cerr << "WARNING: Unexpected error.  Please check the result." << std::endl;
            return -1;
        }

        ismrmrd_dataset->writeHeader(xml_config);

        //Mystery bytes. There seems to be 160 mystery bytes at the end of the data.
        std::streamoff mystery_bytes = (std::streamoff) (ParcFileEntries[measurement_number - 1].off_ +
                                                        ParcFileEntries[measurement_number - 1].len_) -
                                    siemens_dat.tellg();

        if (mystery_bytes > 0) {
            if (mystery_bytes != MYSTERY_BYTES_EXPECTED) {
                // Something in not quite right
                std::cerr << "WARNING: Unexpected number of mystery bytes detected: " << mystery_bytes << std::endl;
                std::cerr << "ParcFileEntries[" << measurement_number - 1 << "].off_ = "
                        << ParcFileEntries[measurement_number - 1].off_ << std::endl;
                std::cerr << "ParcFileEntries[" << measurement_number - 1 << "].len_ = "
                        << ParcFileEntries[measurement_number - 1].len_ << std::endl;
                std::cerr << "siemens_dat.tellg() = " << siemens_dat.tellg() << std::endl;
                std::cerr << "Please check the result." << std::endl;
            } else {
                // Read the mystery bytes
                char mystery_data[MYSTERY_BYTES_EXPECTED];
                siemens_dat.read(reinterpret_cast<char *>(&mystery_data), mystery_bytes);
                //After this we have to be on a 512 byte boundary
                if (siemens_dat.tellg() % 512) {
                    siemens_dat.seekg(512 - (siemens_dat.tellg() % 512), std::ios::cur);
                }
            }
        }

        size_t end_position = siemens_dat.tellg();
        siemens_dat.seekg(0, std::ios::end);
        size_t eof_position = siemens_dat.tellg();
        if (end_position != eof_position && ParcRaidHead.count_ == measurement_number) {
            size_t additional_bytes = eof_position - end_position;
            std::cerr << "WARNING: End of file was not reached during conversion. There are " <<
                    additional_bytes << " additional bytes at the end of file." << std::endl;
        }
    } // Loop through multiple measurements in multi-raid

    return 0;
}

std::vector<ChannelHeaderAndData>
readChannelHeaders(std::ifstream &siemens_dat, bool VBFILE, const sScanHeader &scanhead) {
    size_t nchannels = scanhead.ushUsedChannels;
    auto channels = std::vector<ChannelHeaderAndData>(nchannels);
    sMDH mdh;
    for (unsigned int c = 0; c < nchannels; c++) {
        if (VBFILE) {
            if (c > 0) {
                siemens_dat.read(reinterpret_cast<char *>(&mdh), sizeof(sMDH));
            }
            channels[c].header.ulTypeAndChannelLength = 0;
            channels[c].header.lMeasUID = mdh.lMeasUID;
            channels[c].header.ulScanCounter = mdh.ulScanCounter;
            channels[c].header.ulReserved1 = 0;
            channels[c].header.ulSequenceTime = 0;
            channels[c].header.ulUnused2 = 0;
            channels[c].header.ulChannelId = mdh.ushChannelId;
            channels[c].header.ulUnused3 = 0;
            channels[c].header.ulCRC = 0;
        } else {
            siemens_dat.read(reinterpret_cast<char *>(&channels[c].header), sizeof(sChannelHeader));
        }

        size_t nsamples = scanhead.ushSamplesInScan;
        channels[c].data = std::vector<complex_float_t>(nsamples);
        siemens_dat.read(reinterpret_cast<char *>(&channels[c].data[0]), nsamples * sizeof(complex_float_t));
    }
    return channels;
}

void readScanHeader(std::ifstream &siemens_dat, bool VBFILE, sMDH &mdh, sScanHeader &scanhead) {
    siemens_dat.read(reinterpret_cast<char *>(&scanhead.ulFlagsAndDMALength), sizeof(uint32_t));

    if (VBFILE) {
        siemens_dat.read(reinterpret_cast<char *>(&mdh) + sizeof(uint32_t), sizeof(sMDH) - sizeof(uint32_t));
        scanhead.lMeasUID = mdh.lMeasUID;
        scanhead.ulScanCounter = mdh.ulScanCounter;
        scanhead.ulTimeStamp = mdh.ulTimeStamp;
        scanhead.ulPMUTimeStamp = mdh.ulPMUTimeStamp;
        scanhead.ushSystemType = 0;
        scanhead.ulPTABPosDelay = 0;
        scanhead.lPTABPosX = 0;
        scanhead.lPTABPosY = 0;
        scanhead.lPTABPosZ = mdh.ushPTABPosNeg;//TODO: Modify calculation
        scanhead.ulReserved1 = 0;
        scanhead.aulEvalInfoMask[0] = mdh.aulEvalInfoMask[0];
        scanhead.aulEvalInfoMask[1] = mdh.aulEvalInfoMask[1];
        scanhead.ushSamplesInScan = mdh.ushSamplesInScan;
        scanhead.ushUsedChannels = mdh.ushUsedChannels;
        scanhead.sLC = mdh.sLC;
        scanhead.sCutOff = mdh.sCutOff;
        scanhead.ushKSpaceCentreColumn = mdh.ushKSpaceCentreColumn;
        scanhead.ushCoilSelect = mdh.ushCoilSelect;
        scanhead.fReadOutOffcentre = mdh.fReadOutOffcentre;
        scanhead.ulTimeSinceLastRF = mdh.ulTimeSinceLastRF;
        scanhead.ushKSpaceCentreLineNo = mdh.ushKSpaceCentreLineNo;
        scanhead.ushKSpaceCentrePartitionNo = mdh.ushKSpaceCentrePartitionNo;
        scanhead.sSliceData = mdh.sSliceData;
        memset(scanhead.aushIceProgramPara, 0, sizeof(uint16_t) * 24);
        memcpy(scanhead.aushIceProgramPara, mdh.aushIceProgramPara, 8 * sizeof(uint16_t));
        memset(scanhead.aushReservedPara, 0, sizeof(uint16_t) * 4);
        scanhead.ushApplicationCounter = 0;
        scanhead.ushApplicationMask = 0;
        scanhead.ulCRC = 0;
    } else {
        siemens_dat.read(reinterpret_cast<char *>(&scanhead) + sizeof(uint32_t),
                         sizeof(sScanHeader) - sizeof(uint32_t));
    }
}

ISMRMRD::Acquisition
getAcquisition(bool flash_pat_ref_scan, const Trajectory &trajectory, long dwell_time_0, long* global_table_pos, long max_channels,
               bool isAdjustCoilSens, bool isAdjQuietCoilSens, bool isVB, bool isNX, ISMRMRD::NDArray<float> &traj,
               const sScanHeader &scanhead, const std::vector<ChannelHeaderAndData> &channels) {
    ISMRMRD::Acquisition ismrmrd_acq;
    // The number of samples, channels and trajectory dimensions is set below

    // Acquisition header values are zero by default
    ismrmrd_acq.measurement_uid() = scanhead.lMeasUID;
    ismrmrd_acq.scan_counter() = scanhead.ulScanCounter;
    ismrmrd_acq.acquisition_time_stamp() = scanhead.ulTimeStamp;
    ismrmrd_acq.physiology_time_stamp()[0] = scanhead.ulPMUTimeStamp;
    ismrmrd_acq.available_channels() = (uint16_t) max_channels;
    // uint64_t channel_mask[16];     //Mask to indicate which channels are active. Support for 1024 channels
    ismrmrd_acq.discard_pre() = scanhead.sCutOff.ushPre;
    ismrmrd_acq.discard_post() = scanhead.sCutOff.ushPost;
    ismrmrd_acq.center_sample() = scanhead.ushKSpaceCentreColumn;

    // std::cout << "isAdjustCoilSens, isVB : " << isAdjustCoilSens << " " << isVB << std::endl;

    if (scanhead.aulEvalInfoMask[0] & (1ULL << 25))
    { //This is noise
        ismrmrd_acq.sample_time_us() = compute_noise_sample_in_us(scanhead.ushSamplesInScan, isAdjustCoilSens,
                                                                  isAdjQuietCoilSens, isVB, isNX);

        // std::cout << "Noise sample time us :" << ismrmrd_acq.sample_time_us() << std::endl;
    } else {
        ismrmrd_acq.sample_time_us() = dwell_time_0 / 1000.0f;
    }
    // std::cout << "ismrmrd_acq.sample_time_us(): " << ismrmrd_acq.sample_time_us() << std::endl;

    ismrmrd_acq.position()[0] = scanhead.sSliceData.sSlicePosVec.flSag;// + (float) (global_table_pos[0]);
    ismrmrd_acq.position()[1] = scanhead.sSliceData.sSlicePosVec.flCor;// + (float) (global_table_pos[1]);
    ismrmrd_acq.position()[2] = scanhead.sSliceData.sSlicePosVec.flTra;// + (float) (global_table_pos[2]);

    // Convert Siemens quaternions to direction cosines.
    // In the Siemens convention the quaternion corresponds to a rotation matrix with columns P R S
    // Siemens stores the quaternion as (W,X,Y,Z)
    float quat[4];
    quat[0] = scanhead.sSliceData.aflQuaternion[1]; // X
    quat[1] = scanhead.sSliceData.aflQuaternion[2]; // Y
    quat[2] = scanhead.sSliceData.aflQuaternion[3]; // Z
    quat[3] = scanhead.sSliceData.aflQuaternion[0]; // W
    ISMRMRD::ismrmrd_quaternion_to_directions(quat,
                                              ismrmrd_acq.phase_dir(),
                                              ismrmrd_acq.read_dir(),
                                              ismrmrd_acq.slice_dir());

    //std::cout << "scanhead.ulScanCounter         = " << scanhead.ulScanCounter << std::endl;
    //std::cout << "quat         = [" << quat[0] << " " << quat[1] << " " << quat[2] << " " << quat[3] << "]" << std::endl;
    //std::cout << "phase_dir    = [" << ismrmrd_acq.phase_dir()[0] << " " << ismrmrd_acq.phase_dir()[1] << " " << ismrmrd_acq.phase_dir()[2] << "]" << std::endl;
    //std::cout << "read_dir     = [" << ismrmrd_acq.read_dir()[0] << " " << ismrmrd_acq.read_dir()[1] << " " << ismrmrd_acq.read_dir()[2] << "]" << std::endl;
    //std::cout << "slice_dir    = [" << ismrmrd_acq.slice_dir()[0] << " " << ismrmrd_acq.slice_dir()[1] << " " << ismrmrd_acq.slice_dir()[2] << "]" << std::endl;
    //std::cout << "--------------------------------------------------------" << std::endl;

    ismrmrd_acq.patient_table_position()[0] = (float) scanhead.lPTABPosX;
    ismrmrd_acq.patient_table_position()[1] = (float) scanhead.lPTABPosY;
    ismrmrd_acq.patient_table_position()[2] = (float) scanhead.lPTABPosZ;

    bool fixedE1E2 = true;
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 25))) fixedE1E2 = false; // noise
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 1))) fixedE1E2 = false; // navigator, rt feedback
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 2))) fixedE1E2 = false; // hp feedback
    if ((scanhead.aulEvalInfoMask[1] & (1ULL << 51-32))) fixedE1E2 = false; // dummy
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 5))) fixedE1E2 = false; // synch data

    ismrmrd_acq.idx().average = scanhead.sLC.ushAcquisition;
    ismrmrd_acq.idx().contrast = scanhead.sLC.ushEcho;
    ismrmrd_acq.idx().kspace_encode_step_1 = scanhead.sLC.ushLine;
    ismrmrd_acq.idx().kspace_encode_step_2 = scanhead.sLC.ushPartition;
    ismrmrd_acq.idx().phase = scanhead.sLC.ushPhase;
    ismrmrd_acq.idx().repetition = scanhead.sLC.ushRepetition;
    ismrmrd_acq.idx().segment = scanhead.sLC.ushSeg;
    ismrmrd_acq.idx().set = scanhead.sLC.ushSet;
    ismrmrd_acq.idx().slice = scanhead.sLC.ushSlice;
    ismrmrd_acq.idx().user[0] = scanhead.sLC.ushIda;
    ismrmrd_acq.idx().user[1] = scanhead.sLC.ushIdb;
    ismrmrd_acq.idx().user[2] = scanhead.sLC.ushIdc;
    ismrmrd_acq.idx().user[3] = scanhead.sLC.ushIdd;
    ismrmrd_acq.idx().user[4] = scanhead.sLC.ushIde;
    // TODO: remove this once the GTPlus can properly autodetect partial fourier
    ismrmrd_acq.idx().user[5] = scanhead.ushKSpaceCentreLineNo;
    ismrmrd_acq.idx().user[6] = scanhead.ushKSpaceCentrePartitionNo;

    /*****************************************************************************/
    /* the user_int[0] and user_int[1] are used to store user defined parameters */
    /*****************************************************************************/
    ismrmrd_acq.user_int()[0] = scanhead.aushIceProgramPara[0];
    ismrmrd_acq.user_int()[1] = scanhead.aushIceProgramPara[1];
    ismrmrd_acq.user_int()[2] = scanhead.aushIceProgramPara[2];
    ismrmrd_acq.user_int()[3] = scanhead.aushIceProgramPara[3];
    ismrmrd_acq.user_int()[4] = scanhead.aushIceProgramPara[4];
    ismrmrd_acq.user_int()[5] = scanhead.aushIceProgramPara[5];
    ismrmrd_acq.user_int()[6] = scanhead.aushIceProgramPara[6];
    // TODO: in the newer version of ismrmrd, add field to store time_since_perp_pulse
    ismrmrd_acq.user_int()[7] = scanhead.ulTimeSinceLastRF;

    ismrmrd_acq.user_float()[0] = scanhead.aushIceProgramPara[8];
    ismrmrd_acq.user_float()[1] = scanhead.aushIceProgramPara[9];
    ismrmrd_acq.user_float()[2] = scanhead.aushIceProgramPara[10];
    ismrmrd_acq.user_float()[3] = scanhead.aushIceProgramPara[11];
    ismrmrd_acq.user_float()[4] = scanhead.aushIceProgramPara[12];
    ismrmrd_acq.user_float()[5] = scanhead.aushIceProgramPara[13];
    ismrmrd_acq.user_float()[6] = scanhead.aushIceProgramPara[14];
    ismrmrd_acq.user_float()[7] = scanhead.aushIceProgramPara[15];

    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 25))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_NOISE_MEASUREMENT);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 28))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_FIRST_IN_SLICE);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 29))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_LAST_IN_SLICE);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 11))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_LAST_IN_REPETITION);

    /// if a line is both image and ref, then do not set the ref flag
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 23))) {
        ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_PARALLEL_CALIBRATION_AND_IMAGING);
    } else {
        if ((scanhead.aulEvalInfoMask[0] & (1ULL << 22)))
            ismrmrd_acq.setFlag(
                    ISMRMRD::ISMRMRD_ACQ_IS_PARALLEL_CALIBRATION);
    }

    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 24))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_REVERSE);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 11))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_LAST_IN_MEASUREMENT);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 21))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_PHASECORR_DATA);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 1))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_NAVIGATION_DATA);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 1))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_RTFEEDBACK_DATA);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 2))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_HPFEEDBACK_DATA);
    if ((scanhead.aulEvalInfoMask[1] & (1ULL << 51-32))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_DUMMYSCAN_DATA);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 10)))
        ismrmrd_acq.setFlag(
                ISMRMRD::ISMRMRD_ACQ_IS_SURFACECOILCORRECTIONSCAN_DATA);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 5))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_DUMMYSCAN_DATA);
    // if ((scanhead.aulEvalInfoMask[0] & (1ULL << 1))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_LAST_IN_REPETITION);

    if ((scanhead.aulEvalInfoMask[1] & (1ULL << 46-32))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_LAST_IN_MEASUREMENT);

    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 14))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_PHASE_STABILIZATION_REFERENCE);
    if ((scanhead.aulEvalInfoMask[0] & (1ULL << 15))) ismrmrd_acq.setFlag(ISMRMRD::ISMRMRD_ACQ_IS_PHASE_STABILIZATION);

    if ((flash_pat_ref_scan) & (ismrmrd_acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_IS_PARALLEL_CALIBRATION))) {
        // For some sequences the PAT Reference data is collected using a different encoding space
        // e.g. EPI scans with FLASH PAT Reference
        // enabled by command line option
        // TODO: it is likely that the dwell time is not set properly for this type of acquisition
        ismrmrd_acq.encoding_space_ref() = 1;
    }

    if ((trajectory == Trajectory::TRAJECTORY_SPIRAL) & !(ismrmrd_acq.isFlagSet(
            ISMRMRD::ISMRMRD_ACQ_IS_NOISE_MEASUREMENT))) { //Spiral and not noise, we will add the trajectory to the data

        // from above we have the following
        // traj_dim[0] = dimensionality (2)
        // traj_dim[1] = ngrad i.e. points per interleaf
        // traj_dim[2] = no. of interleaves
        // and
        // traj.getData() is a float * pointer to the trajectory stored
        // kspace_encode_step_1 is the interleaf number

        // Set the acquisition number of samples, channels and trajectory dimensions
        // this reallocates the memory
        auto traj_dim = traj.getDims();
        ismrmrd_acq.resize(scanhead.ushSamplesInScan,
                           scanhead.ushUsedChannels,
                           traj_dim[0]);

        unsigned long traj_samples_to_copy = ismrmrd_acq.number_of_samples();
        if (traj_dim[1] < traj_samples_to_copy) {
            traj_samples_to_copy = (unsigned long) traj_dim[1];
            ismrmrd_acq.discard_post() = (uint16_t) (ismrmrd_acq.number_of_samples() - traj_samples_to_copy);
        }
        float *t_ptr = &traj.getDataPtr()[traj_dim[0] * traj_dim[1] * ismrmrd_acq.idx().kspace_encode_step_1];
        memcpy((void *) ismrmrd_acq.getTrajPtr(), t_ptr, sizeof(float) * traj_dim[0] * traj_samples_to_copy);
    } else { //No trajectory
        // Set the acquisition number of samples, channels and trajectory dimensions
        // this reallocates the memory
        ismrmrd_acq.resize(scanhead.ushSamplesInScan, scanhead.ushUsedChannels);
    }

    for (unsigned int c = 0; c < ismrmrd_acq.active_channels(); c++) {
        memcpy((complex_float_t *) &(ismrmrd_acq.getDataPtr()[c * ismrmrd_acq.number_of_samples()]),
               &channels[c].data[0], ismrmrd_acq.number_of_samples() * sizeof(complex_float_t));
    }


    if (scanhead.ulScanCounter % 1000 == 0) {
        std::cout << "wrote scan : " << scanhead.ulScanCounter << std::endl;
    }

    return ismrmrd_acq;
}

std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> unpack_pmu(const std::vector<PMUdata> &data) {

    auto tup = std::make_tuple(std::vector<uint32_t>(), std::vector<uint32_t>());
    std::get<0>(tup).reserve(data.size());
    std::get<1>(tup).reserve(data.size());

    for (auto d : data) {

        std::get<0>(tup).push_back(d.data);
        std::get<1>(tup).push_back(d.trigger);
    }
    return tup;
}


void makeWaveformHeader(ISMRMRD::IsmrmrdHeader &header) {

    if (!header.waveformInformation.size()) {
        for (int learning_phase = false; learning_phase <= true; learning_phase++) {
            ISMRMRD::WaveformInformation info;
            ISMRMRD::UserParameterLong userParam;
            ISMRMRD::UserParameterString userParamString;
            userParamString.name = "Phase";
            if (learning_phase) {
                userParamString.value = "Learning";
            } else {
                userParamString.value = "Acquisition";
            }

            userParam.name = "TriggerChannel";
            userParam.value = 4; //Trigger is stored in 5th channel for ECG
            info.waveformName = "ECG";
            info.waveformType = ISMRMRD::WaveformType::ECG;
            info.userParameters = ISMRMRD::UserParameters();
            info.userParameters.get().userParameterLong.push_back(userParam);
            header.waveformInformation.push_back(info);


            info.waveformName = "PULS";
            info.waveformType = ISMRMRD::WaveformType::PULSE;
            info.userParameters.get().userParameterLong[0].value = 1; //Trigger is storend in 2nd channel everything else
            header.waveformInformation.push_back(info);

            info.waveformName = "RESP";
            info.waveformType = ISMRMRD::WaveformType::RESPIRATORY;
            header.waveformInformation.push_back(info);

            info.waveformName = "EXT1";
            info.waveformType = ISMRMRD::WaveformType::OTHER;
            header.waveformInformation.push_back(info);

            info.waveformName = "EXT2";
            info.waveformType = ISMRMRD::WaveformType::OTHER;
            header.waveformInformation.push_back(info);
        }

    }


}

const std::map<PMU_Type, int> waveformId = {{PMU_Type::ECG1, 0},
                                            {PMU_Type::ECG2, 0},
                                            {PMU_Type::ECG3, 0},
                                            {PMU_Type::ECG4, 0},
                                            {PMU_Type::PULS, 1},
                                            {PMU_Type::RESP, 2},
                                            {PMU_Type::EXT1, 3},
                                            {PMU_Type::EXT2, 4}};

//It appears Siemens hard-codes sample times for their PMU systems, which sounds suspicious
//const std::map<PMU_Type, float> sample_time_us = {{PMU_Type::ECG1,2500},{PMU_Type::ECG2,2500},{PMU_Type::ECG3,2500},{PMU_Type::ECG4,2500},
//                                               {PMU_Type::PULS,5000},
//                                               {PMU_Type::RESP,20000},
//                                               {PMU_Type::EXT1,20000},
//                                               {PMU_Type::EXT2,20000}};

std::set<PMU_Type> PMU_Types = {PMU_Type::ECG1, PMU_Type::ECG2, PMU_Type::ECG3, PMU_Type::ECG4, PMU_Type::PULS,
                                PMU_Type::RESP, PMU_Type::EXT1, PMU_Type::EXT2, PMU_Type::END};

std::vector<ISMRMRD::Waveform> readSyncdata(std::ifstream &siemens_dat, bool VBFILE, unsigned long acquisitions,
                                            uint32_t dma_length, sScanHeader scanheader, ISMRMRD::IsmrmrdHeader &header,
                                            long last_scan_counter, bool skip_syncdata) {

    size_t len = 0;
    if (VBFILE) {
        len = dma_length - sizeof(sMDH);
        //Is VB magic? For now let's assume it's not, and that this is just Siemens secret sauce.
        siemens_dat.seekg(len, siemens_dat.cur);
        return std::vector<ISMRMRD::Waveform>();
    } else {
        len = dma_length - sizeof(sScanHeader);

//        siemens_dat.seekg(len,siemens_dat.cur);
//        return std::vector<ISMRMRD::Waveform>();
        auto cur_pos = siemens_dat.tellg();
        uint32_t packetSize;
        siemens_dat.read((char *) &packetSize, sizeof(uint32_t));
        std::string packedID;
        {
            char packedIDArr[52];
            siemens_dat.read(packedIDArr, 52);
            packedID = packedIDArr;

        }

        if ((skip_syncdata) || (packedID.find("PMU") == packedID.npos)) { //packedID indicates this isn't PMU data, so let's jump ship.
            siemens_dat.seekg(cur_pos);
            siemens_dat.seekg(len, siemens_dat.cur);
            return std::vector<ISMRMRD::Waveform>();

        }

        bool learning_phase = packedID.find("PMULearnPhase") != packedID.npos;

        uint32_t swappedFlag, timestamp0, timestamp, packerNr, duration;

        siemens_dat.read((char *) &swappedFlag, sizeof(uint32_t));
        siemens_dat.read((char *) &timestamp0, sizeof(uint32_t));
        siemens_dat.read((char *) &timestamp, sizeof(uint32_t));
        siemens_dat.read((char *) &packerNr, sizeof(uint32_t));
        siemens_dat.read((char *) &duration, sizeof(uint32_t));

        PMU_Type magic;
        siemens_dat.read((char *) &magic, sizeof(uint32_t));
        //Read in all the PMU data first, to figure out if we have multiple ECGs.
        std::map<PMU_Type, std::tuple<std::vector<PMUdata>, uint32_t >> pmu_map;
        std::set<PMU_Type> ecg_types = {PMU_Type::ECG1, PMU_Type::ECG2, PMU_Type::ECG3, PMU_Type::ECG4};
        std::map<PMU_Type, std::tuple<std::vector<PMUdata>, uint32_t >> ecg_map;
        while (magic != PMU_Type::END) {
            //Read and store period
            uint32_t period;

            siemens_dat.read((char *) &period, sizeof(uint32_t));

            //Allocate and read data
            std::vector<PMUdata> data(duration / period);
            siemens_dat.read((char *) data.data(), data.size() * sizeof(PMUdata));
            //Split into ECG and PMU sets.
            if (ecg_types.count(magic)) {
                ecg_map[magic] = std::make_tuple(std::move(data), period);
            } else {
                pmu_map[magic] = std::make_tuple(std::move(data), period);
            }
            //Read next tag
            siemens_dat.read((char *) &magic, sizeof(uint32_t));
            if (!PMU_Types.count(magic))
                throw std::runtime_error("Malformed file");


        }

        //Have to handle ECG separately.

        std::vector<ISMRMRD::Waveform> waveforms;
        waveforms.reserve(5);
        if (ecg_map.size() > 0 || pmu_map.size() > 0) {

            if (ecg_map.size() > 0) {

                size_t channels = ecg_map.size();
                size_t number_of_elements = std::get<0>(ecg_map.begin()->second).size();

                auto ecg_waveform = ISMRMRD::Waveform(number_of_elements, channels + 1);
                ecg_waveform.head.waveform_id = waveformId.at(PMU_Type::ECG1) + 5 * learning_phase;

                uint32_t *ecg_waveform_data = ecg_waveform.data;

                uint32_t *trigger_data = ecg_waveform_data + number_of_elements * channels;
                std::fill(trigger_data, trigger_data + number_of_elements, 0);
                //Copy in the data
                for (auto key_val : ecg_map) {
                    auto tup = unpack_pmu(std::get<0>(key_val.second));
                    auto &data = std::get<0>(tup);
                    auto &trigger = std::get<1>(tup);

                    std::copy(data.begin(), data.end(), ecg_waveform_data);
                    ecg_waveform_data += data.size();

                    for (auto i = 0; i < number_of_elements; i++) trigger_data[i] |= trigger[i];

                }

//                ecg_waveform.head.sample_time_us = sample_time_us.at(PMU_Type::ECG1);
                waveforms.push_back(std::move(ecg_waveform));


            }


            for (auto key_val : pmu_map) {
                auto tup = unpack_pmu(std::get<0>(key_val.second));
                auto &data = std::get<0>(tup);
                auto &trigger = std::get<1>(tup);

                auto waveform = ISMRMRD::Waveform(data.size(), 2);
                waveform.head.waveform_id = waveformId.at(key_val.first) + 5 * learning_phase;
                std::copy(data.begin(), data.end(), waveform.data);

                std::copy(trigger.begin(), trigger.end(), waveform.data + data.size());

//                waveform.head.sample_time_us = sample_time_us.at(key_val.first);
                waveforms.push_back(std::move(waveform));
            }
            //Figure out number of ECG channels


        }


        for (auto &waveform : waveforms) {
            waveform.head.time_stamp = timestamp;
            waveform.head.measurement_uid = scanheader.lMeasUID;
            waveform.head.scan_counter = last_scan_counter;
            waveform.head.sample_time_us = double(duration * 100) / waveform.head.number_of_samples;
        }

        if (waveforms.size()) makeWaveformHeader(header); //Add the header if needed

        siemens_dat.seekg(cur_pos);
        siemens_dat.seekg(len, siemens_dat.cur);
        return waveforms;


    }
}

//
//void getsyncData(std::ifstream &siemens_dat, bool VBFILE, uint32_t dma_length) {
//    size_t len = 0;
//    if (VBFILE)
//             {
//                 len = dma_length-sizeof(sMDH);
//             }
//             else
//             {
//                 len = dma_length-sizeof(sScanHeader);
//             }
//
//    std::vector<uint8_t> syncdata(len);
//    siemens_dat.read(reinterpret_cast<char*>(&syncdata[0]), len);
//}

ISMRMRD::NDArray<float>
getTrajectory(const std::vector<std::string> &wip_double, const Trajectory &trajectory, long dwell_time_0,
              long radial_views) {
    std::vector<size_t> traj_dim;
    ISMRMRD::NDArray<float> traj;
    if (trajectory == Trajectory::TRAJECTORY_SPIRAL) {
        int nfov = 1;         /*  number of fov coefficients.             */
        int ngmax = (int) 1e5;  /*  maximum number of gradient samples      */
        double *xgrad;             /*  x-component of gradient.                */
        double *ygrad;             /*  y-component of gradient.                */
        double *x_trajectory;
        double *y_trajectory;
        double *weighting;
        int ngrad;

        double sample_time = (1.0 * dwell_time_0) * 1e-9;
        double smax = atof(wip_double[7].c_str());
        double gmax = atof(wip_double[6].c_str());
        double fov = atof(wip_double[9].c_str());
        double krmax = atof(wip_double[8].c_str());
        long interleaves = radial_views;

        /* calculate gradients */
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

        /* Calculate the trajectory and weights*/
        calc_traj(xgrad, ygrad, ngrad, interleaves, sample_time, krmax, &x_trajectory, &y_trajectory, &weighting);

        // 2 * number of points for each X and Y
        traj_dim.push_back(2);
        traj_dim.push_back(ngrad);
        traj_dim.push_back(interleaves);
        traj.resize(traj_dim);

        for (int i = 0; i < (ngrad * interleaves); i++) {
            traj.getDataPtr()[i * 2] = (float) (-x_trajectory[i] / 2);
            traj.getDataPtr()[i * 2 + 1] = (float) (-y_trajectory[i] / 2);
        }

        delete[] xgrad;
        delete[] ygrad;
        delete[] x_trajectory;
        delete[] y_trajectory;
        delete[] weighting;

    }
    return traj;
}

std::string parseXML(bool debug_xml, const std::string &parammap_xsl_content, std::string &schema_file_name_content,
                     const std::string xml_config) {
    xsltStylesheetPtr cur = NULL;

    xmlDocPtr doc, res, xml_doc;

    const char *params[16 + 1];

    int nbparams = 0;

    params[nbparams] = NULL;

    xmlSubstituteEntitiesDefault(1);

    xmlLoadExtDtdDefaultValue = 1;

    xml_doc = xmlParseMemory(parammap_xsl_content.c_str(), parammap_xsl_content.size());

    if (xml_doc == NULL) {
        std::stringstream sstream;
        sstream << "Error when parsing xsl parameter stylesheet...";
        throw std::runtime_error(sstream.str());

    }

    cur = xsltParseStylesheetDoc(xml_doc);
    doc = xmlParseMemory(xml_config.c_str(), xml_config.size());
    res = xsltApplyStylesheet(cur, doc, params);

    xmlChar *out_ptr = NULL;
    int xslt_length = 0;
    int xslt_result = xsltSaveResultToString(&out_ptr, &xslt_length, res, cur);

    if (xslt_result < 0) {
        std::cerr << "Failed to save converted doc to string" << std::endl;

    }

    std::string xml_result = std::string((char *) out_ptr, xslt_length);

    if (xml_file_is_valid(xml_result, schema_file_name_content) <= 0) {
        std::stringstream sstream;
        sstream << "Generated XML is not valid according to the ISMRMRD schema";
        throw std::runtime_error(sstream.str());

        if (debug_xml) {
            std::ofstream o("processed.xml");
            o.write(xml_result.c_str(), xml_result.size());
        }


    }

    xsltFreeStylesheet(cur);
    xmlFreeDoc(res);
    xmlFreeDoc(doc);

    xsltCleanupGlobals();
    xmlCleanupParser();
    return xml_result;
}

std::string readXmlConfig(bool debug_xml, const std::string &parammap_file_content, uint32_t num_buffers,
                          std::vector<MeasurementHeaderBuffer> &buffers, std::vector<std::string> &wip_double,
                          Trajectory &trajectory, long &dwell_time_0, long &max_channels, long &radial_views,
                          long *global_table_pos, std::string &baseLineString, std::string &protocol_name, std::string& software_version) {
    dwell_time_0 = 0;
    max_channels = 0;
    radial_views = 0;
    protocol_name = "";
    std::vector<std::string> wip_long;
    long center_line = 0;
    long center_partition = 0;
    long lPhaseEncodingLines = 0;
    long iNoOfFourierLines = 0;
    long lPartitions = 0;
    long iNoOfFourierPartitions = 0;
    std::string seqString;
    for (unsigned int b = 0; b < num_buffers; b++) {
        if (buffers[b].name.compare("Meas") != 0) continue;


        std::string config_buffer = std::string(&buffers[b].buf[0], buffers[b].buf.size() - 2);
        XProtocol::XNode n;

        if (debug_xml) {
            std::ofstream o("config_buffer.xprot");
            o.write(config_buffer.c_str(), config_buffer.size());
        }

        bool is_NX = false;
        if(config_buffer.find("syngo MR XA11")!=std::string::npos)
        {
            is_NX = true;
        }

        if (ParseXProtocol(config_buffer, n) < 0) {
            std::stringstream sstream;
            sstream << "Failed to parse XProtocol for buffer " << buffers[b].name;
            throw std::runtime_error(sstream.str());

        }

        //Get some parameters - wip long
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("MEAS.sWipMemBlock.alFree"), n);
            if (n2) {
                wip_long = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "Search path: MEAS.sWipMemBlock.alFree not found." << std::endl;
            }
            if (wip_long.size() == 0) {
                std::stringstream sstream;
                sstream << "Failed to find WIP long parameters";
                throw std::runtime_error(sstream.str());

            }
        }

        //Get some parameters - wip double
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("MEAS.sWipMemBlock.adFree"), n);
            if (n2) {
                wip_double = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "Search path: MEAS.sWipMemBlock.adFree not found." << std::endl;
            }
            if (wip_double.size() == 0) {
                std::stringstream sstream;
                sstream << "Failed to find WIP double parameters";
                throw std::runtime_error(sstream.str());

            }
        }

        //Get some parameters - dwell times
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("MEAS.sRXSPEC.alDwellTime"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "Search path: MEAS.sWipMemBlock.alFree not found." << std::endl;
            }
            if (temp.size() == 0) {
                std::stringstream sstream;
                sstream << "Failed to find dwell times";
                throw std::runtime_error(sstream.str());

            } else {
                dwell_time_0 = atoi(temp[0].c_str());
            }
        }

        //Get some parameters - trajectory
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.ucTrajectory"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "Search path: MEAS.sKSpace.ucTrajectory not found." << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find appropriate trajectory array";
                throw std::runtime_error(sstream.str());

            } else {

                int traj = atoi(temp[0].c_str());
                trajectory = Trajectory(traj);
                std::cout << "Trajectory is: " << traj << std::endl;
            }
        }

        //Get some parameters - max channels
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("YAPS.iMaxNoOfRxChannels"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "YAPS.iMaxNoOfRxChannels" << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find YAPS.iMaxNoOfRxChannels array";
                throw std::runtime_error(sstream.str());

            } else {
                max_channels = atoi(temp[0].c_str());
            }
        }

        //Get some parameters - cartesian encoding bits
        {
            // get the center line parameters
            const XProtocol::XNode *n2 = apply_visitor(
                    XProtocol::getChildNodeByName("MEAS.sKSpace.lPhaseEncodingLines"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "MEAS.sKSpace.lPhaseEncodingLines not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find MEAS.sKSpace.lPhaseEncodingLines array";
                throw std::runtime_error(sstream.str());

            } else {
                lPhaseEncodingLines = atoi(temp[0].c_str());
            }

            n2 = apply_visitor(XProtocol::getChildNodeByName("YAPS.iNoOfFourierLines"), n);
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "YAPS.iNoOfFourierLines not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find YAPS.iNoOfFourierLines array";
                throw std::runtime_error(sstream.str());

            } else {
                iNoOfFourierLines = atoi(temp[0].c_str());
            }

            long lFirstFourierLine;
            bool has_FirstFourierLine = false;
            n2 = apply_visitor(XProtocol::getChildNodeByName("YAPS.lFirstFourierLine"), n);
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "YAPS.lFirstFourierLine not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::cout << "Failed to find YAPS.lFirstFourierLine array" << std::endl;
                has_FirstFourierLine = false;
            } else {
                lFirstFourierLine = atoi(temp[0].c_str());
                has_FirstFourierLine = true;
            }

            // get the center partition parameters
            n2 = apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.lPartitions"), n);
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "MEAS.sKSpace.lPartitions not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find MEAS.sKSpace.lPartitions array";
                throw std::runtime_error(sstream.str());

            } else {
                lPartitions = atoi(temp[0].c_str());
            }

            // Note: iNoOfFourierPartitions is sometimes absent for 2D sequences
            n2 = apply_visitor(XProtocol::getChildNodeByName("YAPS.iNoOfFourierPartitions"), n);
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
                if (temp.size() != 1) {
                    iNoOfFourierPartitions = 1;
                } else {
                    iNoOfFourierPartitions = atoi(temp[0].c_str());
                }
            } else {
                iNoOfFourierPartitions = 1;
            }

            long lFirstFourierPartition;
            bool has_FirstFourierPartition = false;
            n2 = apply_visitor(XProtocol::getChildNodeByName("YAPS.lFirstFourierPartition"), n);
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "YAPS.lFirstFourierPartition not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::cout << "Failed to find YAPS.lFirstFourierPartition array" << std::endl;
                has_FirstFourierPartition = false;
            } else {
                lFirstFourierPartition = atoi(temp[0].c_str());
                has_FirstFourierPartition = true;
            }

            // set the values
            if (has_FirstFourierLine) // bottom half for partial fourier
            {
                center_line = lPhaseEncodingLines / 2 - (lPhaseEncodingLines - iNoOfFourierLines);
            } else {
                center_line = lPhaseEncodingLines / 2;
            }

            if (iNoOfFourierPartitions > 1) {
                // 3D
                if (has_FirstFourierPartition) // bottom half for partial fourier
                {
                    center_partition = lPartitions / 2 - (lPartitions - iNoOfFourierPartitions);
                } else {
                    center_partition = lPartitions / 2;
                }
            } else {
                // 2D
                center_partition = 0;
            }

            // for spiral sequences the center_line and center_partition are zero
            if (trajectory == Trajectory::TRAJECTORY_SPIRAL) {
                center_line = 0;
                center_partition = 0;
            }

            std::cout << "center_line = " << center_line << std::endl;
            std::cout << "center_partition = " << center_partition << std::endl;
        }

        //Get some parameters - radial views
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("MEAS.sKSpace.lRadialViews"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "MEAS.sKSpace.lRadialViews not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find YAPS.MEAS.sKSpace.lRadialViews array";
                throw std::runtime_error(sstream.str());

            } else {
                radial_views = atoi(temp[0].c_str());
            }
        }
            //Get some parameters - global table position
            {
                const XProtocol::XNode* n2 = apply_visitor(XProtocol::getChildNodeByName("DICOM.lGlobalTablePosSag"), n);
                std::vector<std::string> temp;
                if (n2) {
                    temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
                    if (temp.size() != 1)
                    {
                        global_table_pos[0] = 0;
                    }
                    else
                    {
                        global_table_pos[0] = atol(temp[0].c_str());
                    }
                }
                else {
                    std::cout << "DICOM.lGlobalTablePosSag not found" << std::endl;
                    global_table_pos[0] = 0;
                }

        n2 = apply_visitor(XProtocol::getChildNodeByName("DICOM.lGlobalTablePosCor"), n);
                if (n2) {
                    temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
                    if (temp.size() != 1)
                    {
                        global_table_pos[1] = 0;
                    }
                    else
                    {
                        global_table_pos[1] = atol(temp[0].c_str());
                    }
                }
                else {
                    std::cout << "DICOM.lGlobalTablePosCor not found" << std::endl;
                    global_table_pos[1] = 0;
                }

                n2 = apply_visitor(XProtocol::getChildNodeByName("DICOM.lGlobalTablePosTra"), n);
                if (n2) {
                    temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
                    if (temp.size() != 1)
                    {
                        global_table_pos[2] = 0;
                    }
                    else
                    {
                        global_table_pos[2] = atol(temp[0].c_str());
                    }
                }
                else {
                    std::cout << "DICOM.lGlobalTablePosTra not found" << std::endl;
                    global_table_pos[2] = 0;
                }
            }//Get some parameters - protocol name
        {
            const XProtocol::XNode *n2 = apply_visitor(XProtocol::getChildNodeByName("HEADER.tProtocolName"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            } else {
                std::cout << "HEADER.tProtocolName not found" << std::endl;
            }
            if (temp.size() != 1) {
                std::stringstream sstream;
                sstream << "Failed to find HEADER.tProtocolName";
                throw std::runtime_error(sstream.str());

            } else {
                protocol_name = temp[0];
            }
        }

        // Get some parameters - base line
        {
            const XProtocol::XNode *n2 = apply_visitor(
                    XProtocol::getChildNodeByName("MEAS.sProtConsistencyInfo.tBaselineString"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            }
            if (temp.size() > 0) {
                baseLineString = temp[0];
            }
        }

        if (baseLineString.empty()) {
            const XProtocol::XNode *n2 = apply_visitor(
                    XProtocol::getChildNodeByName("MEAS.sProtConsistencyInfo.tMeasuredBaselineString"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            }
            if (temp.size() > 0) {
                baseLineString = temp[0];
            }
        }

        if (baseLineString.empty()) {
            std::cout << "Failed to find MEAS.sProtConsistencyInfo.tBaselineString/tMeasuredBaselineString"
                      << std::endl;
        }

        // Get software version
        {
            const XProtocol::XNode* n2 = apply_visitor(
                XProtocol::getChildNodeByName("Dicom.SoftwareVersions"), n);
            std::vector<std::string> temp;
            if (n2) {
                temp = apply_visitor(XProtocol::getStringValueArray(), *n2);
            }
            if (temp.size() > 0) {
                software_version = temp[0];
            }
        }

        //xml_config = ProcessParameterMap(n, parammap_file);
        return ProcessParameterMap(n, parammap_file_content.c_str());


    }
    throw std::runtime_error("No Meas buffer found in Siemens dataset");
}

std::vector<MeasurementHeaderBuffer> readMeasurementHeaderBuffers(std::ifstream &siemens_dat, uint32_t num_buffers) {
    auto buffers = std::vector<MeasurementHeaderBuffer>(num_buffers);

    std::cout << "Number of parameter buffers: " << num_buffers << std::endl;

    char tmp_bufname[32];
    for (int b = 0; b < num_buffers; b++) {
        siemens_dat.getline(tmp_bufname, 32, '\0');
        std::cout << "Buffer Name: " << tmp_bufname << std::endl;
        buffers[b].name = std::string(tmp_bufname);
        uint32_t buflen = 0;
        siemens_dat.read((char *) (&buflen), sizeof(buflen));
        char *bytebuf = new char[buflen + 1];
        bytebuf[buflen] = 0;
        siemens_dat.read(bytebuf, buflen);
        std::wstring output = utf_to_utf<wchar_t>(bytebuf, bytebuf + buflen);
        buffers[b].buf = ws2s(output);
        delete[] bytebuf;
    }
    return buffers;
}

std::vector<MrParcRaidFileEntry>
readParcFileEntries(std::ifstream &siemens_dat, const MrParcRaidFileHeader &ParcRaidHead, bool VBFILE) {
    std::vector<MrParcRaidFileEntry> ParcFileEntries(64);

    if (VBFILE) {
        std::cout << "VB line file detected." << std::endl;
        //In case of VB file, we are just going to fill these with zeros. It doesn't exist.
        for (unsigned int i = 0; i < 64; i++) {
            memset(&ParcFileEntries[i], 0, sizeof(MrParcRaidFileEntry));
        }

        ParcFileEntries[0].off_ = 0;
        siemens_dat.seekg(0, std::ios_base::end); //Rewind a bit, we have no raid file header.
        ParcFileEntries[0].len_ = siemens_dat.tellg(); //This is the whole size of the dat file
        siemens_dat.seekg(0, std::ios_base::beg); //Rewind a bit, we have no raid file header.

        std::cout << "Protocol name: " << ParcFileEntries[0].protName_ << std::endl; // blank
    } else {
        std::cout << "VD line file detected." << std::endl;
        for (unsigned int i = 0; i < 64; i++) {
            siemens_dat.read((char *) (&ParcFileEntries[i]), sizeof(MrParcRaidFileEntry));

            if (i < ParcRaidHead.count_) {
                std::cout << "Protocol name [" << i+1 << "]: " << ParcFileEntries[i].protName_ << std::endl;
            }
        }
    }
    return ParcFileEntries;
}

std::string get_file_content(const std::string &file) {

    try {
        return load_file(file);
    }
    catch (...) {}

    try {
        return load_embedded(file);
    }
    catch (...) {}

    throw std::runtime_error("Failed to load file: " + file);
}

std::string select_file(
        const std::string &file,
        const std::string &default_file,
        const bool all_measurements,
        const unsigned int currentMeas) {

    if (file.empty()) return default_file;

    std::vector<std::string> files;
    boost::algorithm::split(files, file, boost::is_any_of(","));

    if (!all_measurements) return file;
    if (files.size() == 1) return file;

    try {
        const std::string &file_token = files.at(currentMeas - 1);
        return file_token.empty() ? default_file : file_token;
    }
    catch (const std::out_of_range &e) {
        std::stringstream message;
        message << "Multiple files provided (" << file << "), but the current measurement (" << currentMeas << ") exceeds the number specified.";
        throw std::runtime_error(message.str());
    }
}
