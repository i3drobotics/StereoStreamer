#include "image2string.h"

Image2String::Image2String() {}

const std::string Image2String::base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

inline bool Image2String::is_base64( unsigned char c )
{ 
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Image2String::base64_encode(uchar const* bytes_to_encode, unsigned int in_len)
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

			for (i = 0; (i <4); i++) 
			{
				ret += base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if (i) 
	{
		for (j = i; j < 3; j++) 
		{
			char_array_3[j] = '\0';
		}

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++) 
		{
			ret += base64_chars[char_array_4[j]];
		}
		
		while ((i++ < 3)) 
		{
			ret += '=';
		}
	}

	return ret;
}

std::string Image2String::base64_decode(std::string const& encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) 
	{
		char_array_4[i++] = encoded_string[in_]; in_++;

		if (i == 4) 
		{
			for (i = 0; i < 4; i++) 
			{	
				char_array_4[i] = base64_chars.find(char_array_4[i]);
			}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
			{
				ret += char_array_3[i];
			}

			i = 0;
		}
	}

	if (i) 
	{
		for (j = i; j < 4; j++) 
		{
			char_array_4[j] = 0;
		}
		
		for (j = 0; j < 4; j++) 
		{	
			char_array_4[j] = base64_chars.find(char_array_4[j]);
		}

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
		{	
			ret += char_array_3[j];
		}
	}

	return ret;
}

std::string Image2String::mat2str(const cv::Mat& m, bool uncompressed, int quality)
{
    uchar* result;
    std::vector<uchar> buf;
    if (uncompressed){
        cv::imencode(".tiff", m, buf);
        result = reinterpret_cast<uchar*> (&buf[0]);
    } else {
        int params[3] = {0};
        params[0] = cv::IMWRITE_JPEG_QUALITY;
        params[1] = quality;

        cv::imencode(".jpg", m, buf, std::vector<int>(params, params+2));
        result = reinterpret_cast<uchar*> (&buf[0]);
    }
    return base64_encode(result, buf.size());
}

cv::Mat Image2String::str2mat(const std::string& s)
{
	// Decode data
    std::string decoded_string = base64_decode(s);
    std::vector<uchar> data(decoded_string.begin(), decoded_string.end());

    cv::Mat img = imdecode(data, cv::IMREAD_UNCHANGED);
	return img;
}

Image2String::~Image2String()
{
	// TODO Auto-generated destructor stub
}
