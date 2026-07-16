#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/Json.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFFacilities.h"
#include "../ClientUtil.h"

using namespace wfrest;
using namespace protocol;

TEST(HttpServer, cookie_headers_and_safe_output)
{
    HttpServer server;
    WFFacilities::WaitGroup wait_group(1);

    server.GET("/cookies", [](const HttpReq *req, HttpResp *resp)
    {
        const auto &cookies = req->cookies();
        EXPECT_EQ(&cookies, &req->cookies());

        Json result;
        result["size"] = cookies.size();
        result["user"] = req->cookie("user");
        result["role"] = req->cookie("role");
        result["token"] = req->cookie("token");
        result["duplicate"] = req->cookie("duplicate");
        result["flag"] = req->cookie("flag");

        HttpCookie deletion("session", "");
        deletion.set_max_age(0).set_path("/").set_http_only(true);
        resp->add_cookie(std::move(deletion));

        HttpCookie invalid("unsafe", "line\r\nInjected: yes");
        resp->add_cookie(std::move(invalid));
        resp->Json(result);
    });

    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *client = ClientUtil::create_http_task("cookies");
    client->get_req()->add_header_pair(
        "Cookie", "user=alice; token=a=b=c; duplicate=first");
    client->get_req()->add_header_pair(
        "Cookie", "role=admin; duplicate=second; flag=");
    client->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        const void *body = nullptr;
        size_t body_size = 0;
        const bool parsed_body = task->get_resp()->get_parsed_body(
            &body, &body_size);
        EXPECT_TRUE(parsed_body);
        if (!parsed_body)
        {
            wait_group.done();
            return;
        }
        Json result = Json::parse(std::string(
            static_cast<const char *>(body), body_size));
        EXPECT_TRUE(result.is_valid());
        if (!result.is_valid())
        {
            wait_group.done();
            return;
        }
        EXPECT_EQ(result["size"].get<int>(), 5);
        EXPECT_EQ(result["user"].get<std::string>(), "alice");
        EXPECT_EQ(result["role"].get<std::string>(), "admin");
        EXPECT_EQ(result["token"].get<std::string>(), "a=b=c");
        EXPECT_EQ(result["duplicate"].get<std::string>(), "first");
        EXPECT_EQ(result["flag"].get<std::string>(), "");

        HttpHeaderMap headers(task->get_resp());
        const auto set_cookies = headers.get_strict("Set-Cookie");
        EXPECT_EQ(set_cookies.size(), 1U);
        if (set_cookies.size() == 1)
        {
            EXPECT_EQ(set_cookies[0],
                      "session=; Max-Age=0; Path=/; HttpOnly");
        }
        wait_group.done();
    });
    client->start();

    wait_group.wait();
    server.stop();
}
