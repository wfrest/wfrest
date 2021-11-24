#include <string>
#include <cstring>
#include <iostream>

std::string trim_pairs(const std::string& str, const char* pairs) {
    const char* s = str.c_str();
    const char* e = str.c_str() + str.size() - 1;
    const char* p = pairs;
    bool is_pair = false;
    while (*p != '\0' && *(p+1) != '\0') {
        if (*s == *p && *e == *(p+1)) {
            is_pair = true;
            break;
        }
        p += 2;
    }
    return is_pair ? str.substr(1, str.size()-2) : str;
}

const static std::string content_type_str = R"(Content-Type:multipart/form-data; boundary=ZnGpDtePMx0KrHh_G0X99Yef9r8JZsRJSXC)";

void test01()
{
    const char* boundary = strstr(content_type_str.c_str(), "boundary=");
    if (boundary == nullptr)
    {
        return;
    }
    boundary += strlen("boundary=");
    std::cout << boundary << std::endl;

    std::string boundary_str(boundary);
    boundary_str = trim_pairs(boundary_str, R"(""'')");
    std::cout << boundary_str << std::endl;
}

int main()
{
    test01();
}