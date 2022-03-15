#include "UriUtil.h"
#include "StrUtil.h"

using namespace wfrest;

std::map<std::string, std::string> UriUtil::split_query(const StringPiece &query)
{
    std::map<std::string, std::string> res;

    if (query.empty())
        return res;

    std::vector<StringPiece> arr = StrUtil::split_piece<StringPiece>(query, '&');

    if (arr.empty())
        return res;

    for (const auto &ele: arr)
    {
        if (ele.empty())
            continue;

        std::vector<std::string> kv = StrUtil::split_piece<std::string>(ele, '=');
        size_t kv_size = kv.size();
        std::string &key = kv[0];

        if (key.empty() || res.count(key) > 0)
            continue;

        if (kv_size == 1)
        {
            res.emplace(std::move(key), "");
            continue;
        }

        std::string &val = kv[1];

        if (val.empty())
            res.emplace(std::move(key), "");
        else
            res.emplace(std::move(key), std::move(val));
    }

    return res;
}
