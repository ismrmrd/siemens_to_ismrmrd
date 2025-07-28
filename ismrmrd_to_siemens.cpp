/**
 * @file ismrmrd_to_siemens.cpp
 * @brief Converts ISMRMRD data to Siemens format.
 *
 * This file contains the main function and logic to read ISMRMRD data,
 * convert it to Siemens format, and write the output to a specified file.
 *
 * The output format is as follows:
 * -
 */
#include "ismrmrd/ismrmrd.h"
#include "ismrmrd/version.h"
#include "ismrmrd/xml.h"
#include "ismrmrd/waveform.h"
#include "ismrmrd/serialization.h"
#include "ismrmrd/serialization_iostream.h"
#include "converter_version.h"

#include "XNodeBuilder.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/filesystem.hpp>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include <iostream>


enum SIEMENS_MESSAGE_ID {
    SIEMENS_MESSAGE_HEADER = 1,
    SIEMENS_MESSAGE_CLOSE = 15,
    SIEMENS_MESSAGE_ACQUISITION = 1000,
    SIEMENS_MESSAGE_IMAGE = 2000,
    SIEMENS_MESSAGE_IMAGE_CXFLOAT = 2002,
    SIEMENS_MESSAGE_IMAGE_FLOAT = 2004,
    SIEMENS_MESSAGE_IMAGE_UINT16 = 2009,
    SIEMENS_MESSAGE_WAVEFORM = 3000,
};

template <typename T>
ISMRMRD::Image<T> flip_image(ISMRMRD::Image<T> &img)
{
    ISMRMRD::Image<T> flipped(img);
    for (int16_t i = 0; i < img.getMatrixSizeX(); i++)
    {
        for (int16_t j = 0; j < img.getMatrixSizeY(); j++)
        {
            for (int16_t k = 0; k < img.getMatrixSizeZ(); k++)
            {
                flipped(i, j, k) = img(img.getMatrixSizeX() - i - 1, img.getMatrixSizeY() - j - 1, img.getMatrixSizeZ() - k - 1);
            }
        }
    }
    return flipped;
}

