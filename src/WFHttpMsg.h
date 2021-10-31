#ifndef _WFHTTPMSG_H_
#define _WFHTTPMSG_H_

#include "workflow/HttpMessage.h"
#include "workflow/WFTaskFactory.h"
#include <fcntl.h>

namespace wfrest 
{

class HttpReq : public protocol::HttpRequest
{
public:
    void Body(const char** body, size_t *size) const;
    
    void test() { fprintf(stderr, "req test : %s\n", get_request_uri()); }

    // for regex ? 
};

class HttpResp : public protocol::HttpResponse
{
public:
    void String(const std::string& str);

    void Data(const void* data, size_t len, bool nocopy = true);

    void File(const std::string& path);

    // void Write(const std::string& content, const std::string& path);

    void set_status(int status_code);

    void test() { fprintf(stderr, "resp test : %s\n", get_status_code()); }
};   

using WFWebTask = WFNetworkTask<HttpReq, HttpResp>;

} // namespace wfrest


#endif // _WFHTTPMSG_H_