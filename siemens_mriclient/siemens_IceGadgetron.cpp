
#include "ace/Log_Msg.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_string.h"

#include "IceGadgetronConfig.h"
#include "IceGadgetron.hxx"

using namespace Gadgetron;

void print_usage() 
{
    ACE_DEBUG((LM_INFO, ACE_TEXT("siemens_IceGadgetron is to validate the IceGadgetron configuration xml file \n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("Usage: \n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("siemens_IceGadgetron <FILENAME>  (xml file name\n") ));
    ACE_DEBUG((LM_INFO, ACE_TEXT("                     -t          (besides the schema generated parser, also use the simple xml parser to read)\n") ));

}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[] )
{
    bool useSimpleParser = false;
    std::string xmlName;

    if (argc > 1)
    {
        xmlName = std::string(argv[1]);
    }

    static const ACE_TCHAR options[] = ACE_TEXT(":tX:");
    ACE_Get_Opt cmd_opts(argc, argv, options);

    int option;
    while ((option = cmd_opts()) != EOF)
    {
        switch (option)
        {
        case 't':
            useSimpleParser = true;
            break;
        case ':':
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("-%c requires an argument.\n"), cmd_opts.opt_opt()),-1);
            break;
        default:
            ACE_ERROR_RETURN( (LM_ERROR, ACE_TEXT("Command line parse error\n")), -1);
            break;
        }
    }

    if ( xmlName.empty() )
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("xml file name is not set.\n")));
        print_usage();
        return -1;
    }

    // load in the xml using the schema
    std::ifstream file(xmlName);
    file.seekg(0, std::ios::end);
    size_t file_length = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string xml_string(file_length, '\0' );
    file.read((char*)xml_string.data(), file_length);
    file.close();

    char * gadgetron_home = ACE_OS::getenv("GADGETRON_HOME");
    std::string schema_file_name = std::string(gadgetron_home) + std::string("/schema/IceGadgetron.xsd");

    std::string tmp(schema_file_name);
    tmp = url_encode(tmp);
    schema_file_name = tmp;

    xml_schema::properties props;
    props.schema_location (
        "http://gadgetron.sf.net/gadgetron",
        std::string (schema_file_name));

    bool parse_success = false;

    std::istringstream str_stream(xml_string, std::stringstream::in);
    try
    {
        std::auto_ptr< ::gadgetron::IceGadgetronConfiguration > cfg = gadgetron::IceGadgetronConfiguration_ (str_stream,0,props);
        parse_success = true;
    }
    catch (const xml_schema::exception& e)
    {
        GADGET_ERROR_MSG("Failed to parse IceGadgetron configuration xml : " << e.what());
    }

    if ( parse_success )
    {
        GADGET_MSG("Success in parsing IceGadgetron configuration xml : " << xmlName);
    }

    if ( useSimpleParser )
    {
        IceGadgetronConfig config;

        if ( parseIceGadgetronConfiguration(xmlName, config) )
        {
            GADGET_MSG("Success in parsing IceGadgetron configuration xml with simple parser : " << xmlName);

            config.print();
        }
        else
        {
            GADGET_ERROR_MSG("Failed to parse IceGadgetron configuration xml with simple parser ... ");
        }
    }

    return 0;
}
