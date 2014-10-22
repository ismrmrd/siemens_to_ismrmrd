#include "base64.h"


int main(int argc, char *argv[])
{
    std::string infile_xml_VB(argv[1]);
    std::string infile_xml_VD(argv[2]);
    std::string infile_xsl_1(argv[3]);
    std::string infile_xsl_2(argv[4]);
    std::string infile_xsl_3(argv[5]);
    std::string infile_xsl_4(argv[6]);
    std::string infile_xsd(argv[7]);
    std::string encoded_file(argv[8]);



    std::ifstream t_xml_VB(infile_xml_VB.c_str());
    std::string str_xml_VB((std::istreambuf_iterator<char>(t_xml_VB)), std::istreambuf_iterator<char>());

    std::ifstream t_xml_VD(infile_xml_VD.c_str());
    std::string str_xml_VD((std::istreambuf_iterator<char>(t_xml_VD)), std::istreambuf_iterator<char>());

    std::ifstream t_xsl_1(infile_xsl_1.c_str());
    std::string str_xsl_1((std::istreambuf_iterator<char>(t_xsl_1)), std::istreambuf_iterator<char>());

    std::ifstream t_xsl_2(infile_xsl_2.c_str());
    std::string str_xsl_2((std::istreambuf_iterator<char>(t_xsl_2)), std::istreambuf_iterator<char>());

    std::ifstream t_xsl_3(infile_xsl_3.c_str());
    std::string str_xsl_3((std::istreambuf_iterator<char>(t_xsl_3)), std::istreambuf_iterator<char>());

    std::ifstream t_xsl_4(infile_xsl_4.c_str());
    std::string str_xsl_4((std::istreambuf_iterator<char>(t_xsl_4)), std::istreambuf_iterator<char>());

    std::ifstream t_xsd(infile_xsd.c_str());
    std::string str_xsd((std::istreambuf_iterator<char>(t_xsd)), std::istreambuf_iterator<char>());

    std::string encoded_xml_VB = base64_encode(reinterpret_cast<const unsigned char*>(str_xml_VB.c_str()), str_xml_VB.length());
    std::string encoded_xml_VD = base64_encode(reinterpret_cast<const unsigned char*>(str_xml_VD.c_str()), str_xml_VD.length());
    std::string encoded_xsl_1 = base64_encode(reinterpret_cast<const unsigned char*>(str_xsl_1.c_str()), str_xsl_1.length());
    std::string encoded_xsl_2 = base64_encode(reinterpret_cast<const unsigned char*>(str_xsl_2.c_str()), str_xsl_2.length());
    std::string encoded_xsl_3 = base64_encode(reinterpret_cast<const unsigned char*>(str_xsl_3.c_str()), str_xsl_3.length());
    std::string encoded_xsl_4 = base64_encode(reinterpret_cast<const unsigned char*>(str_xsl_4.c_str()), str_xsl_4.length());

    std::string encoded_xsd = base64_encode(reinterpret_cast<const unsigned char*>(str_xsd.c_str()), str_xsd.length());

    std::ofstream ofs1 (encoded_file.c_str(), std::ofstream::out);

    ofs1 << "#include <iostream>" << std::endl << std::endl;

    ofs1 << "std::string global_xml_VB_string(";
    splitBigString(ofs1, encoded_xml_VB, 12);

    ofs1 << "std::string global_xml_VD_string(";
    splitBigString(ofs1, encoded_xml_VD, 12);

    ofs1 << "std::string global_xsl_string_1(";
    splitBigString(ofs1, encoded_xsl_1, 12);

    ofs1 << "std::string global_xsl_string_2(";
    splitBigString(ofs1, encoded_xsl_2, 12);

    ofs1 << "std::string global_xsl_string_3(";
    splitBigString(ofs1, encoded_xsl_3, 12);

    ofs1 << "std::string global_xsl_string_4(";
    splitBigString(ofs1, encoded_xsl_4, 12);

    ofs1 << "std::string global_xsd_string(";
    splitBigString(ofs1, encoded_xsd, 12);

    ofs1.close();

    return 0;
}
