#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <type_traits>
#include <unistd.h>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"
#include "wfrest/FileCache.h"
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
        create_file(path, generate_file_content());
    }

    static void create_file(const std::string &path, const std::string &content)
    {
        bool write_ok = FileTestUtil::write_file(path, content);
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
                verify_response(task, generate_file_content(), start, end);
                wait_group.done();
            });
        }
        client_task->start();
        wait_group.wait();
        svr.stop();
    }

    static void process_cached_twice(const std::string &path,
                                     size_t start,
                                     size_t end)
    {
        FileCache& cache = FileCache::instance();
        cache.enable();
        cache.clear();
        cache.set_max_size(100 * 1024 * 1024);

        HttpServer svr;
        svr.GET("/file", [&path, start, end](const HttpReq *, HttpResp *resp)
        {
            resp->CachedFile(path, start, end);
        });
        ASSERT_EQ(svr.start("127.0.0.1", 8888), 0);

        for (int request_number = 0; request_number < 2; ++request_number)
        {
            WFFacilities::WaitGroup wait_group(1);
            WFHttpTask *client_task = create_http_task("file");
            client_task->set_callback([&wait_group, start, end](WFHttpTask *task)
            {
                verify_response(task, generate_file_content(), start, end);
                wait_group.done();
            });
            client_task->start();
            wait_group.wait();
            EXPECT_EQ(cache.size(), generate_file_content().size());
        }

        svr.stop();
        cache.clear();
    }

    static void verify_response(WFHttpTask *task,
                                const std::string &file_body,
                                size_t start,
                                size_t end)
    {
        const void *body = nullptr;
        size_t body_len = 0;
        HttpResponse *resp = task->get_resp();
        resp->get_parsed_body(&body, &body_len);
        HttpHeaderMap header(resp);
        EXPECT_EQ(header.get("Content-Type"), "text/plain");

        using SignedSize = std::make_signed<size_t>::type;
        const size_t max_absolute =
            static_cast<size_t>(std::numeric_limits<SignedSize>::max());
        size_t expected_start = start;
        if (start > max_absolute)
            expected_start = file_body.size() - (~start + 1);
        size_t expected_end = end == static_cast<size_t>(-1)
                                  ? file_body.size()
                                  : std::min(end, file_body.size());

        const bool partial = expected_start != 0 ||
                             expected_end != file_body.size();
        EXPECT_STREQ(resp->get_status_code(), partial ? "206" : "200");
        if (partial)
        {
            EXPECT_EQ(header.get("Content-Range"),
                      "bytes " + std::to_string(expected_start) + "-" +
                      std::to_string(expected_end - 1) + "/" +
                      std::to_string(file_body.size()));
        }
        else
        {
            EXPECT_TRUE(header.get("Content-Range").empty());
        }

        std::string actual_body;
        if (body_len > 0)
            actual_body.assign(static_cast<const char *>(body), body_len);
        EXPECT_EQ(actual_body,
                  file_body.substr(expected_start, expected_end - expected_start));
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

TEST(HttpServer, file_range_end_is_clamped_to_eof)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 95, 200);
    FileTest::delete_file(path);
}

TEST(HttpServer, full_file_has_no_content_range)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 0, -1);
    FileTest::delete_file(path);
}

