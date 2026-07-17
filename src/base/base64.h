// modified from baidu lib
#ifndef WFREST_BASE64_H_
#define WFREST_BASE64_H_

#include <cstddef>
#include <string>

namespace wfrest
{

class Base64
{
public:
    static std::string encode(const unsigned char *bytes_to_encode, unsigned int len);

    static bool encode(const unsigned char *data,
                       size_t len,
                       std::string *output);

    static std::string decode(std::string const &encoded_string);

    static bool decode(const std::string& encoded_string,
                       std::string *output);

private:
    static const std::string base64_chars;
};

}   // namespace wfrest

#endif // WFREST_BASE64_H_
