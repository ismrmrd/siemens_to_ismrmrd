#include "base64.h"
#include "boost/filesystem.hpp"

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [files to embed] <output file>" << std::endl;
        return 1;
    }

    char *outfile_name = argv[argc - 1];
    std::ofstream output(outfile_name, std::ofstream::out);
    output << "#include <iostream>" << std::endl;
    output << "#include <map>" << std::endl << std::endl;
    output << "#include <string>" << std::endl << std::endl;

    std::string map_name = "global_embedded_files";
    output << "std::map<std::string, std::string> "<< map_name << ";" << std::endl << std::endl;

    output << "void initializeEmbeddedFiles(void) {" << std::endl;

    for (int i = 1; i < argc - 1; i++) {
        char *infile_name = argv[i];
        std::ifstream infile(infile_name);
        std::string contents((std::istreambuf_iterator<char>)(infile), std::istreambuf_iterator<char>());

        std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(contents.c_str()), contents.length());

        boost::filesystem::path path(infile_name);
        std::string keyname = path.filename().string();
        output << "    " << map_name << "[\"" << keyname << "\"] = ";
        splitBigString(output, encoded, 12);
    }

    output << "}" << std::endl;

    output.close();

    return 0;
}