template <typename T>
void convertImage(ISMRMRD::Image<T>& img, std::ostream& out) {
    // std::cerr << "Converting image " << img.getImageIndex() << std::endl;

    ISMRMRD::Image<T> image = flip_image(img);

    uint32_t image_msg = SIEMENS_MESSAGE_ID::SIEMENS_MESSAGE_IMAGE;
    out.write(reinterpret_cast<const char*>(&image_msg), sizeof(image_msg));

    XProtocol::MiniHeaderBuilder builder;

    builder.setLong("NumberInSeries", image.getImageIndex());
    // builder.setString("AcquisitionDate", "19990919");
    // builder.setString("AcquisitionTime", "142943.497500");
    // builder.setString("AcquisitionNumber", "1");
    // builder.setLong("EchoNumber", 1);
    // builder.setLong("SliceNo", 0);
    // builder.setDouble("TR", 300.0);
    // builder.setDouble("TE", 15.0);
    // builder.setDouble("SliceMeasurementDuration", 76807.5);
    // builder.setString("SequenceDescription", "se_15b130");
    // builder.setDouble("TimeAfterStart", 0.0);
    // builder.setString("UsedChannelString", "X");
    // builder.setString("SequenceString", "se2d1");
    // builder.setLong("EchoColumnPosition", 128);
    // builder.setLong("EchoLinePosition", 128);
    // builder.setLong("EchoPartitionPosition", 32);
    // builder.setLong("RealDwellTime", 15000);
    // builder.setLong("EchoTrainLength", 1);
    // builder.setLong("NoOfAverages", 1);
    // builder.setLong("NoOfPhaseEncodingSteps", 256);
    // builder.setDouble("PercentPhaseFoV", 100.0);
    // builder.setDouble("PercentSampling", 100.0);
    // builder.setLong("ProtocolSliceNumber", 0);
    // builder.setLong("SequenceMask", 8);
    // builder.setLong("BitsStored", 12);
    // builder.setString("CoilString", "C:BC");
    // builder.setString("IceDimString", "1_1_1_1_1_1_1_1_1_1_1_10");
    builder.setDoubleArray("ColVec", {image.getPhaseDirectionX(), image.getPhaseDirectionY(), image.getPhaseDirectionZ()});
    builder.setDoubleArray("RowVec", {image.getReadDirectionX(), image.getReadDirectionY(), image.getReadDirectionZ()});
    builder.setDoubleArray("PosVec", {image.getPositionX(), image.getPositionY(), image.getPositionZ()});
    // builder.setDoubleArray("PixelSpacing", {1.0, 1.0, 1.0});
    builder.setDoubleArray("NormalVec", {image.getSliceDirectionX(), image.getSliceDirectionY(), image.getSliceDirectionZ()});
    // builder.setString("PhaseEncodingDirection", "+COL");
    builder.setDoubleArray("PhaseVec", {image.getPhaseDirectionX(), image.getPhaseDirectionY(), image.getPhaseDirectionZ()});
    builder.setLongArray("TablePosition", {static_cast<long>(image.getPatientTablePositionX() / 1000),
                                             static_cast<long>(image.getPatientTablePositionY() / 1000),
                                             static_cast<long>(image.getPatientTablePositionZ() / 1000)});
    builder.setDoubleArray("PosVecSBCS", {image.getPositionX(), image.getPositionY(), image.getPositionZ()});
    // builder.setDouble("SliceLocation", 0.0);
    // builder.setBool("SwapReadPhase", false);
    // builder.setDouble("SliceThickness", 5.0);
    // builder.setLong("WindowCenter", 1583);
    // builder.setLong("WindowWidth", 2325);
    // builder.setLong("LargestImagePixelValue", 2226);
    // builder.setLong("SmallestImagePixelValue", 0);
    // builder.setLong("NoOfCols", 256);
    // builder.setLong("NoOfRows", 256);

    uint32_t img_type_msg = SIEMENS_MESSAGE_ID::SIEMENS_MESSAGE_CLOSE;
    if (sizeof(T) == sizeof(complex_float_t)) {
        img_type_msg = SIEMENS_MESSAGE_ID::SIEMENS_MESSAGE_IMAGE_CXFLOAT;
        builder.setString("ImageTypeValue3", "P");
    } else if (sizeof(T) == sizeof(float)) {
        img_type_msg = SIEMENS_MESSAGE_ID::SIEMENS_MESSAGE_IMAGE_FLOAT;
        builder.setString("ImageTypeValue3", "M");
        builder.setString("ComplexImageComponent", "MAGNITUDE");
        builder.setString("PixelRepresentation", "0"); // unsigned
    } else if (sizeof(T) == sizeof(uint16_t)) {
        img_type_msg = SIEMENS_MESSAGE_ID::SIEMENS_MESSAGE_IMAGE_UINT16;
        builder.setString("ImageTypeValue3", "M");
        builder.setString("ComplexImageComponent", "MAGNITUDE");
        builder.setString("PixelRepresentation", "0"); // unsigned
    } else {
        throw std::runtime_error("Unsupported image type for conversion.");
    }

    // TODO: Does this need to be set?
    // builder.setStringArray("ImageTypeValue4", {"ND"});

    out.write(reinterpret_cast<const char*>(&img_type_msg), sizeof(img_type_msg));

    builder.setLongArray("MatrixSize", {256, 256, 1});
    builder.setLong("NoOfChannels", 1);
    builder.setDoubleArray("FieldOfView", {200.0, 200.0, 5.0});

    std::string miniHeader(builder.build());
    uint32_t miniHeaderSize = static_cast<uint32_t>(miniHeader.size());
    out.write(reinterpret_cast<const char*>(&miniHeaderSize), sizeof(miniHeaderSize));
    out.write(miniHeader.c_str(), miniHeader.size());

    uint32_t dataSize = static_cast<uint32_t>(image.getDataSize());
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    out.write(reinterpret_cast<const char*>(image.getDataPtr()), image.getDataSize());
}


