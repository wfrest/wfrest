#include "base64.h"

#include <utility>

namespace wfrest
{

namespace
{

bool fail(std::string *output)
{
    if (output != nullptr)
        output->clear();
    return false;
}

int base64_value(unsigned char byte)
{
    if (byte >= 'A' && byte <= 'Z')
        return byte - 'A';
    if (byte >= 'a' && byte <= 'z')
        return byte - 'a' + 26;
    if (byte >= '0' && byte <= '9')
        return byte - '0' + 52;
    if (byte == '+')
        return 62;
    if (byte == '/')
        return 63;
    return -1;
}

} // namespace

const std::string Base64::base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string Base64::encode(const unsigned char *bytes_to_encode,
                           unsigned int len)
{
    std::string output;
    (void)encode(bytes_to_encode, static_cast<size_t>(len), &output);
    return output;
}

bool Base64::encode(const unsigned char *data,
                    size_t len,
                    std::string *output)
{
    if (output == nullptr)
        return false;
    if (data == nullptr && len != 0)
        return fail(output);

    std::string encoded;
    const size_t groups = len / 3 + (len % 3 == 0 ? 0 : 1);
    if (groups > encoded.max_size() / 4)
        return fail(output);
    encoded.reserve(groups * 4);

    size_t offset = 0;
    while (len - offset >= 3)
    {
        const unsigned int first = data[offset];
        const unsigned int second = data[offset + 1];
        const unsigned int third = data[offset + 2];
        encoded.push_back(base64_chars[first >> 2]);
        encoded.push_back(base64_chars[((first & 0x03U) << 4) |
                                       (second >> 4)]);
        encoded.push_back(base64_chars[((second & 0x0FU) << 2) |
                                       (third >> 6)]);
        encoded.push_back(base64_chars[third & 0x3FU]);
        offset += 3;
    }

    const size_t remaining = len - offset;
    if (remaining == 1)
    {
        const unsigned int first = data[offset];
        encoded.push_back(base64_chars[first >> 2]);
        encoded.push_back(base64_chars[(first & 0x03U) << 4]);
        encoded.append("==");
    }
    else if (remaining == 2)
    {
        const unsigned int first = data[offset];
        const unsigned int second = data[offset + 1];
        encoded.push_back(base64_chars[first >> 2]);
        encoded.push_back(base64_chars[((first & 0x03U) << 4) |
                                       (second >> 4)]);
        encoded.push_back(base64_chars[(second & 0x0FU) << 2]);
        encoded.push_back('=');
    }

    *output = std::move(encoded);
    return true;
}

std::string Base64::decode(const std::string& encoded_string)
{
    std::string output;
    (void)decode(encoded_string, &output);
    return output;
}

bool Base64::decode(const std::string& encoded_string,
                    std::string *output)
{
    if (output == nullptr)
        return false;

    const size_t length = encoded_string.size();
    size_t padding = 0;
    while (padding < length &&
           encoded_string[length - padding - 1] == '=')
    {
        ++padding;
    }
    if (padding > 2)
        return fail(output);

    const size_t data_length = length - padding;
    const size_t remainder = data_length % 4;
    if (remainder == 1 ||
        (padding != 0 && length % 4 != 0) ||
        (padding == 1 && remainder != 3) ||
        (padding == 2 && remainder != 2))
    {
        return fail(output);
    }

    for (size_t i = 0; i < data_length; ++i)
    {
        if (base64_value(
                static_cast<unsigned char>(encoded_string[i])) < 0)
        {
            return fail(output);
        }
    }

    if (remainder == 2)
    {
        const int value = base64_value(
            static_cast<unsigned char>(encoded_string[data_length - 1]));
        if ((value & 0x0F) != 0)
            return fail(output);
    }
    else if (remainder == 3)
    {
        const int value = base64_value(
            static_cast<unsigned char>(encoded_string[data_length - 1]));
        if ((value & 0x03) != 0)
            return fail(output);
    }

    std::string decoded;
    const size_t complete_groups = data_length / 4;
    if (complete_groups > decoded.max_size() / 3)
        return fail(output);
    const size_t complete_size = complete_groups * 3;
    const size_t tail_size = remainder == 0 ? 0 : remainder - 1;
    if (tail_size > decoded.max_size() - complete_size)
        return fail(output);
    decoded.reserve(complete_size + tail_size);

    size_t offset = 0;
    for (size_t group = 0; group < complete_groups; ++group)
    {
        const unsigned int first = static_cast<unsigned int>(base64_value(
            static_cast<unsigned char>(encoded_string[offset])));
        const unsigned int second = static_cast<unsigned int>(base64_value(
            static_cast<unsigned char>(encoded_string[offset + 1])));
        const unsigned int third = static_cast<unsigned int>(base64_value(
            static_cast<unsigned char>(encoded_string[offset + 2])));
        const unsigned int fourth = static_cast<unsigned int>(base64_value(
            static_cast<unsigned char>(encoded_string[offset + 3])));
        decoded.push_back(static_cast<char>((first << 2) | (second >> 4)));
        decoded.push_back(static_cast<char>((second << 4) | (third >> 2)));
        decoded.push_back(static_cast<char>((third << 6) | fourth));
        offset += 4;
    }

    if (remainder >= 2)
    {
        const unsigned int first = static_cast<unsigned int>(base64_value(
            static_cast<unsigned char>(encoded_string[offset])));
        const unsigned int second = static_cast<unsigned int>(base64_value(
            static_cast<unsigned char>(encoded_string[offset + 1])));
        decoded.push_back(static_cast<char>((first << 2) | (second >> 4)));

        if (remainder == 3)
        {
            const unsigned int third = static_cast<unsigned int>(base64_value(
                static_cast<unsigned char>(encoded_string[offset + 2])));
            decoded.push_back(
                static_cast<char>((second << 4) | (third >> 2)));
        }
    }

    *output = std::move(decoded);
    return true;
}

} // namespace wfrest