TEST(HttpServer, empty_file_is_served_as_full_content)
{
    std::string path = "./empty.txt";
    FileTest::create_file(path, "");
    FileTest::process(path, 0, -1, [](WFHttpTask *task) {
        const void *body = nullptr;
        size_t body_len = 0;
        HttpResponse *resp = task->get_resp();
        resp->get_parsed_body(&body, &body_len);
        HttpHeaderMap header(resp);
        EXPECT_STREQ(resp->get_status_code(), "200");
        EXPECT_TRUE(header.get("Content-Range").empty());
        EXPECT_EQ(body_len, 0U);
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, empty_file_partial_range_is_invalid)
{
    std::string path = "./empty.txt";
    FileTest::create_file(path, "");
    FileTest::process(path, 0, 1, [](WFHttpTask *task) {
        HttpResponse *resp = task->get_resp();
        HttpHeaderMap header(resp);
        EXPECT_STREQ(resp->get_status_code(), "416");
        EXPECT_EQ(header.get("Content-Range"), "bytes */0");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, cached_range_miss_and_hit_are_identical)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process_cached_twice(path, 10, 20);
    FileTest::delete_file(path);
}

TEST(HttpServer, disabled_cache_delegates_to_ranged_file_path)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileCache& cache = FileCache::instance();
    cache.clear();
    cache.disable();

    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);
    svr.GET("/file", [&path](const HttpReq *, HttpResp *resp)
    {
        resp->CachedFile(path, 10, 20);
    });
    ASSERT_EQ(svr.start("127.0.0.1", 8888), 0);

    WFHttpTask *client_task = FileTest::create_http_task("file");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        FileTest::verify_response(task, FileTest::generate_file_content(), 10, 20);
        wait_group.done();
    });
    client_task->start();
    wait_group.wait();
    EXPECT_EQ(cache.size(), 0U);

    svr.stop();
    cache.enable();
    FileTest::delete_file(path);
}

TEST(HttpServer, file_range_above_int_max)
{
    if (sizeof(size_t) < 8 ||
        std::numeric_limits<off_t>::max() <= std::numeric_limits<int>::max())
    {
        return;
    }

    const std::string path = "./large-sparse.txt";
    const size_t file_size =
        static_cast<size_t>(std::numeric_limits<int>::max()) + 4096;
    const size_t start = file_size - 4;
    int fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(ftruncate(fd, static_cast<off_t>(file_size)), 0);
    ASSERT_EQ(pwrite(fd, "tail", 4, static_cast<off_t>(start)), 4);
    ASSERT_EQ(close(fd), 0);

    FileTest::process(path, start, -1, [file_size, start](WFHttpTask *task) {
        const void *body = nullptr;
        size_t body_len = 0;
        HttpResponse *resp = task->get_resp();
        resp->get_parsed_body(&body, &body_len);
        HttpHeaderMap header(resp);
        EXPECT_STREQ(resp->get_status_code(), "206");
        EXPECT_EQ(header.get("Content-Range"),
                  "bytes " + std::to_string(start) + "-" +
                  std::to_string(file_size - 1) + "/" +
                  std::to_string(file_size));
        ASSERT_EQ(body_len, 4U);
        EXPECT_EQ(std::string(static_cast<const char *>(body), body_len), "tail");
    });
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
        EXPECT_STREQ(resp->get_status_code(), "416");
        EXPECT_EQ(header.get("Content-Range"), "bytes */100");
        std::string body_str(static_cast<const char *>(body), body_len);
        Json js = Json::parse(body_str);
        EXPECT_EQ(js["errmsg"].get<std::string>(), "File Range Invalid");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, file_suffix_before_start_is_invalid)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, -101, -1, [](WFHttpTask *task) {
        HttpResponse *resp = task->get_resp();
        HttpHeaderMap header(resp);
        EXPECT_STREQ(resp->get_status_code(), "416");
        EXPECT_EQ(header.get("Content-Range"), "bytes */100");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, file_start_at_eof_is_invalid)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 100, -1, [](WFHttpTask *task) {
        HttpResponse *resp = task->get_resp();
        HttpHeaderMap header(resp);
        EXPECT_STREQ(resp->get_status_code(), "416");
        EXPECT_EQ(header.get("Content-Range"), "bytes */100");
    });
    FileTest::delete_file(path);
}

TEST(HttpServer, negative_file_end_other_than_eof_is_invalid)
{
    std::string path = "./test.txt";
    FileTest::create_file(path);
    FileTest::process(path, 0, -2, [](WFHttpTask *task) {
        HttpResponse *resp = task->get_resp();
        HttpHeaderMap header(resp);
        EXPECT_STREQ(resp->get_status_code(), "416");
        EXPECT_EQ(header.get("Content-Range"), "bytes */100");
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
