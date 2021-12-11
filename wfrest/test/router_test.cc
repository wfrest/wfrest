#include "wfrest/Router.h"
#include "wfrest/VerbHandler.h"

using namespace wfrest;

int main()
{
    Router router;
    router.handle("/hello", 0, [](const HttpReq* , HttpResp* , SeriesWork *) -> WFGoTask * {
        printf("world");
        return nullptr;
    }, Verb::GET);

    // router.handle("/ping", 0, [](const HttpReq* req, HttpResp* resp){
    //     printf("pong");
    // }, nullptr, Verb::GET);

    // router.handle("/api/v1", 0, [](const HttpReq* req, HttpResp* resp){
    //     printf("pong");
    // }, nullptr, Verb::GET);

    // router.handle("/api/v1/v2/", 0, [](const HttpReq* req, HttpResp* resp){
    //     printf("pong");
    // }, nullptr, Verb::GET);

    // router.handle("/api/v1/v3/v4", 0, [](const HttpReq* req, HttpResp* resp){
    //     printf("pong");
    // }, nullptr, Verb::GET);

    // router.handle("/api/{name}", 0, [](const HttpReq* req, HttpResp* resp){
    //     printf("pong");
    // }, nullptr, Verb::GET);

    // router.handle("/api/action*", 0, [](const HttpReq* req, HttpResp* resp){
    //     printf("pong");
    // }, nullptr, Verb::GET);

    router.print_routes();
}