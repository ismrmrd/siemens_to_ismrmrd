#include <fstream>

#include "ismrmrd/serialization.h"
#include "ismrmrd/serialization_iostream.h"


template <typename T>
void log_image(ISMRMRD::Image<T> &img)
{
    std::cerr << "IMAGE " << img.getHead().image_series_index << ", " << img.getHead().image_index << std::endl;
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
            std::cerr << "Header: " << xmlstream.str().size() << " bytes" << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_ACQUISITION) {
            ISMRMRD::Acquisition acq;
            deserializer.deserialize(acq);
            std::cerr << "ACQUISITION " << acq.getHead().measurement_uid << ", " << acq.getNumberOfDataElements() << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_IMAGE) {
            if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_USHORT) {
                ISMRMRD::Image<unsigned short> img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_SHORT) {
                ISMRMRD::Image<short> img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_UINT) {
                ISMRMRD::Image<unsigned int> img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_INT) {
                ISMRMRD::Image<int> img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_FLOAT) {
                ISMRMRD::Image<float> img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_DOUBLE) {
                ISMRMRD::Image<double> img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_CXFLOAT) {
                ISMRMRD::Image<std::complex<float> > img;
                deserializer.deserialize(img);
                log_image(img);
            } else if (deserializer.peek_image_data_type() == ISMRMRD::ISMRMRD_CXDOUBLE) {
                ISMRMRD::Image<std::complex<double> > img;
                deserializer.deserialize(img);
                log_image(img);
            } else {
                throw std::runtime_error("Unknown image type");
            }
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_WAVEFORM) {
            ISMRMRD::Waveform wfm;
            deserializer.deserialize(wfm);
            std::cerr << "WAVEFORM " << wfm.size() << std::endl;
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_NDARRAY) {
            if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_USHORT) {
                ISMRMRD::NDArray<unsigned short> arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_SHORT) {
                ISMRMRD::NDArray<short> arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_UINT) {
                ISMRMRD::NDArray<unsigned int> arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_INT) {
                ISMRMRD::NDArray<int> arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_FLOAT) {
                ISMRMRD::NDArray<float> arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_DOUBLE) {
                ISMRMRD::NDArray<double> arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_CXFLOAT) {
                ISMRMRD::NDArray<std::complex<float> > arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else if (deserializer.peek_ndarray_data_type() == ISMRMRD::ISMRMRD_CXDOUBLE) {
                ISMRMRD::NDArray<std::complex<double> > arr;
                deserializer.deserialize(arr);
                std::cerr << "NDARRAY " << arr.getNDim() << ", " << arr.getDataType() << std::endl;
            } else {
                throw std::runtime_error("Unknown nd array type");
            }
        } else if (deserializer.peek() == ISMRMRD::ISMRMRD_MESSAGE_TEXT) {
            ISMRMRD::TextMessage txt;
            deserializer.deserialize(txt);
            std::cerr << "TEXT MESSAGE: " << txt.message << std::endl;
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