#include <fstream>

#include "ismrmrd/serialization.h"
#include "ismrmrd/serialization_iostream.h"


template <typename T>
void read_and_log_image(ISMRMRD::ProtocolDeserializer &deserializer)
{
    ISMRMRD::Image<T> img;
    deserializer.deserialize(img);
    std::cout << "IMAGE " << img.getHead().image_series_index << ", " << img.getHead().image_index << std::endl;
}

template <typename T>
void read_and_log_ndarray(ISMRMRD::ProtocolDeserializer &deserializer)
{
    ISMRMRD::NDArray<T> arr;
    deserializer.deserialize(arr);
    std::cout << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
}


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <ismrmrd_stream_filename>" << std::endl;
        return 1;
    }

    std::string ismrmrd_stream_filename(argv[1]);
    std::ifstream ifs(ismrmrd_stream_filename, std::ios::binary);
    ISMRMRD::IStreamView rs(ifs);
    ISMRMRD::ProtocolDeserializer deserializer(rs);


    while (deserializer.peek() != ISMRMRD::ISMRMRD_MESSAGE_CLOSE) {
        if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_HEADER) {
            ISMRMRD::IsmrmrdHeader hdr;
            deserializer.deserialize(hdr);

            std::stringstream xmlstream;
            ISMRMRD::serialize(hdr, xmlstream);
            std::cout << "Header: " << xmlstream.str().size() << " bytes" << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_ACQUISITION) {
            ISMRMRD::Acquisition acq;
            deserializer.deserialize(acq);
            std::cout << "ACQUISITION " << acq.getHead().measurement_uid << ", " << acq.getNumberOfDataElements() << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_IMAGE) {
            if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_USHORT) {
                read_and_log_image<unsigned short>(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_SHORT) {
                read_and_log_image<short>(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_UINT) {
                read_and_log_image<unsigned int>(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_INT) {
                read_and_log_image<int>(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_FLOAT) {
                read_and_log_image<float>(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_DOUBLE) {
                read_and_log_image<double>(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_CXFLOAT) {
                read_and_log_image<std::complex<float> >(deserializer);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_CXDOUBLE) {
                read_and_log_image<std::complex<double> >(deserializer);
            } else {
                throw std::runtime_error("Unknown image type");
            }
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_WAVEFORM) {
            ISMRMRD::Waveform wfm;
            deserializer.deserialize(wfm);
            std::cout << "WAVEFORM " << wfm.size() << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_NDARRAY) {
            if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_USHORT) {
                read_and_log_ndarray<unsigned short>(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_SHORT) {
                read_and_log_ndarray<short>(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_UINT) {
                read_and_log_ndarray<unsigned int>(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_INT) {
                read_and_log_ndarray<int>(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_FLOAT) {
                read_and_log_ndarray<float>(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_DOUBLE) {
                read_and_log_ndarray<double>(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_CXFLOAT) {
                read_and_log_ndarray<std::complex<float> >(deserializer);
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_CXDOUBLE) {
                read_and_log_ndarray<std::complex<double> >(deserializer);
            } else {
                throw std::runtime_error("Unknown nd array type");
            }
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_TEXT) {
            ISMRMRD::TextMessage txt;
            deserializer.deserialize(txt);
            std::cout << "TEXT MESSAGE: " << txt.message << std::endl;
        } else {
            std::stringstream ss;
            ss << "Unknown message type " << deserializer.peek();
            throw std::runtime_error(ss.str());
        }
    }

    // If we can read any more at this point, it is an error
    if (ifs.get() != EOF) {
        throw std::runtime_error("Extra data after ISMRMRD_CLOSE");
    }

    return 0;
}