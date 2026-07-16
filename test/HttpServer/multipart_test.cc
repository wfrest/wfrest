#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "wfrest/HttpServer.h"
#include "wfrest/Json.h"
#include "workflow/WFFacilities.h"
#include "../ClientUtil.h"

using namespace wfrest;
using namespace protocol;

namespace
{

std::string multipart_body(const std::string &boundary,
                           const std::string &closing = "--\r\n")
{
    return "--" + boundary +
           "\r\nContent-Disposition: form-data; name=\"field\"\r\n"
           "\r\nvalue\r\n--" + boundary + closing;
}

std::string multipart_disposition_body(const std::string &boundary,
                                       const std::string &disposition)
{
    return "--" + boundary +
           "\r\nContent-Disposition: " + disposition +
           "\r\n\r\nvalue\r\n--" + boundary + "--\r\n";
}

WFHttpTask *multipart_request(const std::string &content_type,
                              const std::string &body,
                              size_t expected_size,
                              WFFacilities::WaitGroup *wait_group,
                              const std::string &expected_filename = "")
{
    WFHttpTask *task = ClientUtil::create_http_task("multipart");
    task->get_req()->set_method("POST");
    task->get_req()->add_header_pair("Content-Type", content_type);
    task->get_req()->append_output_body(body.data(), body.size());
    task->set_callback([expected_size, expected_filename, wait_group](
        WFHttpTask *current)
    {
        EXPECT_EQ(current->get_state(), WFT_STATE_SUCCESS);
        const void *body_data = nullptr;
        size_t body_size = 0;
        const bool has_body = current->get_resp()->get_parsed_body(
            &body_data, &body_size);
        EXPECT_TRUE(has_body);
        if (!has_body)
        {
            wait_group->done();
            return;
        }

        Json result = Json::parse(std::string(
            static_cast<const char *>(body_data), body_size));
        EXPECT_TRUE(result.is_valid());
        if (!result.is_valid())
        {
            wait_group->done();
            return;
        }
        EXPECT_EQ(result["kind"].get<int>(),
                  static_cast<int>(MULTIPART_FORM_DATA));
        EXPECT_EQ(result["size"].get<int>(), static_cast<int>(expected_size));
        if (expected_size == 1)
        {
            EXPECT_EQ(result["value"].get<std::string>(), "value");
            EXPECT_EQ(result["filename"].get<std::string>(),
                      expected_filename);
        }
        wait_group->done();
    });
    return task;
}

} // namespace

TEST(HttpServer, multipart_content_type_and_completion)
{
    HttpServer server;
    WFFacilities::WaitGroup wait_group(11);

    server.POST("/multipart", [](const HttpReq *req, HttpResp *resp)
    {
        const Form &form = req->form();
        Json result;
        result["kind"] = static_cast<int>(req->content_type());
        result["size"] = form.size();
        if (form.count("field") != 0)
        {
            result["value"] = form.at("field").second;
            result["filename"] = form.at("field").first;
        }
        resp->Json(result);
    });

    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    std::vector<WFHttpTask *> tasks;
    const std::string valid_body = multipart_body("abc");
    tasks.push_back(multipart_request(
        "Multipart/Form-Data; charset=utf-8; Boundary = \"abc\"; version=1",
        valid_body, 1, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=\"a:b?c\"; charset=utf-8",
        multipart_body("a:b?c"), 1, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=abc; boundary=abc",
        valid_body, 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; charset=utf-8",
        valid_body, 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=",
        valid_body, 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=\"unterminated",
        valid_body, 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=\"abc\"junk",
        valid_body, 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=abc@",
        multipart_body("abc@"), 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=" + std::string(71, 'a'),
        multipart_body(std::string(71, 'a')), 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=abc",
        multipart_body("abc", "X\r\n"), 0, &wait_group));
    tasks.push_back(multipart_request(
        "multipart/form-data; boundary=disposition",
        multipart_disposition_body(
            "disposition",
            "FORM-DATA; NAME=field; FILENAME=\"a=b;c.txt\""),
        1, &wait_group, "a=b;c.txt"));

    SeriesWork *series = Workflow::create_series_work(tasks.front(), nullptr);
    for (size_t i = 1; i < tasks.size(); ++i)
        series->push_back(tasks[i]);
    series->start();

    wait_group.wait();
    server.stop();
}
