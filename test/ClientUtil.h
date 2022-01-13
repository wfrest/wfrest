#include "workflow/WFFacilities.h"

class ClientUtil
{
public:
    static WFHttpTask *create_http_task(const std::string &path)
    {
        return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
    }
};



