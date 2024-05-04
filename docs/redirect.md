## Redirect

We can implement navigation between pages using Redirect, which is achieved through the Location Header. Related methods:

```cpp
void Redirect(const std::string& location, int status_code);
```

Redirect is used to guide the client to navigate to a specified address, which can be either a relative path of a local service or a complete HTTP address. Usage example:

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    // curl -L 127.0.0.1:8888/redirect
    svr.GET("/redirect", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redirect("/test", HttpStatusMovedPermanently);
    });

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("{hello : world}");
    });

    if (svr.track().start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

After running, we can access http://127.0.0.1:8888/redirect through the browser and then find that the browser immediately redirects to the http://127.0.0.1:8888/test page.

