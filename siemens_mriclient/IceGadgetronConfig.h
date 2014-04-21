
#pragma once

#include "GadgetronCommon.h"
#include "url_encode.h"
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>

#include "tinyxml.h"

struct GadgetronEmitter
{
    std::string Config;
    std::string ParameterMap;
    std::string XslStylesheet;
    std::string Anchor;
    bool PassOnData;

    GadgetronEmitter()
    {
        Config = "default.xml";
        ParameterMap = "IsmrmrdParameterMap_Siemens.xml";
        XslStylesheet = "IsmrmrdParameterMap_Siemens.xsl";
        Anchor = "Flags";
        PassOnData = false;
    }

    ~GadgetronEmitter() {}

    void print()
    {
        GADGET_MSG("--> GadgetronEmitter");
        GADGET_MSG("---------------------------");
        GADGET_MSG("Config                      is " << Config);
        GADGET_MSG("ParameterMap                is " << ParameterMap);
        GADGET_MSG("XslStylesheet               is " << XslStylesheet);
        GADGET_MSG("Anchor                      is " << Anchor);
        GADGET_MSG("PassOnData                  is " << PassOnData);
        GADGET_MSG("---------------------------");
    }
};

struct GadgetronInjector
{
    std::string Anchor;

    GadgetronInjector()
    {
        Anchor = "DistorCor2D";
    }

    ~GadgetronInjector() {}

    void print()
    {
        GADGET_MSG("--> GadgetronInjector");
        GADGET_MSG("---------------------------");
        GADGET_MSG("Anchor                      is " << Anchor);
        GADGET_MSG("---------------------------");
    }
};

struct GadgetronConfigurator
{
    bool AutoConfigure;
    bool ProcessDependency;
    bool QueryDependencyStatus;
    bool CleanStoredDependency;
    double TimeLimitStoredDependency;
    std::string DependencyConfig;
    std::string DependencyParameterMap;
    std::string DependencyXslStylesheet;
    std::vector<std::string> FunctorRemoveList;

    GadgetronConfigurator()
    {
        AutoConfigure = true;
        ProcessDependency = true;
        QueryDependencyStatus = true;
        CleanStoredDependency = true;
        TimeLimitStoredDependency = 24.0;
        DependencyConfig = "default_measurement_dependencies.xml";
        DependencyParameterMap = "IsmrmrdParameterMap_Siemens.xml";
        DependencyXslStylesheet = "IsmrmrdParameterMap_Siemens.xsl";
        FunctorRemoveList.clear();
    }

    ~GadgetronConfigurator() {}

    void print()
    {
        GADGET_MSG("--> GadgetronConfigurator");
        GADGET_MSG("---------------------------");
        GADGET_MSG("AutoConfigure               is " << AutoConfigure);
        GADGET_MSG("ProcessDependency           is " << ProcessDependency);
        GADGET_MSG("QueryDependencyStatus       is " << QueryDependencyStatus);
        GADGET_MSG("CleanStoredDependency       is " << CleanStoredDependency);
        GADGET_MSG("TimeLimitStoredDependency   is " << TimeLimitStoredDependency);
        GADGET_MSG("DependencyConfig            is " << DependencyConfig);
        GADGET_MSG("DependencyParameterMap      is " << DependencyParameterMap);
        GADGET_MSG("DependencyXslStylesheet     is " << DependencyXslStylesheet);

        GADGET_MSG("---------------------------");
    }
};

struct IceGadgetronConfig
{
    GadgetronEmitter emitter;
    GadgetronInjector injector;
    GadgetronConfigurator configurator;

    bool read_bool_str(const std::string& valueStr)
    {
        if ( valueStr == "true" || valueStr=="True" || valueStr=="TRUE" )
        {
            return true;
        }

        return false;
    }

    void print()
    {
        GADGET_MSG("IceGadgetronConfig");
        GADGET_MSG("----------------------------------------------------------");
        emitter.print();
        injector.print();
        configurator.print();
        GADGET_MSG("----------------------------------------------------------");
    }
};

