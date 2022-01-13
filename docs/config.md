## Custom Server Configuration

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;
    
    svr.GET("/config", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("config");
    });

    if (svr.max_connections(4000)
            .peer_response_timeout(20 * 1000)
            .keep_alive_timeout(30 * 1000)
            .track()
            .start(8888) == 0)
    {
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

## Configuration items

Configuration items are as follows:

```cpp
struct WFServerParams
{
	size_t max_connections;
	int peer_response_timeout;	/* timeout of each read or write operation */
	int receive_timeout;	/* timeout of receiving the whole message */
	int keep_alive_timeout;
	size_t request_size_limit;
	int ssl_accept_timeout;	/* if not ssl, this will be ignored */
};
```

Default values are:

```cpp
static constexpr struct WFServerParams SERVER_PARAMS_DEFAULT =
{
	.max_connections		=	2000,
	.peer_response_timeout	=	10 * 1000,
	.receive_timeout		=	-1,
	.keep_alive_timeout		=	60 * 1000,
	.request_size_limit		=	(size_t)-1,
	.ssl_accept_timeout		=	10 * 1000,
};
```

## Track

You can turn on the track logger.

Format :

```
[WFREST] 2022-01-13 18:00:04 | 200 | 127.0.0.1 | GET | "/data" | -- 
[WFREST] 2022-01-13 18:00:08 | 200 | 127.0.0.1 | GET | "/hello" | -- 
[WFREST] 2022-01-13 18:00:17 | 404 | 127.0.0.1 | GET | "/hello1" | -- 
```

And you can set your own track logger:

```cpp
svr.track([](HttpTask *server_task){
    spdlog::info(...);
    // BOOST_LOG_TRIVIAL(info) << "Status : ";
    // LOG(ERROR) << "time : " << time;
  ...
});
```
