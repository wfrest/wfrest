#include "wfrest/json.hpp"
#include <string>
#include <map>
#include <iostream>
using Json = nlohmann::json;

struct Body
{
    std::string content;
    std::map<std::string, std::string> form_kv;
    std::map<std::string, std::pair<std::string, std::string>> form;
    Json json;
};

int main()
{
    Body body;
    std::cout << sizeof body << std::endl;
    return 0;
}
