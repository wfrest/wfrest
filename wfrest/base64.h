// modified from baidu lib
#ifndef WFREST_base64_H_
#define WFREST_base64_H_

#include <string>

namespace wfrest
{

class Base64
{
public:
    static std::string encode(unsigned char const *bytes_to_encode, unsigned int len);

    static std::string decode(std::string const &encoded_string);

private:
    static const std::string base64_chars;
};

}   // namespace wfrest



#endif // WFREST_base64_H_
