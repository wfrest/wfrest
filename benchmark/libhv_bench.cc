#include <HttpServer.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s port [thread num]\n", argv[0]);
        return -1;
    }
    unsigned short port = atoi(argv[1]);

    HttpService router;
    
    router.GET("/", [](HttpRequest* req, HttpResponse* resp) {
        return resp->String("");
    });

    router.GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
        return resp->String("pong");
    });

    router.POST("/echo", [](const HttpContextPtr& ctx) {
        return ctx->send(ctx->body(), ctx->type());
    });

    http_server_t server;
    server.port = atoi(argv[1]);
    server.service = &router;
    server.worker_threads = atoi(argv[2])
    http_server_run(&server);
    return 0;
}
