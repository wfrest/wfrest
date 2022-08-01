#include "CodeUtil.h"
#include "StringPiece.h"

namespace wfrest
{

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
    result.reserve(value.size() / 3 + (value.size() % 3)); // Minimum size of result

    for (std::size_t i = 0; i < value.size(); ++i) 
    {
        auto &chr = value[i];
        if (chr == '%' && i + 2 < value.size()) 
        {
            auto hex = value.substr(i + 1, 2);
            auto decoded_chr =
                static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result += decoded_chr;
            i += 2;
        } else if (chr == '+')
        {
            result += ' ';
        } else
        {
            result += chr;
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