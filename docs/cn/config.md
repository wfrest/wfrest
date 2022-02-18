## 配置

串式操作设置:

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

## 配置项

```cpp
struct WFServerParams
{
	size_t max_connections;
	int peer_response_timeout;	/* 每一个读写操作的超时时间 */ 
	int receive_timeout;	/* 接受整个消息的超时时间 */
	int keep_alive_timeout;
	size_t request_size_limit;
	int ssl_accept_timeout;	/* 如果不是ssl，这个将会被忽略掉 */
};
```

默认值为:

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

## 日志跟踪的接口

你可以在配置中打开track，`svr.track()`

默认格式为 :

```
[WFREST] 2022-01-13 18:00:04 | 200 | 127.0.0.1 | GET | "/data" | -- 
[WFREST] 2022-01-13 18:00:08 | 200 | 127.0.0.1 | GET | "/hello" | -- 
[WFREST] 2022-01-13 18:00:17 | 404 | 127.0.0.1 | GET | "/hello1" | -- 
```

你也可以射中你自己的追踪日志，可以定义你自己的格式，用自己选择的日志:

```cpp
svr.track([](HttpTask *server_task){
    spdlog::info(...);
    // BOOST_LOG_TRIVIAL(info) << "Status : ";
    // LOG(ERROR) << "time : " << time;
  ...
});
```
