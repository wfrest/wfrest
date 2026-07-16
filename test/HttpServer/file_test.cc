#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <csignal>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <limits>
#include <sys/resource.h>
#include <sys/stat.h>
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

    static std::string read_file(const std::string &path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }

    static std::string response_body(WFHttpTask *task)
    {
        const void *body = nullptr;
        size_t body_len = 0;
        if (!task->get_resp()->get_parsed_body(&body, &body_len) ||
            body_len == 0)
        {
            return "";
        }
        return std::string(static_cast<const char *>(body), body_len);
    }

    static void expect_file_write_error(WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        EXPECT_STREQ(task->get_resp()->get_status_code(), "503");
        const Json error = Json::parse(response_body(task));
        ASSERT_TRUE(error.is_valid());
        EXPECT_EQ(error["errmsg"].get<std::string>(), "File Write Error");
    }

    static void process(const std::string &path,
                        size_t start,
                        size_t end,
                        const std::function<void(WFHttpTask *task)> &callback = nullptr)
    {
        HttpServer svr;
        WFFacilities::WaitGroup wait_group(1);

        svr.GET("/file", [&path, start, end](const HttpReq *, HttpResp *resp)
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
    svr.GET("/file", [&path, &file_body](const HttpReq *, HttpResp *resp,
                                         SeriesWork *series)
    {
        resp->Save(path, file_body);
        series->set_callback([&path, &file_body](const SeriesWork *) {
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

    client_task->set_callback([&wait_group](WFHttpTask *)
    {
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}

TEST(HttpServer, save_file_replaces_existing_contents)
{
    const std::string short_path = "./wfrest-save-short.tmp";
    const std::string empty_path = "./wfrest-save-empty.tmp";
    const std::string moved_path = "./wfrest-save-moved.tmp";
    std::remove(moved_path.c_str());
    FileTest::create_file(short_path, "new-old-tail");
    FileTest::create_file(empty_path, "must-disappear");
    std::atomic<int> moved_callback_count{0};
    std::atomic<int> moved_callback_fd{0};
    std::atomic<bool> moved_callback_buffer_ok{false};

    HttpServer server;
    WFFacilities::WaitGroup wait_group(3);
    server.GET("/short", [&short_path](const HttpReq *, HttpResp *resp)
    {
        const std::string content = "new";
        resp->Save(short_path, content, "saved");
    });
    server.GET("/empty", [&empty_path](const HttpReq *, HttpResp *resp)
    {
        resp->Save(empty_path, std::string(), "saved");
    });
    server.GET("/moved", [&](const HttpReq *, HttpResp *resp)
    {
        std::string content = "moved-content";
        resp->Save(moved_path, std::move(content),
                   [&](const FileIOArgs *args)
        {
            moved_callback_fd.store(args->fd);
            moved_callback_buffer_ok.store(
                args->count == 13 &&
                std::string(static_cast<const char *>(args->buf),
                            args->count) == "moved-content");
            moved_callback_count.fetch_add(1);
        });
    });
    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    for (const char *route : {"short", "empty"})
    {
        WFHttpTask *task = FileTest::create_http_task(route);
        task->set_callback([&wait_group](WFHttpTask *current)
        {
            EXPECT_EQ(current->get_state(), WFT_STATE_SUCCESS);
            EXPECT_STREQ(current->get_resp()->get_status_code(), "200");
            EXPECT_EQ(FileTest::response_body(current), "saved");
            wait_group.done();
        });
        task->start();
    }

    WFHttpTask *moved_task = FileTest::create_http_task("moved");
    moved_task->set_callback([&wait_group](WFHttpTask *current)
    {
        EXPECT_EQ(current->get_state(), WFT_STATE_SUCCESS);
        EXPECT_STREQ(current->get_resp()->get_status_code(), "200");
        EXPECT_TRUE(FileTest::response_body(current).empty());
        wait_group.done();
    });
    moved_task->start();

    wait_group.wait();
    server.stop();
    EXPECT_EQ(FileTest::read_file(short_path), "new");
    EXPECT_TRUE(FileTest::read_file(empty_path).empty());
    EXPECT_EQ(FileTest::read_file(moved_path), "moved-content");
    EXPECT_EQ(moved_callback_count.load(), 1);
    EXPECT_EQ(moved_callback_fd.load(), -1);
    EXPECT_TRUE(moved_callback_buffer_ok.load());
    FileTest::delete_file(short_path);
    FileTest::delete_file(empty_path);
    FileTest::delete_file(moved_path);
}

TEST(HttpServer, save_file_reports_open_and_device_failures)
{
    const std::string missing_parent =
        "./wfrest-missing-save-parent-" + std::to_string(getpid());
    const std::string missing_path = missing_parent + "/output.tmp";
    rmdir(missing_parent.c_str());
    const bool has_dev_full = access("/dev/full", W_OK) == 0;
    std::atomic<int> callback_count{0};
    std::atomic<int> callback_fd{0};
    std::atomic<size_t> callback_count_arg{0};
    std::atomic<bool> callback_buffer_ok{false};

    HttpServer server;
    WFFacilities::WaitGroup wait_group(has_dev_full ? 2 : 1);
    server.GET("/missing", [&](const HttpReq *, HttpResp *resp)
    {
        resp->Save(missing_path, std::string("data"),
                   [&](const FileIOArgs *args)
        {
            callback_fd.store(args->fd);
            callback_count_arg.store(args->count);
            callback_buffer_ok.store(
                std::string(static_cast<const char *>(args->buf),
                            args->count) == "data");
            callback_count.fetch_add(1);
        });
    });
    server.GET("/full", [](const HttpReq *, HttpResp *resp)
    {
        resp->Save("/dev/full", std::string("data"), "must-not-appear");
    });
    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *missing_task = FileTest::create_http_task("missing");
    missing_task->set_callback([&wait_group](WFHttpTask *current)
    {
        FileTest::expect_file_write_error(current);
        wait_group.done();
    });
    missing_task->start();

    if (has_dev_full)
    {
        WFHttpTask *full_task = FileTest::create_http_task("full");
        full_task->set_callback([&wait_group](WFHttpTask *current)
        {
            FileTest::expect_file_write_error(current);
            EXPECT_EQ(FileTest::response_body(current).find("must-not-appear"),
                      std::string::npos);
            wait_group.done();
        });
        full_task->start();
    }

    wait_group.wait();
    server.stop();
    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_EQ(callback_fd.load(), -1);
    EXPECT_EQ(callback_count_arg.load(), 4U);
    EXPECT_TRUE(callback_buffer_ok.load());
    EXPECT_FALSE(FileUtil::file_exists(missing_path));
}

TEST(HttpServer, save_file_rejects_short_writes)
{
    struct rlimit original_limit;
    if (getrlimit(RLIMIT_FSIZE, &original_limit) != 0 ||
        original_limit.rlim_max < 5)
    {
        GTEST_SKIP() << "RLIMIT_FSIZE cannot be set to five bytes";
    }

    const std::string path = "./wfrest-save-short-write.tmp";
    std::remove(path.c_str());
    HttpServer server;
    WFFacilities::WaitGroup wait_group(1);
    server.GET("/short-write", [&path](const HttpReq *, HttpResp *resp)
    {
        resp->Save(path, std::string("0123456789"), "must-not-appear");
    });
    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    const auto original_handler = std::signal(SIGXFSZ, SIG_IGN);
    struct rlimit limited = original_limit;
    limited.rlim_cur = 5;
    if (setrlimit(RLIMIT_FSIZE, &limited) != 0)
    {
        std::signal(SIGXFSZ, original_handler);
        server.stop();
        GTEST_SKIP() << "RLIMIT_FSIZE update failed";
    }

    WFHttpTask *task = FileTest::create_http_task("short-write");
    task->set_callback([&wait_group](WFHttpTask *current)
    {
        FileTest::expect_file_write_error(current);
        EXPECT_EQ(FileTest::response_body(current).find("must-not-appear"),
                  std::string::npos);
        wait_group.done();
    });
    task->start();
    wait_group.wait();

    const int restore_limit = setrlimit(RLIMIT_FSIZE, &original_limit);
    std::signal(SIGXFSZ, original_handler);
    server.stop();
    EXPECT_EQ(restore_limit, 0);

    struct stat file_stat;
    ASSERT_EQ(stat(path.c_str(), &file_stat), 0);
    EXPECT_EQ(file_stat.st_size, 5);
    FileTest::delete_file(path);
}
