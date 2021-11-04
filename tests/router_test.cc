//
// Created by Chanchan on 11/1/21.
//

#include "Router.h"
#include "VerbHandler.h"

using namespace wfrest;

int main()
{
    Router router;
    router.handle("/hello", [](const HttpReq* req, HttpResp* resp){
        printf("world");
    }, GET);

    router.handle("/ping", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    },GET);

    router.handle("/api/v1", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, GET);

    router.handle("/api/v1/v2/", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, GET);

    router.handle("/api/v1/v3/v4", [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, GET);

    router.print_routes();


}