#include "workflow/WFFacilities.h"
#include "workflow/HttpUtil.h"
#include <unistd.h>
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"
#include "wfrest/Json.h"
#include "wfrest/PathUtil.h"
#include "wfrest/FileUtil.h"
#include "wfrest/HttpContent.h"
#include "wfrest/StringPiece.h"
#include "../ClientUtil.h"
#include "../FileTestUtil.h"

using namespace wfrest;
using namespace protocol;

class MultiPartEncoderTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // create dir
        dir_path_ = "./www";
        FileTestUtil::create_dir(dir_path_.c_str(), 0777);
        EXPECT_TRUE(PathUtil::is_dir(dir_path_));
        // create files
        for(size_t i = 0; i < 2; i++) {
            std::string file_path = dir_path_ + "/test_" + std::to_string(i+1) + ".txt";
            std::string content;
            size_t n  = i * 10;
            for (size_t j = 0; j < n; j++)
            {
                content.append(std::to_string(j));
            }
            bool write_ok = FileTestUtil::write_file(file_path, "file_body" + content);
            EXPECT_TRUE(write_ok);
            EXPECT_TRUE(FileUtil::file_exists(file_path));
            file_list_.push_back(file_path);
        }
    }

    void TearDown() override
    {
        FileTestUtil::recursive_delete(dir_path_.c_str());
        for(size_t i = 0; i < file_list_.size(); i++) {
            EXPECT_FALSE(FileUtil::file_exists(file_list_[i]));
        }
    }
protected:
    std::string dir_path_;
    std::vector<std::string> file_list_;
};

