#include "StrUtil.h"

using namespace wfrest;

void test01()
{
    fprintf(stderr, "test 01\n");
    StringPiece str("host=admin&password=123&time=10000101");

    std::vector<std::string> str_list = StrUtil::split_piece<std::string>(str, '&');
    for(auto &s : str_list)
    {
        fprintf(stderr, "%s\n", s.c_str());
    }
}

void test02()
{
    fprintf(stderr, "test 02\n");
    StringPiece str(R"(Content-Disposition: form-data; name="avatar"; filename="user.jpg")");
    // StringPiece feature : only a watcher, still find the last '\0'
    std::vector<StringPiece> list = StrUtil::split_piece<StringPiece>(str, ';');

    fprintf(stderr, "string piece feature : a watcher\n");
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
}

int main()
{
    test01();
    test02();
}