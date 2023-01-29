#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include <fstream>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"
#include "wfrest/FileUtil.h"
#include "wfrest/PathUtil.h"
#include "wfrest/Json.h"
#include "../FileTestUtil.h"

using namespace wfrest;
using namespace protocol;

class FileTest
{
public:
    static void create_file(const std::string &path)
    {
        std::string file_body = generate_file_content();
        bool write_ok = FileTestUtil::write_file(path, file_body);
        EXPECT_TRUE(write_ok);
        EXPECT_TRUE(FileUtil::file_exists(path));
    }

    static void delete_file(const std::string &path)
    {
        std::remove(path.c_str());
        EXPECT_FALSE(FileUtil::file_exists(path));
    }

    static void create_path(const std::string &path)
    {
        FileTestUtil::mkpath(path.c_str(), 0777);
        EXPECT_TRUE(PathUtil::is_dir(path));
    }

    static void delete_dir(const std::string &path)
    {
        FileTestUtil::recursive_delete(path.c_str());
        EXPECT_FALSE(PathUtil::is_dir(path));
    }

    static void process(const std::string &path,
                        size_t start,
                        size_t end,
                        const std::function<void(WFHttpTask *task)> &callback = nullptr)
    {
        HttpServer svr;
        WFFacilities::WaitGroup wait_group(1);

        svr.GET("/file", [&path, start, end](const HttpReq *req, HttpResp *resp)
        {
            resp->File(path, start, end);
        });

        EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

        WFHttpTask *client_task = create_http_task("file");

        if(callback)
        {
            client_task->set_callback([&wait_group, &callback](WFHttpTask *task){
                callback(task);
                wait_group.done();
            });
        } else
        {
            client_task->set_callback([&wait_group, start, end](WFHttpTask *task)
            {
                const void *body;
                size_t body_len;
                HttpResponse *resp = task->get_resp();
                resp->get_parsed_body(&body, &body_len);
                HttpHeaderMap header(resp);
                std::string content_type = header.get("Content-Type");
                EXPECT_EQ(content_type, "text/plain");
                std::string content_range = header.get("Content-Range");
                std::string file_body = generate_file_content();

                int exp_start = start;
                int exp_end = end;
                if(exp_end == -1) exp_end = file_body.size();
                if(exp_start < 0) exp_start = file_body.size() + start;

                EXPECT_EQ(content_range, "bytes " + std::to_string(exp_start)
                                        + "-" + std::to_string(exp_end)
                                        + "/" + std::to_string(exp_end - exp_start));

                std::string file_body_range = file_body.substr(exp_start, exp_end - exp_start);
                EXPECT_EQ(file_body_range, std::string(static_cast<const char *>(body), body_len));

                wait_group.done();
            });
        }
        client_task->start();
        wait_group.wait();
        svr.stop();
    }
    static WFHttpTask *create_http_task(const std::string &path)
    {
        return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
    }

    static std::string generate_file_content()
    {
        std::string str;
        for (size_t i = 0; i < 100; i++)
        {
            str.append(std::to_string(i % 5));
        }
        return str;
    }
};

TEST(HttpServer, file_range1)
{
    std::string path = "test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 10, 20);
    FileTest::delete_file(path);
}

TEST(HttpServer, file_range2)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 0, 20);
    FileTest::delete_file(path);
}

TEST(HttpServer, file_range3)
{
    std::string path = "test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 10, -1);
    FileTest::delete_file(path);
}

TEST(HttpServer, file_range4)
{
    std::string root_dir = "./test_dir";
    std::string dir_path = root_dir + "/tmp/a";
    FileTest::create_path(dir_path);
    std::string file_path = dir_path + "/test.txt";
    FileTest::create_file(file_path);
    FileTest::process(file_path, -10, -1);
    FileTest::delete_dir(root_dir);
}

TEST(HttpServer, file_range5)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, -10, 95);
    FileTest::delete_file(path);
}

TEST(HttpServer, file_no_extension)
{
    std::string path = "./test_file";
    FileTest::create_file(path);
    FileTest::process(path, -10, 95, [](WFHttpTask *task) {
        HttpResponse *resp = task->get_resp();
        HttpHeaderMap header(resp);
        std::string content_type = header.get("Content-Type");
        EXPECT_EQ(content_type, "application/octet-stream");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, file_png)
{
    std::string path = "./test.png";
    FileTest::create_file(path);
    FileTest::process(path, -10, 95, [](WFHttpTask *task) {
        HttpResponse *resp = task->get_resp();
        HttpHeaderMap header(resp);
        std::string content_type = header.get("Content-Type");
        EXPECT_EQ(content_type, "image/png");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, file_range_invalid)
{
    std::string path = "./test.png";
    FileTest::create_file(path);
    FileTest::process(path, -5, 90, [](WFHttpTask *task) {
        const void *body;
        size_t body_len;
        HttpResponse *resp = task->get_resp();
        resp->get_parsed_body(&body, &body_len);
        HttpHeaderMap header(resp);
        std::string content_type = header.get("Content-Type");
        EXPECT_EQ(content_type, "application/json");
        std::string body_str(static_cast<const char *>(body), body_len);
        Json js = Json::parse(body_str);
        EXPECT_EQ(js["errmsg"].get<std::string>(), "File Range Invalid");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, not_file)
{
    std::string root_dir = "./test_dir";
    std::string dir_path = root_dir + "/tmp/a";
    FileTest::create_path(dir_path);
    FileTest::process(dir_path, -5, 90, [](WFHttpTask *task) {
        const void *body;
        size_t body_len;
        HttpResponse *resp = task->get_resp();
        resp->get_parsed_body(&body, &body_len);
        HttpHeaderMap header(resp);
        std::string content_type = header.get("Content-Type");
        EXPECT_EQ(content_type, "application/json");
        std::string body_str(static_cast<const char *>(body), body_len);
        Json js = Json::parse(body_str);
        EXPECT_EQ(js["errmsg"].get<std::string>(), "404 Not Found");
        EXPECT_TRUE(strcmp(resp->get_status_code(), "404") == 0);
    });
    FileTest::delete_dir(root_dir);
}

TEST(HttpServer, save_file)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    std::string path = "test.txt";
    EXPECT_FALSE(FileUtil::file_exists(path));
    std::string file_body = FileTest::generate_file_content();
    svr.GET("/file", [&path, &file_body](const HttpReq *req, HttpResp *resp, SeriesWork *series)
    {
        resp->Save(path, file_body);
        series->set_callback([&path, &file_body](const SeriesWork *sereis) {
            EXPECT_TRUE(FileUtil::file_exists(path));
            std::ifstream file(path);
            std::string str;
            std::string file_contents;
            file_contents.reserve(1024);
            while (std::getline(file, str))
            {
                file_contents.append(std::move(str));
            }
            EXPECT_EQ(file_body, file_contents);
            FileTest::delete_file(path);
            EXPECT_FALSE(FileUtil::file_exists(path));
        });
    });

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = FileTest::create_http_task("file");

    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}
