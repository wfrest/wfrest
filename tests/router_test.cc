//
// Created by Chanchan on 11/1/21.
//

#include "Router.h"

using namespace wfrest;

int main()
{
    Router router;
    router.handle("/hello", [](const HttpReq* req, HttpResp* resp){
        printf("world");
    }, Router::GET);

    router.handle("/ping", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, Router::GET);

    router.handle("/api/v1", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, Router::GET);

    router.handle("/api/v1/v2/", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, Router::GET);

    router.handle("/api/v1/v3/v4", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, Router::GET);

    router.print_routes();


}