bool parseIceGadgetronConfiguration(const std::string& xmlName, IceGadgetronConfig& config)
{
    try
    {
        std::ifstream file(xmlName.c_str());

        if ( !file.good() )
        {
            GADGET_ERROR_MSG("Cannot open xml file : " << xmlName);
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t file_length = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string xml_string(file_length, '\0' );
        file.read((char*)xml_string.data(), file_length);
        file.close();

        GADGET_MSG("Read xml configuration file : " << std::endl << xml_string);

        TiXmlDocument doc;
        doc.Parse(xml_string.c_str());

        TiXmlHandle docHandle(&doc);

        TiXmlElement* pElem=docHandle.FirstChildElement().Element()->FirstChild()->ToElement();

        for ( pElem; pElem!=NULL; pElem=pElem->NextSiblingElement() )
        {
            std::string elemStr = std::string(pElem->Value());

            if ( elemStr == "GadgetronEmitter" )
            {
                TiXmlElement* value = pElem->FirstChildElement("Config")->ToElement();
                if ( value != NULL )
                {
                    config.emitter.Config = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("ParameterMap")->ToElement();
                if ( value != NULL )
                {
                    config.emitter.ParameterMap = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("XslStylesheet")->ToElement();
                if ( value != NULL )
                {
                    config.emitter.XslStylesheet = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("Anchor")->ToElement();
                if ( value != NULL )
                {
                    config.emitter.Anchor = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("PassOnData")->ToElement();
                if ( value != NULL )
                {
                    std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                    config.emitter.PassOnData = config.read_bool_str(valueStr);
                }
            }
            else if ( elemStr == "GadgetronInjector" )
            {
                TiXmlElement* value = pElem->FirstChildElement("Anchor")->ToElement();
                if ( value != NULL )
                {
                    config.injector.Anchor = value->FirstChild()->ToText()->ValueStr();
                }
            }
            else if ( elemStr == "GadgetronConfigurator" )
            {
                TiXmlElement* value = pElem->FirstChildElement("AutoConfigure")->ToElement();
                if ( value != NULL )
                {
                    std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                    config.configurator.AutoConfigure = config.read_bool_str(valueStr);
                }

                value = pElem->FirstChildElement("ProcessDependency")->ToElement();
                if ( value != NULL )
                {
                    std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                    config.configurator.ProcessDependency = config.read_bool_str(valueStr);
                }

                value = pElem->FirstChildElement("QueryDependencyStatus")->ToElement();
                if ( value != NULL )
                {
                    std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                    config.configurator.QueryDependencyStatus = config.read_bool_str(valueStr);
                }

                value = pElem->FirstChildElement("CleanStoredDependency")->ToElement();
                if ( value != NULL )
                {
                    std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                    config.configurator.CleanStoredDependency = config.read_bool_str(valueStr);
                }

                value = pElem->FirstChildElement("TimeLimitStoredDependency")->ToElement();
                if ( value != NULL )
                {
                    std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                    config.configurator.TimeLimitStoredDependency = std::atof(valueStr.c_str());
                }

                value = pElem->FirstChildElement("DependencyConfig")->ToElement();
                if ( value != NULL )
                {
                    config.configurator.DependencyConfig = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("DependencyParameterMap")->ToElement();
                if ( value != NULL )
                {
                    config.configurator.DependencyParameterMap = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("DependencyXslStylesheet")->ToElement();
                if ( value != NULL )
                {
                    config.configurator.DependencyXslStylesheet = value->FirstChild()->ToText()->ValueStr();
                }

                value = pElem->FirstChildElement("FunctorRemoveList")->ToElement();
                if ( value != NULL )
                {
                    do
                    {
                        std::string valueStr = value->FirstChild()->ToText()->ValueStr();
                        config.configurator.FunctorRemoveList.push_back( valueStr );
                        value = value->NextSiblingElement( "FunctorRemoveList" );
                    }
                    while ( value != NULL );
                }
            }
            else
            {
                return false;
            }
        }
    }
    catch(...)
    {
        GADGET_ERROR_MSG("Errors happened in parseIceGadgetronConfiguration(const std::string& xmlName, IceGadgetronConfig& config) ... ");
        return false;
    }

    return true;
}
