#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"
#include "wfrest/PathUtil.h"
#include "wfrest/FileUtil.h"
#include "wfrest/HttpContent.h"
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

    svr.GET("/form", [](const HttpReq *req, HttpResp *resp)
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

    svr.GET("/form", [](const HttpReq *req, HttpResp *resp)
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

    svr.GET("/form", [](const HttpReq *req, HttpResp *resp)
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