TEST_F(MultiPartEncoderTest, multi_part_form_params)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/form", [](const HttpReq *, HttpResp *resp)
    {
        MultiPartEncoder encoder;
        encoder.add_param("Filename1", "1.jpg");
        encoder.add_param("Filename2", "2.jpg");
        resp->String(std::move(encoder));
    });

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = ClientUtil::create_http_task("form");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        static const std::string boudary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        std::string content;
        content.append("--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("Filename1");
        content.append("\"\r\n\r\n");
        content.append("1.jpg");

        content.append("\r\n--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("Filename2");
        content.append("\"\r\n\r\n");
        content.append("2.jpg");
        // end
        content.append("\r\n--");
        content.append(boudary);
        content.append("--\r\n");
        // fprintf(stderr, "content : %s\n", content.c_str());

        // fprintf(stderr, "body : %s\n", static_cast<const char *>(body));
        EXPECT_TRUE(strcmp(content.c_str(), static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}

TEST_F(MultiPartEncoderTest, multi_part_form_files)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/form", [](const HttpReq *, HttpResp *resp)
    {
        MultiPartEncoder encoder;
        encoder.add_file("test_1.txt", "./www/test_1.txt");
        encoder.add_file("test_2.txt", "./www/test_2.txt");
        resp->String(std::move(encoder));
    });

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = ClientUtil::create_http_task("form");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        static const std::string boudary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        std::string content;
        content.append("--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("test_1.txt");
        content.append("\"; filename=\"");
        content.append("test_1.txt");
        content.append("\"\r\nContent-Type: ");
        content.append("text/plain");
        content.append("\r\n\r\n");
        content.append("file_body");

        content.append("\r\n--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("test_2.txt");
        content.append("\"; filename=\"");
        content.append("test_2.txt");
        content.append("\"\r\nContent-Type: ");
        content.append("text/plain");
        content.append("\r\n\r\n");
        content.append("file_body0123456789");
        content.append("\r\n--");
        content.append(boudary);
        content.append("--\r\n");
        EXPECT_TRUE(strcmp(content.c_str(), static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}


TEST_F(MultiPartEncoderTest, multi_part_form_param_file)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/form", [](const HttpReq *, HttpResp *resp)
    {
        MultiPartEncoder encoder;
        encoder.add_param("Filename", "1.jpg");
        encoder.add_file("test_1.txt", "./www/test_1.txt");
        encoder.add_file("test_2.txt", "./www/test_2.txt");
        resp->String(std::move(encoder));
    });

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = ClientUtil::create_http_task("form");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        static const std::string boudary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        std::string content;
        content.append("--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("Filename");
        content.append("\"\r\n\r\n");
        content.append("1.jpg");
        content.append("\r\n--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("test_1.txt");
        content.append("\"; filename=\"");
        content.append("test_1.txt");
        content.append("\"\r\nContent-Type: ");
        content.append("text/plain");
        content.append("\r\n\r\n");
        content.append("file_body");

        content.append("\r\n--");
        content.append(boudary);
        content.append("\r\nContent-Disposition: form-data; name=\"");
        content.append("test_2.txt");
        content.append("\"; filename=\"");
        content.append("test_2.txt");
        content.append("\"\r\nContent-Type: ");
        content.append("text/plain");
        content.append("\r\n\r\n");
        content.append("file_body0123456789");
        content.append("\r\n--");
        content.append(boudary);
        content.append("--\r\n");
        // fprintf(stderr, "content : %s\n", content.c_str());

        // fprintf(stderr, "body : %s\n", static_cast<const char *>(body));
        EXPECT_TRUE(strcmp(content.c_str(), static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}

TEST_F(MultiPartEncoderTest, validates_boundary_and_quoted_metadata)
{
    HttpServer server;
    WFFacilities::WaitGroup wait_group(2);

    server.GET("/empty", [](const HttpReq *, HttpResp *resp)
    {
        MultiPartEncoder encoder;
        encoder.add_file("missing", "./www/missing-empty");
        resp->String(std::move(encoder));
    });
    server.GET("/metadata", [](const HttpReq *, HttpResp *resp)
    {
        MultiPartEncoder encoder;
        encoder.set_boundary("a:b?c");
        encoder.add_param("quo\"te\\field", "value");
        encoder.add_param("bad\r\nInjected", "ignored");
        resp->String(std::move(encoder));
    });

    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *empty_task = ClientUtil::create_http_task("empty");
    empty_task->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Content-Type"),
                  "multipart/form-data; boundary=\"" +
                      MultiPartForm::k_default_boundary + "\"");

        const void *body = nullptr;
        size_t body_len = 0;
        EXPECT_TRUE(task->get_resp()->get_parsed_body(&body, &body_len));
        EXPECT_EQ(std::string(static_cast<const char *>(body), body_len),
                  "--" + MultiPartForm::k_default_boundary + "--\r\n");
        wait_group.done();
    });

    WFHttpTask *metadata_task = ClientUtil::create_http_task("metadata");
    metadata_task->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Content-Type"),
                  "multipart/form-data; boundary=\"a:b?c\"");

        const void *body = nullptr;
        size_t body_len = 0;
        EXPECT_TRUE(task->get_resp()->get_parsed_body(&body, &body_len));
        const std::string content(static_cast<const char *>(body), body_len);
        EXPECT_EQ(content.find("bad\r\nInjected"), std::string::npos);

        MultiPartForm parser;
        parser.set_boundary("a:b?c");
        const Form form = parser.parse_multipart(StringPiece(content));
        EXPECT_EQ(form.size(), 1U);
        if (form.count("quo\"te\\field") != 0)
        {
            EXPECT_EQ(form.at("quo\"te\\field").second, "value");
        }
        wait_group.done();
    });

    SeriesWork *series = Workflow::create_series_work(empty_task, nullptr);
    series->push_back(metadata_task);
    series->start();
    wait_group.wait();
    server.stop();
}

TEST_F(MultiPartEncoderTest, owns_file_tasks_and_preserves_series_state)
{
    const std::string special_path = dir_path_ + "/a\"b\\c.txt";
    const std::string empty_path = dir_path_ + "/empty.txt";
    const std::string invalid_metadata_path =
        dir_path_ + "/bad\r\nfilename.txt";
    ASSERT_TRUE(FileTestUtil::write_file(special_path, "special-body"));
    ASSERT_TRUE(FileTestUtil::write_file(empty_path, ""));
    ASSERT_TRUE(FileTestUtil::write_file(invalid_metadata_path, "ignored"));

    HttpServer server;
    WFFacilities::WaitGroup wait_group(2);
    int series_context = 42;

    server.GET("/files", [&](const HttpReq *, HttpResp *resp,
                              SeriesWork *series)
    {
        series->set_context(&series_context);
        series->set_callback([&wait_group, &series_context](
            const SeriesWork *completed)
        {
            EXPECT_EQ(completed->get_context(), &series_context);
            wait_group.done();
        });

        MultiPartEncoder encoder;
        encoder.add_file("missing-before", dir_path_ + "/missing-before");
        encoder.add_file("up\"load\\field", special_path);
        encoder.add_file("empty", empty_path);
        encoder.add_file("bad\r\nfield", special_path);
        encoder.add_file("bad-filename", invalid_metadata_path);
        encoder.add_file("missing-after", dir_path_ + "/missing-after");
        resp->String(std::move(encoder));
    });

    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *client_task = ClientUtil::create_http_task("files");
    client_task->set_callback([&wait_group, &special_path](WFHttpTask *task)
    {
        EXPECT_STREQ(task->get_resp()->get_status_code(), "200");
        const void *body = nullptr;
        size_t body_len = 0;
        EXPECT_TRUE(task->get_resp()->get_parsed_body(&body, &body_len));
        const std::string content(static_cast<const char *>(body), body_len);

        MultiPartForm parser;
        parser.set_boundary(MultiPartForm::k_default_boundary);
        const Form form = parser.parse_multipart(StringPiece(content));
        EXPECT_EQ(form.size(), 2U);
        if (form.count("up\"load\\field") != 0)
        {
            EXPECT_EQ(form.at("up\"load\\field").first,
                      PathUtil::base(special_path));
            EXPECT_EQ(form.at("up\"load\\field").second, "special-body");
        }
        if (form.count("empty") != 0)
        {
            EXPECT_EQ(form.at("empty").first, "empty.txt");
            EXPECT_TRUE(form.at("empty").second.empty());
        }
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    server.stop();
}

TEST_F(MultiPartEncoderTest, rejects_short_file_reads)
{
    HttpServer server;
    WFFacilities::WaitGroup wait_group(1);
    const std::string path = file_list_.back();

    server.GET("/short-read", [&path](const HttpReq *, HttpResp *resp)
    {
        MultiPartEncoder encoder;
        encoder.add_file("upload", path);
        resp->String(std::move(encoder));

        EXPECT_EQ(truncate(path.c_str(), 1), 0);
    });

    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *client_task = ClientUtil::create_http_task("short-read");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        EXPECT_STREQ(task->get_resp()->get_status_code(), "503");
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Content-Type"), "application/json");

        const void *body = nullptr;
        size_t body_len = 0;
        EXPECT_TRUE(task->get_resp()->get_parsed_body(&body, &body_len));
        const Json error = Json::parse(
            std::string(static_cast<const char *>(body), body_len));
        EXPECT_TRUE(error.is_valid());
        if (error.is_valid())
        {
            EXPECT_EQ(error["errmsg"].get<std::string>(), "File Read Error");
        }
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    server.stop();
}
