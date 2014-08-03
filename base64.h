#ifndef base64_h
#define base64_h

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>


static const std::string base64_chars =
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "abcdefghijklmnopqrstuvwxyz"
               "0123456789+/";


static inline bool is_base64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& encoded_string);
void splitBigString(std::ofstream &resFile, std::string bigString, int parts);

#endif