int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Change std::cin/std::cout to binary mode
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    std::string ismrmrd_filename;
    std::string output_filename;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce HELP message")
        ("version,v", "Prints converter version and ISMRMRD version")
        ("file,f", po::value<std::string>(&ismrmrd_filename), "<ISMRMRD file (default: stdin)>")
        ("output,o", po::value<std::string>(&output_filename), "<Siemens output file (default: stdout)>")
        ;

    po::options_description display_options("Allowed options");
    display_options.add_options()
        ("help,h", "Produce HELP message")
        ("version,v", "Prints converter version and ISMRMRD version")
        ("file,f", "<ISMRMRD file (default: stdin)>")
        ("output,o", "<Siemens output file (default: stdout)>")
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cerr << display_options << "\n";
            return 0;
        }

        if (vm.count("version")) {
            std::cerr << "Converter version is: " << SIEMENS_TO_ISMRMRD_VERSION_MAJOR << "."
                << SIEMENS_TO_ISMRMRD_VERSION_MINOR << "." << SIEMENS_TO_ISMRMRD_VERSION_PATCH << "\n";
            std::cerr << "Built against ISMRMRD version: " << ISMRMRD_VERSION_MAJOR << "." << ISMRMRD_VERSION_MINOR
                << "." << ISMRMRD_VERSION_PATCH << "\n";
            return 0;
        }
    } catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << display_options << std::endl;
        return -1;
    }

    std::unique_ptr<std::istream> infile;
    if (ismrmrd_filename.length() != 0)
    {
        infile = std::make_unique<std::ifstream>(ismrmrd_filename.c_str(), std::ios::binary);
        if (!infile->good())
        {
            std::cerr << "Provided ISMRMRD file can not be opened or does not exist." << std::endl;
            std::cerr << display_options << "\n";
            return -1;
        }
        std::cerr << "ISMRMRD file is: " << ismrmrd_filename << std::endl;
    }

    std::unique_ptr<std::ostream> outfile;
    if (output_filename.length() != 0)
    {
        outfile = std::make_unique<std::ofstream>(output_filename.c_str(), std::ios::binary);
        if (!outfile->good())
        {
            std::cerr << "Provided Siemens output file can not be opened." << std::endl;
            std::cerr << display_options << "\n";
            return -1;
        }
        std::cerr << "Siemens output file is: " << output_filename << std::endl;
    }

    std::ostream& output = outfile ? *outfile : std::cout;
    ISMRMRD::IStreamView rs(infile ? *infile : std::cin);
    ISMRMRD::ProtocolDeserializer deserializer(rs);

    while (deserializer.peek() != ISMRMRD::ISMRMRD_MESSAGE_CLOSE) {
        if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_HEADER) {
            ISMRMRD::IsmrmrdHeader header;
            deserializer.deserialize(header);
            std::cerr << "ISMRMRD Header received - DISCARDED" << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_ACQUISITION) {
            ISMRMRD::Acquisition acq;
            deserializer.deserialize(acq);
            std::cerr << "Acquisition received - DISCARDED" << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_WAVEFORM) {
            ISMRMRD::Waveform waveform;
            deserializer.deserialize(waveform);
            std::cerr << "Waveform received - DISCARDED" << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_IMAGE) {
            if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_USHORT) {
                ISMRMRD::Image<unsigned short> image;
                deserializer.deserialize(image);
                convertImage(image, output);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_FLOAT) {
                ISMRMRD::Image<float> image;
                deserializer.deserialize(image);
                convertImage(image, output);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_CXFLOAT) {
                ISMRMRD::Image<std::complex<float>> image;
                deserializer.deserialize(image);
                convertImage(image, output);
            } else {
                std::cerr << "Unsupported image data type encountered." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown message type encountered." << std::endl;
            return 1;
        }
    }

    uint32_t close_msg = SIEMENS_MESSAGE_ID::SIEMENS_MESSAGE_CLOSE;
    output.write(reinterpret_cast<const char*>(&close_msg), sizeof(close_msg));
    output.flush();

    std::cerr << "Finished converting ISMRMRD input to Siemens format" << std::endl;
    return 0;
}