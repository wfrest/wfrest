#include "wfrest/StrUtil.h"

using namespace wfrest;

int main()
{
    StringPiece str(R"(Content-Disposition: form-data; name="avatar"; filename="user.jpg")");
    // StringPiece feature : only a watcher, still find the last '\0'
    std::vector<StringPiece> list = StrUtil::split_piece<StringPiece>(str, ';');
    for(auto &l : list)
    {
        fprintf(stderr, "dispo : %s\n", l.data());
    }
    fprintf(stderr, "\nas_string()\n");

    // please watch this to compare with the above
    for(auto &l : list)
    {
        fprintf(stderr, "dispo : %s\n", l.as_string().c_str());
    }

    for(auto &l : list)
    {
        auto kv = StrUtil::split_piece<StringPiece>(StrUtil::trim(l), '=');
        for(auto k : kv)
        {
            fprintf(stderr, "k : %s\n", k.as_string().c_str());
        }
    }
    return 0;
}