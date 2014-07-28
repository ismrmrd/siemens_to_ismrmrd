#include "base64.h"


int main(int argc, char *argv[])
{
	std::string infile_xml_VB(argv[1]);
	std::string infile_xml_VD(argv[2]);
	std::string infile_xsl(argv[3]);
	std::string infile_xsd(argv[4]);
	std::string encoded_file(argv[5]);

	std::ifstream t_xml_VB(infile_xml_VB.c_str());
	std::string str_xml_VB((std::istreambuf_iterator<char>(t_xml_VB)), std::istreambuf_iterator<char>());

	std::ifstream t_xml_VD(infile_xml_VD.c_str());
	std::string str_xml_VD((std::istreambuf_iterator<char>(t_xml_VD)), std::istreambuf_iterator<char>());

	std::ifstream t_xsl(infile_xsl.c_str());
	std::string str_xsl((std::istreambuf_iterator<char>(t_xsl)), std::istreambuf_iterator<char>());

	std::ifstream t_xsd(infile_xsd.c_str());
	std::string str_xsd((std::istreambuf_iterator<char>(t_xsd)), std::istreambuf_iterator<char>());

	std::string encoded_xml_VB = base64_encode(reinterpret_cast<const unsigned char*>(str_xml_VB.c_str()), str_xml_VB.length());
	std::string encoded_xml_VD = base64_encode(reinterpret_cast<const unsigned char*>(str_xml_VD.c_str()), str_xml_VD.length());
	std::string encoded_xsl = base64_encode(reinterpret_cast<const unsigned char*>(str_xsl.c_str()), str_xsl.length());
	std::string encoded_xsd = base64_encode(reinterpret_cast<const unsigned char*>(str_xsd.c_str()), str_xsd.length());

	std::ofstream ofs1 (encoded_file.c_str(), std::ofstream::out);

	ofs1 << "#include <iostream>" << std::endl << std::endl;

	ofs1 << "std::string global_xml_VB_string(";
	splitBigString(ofs1, encoded_xml_VB, 4);

	ofs1 << "std::string global_xml_VD_string(";
	splitBigString(ofs1, encoded_xml_VD, 4);

	ofs1 << "std::string global_xsl_string(";
	splitBigString(ofs1, encoded_xsl, 4);

	ofs1 << "std::string global_xsd_string(";
	splitBigString(ofs1, encoded_xsd, 4);

	ofs1.close();

	return 0;
}
