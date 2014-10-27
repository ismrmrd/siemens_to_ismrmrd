#include "base64.h"
#include "sstream"

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }
    return ret;
}


std::string base64_decode(std::string const& encoded_string)
{
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4)
        {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

              char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
              char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
              char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

              for (i = 0; (i < 3); i++)
                  ret += char_array_3[i];
              i = 0;
        }
    }

    if (i)
    {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

          char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
          char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
          char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

          for (j = 0; (j < i - 1); j++)
              ret += char_array_3[j];
    }
    return ret;
}

void splitBigString(std::ofstream &resFile, std::string bigString, int parts)
{
  int len = bigString.length();

  int at, pre=0, i;

  int size_a_part = len/parts;
  std::cout << "size_a_part = " << size_a_part << std::endl;

  if ( size_a_part == 0 )
    {
      resFile << "    v = \""<< bigString.substr(0, std::string::npos) << "\";" << std::endl;
    }
  else
    {
      for (pre = i = 0; i < parts-1; ++i)
        {
	  std::stringstream str;
	  str << "    std::string v" << i << " = " << "\""<< bigString.substr(pre, size_a_part) << "\";" << std::endl;
	  resFile << str.str();
	  pre += size_a_part;
        }

      {
	std::stringstream str;
	str << "    std::string v" << parts-1 << " = " << "\""<< bigString.substr(pre, std::string::npos) << "\";" << std::endl;
	resFile << str.str();
      }

      for (pre = i = 0; i < parts; ++i)
        {
	  std::stringstream str;
	  str << "    v += v" << i << ";" << std::endl;
	  resFile << str.str();
        }
    }
}

