#include "CodeUtil.h"
#include "StringPiece.h"

namespace wfrest
{

namespace
{

int hex_value(unsigned char chr)
{
    if (chr >= '0' && chr <= '9')
        return chr - '0';
    if (chr >= 'A' && chr <= 'F')
        return chr - 'A' + 10;
    if (chr >= 'a' && chr <= 'f')
        return chr - 'a' + 10;
    return -1;
}

} // namespace

std::string CodeUtil::url_encode(const std::string &value)
{
    static auto hex_chars = "0123456789ABCDEF";

    std::string result;
    result.reserve(value.size()); // Minimum size of result

    for (auto &chr : value) 
    {
        if (!((chr >= '0' && chr <= '9') || (chr >= 'A' && chr <= 'Z') ||
            (chr >= 'a' && chr <= 'z') || chr == '-' || chr == '.' ||
            chr == '_' || chr == '~' || chr == '/'))
        {
            result += std::string("%") +
                    hex_chars[static_cast<unsigned char>(chr) >> 4] +
                    hex_chars[static_cast<unsigned char>(chr) & 15];
        } else
        {
            result += chr;
        }   
    }

    return result;
}

std::string CodeUtil::url_decode(const std::string &value)
{
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) 
    {
        const unsigned char chr = static_cast<unsigned char>(value[i]);
        if (chr == '%' && i + 2 < value.size()) 
        {
            const int high = hex_value(static_cast<unsigned char>(value[i + 1]));
            const int low = hex_value(static_cast<unsigned char>(value[i + 2]));
            if (high >= 0 && low >= 0)
            {
                result.push_back(static_cast<char>((high << 4) | low));
                i += 2;
            }
            else
            {
                result.push_back('%');
            }
        } else if (chr == '+')
        {
            result.push_back(' ');
        } else
        {
            result.push_back(static_cast<char>(chr));
        }
    }
    return result;
}

bool CodeUtil::is_url_encode(const std::string &str)
{
    return str.find("%") != std::string::npos ||
           str.find("+") != std::string::npos;
}

}  // namespace wfrest
