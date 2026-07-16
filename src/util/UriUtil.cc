#include "UriUtil.h"

#include <utility>

#include "CodeUtil.h"
#include "StringPiece.h"

using namespace wfrest;

std::map<std::string, std::string> UriUtil::split_query(const StringPiece &query)
{
    std::map<std::string, std::string> result;
    if (query.empty())
        return result;

    const char *field_begin = query.begin();
    while (field_begin < query.end())
    {
        const char *field_end = field_begin;
        while (field_end < query.end() && *field_end != '&')
            ++field_end;

        if (field_begin != field_end)
        {
            const char *separator = field_begin;
            while (separator < field_end && *separator != '=')
                ++separator;

            StringPiece raw_key(field_begin, separator - field_begin);
            const char *value_begin = separator < field_end
                                        ? separator + 1
                                        : field_end;
            StringPiece raw_value(value_begin, field_end - value_begin);

            std::string key = CodeUtil::url_decode(raw_key.as_string());
            if (!key.empty())
            {
                std::string value = CodeUtil::url_decode(raw_value.as_string());
                result.emplace(std::move(key), std::move(value));
            }
        }

        field_begin = field_end < query.end() ? field_end + 1 : query.end();
    }

    return result;
}
